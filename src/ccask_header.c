#include "ccask_header.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>

struct ccask_header {
    time_t timestamp;
    uint32_t crc;
    uint32_t key_size;
    uint32_t value_size;
};

size_t HEADER_SIZE = sizeof(ccask_header);
size_t HEADER_BYTES = sizeof(time_t) + 3*sizeof(uint32_t);

ccask_header* ccask_header_init(ccask_header* c, uint32_t crc, time_t timestamp, uint32_t key_size, uint32_t value_size) {
    if (c) {
        *c = (ccask_header) {
            .crc = crc,
            .timestamp = timestamp,
            .key_size = key_size,
            .value_size = value_size,
        };
    }

    return c;
}

ccask_header* ccask_header_new(uint32_t crc, time_t timestamp, uint32_t key_size, uint32_t value_size) {
    return ccask_header_init(malloc(sizeof(ccask_header)), crc, timestamp, key_size, value_size);
}

void ccask_header_destroy(ccask_header* c) {
    if (c) {
        ccask_header_init(c, 0, 0, 0, 0);
    }
}

void ccask_header_delete(ccask_header* c) {
    ccask_header_destroy(c);
    free(c);
}

uint8_t* split_uint32_bytewise(uint8_t* dest, uint32_t src) {
    if (dest) {
        dest[3] = (src & 0x000000ff);
        dest[2] = (src & 0x0000ff00) >> 8;
        dest[1] = (src & 0x00ff0000) >> 16;
        dest[0] = (src & 0xff000000) >> 24;
    }

    return dest;
}

uint32_t combine_uint8_to32(uint8_t* src) {
    return src[0] | (src[1] << 8) | (src[2] << 16) | (src[3]) << 24;
}

time_t combine_uint8_totimet(uint8_t* src) {
    size_t tsz = sizeof(time_t);
    time_t acc = 0;
    for (size_t i = 0; i < tsz; ++i) {
        size_t offset = 8 * i;
        acc |= src[i] << offset;
    }

    return acc;
}

uint8_t* ccask_header_serialize(uint8_t* dest, const ccask_header* src) {
    if (!dest) return 0;

    if(!split_uint32_bytewise(dest, src->crc)) return 0;
    if(!split_uint32_bytewise(dest+4, src->timestamp)) return 0;
    if(!split_uint32_bytewise(dest+8, src->key_size)) return 0;
    if(!split_uint32_bytewise(dest+12, src->value_size)) return 0;

    return dest;
}


ccask_header* ccask_header_deserialize(ccask_header* dest, uint8_t* data) {
    if (!dest) return 0;

    size_t index = 0;
    dest->crc = combine_uint8_to32(data);
    index += 4;
    dest->timestamp = combine_uint8_totimet(data+index);
    index += sizeof(time_t);
    dest->key_size = combine_uint8_to32(data+index);
    index += 4;
    dest->value_size = combine_uint8_to32(data+index);

    return dest;
}

// accessors

/**@brief return the timestamp of the ccask_header src if it is initialized*/
time_t ccask_header_timestamp(const ccask_header* src) {
    if (!src) return 0;

    return src->timestamp;
}

/**@brief return the crc of the ccask_header src if it is initialized*/
uint32_t ccask_header_crc(const ccask_header* src) {
    if (!src) return 0;

    return src->crc;
}

/**@brief return the key_size of the ccask_header src if it is initialized*/
uint32_t ccask_header_ksz(const ccask_header* src) {
    if (!src) return 0;

    return src->key_size;
}

/**@brief return the value_size of the ccask_header src if it is initialized*/
uint32_t ccask_header_vsz(const ccask_header* src) {
    if (!src) return 0;

    return src->value_size;
}

// print/debug

void ccask_header_print(const ccask_header* src) {
    if (!src) return;

    printf("{\n\tcrc: %x\n\ttimestamp: %zu\n\tkey_size: %x\n\tvalue_size: %x\n}\n",
           src->crc,
           src->timestamp,
           src->key_size,
           src->value_size);
}
