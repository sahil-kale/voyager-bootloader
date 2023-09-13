#include "CppUTest/TestHarness.h"

// create a test group
TEST_GROUP(dummy_test){

};

// create a test for that test group
TEST(dummy_test, pass_me) { CHECK_EQUAL(1, 1); }
