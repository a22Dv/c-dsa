/*
    c_dsa_details.h
    Internal implementation detail. Holds utility functions and macros.
*/

#pragma once
#define FAILURE 0
#define SUCCESS 1
#define CHECK(fail_cond, action)                                                                                          \
    if (fail_cond) {                                                                                                     \
        action;                                                                                                        \
    }

static inline size_t _n2exp(size_t n) {
    n > 0 ? --n : 0;
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

