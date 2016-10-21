//
//  AlgUtils.c
//  SquareCam
//
//  Created by  on 12-5-19.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//
#include <math.h>
#include "AlgUtils.h"
#include "Sin_Table.h"


static void fft1024_internal(float *gRe, float *gIm, float *GRe, float *GIm);
static void fft2048_internal(float *gRe, float *gIm, float *GRe, float *GIm);

///////////////////////////////////////////////////////////////////////////////////////////////////
void fft1024(DSPSplitComplex* A, DSPSplitComplex* result1) {
    fft1024_internal(A->realp, A->imagp, result1->realp, result1->imagp);
}
void fft2048(DSPSplitComplex* A, DSPSplitComplex* result1) {
    fft2048_internal(A->realp, A->imagp, result1->realp, result1->imagp);
}

// n = 1024
// inverse == -1表示正向FFT
// inverse ==  1表示反响FFT
static void fft1024_internal(float *gRe, float *gIm, float *GRe, float *GIm) {
	int i, j, k;
	int i1, l1, l2, l;
	float tx, ty;
	float u1, u2, t1, t2, z, ca, sa;
	
	const int l2n = 10;	

	memcpy(GRe, gRe, 4096); // 1024 * sizeof(float)
	memcpy(GIm, gIm, 4096);
	
	j = 0;
	i1 = 512;
	for(i = 0; i < 1023; i++) {
		if(i < j) {
			tx = GRe[i];
			ty = GIm[i];
			GRe[i] = GRe[j];
			GIm[i] = GIm[j];
			GRe[j] = tx;
			GIm[j] = ty;
		}
		k = i1;
		while (k <= j) {
            j -= k; k>>=1;
        }
		j += k;
	}
	
	ca = -1.0;
	sa = 0.0;
	l1 = 1;
	l2 = 1;

	for(l=0; l < l2n; l++) {
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;

		for(j = 0; j < l1; j++) {
			for(i = j; i < 1024; i += l2) {
				i1 = i + l1;
				
				t1 = u1 * GRe[i1] - u2 * GIm[i1];
				t2 = u1 * GIm[i1] + u2 * GRe[i1];
				GRe[i1] = GRe[i] - t1;
				GIm[i1] = GIm[i] - t2;
				GRe[i] += t1;
				GIm[i] += t2;
			}
			z =  u1 * ca - u2 * sa;
			u2 = u1 * sa + u2 * ca;
			u1 = z;
		}

        // sin(2x), cos(2x)
        // sin(x) ==> sqrt((1 - cos(2x)) / 2)
        // cos(x) ==> sqrt((1 + cos(2x)) / 2)
		sa = -sqrt((1.0 - ca) * 0.5);
		ca =  sqrt((1.0 + ca) * 0.5);
	}
}

static void fft2048_internal(float *gRe, float *gIm, float *GRe, float *GIm) {
	int i, j, k;
	int i1, l1, l2, l;
	float tx, ty;
	float u1, u2, t1, t2, z, ca, sa;

	const int l2n = 11;

	memcpy(GRe, gRe, 8192); // 1024 * sizeof(float)
	memcpy(GIm, gIm, 8192);

	j = 0;
	i1 = 1024;
	for(i = 0; i < 2047; i++) {
		if(i < j) {
			tx = GRe[i];
			ty = GIm[i];
			GRe[i] = GRe[j];
			GIm[i] = GIm[j];
			GRe[j] = tx;
			GIm[j] = ty;
		}
		k = i1;
		while (k <= j) {
            j -= k; k>>=1;
        }
		j += k;
	}

	ca = -1.0;
	sa = 0.0;
	l1 = 1;
	l2 = 1;

	for(l=0; l < l2n; l++) {
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;

		for(j = 0; j < l1; j++) {
			for(i = j; i < 2048; i += l2) {
				i1 = i + l1;

				t1 = u1 * GRe[i1] - u2 * GIm[i1];
				t2 = u1 * GIm[i1] + u2 * GRe[i1];
				GRe[i1] = GRe[i] - t1;
				GIm[i1] = GIm[i] - t2;
				GRe[i] += t1;
				GIm[i] += t2;
			}
			z =  u1 * ca - u2 * sa;
			u2 = u1 * sa + u2 * ca;
			u1 = z;
		}

        // sin(2x), cos(2x)
        // sin(x) ==> sqrt((1 - cos(2x)) / 2)
        // cos(x) ==> sqrt((1 + cos(2x)) / 2)
		sa = -sqrt((1.0 - ca) * 0.5);
		ca =  sqrt((1.0 + ca) * 0.5);
	}
}

#pragma mark - 滤波器相关函数
///////////////////////////////////////////////////////////////////////////////////////////////////
void average_filter(float* inputs, const int len, const int start, const int size, float* outputs) {

    double lastNoZero = 0;
    // 防止inputs中出现奇怪的数据?
    // 如何出现的呢?
    int i, k;
    for (i = 0; i < len; i++) {
        const int newIdx = FAST_MOD(start + i, size);
        float input_i = inputs[newIdx];

        if (fabs(input_i) < 0.01) {
            if (fabs(lastNoZero) > 0.01) {
                inputs[newIdx] = lastNoZero;
            }

        } else {
            if (fabs(lastNoZero) < 0.01) {
                lastNoZero = input_i;

                for (k = 0; k < i; k++) {
                    int newIdx1 = FAST_MOD(start + k, size);
                    inputs[newIdx1] = lastNoZero;
                }

            } else {
                lastNoZero = input_i;
            }
        }
    }

    // 5点均值滤波
    // start: [0, size]
    // 五点Average滤波
    double last4Sum = 0;
    double sum1 = 0;
    for (i = 0; i < len; i++) {
        int newIdx = FAST_MOD(start + i, size);
        last4Sum += inputs[newIdx];

        if (i >= 5) {
            newIdx = FAST_MOD(newIdx - 5, size);
            last4Sum -= inputs[newIdx];
            outputs[i] = last4Sum * 0.2;

        } else {
            outputs[i] = last4Sum / (i + 1);
        }
        sum1 += outputs[i];
    }

    sum1 /= len;
    for (i = 0; i < len; i++) {
        outputs[i] = outputs[i] - sum1;
    }

}

/** Hanning窗，输入的下标从0开始，输出的下标也从0开始 */
void hanning_filter_no_rotate(const float* x, const int xLen, float* y) {
    TTDASSERT(xLen > 1);

    float step = 4096.0 / (xLen - 1);
    int i;
    for (i = 0; i  < xLen; i++) {
        float cos_i;
        int idx = (int)i * step;
        GET_COS(idx, cos_i);

        y[i] = x[i] * 0.5 * (1 - cos_i);
    }
}
