#ifndef VOYAGER_PRIVATE_H
#define VOYAGER_PRIVATE_H
#include "voyager.h"
#include <stdbool.h>

typedef struct {
  voyager_bootloader_state_E state;
  voyager_bootloader_request_E request;
} voyager_data_t;

typedef struct {
  uint8_t message_id;
} voyager_message_header_t;

typedef struct {
  voyager_message_header_t header;
  uint32_t app_size; // NOTE even though within the struct this data member is 4
                     // bytes, only 3 are transmitted over the data link layer!
  uint32_t app_crc;
} voyager_message_t;

typedef enum {
  VOYAGER_MESSAGE_ID_START = 0,
  VOYAGER_MESSAGE_ID_ACK,
  VOYAGER_MESSAGE_ID_DATA,
} voyager_message_id_E;

/**
 * @brief voyager_private_calculate_crc Calculates the CRC of the application
 * @param app_start_address The start address of the application
 * @param app_end_address The end address of the application
 * @return The CRC of the application as a uint32_t
 */
uint32_t voyager_private_calculate_crc(const uint32_t app_start_address,
                                       const uint32_t app_end_address);

#ifdef UNIT_TEST
/**
 * @brief voyager_private_get_data Gets a pointer of the voyager data struct
 * @return The voyager data struct pointer
 *
 * @note This function is used for unit testing only. It should not be used in
 * regular application code
 */
voyager_data_t *voyager_private_get_data(void);
#endif // UNIT_TEST

#endif // VOYAGER_PRIVATE_H