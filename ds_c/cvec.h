/*  cvec.h
 *  A minimal vector implementation in C.
 *  https://github.com/a22Dv/c-dsa
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Utility macros.
#define _CVSTCINL static inline
#define _CVREQUIRE(condition, action)                                                              \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            action;                                                                                \
        }                                                                                          \
    } while (0)
#define _CVFALSE 0
#define _CVTRUE 1
#define _CVFOR(iter, start, end, step) for (size_t iter = (start); iter < (end); iter += (step))

// Vector header data.
typedef struct {
    size_t _element_size;
    size_t _capacity;
    size_t _size;
    void (*_destructor)(void *);
} cvec_t;

// Accessor macros
#define _CVHEADER(vec) (((cvec_t *)vec) - 1)
#define _CVCAPACITY(vec) (_CVHEADER(vec)->_capacity)
#define _CVSIZE(vec) (_CVHEADER(vec)->_size)
#define _CVESIZE(vec) (_CVHEADER(vec)->_element_size)
#define _CVDESTRUCTOR(vec) (_CVHEADER(vec)->_destructor)

_CVSTCINL size_t _cvec_nexp2(size_t n) {
    _CVREQUIRE(n, return 1);
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

// Initializes a given vector with a header.
_CVSTCINL _Bool cvec_init(
    void **restrict vec, size_t element_size, size_t initial_capacity, void (*destructor)(void *)
) {
    _CVREQUIRE(
        vec && element_size && initial_capacity < SIZE_MAX / element_size &&
            SIZE_MAX - initial_capacity * element_size > sizeof(cvec_t),
        return _CVFALSE
    );
    cvec_t *cvec = (cvec_t *)malloc(sizeof(cvec_t) + element_size * initial_capacity);
    _CVREQUIRE(cvec, return _CVFALSE);
    *cvec = (cvec_t){._element_size = element_size,
                     ._capacity = initial_capacity,
                     ._size = 0,
                     ._destructor = destructor};
    *vec = ++cvec;
    return _CVTRUE;
}

// Uninitializes/destroys a vector along with its elements should a destructor be provided.
_CVSTCINL void cvec_uninit(void **restrict vec) {
    _CVREQUIRE(vec && *vec, return);
    cvec_t *cvec = _CVHEADER(*vec);
    if (cvec->_destructor) {
        _CVFOR(i, 0, _CVSIZE(*vec), 1) { cvec->_destructor((char *)*vec + i * _CVESIZE(*vec)); }
    }
    free(cvec);
    *vec = NULL;
}

// Guarantees the vector's capacity >= specified capacity.
_CVSTCINL _Bool cvec_reserve(void **restrict vec, size_t new_capacity) {
    _CVREQUIRE(vec && *vec, return _CVFALSE);
    _CVREQUIRE(new_capacity > _CVCAPACITY(*vec), return _CVTRUE);
    new_capacity = _cvec_nexp2(new_capacity);
    _CVREQUIRE(
        SIZE_MAX / _CVESIZE(*vec) > new_capacity &&
            SIZE_MAX - _CVESIZE(*vec) * new_capacity > sizeof(cvec_t),
        return _CVFALSE
    );
    cvec_t *cvec = _CVHEADER(*vec);
    cvec_t *tmp = (cvec_t *)realloc(cvec, sizeof(cvec_t) + _CVESIZE(*vec) * new_capacity);
    _CVREQUIRE(tmp, return _CVFALSE);
    tmp->_capacity = new_capacity;
    *vec = ++tmp;
    return _CVTRUE;
}

// Pushes an element to the end of the vector.
_CVSTCINL _Bool cvec_pushback(void **restrict vec, const void *restrict element) {
    _CVREQUIRE(vec && *vec && element, return _CVFALSE);
    if (_CVSIZE(*vec) == _CVCAPACITY(*vec)) {
        _CVREQUIRE(cvec_reserve(vec, _CVCAPACITY(*vec) + 1), return _CVFALSE);
    }
    memcpy((char *)*vec + _CVSIZE(*vec) * _CVESIZE(*vec), element, _CVESIZE(*vec));
    ++_CVHEADER(*vec)->_size;
    return _CVTRUE;
}

// Pops the last element of the vector.
_CVSTCINL void cvec_popback(void **restrict vec) {
    _CVREQUIRE(vec && *vec && _CVSIZE(*vec) > 0, return);
    if (_CVDESTRUCTOR(*vec)) {
        _CVDESTRUCTOR (*vec)((char *)*vec + (_CVSIZE(*vec) - 1) * _CVESIZE(*vec));
    }
    --_CVSIZE(*vec);
}

// Inserts an element to a specified index of the vector.
_CVSTCINL _Bool cvec_insert(void **restrict vec, size_t index, const void *restrict element) {
    _CVREQUIRE(vec && *vec && element && index < _CVSIZE(*vec) + 1, return _CVFALSE);
    if (_CVSIZE(*vec) == _CVCAPACITY(*vec)) {
        _CVREQUIRE(cvec_reserve(vec, _CVCAPACITY(*vec) + 1), return _CVFALSE);
    }
    memmove(
        (char *)*vec + (index + 1) * _CVESIZE(*vec), (char *)*vec + index * _CVESIZE(*vec),
        (_CVSIZE(*vec) - index) * _CVESIZE(*vec)
    );
    memcpy((char *)*vec + (index * _CVESIZE(*vec)), element, _CVESIZE(*vec));
    ++_CVSIZE(*vec);
    return _CVTRUE;
}

// Removes an element at a specified index of the vector.
_CVSTCINL void cvec_remove(void **restrict vec, size_t index) {
    _CVREQUIRE(vec && *vec && _CVSIZE(*vec) && index < _CVSIZE(*vec), return);
    if (_CVDESTRUCTOR(*vec)) {
        _CVDESTRUCTOR (*vec)((char *)*vec + index * _CVESIZE(*vec));
    }
    memmove(
        (char *)*vec + index * _CVESIZE(*vec), (char *)*vec + (index + 1) * _CVESIZE(*vec),
        (_CVSIZE(*vec) - index - 1) * _CVESIZE(*vec)
    );
    --_CVSIZE(*vec);
}

// Clears all the elements of the vector.
_CVSTCINL void cvec_clear(void **restrict vec) {
    _CVREQUIRE(vec && *vec, return);
    if (_CVDESTRUCTOR(*vec)) {
        _CVFOR(i, 0, _CVSIZE(*vec), 1) { _CVDESTRUCTOR (*vec)((char *)*vec + i * _CVESIZE(*vec)); };
    }
    _CVSIZE(*vec) = 0;
}

// Performs a shallow copy on a given vector.
_CVSTCINL _Bool cvec_shlwcopy(void **restrict dst_vec, const void **restrict src_vec) {
    _CVREQUIRE(dst_vec && src_vec && *src_vec, return _CVFALSE);
    *dst_vec = malloc(sizeof(cvec_t) + _CVCAPACITY(*src_vec) * _CVESIZE(*src_vec));
    _CVREQUIRE(*dst_vec, return _CVFALSE);
    memcpy(*dst_vec, _CVHEADER(*src_vec), sizeof(cvec_t) + _CVSIZE(*src_vec) * _CVESIZE(*src_vec));
    ++*(cvec_t **)dst_vec;
    return _CVTRUE;
}

// Performs a deep copy on a given vector.
_CVSTCINL _Bool cvec_dpcopy(
    void **restrict dst_vec, const void **restrict src_vec, _Bool (*copy_func)(void *, const void *)
) {
    _CVREQUIRE(dst_vec && src_vec && *src_vec, return _CVFALSE);
    _CVREQUIRE(
        cvec_init(dst_vec, _CVESIZE(*src_vec), _CVCAPACITY(*src_vec), _CVDESTRUCTOR(*src_vec)),
        return _CVFALSE
    );
    _CVFOR(i, 0, _CVSIZE(*src_vec), 1) {
        const _Bool cpy = copy_func(
            (char *)*dst_vec + i * _CVESIZE(*dst_vec),
            (const char *)*src_vec + i * _CVESIZE(*src_vec)
        );
        if (!cpy) {
            _CVSIZE(*dst_vec) = i;
            cvec_uninit(dst_vec);
            return _CVFALSE;
        }
    }
    _CVSIZE(*dst_vec) = _CVSIZE(*src_vec);
    return _CVTRUE;
}

// Macro API.
#if defined(__GNUC__) || defined(__clang__)
#define CVEC_INIT(vec, init_capacity, destructor)                                                  \
    cvec_init((void **)&vec, sizeof(*vec), init_capacity, destructor)
#define CVEC_PUSHBACK(vec, element)                                                                \
    cvec_pushback((void **)&vec, (const void *)&(typeof(*vec)){element})
#define CVEC_INSERT(vec, index, element)                                                           \
    cvec_insert((void **)&vec, index, (const void *)&(typeof(*vec)){element})
#else
#define CVEC_INIT(type, vec, init_capacity, destructor)                                            \
    cvec_init((void **)&vec, sizeof(*vec), init_capacity, destructor)
#define CVEC_PUSHBACK(type, vec, element)                                                          \
    cvec_pushback((void **)&vec, (const void *)&(type){element})
#define CVEC_INSERT(type, vec, index, element)                                                     \
    cvec_insert((void **)&vec, index, (const void *)&(type){element})
#endif

#define CVEC_SIZE(vec) _CVSIZE(vec)
#define CVEC_CAPACITY(vec) _CVCAPACITY(vec)
#define CVEC_ESIZE(vec) _CVESIZE(vec)
#define CVEC_DESTRUCTOR(vec) _CVDESTRUCTOR(vec)
#define CVEC_UNINIT(vec) cvec_uninit((void **)&vec)
#define CVEC_POPBACK(vec) cvec_popback((void **)&vec)
#define CVEC_REMOVE(vec, index) cvec_remove((void **)&vec, index)
#define CVEC_CLEAR(vec) cvec_clear((void **)&vec)
#define CVEC_SCOPY(dst, src) cvec_shlwcopy((void **)&dst, (const void **)&src)
#define CVEC_DCOPY(dst, src, cpy_func) cvec_dpcopy((void **)&dst, (const void **)&src, cpy_func)