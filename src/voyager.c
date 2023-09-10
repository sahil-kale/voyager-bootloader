#include "voyager.h"
#include "voyager_private.h"

static voyager_data_t voyager_data = {
    .state = VOYAGER_STATE_NOT_INITIALIZED,
};

voyager_error_E voyager_bootloader_init(void) {
  voyager_data.state = VOYAGER_STATE_IDLE;
  voyager_data.request = VOYAGER_REQUEST_KEEP_IDLE;
  return VOYAGER_ERROR_NONE;
}

voyager_bootloader_state_E voyager_bootloader_get_state(void) {
  return voyager_data.state;
}

voyager_error_E
voyager_bootloader_request(const voyager_bootloader_request_E request) {
  voyager_data.request = request;
  return VOYAGER_ERROR_NONE;
}

#ifdef UNIT_TEST
voyager_data_t *voyager_private_get_data(void) { return &voyager_data; }
#endif // UNIT_TEST
