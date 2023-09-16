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

#define VOYAGER_DFU_ACK_MESSAGE_SIZE 8U

typedef struct {
  voyager_bootloader_state_E state;
  voyager_bootloader_request_E request;
  bool app_failed_crc_check;
  bool valid_start_request_received;

  uint8_t message_buffer[VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE];
  uint8_t packet_size;
  bool pending_data; // note: this flag acts as a de-facto mutex
  bool packet_overrun;

  size_t app_size_cached; // Used to prevent the app size from being read from
                          // NVM multiple times

  voyager_dfu_error_E dfu_error;

  uint8_t ack_message_buffer[VOYAGER_DFU_ACK_MESSAGE_SIZE];

  uint8_t dfu_sequence_number;
  voyager_bootloader_app_size_t bytes_written;
} voyager_data_t;

typedef struct {
  uint8_t message_id;
} voyager_message_header_t;

typedef struct {
  voyager_message_header_t header;
  union {
    struct {
      uint32_t app_size; // NOTE: only 3 bytes wide!
      uint32_t app_crc;
    } start_packet_data;
    struct {
      uint8_t sequence_number;
      uint8_t *payload;
      size_t payload_size;
    } data_packet_data;
  };
} voyager_message_t;

typedef union {
  void (*func)(void);
  voyager_bootloader_nvm_data_t addr;
} reset_vector_U;

typedef enum {
  VOYAGER_MESSAGE_ID_UNKNOWN = 0,
  VOYAGER_MESSAGE_ID_START,
  VOYAGER_MESSAGE_ID_ACK,
  VOYAGER_MESSAGE_ID_DATA,
} voyager_message_id_E;

/**
 * @brief voyager_private_calculate_crc Calculates the CRC of the application
 * @param app_start_address The start address of the application
 * @param app_size The size of the application
 * @return The CRC of the application as a uint32_t
 */
voyager_bootloader_app_crc_t
voyager_private_calculate_crc(const void *buffer, const size_t app_size);

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
voyager_private_run_state(const voyager_bootloader_state_E state);

/**
 * @brief voyager_private_generate_ack_message Generates an ACK message
 * @param error The error code to send in the ACK message
 * @param metadata The metadata to send in the ACK message
 * @param message_buffer The buffer to store the ACK message in
 * @param message_size The size of the ACK message buffer
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_private_generate_ack_message(
    const voyager_dfu_error_E error, uint8_t metadata[4],
    uint8_t *const message_buffer, size_t const message_size);

/**
 * @brief voyager_private_unpack_message Unpacks a message
 * @param message_buffer The buffer containing the message
 * @param message_size The size of the message buffer
 * @return The unpacked message
 */
voyager_message_t voyager_private_unpack_message(uint8_t *const message_buffer,
                                                 size_t const message_size);

/**
 * @brief voyager_private_init_dfu Initializes the DFU bootloader by erasing
 * flash
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_private_init_dfu(void);

/**
 * @brief voyager_private_process_start_packet Processes a start packet
 * @param message The message to process
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 * @note This function populates the acknowledgement message buffer, but does
 * NOT send an ack message
 */
voyager_error_E
voyager_private_process_start_packet(const voyager_message_t *const message);

/**
 * @brief voyager_private_process_data_packet Processes a data packet
 * @param message The message to process
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 * @note This function populates the acknowledgement message buffer, but does
 * NOT send an ack message
 */
voyager_error_E
voyager_private_process_data_packet(const voyager_message_t *const message);

/**
 * @brief voyager_private_verify_flash Verifies the flash memory of the MCU
 * @param result The result of the flash verification
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_private_verify_flash(bool *const result);

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