#ifndef VOYAGER_PRIVATE_H
#define VOYAGER_PRIVATE_H
#include "voyager.h"

typedef struct {

} voyager_data_t;

/**
 * @brief voyager_private_calculate_crc Calculates the CRC of the application
 * @param app_start_address The start address of the application
 * @param app_end_address The end address of the application
 * @return The CRC of the application as a uint32_t
 */
uint32_t voyager_private_calculate_crc(const uint32_t app_start_address,
                                       const uint32_t app_end_address);

#endif // VOYAGER_PRIVATE_H