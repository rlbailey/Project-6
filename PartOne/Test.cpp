/*
 * Google Test Framework
 * Sample Unit Tests
 */

#include <gtest/gtest.h>

using namespace std;

class StringTest : public testing::Test {
protected:
	StringTest(void);
	~StringTest(void);
	virtual void SetUp(void);
	virtual void TearDown(void);

	string s1;
	string *p1;
};

// Unit test with a fixture (i.e., a pre-created object.)
TEST_F(StringTest, CompareStringTest)
{
	// Stops running unit tests if this statement fails.
	ASSERT_STREQ("Hello World!", s1.c_str());
}

TEST_F(StringTest, LengthTest)
{
	// Continues running unit tests even if this statement fails.
	EXPECT_EQ(9, p1->length());
}

StringTest::StringTest(void) : s1("Hello World!"), p1(NULL) { }

StringTest::~StringTest(void) { }

// Set up fixtures ONCE for EACH unit test.
void StringTest::SetUp(void) {
	p1 = new string("Good-bye.");
}

// Clean up fixtures after EACH unit test is done running.
void StringTest::TearDown(void) {
	delete p1;
}

class BoolTest : public testing::Test
{

};

// Unit Test without a fixture.
TEST(BoolTest, TrueTest)
{
	ASSERT_TRUE(true);
}
