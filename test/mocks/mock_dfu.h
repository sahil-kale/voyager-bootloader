#ifndef MOCK_DFU_H
#define MOCK_DFU_H
#include <stdint.h>

#define FAKE_FLASH_SIZE (128U)

const uint8_t fake_flash_data_0[FAKE_FLASH_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
};

const uint8_t fake_flash_data_1[FAKE_FLASH_SIZE] = {
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
};

void mock_reset_vector(void);

void mock_dfu_init(void);

uint8_t *mock_dfu_get_flash(void);

#endif // MOCK_DFU_H
