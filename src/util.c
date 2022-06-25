#include "util.h"

uint8_t* u32_to_nwk_byte_arr(uint8_t* dest, uint32_t src) {
	if (!dest) return 0;

	uint32_t nwko = htonl(src);
	
	dest[0] = src & (0xFF << 24);
	dest[1] = src & (0xFF << 16);
	dest[2] = src & (0xFF << 8);
	dest[3] = src & (0xFF);
	
	return dest;
}
