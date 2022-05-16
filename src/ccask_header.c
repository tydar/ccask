#include "ccask_header.h"
#include <stddef.h>

struct ccask_header {
	uint32_t crc;
	uint32_t timestamp;
	uint32_t key_size;
	uint32_t value_size;
};

HEADER_SIZE = sizeof(ccask_header);

ccask_header* ccask_header_init(ccask_header* c, uint32_t crc, uint32_t timestamp, uint32_t key_size, uint32_t value_size) {
	return 0;
}

ccask_header* ccask_header_new(uint32_t crc, uint32_t timestamp, uint32_t key_size, uint32_t value_size) {
	return 0;
}

ccask_header* ccask_header_destroy(ccask_header* c) {
	return 0;
}

ccask_header* ccask_header_delete(ccask_header* c) {
	return 0;
}

uint8_t* ccask_header_serialize(uint8_t* dest, const ccask_header* src) {
	return 0;
}

ccask_header* ccask_header_deserialize(ccask_header* dest, const uint8_t* data) {
	return 0;
}
