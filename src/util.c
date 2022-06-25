#include "util.h"

uint8_t* u32_to_nwk_byte_arr(uint8_t* dest, uint32_t src) {
	if (!dest) return 0;

	dest[0] = (src & 0xFF000000) >> 24;
	dest[1] = (src & 0x00FF0000) >> 16;
	dest[2] = (src & 0x0000FF00) >> 8;
	dest[3] = (src & 0x000000FF);
	
	return dest;
}

uint8_t* u32_to_host_byte_arr(uint8_t* dest, uint32_t src) {
	if (!dest) return 0;
	
	if (htonl(src) == src) {
		// we're on a big endian machine, so put MSB in at index 0
		dest[0] = (src & 0xFF000000) >> 24;
		dest[1] = (src & 0x00FF0000) >> 16;
		dest[2] = (src & 0x0000FF00) >> 8;
		dest[3] = src & 0x000000FF;
	} else {
		// little endian; so LSB at index 0
		dest[0] = src & 0xFF;
		dest[1] = (src & 0x0000FF00) >> 8;
		dest[2] = (src & 0x00FF0000) >> 16;
		dest[3] = (src & 0xFF000000) >> 24;
	}
	
	return dest;
}
