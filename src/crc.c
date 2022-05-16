#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include "crc.h"

uint32_t CRC_TABLE[256];

/**@brief crc_init sets up the lookup table for the CRC computation
 *
 * Adapted from code fragment 8: https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Generating_the_tables
 * */
void crc_init() {
    uint32_t crc = 1;
    CRC_TABLE[0] = 0;
    for (size_t i = 128; i > 0; i >>= 1) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xEDB88320; // this magic number is the crc polynomial reflected
        } else {
            crc >>= 1;
        }

        for (size_t j = 0; j < 255; j += 2*i) {
            CRC_TABLE[i + j] = crc ^ CRC_TABLE[j];
        }
    }
}

/**@brief crc_print_table prints the computed CRC_TABLE to stdout for review.
 *
 * **Must not** be called before crc_init()
 */
void crc_print_table() {
    for (size_t i = 0; i < 32; i++) {
        for (size_t j = 0; j < 8; j++) {
            printf("%X ", CRC_TABLE[i*8 + j]);
        }
    }
}

/**@brief crc_compute computes the CRC32 result for given data
 *
 * **MUST NOT** be called before crc_init()
 *
 * Adapted from https://en.wikipedia.org/wiki/Cyclic_redundancy_check#CRC-32_algorithm
 *
 * @param data an array of length data_length containing the data to compute the CRC value for
 * @param data_length the size of the data array
 * @return the CRC value as a uint32_t
 */
uint32_t crc_compute(const uint8_t* data, size_t data_length) {
    uint32_t crc32 = 0xFFFFFFFFu;

    for (size_t i = 0; i < data_length; i++) {
        const uint32_t lookupIndex = (crc32 ^ data[i]) & 0xff;
        crc32 = (crc32 >> 8) ^ CRC_TABLE[lookupIndex];
    }

    crc32 ^= 0xFFFFFFFFu;
    return crc32;
}
