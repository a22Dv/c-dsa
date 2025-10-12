/*
    c_vector.h
    A minimal resizable array (a.k.a vector) implementation.
*/

#pragma once

#include <cstddef>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define _VSUCCESS 1
#define _VFAILURE 0

#define _VREQUIRE(cond, action)                                                                                        \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            action;                                                                                                    \
        }                                                                                                              \
    } while (0)

// Next largest power of 2
static inline size_t _n2exp(size_t n) {
    if (n == 0)
        return 1;
    --n;
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
#ifdef __x86_64__
    n |= (n >> 32);
#endif
    ++n;
    return n;
}

typedef struct alignas(max_align_t) {
    size_t _capacity;
    size_t _size;
    size_t _esize;
    void (*_dtor)(void *);
} cvec_metadata;

// Get base address.
static inline cvec_metadata *vec_base(void *vec) { return vec ? (((cvec_metadata *)vec) - 1) : NULL; }

// Get vector size.
static inline size_t vec_size(void *vec) { return vec ? vec_base(vec)->_size : 0; }

// Get vector capacity.
static inline size_t vec_capacity(void *vec) { return vec ? vec_base(vec)->_capacity : 0; }

// Get vector destructor.
static inline void (*vec_dtor(void *vec))(void *) { return vec ? vec_base(vec)->_dtor : NULL; }

// Get vector element size.
static inline size_t vec_esize(void *vec) { return vec ? vec_base(vec)->_esize : 0; }

// Returns a `void*` pointer to the end of the vector exclusive of the last valid element.
static inline void *vec_end(void *vec) { return vec ? (char *)vec + (vec_size(vec)) * vec_esize(vec) : NULL; }

// Returns a `void*` pointer to the starting valid element of the vector.
static inline void *vec_begin(void *vec) { return vec && vec_size(vec) > 0 ? vec : NULL; }

// Returns a `void*` pointer to the i'th element of the vector.
static inline void *vec_at(void *vec, size_t i) {
    return vec && vec_size(vec) > i ? (char *)vec + i * vec_esize(vec) : NULL;
}

/**
    Initializes a given pointer with a metadata struct.
    Deconstructor argument has to be of type `void(*)(T*)`.
*/
static inline int vec_init(void **vec, size_t e_size, size_t init_cpcty, void (*dtor)(void *)) {
    int rcode = _VSUCCESS;
    size_t ncap = 0;
    cvec_metadata *base = NULL;
    char *data = NULL;
    _VREQUIRE(vec && e_size > 0, rcode = _VFAILURE; goto end);
    ncap = _n2exp(init_cpcty);
    *vec = malloc(sizeof(cvec_metadata) + (ncap * e_size));
    _VREQUIRE(*vec, rcode = _VFAILURE; goto end);
    base = (cvec_metadata *)*vec;
    base->_esize = e_size;
    base->_size = 0;
    base->_capacity = ncap;
    base->_dtor = dtor;
    data = (char *)((base + 1));
    *vec = data;
end:
    return rcode;
}

/**
    Releases any memory held by the elements by calling
    the destructor if it was passed during construction.
    Frees the array itself and sets it to NULL.
*/
static inline void vec_uninit(void **vec) {
    void (*dtor)(void *) = NULL;
    size_t size = 0;
    size_t esize = 0;
    _VREQUIRE(vec && *vec, goto end);
    dtor = vec_dtor(*vec);
    size = vec_size(*vec);
    esize = vec_esize(*vec);
    if (dtor) {
        for (size_t i = 0; i < size; ++i) {
            dtor((char *)(*vec) + (i * esize));
        }
    }
    free((cvec_metadata *)(*vec) - 1);
    *vec = NULL;
end:
    return;
}

// Reserves enough bytes to hold more than or equal to `cpcty` elements.
static inline int vec_reserve(void **vec, size_t cpcty) {
    int rcode = _VSUCCESS;
    cvec_metadata *base = NULL;
    cvec_metadata *tmp = NULL;
    size_t ncap = 0;
    _VREQUIRE(vec && *vec && cpcty < SIZE_MAX / vec_esize(*vec), rcode = _VFAILURE; goto end);
    _VREQUIRE(vec_capacity(*vec) < cpcty, goto end);
    ncap = _n2exp(cpcty);
    base = vec_base(*vec);
    tmp = (cvec_metadata *)realloc(base, sizeof(cvec_metadata) + ncap * base->_esize);
    _VREQUIRE(tmp, rcode = _VFAILURE; goto end);
    tmp->_capacity = ncap;
    *vec = (char *)tmp + sizeof(cvec_metadata);
end:
    return rcode;
}

// Resizes the vector to a specified size. Allocating as needed, and null-initializing the members if required.
static inline int vec_resize(void **vec, size_t size) {
    int rcode = _VSUCCESS;
    cvec_metadata *base = 0;
    _VREQUIRE(vec && *vec, rcode = _VFAILURE; goto end);
    base = vec_base(*vec);
    if (size > vec_capacity(*vec)) {
        _VREQUIRE(vec_reserve(vec, size), rcode = _VFAILURE; goto end);
    } else if (size < base->_size && base->_dtor) {
        void (*dtor)(void *) = base->_dtor;
        size_t vsize = base->_size;
        size_t esize = base->_esize;
        for (size_t i = size; i < vsize; ++i) {
            dtor((char *)*vec + i * esize);
        }
    }
    if (size > base->_size) {
        memset((char *)*vec + base->_esize * base->_size, 0, base->_esize * (size - base->_size));
    }
    base->_size = size;
end:
    return rcode;
}

// Shrinks the vector's allocation to exactly vec_size(vec) elements.
static inline int vec_shrink_to_fit(void **vec) {
    int rcode = _VSUCCESS;
    cvec_metadata *base = NULL;
    cvec_metadata *tmp = NULL;
    _VREQUIRE(vec && *vec, rcode = _VFAILURE; goto end);
    base = vec_base(*vec);
    tmp = (cvec_metadata *)realloc(base, sizeof(cvec_metadata) + base->_size);
    _VREQUIRE(tmp, rcode = _VFAILURE; goto end);
    *vec = tmp;
end:
    return rcode;
}

// Inserts an element at a specified index.
static inline int vec_insert(void** vec, void* element, size_t i) {
    _VREQUIRE(vec && *vec && element && vec_size(*vec) >= i, return _VFAILURE);
    _VREQUIRE(vec_reserve(vec, vec_size(*vec) + 1), return _VFAILURE);
    size_t size = vec_size(*vec);
    size_t esize =  vec_esize(*vec);
    memmove((char*)*vec + (i + 1) * esize, (char*)*vec + i * esize, (size - i) * esize);
    memcpy((char*)*vec + i * esize, element, esize);
    ++vec_base(*vec)->_size;
    return _VSUCCESS;
}

// Removes an element at a specified index and calls the deconstructor if provided.
static inline void vec_remove(void** vec, size_t i) {
    _VREQUIRE(vec && *vec && vec_size(*vec) > i, return);
    cvec_metadata* base = vec_base(*vec);
    if (base->_dtor) {
        base->_dtor((char*)*vec + i * base->_esize);
    }
    size_t esize = base->_esize;
    memmove((char*)*vec + i * esize, (char*)*vec + (i + 1) * esize, (vec_size(*vec) - i - 1) * esize);
    --base->_size;
}

// Fills a given out-parameter with the address of the end of the vector.
static inline int vec_emplace_back(void **vec, void **out) {
    _VREQUIRE(vec && *vec && out, return _VFAILURE);
    _VREQUIRE(vec_reserve(vec, vec_size(*vec) + 1), return _VFAILURE);
    ++vec_base(*vec)->_size;
    *out = (char *)vec_end(*vec) - vec_esize(*vec);
    return _VSUCCESS;
}

// Pushes elements to the rear of the vector, resizing as needed.
static inline int vec_push_back(void **vec, void *element) {
    int rcode = _VSUCCESS;
    size_t nsize, esize;
    _VREQUIRE(vec && *vec && element, rcode = _VFAILURE; goto end);
    nsize = vec_size(*vec) + 1;
    esize = vec_esize(*vec);
    if (vec_capacity(*vec) < nsize) {
        _VREQUIRE(vec_reserve(vec, nsize), rcode = _VFAILURE; goto end);
    }
    memcpy(((char *)*vec) + (nsize - 1) * esize, element, esize);
    (vec_base(*vec)->_size) = nsize;
end:
    return rcode;
}

// Pops elements out of the rear of the vector. Popping an empty vector is a no-op.
static inline void vec_pop_back(void *vec) {
    cvec_metadata *base = NULL;
    _VREQUIRE(vec, goto end);
    base = vec_base(vec);
    if (base->_size > 0) {
        if (base->_dtor) {
            (base->_dtor)((char *)vec + (base->_size - 1) * base->_esize);
        }
        --base->_size;
    }
end:
    return;
}

// Returns a void* pointer to the end of the vector after the last valid element.
static inline void vec_clear(void *vec) {
    cvec_metadata *base = NULL;
    _VREQUIRE(vec, goto end);
    if ((base = vec_base(vec))->_dtor) {
        for (size_t i = 0; i < base->_size; ++i) {
            (base->_dtor)((char *)vec + i * base->_esize);
        }
    }
    vec_base(vec)->_size = 0;
end:
    return;
}

#define VTYPES(T)
#ifdef VTYPES
#define VEC_AT(vec, i) vec_at((void*)vec, i);
#define VEC_BEGIN(vec) vec_begin((void *)vec)
#define VEC_END(vec) vec_end((void *)vec)
#define VEC_ESIZE(vec) vec_esize((void *)vec)
#define VEC_DTOR(vec) vec_dtor((void *)vec)
#define VEC_CAPACITY(vec) vec_capacity((void *)vec)
#define VEC_SIZE(vec) vec_size((void *)vec)
#define VEC_UNINIT(vec) vec_uninit((void **)&vec)
#define VEC_CLEAR(vec) vec_clear((void *)vec)
#define VEC_RESERVE(vec, size) vec_reserve((void **)&vec, size)
#define VEC_POP_BACK(vec) vec_pop_back((void *)vec)
#define VEC_EMPLACE_BACK(vec, ptr) vec_emplace_back((void**)&vec, (void**)&ptr)
#define VEC_SHRINK_TO_FIT(vec) vec_shrink_to_fit((void**)&vec)
// Vector push back implementation.
#define _IMPL_VPUSH_BACK(T)                                                                                            \
    static inline int vec_##T##_push_back(T **vec, T e) { return vec_push_back((void **)vec, (void *)&(T){e}); }
VTYPES(_IMPL_VPUSH_BACK)
#define _IMPL_GENERIC_PUSH_BACK(type) type ** : vec_##type##_push_back,
#define VEC_PUSH_BACK(vec, e) _Generic(&vec, VTYPES(_IMPL_GENERIC_PUSH_BACK) default: (void)0)(&vec, e)

// Vector initialization macros.
#define _IMPL_VINIT_DEFAULT(T)                                                                                         \
    static inline int vec_##T##_init_default(T **vec) { return vec_init((void **)vec, sizeof(T), 0, NULL); }
#define _IMPL_VINIT_SIZE(T)                                                                                            \
    static inline int vec_##T##_init_size(T **vec, size_t size) {                                                      \
        return vec_init((void **)vec, sizeof(T), size, NULL);                                                          \
    }
#define _IMPL_VINIT_SIZE_DTOR(T)                                                                                       \
    static inline int vec_##T##_init_size_dtor(T **vec, size_t size, void (*dtor)(void *)) {                           \
        return vec_init((void **)vec, sizeof(T), size, dtor);                                                          \
    }
VTYPES(_IMPL_VINIT_DEFAULT)
VTYPES(_IMPL_VINIT_SIZE)
VTYPES(_IMPL_VINIT_SIZE_DTOR)
#define _IMPL_VINIT_DISPATCH(_1, _2, _3, NAME, ...) NAME
#define _IMPL_VINIT_SDT_GENERIC(type) type ** : vec_##type##_init_size_dtor,
#define _IMPL_VINIT_S_GENERIC(type) type ** : vec_##type##_init_size,
#define _IMPL_VINIT_D_GENERIC(type) type ** : vec_##type##_init_default,
#define VEC_INIT(vec, ...)                                                                                             \
    _Generic(&vec,                                                                                                     \
        VTYPES(_IMPL_VINIT_DISPATCH(&vec, __VA_ARGS__, _IMPL_VINIT_SDT_GENERIC, _IMPL_VINIT_S_GENERIC,                 \
                                    _IMPL_VINIT_D_GENERIC)) default: NULL)(&vec, __VA_ARGS__)
#endif
