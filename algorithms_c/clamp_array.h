/**
 * clamp_array.h
 * Clamps an array of values.
 * Has options for SIMD-enabled functionality.
 *
 * NOTE:
 *
 * For SIMD functions, those with (_mm256) in their names,
 * use aligned alloc to 32 for the fastest result.
 */

#pragma once

#include <immintrin.h>

#define _CLAMP_ARRAY_NOSIMD_IMPL(TYPE)                                                             \
    do {                                                                                           \
        if (!arr) {                                                                                \
            return;                                                                                \
        }                                                                                          \
        for (size_t i = 0; i < size; ++i) {                                                        \
            TYPE v = arr[i];                                                                       \
            if (v < min_v) {                                                                       \
                arr[i] = min_v;                                                                    \
            } else if (v > max_v) {                                                                \
                arr[i] = max_v;                                                                    \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define _CLAMP_ARRAY_SIMD_IMPL(TYPE, INSTRUC_SUFFIX, TYPE_SHRT_CLMP, SIMD_TYPE)                    \
    do {                                                                                           \
        if (!arr) {                                                                                \
            return;                                                                                \
        }                                                                                          \
        const size_t stride = 256 / (sizeof(TYPE) * 8);                                            \
        const size_t lft_ov = size & (stride - 1);                                                 \
        size_t i = 0;                                                                              \
        const SIMD_TYPE min_bdcst = _mm256_set1_##INSTRUC_SUFFIX(min_v);                           \
        const SIMD_TYPE max_bdcst = _mm256_set1_##INSTRUC_SUFFIX(max_v);                           \
        for (; i <= size - stride; i += stride) {                                                  \
            SIMD_TYPE flts = _mm256_loadu_##INSTRUC_SUFFIX(&arr[i]);                               \
            const SIMD_TYPE flts_min = _mm256_min_##INSTRUC_SUFFIX(flts, max_bdcst);               \
            const SIMD_TYPE flts_max = _mm256_max_##INSTRUC_SUFFIX(flts_min, min_bdcst);           \
            _mm256_storeu_##INSTRUC_SUFFIX(&arr[i], flts_max);                                     \
        }                                                                                          \
        if (lft_ov) {                                                                              \
            clamp_array_##TYPE_SHRT_CLMP(arr + i, min_v, max_v, lft_ov);                           \
        }                                                                                          \
    } while (0)

static inline void clamp_array_f(float *arr, float min_v, float max_v, size_t size) {
    _CLAMP_ARRAY_NOSIMD_IMPL(float);
}

static inline void clamp_array_d(double *arr, double min_v, double max_v, size_t size) {
    _CLAMP_ARRAY_NOSIMD_IMPL(double);
}

static inline void clamp_array_mm256u_f(float *arr, float min_v, float max_v, size_t size) {
    _CLAMP_ARRAY_SIMD_IMPL(float, ps, f, __m256);
}

static inline void clamp_array_mm256u_d(double *arr, double min_v, double max_v, size_t size) {
    _CLAMP_ARRAY_SIMD_IMPL(double, pd, d, __m256d);
}
