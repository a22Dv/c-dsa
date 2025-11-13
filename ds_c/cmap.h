/*  cmap.h
 *  A minimal hashmap (a.k.a std::unordered_map) implementation in C.
 *  https://github.com/a22Dv/c-dsa
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define _CMSTCINL static inline
#define _CMREQUIRE(condition, action)                                                              \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            action;                                                                                \
        }                                                                                          \
    } while (0)
    
#define _CMFALSE 0
#define _CMTRUE 1
#define _CMFOR(iter, start, end, step) for (size_t iter = (start); iter < (end); iter += (step))
#define _CMSTRITER(iter, str) for (const char *iter = str; *(iter); ++iter)
#define _CMMAX(a, b) ((a) > (b) ? (a) : (b))
#define _CM_INLINE_SIZE 3
#define _CMLFACTOR_LIMIT 0.7
#define _CMLFACTOR_MIN 0.2

#define _CMSENTINEL 0xC0FFEEC0FFEECAFE
#define _CM_MAX_ENTRIES UINT16_MAX

typedef struct {
    void *key;   // Not guaranteed to be a pointer.
    void *value; // Not guaranteed to be a pointer.
} cmap_entry_t;

typedef struct {
    cmap_entry_t _inline_entries[_CM_INLINE_SIZE];
    cmap_entry_t *_overflow_entries;
    uint16_t _overflow_capacity;
    uint16_t _total_entries;
    _Bool _occupied;
} cmap_bucket_t;

typedef struct {
    cmap_bucket_t *_buckets;
    size_t _size;
    size_t _capacity;
    uint8_t _key_size; // Limited to sizeof(void*). Otherwise, store indirectly through a pointer.
    uint8_t _val_size; // Limited to sizeof(void*). Otherwise, store indirectly through a pointer.
    size_t (*_hash_func)(const void *);
    int (*_comparison_func)(const void *, const void *); // Receives the element to be compared.
    void (*_key_destructor)(void **); // Receives a pointer to the element to be destroyed.
    void (*_val_destructor)(void **); // Receives a pointer to the element to be destroyed.
} cmap_t;

typedef struct {
    cmap_t *map;
    size_t bucket_idx;
    size_t st_idx;
} cmap_iterator_t;

_CMSTCINL size_t _cmap_nexp2(size_t n) {
    _CMREQUIRE(n, return 1);
    if (n > SIZE_MAX / 2) {
        return SIZE_MAX;
    }
    n -= 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if (UINTPTR_MAX == UINT64_MAX)
    n |= n >> 32;
#endif
    n += 1;
    return n;
}

/*
    Generic hash function for integer-types (char, int, long, ...).
    Uses XXH64_avalanche() and XXH32_avalanche() from xxHash.
*/
_CMSTCINL size_t cmap_genhash(const void *in_key) {
    size_t key = (size_t)in_key;
#if SIZE_MAX == UINT64_MAX
    key ^= key >> 33;
    key *= 0xC2B2AE3D27D4EB4FULL; // XXH_PRIME64_2 (xxhash.h)
    key ^= key >> 29;
    key *= 0x165667B19E3779F9ULL; // XXH_PRIME64_3 (xxhash.h)
    key ^= key >> 32;
#elif SIZE_MAX == UINT32_MAX
    key ^= key >> 15;
    key *= 0x85EBCA77U; // XXH_PRIME32_2 (xxhash.h)
    key ^= key >> 13;
    key *= 0xC2B2AE3DU; // XXH_PRIME32_3 (xxhash.h)
    key ^= key >> 16;
#else
#error "UNKNOWN ARCHITECTURE (Only 64-bit/32-bit supported)."
#endif
    return key;
}

/*
    Generic hash function for strings (const char*),
    but this does not work on wide-strings (const wchar*).
    Uses the FNV-1a hash function.
*/
_CMSTCINL size_t cmap_strhash(const void *in_key) {
    const char *key = (const char *)in_key;
#if SIZE_MAX == UINT64_MAX
    size_t hash = 0xCBF29CE484222325; // FNV offset-basis (64-bits)
    _CMSTRITER(c, key) {
        hash ^= (size_t)*c;
        hash *= 0x00000100000001B3; // FNV prime (64-bits)
    }
#elif SIZE_MAX == UINT32_MAX
    size_t hash = 0x811C9DC5; // FNV offset-basis (32-bits)
    _CMSTRITER(c, key) {
        hash ^= (size_t)c;
        hash *= 0x01000193; // FNV prime (32-bits)
    }
#else
#error "UNKNOWN ARCHITECTURE (Only 64-bit/32-bit supported)."
#endif
    return hash;
}

// Generic wrapper around strcmp.
_CMSTCINL int cmap_strcmp(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

// Generic integer comparison function.
_CMSTCINL int cmap_gencmp(const void *a, const void *b) { return (a > b) - (a < b); }

// Generic destructor function.
_CMSTCINL void cmap_gendtor(void **addr) { free(*addr); }

// Initializes a given pointer with a cmap_t instance.
_CMSTCINL _Bool cmap_init(
    cmap_t **map,
    size_t key_size,
    size_t val_size,
    size_t initial_capacity,
    size_t (*hash_func)(const void *),
    int (*comparison_func)(const void *, const void *),
    void (*key_destructor)(void **),
    void (*val_destructor)(void **)
) {
    _CMREQUIRE(map && key_size && val_size && hash_func && comparison_func, return _CMFALSE);
    _CMREQUIRE(initial_capacity < SIZE_MAX / sizeof(cmap_bucket_t), return _CMFALSE);
    _CMREQUIRE(key_size <= sizeof(void *) && val_size <= sizeof(void *), return _CMFALSE);
    _CMREQUIRE(initial_capacity > 2, initial_capacity = 2);
    cmap_t *cmap = malloc(sizeof(cmap_t));
    size_t ncapacity = _cmap_nexp2(initial_capacity);
    _CMREQUIRE(cmap, return _CMFALSE);
    *cmap = (cmap_t){._buckets = NULL,
                     ._size = 0,
                     ._capacity = ncapacity,
                     ._key_size = key_size,
                     ._val_size = val_size,
                     ._hash_func = hash_func,
                     ._comparison_func = comparison_func,
                     ._key_destructor = key_destructor,
                     ._val_destructor = val_destructor};
    cmap->_buckets = calloc(ncapacity, sizeof(cmap_bucket_t));
    _CMREQUIRE(cmap->_buckets, free(cmap); return _CMFALSE);

    _CMFOR(i, 0, initial_capacity, 1) {
        cmap_bucket_t *bucket = &cmap->_buckets[i];
        _CMFOR(j, 0, _CM_INLINE_SIZE, 1) { bucket->_inline_entries[j].key = (void *)_CMSENTINEL; }
    }
    *map = cmap;
    return _CMTRUE;
}

// Implementation detail.
_CMSTCINL void _cmap_uninit_entry(cmap_entry_t *entry, cmap_t *map) {
    if (entry->key == (void *)_CMSENTINEL) {
        return;
    }
    if (map->_key_destructor) {
        map->_key_destructor((void **)&(entry->key));
    }
    if (map->_val_destructor) {
        map->_val_destructor((void **)&(entry->value));
    }
}

// Uninitializes a pointer to a cmap_t instance.
_CMSTCINL void cmap_uninit(cmap_t **map) {
    _CMREQUIRE(map && *map, return);
    cmap_t *cmap = *map;
    _CMFOR(i, 0, cmap->_capacity, 1) {
        if (!cmap->_buckets[i]._occupied) {
            continue;
        }
        cmap_bucket_t *bucket = &(cmap->_buckets[i]);
        _CMFOR(j, 0, _CM_INLINE_SIZE, 1) {
            _cmap_uninit_entry(&(bucket->_inline_entries[j]), cmap);
        }
        if (!bucket->_overflow_entries) {
            continue;
        }
        _CMFOR(j, 0, bucket->_overflow_capacity, 1) {
            _cmap_uninit_entry(&(bucket->_overflow_entries[j]), cmap);
        }
        free(bucket->_overflow_entries);
    }
    free(cmap->_buckets);
    free(cmap);
    *map = NULL;
}

// Allocates a new array if given pointer is NULL, else reallocates.
_CMSTCINL _Bool _cmap_mdfd_n2exp_alloc(void **arr, size_t nsize, size_t esize, size_t *out_ncap) {
    _CMREQUIRE(arr && nsize * esize != 0, return _CMFALSE);
    size_t nexp2_size = _cmap_nexp2(nsize);
    size_t alloc_size = nexp2_size * esize;
    void *tmp = NULL;
    if (*arr) {
        tmp = realloc(*arr, alloc_size);
    } else {
        tmp = malloc(alloc_size);
    }
    _CMREQUIRE(tmp, return _CMFALSE);
    *arr = tmp;
    *out_ncap = nexp2_size;
    return _CMTRUE;
}

_CMSTCINL _Bool cmap_insert(cmap_t **map, void *key, void *value);

// Resizes a given map to a given size.
_CMSTCINL _Bool cmap_resize(cmap_t **map, size_t nsize) {
    _CMREQUIRE(map && *map, return _CMFALSE);
    size_t new_capacity = _cmap_nexp2(nsize);
    cmap_t *cmap = *map;
    size_t prv_capacity = cmap->_capacity;
    size_t prv_size = cmap->_size;
    cmap_bucket_t *prv_buckets = cmap->_buckets;
    cmap_bucket_t *new_buckets = calloc(new_capacity, sizeof(cmap_bucket_t));
    _Bool ret = _CMTRUE;
    _CMREQUIRE(new_buckets, return _CMFALSE);

    _CMFOR(i, 0, new_capacity, 1) {
        cmap_bucket_t *bucket = &new_buckets[i];
        _CMFOR(j, 0, _CM_INLINE_SIZE, 1) { bucket->_inline_entries[j].key = (void *)_CMSENTINEL; }
    }

    cmap->_buckets = new_buckets;
    cmap->_capacity = new_capacity;
    cmap->_size = 0; // Prevents ping-pong recursion by resetting the load factor.

    _CMFOR(i, 0, prv_capacity, 1) {
        if (!prv_buckets[i]._occupied) {
            continue;
        }
        _CMFOR(j, 0, _CM_INLINE_SIZE, 1) {
            cmap_entry_t *entry = &prv_buckets[i]._inline_entries[j];
            if (entry->key == (void *)_CMSENTINEL) {
                continue;
            }
            _CMREQUIRE(cmap_insert(map, entry->key, entry->value), ret = _CMFALSE; goto end);
        }
        if (!prv_buckets[i]._overflow_entries) {
            continue;
        }
        _CMFOR(j, 0, prv_buckets[i]._overflow_capacity, 1) {
            cmap_entry_t *entry = &prv_buckets[i]._overflow_entries[j];
            if (entry->key == (void *)_CMSENTINEL) {
                continue;
            }
            _CMREQUIRE(cmap_insert(map, entry->key, entry->value), ret = _CMFALSE; goto end);
        }
    }
end:
    if (!ret) {
        _CMFOR(i, 0, new_capacity, 1) {
            cmap_bucket_t *bucket = &new_buckets[i];
            _CMFOR(j, 0, _CM_INLINE_SIZE, 1) {
                _cmap_uninit_entry(&bucket->_inline_entries[j], cmap);
            }
            if (new_buckets[i]._overflow_entries) {
                _CMFOR(j, 0, new_buckets[i]._overflow_capacity, 1) {
                    _cmap_uninit_entry(&bucket->_overflow_entries[j], cmap);
                }
                free(new_buckets[i]._overflow_entries);
            }
        }
        free(new_buckets);
        cmap->_buckets = prv_buckets;
        cmap->_capacity = prv_capacity;
        cmap->_size = prv_size;
        return ret;
    }
    _CMFOR(i, 0, prv_capacity, 1) {
        if (prv_buckets[i]._overflow_entries) {
            free(prv_buckets[i]._overflow_entries);
        }
    }
    free(prv_buckets);
    return ret;
}

// Inserts a key-value pair into the map. If a key already exists, replace the value.
_CMSTCINL _Bool cmap_insert(cmap_t **map, void *key, void *value) {
    _CMREQUIRE(map && *map, return _CMFALSE);
    if ((float)(*map)->_size / (*map)->_capacity >= _CMLFACTOR_LIMIT) {
        cmap_resize(map, (*map)->_capacity + 1);
    }

    cmap_t *cmap = *map;
    cmap_bucket_t *bucket = &cmap->_buckets[cmap->_hash_func(key) & (cmap->_capacity - 1)];
    cmap_entry_t *insertion_slot = NULL;

    // Bucket is empty.
    if (!bucket->_occupied) {
        insertion_slot = &bucket->_inline_entries[0];
        bucket->_occupied = _CMTRUE;
    }

    // Search for an empty slot in the inlined data.
    if (!insertion_slot) {
        _CMFOR(i, 0, _CM_INLINE_SIZE, 1) {
            if (bucket->_inline_entries[i].key == (void *)_CMSENTINEL && !insertion_slot) {
                insertion_slot = &bucket->_inline_entries[i];
                break;
            }
            if (cmap->_comparison_func(bucket->_inline_entries[i].key, key) == 0) {
                bucket->_inline_entries[i].value = value;
                return _CMTRUE;
            }
        }
    }

    // Search for a new slot in the overflow data.
    if (!insertion_slot) {
        if (!bucket->_overflow_entries) {
            size_t ncapacity = 0;
            _CMREQUIRE(
                _cmap_mdfd_n2exp_alloc(
                    (void **)&bucket->_overflow_entries, 2, sizeof(cmap_entry_t), &ncapacity
                ),
                return _CMFALSE
            );
            _CMFOR(i, 0, ncapacity, 1) { bucket->_overflow_entries[i].key = (void *)_CMSENTINEL; }
            bucket->_overflow_capacity = ncapacity;
        }
        _CMFOR(i, 0, bucket->_overflow_capacity, 1) {
            if (bucket->_overflow_entries[i].key == (void *)_CMSENTINEL && !insertion_slot) {
                insertion_slot = &bucket->_overflow_entries[i];
                break;
            }
            if (cmap->_comparison_func(bucket->_overflow_entries[i].key, key) == 0) {
                bucket->_overflow_entries[i].value = value;
                return _CMTRUE;
            }
        }
    }

    // All slots are used, reallocate overflow and set insertion_slot to the end.
    if (!insertion_slot) {
        size_t ncapacity = 0;
        _CMREQUIRE(
            _cmap_mdfd_n2exp_alloc(
                (void **)&bucket->_overflow_entries, bucket->_overflow_capacity + 1,
                sizeof(cmap_entry_t), &ncapacity
            ),
            return _CMFALSE
        );
        _CMFOR(i, bucket->_overflow_capacity, ncapacity, 1) {
            bucket->_overflow_entries[i].key = (void *)_CMSENTINEL;
        }
        insertion_slot = &bucket->_overflow_entries[bucket->_overflow_capacity];
        bucket->_overflow_capacity = ncapacity;
    }

    // Insert element.
    insertion_slot->key = key;
    insertion_slot->value = value;
    ++bucket->_total_entries;
    ++cmap->_size;
    return _CMTRUE;
}

// Returns a pointer to the key-value pair if found, else returns `NULL`.
_CMSTCINL cmap_entry_t *cmap_get_entry(cmap_t **map, void *key) {
    _CMREQUIRE(map && *map, return NULL);
    cmap_t *cmap = *map;
    cmap_bucket_t *bucket = &cmap->_buckets[cmap->_hash_func(key) & (cmap->_capacity - 1)];
    if (!bucket->_occupied) {
        return NULL;
    }
    _CMFOR(i, 0, _CM_INLINE_SIZE, 1) {
        if (bucket->_inline_entries[i].key == (void *)_CMSENTINEL) {
            continue;
        }
        if (cmap->_comparison_func(bucket->_inline_entries[i].key, key) == 0) {
            return &bucket->_inline_entries[i];
        }
    }
    if (!bucket->_overflow_entries) {
        return NULL;
    }
    _CMFOR(i, 0, bucket->_overflow_capacity, 1) {
        if (bucket->_overflow_entries[i].key == (void *)_CMSENTINEL) {
            continue;
        }
        if (cmap->_comparison_func(bucket->_overflow_entries[i].key, key) == 0) {
            return &bucket->_overflow_entries[i];
        }
    }
    return NULL;
}

// Gets the value and assigns it to an out-parameter.
_CMSTCINL _Bool cmap_get(cmap_t **map, void *key, void **out) {
    _CMREQUIRE(map && *map && out, return _CMFALSE);
    cmap_entry_t *entry = cmap_get_entry(map, key);
    if (!entry) {
        return _CMFALSE;
    }
    *out = entry->value;
    return _CMTRUE;
}

// Removes a specified key-value entry from the map.
_CMSTCINL void cmap_remove(cmap_t **map, void *key) {
    _CMREQUIRE(map && *map, return);
    cmap_entry_t *entry = cmap_get_entry(map, key);
    _CMREQUIRE(entry, return);
    cmap_t *cmap = *map;
    if (cmap->_key_destructor) {
        cmap->_key_destructor((void **)&(entry->key));
    }
    if (cmap->_val_destructor) {
        cmap->_val_destructor((void **)&(entry->value));
    }
    entry->key = (void *)_CMSENTINEL;
    --cmap->_size;
    cmap_bucket_t *bucket = &cmap->_buckets[cmap->_hash_func(key) & (cmap->_capacity - 1)];
    --bucket->_total_entries;
    if (!bucket->_total_entries) {
        bucket->_occupied = _CMFALSE;
    }
    if ((float)cmap->_size / cmap->_capacity < _CMLFACTOR_MIN) {
        cmap_resize(map, cmap->_capacity / 2);
    }
}

_CMSTCINL _Bool cmap_iter_next(cmap_iterator_t *iter, cmap_entry_t *out) {
    _CMREQUIRE(iter && out && iter->map && iter->bucket_idx < iter->map->_size, return _CMFALSE);
    cmap_t *map = iter->map;
    size_t bucket_idx = iter->bucket_idx;
    size_t st_idx = iter->st_idx;
    cmap_bucket_t *bucket = &map->_buckets[bucket_idx];

    ++st_idx; // Increment to possible next entry.

    while (bucket_idx < map->_capacity) {
        if (!bucket->_occupied) {
            bucket = &map->_buckets[++bucket_idx];
            continue;
        }
        _Bool found = _CMFALSE;
        size_t tbkt_capacity = bucket->_overflow_capacity + _CM_INLINE_SIZE;
        while (st_idx < tbkt_capacity) {
            if (st_idx < _CM_INLINE_SIZE &&
                bucket->_inline_entries[st_idx].key != (void *)_CMSENTINEL) {
                found = _CMTRUE;
                break;
            } else if (st_idx >= _CM_INLINE_SIZE &&
                       bucket->_overflow_entries[st_idx - _CM_INLINE_SIZE].key !=
                           (void *)_CMSENTINEL) {
                found = _CMTRUE;
                break;
            }
            ++st_idx;
        }
        if (found) {
            break;
        } else if (st_idx == tbkt_capacity) {
            bucket = &map->_buckets[++bucket_idx];
            st_idx = 0;
        }
    }
    if (bucket_idx == map->_size) {
        iter->bucket_idx = bucket_idx;
        iter->st_idx = st_idx;
        return _CMFALSE;
    }
    cmap_entry_t *arr = st_idx < _CM_INLINE_SIZE ? map->_buckets[bucket_idx]._inline_entries
                                                 : map->_buckets[bucket_idx]._overflow_entries;
    *out = arr[st_idx - ((st_idx < _CM_INLINE_SIZE) ? 0 : _CM_INLINE_SIZE)];
    iter->bucket_idx = bucket_idx;
    iter->st_idx = st_idx;
    return _CMTRUE;
}

_CMSTCINL _Bool cmap_iter_start(cmap_t **map, cmap_iterator_t *iter, cmap_entry_t *out) {
    _CMREQUIRE(map && *map && iter && out, return _CMFALSE);
    _CMREQUIRE((*map)->_size > 0, return _CMFALSE);

    iter->bucket_idx = 0;
    iter->map = *map;
    iter->st_idx = 0;

    cmap_t *cmap = *map;
    if (cmap->_buckets[0]._inline_entries[0].key != (void *)_CMSENTINEL) {
        *out = cmap->_buckets[0]._inline_entries[0];
    } else {
        _CMREQUIRE(cmap_iter_next(iter, out), return _CMFALSE);
    }
    return _CMTRUE;
}

// Macro API accessors.
#define CMAP_SIZE(map) (map->_size)
#define CMAP_CAPACITY(map) (map->_capacity)
#define CMAP_BUCKETS(map) (map->_buckets)
#define CMAP_KEY_SIZE(map) (map->_key_size)
#define CMAP_VALUE_SIZE(map) (map->_val_size)
#define CMAP_HASH_FUNCTION(map) (map->_hash_func)
#define CMAP_CMP_FUNCTION(map) (map->_comparison_func)
#define CMAP_KEY_DESTRUCTOR(map) (map->_key_destructor)
#define CMAP_VALUE_DESTRUCTOR(map) (map->_val_destructor)

// Macro API functions.
#define CMAP_INIT(map, key_type, val_type, init_capacity, hash_func, cmp_func, kdtor, vdtor)       \
    (cmap_init(                                                                                    \
        &map, sizeof(key_type), sizeof(val_type), init_capacity, hash_func, cmp_func, kdtor, vdtor \
    ))
#define CMAP_UNINIT(map) (cmap_uninit(&map))
#define CMAP_INSERT(map, key, value) (cmap_insert(&map, (void *)key, (void *)value))
#define CMAP_REMOVE(map, key) (cmap_remove(&map, (void *)key))
#define CMAP_GETENTRY(map, key) (cmap_get_entry(&map, (void *)key))
#define CMAP_GETVAL(map, key, out) (cmap_get(&map, (void *)key, (void **)&out))
#define CMAP_RESIZE(map, nsize) (cmap_resize(&map, nsize))
#define CMAP_ITER_START(map, iter, out) (cmap_iter_start(&map, &iter, &out))
#define CMAP_ITER_NEXT(iter, out) (cmap_iter_next(&iter, &out))