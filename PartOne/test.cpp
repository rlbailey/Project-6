/*
 * Google Test Framework
 * Sample Unit Tests
 */

#include "partone.cpp"
#include "gtest/gtest.h"

class FloppyTest : public testing::Test {
protected:
	FloppyTest(void) { }
	~FloppyTest(void) { }
	virtual void SetUp(void) { }
	virtual void TearDown(void) { }

	Floppy floppy;
};

TEST_F(FloppyTest, DefaultConstructorTest) {
	char s[1024];

	// Test boot sector.
	for (size_t i = 0; i < 512; ++i)
	{
		sprintf(s + 2 * i, "%02X", floppy.bytes[i]);
	}

	ASSERT_STREQ("B8C0070520018ED0BC0010B8C0078ED8BE2200E89300BE4100E88D00BE6100E8870043532D444F53204F7065726174696E672053797374656D2076312E300D0A004372656174656420666F72204353333432332C2046616C6C20323031330D0A0028432920436F7079726967687420323031332062792052494348415244204241494C455920616E6420485545204D4F55412E20416C6C207269676874732072657365727665642E00B40EAC3C007404CD10EBF7C3000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000055AA", s);
}

class FATTest : public testing::Test {
protected:
	FATTest(void) : entries(floppy.fat.entries) { }
	~FATTest(void) { }
	virtual void SetUp(void) { }
	virtual void TearDown(void) { }

	Floppy floppy, copied, deleted;
	Floppy::FAT::Entry *entries;
};

TEST_F(FATTest, CopyTest) {
	floppy.copy("CONSTITU.TXT");

	EXPECT_EQ(3, *entries[2]);
	EXPECT_EQ(4, *entries[3]);
	EXPECT_EQ(0xFF8, *entries[4]);
}

TEST_F(FATTest, DeleteTest) {
	floppy.copy("CONSTITU.TXT");
	floppy.remove("CONSTITU.TXT");

	EXPECT_EQ(0, *entries[2]);
	EXPECT_EQ(0, *entries[3]);
	EXPECT_EQ(0, *entries[4]);
}

class DirTest : public testing::Test {
protected:
	DirTest(void) : entry(&floppy.rootDir.entries[0]) { }
	~DirTest(void) { }
	virtual void SetUp(void) { }

	virtual void TearDown(void) { }

	Floppy floppy;
	Floppy::RootDir::Entry *entry;
};

TEST_F(DirTest, CopyTest) {
	floppy.copy("CONSTITU.TXT");

	EXPECT_STREQ("CONSTITU", entry->getFilename().c_str());
	EXPECT_STREQ("TXT", entry->getExtension().c_str());
	EXPECT_STREQ("11-19-2013", toDate(*entry->lastAccessDate).c_str());
	EXPECT_STREQ(" 4:38:48p", toTime(*entry->lastWriteTime).c_str());
	EXPECT_STREQ("11-19-2013", toDate(*entry->lastWriteDate).c_str());
	EXPECT_EQ(2, *entry->firstLogicalSector);
	EXPECT_EQ(1287, *entry->fileSize);
}

TEST_F(DirTest, DeleteTest) {
	floppy.copy("CONSTITU.TXT");
	floppy.remove("CONSITU.TXT");

	EXPECT_EQ(0xE5, *entry->filename);
}

TEST_F(DirTest, RenameTest) {
	floppy.copy("CONSTITU.TXT");
	floppy.rename("CONSTITU.TXT", "DOCUMENT.LOG");

	EXPECT_STREQ("DOCUMENT", entry->getFilename().c_str());
	EXPECT_STREQ("LOG", entry->getExtension().c_str());
}

class StringTest : public testing::Test {
protected:
	StringTest(void) : s1("Hello World!"), p1(NULL) { }
	~StringTest(void) { }

	// Set up fixtures ONCE for EACH unit test.
	virtual void SetUp(void) {
		p1 = new std::string("Good-bye.");
	}

	// Clean up fixtures after EACH unit test is done running.
	virtual void TearDown(void) {
		delete p1;
	}

	std::string s1;
	std::string *p1;
};

// Unit test with a fixture (i.e., a pre-created object.)
TEST_F(StringTest, CompareStringTest) {
	// Stops running unit tests if this statement fails.
	ASSERT_STREQ("Hello World!", s1.c_str());
}

TEST_F(StringTest, LengthTest) {
	// Continues running unit tests even if this statement fails.
	EXPECT_EQ(9, p1->length());
}

class BoolTest : public testing::Test {

};

// Unit Test without a fixture.
TEST(BoolTest, TrueTest) {
	ASSERT_TRUE(true);
}
