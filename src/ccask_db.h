#ifndef _CCASK_DB_H
#define _CCASK_DB_H

#include <inttypes.h>
#include <stdbool.h>

#include "ccask_kv.h"
#include "ccask_config.h"

#define MAX_FILES 256
#define MAX_FILE_CHARS 4 // number of digits in MAX_FILES + 1 for \0
#define CCASK_MAGIC_NUMBER 0x0CCA2CFF

// if we are compiling tests, we want to have a small max-file-size for easier testing
//#define MAX_FILE_BYTES 1024
#define MAX_FILE_BYTES 512*(1024)*(1024) // 512 MB


enum response_type {
    GET_SUCCESS,
    GET_FAIL,
    SET_SUCCESS,
    SET_FAIL,
    BAD_COMMAND
};

typedef struct ccask_db ccask_db;
typedef struct ccask_get_result ccask_get_result;
typedef struct ccask_result ccask_result;
typedef enum response_type response_type;

// ccask_db functions

// initializer / destructors
ccask_db* ccask_db_init(ccask_db* db, const char* path, ccask_config* cfg);
ccask_db* ccask_db_new(const char* path, ccask_config* cfg);
void ccask_db_destroy(ccask_db* db);
void ccask_db_delete(ccask_db* db);

// get / set
ccask_db* ccask_db_set(ccask_db* db, uint32_t key_size, uint8_t* key, uint32_t value_size, uint8_t* value);
ccask_get_result* ccask_db_get(ccask_db* db, uint32_t key_size, uint8_t* key);

// getters
size_t ccask_db_fid(const ccask_db* db);

// ccask_get_result functions
ccask_get_result* ccask_gr_init(ccask_get_result* gr, uint32_t value_size, uint8_t* value, bool crc_passed);
ccask_get_result* ccask_gr_new(uint32_t value_size, uint8_t* value, bool crc_passed);
uint32_t ccask_gr_vsz(const ccask_get_result* gr);
uint8_t* ccask_gr_val(uint8_t* dest, const ccask_get_result* src);
void ccask_gr_destroy(ccask_get_result* gr);
void ccask_gr_delete(ccask_get_result* gr);
void ccask_gr_print(ccask_get_result* gr);
uint32_t ccask_gr_bytes(ccask_get_result* gr, uint8_t* buf, size_t buflen);
uint32_t ccask_res_bytes(ccask_result* res, uint8_t* buf, size_t buflen);

// query interp
ccask_result* ccask_res_init(ccask_result* res, response_type type);
ccask_result* ccask_res_new(response_type type);
void ccask_res_destroy(ccask_result* res);
void ccask_res_delete(ccask_result* res);
void ccask_res_print(ccask_result* res);

uint8_t ccask_res_type(const ccask_result* res);
uint8_t* ccask_res_value(uint8_t* dest, const ccask_result* res);
uint32_t ccask_res_vsz(const ccask_result* res);
ccask_result* ccask_query_interp(ccask_db* db, uint8_t* cmd);

#endif
