
/* The keydir is a hash table from keys -> file_id, value_size, value_pos, timestamp
 *
 * Hash function: division hash
 * Collision resolution: separate chaining
 */

#include "ccask_keydir.h"

/*-----------struct defs---------------------*/
struct ccask_kdrow {
    uint32_t key_size;
    uint8_t* key;
    uint32_t file_id;
    uint32_t value_size;
    size_t value_pos;
    uint32_t timestamp;
    ccask_kdrow* next;
};

struct ccask_keydir {
    size_t size;
    ccask_kd_row* entries;
};


/*---------------kdrow functions-------------*/
ccask_kdrow* ccask_kdrow_init(ccask_kdrow* kdr,
                              uint32_t key_size, uint8_t* key, uint32_t file_id, uint32_t value_size, uint32_t value_pos, uint32_t timestamp) {
    uint8_t* nkey = malloc(key_size);
    memcpy(nkey, key, key_size);
    if (kdr && nkey) {
        *kdr = (ccask_kdrow) {
            .key_size = key_size,
            .key = nkey,
            .file_id = file_id,
            .value_size = value_size,
            .value_pos = value_pos,
            .timestamp = .timestamp,
            .next = 0,
        };
    } else {
        *kdr = (ccask_kdrow) {
            0
        };
    }

    return kdr;
}

ccask_kdrow* ccask_kdrow_new(uint32_t key_size,
                             uint8_t* key, uint32_t file_id, uint32_t value_size, uint32_t value_pos, uint32_t timestamp) {
    ccask_kdrow* kdr = malloc(sizeof(ccask_kdrow));

    return ccask_kdrow_init(kdr, key_size, key, file_id, value_size, value_pos, timestamp);
}

/**@brief zeroes out a ccask_kdrow obj and frees any allocated memory*/
void ccask_kdrow_destroy(ccask_kdrow* kdr) {
    if (kdr) {
        kdr->next = 0;
        free(kdr->key);
        *kdr = { 0 };
    }
}

void ccask_kdrow_delete(ccask_kdrow* kdr) {
    ccask_kdrow_destroy(kdr);
    free(kdr);
}

/*------------------keydir functions------------------*/
ccask_keydir* ccask_keydir_init(ccask_keydir* kd, size_t size) {
    if (kd) {
        ccask_kdrow* entries = malloc(size*sizeof(ccask_kdrow));
        *kd = (ccask_keydir) {
            .size = size,
            .entries = entries,
        }
    } else {
        *kd = (ccask_keydir) {
            0
        };
    }
    return kd;
}

ccask_keydir* ccask_keydir_new(size_t size) {
    ccask_keydir* kd = malloc(sizeof(ccask_keydir));
    kd = ccask_keydir_init(kd, size);
    return kd;
}

void ccask_keydir_destroy(ccask_keydir* kd) {
    if (kd) {
        free(kd->entries);
        *kd = (ccask_keydir) {
            0
        };
    }
}

void ccask_keydir_delete(ccask_keydir* kd) {
    ccask_keydir_destroy(kd);
    free(kd);
}

ccask_keydir* ccask_keydir_insert(ccask_kdrow* elem) {
    return 0;
}

ccask_kdrow* ccask_keydir_get(uint32_t key_size, uint8_t* key) {
    return 0;
}

uint8_t* hash(uint32_t key_size, uint8_t* key) {
    return 0;
}
