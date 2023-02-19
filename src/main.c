#include <stdio.h>
#include <stdlib.h>

#include "crc.h"
#include "ccask_kv.h"
#include "ccask_keydir.h"
#include "ccask_db.h"
#include "ccask_server.h"
#include "ccask_config.h"

int main(void) {
    ccask_config* cfg = ccask_config_from_env();
    if (!cfg) {
        fprintf(stderr, "ccask: failure to configure\n");
        return 1;
    }

    crc_init();
    ccask_db* db = ccask_db_new("./ccask_file", cfg);

    if (db == NULL) {
        fprintf(stderr, "error initializing ccask\n");
        exit(1);
    }

    /*
    uint8_t key[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t value[10] = { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 };

    uint8_t key2[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
    uint8_t value2[10] = { 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42 };

    db = ccask_db_set(db, 5, key, 10, value);
    db = ccask_db_set(db, 5, key2, 10, value2);

    ccask_get_result* res = ccask_db_get(db, 5, key);

    ccask_gr_print(res);
    ccask_gr_delete(res);

    res = ccask_db_get(db, 5, key2);

    ccask_gr_print(res);
    ccask_gr_delete(res);

    uint8_t key3[5] = { 0, 1, 2, 3, 4 };
    uint8_t value3[12] = {
        [3] = 0xFE,
        [7] = 0xEF,
        [9] = 0xDB,
    };

    db = ccask_db_set(db, 5, key3, 12, value3);

    res = ccask_db_get(db, 5, key3);
    ccask_gr_print(res);
    ccask_gr_delete(res);
    */

    ccask_server* srv = ccask_server_new(db, cfg);
    ccask_server_run(srv);

    ccask_db_delete(db);
    return EXIT_SUCCESS;
}
