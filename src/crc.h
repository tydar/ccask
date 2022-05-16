#ifndef _CRC_H
#define _CRC_H

#include <inttypes.h>
#include <stddef.h>

void crc_init();
void crc_print_table();

uint32_t crc_compute(const uint8_t* data, size_t data_length);

#endif
