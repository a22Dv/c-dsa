/*
    cmap.h
    A minimal hashmap (a.k.a std::unordered_map) implementation in C.
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>

#define _CMREQUIRE(condition, action)                                                              \
    do {                                                                                           \
        if (!(condition))                                                                          \
            action;                                                                                \
    } while (0)
#define _CMFALSE 0
#define _CMTRUE 1
#define _CMFOR(iter, start, end, step) for (size_t iter = (start); iter < (end); iter += (step))
#define _CM_INLINE_SIZE 3

/*
    This is not a hard limit.
    However, exceeding 32 entries
    means that the map cannot reliably
    track empty and occupied entries through the bitset.
    The map uses a marker to distinguish empty keys 
    from that point forward.
*/
#define _CM_MAX_ENTRIES 32 
#define _CM_ENTRIES_MARKER 0xC0FFEEC0FFEECAFE

typedef struct {
    void *key;   // Not guaranteed to be a pointer.
    void *value; // Not guaranteed to be a pointer.
} cmap_entry_t;

typedef struct {
    cmap_entry_t _inline_entries[_CM_INLINE_SIZE];
    cmap_entry_t *_overflow_entries;
    uint32_t _occupied_entries;  // Bitset to store valid/deleted entries.
    uint8_t _inline_entries_count;
    uint8_t _overflow_entries_count;
    uint8_t _overflow_capacity;
    _Bool _occupied;
} cmap_bucket_t;

typedef struct {
    cmap_bucket_t *_buckets;
    size_t _size;
    size_t _capacity;
    uint8_t _key_size; // Limited to sizeof(void*). Otherwise, store indirectly through a pointer.
    uint8_t _val_size; // Limited to sizeof(void*). Otherwise, store indirectly through a pointer.
    _Bool _key_is_ptr;
    _Bool _val_is_ptr;
    size_t (*hash_func)(const void *);
    int (*_comparison_func)(const void *, const void *);
    void (*_key_destructor)(void *);
    void (*_val_destructor)(void *);
} cmap_t;