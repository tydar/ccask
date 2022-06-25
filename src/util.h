#ifndef _UTIL_H
#define _UTIL_H

#include <inttypes.h>
#include <arpa/inet.h>

#define NWK_BYTE_ARR_U32(src) ntohl(src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24)
#define HST_BYTE_ARR_U32(src) src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24

uint8_t* u32_to_nwk_byte_arr(uint8_t* dest, uint32_t src);
uint8_t* u32_to_host_byte_arr(uint8_t* dest, uint32_t src);

#endif
