#ifndef _CCASK_KV_H
#define _CCASK_KV_H

#include <stddef.h>
#include <inttypes.h>

typedef struct ccask_kv ccask_kv;

// initializers / destructors
ccask_kv* ccask_kv_init(ccask_kv* kv, uint32_t key_size, uint32_t value_size, const uint8_t* key, const uint8_t* value);
ccask_kv* ccask_kv_new(uint32_t key_size, uint32_t value_size, const uint8_t* key, const uint8_t* value);
void ccask_kv_destroy(ccask_kv* kv);
void ccask_kv_delete(ccask_kv* kv);

// serialize/deserialize
uint8_t* ccask_kv_serialize(uint8_t* dest, const ccask_kv* src);
ccask_kv* ccask_kv_deserialize(uint32_t key_size, uint32_t value_size, ccask_kv* dest, uint8_t* src);

// accessors
uint8_t* ccask_kv_value(uint8_t* dest, const ccask_kv* src);
uint8_t* ccask_kv_key(uint8_t* dest, const ccask_kv* src);
uint32_t ccask_kv_ksz(const ccask_kv* src);
uint32_t ccask_kv_vsz(const ccask_kv* src);

// print/debug
void ccask_kv_print(ccask_kv* kv);

#endif
