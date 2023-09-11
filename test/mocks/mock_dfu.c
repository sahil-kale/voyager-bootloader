#include "mock_dfu.h"
#include "voyager.h"
#include "voyager_private.h"
#include <CppUTestExt/MockSupport_c.h>

static uint8_t fake_flash[FAKE_FLASH_SIZE] = {0};

void mock_reset_vector(void) { mock_c()->actualCall("mock_reset_vector"); }

void mock_dfu_init(void) {
  // memcpy the fake flash data 0 to fake_flash
  memcpy(fake_flash, fake_flash_data_0, FAKE_FLASH_SIZE);
}

uint8_t *mock_dfu_get_flash(void) { return fake_flash; }