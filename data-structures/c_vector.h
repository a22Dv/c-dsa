/*
    c_vector.h
    A minimal resizable array (a.k.a vector) implementation.
    NOTE: TESTING REQUIRED.
*/

#pragma once

#include <cstddef>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define _VSUCCESS 1
#define _VFAILURE 0

#define _VMAX(a, b) ((a) > (b) ? (a) : (b))
#define _VMIN(a, b) ((a) < (b) ? (a) : (b))
#define _VREQUIRE(cond, action)                                                                                        \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            action;                                                                                                    \
        }                                                                                                              \
    } while (0)

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

static inline cvec_metadata *vec_base(void *vec) { return vec ? (((cvec_metadata *)vec) - 1) : NULL; }
static inline size_t vec_size(void *vec) { return vec ? vec_base(vec)->_size : 0; }
static inline size_t vec_capacity(void *vec) { return vec ? vec_base(vec)->_capacity : 0; }
static inline void (*vec_dtor(void *vec))(void *) { return vec ? vec_base(vec)->_dtor : NULL; }
static inline size_t vec_esize(void *vec) { return vec ? vec_base(vec)->_esize : 0; }

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

/**
    Reserves enough bytes to hold more than or equal to `cpcty` elements.
*/
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

/**
    Pushes elements to the rear of the vector,
    resizing as needed.
*/
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

/**
    Pops elements out of the rear of the vector.
    Popping an empty vector is a no-op.
*/
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

/**
    Returns a void* pointer to the end of the vector.
    After the last valid element.
*/
static inline void *vec_end(void *vec) { return vec ? (char *)vec + (vec_size(vec)) * vec_esize(vec) : NULL; }
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

#ifdef VTYPES
#define VEC_BACK(vec) (vec[vec_size((void *)vec) - 1])
#define VEC_FRONT(vec) (vec[0])
#define VEC_BEGIN(vec) (&(VEC_FRONT(vec)))
#define VEC_END(vec) vec_end((void *)vec)
#define VEC_ESIZE(vec) vec_esize(vec)
#define VEC_DTOR(vec) vec_dtor(vec)
#define VEC_CAPACITY(vec) vec_capacity(vec)
#define VEC_SIZE(vec) vec_size(vec)
#define VEC_UNINIT(vec) vec_uninit((void **)&vec)
#define VEC_CLEAR(vec) vec_clear((void *)vec)
#define VEC_RESERVE(vec, size) vec_reserve((void **)&vec, size)
#define VEC_POP_BACK(vec) vec_pop_back((void *)vec)

#define _IMPL_VPUSH_BACK(T)                                                                                            \
    static inline int vec_##T##_push_back(T **vec, T e) {                                                              \
        T element = e;                                                                                                 \
        return vec_push_back((void **)vec, (void *)&element);                                                          \
    }
VTYPES(_IMPL_VPUSH_BACK)
#define _IMPL_GENERIC_PUSH_BACK(type) type ** : vec_##type##_push_back,
#define VEC_PUSH_BACK(vec, e) _Generic(&vec, VTYPES(_IMPL_GENERIC_PUSH_BACK) default: (void)0)(&vec, e)
#define VEC_RESERVE(vec, capacity) (vec_reserve(&vec, capacity))

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

// Define implementations
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
