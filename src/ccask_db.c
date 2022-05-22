#include "ccask_db.h"
#include "ccask_keydir.h"
#include "ccask_header.h"
#include "ccask_kv.h"
#include "crc.h"

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


// TODO: make KEYDIR_SIZE configurable or at least think harder about a sensible value
#define KEYDIR_SIZE 1024

// struct defs

struct ccask_db {
    ccask_keydir* keydir; // The keydir structure for this ccask instance
    size_t file_pos;      // Cursor pos in the file
    size_t file_id;
    char* path;           // Path to the DB file
    FILE* file;           // Pointer to the file we are writing
};

struct ccask_get_result {
    uint32_t value_size;
    uint8_t* value;
    bool crc_passed;
};

//ccask_get_result functions
ccask_get_result* ccask_gr_init(ccask_get_result* gr, uint32_t value_size, uint8_t* value, bool crc_passed) {
    if (gr) {
        *gr = (ccask_get_result) {
            .value_size = value_size,
            .value = malloc(value_size),
            .crc_passed = crc_passed,
        };

        memcpy(gr->value, value, value_size);
    } else {
        *gr = (ccask_get_result) {
            0
        };
    }

    return gr;
}

ccask_get_result* ccask_gr_new(uint32_t value_size, uint8_t* value, bool crc_passed) {
    ccask_get_result* gr = malloc(sizeof(ccask_get_result));
    gr = ccask_gr_init(gr, value_size, value, crc_passed);
    return gr;
}

void ccask_gr_destroy(ccask_get_result* gr) {
    if (gr) {
        free(gr->value);
        *gr = (ccask_get_result) {
            0
        };
    }
}

void ccask_gr_delete(ccask_get_result* gr) {
    ccask_gr_destroy(gr);
    free(gr);
}

void ccask_gr_print(ccask_get_result* gr) {
    printf("Value size: %u\nValue:\t{\n", gr->value_size);
    for (uint32_t i = 0; i < gr->value_size; i++) {
        printf("\t%hhu\n", gr->value[i]);
    }
    printf("}\n");
}


//ccask_db functions

ccask_db* ccask_db_init(ccask_db* db, const char* path) {
    if (db && path) {
        *db = (ccask_db) {
            .path = malloc(strlen(path)),
            .file_pos = 0,
            .file_id = 0,
            .keydir = ccask_keydir_new(KEYDIR_SIZE),
            .file = 0,
        };

        //TODO: check errors on fopen
        db->path = strncpy(db->path, path, strlen(path));
        FILE* f = fopen(path, "wb+");
        if (f) db->file = f;
        if (!f) {
            perror("fopen");
            errno = 0;
        }
    } else {
        *db = (ccask_db) {
            0
        };
    }

    return db;
}

ccask_db* ccask_db_new(const char* path) {
    ccask_db* db = malloc(sizeof(ccask_db));
    db = ccask_db_init(db, path);
    return db;
}

void ccask_db_destroy(ccask_db* db) {
    if (db) {
        free(db->path);
        ccask_keydir_delete(db->keydir);
        fclose(db->file);
        *db = (ccask_db) {
            0
        };
    }
}

void ccask_db_delete(ccask_db* db) {
    ccask_db_destroy(db);
    free(db);
}

/**
 * set implementation
 * 1) calc crc
 * 2) at file_pos write crc timestamp keysize valuesize key value
 * 3) create and insert a keydir entry
 *
 * Keydir value_pos should be file_pos prior to write
 */
ccask_db* ccask_db_set(ccask_db* db, uint32_t key_size, uint8_t* key, uint32_t value_size, uint8_t* value) {
    time_t ts = time(NULL);

    // allocate a byte array large enough for the
    size_t row_size = sizeof(uint32_t) + sizeof(ts) + sizeof(key_size) + key_size + sizeof(value_size) + value_size;
    uint8_t* row = malloc(row_size);

    // copy each piece to the byte array
    // TODO: here is where we really want to use the *_serialize methods
    // but it doesn't really make sense since we need to construct most
    // of the row in order to calculate the CRC. Is there a better way to do this?
    size_t index = sizeof(uint32_t); // start offset from the CRC
    printf("Writing TS at index %zu\n", index);
    memcpy(row+index, &ts, sizeof(ts));
    index += sizeof(ts);
    printf("Writing KS at index %zu\n", index);
    memcpy(row+index, &key_size, sizeof(key_size));
    index += sizeof(key_size);
    printf("Writing VS at index %zu\n", index);
    memcpy(row+index, &value_size, sizeof(value_size));
    index += sizeof(value_size);
    printf("Writing K at index %zu\n", index);
    memcpy(row+index, key, key_size);
    index += key_size;
    printf("Writing V at index %zu\n", index);
    memcpy(row+index, value, value_size);

    // calc the crc -- assuming crc_init has been called elsewhere.
    uint32_t crc = crc_compute(row+sizeof(uint32_t), row_size-sizeof(uint32_t));
    memcpy(row, &crc, sizeof(crc));

    if (ftell(db->file) != db->file_pos) {
        if(!fseek(db->file, db->file_pos, SEEK_SET)) {
            perror("seek error");
            errno = 0;
            return 0;
        }
    }

    size_t n = fwrite(row, 1, row_size, db->file);

    // if we didn't write a full row, just return a null pointer.
    // we won't reset the file_pos or create a keydir entry.
    // so we will try to overwrite whatever we wrote with the next ccask_db_set
    // TODO: real error handling in this function so the caller can react appropriately
    if(n != row_size) return 0;

    size_t value_pos = db->file_pos;
    db->file_pos = ftell(db->file);

    // now create the keydir entry
    ccask_kdrow* kdr = ccask_kdrow_new(key_size, key, db->file_id, value_size, value_pos, ts);
    if(!ccask_keydir_insert(db->keydir, kdr)) return 0;

    ccask_kdrow_print(kdr);

    free(row);
    ccask_kdrow_delete(kdr); // I believe it is safe to do this bc ccask_keydir_insert ends up allocating its own memory.


    return db;
}

/**
 * get implementation
 * 1) try to get kdrow from keydir (ccask_keydir_get(...))
 * 2) from kdrow, we additionally get value_size, file_id, value_pos
 * 3) now from our file pointer, we seek to value_pos.
 * 4) read 32 bit crc
 * 5) read 32 bit tstamp, 32bit ksz, 32bit valuesz, key_size bytes key, value_size bytes value.
 * 6) calc crc of values from (5)
 * 7) compare read crc and calc'd crc
 * 8) return the value & crc result.
 */
ccask_get_result* ccask_db_get(ccask_db* db, uint32_t key_size, uint8_t* key) {
    if (!db) return 0;

    ccask_kdrow* kdr = ccask_keydir_get(db->keydir, key_size, key);
    if (!kdr) return 0;

    uint32_t file_id = ccask_kdrow_fid(kdr);
    uint32_t value_size = ccask_kdrow_vsize(kdr);
    size_t value_pos = ccask_kdrow_vpos(kdr);

    if (file_id == UINT32_MAX || value_size == UINT32_MAX || value_pos == SIZE_MAX) return 0;

    if (fseek(db->file, value_pos, SEEK_SET) != 0) {
        perror("seek error");
        errno = 0;
        return 0;
    }

    size_t row_size = HEADER_SIZE + value_size + key_size;

    uint8_t* row_ptr = malloc(row_size);
    size_t read_size = fread(row_ptr, row_size, 1, db->file);

    ccask_header* hdr = ccask_header_new(0, 0, 0, 0);
    if (!ccask_header_deserialize(hdr, row_ptr)) return 0;
    ccask_header_print(hdr);

    printf("Reading KV at index %zu\n", HEADER_SIZE);
    ccask_kv* kv = ccask_kv_new(0, 0, 0, 0);
    if (!ccask_kv_deserialize(key_size, value_size, kv, (row_ptr+HEADER_BYTES))) return 0;
    ccask_kv_print(kv);

    uint8_t* value = malloc(value_size);
    value = ccask_kv_value(value, kv);

    //TODO: implement CRC check on read
    ccask_get_result* gr = ccask_gr_new(value_size, value, true);

    ccask_kv_delete(kv);
    ccask_header_delete(hdr);
    free(row_ptr);

    return gr;
}
