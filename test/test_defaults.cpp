#include "test_defaults.hpp"

const voyager_bootloader_config_t default_test_config{
    .jump_to_app_after_dfu_recv_complete = true,
    .custom_crc_stream = NULL,
};
