/**
 * @file voyager.h
 * @brief This file contains the public API for the voyager bootloader
 *
 * See the README.md for more information on how to use this API
 *
 * See LICENCE.md for the license information for this library
 *
 * @author Sahil Kale
 */

#ifndef VOYAGER_H
#define VOYAGER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "voyager_cfg.h"  // Override with cfg_cfg_xxx.h

/// @brief Error codes for the voyager bootloader library
typedef enum {
    /// @brief The voyager library has not encountered any errors and is functioning as expected
    VOYAGER_ERROR_NONE = 0,
    /// @brief The voyager library has encountered an error due to an invalid argument
    VOYAGER_ERROR_INVALID_ARGUMENT,
    /// @brief The voyager library has encountered an error due to functionality that has not been implemented
    VOYAGER_ERROR_NOT_IMPLEMENTED,
    /// @brief The voyager library has encountered an error without a specific error code map.
    VOYAGER_ERROR_GENERIC_ERROR,
} voyager_error_E;

/// @brief Error codes for the voyager DFU subsystem
typedef enum {
    /// @brief The voyager DFU subsystem has not encountered any errors and is functioning as expected
    VOYAGER_DFU_ERROR_NONE = 0,
    /// @brief The voyager DFU subsystem has encountered an error due to unexpectedly receiving a packet while a pending packet
    /// had not been processed
    VOYAGER_DFU_ERROR_PACKET_OVERRUN,
    /// @brief The voyager DFU subsystem has encountered an error due to recieving a DFU start packet while the device did not
    /// request the library to enter DFU mode
    VOYAGER_DFU_ERROR_ENTER_DFU_NOT_REQUESTED,
    /// @brief The voyager DFU subsystem has encountered an error due to recieving a DFU data packet in an incorrect sequence
    VOYAGER_DFU_ERROR_OUT_OF_SEQUENCE,
    /// @brief The voyager DFU subsystem has encountered an error due to recieving a DFU data packet with an invalid message ID
    VOYAGER_DFU_ERROR_INVALID_MESSAGE_ID,
    /// @brief The voyager DFU subsystem has encountered an error due to recieving a DFU data packet with an invalid length
    VOYAGER_DFU_ERROR_SIZE_TOO_LARGE,
    /// @brief The voyager DFU subsystem has encountered an error due to an internal processing error
    VOYAGER_DFU_ERROR_INTERNAL_ERROR,
} voyager_dfu_error_E;

/// @brief The current state of the voyager bootloader
typedef enum {
    /// @brief The voyager bootloader is in an uninitialized state
    VOYAGER_STATE_NOT_INITIALIZED = 0,
    /// @brief The voyager bootloader is in an idle state and is available to enter DFU or enter the application
    VOYAGER_STATE_IDLE,
    /// @brief The voyager bootloader is performing a DFU and receiving data from the host
    VOYAGER_STATE_DFU_RECEIVE,
    /// @brief The voyager bootloader is attempting to jump to the application
    VOYAGER_STATE_JUMP_TO_APP,
} voyager_bootloader_state_E;

/**
 * @brief The keys for the NVM storage
 * @note if A/B partitioning is used later on, these keys will require separate
 * values for each partition Propose modifying the NVM function signatures to
 * specify the partition number and this partition number is required to be
 * passed in to the bootloader
 */
typedef enum {
    /// @brief The stored CRC of the application to check against to verify application flash integrity
    VOYAGER_NVM_KEY_APP_CRC = 0,
    /// @brief The start address of the application in flash
    VOYAGER_NVM_KEY_APP_START_ADDRESS,
    /// @brief The end address of the application in flash
    VOYAGER_NVM_KEY_APP_END_ADDRESS,
    /// @brief The size of the application in flash
    VOYAGER_NVM_KEY_APP_SIZE,
} voyager_nvm_key_E;

/// @brief The requests that can be made to the voyager bootloader
typedef enum {
    /// @brief Request the voyager bootloader to keep idle
    VOYAGER_REQUEST_KEEP_IDLE = 0,
    /// @brief Request the voyager bootloader to enter DFU mode
    VOYAGER_REQUEST_ENTER_DFU,
    /// @brief Request the voyager bootloader to jump to the application
    VOYAGER_REQUEST_JUMP_TO_APP,
} voyager_bootloader_request_E;

/// @brief Size of addresses in the voyager bootloader library
typedef uintptr_t voyager_bootloader_addr_size_t;
/// @brief Size of app crc in the voyager bootloader library
typedef uint32_t voyager_bootloader_app_crc_t;
/// @brief Size of app sizes in the voyager bootloader library
typedef uint32_t voyager_bootloader_app_size_t;

/// @brief Non-volatile memory data for the voyager bootloader library
typedef union {
    /// @brief The application CRC to check against to verify application flash integrity
    voyager_bootloader_app_crc_t app_crc;
    /// @brief The start address of the application in flash
    voyager_bootloader_addr_size_t app_start_address;
    /// @brief The end address of the application in flash
    voyager_bootloader_addr_size_t app_end_address;
    /// @brief The size of the application in flash
    voyager_bootloader_app_size_t app_size;
} voyager_bootloader_nvm_data_t;

/**
 * @brief voyager_custom_crc_stream_F Function pointer signature for a custom CRC stream function
 * @param byte The byte to process
 * @param crc pointer to the current CRC value that is expected to be modified
 */
typedef void (*voyager_custom_crc_stream_F)(const uint8_t byte, voyager_bootloader_app_crc_t *const crc);

/// @brief Configuration and feature flags for the voyager bootloader
typedef struct {
    /// @brief Controls whether the bootloader should jump to the application after receiving a complete DFU or return to IDLE
    bool jump_to_app_after_dfu_recv_complete;
    /// @brief Enables the use of a custom CRC stream function to calculate the CRC of the application
    voyager_custom_crc_stream_F custom_crc_stream;
} voyager_bootloader_config_t;

/** Primary Bootloader Functions **/
/**
 * @brief voyager_bootloader_init Resets and initializes the bootloader.
 * @param config pointer to configuration struct that stores various feature flags and call-outs
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note should be called on startup before any other bootloader functions
 */
voyager_error_E voyager_bootloader_init(voyager_bootloader_config_t const *const config);

/**
 * @brief voyager_bootloader_run Runs the bootloader
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note should be called on startup before any other bootloader functions
 */
voyager_error_E voyager_bootloader_run(void);

/**
 * @brief voyager_bootloader_process_receieved_packet Processes a packet
 * received from the host
 * @param data The data received from the host
 * @param length The length of the data received from the host
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note should be called when a packet from the DFU host device is received
 * with the relevant data
 * @note this function does perform error correction or verification - it
 * assumes the data link layer has already done this and the data is physically
 * valid.
 */
voyager_error_E voyager_bootloader_process_receieved_packet(uint8_t const *const data, size_t const length);

/**
 * @brief voyager_bootloader_get_state Gets the current state of the bootloader
 * @return The current state of the bootloader
 */
voyager_bootloader_state_E voyager_bootloader_get_state(void);

/**
 * @brief voyager_bootloader_external_request Processes an external request to
 * enter DFU mode or jump to the application
 * @param request The request to process
 * @return VOYAGER_ERROR_NONE if successfully received, otherwise an error code
 */
voyager_error_E voyager_bootloader_request(const voyager_bootloader_request_E request);

/** User Implemented Functions **/

/**
 * @brief voyager_bootloader_send_to_host Writes a desired packet to the DFU
 * host
 * @param data The data to write to the DFU host
 * @param len The length of the data to write to the DFU host
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 */
voyager_error_E voyager_bootloader_send_to_host(void const *const data, size_t len);

/**
 * @brief voyager_bootloader_nvm_write Writes a value to a given key in NVM
 * @param key The key to write to
 * @param data The data to write to the key
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note This function is called by the bootloader and is required to be
 * implemented by the application.
 */
voyager_error_E voyager_bootloader_nvm_write(const voyager_nvm_key_E key, voyager_bootloader_nvm_data_t const *const data);

/**
 * @brief voyager_bootloader_nvm_read Reads a value from a given key in NVM
 * @param key The key to read from
 * @param data The data to read from the key
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note This function is called by the bootloader and is required to be
 * implemented by the application.
 */
voyager_error_E voyager_bootloader_nvm_read(const voyager_nvm_key_E key, voyager_bootloader_nvm_data_t *const data);

/**
 * @brief voyager_bootloader_hal_erase_flash Erases the flash memory of the MCU
 * @param start_address The start address of the flash memory to erase
 * @param end_address The end address of the flash memory to erase
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note This function is called by the bootloader and is required to be
 * implemented by the application
 * @note The start and end addresses are provided for convinence, but the
 * bootloader presently assumes only 1 partition and will be equal to the values
 * returned by the NVM_read function.
 */
voyager_error_E voyager_bootloader_hal_erase_flash(const voyager_bootloader_addr_size_t start_address,
                                                   const voyager_bootloader_addr_size_t end_address);

/**
 * @brief voyager_bootloader_hal_write_flash Writes data to the flash memory of
 * the MCU
 * @param address The address of the flash memory to write to
 * @param data The data to write to the flash memory
 * @param length The length of the data to write to the flash memory
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note This function is called by the bootloader and is required to be
 * implemented by the application
 */
voyager_error_E voyager_bootloader_hal_write_flash(const voyager_bootloader_addr_size_t address, void const *const data,
                                                   size_t const length);

/**
 * @brief voyager_bootloader_hal_read_flash Reads data from the flash memory of
 * the MCU
 * @param address The address of the flash memory to read from
 * @param data The buffer to store the read data in
 * @param length The length of the data to read from the flash memory
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 * @note This function is called by the bootloader to verify the flash memory. The address is in one-byte increments.
 */
voyager_error_E voyager_bootloader_hal_read_flash(const voyager_bootloader_addr_size_t address, void *const data,
                                                  size_t const length);

/**
 * @brief voyager_bootloader_hal_jump_to_app Jumps to the application
 * @param app_start_address The start address of the application
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 * @note This function is called by the bootloader and is required to be implemented by the application
 */
voyager_error_E voyager_bootloader_hal_jump_to_app(const voyager_bootloader_addr_size_t app_start_address);

#endif  // VOYAGER_H
