#ifndef MOCK_NVM_H
#define MOCK_NVM_H
#include "voyager.h"

typedef struct {
  voyager_error_E write_error;
  voyager_error_E read_error;
  voyager_bootloader_app_crc_t app_crc;
  voyager_bootloader_addr_size_t app_start_address;
  voyager_bootloader_addr_size_t app_end_address;
  voyager_bootloader_addr_size_t app_reset_vector_address;
  voyager_bootloader_app_size_t app_size;
  voyager_bootloader_verify_flash_before_jumping_t verify_flash_before_jumping;
} mock_nvm_data_t;

mock_nvm_data_t *mock_nvm_get_data(void);

#endif // MOCK_NVM_H
