#ifndef VOYAGER_PRIVATE_H
#define VOYAGER_PRIVATE_H
#include "voyager.h"
#include <stdbool.h>

#ifdef UNIT_TEST
#define VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE 64
#endif // UNIT_TEST

#ifndef VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE
#error "The VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE macro must be defined."
#else
// Assert that the VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE macro is >= 8
#if VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE < 8
#error "The VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE macro must be >= 8."
#endif // VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE < 8
#endif // VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE

typedef struct {
  voyager_bootloader_state_E state;
  voyager_bootloader_request_E request;
  bool app_failed_crc_check;
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

typedef union {
  void (*func)(void);
  voyager_bootloader_nvm_data_t addr;
} reset_vector_U;

typedef enum {
  VOYAGER_MESSAGE_ID_START = 0,
  VOYAGER_MESSAGE_ID_ACK,
  VOYAGER_MESSAGE_ID_DATA,
} voyager_message_id_E;

/**
 * @brief voyager_private_calculate_crc Calculates the CRC of the application
 * @param app_start_address The start address of the application
 * @param app_size The size of the application
 * @return The CRC of the application as a uint32_t
 */
uint32_t voyager_private_calculate_crc(uint8_t *buffer, const size_t app_size);

/**
 * @brief voyager_private_get_desired_state Gets the desired state of the
 * bootloader
 * @return The desired state of the bootloader
 */
voyager_bootloader_state_E voyager_private_get_desired_state(void);

/**
 * @brief voyager_private_set_desired_state current state to exit
 * @param current_state The current state of the bootloader
 * @param desired_state The desired state of the bootloader
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E
voyager_private_exit_state(const voyager_bootloader_state_E current_state,
                           const voyager_bootloader_state_E desired_state);

/**
 * @brief voyager_private_set_desired_state current state to enter
 * @param current_state The current state of the bootloader
 * @param desired_state The desired state of the bootloader
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E
voyager_private_enter_state(const voyager_bootloader_state_E current_state,
                            const voyager_bootloader_state_E desired_state);

/**
 * @brief voyager_private_run_state runs the desired state
 * @param state the state to run
 * @return VOYAGER_ERROR_NONE if successful, overwise an error code
 */
voyager_error_E
voyager_prviate_run_state(const voyager_bootloader_state_E state);

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