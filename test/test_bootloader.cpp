#include "CppUTest/TestHarness.h"

extern "C" {
#include "voyager.h"
#include "voyager_private.h"
}

// create a test group
TEST_GROUP(test_bootloader_state_machine){

};

// create a test for that test group
TEST(test_bootloader_state_machine, test_bootloader_is_off_after_init) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

// Check that an external request to put the bootloader into DFU mode keeps the
// state in IDLE
TEST(test_bootloader_state_machine,
     test_bootloader_is_off_after_external_request) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
  // Ensure that the bootloader is still off after an external request
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Check that the request is put inside the voyager data struct
  CHECK_EQUAL(VOYAGER_REQUEST_ENTER_DFU, voyager_private_get_data()->request);
}