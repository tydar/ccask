
/* The keydir is a hash table from keys -> file_id, value_size, value_pos, timestamp
 *
 * Hash function: division hash
 * Collision resolution: separate chaining
 */

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>

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
    ccask_kdrow* entries;
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
            .timestamp = timestamp,
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
        *kdr = (ccask_kdrow) {
            0
        };
    }
}

void ccask_kdrow_delete(ccask_kdrow* kdr) {
    ccask_kdrow_destroy(kdr);
    free(kdr);
}

/*------------------keydir functions------------------*/
ccask_keydir* ccask_keydir_init(ccask_keydir* kd, size_t size) {
    ccask_kdrow* entries = malloc(size*sizeof(ccask_kdrow));
    entries = init_entries(size, entries);
    if (kd) {
        *kd = (ccask_keydir) {
            .size = size,
            .entries = entries,
        };
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

ccask_keydir* ccask_keydir_insert(ccask_keydir* kd, ccask_kdrow elem) {
    size_t index = hash(elem->key_size, elem->key, kd->size);
    if (!kd->entries[index]->key) {
        *kd->entries[index] = elem;
    } else {
        keydir_chain_insert(kd->entries[index], elem);
    }

    return kd;
}

/**@brief walk the chain of elements until the next pointer is null, then insert our new element there*/
ccask_kdrow* keydir_chain_insert(ccask_kdrow* entry, ccask_kdrow elem) {
    ccask_kdrow* next = entry->next;
    while(next->next) {
        next = next->next;
    }

    next->next = malloc(sizeof(ccask_kdrow));
    *next->next = elem;

    return entry;
}

ccask_kdrow* ccask_keydir_get(ccask_keydir* kd, uint32_t key_size, uint8_t* key) {
    return 0;
}

/*-----------------utility functions-------------------*/

/**@brief FNV-1a hash of the key *key* of length *key_size*.
 *
 * [FNV-1a hash](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function)
 *
 * @param key array of key_size bytes
 * @param key_size size of the key in bytes
 */
size_t hash(uint32_t key_size, uint8_t* key, size_t table_size) {
    size_t hash = 14695981039346656037; // FNV offset basis constant

    for (uint32_t i = 0; i < key_size; i++) {
        hash ^= key[i];
        hash *= 1099511628211; // FNV 64bit prime constant
    }

    return hash % table_size;
}

ccask_kdrow* init_entries(size_t size, ccask_kdrow* entries) {
    for (size_t i = 0; i < size; i++) {
        entries[0] = (ccask_kdrow) {
            0
        };
    }
}
