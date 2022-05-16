#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include "crc.h"

uint32_t CRC_TABLE[256];

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

void crc_print_table() {
	for (size_t i = 0; i < 32; i++) {
		for (size_t j = 0; j < 8; j++) {
			printf("%X ", CRC_TABLE[i*8 + j]);
		}
	}
}

uint32_t crc_compute(const uint8_t* data, size_t data_length) {
	uint32_t crc32 = 0xFFFFFFFFu;

	for (size_t i = 0; i < data_length; i++) {
		const uint32_t lookupIndex = (crc32 ^ data[i]) & 0xff;
		crc32 = (crc32 >> 8) ^ CRC_TABLE[lookupIndex];
	}

	crc32 ^= 0xFFFFFFFFu;
	return crc32;
}
