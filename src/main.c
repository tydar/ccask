#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "crc.h"
#include "ccask_header.h"
#include "ccask_kv.h"
#include "ccask_keydir.h"
#include "ccask_db.h"

int main(void) {
    crc_init();

    ccask_db* db = ccask_db_new("./ccask_file");

    uint8_t key[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t value[10] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    db = ccask_db_set(db, 5, key, 10, value);

    ccask_get_result*  res = ccask_db_get(db, 5, key);

    ccask_gr_print(res);

    ccask_gr_delete(res);

    /*
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

    ccask_keydir* kd = ccask_keydir_new(256);
    uint8_t key2[3] = {  10, 11, 12 };

    ccask_kdrow* elem = ccask_kdrow_new(3, key2, 0, 5, 0, 0);
    kd = ccask_keydir_insert(kd, elem);

    ccask_kdrow* gotten = ccask_keydir_get(kd, 3, key2);
    ccask_kdrow_print(gotten);

    ccask_kdrow* elem2 = ccask_kdrow_new(3, key2, 0, 5, 0, 1);
    kd = ccask_keydir_insert(kd, elem2);

    gotten = ccask_keydir_get(kd, 3, key2);
    ccask_kdrow_print(gotten);

    uint8_t key3[3] = { 20, 21, 22 };
    ccask_kdrow* elem3 = ccask_kdrow_new(3, key3, 0, 5, 0, 0);
    kd = ccask_keydir_insert(kd, elem3);

    gotten = ccask_keydir_get(kd, 3, key3);
    ccask_kdrow_print(gotten);

    gotten = ccask_keydir_get(kd, 3, key2);
    ccask_kdrow_print(gotten);
    */

    return EXIT_SUCCESS;
}
