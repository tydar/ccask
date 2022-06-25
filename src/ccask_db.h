#ifndef _CCASK_DB_H
#define _CCASK_DB_H

#include <inttypes.h>
#include <stdbool.h>

#include "ccask_kv.h"

typedef struct ccask_db ccask_db;
typedef struct ccask_get_result ccask_get_result;
typedef struct ccask_result ccask_result;

// ccask_db functions

// initializer / destructors
ccask_db* ccask_db_init(ccask_db* db, const char* path);
ccask_db* ccask_db_new(const char* path);
void ccask_db_destroy(ccask_db* db);
void ccask_db_delete(ccask_db* db);

// get / set
ccask_db* ccask_db_set(ccask_db* db, uint32_t key_size, uint8_t* key, uint32_t value_size, uint8_t* value);
ccask_get_result* ccask_db_get(ccask_db* db, uint32_t key_size, uint8_t* key);

// ccask_get_result functions
ccask_get_result* ccask_gr_init(ccask_get_result* gr, uint32_t value_size, uint8_t* value, bool crc_passed);
ccask_get_result* ccask_gr_new(uint32_t value_size, uint8_t* value, bool crc_passed);
uint32_t ccask_gr_vsz(const ccask_get_result* gr);
uint8_t* ccask_gr_val(uint8_t* dest, const ccask_get_result* src);
void ccask_gr_destroy(ccask_get_result* gr);
void ccask_gr_delete(ccask_get_result* gr);
void ccask_gr_print(ccask_get_result* gr);

// query interp
ccask_result* ccask_res_init(ccask_result* res, uint8_t type);
ccask_result* ccask_res_new(uint8_t type);
void ccask_res_destroy(ccask_result* res);
void ccask_res_delete(ccask_result* res);
void ccask_res_print(ccask_result* res);

uint8_t ccask_res_type(const ccask_result* res);
uint8_t* ccask_res_value(uint8_t* dest, const ccask_result* res);
uint32_t ccask_res_vsz(const ccask_result* res);
ccask_result* ccask_query_interp(ccask_db* db, uint8_t* cmd);

#endif
