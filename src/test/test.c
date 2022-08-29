#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define _TEST_

#include "crc.h"
#include "ccask_header.h"
#include "ccask_kv.h"
#include "ccask_keydir.h"
#include "ccask_db.h"
#include "ccask_config.h"
#include "util.h"

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

    ccask_kdrow_delete(kdr1);

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

    ccask_keydir_delete(kd);
    ccask_kdrow_delete(kdr);
    ccask_kdrow_delete(kdr2);
    ccask_kdrow_delete(kdr3);
    puts("\t===== completed ccask_keydir tests =====");
}

void test_db(void) {
    puts("\t===== ccask_db tests ======");
	ccask_config* cfg = ccask_config_from_env();

    ccask_db* db = ccask_db_new("TEST_DB_FILE", cfg);
    puts("Assert DB ptr not null after _new...");
    assert(db != 0);

    uint32_t ksz = 5;
    uint8_t key[5] = { 1, 2, 3, 4, 5 };

    uint32_t vsz = 5;
    uint8_t val[5] = { 5, 4, 3, 2, 1 };
    db = ccask_db_set(db, ksz, key, vsz, val);
    puts("Assert DB not null after set....");
    assert(db != 0);

    ccask_get_result* gr = 0;
    gr = ccask_db_get(db, ksz, key);
    puts("Assert get result not null after good get....");
    assert(gr != 0);

    uint32_t gr_vsz = ccask_gr_vsz(gr);
    puts("Assert get result vsz == vsz");
    assert(gr_vsz == vsz);

    uint8_t* gr_val = 0;
    gr_val = malloc(gr_vsz);
    gr_val = ccask_gr_val(gr_val, gr);
    puts("Assert get result val == val");
    assert(memcmp(val, gr_val, gr_vsz) == 0);

    // size of our get command
    // 18 bytes
    uint32_t cmdsz = 4 + 1 + 4 + 4 + 5;
    uint8_t* cmd = 0;
    cmd = malloc(cmdsz);

    size_t index = 0;
    u32_to_nwk_byte_arr(cmd+index, cmdsz);
    index += sizeof(cmdsz);
    *(cmd+index) = 0; // GET command
    index++;
    u32_to_nwk_byte_arr(cmd+index, ksz);
    index += sizeof(ksz);
    u32_to_nwk_byte_arr(cmd+index, 0);
    index += sizeof(uint32_t);
    memcpy(cmd+index, key, ksz);

    ccask_result* res = 0;
    res = ccask_query_interp(db, cmd);
    assert(ccask_res_type(res) == 0);
    puts("query interp succeeds");

    uint32_t res_vsz = ccask_res_vsz(res);
    assert(res_vsz == vsz);
    puts("query result accurate vsz");

    uint8_t* res_val = 0;
    res_val = malloc(res_vsz);
    res_val = ccask_res_value(res_val, res);
    assert(memcmp(val, res_val, vsz) == 0);
    puts("query get result accurate value");

    cmdsz = 4 + 1 + 4 + 4 + 5 + 5; // add val size
    index = 0;
    free(cmd);
    cmd = malloc(cmdsz);

    uint8_t key2[5] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    uint8_t val2[5] = {
        0xAA, 0xAA, 0xAA, 0xAA, 0xAA
    };

    u32_to_nwk_byte_arr(cmd+index, cmdsz);
    index += sizeof(cmdsz);
    *(cmd+index) = 1; // SET command
    index++;
    u32_to_nwk_byte_arr(cmd+index, ksz);
    index += sizeof(ksz);
    u32_to_nwk_byte_arr(cmd+index, vsz);
    index += sizeof(vsz);
    memcpy(cmd+index, key2, ksz);
    index += ksz;
    memcpy(cmd+index, val2, vsz);

    ccask_res_delete(res);
    res = 0;
    res = ccask_query_interp(db, cmd);
    assert(res != 0);
    assert(ccask_res_type(res) == SET_SUCCESS);
    puts("query type set accurate");

    cmdsz = 4 + 1 + 4 + 4 + 5;
    index = 0;

    u32_to_nwk_byte_arr(cmd+index, cmdsz);
    index += sizeof(cmdsz);
    *(cmd+index) = 0; // GET command
    index++;
    u32_to_nwk_byte_arr(cmd+index, ksz);
    index += sizeof(ksz);
    u32_to_nwk_byte_arr(cmd+index, 0);
    index += sizeof(uint32_t);
    memcpy(cmd+index, key2, ksz);

    ccask_res_delete(res);
    res = 0;
    res = ccask_query_interp(db, cmd);
    assert(ccask_res_type(res) == GET_SUCCESS);
    puts("query interp succeeds");

    res_vsz = ccask_res_vsz(res);
    assert(res_vsz == vsz);
    puts("query result accurate vsz");

    free(res_val);
    res_val = 0;
    res_val = malloc(res_vsz);
    res_val = ccask_res_value(res_val, res);
    assert(memcmp(val2, res_val, vsz) == 0);
    puts("query get result accurate value");

    ccask_db_delete(db);
    ccask_gr_delete(gr);
    ccask_res_delete(res);
    free(gr_val);
    free(res_val);
    puts("\t===== ccask_db tests complete =====");
    return;
}

void test_server(void) {
    return;
}

void test_util(void) {
    puts("\t===== test util fn =====");

    // gotta test my bytewise conversion macros
    // since I am rusty on my endianness math...
    printf("uint8_t[] -> uint32_t macros......");
    fflush(stdin);
    uint8_t be255[4] = { 0, 0, 0, 0xFF };
    uint8_t be8192[4] = { 0, 0, 0x20, 0 };
    uint8_t le255[4] = { 0xFF, 0, 0, 0 };
    uint8_t le8192[4] = { 0, 0x20, 0, 0 };

    assert(NWK_BYTE_ARR_U32(be255) == 255);
    assert(NWK_BYTE_ARR_U32(be8192) == 8192);

    assert(HST_BYTE_ARR_U32(le255) == 255);
    assert(HST_BYTE_ARR_U32(le8192) == 8192);
    puts("worked!");

    uint8_t* narr = malloc(sizeof(uint32_t));
    narr = u32_to_nwk_byte_arr(narr, 255);
    uint32_t rev = NWK_BYTE_ARR_U32(narr);
    assert(rev == 255);


    narr = u32_to_nwk_byte_arr(narr, 25467);
    rev = NWK_BYTE_ARR_U32(narr);
    assert(rev == 25467);

    narr = u32_to_host_byte_arr(narr, 255);
    assert(HST_BYTE_ARR_U32(narr) == 255);

    narr = u32_to_host_byte_arr(narr, 25467);
    assert(HST_BYTE_ARR_U32(narr) == 25467);
    puts("u32->byte array tests complete!");

    puts("\t===== done =====");
}

void test_config(void) {
    puts("\t===== test ccask_config =====");
    int yes_replace = 1;

    char* env_port = "8001";
    assert(setenv("CCASK_PORT", env_port, yes_replace) == 0);

    char* env_kdsize = "2048";
    assert(setenv("CCASK_KDSIZE", env_kdsize, yes_replace) == 0);

    char* env_maxconn = "10";
    assert(setenv("CCASK_MAXCONN", env_maxconn, yes_replace) == 0);

    char* env_maxmsg = "2048";
    assert(setenv("CCASK_MAX_MSG_SIZE", env_maxmsg, yes_replace) == 0);

    char* env_ipv = "INET4";
    assert(setenv("CCASK_IPV", env_ipv, yes_replace) == 0);

    ccask_config* cfg = ccask_config_from_env();
    puts("config created from env successfully");

    char* dest = malloc(strlen(env_port));
    int rv = ccask_config_port(dest, cfg, strlen(env_port));
    assert(rv == strlen(env_port));
    assert(strcmp(dest, env_port) == 0);

    assert(ccask_config_kdsize(cfg) == 2048);
    assert(ccask_config_maxconn(cfg) == 10);
    assert(ccask_config_maxmsg(cfg) == 2048);
    assert(ccask_config_ipv(cfg) == INET4);
    puts("config object populated as expected");

    puts("\t===== done =====");
}

int main(void) {
    test_kdrow();
    puts("");
    test_keydir();
    puts("");
    test_util();
    puts("");
    test_db();
    puts("");
    test_config();
}
