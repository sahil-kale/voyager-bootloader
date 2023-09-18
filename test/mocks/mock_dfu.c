#include "mock_dfu.h"

#include <CppUTestExt/MockSupport_c.h>

#include "voyager.h"
#include "voyager_private.h"

static uint8_t fake_flash[FAKE_FLASH_SIZE] = {0};

voyager_error_E voyager_bootloader_hal_jump_to_app(const voyager_bootloader_addr_size_t app_start_address) {
    mock_c()
        ->actualCall("voyager_bootloader_hal_jump_to_app")
        ->withUnsignedLongIntParameters("app_start_address", app_start_address);

    return (voyager_error_E)mock_c()->returnValue().value.intValue;
}

void mock_dfu_init(void) {
    // memcpy the fake flash data 0 to fake_flash
    memcpy(fake_flash, fake_flash_data_0, FAKE_FLASH_SIZE);
}

uint8_t *mock_dfu_get_flash(void) { return fake_flash; }

voyager_error_E voyager_bootloader_send_to_host(void const *const data, size_t len) {
    mock_c()->actualCall("voyager_bootloader_send_to_host")->withMemoryBufferParameter("data", data, len);

    return (voyager_error_E)mock_c()->returnValue().value.intValue;
}

voyager_error_E voyager_bootloader_hal_erase_flash(const voyager_bootloader_addr_size_t start_address,
                                                   const voyager_bootloader_addr_size_t end_address) {
    mock_c()
        ->actualCall("voyager_bootloader_hal_erase_flash")
        ->withUnsignedLongIntParameters("start_address", start_address)
        ->withUnsignedLongIntParameters("end_address", end_address);

    return (voyager_error_E)mock_c()->returnValue().value.intValue;
}

voyager_error_E voyager_bootloader_hal_write_flash(const voyager_bootloader_addr_size_t address, void const *const data,
                                                   size_t const length) {
    mock_c()
        ->actualCall("voyager_bootloader_hal_write_flash")
        ->withUnsignedLongIntParameters("address", address)
        ->withMemoryBufferParameter("data", data, length);

    return (voyager_error_E)mock_c()->returnValue().value.intValue;
}