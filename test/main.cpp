// test/main.cpp
#include "CppUTest/CommandLineTestRunner.h"

IMPORT_TEST_GROUP(dummy_test);
IMPORT_TEST_GROUP(test_bootloader_state_machine);
IMPORT_TEST_GROUP(test_dfu);
IMPORT_TEST_GROUP(test_bootloader_api);

int main(int ac, char **av) { return CommandLineTestRunner::RunAllTests(ac, av); }