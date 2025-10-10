// array.h

#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline size_t _n2exp(size_t n) {
    n > 0 ? --n : 0;
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
#ifdef __x86_64
    n |= (n >> 32);
#endif
    ++n;
    return n;
}

#define DEFINE_TYPE(type)                                                                                              \
    typedef struct {                                                                                                   \
        type *_data;                                                                                                   \
        size_t _size;                                                                                                  \
        size_t _capacity;                                                                                              \
    } type##_array;                                                                                                    \
    size_t type##_array_capacity(type##_array *self);                                                                  \
    size_t type##_array_size(type##_array *self);                                                                      \
    type *type##_array_data(type##_array *self);                                                                       \
    int type##_array_reserve(type##_array *self, const size_t new_capacity);                                           \
    int type##_array_resize(type##_array *self, const size_t new_size);                                                \
    int type##_array_init(type##_array *self, const size_t size);                                                      \
    void type##_array_delete(type##_array *self);                                                                      \
    int type##_array_append(type##_array *dst, type##_array *src);                                                     \
    int type##_array_push_back(type##_array *self, type element);                                                      \
    void type##_array_pop_back(type##_array *self);                                                                    \
    type *type##_array_front(type##_array *self);                                                                      \
    type *type##_array_back(type##_array *self);                                                                       \
    int type##_array_insert(type##_array *self, type element, size_t index);                                           \
    void type##_array_remove(type##_array *self, size_t index);                                                        \
    void type##_array_clear(type##_array *self);                                                                       \
    type *type##_array_at(type##_array *self, size_t index);

#define IMPL_TYPE(type)                                                                                                \
    size_t type##_array_capacity(type##_array *self) {                                                                 \
        if (!self) {                                                                                                   \
            return 0;                                                                                                  \
        }                                                                                                              \
        return self->_capacity;                                                                                        \
    }                                                                                                                  \
    size_t type##_array_size(type##_array *self) {                                                                     \
        if (!self) {                                                                                                   \
            return 0;                                                                                                  \
        }                                                                                                              \
        return self->_size;                                                                                            \
    }                                                                                                                  \
    type *type##_array_data(type##_array *self) {                                                                      \
        if (!self) {                                                                                                   \
            return NULL;                                                                                               \
        }                                                                                                              \
        return self->_data;                                                                                            \
    }                                                                                                                  \
    int type##_array_reserve(type##_array *self, const size_t new_capacity) {                                          \
        if (!self) {                                                                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
        if (self->_capacity >= new_capacity) {                                                                         \
            return 0;                                                                                                  \
        }                                                                                                              \
        size_t new_total_capacity = _n2exp(new_capacity);                                                              \
        type *temp = (type *)realloc(self->_data, new_total_capacity * sizeof(type));                                  \
        if (!temp) {                                                                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
        self->_data = temp;                                                                                            \
        self->_capacity = new_total_capacity;                                                                          \
        return 0;                                                                                                      \
    }                                                                                                                  \
    int type##_array_resize(type##_array *self, const size_t new_size) {                                               \
        if (!self) {                                                                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
        if (self->_capacity < new_size) {                                                                              \
            if (type##_array_reserve(self, new_size)) {                                                                \
                return 1;                                                                                              \
            }                                                                                                          \
        }                                                                                                              \
        self->_size = new_size;                                                                                        \
        return 0;                                                                                                      \
    }                                                                                                                  \
    int type##_array_init(type##_array *self, const size_t size) {                                                     \
        if (!self) {                                                                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
        size_t new_capacity = _n2exp(size);                                                                            \
        self->_data = (type *)malloc(new_capacity * sizeof(type));                                                     \
        if (!self->_data) {                                                                                            \
            return 1;                                                                                                  \
        }                                                                                                              \
        self->_capacity = new_capacity;                                                                                \
        self->_size = 0;                                                                                               \
        return 0;                                                                                                      \
    }                                                                                                                  \
    void type##_array_delete(type##_array *self) {                                                                     \
        if (!self) {                                                                                                   \
            return;                                                                                                    \
        }                                                                                                              \
        free(self->_data);                                                                                             \
        self->_data = NULL;                                                                                            \
        self->_size = 0;                                                                                               \
        self->_capacity = 0;                                                                                           \
    }                                                                                                                  \
    int type##_array_append(type##_array *dst, type##_array *src) {                                                    \
        if (!dst || !src) {                                                                                            \
            return 1;                                                                                                  \
        }                                                                                                              \
        if (src->_size == 0) {                                                                                         \
            return 0;                                                                                                  \
        }                                                                                                              \
        size_t new_dst_size = dst->_size + src->_size;                                                                 \
        if (new_dst_size > dst->_capacity) {                                                                           \
            if (type##_array_reserve(dst, new_dst_size)) {                                                             \
                return 1;                                                                                              \
            }                                                                                                          \
        }                                                                                                              \
        memcpy((void *)(dst->_data + dst->_size), (void *)(src->_data), src->_size * sizeof(type));                    \
        dst->_size = new_dst_size;                                                                                     \
        return 0;                                                                                                      \
    }                                                                                                                  \
    int type##_array_push_back(type##_array *self, type element) {                                                     \
        if (!self) {                                                                                                   \
            return 1;                                                                                                  \
        }                                                                                                              \
        if (self->_size + 1 > self->_capacity) {                                                                       \
            if (type##_array_reserve(self, self->_size + 1)) {                                                         \
                return 1;                                                                                              \
            }                                                                                                          \
        }                                                                                                              \
        self->_data[self->_size] = element;                                                                            \
        ++self->_size;                                                                                                 \
        return 0;                                                                                                      \
    }                                                                                                                  \
    void type##_array_pop_back(type##_array *self) {                                                                   \
        if (!self || !self->_size) {                                                                                   \
            return;                                                                                                    \
        }                                                                                                              \
        --self->_size;                                                                                                 \
    }                                                                                                                  \
    type *type##_array_front(type##_array *self) {                                                                     \
        if (!self || self->_size == 0) {                                                                               \
            return NULL;                                                                                               \
        }                                                                                                              \
        return self->_data;                                                                                            \
    }                                                                                                                  \
    type *type##_array_back(type##_array *self) {                                                                      \
        if (!self || self->_size == 0) {                                                                               \
            return NULL;                                                                                               \
        }                                                                                                              \
        return &(self->_data[self->_size - 1]);                                                                        \
    }                                                                                                                  \
    int type##_array_insert(type##_array *self, type element, size_t index) {                                          \
        if (!self || index > self->_size) {                                                                            \
            return 1;                                                                                                  \
        }                                                                                                              \
        if (self->_size + 1 > self->_capacity) {                                                                       \
            if (type##_array_reserve(self, self->_size + 1)) {                                                         \
                return 1;                                                                                              \
            }                                                                                                          \
        }                                                                                                              \
        memmove((void *)(self->_data + index + 1), (void *)(self->_data + index),                                      \
                (self->_size - index) * sizeof(type));                                                                 \
        self->_data[index] = element;                                                                                  \
        ++self->_size;                                                                                                 \
        return 0;                                                                                                      \
    }                                                                                                                  \
    void type##_array_remove(type##_array *self, size_t index) {                                                       \
        if (!self || index >= self->_size) {                                                                           \
            return;                                                                                                    \
        }                                                                                                              \
        memmove((void *)(self->_data + index), (void *)(self->_data + index + 1),                                      \
                (self->_size - index - 1) * sizeof(type));                                                             \
        --self->_size;                                                                                                 \
    }                                                                                                                  \
    void type##_array_clear(type##_array *self) { self->_size = 0; }                                                   \
    type *type##_array_at(type##_array *self, size_t index) {                                                          \
        if (!self || self->_size <= index) {                                                                           \
            return NULL;                                                                                               \
        }                                                                                                              \
        return &(self->_data[index]);                                                                                  \
    }

#ifdef ARRAY_TYPES
#define _FUNC_GEN(type, func) type##_array * : type##_array_##func
#define _POP_BACK_GEN(type) _FUNC_GEN(type, pop_back),
#define POP_BACK(array) _Generic(&array, ARRAY_TYPES(_POP_BACK_GEN) default: (void)0)(&array)
#define _PUSH_BACK_GEN(type) _FUNC_GEN(type, push_back),
#define PUSH_BACK(array, element) _Generic(&array, ARRAY_TYPES(_PUSH_BACK_GEN) default: (void)0)(&array, element)
#define _SIZE_GEN(type) _FUNC_GEN(type, size),
#define SIZE(array) _Generic(&array, ARRAY_TYPES(_SIZE_GEN) default: (void)0)(&array)
#define _DATA_GEN(type) _FUNC_GEN(type, data),
#define DATA(array) _Generic(&array, ARRAY_TYPES(_DATA_GEN) default: (void)0)(&array)
#define _RESERVE_GEN(type) _FUNC_GEN(type, reserve),
#define RESERVE(array, capacity) _Generic(&array, ARRAY_TYPES(_RESERVE_GEN) default: (void)0)(&array, capacity)
#define _RESIZE_GEN(type) _FUNC_GEN(type, resize),
#define RESIZE(array, size) _Generic(&array, ARRAY_TYPES(_RESIZE_GEN) default: (void)0)(&array, size)
#define _INIT_GEN(type) _FUNC_GEN(type, init),
#define INIT(array, size) _Generic(&array, ARRAY_TYPES(_INIT_GEN) default: (void)0)(&array, size)
#define _DELETE_GEN(type) _FUNC_GEN(type, delete),
#define DELETE(array) _Generic(&array, ARRAY_TYPES(_DELETE_GEN) default: (void)0)(&array)
#define _APPEND_GEN(type) _FUNC_GEN(type, append),
#define APPEND(array) _Generic(&array, ARRAY_TYPES(_APPEND_GEN) default: (void)0)(&array)
#define _FRONT_GEN(type) _FUNC_GEN(type, front),
#define FRONT(array) _Generic(&array, ARRAY_TYPES(_FRONT_GEN) default: (void)0)(&array)
#define _BACK_GEN(type) _FUNC_GEN(type, back),
#define BACK(array) _Generic(&array, ARRAY_TYPES(_BACK_GEN) default: (void)0)(&array)
#define _INSERT_GEN(type) _FUNC_GEN(type, insert),
#define INSERT(array, element, index)                                                                                  \
    _Generic(&array, ARRAY_TYPES(_INSERT_GEN) default: (void)0)(&array, element, index)
#define _REMOVE_GEN(type) _FUNC_GEN(type, remove),
#define REMOVE(array, index) _Generic(&array, ARRAY_TYPES(_INSERT_GEN) default: (void)0)(&array, index)
#define _CLEAR_GEN(type) _FUNC_GEN(type, clear),
#define CLEAR(array) _Generic(&array, ARRAY_TYPES(_CLEAR_GEN) default: (void)0)(&array)
#define _AT_GEN(type) _FUNC_GEN(type, at),
#define AT(array, index) _Generic(&array, ARRAY_TYPES(_AT_GEN) default: (void)0)(&array, index)

#define TYPE(type) DEFINE_TYPE(type)
ARRAY_TYPES(TYPE)
#undef TYPE
#endif

#ifdef ARRAY_IMPL
#define TYPE(type) IMPL_TYPE(type)
ARRAY_TYPES(TYPE)
#undef TYPE
#endif