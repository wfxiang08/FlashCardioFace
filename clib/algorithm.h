//
//  algorithm.h
//  MacCardioFace
//
//  Created by Fei Wang on 12-6-3.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#ifndef MacCardioFace_algorithm_h
#define MacCardioFace_algorithm_h

#define IS_DEBUG

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "ConstantDefine.h"
#include "AS3.h"

#ifndef TTDASSERT
#define TTDASSERT(p)  assert(p)
#endif


#ifndef __VDSP__
struct DSPSplitComplex {
    float *             realp;
    float *             imagp;
};
typedef struct DSPSplitComplex          DSPSplitComplex;
#endif

typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;
typedef int  int32_t;
typedef long long int64_t;

#ifndef NULL
#define NULL 0
#endif

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi */
#endif

#ifndef _OBJC_OBJC_H_
typedef signed char BOOL;
#endif

enum {
    NO = 0, YES = 1, FALSE = 0, TRUE = 1
};

#ifndef CGGEOMETRY_H_
struct CGPoint {
    float x;
    float y;
};
typedef struct CGPoint CGPoint;

/* Sizes. */

struct CGSize {
    float width;
    float height;
};
typedef struct CGSize CGSize;

/* Rectangles. */

struct CGRect {
    CGPoint origin;
    CGSize size;
};
typedef struct CGRect CGRect;
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// 算法中的回调函数
void callbackRunningStatusChanged(BOOL newStatus);
void callbackUpdateHeartrate(double hr);
void callbackUpdateFinalHeartrate(double hr);
void callbackUpdateRealTimeCurve(int count);
void callbackUpdateFFTCurve(const float* psd, int psdNum, const float localMax);

#endif
