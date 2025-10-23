/*
    cvec.h
    A minimal vector implementation in C.
*/

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Utility macros
#define _VREQUIRE(condition, action)                                                               \
    do {                                                                                           \
        if (!(condition))                                                                          \
            action;                                                                                \
    } while (0)
#define _VFALSE 0
#define _VTRUE 1
#define _VMAX(a, b) ((a) > (b) ? (a) : (b))
#define _VMIN(a, b) ((a) < (b) ? (a) : (b))
#define _VFOR(iter, start, end, step) for (size_t iter = (start); iter < (end); iter += (step))

typedef struct {
    size_t _element_size;
    size_t _capacity;
    size_t _size;
    void (*_destructor)(void *);
} cvec_t;

// Accessor macros
#define _VHEADER(vec) (((cvec_t *)vec) - 1)
#define _VCAPACITY(vec) (_VHEADER(vec)->_capacity)
#define _VSIZE(vec) (_VHEADER(vec)->_size)
#define _VESIZE(vec) (_VHEADER(vec)->_element_size)
#define _VDESTRUCTOR(vec) (_VHEADER(vec)->_destructor)

static inline size_t cvec_nexp2(size_t n) {
    _VREQUIRE(n, return 1);
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

static inline _Bool cvec_init(
    void **restrict vec, size_t element_size, size_t initial_capacity, void (*destructor)(void *)
) {
    _VREQUIRE(vec && element_size, return _VFALSE);
    cvec_t *cvec = (cvec_t *)malloc(sizeof(cvec_t) + element_size * initial_capacity);
    _VREQUIRE(cvec, return _VFALSE);
    *cvec = (cvec_t){._element_size = element_size,
                     ._capacity = initial_capacity,
                     ._size = 0,
                     ._destructor = destructor};
    cvec += 1;
    *vec = cvec;
    return _VTRUE;
}

static inline void cvec_uninit(void **restrict vec) {
    _VREQUIRE(vec && *vec, return);
    cvec_t *cvec = _VHEADER(*vec);
    void *data = *vec;
    if (cvec->_destructor) {
        _VFOR(i, 0, _VSIZE(*vec), 1) { cvec->_destructor((char *)data + i * _VESIZE(*vec)); }
    }
    free(cvec);
    *vec = NULL;
}

static inline _Bool cvec_reserve(void **restrict vec, size_t new_capacity) {
    _VREQUIRE(vec && *vec, return _VFALSE);
    _VREQUIRE(new_capacity > _VCAPACITY(*vec), return _VTRUE);
    cvec_t *cvec = _VHEADER(*vec);
    cvec_t *tmp = (cvec_t *)realloc(cvec, sizeof(cvec_t) + _VESIZE(*vec) * new_capacity);
    _VREQUIRE(tmp, return _VFALSE);
    tmp->_capacity = new_capacity;
    *vec = tmp + 1;
    return _VTRUE;
}

static inline _Bool cvec_pushback(void **restrict vec, void *restrict element) {
    _VREQUIRE(vec && *vec && element, return _VFALSE);
    if (_VSIZE(*vec) == _VCAPACITY(*vec)) {
        _VREQUIRE(cvec_reserve(vec, cvec_nexp2(_VCAPACITY(*vec) + 1)), return _VFALSE);
    }
    memcpy((char *)*vec + _VSIZE(*vec) * _VESIZE(*vec), element, _VESIZE(*vec));
    ++_VHEADER(*vec)->_size;
    return _VTRUE;
}

static inline void cvec_popback(void **restrict vec) {
    _VREQUIRE(vec && *vec && _VSIZE(*vec) > 0, return);
    if (_VDESTRUCTOR(*vec)) {
        _VDESTRUCTOR (*vec)((char *)*vec + (_VSIZE(*vec) - 1) * _VESIZE(*vec));
    }
    --_VSIZE(*vec);
}

static inline void cvec_clear(void **restrict vec) {
    _VREQUIRE(vec && *vec, return);
    if (_VDESTRUCTOR(*vec)) {
        _VFOR(i, 0, _VSIZE(*vec), 1) {
            _VDESTRUCTOR (*vec)((char *)*vec + i * _VESIZE(*vec));
        };
    }
    _VSIZE(*vec) = 0;
}