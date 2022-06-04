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
    printf("} CRC?: %s\n", gr->crc_passed ? "true" : "false");
}


//ccask_db functions

// internal ccask_db initialization APIs
// used to populate the keydir when the software starts iff a ccask file is present

/**@brief internal fn to populate next keydir entry*/
ccask_db* ccask_db_popnext(ccask_db* db) {
    FILE* file = db->file;
    size_t pos = ftell(file);
    uint8_t* hdrb = malloc(HEADER_BYTES);
    size_t hdr_n = fread(hdrb, 1, HEADER_BYTES, file);
    if (hdr_n < HEADER_BYTES) {
        free(hdrb);
        return 0;
    }

    ccask_header* hdr = ccask_header_new(0, 0, 0, 0);
    if (!ccask_header_deserialize(hdr, hdrb)) {
        free(hdrb);
        ccask_header_delete(hdr);
        return 0;
    }

    uint32_t ksz = ccask_header_ksz(hdr);
    uint32_t vsz = ccask_header_vsz(hdr);

    uint8_t* kvb = malloc(ksz + vsz);
    size_t kv_n = fread(kvb, 1, ksz+vsz, file);
    if (kv_n < ksz+vsz) {
        free(hdrb);
        free(kvb);
        ccask_header_delete(hdr);
        return 0; // read error TODO: handle
    }

    ccask_kv* kv = ccask_kv_new(0, 0, 0, 0);
    if (!ccask_kv_deserialize(ksz, vsz, kv, kvb)) {
        free(hdrb);
        free(kvb);
        ccask_header_delete(hdr);
        ccask_kv_delete(kv);
        return 0; // parse error TODO
    }

    uint8_t* key = malloc(ksz);
    key = ccask_kv_key(key, kv);

    // TODO: file ID
    ccask_kdrow* kdr = ccask_kdrow_new(ksz, key, 0, vsz, pos, time(NULL));
    if (!ccask_keydir_insert(db->keydir, kdr)) {
        free(key);
        free(hdrb);
        free(kvb);

        ccask_header_delete(hdr);
        ccask_kv_delete(kv);
        ccask_kdrow_delete(kdr);
        return 0; // keydir insert error TODO
    }

    free(hdrb);
    free(kvb);
    free(key);

    ccask_header_delete(hdr);
    ccask_kv_delete(kv);
    ccask_kdrow_delete(kdr);

    return db;
}

/**@brief given a malloc'd and initialized db and a valid file populates the keydir*/
ccask_db* ccask_db_populate(ccask_db* db) {
    if (!db) return 0;
    ccask_db* iter = ccask_db_popnext(db);

    while(iter) {
        iter = ccask_db_popnext(db);
    }

	if (fseek(db->file, 0, SEEK_SET) == -1) {
		perror("seek error");
		errno = 0;
	}

	db->file_pos = ftell(db->file);

    return db;
}

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

        //TODO: allow restarting the ccask process without clobbering the current file
        FILE* f = fopen(path, "ab+");

        if (!f) {
            perror("fopen");
            errno = 0;
        } else {
            // if the file existed with content we want to rewind to the start
            if (ftell(f) != 0) fseek(f, 0, SEEK_SET);
            db->file = f;
            if (!ccask_db_populate(db)) *db = (ccask_db) {
                0
            };
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
        //free(db->keydir);
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
    memcpy(row+index, &ts, sizeof(ts));
    index += sizeof(ts);
    memcpy(row+index, &key_size, sizeof(key_size));
    index += sizeof(key_size);
    memcpy(row+index, &value_size, sizeof(value_size));
    index += sizeof(value_size);
    memcpy(row+index, key, key_size);
    index += key_size;
    memcpy(row+index, value, value_size);

    // calc the crc -- assuming crc_init has been called elsewhere.
    uint32_t crc = crc_compute(row+sizeof(uint32_t), row_size-sizeof(uint32_t));
    memcpy(row, &crc, sizeof(crc));

    if (ftell(db->file) != db->file_pos) {
        if(fseek(db->file, db->file_pos, SEEK_SET) == -1) {
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

    free(row);
    ccask_kdrow_delete(kdr); // I believe it is safe to do this bc ccask_keydir_insert ends up allocating its own memory.


    return db;
}

/**@brief take each byte from a uint32_t and write it to the index in a uint8_t* */
// TODO: dedup this between here and ccask_header
uint8_t* split_uint32t_bytewise(uint8_t* dest, uint32_t src, size_t index) {
    if (!dest) return 0;

    dest[index] = (src & 0x000000ff);
    dest[index+1] = (src & 0x0000ff00) >> 8;
    dest[index+2] = (src & 0x00ff0000) >> 16;
    dest[index+3] = (src & 0xff000000) >> 24;

    return dest;
}

/**@brief take each byte from a time_t and write it to the index in a uint8_t*. */
uint8_t* split_timet_bytewise(uint8_t* dest, time_t src, size_t index) {
    if (!dest) return 0;

    size_t tsz = sizeof(time_t);
    uint8_t* ptr = dest + index;

    for (size_t i = tsz-1; i < tsz; --i) {
        time_t mask = 0xff << (i * 8);
        time_t offset = 8 * i;
        ptr[index+i] = (src & mask) >> offset;
    }

    return dest;
}

/**@brief given a header struct and key-value struct, return true if the CRC in the header
 * 		  matches the CRC computed for the header and key-value.
 */
bool crc_check(const ccask_header* hdr, const ccask_kv* kv) {
    if (!hdr || !kv) return false;

    time_t ts = ccask_header_timestamp(hdr);
    uint32_t ksz = ccask_header_ksz(hdr);
    uint32_t vsz = ccask_header_vsz(hdr);
    uint32_t crc = ccask_header_crc(hdr);

    if (ksz != ccask_kv_ksz(kv)) return false;
    if (vsz != ccask_kv_vsz(kv)) return false;

    uint8_t* key = malloc(ksz);
    uint8_t* val = malloc(vsz);
    key = ccask_kv_key(key, kv);
    val = ccask_kv_value(val, kv);

    // create a buffer that can hold everything but the CRC
    size_t row_size = HEADER_BYTES + ksz + vsz - sizeof(uint32_t);
    uint8_t* row_ptr = malloc(row_size);

    size_t index = 0;
    split_timet_bytewise(row_ptr, ts, index);
    index += sizeof(time_t);
    split_uint32t_bytewise(row_ptr, ccask_header_ksz(hdr), index);
    index += sizeof(uint32_t);
    split_uint32t_bytewise(row_ptr, ccask_header_vsz(hdr), index);
    index += sizeof(uint32_t);
    memcpy(row_ptr + index, key, ksz);
    index += ksz;
    memcpy(row_ptr + index, val, vsz);
    index += vsz;

    uint32_t computed_crc = crc_compute(row_ptr, row_size);

    free(row_ptr);
    free(key);
    free(val);

    if (computed_crc == crc) return true;

    return false;
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
    if (!kdr) {
        return 0;
    }

    uint32_t file_id = ccask_kdrow_fid(kdr);
    uint32_t value_size = ccask_kdrow_vsize(kdr);
    size_t value_pos = ccask_kdrow_vpos(kdr);

    if (file_id == UINT32_MAX || value_size == UINT32_MAX || value_pos == SIZE_MAX) {
        return 0;
    }

    if (fseek(db->file, value_pos, SEEK_SET) == -1) {
        perror("seek error");
        errno = 0;
        return 0;
    }

    size_t row_size = HEADER_SIZE + value_size + key_size;

    uint8_t* row_ptr = malloc(row_size);
    size_t read_size = fread(row_ptr, row_size, 1, db->file);

    ccask_header* hdr = ccask_header_new(0, 0, 0, 0);
    if (!ccask_header_deserialize(hdr, row_ptr)) {
        ccask_header_delete(hdr);

        free(row_ptr);
        return 0;
    }

    ccask_kv* kv = ccask_kv_new(0, 0, 0, 0);
    if (!ccask_kv_deserialize(key_size, value_size, kv, (row_ptr+HEADER_BYTES))) {
        ccask_header_delete(hdr);
        ccask_kv_delete(kv);

        free(row_ptr);
        return 0;
    }

    uint8_t* value = malloc(value_size);
    value = ccask_kv_value(value, kv);

    bool crc_flag = crc_check(hdr, kv);
    ccask_get_result* gr = ccask_gr_new(value_size, value, crc_flag);

    ccask_kv_delete(kv);
    ccask_header_delete(hdr);
    free(row_ptr);
    free(value);

    return gr;
}
