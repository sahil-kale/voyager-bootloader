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
TEST_GROUP(test_bootloader_api){

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

// Test that a nullptr'ed config returns an error
TEST(test_bootloader_api, test_init_nullptr_is_rejected) {
    CHECK_EQUAL(VOYAGER_ERROR_INVALID_ARGUMENT, voyager_bootloader_init(NULL));
}

// The below is not implemented as a mock due to the function pointer implementation
static const voyager_bootloader_app_crc_t test_crc_constant = 0xDEADBEEF;

void custom_crc_implementation(const uint8_t byte, voyager_bootloader_app_crc_t *const crc) {
    (void)byte;
    *crc = test_crc_constant;
}

// Test that a custom CRC implementation can be overriden with the function pointer
TEST(test_bootloader_api, test_custom_crc_implementation) {
    // Make a struct with a custom CRC function pointer
    voyager_bootloader_config_t config_custom_crc{.jump_to_app_after_dfu_recv_complete = true,
                                                  .custom_crc_stream = custom_crc_implementation};

    voyager_bootloader_init(&config_custom_crc);

    voyager_bootloader_app_crc_t test_crc;
    voyager_private_calculate_crc_stream(1, &test_crc);
    CHECK_EQUAL(test_crc_constant, test_crc);
}
