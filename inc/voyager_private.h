/**
 * @file voyager_private.h
 * @brief This file contains the private API for the voyager bootloader
 *
 * See LICENCE.md for the license information for this library
 *
 * @author Sahil Kale
 */

#ifndef VOYAGER_PRIVATE_H
#define VOYAGER_PRIVATE_H
#include <stdbool.h>

#include "voyager.h"

#ifndef VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE
#error "The VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE macro must be defined."
#else
#if VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE < 8
#error "The VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE macro must be >= 8."
#endif  // VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE < 8
#endif  // VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE

/// @brief The size of the ACK message
#define VOYAGER_DFU_ACK_MESSAGE_SIZE 8U

/// @brief Module data of the voyager bootloader
typedef struct {
    /// @brief Pointer to the bootloader configuration struct
    voyager_bootloader_config_t const *config;
    /// @brief Stores the current state of the bootloader
    voyager_bootloader_state_E state;
    /// @brief Stores the desired request of the bootloader
    voyager_bootloader_request_E request;
    /// @brief Stores the last thrown error code by the bootloader
    voyager_error_E error_latched;
    /// @brief Stores whether the application has failed the CRC check
    bool app_failed_crc_check;
    /// @brief Stores whether the bootloader received a valid DFU start request
    bool valid_dfu_start_request_received;

    /// @brief Stores the last received message in a private buffer
    uint8_t message_buffer[VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE];
    /// @brief Stores the size of the last received message
    uint32_t packet_size;
    /// @brief Stores whether there is pending data to be processed
    bool pending_data;
    /// @brief Stores whether the bootloader saw a packet overrun
    bool packet_overrun;

    /// @brief Stores the size of the application in a RAM cache to prevent multiple reads from NVM
    size_t app_size_cached;

    /// @brief Stores the last thrown error code by the bootloader's DFU subsystem
    voyager_dfu_error_E dfu_error;

    /// @brief Buffer for ack messages
    uint8_t ack_message_buffer[VOYAGER_DFU_ACK_MESSAGE_SIZE];

    /// @brief Stores the current sequence number of the DFU data packets
    uint8_t dfu_sequence_number;
    /// @brief Tracks the number of bytes written to flash in DFU mode
    voyager_bootloader_app_size_t bytes_written;
} voyager_data_t;

/*! \cond PRIVATE */
typedef struct {
    uint8_t message_id;
} voyager_message_header_t;

typedef struct {
    voyager_message_header_t header;
    union {
        struct {
            uint32_t app_size;  // NOTE: only 3 bytes wide!
            uint32_t app_crc;
        } start_packet_data;
        struct {
            uint8_t sequence_number;
            uint8_t *payload;
            size_t payload_size;
        } data_packet_data;
    } message_payload;
} voyager_message_t;
/*! \endcond */

/// @brief The message IDs for the voyager bootloader
typedef enum {
    /// @brief The message ID is unknown
    VOYAGER_MESSAGE_ID_UNKNOWN = 0,
    /// @brief DFU Start message ID
    VOYAGER_MESSAGE_ID_START,
    /// @brief DFU acknowledgement message ID
    VOYAGER_MESSAGE_ID_ACK,
    /// @brief DFU Data message ID
    VOYAGER_MESSAGE_ID_DATA,
} voyager_message_id_E;

/**
 * @brief voyager_private_calculate_crc Calculates the CRC of the application
 * @param buffer The buffer containing the packet to calculate the CRC of
 * @param app_size The size of the application
 * @return The CRC of the application as a uint32_t
 */
voyager_bootloader_app_crc_t voyager_private_calculate_crc(const void *buffer, const size_t app_size);

/**
 * @brief voyager_private_calculate_crc_stream Further calculates the CRC using the existing CRC tables and a single byte
 * @param byte The byte to add to the CRC
 * @param crc The CRC to add the byte to
 */
void voyager_private_calculate_crc_stream(const uint8_t byte, voyager_bootloader_app_crc_t *const crc);

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
voyager_error_E voyager_private_exit_state(const voyager_bootloader_state_E current_state,
                                           const voyager_bootloader_state_E desired_state);

/**
 * @brief voyager_private_set_desired_state current state to enter
 * @param current_state The current state of the bootloader
 * @param desired_state The desired state of the bootloader
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_private_enter_state(const voyager_bootloader_state_E current_state,
                                            const voyager_bootloader_state_E desired_state);

/**
 * @brief voyager_private_run_state runs the desired state
 * @param state the state to run
 * @return VOYAGER_ERROR_NONE if successful, overwise an error code
 */
voyager_error_E voyager_private_run_state(const voyager_bootloader_state_E state);

/**
 * @brief voyager_private_generate_ack_message Generates an ACK message
 * @param error The error code to send in the ACK message
 * @param metadata The metadata to send in the ACK message
 * @param message_buffer The buffer to store the ACK message in
 * @param message_size The size of the ACK message buffer
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_private_generate_ack_message(const voyager_dfu_error_E error, uint8_t metadata[4],
                                                     uint8_t *const message_buffer, size_t const message_size);

/**
 * @brief voyager_private_unpack_message Unpacks a message
 * @param message_buffer The buffer containing the message
 * @param message_size The size of the message buffer
 * @return The unpacked message
 */
voyager_message_t voyager_private_unpack_message(uint8_t *const message_buffer, size_t const message_size);

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
voyager_error_E voyager_private_process_start_packet(const voyager_message_t *const message);

/**
 * @brief voyager_private_process_data_packet Processes a data packet
 * @param message The message to process
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 * @note This function populates the acknowledgement message buffer, but does
 * NOT send an ack message
 */
voyager_error_E voyager_private_process_data_packet(const voyager_message_t *const message);

/**
 * @brief voyager_private_verify_flash Verifies the flash memory of the MCU
 * @param result The result of the flash verification
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_private_verify_flash(bool *const result);

/**
 * @brief voyager_private_pack_crc_into_buffer Packs a CRC into a buffer
 * @param buffer The buffer to pack the CRC into
 * @param crc The CRC to pack into the buffer
 */
void voyager_private_pack_crc_into_buffer(uint8_t buffer[4], const voyager_bootloader_app_crc_t crc);

/**
 * @brief voyager_private_run_idle_state Runs the idle state and associated action
 * @return voyager_error_E error code of the run action
 */
voyager_error_E voyager_private_run_idle_state(void);

/**
 * @brief voyager_private_run_dfu_receive_state Runs the DFU receive state and associated action
 * @return voyager_error_E error code of the run action
 */
voyager_error_E voyager_private_run_dfu_receive_state(void);

#ifdef VOYAGER_UNIT_TEST
/**
 * @brief voyager_private_get_data Gets a pointer of the voyager data struct
 * @return The voyager data struct pointer
 *
 * @note This function is used for unit testing only. It should not be used in
 * regular application code
 */
voyager_data_t *voyager_private_get_data(void);
#endif  // VOYAGER_UNIT_TEST

#endif  // VOYAGER_PRIVATE_H