#ifndef _CCASK_HEADER_H
#define _CCASK_HEADER_H

#include <stddef.h>
#include <inttypes.h>
#include <time.h>

typedef struct ccask_header ccask_header;
size_t HEADER_SIZE;
size_t HEADER_BYTES;


// initializers / destructors
ccask_header* ccask_header_init(ccask_header* c, uint32_t crc, time_t timestamp, uint32_t key_size, uint32_t value_size);
ccask_header* ccask_header_new(uint32_t crc, time_t timestamp, uint32_t key_size, uint32_t value_size);
void ccask_header_destroy(ccask_header* c);
void ccask_header_delete(ccask_header* c);

// serialize/deserialize
uint8_t* ccask_header_serialize(uint8_t* dest, const ccask_header* src);
ccask_header* ccask_header_deserialize(ccask_header* dest, uint8_t* data);

// accessors
time_t ccask_header_timestamp(const ccask_header* src);
uint32_t ccask_header_crc(const ccask_header* src);
uint32_t ccask_header_ksz(const ccask_header* src);
uint32_t ccask_header_vsz(const ccask_header* src);

// print/debug
void ccask_header_print(const ccask_header* src);

#endif
