//
//  ConstantDefine.h
//  SquareCam
//
//  Created by  on 12-5-6.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#ifndef SquareCam_ConstantDefine_h
#define SquareCam_ConstantDefine_h

#define kINF 100000000

// 用于实现MOD运算
#define kMaxSignalNumMask 0X0FF
enum {
    kMaxSignalNum = 256,
    kMinSigNum = 60,
};

#define kFFTPointsNum   2048
#define kFFTPointsNumHalf 1024
// Buffer包含实数部分和虚数部分
#define kFFTPointsBufferLen   4096

//#define kFFTPointsNum   1024
//#define kFFTPointsNumHalf 512
//// Buffer包含实数部分和虚数部分
//#define kFFTPointsBufferLen   2048

#define kLastingNums 12
#define kVideoImageWidth 640
#define kVideoImageHeight 480
#endif
