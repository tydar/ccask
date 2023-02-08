#define _DEFAULT_SOURCE

#include "ccask_db.h"
#include "ccask_keydir.h"
#include "ccask_header.h"
#include "ccask_kv.h"
#include "crc.h"
#include "util.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**@file
 * @brief ccask_db.c implements useful DB operations (get, set, populate from file)
 */

// TODO: change commands to an enum
#define GET_CMD 0
#define SET_CMD 1

#define LOCKFILE_NAME "ccask.lock"

// Response formats

// struct defs

struct ccask_db {
    ccask_keydir* keydir;   // The keydir structure for this ccask instance

    // active file information
    size_t file_pos;        // Cursor pos in the file
    size_t file_id;
    size_t bytes_written;
    FILE* file;             // Pointer to the file we are writing

    // db dir information
    char* path;             // Path to the DB dir
    DIR* dir;               // Pointer to the DB file dir
    FILE* files[MAX_FILES]; // collection of all files in this ccask
};

struct ccask_get_result {
    uint32_t value_size;
    uint8_t* value;
    bool crc_passed;
};

struct ccask_result {
    response_type type;
    ccask_get_result* gr; // null if type != GET_SUCCESS
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

/**@brief getter for ccask_get_result value size*/
uint32_t ccask_gr_vsz(const ccask_get_result* gr) {
    return gr->value_size;
}

/**@brief given a non-null ccask_get_result* and a uint8_t* dest allocated to the appropriate size, return a copy of the value in dest*/
uint8_t* ccask_gr_val(uint8_t* dest, const ccask_get_result* src) {
    if (!dest || !src) return 0;

    memcpy(dest, src->value, src->value_size);

    return dest;
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


/* @brief ccask_gr_bytes puts a bytecode representation of a get request into buf
 *
 * @param[out] buf byte array of gr formatted as msgsz|result_type|vsz|value
 * */
uint32_t ccask_gr_bytes(ccask_get_result* gr, uint8_t* buf, size_t buflen) {
    if (gr == 0 || !gr->crc_passed) {
        // handle the error case
        char* errmsg = "No such key found or internal error";
        if (gr != 0 && !gr->crc_passed) errmsg = "CRC failed";
        uint32_t len = strlen(errmsg);
        uint32_t ressz = sizeof(uint32_t) + 1 + sizeof(uint32_t) + (sizeof(*errmsg) * len);

        if (ressz > buflen) return UINT32_MAX;

        uint32_t resszn = htonl(ressz);
        memcpy(buf, &resszn, sizeof(resszn));
        buf += sizeof(resszn);

        *buf = GET_FAIL;
        buf++;

        uint32_t lenn = htonl(len);
        memcpy(buf, &lenn, sizeof(lenn));
        buf += sizeof(lenn);

        memcpy(buf, errmsg, len);

        return ressz;
    } else {
        uint32_t ressz = sizeof(uint32_t) + 1 + sizeof(gr->value_size) + gr->value_size;
        if (ressz > buflen) return UINT32_MAX;

        uint32_t resszn = htonl(ressz);
        memcpy(buf, &resszn, sizeof(resszn));
        buf += sizeof(resszn);

        *buf = GET_SUCCESS;
        buf++;

        uint32_t vszn = htonl(gr->value_size);
        memcpy(buf, &vszn, sizeof(vszn));
        buf += sizeof(vszn);

        memcpy(buf, gr->value, gr->value_size);
        buf += gr->value_size;

        return ressz;
    }
}

//ccask_db functions

// internal ccask_db initialization APIs
// used to populate the keydir when the software starts iff a ccask file is present

/**@brief internal fn to populate next keydir entry*/
ccask_db* ccask_db_popnext(ccask_db* db, int index) {
    FILE* file = db->files[index];
    if (!file) return 0;

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
    ccask_kdrow* kdr = ccask_kdrow_new(ksz, key, index, vsz, pos, time(NULL));
    ccask_kdrow_print(kdr);
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

    errno = 0;
    struct dirent* pDirent;

    for (pDirent = readdir(db->dir); pDirent; pDirent = readdir(db->dir)) {
        if (strcmp(".", pDirent->d_name) == 0
                || strcmp("..", pDirent->d_name) == 0
                || strcmp(LOCKFILE_NAME, pDirent->d_name) == 0)

            continue;

        // we will treat every file in the directory as if it were a ccask file
        // TODO: introduce magic number or other file ID method
        size_t pathlen = strlen(db->path) + strlen(pDirent->d_name) + 1 + 1; // path len + filename len + / + \0
        char* path = malloc(pathlen);
        printf("file ID: %zu file name: %s\n", db->file_id, pDirent->d_name);

        path = strcpy(path, db->path);
        path = strcat(path, "/");
        path = strncat(path, pDirent->d_name, strlen(pDirent->d_name));
        if (!path) return 0;

        FILE *f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "fopen(%s,...)\n", path);
            perror("fopen");
            exit(1);
        }

        db->files[db->file_id] = f;
        db->file_id++;
    }

    if (errno != 0) {
        perror("readdir");
        exit(1);
    }


    for (size_t i = 0; i < MAX_FILES; i++) {
        if (!db->files[i]) break;

        ccask_db* iter = ccask_db_popnext(db, i);

        while(iter) {
            iter = ccask_db_popnext(db, i);
        }
    }

    return db;
}


/**@brief this function is registered with on_exit to ensure the lockfile is cleared in normal termination circumstances*/
void delete_lockfile(int status, void* lfpath) {
    if (lfpath == NULL) {
        fprintf(stderr, "ccask: failed to delete lockfile\n");
    }

    char* lfpath_s = (char*)lfpath;
    int res;

    errno = 0;

    res = remove(lfpath_s);
    if (res == -1) {
        fprintf(stderr, "ccask: failed to delete lockfile\n");
        perror("remove");
    }
}

#define DIR_LOCKED 0
#define DIR_LOCK_CREATED 1
#define DIR_ERROR 2
/*@brief this function is an internal API used by ccask_db_init to determine whether
 * 		 the ccask directory is already locked.
 *
 * 		 If the lockfile exists in the directory, return DIR_LOCKED
 *
 * 		 If the lockfile does not exist, create it and return DIR_LOCK_CREATED
 **/
int dir_lock(ccask_db* db) {
    if (db == NULL) return DIR_ERROR;

    char* fn;
    int res, fd;
    size_t fnlen;
    FILE* fp;

    pid_t p = getpid();
    mode_t m = S_IRUSR | S_IWUSR;
    errno = 0;

    fnlen = strlen(db->path) + strlen(LOCKFILE_NAME) + 2;

    fn = malloc(fnlen + 2);
    res = snprintf(fn, fnlen, "%s/%s", db->path, LOCKFILE_NAME);

    if (res < 0) return DIR_ERROR;

    fd = open(fn, O_WRONLY | O_CREAT | O_EXCL, m);
    if(fd == EEXIST) {
        return DIR_LOCKED;
    } else if (fd < 0) {
        return DIR_ERROR;
    }

    fp = fdopen(fd, "w");

    fprintf(fp, "%d", p);
    fflush(fp);
    fclose(fp);

    if(on_exit(delete_lockfile, fn) != 0) {
        delete_lockfile(0, (void*)fn);
        exit(1);
    }

    free(fn);
    return DIR_LOCK_CREATED;
}

ccask_db* ccask_db_init(ccask_db* db, const char* path, ccask_config* cfg) {
    if (db && path) {
        *db = (ccask_db) {
            .path = malloc(strlen(path)),
            .file_pos = 0,
            .file_id = 0,
            .bytes_written = 0,
            .keydir = ccask_keydir_new(ccask_config_kdsize(cfg), ccask_config_kdmax(cfg)),
            .file = 0,
            .dir = 0,
            .files = { 0 },
        };

        db->path = strncpy(db->path, path, strlen(path));
        DIR* dir = opendir(path);
        if (dir == 0) {
            // we were unable to open the dir
            if (errno == ENOENT && strcmp(db->path, "") != 0) {
                // directory does not exist, try to create it
                errno = 0;
                int res = mkdir(path, 0700);
                if (res == -1) {
                    perror("mkdir");
                    exit(1);
                }

                dir = opendir(path);
                if (dir == 0) {
                    perror("opendir");
                    exit(1);
                }
            } else {
                perror("opendir");
                exit(1);
            }
        }

        db->dir = dir;

        int res = dir_lock(db);

		if (res == DIR_LOCKED || res == DIR_ERROR) {
			fprintf(stderr, "ccask error: failure to obtain ccask lock. is a ccask instance running on this dir?\n");
			exit(1);
		}

        if (!ccask_db_populate(db)) *db = (ccask_db) {
            0
        };

        char* new_filename = malloc(strlen(path) + 1 + strlen(path) + MAX_FILE_CHARS);
        res = snprintf(new_filename, strlen(path) + strlen(path) + 1 + MAX_FILE_CHARS, "%s/%s_%zu", path, path, db->file_id);
        if (res < 0) {
            fprintf(stderr, "error constructing new filename\n");
            exit(1);
        }

        errno = 0;
        FILE* new_file = fopen(new_filename, "w+b");
        if (!new_file) {
            fprintf(stderr, "%s\n", new_filename);
            perror("fopen");
            exit(1);
        }

        printf("new file %s open for writing\n", new_filename);
        free(new_filename);

        db->file = new_file;
        db->files[db->file_id] = new_file;
    } else {
        *db = (ccask_db) {
            0
        };
    }

    return db;
}

ccask_db* ccask_db_new(const char* path, ccask_config* cfg) {
    ccask_db* db = malloc(sizeof(ccask_db));
    db = ccask_db_init(db, path, cfg);
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

/**@brief opens a new file for *db* or fails and quits the ccask process*/
void ccask_db_newfile(ccask_db* db) {
    db->file_id++;
    if (db->file_id == UINT32_MAX) {
        fprintf(stderr, "ccask_db: too many files\n");
        exit(1);
    }

    char* new_filename = malloc(strlen(db->path) + 1 + strlen(db->path) + MAX_FILE_CHARS);
    int res = snprintf(new_filename, strlen(db->path) + strlen(db->path) + 1 + MAX_FILE_CHARS, "%s/%s_%zu", db->path, db->path, db->file_id);
    if (res < 0) {
        fprintf(stderr, "error constructing new filename\n");
        exit(1);
    }

    errno = 0;
    FILE* new_file = fopen(new_filename, "w+b");
    if (!new_file) {
        fprintf(stderr, "%s\n", new_filename);
        perror("fopen");
        exit(1);
    }

    printf("ccask_db_newfile: new file %s open for writing\n", new_filename);

    db->file = new_file;
    db->files[db->file_id] = new_file;
    db->bytes_written = 0;

    free(new_filename);
}

size_t ccask_db_fid(const ccask_db* db) {
    if(!db) return SIZE_MAX;
    return db->file_id;
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

    // check to see if we have room in the file for this row
    // if not, we need to go to a new file
    if(db->bytes_written + row_size > MAX_FILE_BYTES || db->bytes_written + row_size < db->bytes_written) ccask_db_newfile(db);

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
    db->bytes_written += row_size;

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

    FILE* file = db->files[file_id];

    if (fseek(file, value_pos, SEEK_SET) != 0) {
        perror("seek error");
        errno = 0;
        return 0;
    }

    size_t row_size = HEADER_BYTES + value_size + key_size;

    uint8_t* row_ptr = malloc(row_size);
    size_t read_size = fread(row_ptr, row_size, 1, file);

    if (read_size < 1) {
        printf("read size: %zu\n", read_size);
        for (size_t i = 0; i < read_size*row_size; i++) {
            printf("0x%.02x ", row_ptr[i]);
        }
        puts("");
        free(row_ptr);

        int ferr = ferror(file);
        printf("ferror: %d\n", ferr);

        if(ferr) perror("fread");

        int fe = feof(file);
        printf("feof: %d\n", fe);

        int ft = ftell(file);
        printf("ftell: %d\n", ft);

        printf("file_id: %u\n", file_id);

        return 0;
    }

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

    ccask_gr_print(gr);

    ccask_kv_delete(kv);
    ccask_header_delete(hdr);
    free(row_ptr);
    free(value);

    return gr;
}

/**************
 *
 * ccask_net_result functions
 *
 * handle interpretation of queries
 *
 **************/

ccask_result* ccask_res_init(ccask_result* res, response_type type) {
    if (res) {
        // we always want to init res->gr to 0, since ccask_db_get will allocate memory for us
        *res = (ccask_result) {
            .type = type,
            .gr = 0,
        };
    } else {
        *res = (ccask_result) {
            0
        };
    }

    return res;
}

ccask_result* ccask_res_new(response_type type) {
    ccask_result* res = malloc(sizeof(ccask_result));
    return ccask_res_init(res, type);
}

void ccask_res_destroy(ccask_result* res) {
    if (res->gr) ccask_gr_delete(res->gr);
    *res = (ccask_result) {
        0
    };
}

void ccask_res_delete(ccask_result* res) {
    ccask_res_destroy(res);
    free(res);
}

void ccask_res_print(ccask_result* res) {
    printf("Type: %s\n", res->type == 0 ? "GET" : "SET");
    if (res->type == 0) {
        ccask_gr_print(res->gr);
    }
}

uint8_t ccask_res_type(const ccask_result* res) {
    if (res) return res->type;
    return 255;
}

uint8_t* ccask_res_value(uint8_t* dest, const ccask_result* res) {
    if (!res || !res->gr) return 0;

    return ccask_gr_val(dest, res->gr);
}

uint32_t ccask_res_vsz(const ccask_result* res) {
    if (!res || !res->gr) return UINT32_MAX;

    return ccask_gr_vsz(res->gr);
}

uint32_t ccask_sr_bytes(response_type rt, uint8_t* buf, size_t buflen) {
    if (rt == SET_SUCCESS) {
        char* msg = "SET succeeded";
        uint32_t len = strlen(msg);
        uint32_t msgsz = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + len;
        if (buflen < msgsz) return UINT32_MAX;

        uint32_t msgszn = htonl(msgsz);
        memcpy(buf, &msgszn, sizeof(msgszn));
        buf += sizeof(msgszn);

        *buf = SET_SUCCESS;
        buf++;

        uint32_t lenn = htonl(len);
        memcpy(buf, &lenn, sizeof(lenn));
        buf += sizeof(lenn);

        memcpy(buf, msg, len);
        return msgsz;
    } else if (rt == SET_FAIL) {
        char* msg = "SET failed";
        uint32_t len = strlen(msg);
        uint32_t msgsz = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + len;
        if (buflen < msgsz) return UINT32_MAX;

        uint32_t msgszn = htonl(msgsz);
        memcpy(buf, &msgszn, sizeof(msgszn));
        buf += sizeof(msgszn);

        *buf = SET_FAIL;
        buf++;

        uint32_t lenn = htonl(len);
        memcpy(buf, &lenn, sizeof(lenn));
        buf += sizeof(lenn);

        memcpy(buf, msg, len);
        return msgsz;
    }

    return UINT32_MAX;
}

uint32_t ccask_res_bytes(ccask_result* res, uint8_t* buf, size_t buflen) {
    if (buflen == 0 || !buf) return UINT32_MAX;

    switch(res->type)  {
    case GET_SUCCESS:
    case GET_FAIL:
        return ccask_gr_bytes(res->gr, buf, buflen);
    case SET_SUCCESS:
    case SET_FAIL:
        return ccask_sr_bytes(res->type, buf, buflen);
    case BAD_COMMAND:
    default:
        return UINT32_MAX;
    }
}

/**@brief given a byte array representing a query return a ccask_result representing the request or 0 if the request is invalid*/
ccask_result* ccask_query_interp(ccask_db* db, uint8_t* cmd) {
    if (!db || !cmd) return 0;
    // extract the message size:
    uint32_t msgsz = NWK_BYTE_ARR_U32(cmd);
    size_t index = 4;

    uint8_t cmd_byte = *(cmd+index);
    index += 1;

    uint32_t ksz = NWK_BYTE_ARR_U32((cmd+index));
    index += 4;
    uint32_t vsz = NWK_BYTE_ARR_U32((cmd+index));
    index += 4;


    uint8_t* key = 0;
    if (ksz > 0) {
        key = malloc(ksz);
        memcpy(key, cmd+index, ksz);
        index += ksz;
    }

    uint8_t* val = 0;
    if (vsz > 0) {
        val = malloc(vsz);
        memcpy(val, cmd+index, vsz);
        index += vsz;
    }


    ccask_get_result* gr = 0;
    response_type rt = BAD_COMMAND;
    switch(cmd_byte) {
    case GET_CMD:
        gr = ccask_db_get(db, ksz, key);
        rt = gr == 0 ? GET_FAIL : GET_SUCCESS;
        break;
    case SET_CMD:
        db = ccask_db_set(db, ksz, key, vsz, val);
        rt = db == 0 ? SET_FAIL : SET_SUCCESS;
        break;
    default:
        break;
    }

    ccask_result* res = ccask_res_new(rt);
    res->gr = gr;

    free(key);
    free(val);
    return res;
}
