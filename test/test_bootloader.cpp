#include <CppUTestExt/MockSupport.h>

#include "CppUTest/TestHarness.h"
#include "test_defaults.hpp"

extern "C" {
#include "mock_dfu.h"
#include "mock_nvm.h"
#include "voyager.h"
#include "voyager_private.h"
}

extern const voyager_bootloader_config_t default_test_config;

// create a test group
TEST_GROUP(test_bootloader_state_machine){

    void setup(){mock().clear();
mock_nvm_data_t *nvm_data = mock_nvm_get_data();

uint8_t *mock_app_start_address = mock_dfu_get_flash();
nvm_data->app_start_address = (uintptr_t)mock_app_start_address;
nvm_data->app_end_address = nvm_data->app_start_address + FAKE_FLASH_SIZE * 2;
nvm_data->app_size = FAKE_FLASH_SIZE;
mock_dfu_init();
}

void teardown() { mock().clear(); }
}
;

// create a test for that test group
TEST(test_bootloader_state_machine, test_bootloader_is_off_after_init) {
    // Ensure that the init function does not return an error
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init(&default_test_config));
    // Ensure that the bootloader is off after init
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

// Check that an external request to put the bootloader into DFU mode keeps the
// state in IDLE
TEST(test_bootloader_state_machine, test_bootloader_is_off_after_external_request) {
    // Ensure that the init function does not return an error
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init(&default_test_config));
    // Ensure that the bootloader is off after init
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
    // Ensure that the bootloader is still off after an external request
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

    // Check that the request is put inside the voyager data struct
    CHECK_EQUAL(VOYAGER_REQUEST_ENTER_DFU, voyager_private_get_data()->request);
}

// Test that a request to jump to the app with an invalid CRC does not jump to
// the app and instead stays in the idle state with no request
TEST(test_bootloader_state_machine, test_app_bad_crc_verify) {
    // get the NVM data struct
    mock_nvm_data_t *nvm_data = mock_nvm_get_data();
    // set the feature flag that enables flash verification
    nvm_data->app_crc = voyager_private_calculate_crc(mock_dfu_get_flash(), FAKE_FLASH_SIZE) + 1;

    // Ensure that the init function does not return an error
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init(&default_test_config));
    // Ensure that the bootloader is off after init
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
    // Ensure that the bootloader is still off after an external request
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_request(VOYAGER_REQUEST_JUMP_TO_APP));
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

    CHECK_EQUAL(VOYAGER_STATE_JUMP_TO_APP, voyager_private_get_desired_state());

    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

    // Check that the desired state is STATE_IDLE
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_private_get_desired_state());
    // Check that the request is set to VOYAGER_REQUEST_KEEP_IDLE
    CHECK_EQUAL(VOYAGER_REQUEST_KEEP_IDLE, voyager_private_get_data()->request);
}

// Test that a request to jump to an app with a valid CRC does jump to the app
// Test that a request to jump to the app with an invalid CRC does not jump to
// the app and instead stays in the idle state with no request
TEST(test_bootloader_state_machine, test_app_good_crc_verify) {
    // get the NVM data struct
    mock_nvm_data_t *nvm_data = mock_nvm_get_data();
    // set the feature flag that enables flash verification
    nvm_data->app_crc = voyager_private_calculate_crc(mock_dfu_get_flash(), FAKE_FLASH_SIZE);

    // Ensure that the init function does not return an error
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init(&default_test_config));
    // Ensure that the bootloader is off after init
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
    // Ensure that the bootloader is still off after an external request
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_request(VOYAGER_REQUEST_JUMP_TO_APP));
    CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

    CHECK_EQUAL(VOYAGER_STATE_JUMP_TO_APP, voyager_private_get_desired_state());

    // Check that the reset vector is called
    mock()
        .expectOneCall("voyager_bootloader_hal_jump_to_app")
        .withUnsignedLongLongIntParameter("app_start_address", nvm_data->app_start_address)
        .andReturnValue(VOYAGER_ERROR_NONE);
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());
    mock().checkExpectations();

    // Ensure that the voyager data request is set to VOYAGER_REQUEST_KEEP_IDLE
    CHECK_EQUAL(VOYAGER_REQUEST_KEEP_IDLE, voyager_private_get_data()->request);
}
