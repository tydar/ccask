#include "ccask_kv.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ccask_kv {
    uint32_t key_size;
    uint32_t value_size;
    uint8_t* key;
    uint8_t* value;
};

/**@brief Initialize a ccask key-value object
 *
 * The parameters *key* and *value* are copied. It is the callers responsibility to free the original key/value
 * objects. The internal key/value objects will be free'd when ccask_kv_delete is called.
 *
 * @param kv the uninitialized ccask_kv pointer to initialize
 * @param key_size the size of the key in bytes
 * @param value_size the size of the value in bytes
 * @param key a pointer to the first byte of the key.
 * @param value a pointer to the first byte of the value.
 */
ccask_kv* ccask_kv_init(ccask_kv* kv, uint32_t key_size, uint32_t value_size, const uint8_t* key, const uint8_t* value) {
    uint8_t* nkey = malloc(key_size);
    memcpy(nkey, key, key_size);

    uint8_t* nval = malloc(value_size);
    memcpy(nval, value, value_size);

    if (kv && nkey && nval) {
        *kv = (ccask_kv) {
            .key_size = key_size,
            .value_size = value_size,
            .key = nkey,
            .value = nval,
        };
    } else {
        *kv = (ccask_kv) {
            .key_size = 0,
            .value_size = 0,
            .key = 0,
            .value = 0,
        };
    }

    return kv;
}

ccask_kv* ccask_kv_new(uint32_t key_size, uint32_t value_size, const uint8_t* key, const uint8_t* value) {
    ccask_kv* kv = malloc(sizeof(ccask_kv));
    return ccask_kv_init(kv, key_size, value_size, key, value);
}

void ccask_kv_destroy(ccask_kv* kv) {
    if (kv) {
        free(kv->key);
        free(kv->value);
        *(kv) = (ccask_kv) {
            0
        };
    }
}

void ccask_kv_delete(ccask_kv* kv) {
    ccask_kv_destroy(kv);
    free(kv);
}

/**@brief serializes a ccask_kv object into a byte array
 *
 * @param dest the allocated byte array of size key_size + value_size
 * @param src the source ccask_kv pointer
 * @return a pointer to the populated byte array or a null pointer if an issue occurred
 */
uint8_t* ccask_kv_serialize(uint8_t* dest, const ccask_kv* src) {
    if (!dest || !src) return 0;

    memcpy(dest, src->key, src->key_size);
    memcpy(dest+src->key_size, src->value, src->value_size);

    return dest;
}

/**@brief deserializes a byte array into a ccask_kv object
 *
 * @param key_size the key size
 * @param value_size the value size
 * @param dest a 0-intialized ccask_kv obj
 * @param src a key_size + value_size sized byte array
 *
 * @return a pointer to the populated ccask_kv object or a null pointer if an issue occurred
 */
ccask_kv* ccask_kv_deserialize(uint32_t key_size, uint32_t value_size, ccask_kv* dest, uint8_t* src) {
    if (!dest || !src) return 0;

    dest->key_size = key_size;
    dest->value_size = value_size;

    uint8_t* key = malloc(key_size);
    uint8_t* value = malloc(value_size);
    memcpy(key, src, key_size);
    memcpy(value, src+key_size, value_size);

    dest->key = key;
    dest->value = value;

    return dest;
}

uint8_t* ccask_kv_value(uint8_t* dest, const ccask_kv* src) {
    if (!dest || !src) return 0;

    memcpy(dest, src->value, src->value_size);

    return dest;
}

/**@brief pretty-print the ccask_kv *kv* */
void ccask_kv_print(ccask_kv* kv) {
    printf("0x");
    for (size_t i = 0; i < kv->key_size; i++) {
        printf("%0.2X", kv->key[i]);
    }

    printf(" : 0x");

    for (size_t i = 0; i < kv->value_size; i++) {
        printf("%0.2X", kv->value[i]);
    }

    printf("\n");
}
