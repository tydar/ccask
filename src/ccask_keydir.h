#ifndef _CCASK_KEYDIR_H
#define _CCASK_KEYDIR_H

#include <inttypes.h>
#include <stddef.h>
#include <time.h>

/**@file*/

// ccask_kdrow
typedef struct ccask_kdrow ccask_kdrow;
// ccask_kdrow init / delete
ccask_kdrow* ccask_kdrow_init(ccask_kdrow* kdr, uint32_t key_size, uint8_t* key,
                              uint32_t file_id, uint32_t value_size, uint32_t value_pos, time_t timestamp);
ccask_kdrow* ccask_kdrow_new(uint32_t key_size, uint8_t* key, uint32_t file_id,
                             uint32_t value_size, uint32_t value_pos, time_t timestamp);
void ccask_kdrow_destroy(ccask_kdrow* kdr);
void ccask_kdrow_delete(ccask_kdrow* kdr);

// attr accessors
uint32_t ccask_kdrow_fid(ccask_kdrow* kdr);
uint32_t ccask_kdrow_vsize(ccask_kdrow* kdr);
size_t ccask_kdrow_vpos(ccask_kdrow* kdr);


void ccask_kdrow_print(ccask_kdrow* kdr);

// ccask_keydir
typedef struct ccask_keydir ccask_keydir;

// ccask_keydir init / delete
ccask_keydir* ccask_keydir_init(ccask_keydir* kd, size_t size, size_t max_size);
ccask_keydir* ccask_keydir_new(size_t size, size_t max_size);
void ccask_keydir_destroy(ccask_keydir* kd);
void ccask_keydir_delete(ccask_keydir* kd);

// get/set
ccask_keydir* ccask_keydir_insert(ccask_keydir* kd, ccask_kdrow* elem);
ccask_kdrow* ccask_keydir_get(ccask_keydir* kd, uint32_t key_size, uint8_t* key);

// internal fns that we want to expose for testing only
#ifdef _TEST_
ccask_kdrow* ccask_kdrow_copy(ccask_kdrow* dest, const ccask_kdrow* src);
size_t KDROW_SIZE;
#endif

#endif
