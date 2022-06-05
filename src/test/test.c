#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define _TEST_

#include "crc.h"
#include "ccask_header.h"
#include "ccask_kv.h"
#include "ccask_keydir.h"
#include "ccask_db.h"

void test_kdrow(void) {
	puts("\t===== starting ccask_kdrow tests =====");
	uint32_t ksz = 5;
	uint32_t fid = 10;
	uint32_t vsz = 5;
	uint32_t vpos = 15;
	time_t time = 123456789;
	uint8_t key1[5] = { 0, 1, 2, 3, 4 };
	ccask_kdrow* kdr1 = ccask_kdrow_new(ksz, key1, fid, vsz, vpos, time);

	// asserts
	puts("kdr1 non-null after ccask_kdrow_new");
	assert(kdr1 != 0);
	puts("kdr1->fid == fid");
	assert(ccask_kdrow_fid(kdr1) == fid);
	puts("kdr1->vpos == vpos");
	assert(ccask_kdrow_vpos(kdr1) == vpos);
	puts("kdr1->vsize == vsz");
	assert(ccask_kdrow_vsize(kdr1) == vsz);

	ccask_kdrow* kdr2 = 0;
	puts("ccask_kdrow_copy(kdr2, kdr1) fails when kdr2 is null");
	assert(ccask_kdrow_copy(kdr2, kdr1) == 0);
	kdr2 = malloc(KDROW_SIZE);
	puts("ccask_kdrow_copy(kdr2, kdr1) does not fail after malloc");
	assert(ccask_kdrow_copy(kdr2, kdr1) != 0);
	puts("kdr2 != 0 after copy");
	assert(kdr2 != 0);

	puts("check that accessor fn return same value for kdr1 & kdr2 after copy");
	assert(ccask_kdrow_fid(kdr1) == ccask_kdrow_fid(kdr2));
	assert(ccask_kdrow_vpos(kdr1) == ccask_kdrow_vpos(kdr2));
	assert(ccask_kdrow_vsize(kdr1) == ccask_kdrow_vsize(kdr2));

	ccask_kdrow_delete(kdr2);

	puts("kdr1 still holds values after kdr2 delete");
	assert(ccask_kdrow_fid(kdr1) == fid);


	puts("\t===== completed ccask_kdrow tests =====");
}

void test_keydir(void) {
	puts("\t===== starting ccask_keydir tests =====");
	ccask_keydir* kd = 0;
	size_t kdsz = 64;

	puts("keydir non-null after ccask_keydir_new");
	kd = ccask_keydir_new(kdsz);
	assert(kd != 0);

	puts("keydir non-null after insert");
	ccask_kdrow* kdr = 0;
	uint8_t key[5] = { 0, 1, 2, 3, 4 };
	size_t vpos1 = 2;
	kdr = ccask_kdrow_new(5, key, 0, 0, vpos1, 0);

	assert(ccask_keydir_insert(kd, kdr) != 0);

	puts("element retrievable after insert");
	assert(ccask_keydir_get(kd, 5, key) != 0);

	puts("chained insert succeeds with same key");
	ccask_kdrow* kdr2 = 0;
	size_t vpos2 = 5;
	kdr2 = ccask_kdrow_new(5, key, 0, 0, vpos2, 1);
	assert(ccask_keydir_insert(kd, kdr2) != 0);

	puts("most recent insert selected when multiple entries with identical key");
	ccask_kdrow* res = ccask_keydir_get(kd, 5, key);
	assert(res != 0);
	assert(ccask_kdrow_vpos(res) == vpos2);

	ccask_keydir_delete(kd);

	puts("with keydir size 1 (i.e. all vals mapped to same key hash) we can still discriminate btwn keys");
	kd = ccask_keydir_new(1);
	assert(kd != 0);

	uint8_t key2[5] = { 0, 0, 1, 0, 0 };
	size_t vpos3 = 15;
	ccask_kdrow* kdr3 = ccask_kdrow_new(5, key2, 0, 0, vpos3, 0);
	assert(ccask_keydir_insert(kd, kdr) != 0);
	assert(ccask_keydir_insert(kd, kdr3) != 0);

	res = ccask_keydir_get(kd, 5, key2);
	assert(ccask_kdrow_vpos(res) == vpos3);

	res = ccask_keydir_get(kd, 5, key);
	assert(ccask_kdrow_vpos(res) == vpos1);

	puts("\t===== completed ccask_keydir tests =====");
}

void test_db(void) {
	return;
}

int main(void) {
	test_kdrow();
	puts("\n\n");
	test_keydir();
}
