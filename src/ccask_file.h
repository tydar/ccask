#ifndef _CCASK_FILE_H
#define _CCASK_FILE_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "ccask_header.h"

typedef struct ccask_file ccask_file;

ccask_file* ccask_file_init(ccask_file* cf, const char* path, uint32_t fid);
ccask_file* ccask_file_new(const char* path, uint32_t fid);

void ccask_file_destroy(ccask_file* cf);
void ccask_file_delete(ccask_file* cf);

int ccask_file_get(ccask_file* cf, uint32_t value_size, size_t value_pos, uint32_t key_size, uint8_t* buf, size_t bufsz);
int ccask_file_set(ccask_file* cf, uint32_t value_size, uint8_t* value);

#endif
