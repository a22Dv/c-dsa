/*
    c_prmgen.h
    A minimal permutation generator in C.
*/

#pragma once

#include <stdlib.h>
#include <string.h>

#define _PRM_VLA_LIMIT 2048
#define _PRMREQUIRE(cond, action)                                                                                      \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            action;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define _PRMSWP(a, b, t, es)                                                                                           \
    do {                                                                                                               \
        memcpy(t, a, es);                                                                                              \
        memcpy(a, b, es);                                                                                              \
        memcpy(b, t, es);                                                                                              \
    } while (0)

static inline int next_prm(void *seq, size_t seq_size, size_t elmnt_size, int (*cmp_func)(const void *, const void *)) {
    int ret_code = 1;
    char *pivot = NULL, *successor = NULL, *data = NULL, *a = NULL, *b = NULL;
    void *tmp = NULL;
#if !defined(__STDC_NO_VLA__) && (elmnt_size < _PRM_VLA_LIMIT)
    char bfr[elmnt_size];
#endif
    size_t lpivot = 0;
    _PRMREQUIRE(seq && seq_size && elmnt_size && cmp_func, ret_code = 0; goto end);
#if !defined(__STDC_NO_VLA__) && (elmnt_size < _PRM_VLA_LIMIT)
    tmp = bfr;
#else 
    tmp = malloc(elmnt_size);
    _PRMREQUIRE(tmp, ret_code = 0; goto end);
#endif
    data = (char *)seq;
    for (size_t i = seq_size; i--;) {
        if (i != seq_size - 1 && cmp_func(data + i * elmnt_size, data + (i + 1) * elmnt_size) < 0) {
            lpivot = i;
            pivot = data + i * elmnt_size;
            break;
        }
    }
    _PRMREQUIRE(pivot, ret_code = 0; goto end);
    for (size_t i = seq_size; i--;) {
        if (cmp_func(pivot, data + i * elmnt_size) < 0) {
            successor = data + i * elmnt_size;
            break;
        }
    }
    _PRMSWP(pivot, successor, tmp, elmnt_size);
    for (size_t i = 0; i < (seq_size - lpivot - 1) / 2; ++i) {
        a = data + (seq_size - i - 1) * elmnt_size;
        b = data + ((lpivot + 1) + i) * elmnt_size;
        _PRMSWP(a, b, tmp, elmnt_size);
    }
end:
#if defined(__STDC_NO_VLA__) || (elmnt_size >= _PRM_VLA_LIMIT)
    if (tmp) {
        free(tmp);
    }
#endif
    return ret_code;
}
