#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "crc.h"

int main(void) {
	crc_init();
	crc_print_table();

	uint8_t data[3] = { 65, 66, 67 };
	uint32_t crc = crc_compute(data, 3);

	printf("CRC32 of [ 65, 66, 67 ]: %x\n", crc);

	return EXIT_SUCCESS;
}
