// nolint
//  AlgUtils.h
//
#ifndef SquareCam_AlgUtils_h
#define SquareCam_AlgUtils_h

#include "algorithm.h"

#define USE_FFT_2048
#define TWO_PI (6.2831853071795864769252867665590057683943L)

typedef struct {
    int index;
    double maxValue;
} LocalMaxPoints;


static inline int FAST_MOD(int i, const int size) {
    // i的范围[0, size - 1]
    // return (i + size) % size;
    if (i >= size) {
        i -= size;

    } else if (i < 0) {
        i += size;
    }

    TTDASSERT(i >= 0 && i < size);

    return i;
}

static inline float DegreesToRadians(float degrees) {
    return degrees * M_PI / 180;
}

void fft1024(DSPSplitComplex* A, DSPSplitComplex* result1);
void fft2048(DSPSplitComplex* A, DSPSplitComplex* result1);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/** 均值滤波 */
void average_filter(float* inputs, const int len, const int start, const int size, float* outputs);

/** Hanning窗，输入的下标从0开始，输出的下标也从0开始 */
void hanning_filter_no_rotate(const float* x, const int xLen, float* y);

#endif
