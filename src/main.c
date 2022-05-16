#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "crc.h"
#include "ccask_header.h"
#include "ccask_kv.h"

int main(void) {
    crc_init();

    uint8_t data[3] = { 65, 66, 67 };
    uint32_t crc = crc_compute(data, 3);


    ccask_header* hdr = ccask_header_new(crc, 10, 10, 10);
    ccask_header_print(hdr);

    uint8_t* ser = malloc(HEADER_SIZE);
    ser = ccask_header_serialize(ser, hdr);

    printf("Serialized: [  ");
    for (size_t i = 0; i < HEADER_SIZE; i++) {
        printf("%x ", ser[i]);
    }
    printf("]\n");

    ccask_header* deser = ccask_header_new(0, 0, 0, 0);
    deser = ccask_header_deserialize(deser, ser);

    ccask_header_print(deser);

    uint8_t key[2] = { 0xA2, 2 };
    uint8_t val[5] = { 1, 2, 0xFF, 4, 5 };
    ccask_kv* kv = ccask_kv_new(2, 5, key, val);

    ccask_kv_print(kv);

    return EXIT_SUCCESS;
}
