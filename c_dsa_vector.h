/**
    c_dsa_vector.h
    A minimal and generic resizable array (a.k.a vector) implementation.

    HOWTO:
    To use this header file, simply define the types you want to use
    using VECTOR_TYPES(T) T(int) ..., and define VECTOR_IMPL on one
    of the .c files.

    NOTE:
    When using this for owning structs,
    you MUST use pointers to the objects, not the objects
    themselves. This is because the vector treats everything
    in it as POD. It calls the destructor if it was specified,
    but the element in the vector must be already a pointer
    to the object.
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "c_dsa_details.h"

#define VECTOR(T) vec_##T

#define DECL_VEC(T)                                                                                                    \
    typedef struct {                                                                                                   \
        T *_data;                                                                                                      \
        void (*_dtor)(void *);                                                                                         \
        size_t _size;                                                                                                  \
        size_t _capacity;                                                                                              \
    } vec_##T

#define SIZE(self) (self._size)
#define DATA(self) (self._data)
#define CAPACITY(self) (self._capacity)
#define FRONT(self) (self._data[0])
#define BACK(self) (self._data[self._size - 1])
#define NEW(destructor)                                                                                                \
    {                                                                                                                  \
        ._data = NULL,                                                                                                 \
        ._dtor = destructor,                                                                                           \
        ._size = 0,                                                                                                    \
        ._capacity = 0,                                                                                                \
    }
#define EMPTY(self) (self._size == 0)
#define DELETE_IMPL(T)                                                                                                 \
    void vec_##T##_delete(vec_##T *self) {                                                                             \
        if (self->_dtor) {                                                                                             \
            for (size_t i = 0; i < self->_size; ++i) {                                                                 \
                self->_dtor((void *)((self->_data)[i]));                                                               \
            }                                                                                                          \
        }                                                                                                              \
        free(self->_data);                                                                                             \
        self->_data = NULL;                                                                                            \
        self->_size = 0;                                                                                               \
        self->_capacity = 0;                                                                                           \
    }
#define DELETE_GENERIC(T) VECTOR(T) * : vec_##T##_delete,
#define DELETE(self) _Generic(&self, VECTOR_TYPES(DELETE_GENERIC) default: NULL)(&self)

#define RESERVE_IMPL(T)                                                                                                \
    int vec_##T##_reserve(vec_##T *self, size_t ncapacity) {                                                           \
        CHECK(!self || self->_capacity >= ncapacity, return SUCCESS);                                                  \
        ncapacity = _n2exp(ncapacity);                                                                                 \
        void *temp = realloc(self->_data, sizeof(T) * ncapacity);                                                      \
        CHECK(!temp, return FAILURE)                                                                                   \
        self->_data = (T *)temp;                                                                                       \
        self->_capacity = ncapacity;                                                                                   \
        return SUCCESS;                                                                                                \
    }
#define RESERVE_GENERIC(T) VECTOR(T) * : vec_##T##_reserve,
#define RESERVE(self, capacity) _Generic(&self, VECTOR_TYPES(RESERVE_GENERIC) default: NULL)(&self, capacity)

#define RESIZE_IMPL(T)                                                                                                 \
    int vec_##T##_resize(vec_##T *self, size_t nsize) {                                                                \
        CHECK(!self, return FAILURE);                                                                                  \
        if (nsize < self->_size) {                                                                                     \
            if (self->_dtor && self->_size > 0) {                                                                      \
                for (size_t i = self->_size; i > nsize; --i) {                                                         \
                    (self->_dtor)((void *)((self->_data)[i - 1]));                                                     \
                }                                                                                                      \
            }                                                                                                          \
            self->_size = nsize;                                                                                       \
            return SUCCESS;                                                                                            \
        }                                                                                                              \
        CHECK(!vec_##T##_reserve(self, nsize), return FAILURE);                                                        \
        for (size_t i = self->_size; i < nsize; ++i) {                                                                 \
            self->_data[i] = (T){0};                                                                                   \
        }                                                                                                              \
        self->_size = nsize;                                                                                           \
        return SUCCESS;                                                                                                \
    }
#define RESIZE_GENERIC(T) VECTOR(T) * : vec_##T##_resize,
#define RESIZE(self, size) _Generic(&self, VECTOR_TYPES(RESIZE_GENERIC) default: NULL)(&self, size)

#define POP_BACK_IMPL(T)                                                                                               \
    void vec_##T##_pop_back(vec_##T *self) {                                                                           \
        CHECK(!self || !self->_size, return);                                                                          \
        if (self->_dtor) {                                                                                             \
            (self->_dtor)((void *)((self->_data)[self->_size - 1]));                                                   \
        }                                                                                                              \
        --(self->_size);                                                                                               \
    }
#define POP_BACK_GENERIC(T) VECTOR(T) * : vec_##T##_pop_back,
#define POP_BACK(self) _Generic(&self, VECTOR_TYPES(POP_BACK_GENERIC) default: NULL)(&self)

#define PUSH_BACK_IMPL(T)                                                                                              \
    int vec_##T##_push_back(vec_##T *self, T element) {                                                                \
        CHECK(!self, return FAILURE);                                                                                  \
        if (self->_capacity < self->_size + 1) {                                                                       \
            CHECK(!vec_##T##_reserve(self, self->_size + 1), return FAILURE);                                          \
        }                                                                                                              \
        self->_data[self->_size] = element;                                                                            \
        ++(self->_size);                                                                                               \
        return SUCCESS;                                                                                                \
    }
#define PUSH_BACK_GENERIC(T) VECTOR(T) * : vec_##T##_push_back,
#define PUSH_BACK(self, element) _Generic(&self, VECTOR_TYPES(PUSH_BACK_GENERIC) default: NULL)(&self, element)

#ifdef VECTOR_TYPES
#ifdef VECTOR_IMPL
VECTOR_TYPES(DECL_VEC);
VECTOR_TYPES(RESERVE_IMPL);
VECTOR_TYPES(RESIZE_IMPL);
VECTOR_TYPES(DELETE_IMPL);
VECTOR_TYPES(POP_BACK_IMPL);
VECTOR_TYPES(PUSH_BACK_IMPL);
#endif
#endif
