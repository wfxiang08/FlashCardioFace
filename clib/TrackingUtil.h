//
//  TrackingUtil.h
//  CardioFace
//
//  Created by 老师 无 on 4/20/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SquareCam_TrackingUtil_h
#define SquareCam_TrackingUtil_h


#include "math.h"
#include "algorithm.h"

#define kFeatureNum 10
#define kXBound 25
#define kYBound 25
// ARGB --> BGRA
typedef struct _BGRAColor {
	uint8_t alpha;
	uint8_t red;
    uint8_t green;
	uint8_t blue;
} BGRAColor;

// BGRA(输入为uint32_t)
#define RED_COMP(i)  ((int)(((i) >> 8) & 0xFF))


// 在给定的Image的ROI上做Gradient
void image_gradient_color_x2(const BGRAColor* matrix,
                             const int imgWidth, const int imgHeight, const int X, const int Y,
                             const int width, const int height, float* dx, float* dy);

// 在给定的"灰色"图上做Gradient
void image_gradient_x2(const uint8_t* matrix, const int width, const int height, float* dx, float* dy);

/**
 * 输入为uint8的图像, 输出的梯度dx, dy为整数，且结果为正常值的2倍(x2), 避免了0.5系数相乘
 */
void image_gradient_int_x2(const uint8_t* matrix, const int width, const int height, int* dx, int* dy);


// Deprecated
void image_gussian_filter(float* img, float* retMatrix, const int imgWidth, const int imgHeight,
                          const int winLength);

/**
 * 1. 在tracking_start中使用，输入为dx.*dx, dy.*dy, dx.*dy, 都为整数，且范围在+/255^2,
 * 2. 整数版本的Gussian Filter和正常的相比，少了1/255.0因子，因此导致输出结果的范围在: +/-255^3(24-bit)
 */
void image_gussian_filter_5(int32_t* inData, int32_t* outData, int32_t* buffer,
                            const int imgWidth, const int imgHeight);

// 2*2的矩阵求逆转
void matrix22_inv(const float* matrix, float* retMatrix);

#endif
