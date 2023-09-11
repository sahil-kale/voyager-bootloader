#include "mock_nvm.h"
#include "voyager.h"

static mock_nvm_data_t mock_nvm_data = {
    .write_error = VOYAGER_ERROR_NONE,
    .read_error = VOYAGER_ERROR_NONE,
};

mock_nvm_data_t *mock_nvm_get_data(void) { return &mock_nvm_data; }

voyager_error_E
voyager_bootloader_nvm_write(const voyager_nvm_key_E key,
                             voyager_bootloader_nvm_data_t const *const data) {
  voyager_error_E error = mock_nvm_data.write_error;
  switch (key) {
  case VOYAGER_NVM_KEY_APP_SIZE: {
    mock_nvm_data.app_size = data->app_size;
  } break;
  case VOYAGER_NVM_KEY_APP_CRC: {
    mock_nvm_data.app_crc = data->app_crc;
  } break;
  case VOYAGER_NVM_KEY_APP_START_ADDRESS: {
    mock_nvm_data.app_start_address = data->app_start_address;
  } break;
  case VOYAGER_NVM_KEY_APP_END_ADDRESS: {
    mock_nvm_data.app_end_address = data->app_end_address;
  } break;
  case VOYAGER_NVM_KEY_VERIFY_FLASH_BEFORE_JUMPING: {
    mock_nvm_data.verify_flash_before_jumping =
        data->verify_flash_before_jumping;
  } break;
  case VOYAGER_NVM_KEY_APP_RESET_VECTOR_ADDRESS: {
    mock_nvm_data.app_reset_vector_address = data->app_reset_vector_address;
  } break;
  default: {
    error = VOYAGER_ERROR_INVALID_ARGUMENT;
  } break;
  }

  return error;
}

voyager_error_E
voyager_bootloader_nvm_read(const voyager_nvm_key_E key,
                            voyager_bootloader_nvm_data_t *const data) {
  voyager_error_E error = mock_nvm_data.read_error;
  switch (key) {
  case VOYAGER_NVM_KEY_APP_SIZE: {
    data->app_size = mock_nvm_data.app_size;
  } break;
  case VOYAGER_NVM_KEY_APP_CRC: {
    data->app_crc = mock_nvm_data.app_crc;
  } break;
  case VOYAGER_NVM_KEY_APP_START_ADDRESS: {
    data->app_start_address = mock_nvm_data.app_start_address;
  } break;
  case VOYAGER_NVM_KEY_APP_END_ADDRESS: {
    data->app_end_address = mock_nvm_data.app_end_address;
  } break;
  case VOYAGER_NVM_KEY_VERIFY_FLASH_BEFORE_JUMPING: {
    data->verify_flash_before_jumping =
        mock_nvm_data.verify_flash_before_jumping;
  } break;
  case VOYAGER_NVM_KEY_APP_RESET_VECTOR_ADDRESS: {
    data->app_reset_vector_address = mock_nvm_data.app_reset_vector_address;
  } break;
  default: {
    error = VOYAGER_ERROR_INVALID_ARGUMENT;
  } break;
  }

  return error;
}