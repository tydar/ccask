#include "ccask_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT "29456"
#define DEFAULT_KDSIZE 1024
#define DEFAULT_MAXCONN 5
#define DEFAULT_MAXMSG 1024
#define DEFAULT_IPV UNSPEC

char* ipv_string(ccask_ip_v ipv) {
    switch(ipv) {
    case INET4:
        return "INET4";
    case INET6:
        return "INET6";
    case UNSPEC:
        return "UNSPEC";
    default:
        return "UNKNOWN";
    }
}

struct ccask_config {
    char* port;
    size_t keydir_size;
    size_t maxconn;
    size_t max_msg_size;
    ccask_ip_v ipv;
};

char* PORT = "CCASK_PORT";
char* KDSIZE = "CCASK_KDSIZE";
char* MAXCONN = "CCASK_MAXCONN";
char* MAXMSG = "CCASK_MAX_MSG_SIZE";
char* IPV = "CCASK_IPV";

ccask_config* ccask_config_init(ccask_config* cf, char* port, size_t keydir_size, size_t maxconn, size_t max_msg_size,
                                ccask_ip_v ipv) {
    if (cf) {
        *cf = (ccask_config) {
            .port = malloc(strlen(port)),
            .keydir_size = keydir_size,
            .maxconn = maxconn,
            .max_msg_size = max_msg_size,
            .ipv = ipv
        };

        if (cf->port) {
            strcpy(cf->port, port);
        } else {
            *cf = (ccask_config) {
                0
            };
        }
    } else {
        *cf = (ccask_config) {
            0
        };
    }

    return cf;
}

ccask_config* ccask_config_new(char* port, size_t keydir_size, size_t maxconn, size_t max_msg_size, ccask_ip_v ipv) {
    ccask_config* cf = malloc(sizeof(ccask_config));
    if (!cf) return 0;
    cf = ccask_config_init(cf, port, keydir_size, maxconn, max_msg_size, ipv);
    return cf;
}

/**@brief ccask_config_from_env creates a new config struct from environment variable parsing & defaults*/
ccask_config* ccask_config_from_env() {
    char* port_str = getenv(PORT);
    char* kdsize_str = getenv(KDSIZE);
    char* maxconn_str = getenv(MAXCONN);
    char* maxmsg_str = getenv(MAXMSG);
    char* ipv_str = getenv(IPV);


    char* port = 0;
    if (!port_str) {
        fprintf(stderr, "config: using default port %s\n", DEFAULT_PORT);
        port = DEFAULT_PORT;
    } else {
        port = port_str;
    }

    size_t kdsize = DEFAULT_KDSIZE;
    if (kdsize_str) {
        kdsize = strtoull(kdsize_str, NULL, 10);
        if (kdsize == 0)  {
            fprintf(stderr, "config: CCASK_KDSIZE env value %s invalid; using default %ul\n", kdsize_str, DEFAULT_KDSIZE);
            kdsize = DEFAULT_KDSIZE;
        }
    }

    size_t maxconn = DEFAULT_MAXCONN;
    if (maxconn_str) {
        maxconn = strtoull(maxconn_str, NULL, 10);
        if (maxconn == 0) {
            fprintf(stderr, "config: CCASK_MAXCONN env value %s invalid; using default %ul\n", maxconn_str, DEFAULT_MAXCONN);
            maxconn = DEFAULT_MAXCONN;
        }
    }

    size_t maxmsg = DEFAULT_MAXMSG;
    if (maxmsg_str) {
        maxmsg = strtoull(maxmsg_str, NULL, 10);
        if (maxmsg == 0) {
            fprintf(stderr, "config: CCASK_MAXMSG env value %s invalid; using default %ul\n", maxmsg_str, DEFAULT_MAXMSG);
            maxmsg = DEFAULT_MAXMSG;
        }
    }

    ccask_ip_v ipv = DEFAULT_IPV;
    if (ipv_str) {
        if (strcmp(ipv_str, "INET4") == 0) {
            ipv = INET4;
        } else if (strcmp(ipv_str, "INET6") == 0) {
            ipv = INET6;
        } else if (strcmp(ipv_str, "UNSPEC") == 0) {
            ipv = UNSPEC;
        } else {
            fprintf(stderr, "config: CCASK_IPV env value %s unrecognized; using default %s\n", ipv_str, ipv_string(DEFAULT_IPV));
        }
    }

    return ccask_config_new(port, kdsize, maxconn, maxmsg, ipv);
}

void ccask_config_destroy(ccask_config* cf) {
    if(cf) {
        free(cf->port);
        *cf = (ccask_config) {
            0
        };
    }
}

void ccask_config_delete(ccask_config* cf) {
    ccask_config_destroy(cf);
    free(cf);
}

void ccask_config_print(ccask_config* cf) {
    printf("port: %s\tkeydir size: %zu\tmax connection count: %zu\nmax message size: %zu B\tIP type: %s\n",
           cf->port,
           cf->keydir_size,
           cf->maxconn,
           cf->max_msg_size,
           ipv_string(cf->ipv));
}

int ccask_config_port(char* dest, ccask_config* src, size_t destlen) {
    size_t len = strlen(src->port);
    if (destlen < len || !dest) return -1;

    strcpy(dest, src->port);
    return len;
}

size_t ccask_config_kdsize(ccask_config* src) {
    return src->keydir_size;
}

size_t ccask_config_maxconn(ccask_config* src) {
    return src->maxconn;
}

size_t ccask_config_maxmsg(ccask_config* src) {
    return src->max_msg_size;
}

ccask_ip_v ccask_config_ipv(ccask_config* src) {
    return src->ipv;
}
