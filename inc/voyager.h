#ifndef VOYAGER_H
#define VOYAGER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    VOYAGER_ERROR_NONE = 0,
    VOYAGER_ERROR_INVALID_ARGUMENT,
    VOYAGER_ERROR_NOT_IMPLEMENTED,
    VOYAGER_ERROR_GENERIC_ERROR,
} voyager_error_E;

typedef enum {
    VOYAGER_DFU_ERROR_NONE = 0,
    VOYAGER_DFU_ERROR_PACKET_OVERRUN,
    VOYAGER_DFU_ERROR_ENTER_DFU_NOT_REQUESTED,
    VOYAGER_DFU_ERROR_OUT_OF_SEQUENCE,
    VOYAGER_DFU_ERROR_INVALID_MESSAGE_ID,
    VOYAGER_DFU_ERROR_SIZE_TOO_LARGE,
    VOYAGER_DFU_ERROR_INTERNAL_ERROR,
} voyager_dfu_error_E;

typedef enum {
    VOYAGER_STATE_NOT_INITIALIZED = 0,  // This state is only entered on startup and cleared once the
                                        // bootloader is initialized
    VOYAGER_STATE_IDLE,
    VOYAGER_STATE_DFU_RECEIVE,
    VOYAGER_STATE_JUMP_TO_APP,
} voyager_bootloader_state_E;

// Note: if A/B partitioning is used later on, these keys will require separate
// values for each partition Propose modifying the NVM function signatures to
// specify the partition number and this partition number is required to be
// passed in to the bootloader
typedef enum {
    VOYAGER_NVM_KEY_APP_CRC = 0,
    VOYAGER_NVM_KEY_APP_START_ADDRESS,
    VOYAGER_NVM_KEY_APP_END_ADDRESS,  // Used to ensure the app size does not
                                      // exceed the max bound
    VOYAGER_NVM_KEY_APP_SIZE,
} voyager_nvm_key_E;

typedef enum {
    VOYAGER_REQUEST_KEEP_IDLE = 0,  // Default request, keeping the request at none will prevent the
                                    // bootloader from doing anything
    VOYAGER_REQUEST_ENTER_DFU,
    VOYAGER_REQUEST_JUMP_TO_APP,
} voyager_bootloader_request_E;

typedef uintptr_t voyager_bootloader_addr_size_t;
typedef uint32_t voyager_bootloader_app_crc_t;
typedef uint32_t voyager_bootloader_app_size_t;
typedef bool voyager_bootloader_verify_flash_before_jumping_t;

typedef union {
    voyager_bootloader_app_crc_t app_crc;
    voyager_bootloader_addr_size_t app_start_address;
    voyager_bootloader_addr_size_t app_end_address;
    voyager_bootloader_app_size_t app_size;
    voyager_bootloader_verify_flash_before_jumping_t verify_flash_before_jumping;
} voyager_bootloader_nvm_data_t;

/** Primary Bootloader Functions **/
/**
 * @brief voyager_bootloader_init Resets and initializes the bootloader.
 * @return VOYAGER_ERROR_NONE if successful, otherwise an error code
 *
 * @note should be called on startup before any other bootloader functions
 */
voyager_error_E voyager_bootloader_init(void);

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
