#ifndef _CONFIG_H
#define _CONFIG_H

#include <stddef.h>

enum ccask_ip_v {
    INET4,
    INET6,
    UNSPEC
};

typedef struct ccask_config ccask_config;
typedef enum ccask_ip_v ccask_ip_v;

ccask_config* ccask_config_init(ccask_config* cf, char* port, size_t keydir_size, size_t maxconn, size_t max_msg_size,
                                ccask_ip_v ipv, size_t keydir_max_size);
ccask_config* ccask_config_new(char* port, size_t keydir_size, size_t maxconn, size_t max_msg_size, ccask_ip_v ipv,
                               size_t keydir_max_size);
ccask_config* ccask_config_from_env();

void ccask_config_delete(ccask_config* cf);
void ccask_config_destroy(ccask_config* cf);

void ccask_config_print(ccask_config* cf);

int ccask_config_port(char* dest, ccask_config* src, size_t destlen);
size_t ccask_config_kdsize(const ccask_config* src);
size_t ccask_config_maxconn(const ccask_config* src);
size_t ccask_config_maxmsg(const ccask_config* src);
ccask_ip_v ccask_config_ipv(const ccask_config* src);
size_t ccask_config_kdmax(const ccask_config* src);

#endif
