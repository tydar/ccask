
/* The keydir is a hash table from keys -> file_id, value_size, value_pos, timestamp
 *
 * Hash function: FNV-1a
 * Collision resolution: separate chaining
 *
 * TODO: make the hash function swappable via fn pointer in the ccask_keydir struct (for easy testing)
 */

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "ccask_keydir.h"

/*-----------struct defs---------------------*/
struct ccask_kdrow {
    uint32_t key_size;
    uint8_t* key;
    uint32_t file_id;
    uint32_t value_size;
    size_t value_pos;
    time_t timestamp;
    ccask_kdrow* next;
};

struct ccask_keydir {
    size_t size;
    ccask_kdrow* entries;
};

/*-----------------utility functions-------------------*/

/**@brief FNV-1a hash of the key *key* of length *key_size*.
 *
 * [FNV-1a hash](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function)
 *
 * @param key array of key_size bytes
 * @param key_size size of the key in bytes
 */
size_t hash(uint32_t key_size, uint8_t* key, size_t table_size) {
    size_t hash = 14695981039346656037ULL; // FNV offset basis constant

    for (uint32_t i = 0; i < key_size; i++) {
        hash ^= key[i];
        hash *= 1099511628211; // FNV 64bit prime constant
    }

    return hash % table_size;
}

ccask_kdrow* init_entries(size_t size, ccask_kdrow* entries) {
    for (size_t i = 0; i < size; i++) {
        entries[i] = (ccask_kdrow) {
            0
        };
    }

    return entries;
}

/*---------------kdrow functions-------------*/
ccask_kdrow* ccask_kdrow_init(ccask_kdrow* kdr, uint32_t key_size, uint8_t* key,
                              uint32_t file_id, uint32_t value_size, uint32_t value_pos, time_t timestamp) {
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

ccask_kdrow* ccask_kdrow_new(uint32_t key_size, uint8_t* key, uint32_t file_id,
                             uint32_t value_size, uint32_t value_pos, time_t timestamp) {
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

void ccask_kdrow_print(ccask_kdrow* kdr) {
    printf("Key size: %u ", kdr->key_size);
    printf("Key: [ ");
    for (uint32_t i = 0; i < kdr->key_size; i++) {
        printf("%hhx ", kdr->key[i]);
    }
    printf("] Timestamp: %ld\n", kdr->timestamp);
}

/*------------------keydir functions------------------*/
ccask_keydir* ccask_keydir_init(ccask_keydir* kd, size_t size) {
    ccask_kdrow* entries = malloc(size*sizeof(ccask_kdrow));
    init_entries(size, entries);
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

/**@brief walk the chain of elements until the next pointer is null, then insert our new element there*/
ccask_kdrow* keydir_chain_insert(ccask_kdrow* entry, ccask_kdrow* elem) {
    if(!entry->next) {
        entry->next = malloc(sizeof(ccask_kdrow));
        *entry->next = *elem;
        return entry;
    }

    ccask_kdrow* next = entry->next;
    ccask_kdrow* last = next;

    while(next) {
        last = next;
        next = next->next;
    }

    last->next = malloc(sizeof(ccask_kdrow));
    *last->next = *elem;

    return last;
}

ccask_keydir* ccask_keydir_insert(ccask_keydir* kd, ccask_kdrow* elem) {
    size_t index = hash(elem->key_size, elem->key, kd->size);
    if (kd->entries[index].key_size == 0) {
        kd->entries[index] = *elem;
    } else {
        keydir_chain_insert(&(kd->entries[index]), elem);
    }

    return kd;
}

// To retrieve a keydir entry, we need to
// 1) get correct hash bucket
// 2) check the key of the stored value
// 3) if there is a chain, walk the chain to be sure we don't have a newer one
// 4) return the newest item from that bucket with a matching key
//
// If a value has been stored under that key a bunch of times, it could expensive
// but eventually should implement chain compression as part of the get process to
// remove all but the oldest keydir entry.
//
// Alternatively could support rollbacks somehow by maintaining the chain...

ccask_kdrow* keydir_chain_get(ccask_kdrow* entry, uint32_t key_size, uint8_t* key) {
    ccask_kdrow* match = { 0 };
    ccask_kdrow* next = entry;

    do {
        if(next->key_size == key_size && memcmp(key, next->key, key_size) == 0) {
            match = next;
        }
        next = next->next;
    } while(next);

    return match;
}

ccask_kdrow* ccask_keydir_get(ccask_keydir* kd, uint32_t key_size, uint8_t* key) {
    size_t index = hash(key_size, key, kd->size);
    ccask_kdrow* match = { 0 };
    if(kd->entries[index].key_size != 0) {
        match = keydir_chain_get(&(kd->entries[index]), key_size, key);
    }
    return match;
}
