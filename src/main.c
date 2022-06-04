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

    uint8_t key2[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
    uint8_t value2[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

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

    ccask_db_delete(db);

    return EXIT_SUCCESS;
}
