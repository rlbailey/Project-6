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

TEST_F(FloppyTest, LoadFloppyTest) {
	char s[1024];

	for (size_t i = 0; i < 512; ++i)
	{
		sprintf(s + 2 * i, "%02X", floppy.bytes[i]);
	}

	ASSERT_STREQ("B8C0070520018ED0BC0010B8C0078ED8BE2200E89300BE4100E88D00BE6100E8870043532D444F53204F7065726174696E672053797374656D2076312E300D0A004372656174656420666F72204353333432332C2046616C6C20323031330D0A0028432920436F7079726967687420323031332062792052494348415244204241494C455920616E6420485545204D4F55412E20416C6C207269676874732072657365727665642E00B40EAC3C007404CD10EBF7C3000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000055AA", s);
}

class FATTest : public testing::Test {
protected:
	FATTest(void) { }
	~FATTest(void) { }
	virtual void SetUp(void) { }
	virtual void TearDown(void) { }

	Floppy floppy;
};

TEST_F(FATTest, DefaultConstructorTest) {
	EXPECT_EQ(floppy.bytes[512], 0xFF);
	EXPECT_EQ(floppy.bytes[513], 0xF);
	EXPECT_EQ(floppy.bytes[514], 0xF0);
	EXPECT_EQ(floppy.bytes[5120], 0xFF);
	EXPECT_EQ(floppy.bytes[5121], 0xF);
	EXPECT_EQ(floppy.bytes[5122], 0xF0);
}

class DirTest : public testing::Test {
protected:
	DirTest(void) { }
	~DirTest(void) { }
	virtual void SetUp(void)
	{
		floppy.rootDir.entries[0].initialize(&floppy, 0, "WHALE", "TXT", 0, 0, 0, 0, 0, 0, 0xF21, 0XB11, 0, (unsigned long)1193405);
		entry = floppy.rootDir.entries[0];
	}

	virtual void TearDown(void) { }

	Floppy floppy;
	Floppy::RootDir::Entry entry;
};

TEST_F(DirTest, FilenameTest) {
	EXPECT_STREQ("WHALE", (char*)entry.getFilename().c_str());
}

TEST_F(DirTest, ExtensionTest) {
	EXPECT_STREQ("TXT", (char*)entry.getExtension().c_str());
}

TEST_F(DirTest, FileSizeTest) {
	EXPECT_EQ(1193405, entry.fileSize);
}

TEST_F(DirTest, LastWriteDateTest) {
	EXPECT_EQ(0xB11, entry.lastWriteDate);
}

TEST_F(DirTest, LastWriteTimeTest) {
	EXPECT_EQ(0xF21, entry.lastWriteTime);
}

TEST_F(DirTest, ToDateTest) {
	EXPECT_STREQ("11-17-13", toDate(entry.lastWriteDate).c_str());
}

TEST_F(DirTest, ToTimeTest) {
	EXPECT_STREQ(" 3:33p", toTime(entry.lastWriteTime).c_str());
}

TEST_F(DirTest, CopyTest) {
	Floppy f;
	Floppy::RootDir::Entry *entry = &f.rootDir.entries[0];

	f.copy("CONSTITU.TXT");

	EXPECT_STREQ("CONSTITU", (char*)entry->getFilename().c_str());
	EXPECT_STREQ("TXT", (char*)entry->getExtension().c_str());
	EXPECT_EQ(1287, entry->fileSize);
	EXPECT_EQ(fromDate("11-19"), entry->lastWriteDate);
	EXPECT_EQ(fromDate("12-01"), entry->lastAccessDate);
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
