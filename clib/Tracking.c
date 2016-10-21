//
//  Tracking.m
//  CardioFace
//
//  Created by 老师 无 on 4/20/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Tracking.h"
#include <time.h>

FeaturePoints _featurePoints;

static int _imgWidth;
static int _imgHeight;
static int _fullImageSize; // _imgWidth * _imgHeight
static int _x;
static int _y;
static int _width;
static int _height;

static int _roiImageRecStartX;
static int _roiImageRecStartY;
static int _roiImageRecWidth;
static int _roiImageRecHeight;

// 内存Buffer
static BOOL   _isInitialized = NO;

// 内存始终保持分配状态
uint32_t* _prevImage = NULL;
uint32_t* _currentImage = NULL;
uint32_t* _pixels = NULL;


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 内存管理部分
// 4-byte对齐
static float* _memory_pool = NULL;
static int memory_size = 0;
static int memory_top = 0;

static inline void* pool_alloc(int size) {
    // 内存总是够用
    TTDASSERT(memory_top + size < memory_size);

    // size = ((size  + 3) >> 2) << 2; // align to 4-bytes ready align to 4-bytes by type of float

    float* result = _memory_pool + memory_top;
    memory_top += size;

    //printf("MaxTop: %d ==> %dk ==> %.3fM\n", memory_top, memory_top >> 10, (float)memory_top / (1024 * 1024));
    return result;
}



static inline void pool_restore_checkpoint(int checkpoint) {
    TTDASSERT(checkpoint >= 0 && checkpoint < memory_size);
    memory_top = checkpoint;
}

static inline int pool_get_top() {
    return memory_top;
}

static inline void swap_buffer_images() {
    // 只是交换引用
    uint32_t* tmp = _currentImage;
    _currentImage = _prevImage;
    _prevImage = tmp;
}


void tracking_roi_image_rec(const int x, const int y, const int width, const int height) {
    _roiImageRecStartX = x;
    _roiImageRecStartY = y;
    _roiImageRecWidth = width;
    _roiImageRecHeight = height;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 分配必须的内存
BOOL tracking_initialize(const int imageWidth, const int imageHeight) {
    if (_isInitialized) {
        return YES;
    }

    _imgWidth = imageWidth;
    _imgHeight = imageHeight;

    const int image_size = imageWidth * imageHeight;
    const int buffer_num = 3 /* ImageBuffer */ + 9;

    _memory_pool = (float*) malloc(sizeof(float) * image_size * buffer_num);

    if (!_memory_pool) {
        // 内存分配失败
        return NO;
    }
    memory_size = image_size * buffer_num;
    memory_top = 0;

    _fullImageSize = image_size;
    _prevImage    = (uint32_t*)pool_alloc(image_size);
    _currentImage = (uint32_t*)pool_alloc(image_size);
    _pixels = (uint32_t*)pool_alloc(image_size);
    _isInitialized = YES;
    return YES;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * 从输入的Image(imageData, imgWidth, imgHeight)的ROI(x, y, width, height)中获取Feature Points
 */
void tracking_start(uint32_t * const imageData, const int imgWidth, const int imgHeight, CGRect roiRect) {

    TTDASSERT(_isInitialized);
    TTDASSERT(_imgWidth == imgWidth);
    TTDASSERT(_imgHeight == imgHeight);
    int i, k;

    // time_t start = time(NULL);

    // double difftime(time_t time1, time_t time0);
    const int check_point = pool_get_top();

    // clock_t firTime = clock();

    _x = (int)roiRect.origin.x;
    _y = (int)roiRect.origin.y;
    _width = (int)roiRect.size.width;
    _height = (int)roiRect.size.height;

    {
        // TODO: 可能之需要拷贝部分图片，例如: 用户可见的区域

        // memcpy(_prevImage, imageData, sizeof(int32_t) * _fullImageSize);
        // 只拷贝感兴趣的区域 (时间: 2/3 find_feature_points ==> 2/7 (采用: vDSP_mmov))
        int startOffset = (_roiImageRecStartY - kMaxAccumOffset) * _imgWidth + (_roiImageRecStartX - kMaxAccumOffset);
//        vDSP_mmov((float*)imageData + startOffset,
//                  (float*)_prevImage + startOffset,
//                  _roiImageRecWidth + (kMaxAccumOffset << 1),
//                  _roiImageRecHeight + (kMaxAccumOffset << 1), _imgWidth, _imgWidth);
        
        const int roiImageRectCopyWidth = _roiImageRecWidth + (kMaxAccumOffset << 1);
        const int roiImageRectCopyHeight = _roiImageRecHeight + (kMaxAccumOffset << 1);
        float* source1 = (float*)imageData + startOffset;
        float* dest1 = (float*)_prevImage + startOffset;
        for (i = 0; i < roiImageRectCopyHeight; i++) {
            memcpy(dest1, source1, roiImageRectCopyWidth * sizeof(float));
            dest1 += _imgWidth;
            source1 += _imgWidth;
        }
    }

    // AS3_Trace(AS3_String("Before Feature Finding"));

    // time_t start1 = time(NULL);
    // AS3_Trace(AS3_Int(difftime(start1, start) * 1000000));

    // 2. 拷贝从(x, y)开始的ROI给_newImage
    uint8_t* const trackingROI = pool_alloc(((_height * _width) + 3) >> 2);

    int offset = _y * imgWidth + _x;
    int offsetNew = 0;
    for (i = 0; i < _height; ++i) {
        for (k = 0; k < _width; ++k) {
            // 只取红色分量
            trackingROI[offsetNew++] = ((_prevImage[offset + k] >> 16)& 0xFF);
        }
        offset += imgWidth;
    }

    // 3. 寻找特征点
    // ROI上数据的拷贝，相比后面频繁的结构体操作要省时间
    _featurePoints.nFeatures = 0;
    tracking_find_features(trackingROI, _width, _height, kFeatureNum, kXBound, kYBound);

    // AS3_Trace(AS3_String("End of Feature Finding"));
    // start1 = time(NULL);
    // AS3_Trace(AS3_Int(difftime(start1, start) * 1000000));

#ifdef IS_TEST_CASE
    draw_rect(imageData, imgWidth, imgHeight, roiRect);
#endif

    // 4. 特征点的坐标还原
    for (i = 0; i < _featurePoints.nFeatures; ++i) {

        _featurePoints.column[i] += _x;
        _featurePoints.row[i]    += _y;

#ifdef IS_TEST_CASE
        draw_point(imageData, _imgWidth, _imgHeight, (int)_featurePoints.column[i], (int)_featurePoints.row[i]);
#endif
        // printf("Feature %d: %f %f\n", i, _featurePoints.row[i], _featurePoints.column[i]);
    }

//    printf("Total time for finding %d features: %f\n", _featurePoints.nFeatures,
//           (clock() - firTime) / (float)CLOCKS_PER_SEC);

    pool_restore_checkpoint(check_point);

}

/**
 * 在给定的Image上绘制一个矩形框
 */
static void draw_rect_1(uint32_t* inputImgData, const int imgWidth, const int imgHeight,
                      int startX, int endX, int startY, int endY,
                      int32_t lineColor) {
	int i;
    // BGRA
    for (i = startX; i <= endX; i++) {
        inputImgData[startY * imgWidth + i] = lineColor;
    }
    for (i = startX; i <= endX; i++) {
        inputImgData[endY * imgWidth + i] = lineColor;
    }

    for (i = startY; i <= endY; i++) {
        inputImgData[i * imgWidth + startX] = lineColor;
    }
    for (i = startY; i <= endY; i++) {
        inputImgData[i * imgWidth + endX] = lineColor;
    }
}

/**
 * 在给定的Image上绘制一个矩形框
 */
void draw_rect(uint32_t* inputImgData, const int imgWidth, const int imgHeight, CGRect rect) {
    int startY = MAX((int)rect.origin.y, 0);
    int endY   = MIN((int)(rect.origin.y + rect.size.height), imgHeight - 1);
    int startX = MAX((int)rect.origin.x, 0);
    int endX   = MIN((int)(rect.origin.x + rect.size.width),  imgWidth -1);

    // // BGRA
    const int32_t lineColor = 0x0000FFFF;
    draw_rect_1(inputImgData, imgWidth, imgHeight, startX, endX, startY, endY, lineColor);

}

/**
 * 在给定的Image上绘制一个矩形框代表点
 */
void draw_point(uint32_t* inputImgData, const int imgWidth, const int imgHeight, const int x, const int y) {
    const int radius = 3;

    int startX = MAX(x - radius, 0);
    int endX   = MIN(x + radius, imgWidth - 1);

    int startY = MAX(y - radius, 0);
    int endY   = MIN(y + radius, imgHeight - 1);

    // 绘制一个红色的小框
    const int32_t pointColor = 0x0000FFFF;
    draw_rect_1(inputImgData, imgWidth, imgHeight, startX, endX, startY, endY, pointColor);

}

// 将_featurePoints按照value从小到大排列(ascending)
static int feature_compare(const void *a, const void *b) {
    float result = _featurePoints.value[*(const int32_t*)a] - _featurePoints.value[*(const int32_t*)b];
    if (result < 0) {
        return -1;

    } else if (result == 0) {
        return 0;

    } else {
        return 1;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL tracking_step(uint32_t * const inputImgData, const int width, const int height,
                   float* trackedX, float* trackedY, const int frame_idx) {

    TTDASSERT(_isInitialized);

    int i, j;
    const int checkpoint = pool_get_top();
    BOOL success = NO;

    // clock_t step_start = clock();

    // 拷贝:
    int nFeatures = _featurePoints.nFeatures;
    float * const rows =   pool_alloc(kMaxFeatureNum);
    float * const colums = pool_alloc(kMaxFeatureNum);
    if (nFeatures > 0) {
        // rows, columns, _featurePoints.row/column的内存必须连续
        memcpy(rows, _featurePoints.row, kMaxSignalNum << 3); // sizeof (float) * kMaxFeatureNum * 2
    }

    const int kWinSize     = 5;
    const int kWinSize2    = kWinSize << 1;
    const int kWinSize_2_2 = kWinSize2 * kWinSize2;

    // MOST TIME Consuming?
    // 在BoudingBox周围计算
    float* const dx = pool_alloc(_fullImageSize);
    float* const dy = pool_alloc(_fullImageSize);
    char msg[200];
    // ROI区域的dx, dy的求取
    {
        float minX = 10000000, maxX = -1000000, minY = 1000000, maxY = -10000000;
        for (i = 0; i < _featurePoints.nFeatures; i++) {
            float x = _featurePoints.column[i];
            float y = _featurePoints.row[i];

            if (x < minX) {
                minX = x;
            }
            if (x > maxX) {
                maxX = x;
            }
            if (y < minY) {
                minY = y;
            }
            if (y > maxY) {
                maxY = y;
            }
        }


        int minX1 = floor(minX) - 20;  minX1 = MAX(minX1, 0);
        int minY1 = floor(minY) - 20;  minY1 = MAX(minY1, 0);

        int maxX1 = ceil(maxX) + 20;   maxX1 = MIN(maxX1, _imgWidth  - 1);
        int maxY1 = ceil(maxY) + 20;   maxY1 = MIN(maxY1, _imgHeight - 1);


        image_gradient_color_x2((BGRAColor*)_prevImage, _imgWidth, _imgHeight,
                                minX1, minY1, maxX1 - minX1 + 1, maxY1 - minY1 + 1,
                                dx, dy);

    }


    // 只拷贝感兴趣的区域
    int startOffset = (_roiImageRecStartY - kMaxAccumOffset) * _imgWidth + (_roiImageRecStartX - kMaxAccumOffset);
//    vDSP_mmov((float*)inputImgData + startOffset,
//              (float*)_currentImage + startOffset,
//              _roiImageRecWidth + (kMaxAccumOffset << 1),
//              _roiImageRecHeight + (kMaxAccumOffset << 1), _imgWidth, _imgWidth);
    
    const int roiImageRectCopyWidth = _roiImageRecWidth + (kMaxAccumOffset << 1);
    const int roiImageRectCopyHeight = _roiImageRecHeight + (kMaxAccumOffset << 1);
    float* source1 = (float*)inputImgData + startOffset;
    float* dest1 = (float*)_currentImage + startOffset;
    for (i = 0; i < roiImageRectCopyHeight; i++) {
        memcpy(dest1, source1, roiImageRectCopyWidth * sizeof(float));
        dest1 += _imgWidth;
        source1 += _imgWidth;
    }

    // 2. 跟踪所有的恶Feature Point
    for (j = 0; j < _featurePoints.nFeatures; ++j) {

        const int checkpoint_forloop = pool_get_top();

        // FOR 2: 图片差值(3 * 100)个点的差值
        float * const W1     = pool_alloc(kWinSize_2_2);
        float * const Wprime = pool_alloc(kWinSize_2_2);

        float * const dx1 = pool_alloc(kWinSize_2_2);
        float * const dy1 = pool_alloc(kWinSize_2_2);

        // 在现有情况下，所有的dx1, dy1都应该能找到有效的差值

        const float xCenter = colums[j];
        const float yCenter = rows[j];
        // 通过图片的二维差值计算FeaturePoints的Value
        // TODO: 将float运算转换为int运算
        image_interp2_color((const BGRAColor*)_prevImage,  W1, _imgWidth, _imgHeight, kWinSize, xCenter, yCenter);
        image_interp2(dx, dx1, _imgWidth, _imgHeight, kWinSize, xCenter, yCenter);
        image_interp2(dy, dy1, _imgWidth, _imgHeight, kWinSize, xCenter, yCenter);


        // FOR 3: SigX1, SigY1, SigXY1
        float SigX1 = 0, SigY1 = 0, SigXY1 = 0;
        for (i = 0; i < kWinSize_2_2; ++i) {
            SigX1  += dx1[i] * dx1[i];
            SigY1  += dy1[i] * dy1[i];
            SigXY1 += dx1[i] * dy1[i]; // X4
        }


        // 可能出现这样的Case:
        // 1. 图片太亮，以至于RGB饱和，会导致这部分的图片的梯度为0, 从而导致矩阵计算出现问题
        // 2. 其他情况? 特征点周围变化太小
        if (fabs(SigX1 * SigY1 - SigXY1 * SigXY1) < 1e-4) {

            // AS3_Trace(AS3_String("_featurePoints matrix invser failed"));

            _featurePoints.value[j] = kINF;
            nFeatures--;
            pool_restore_checkpoint(checkpoint_forloop); //　注意checkpoint的管理
            continue;
        }
        const float M[4] = {SigX1, SigXY1,
                            SigXY1, SigY1};
        float inv_M[4];
        matrix22_inv(M, inv_M); // 1/4


        float oldScore = kINF;
        float stopScore = 9999;
        float xTemp = 0, yTemp = 0;
        int count = 1;


        while (stopScore > 0.1 && count < 40) {

            // Wprime应该不会超出有效的图片区域
            image_interp2_color((const BGRAColor*)_currentImage, Wprime, _imgWidth, _imgHeight,
                                kWinSize, xCenter + xTemp, yCenter + yTemp);


            float SigXIT = 0;
            float SigYIT = 0;
            float newScore = 0.0;
            for (i = 0; i < kWinSize_2_2; ++i) {

                float tmp = Wprime[i] - W1[i]; // Matrix It
                SigXIT += dx1[i] * tmp;
                SigYIT += dy1[i] * tmp;  // X2

                newScore += tmp * tmp;
            }


            _featurePoints.value[j] = newScore;

            if (oldScore < newScore){
                _featurePoints.value[j] = oldScore;
                break;
            }

            stopScore = fabsf(oldScore - newScore);
            oldScore = newScore;

            // 计算UVMat
            float u = - (inv_M[0] * SigXIT  + inv_M[1] * SigYIT) * 2;
            float v = - (inv_M[2] * SigXIT  + inv_M[3] * SigYIT) * 2; // 1/2


            xTemp += u;
            yTemp += v;

            count++;
        }

        // 如果oldScore过大，则表示匹配失败
        if (fabs(xTemp) < kWinSize && fabs(yTemp) < kWinSize && oldScore < 250000) {
            // 如果匹配成功，则更新OffSet
            colums[j] += xTemp;
            rows[j]   += yTemp;

        }  else {
            _featurePoints.value[j] = kINF;
            nFeatures--;
        }

        pool_restore_checkpoint(checkpoint_forloop); // Forloop内存控制

    }

    //　如果匹配到的个数达到Half, 则认为成功
    if (_featurePoints.nFeatures <= (nFeatures << 1)) {
        success = YES;

        // ascending order
        int32_t* feature_idxs = (int32_t*)pool_alloc(_featurePoints.nFeatures);
        for (i = 0; i < _featurePoints.nFeatures; i++) {
            feature_idxs[i] = i;
        }

        // 排序后, feature_idxs中对应的元素的下标按照_feature中的value从小到大排列
        qsort (feature_idxs, _featurePoints.nFeatures, sizeof(int32_t), &feature_compare);

        // 或者使用中位数
        int centerIdx = _featurePoints.nFeatures >> 1; // 中间点
        int new_idx = feature_idxs[centerIdx];
        // 需要同时存在两个版本的: rows, columns
        float tmpX = colums[new_idx] - _featurePoints.column[new_idx];
        float tmpY = rows[new_idx]   - _featurePoints.row[new_idx];

        // 实现round操作
        *trackedX = tmpX;
        *trackedY = tmpY;

        // 更新状态
        if (_featurePoints.nFeatures > 0) {
            // rows, columns, _featurePoints.row/column的内存必须连续
            memcpy(_featurePoints.row,  rows,  kMaxFeatureNum << 3);    // sizeof (float) * kMaxFeatureNum * 2
        }

    } else {

        // 匹配失败??
        // printf("Tracking Failed...\n");
        success = NO;
    }

    // 保存上一幅图片
    swap_buffer_images();

    // printf("%d tracked! Total time: %.4f\n", nFeatures, (clock() - step_start) / (float)CLOCKS_PER_SEC);

    pool_restore_checkpoint(checkpoint);
    return success;
}


/**
 * 计算特征点
 */
BOOL tracking_find_features(uint8_t* roiImageData, const int roiWidth, const int roiHeight,
                            const int featureNum, const int xBound, const int yBound) {

    TTDASSERT(_isInitialized);
    TTDASSERT(featureNum < kMaxFeatureNum);

    int i;
    const int checkpoint = pool_get_top();

    const int image_size = roiWidth * roiHeight;

    int32_t* const dx   = pool_alloc(image_size);
    int32_t* const dy   = pool_alloc(image_size);
    int32_t* const IxIy = pool_alloc(image_size);
    float* const H      = pool_alloc(image_size);
    // 算法部分
    // 1. 首先计算梯度
    image_gradient_int_x2(roiImageData, roiWidth, roiHeight, dx, dy);   // 2.2%


    // 2. 计算梯度的平方 %7.7
    // 注意下面三个函数的顺序
    for (i = 0; i < image_size; ++i) {
        IxIy[i] = dx[i] * dy[i]; // 范围: +- 0~255^2
        dx[i]   = dx[i] * dx[i];
        dy[i]   = dy[i] * dy[i];
    }

    // 3. 高斯滤波
    int32_t* const _buffer_SigX2 = pool_alloc(image_size);
    int32_t* const _buffer_SigY2 = pool_alloc(image_size);
    int32_t* const _buffer_SigXY = pool_alloc(image_size);
    int32_t* const buffer = pool_alloc(image_size);

    // TODO: 优化高斯滤波
    image_gussian_filter_5(dx,   _buffer_SigX2, buffer, roiWidth, roiHeight); // 范围: +- 255^3
    image_gussian_filter_5(dy,   _buffer_SigY2, buffer, roiWidth, roiHeight);
    image_gussian_filter_5(IxIy, _buffer_SigXY, buffer, roiWidth, roiHeight);

    // H的相对大小是关键，不在乎绝对大小
    for (i = 0; i < image_size; ++i) {
        int64_t sum  = (int64_t)(_buffer_SigX2[i] + _buffer_SigY2[i]);
        int64_t tmp1 = (int64_t) _buffer_SigX2[i] * (int64_t) _buffer_SigY2[i]; // 2^24 --> 2^48
        int64_t tmp2 = (int64_t) _buffer_SigXY[i] * (int64_t) _buffer_SigXY[i];
        int64_t result = 25 * (tmp1 - tmp2) - sum * sum;
        if (result < 0) {
            result = -result;
        }
        H[i] = (float)result; // 丢失精度没有关系
    }


    _featurePoints.nFeatures = 0;

    int iC;
    int iR;

    for (i = 0; i < featureNum; ++i) {

        // FOR 1. 求矩阵H的最大值, 一级对应的坐标(iR, iC)
        // unsigned long maxIdx;
        float maxH = -kINF;
//        vDSP_maxvi(H, 1, &maxH, &maxIdx, image_size);     // 43.4% ==> 6.0%
//
//        iR = maxIdx / roiWidth;
//        iC = maxIdx % roiWidth;

        int offset = 0;
        int a, b;
        for (a = 0; a < roiHeight; ++a) {
            for (b = 0; b < roiWidth; ++b) {
                if (H[offset + b] > maxH) {
                    maxH = H[offset + b];
                    iR = a;
                    iC = b;
                }
            }
            offset += roiWidth;
        }

        // FOR 2. 准备清空iR, iC周围的H矩阵
        // FOR 2. 准备清空iR, iC周围的H矩阵
        int xF = MAX(iC - xBound, 0);
        int xE = MIN(iC + xBound, roiWidth - 1);

        int yF = MAX(iR - yBound, 0);
        int yE = MIN(iR + yBound, roiHeight - 1);

        // FOR 3. 将iR, iC周围的点给设置为-1
        offset = yF * roiWidth;
        int y, x;
        for (y = yF; y <= yE; ++y) {
            for (x = xF; x <= xE; ++x) {
                H[offset + x] = -1;
            }
            offset += roiWidth;
        }

        //  FOR 4. 记录FeaturePoints
        _featurePoints.row[i] = iR;
        _featurePoints.column[i] = iC;
        _featurePoints.value[i] = kINF;
    }
    _featurePoints.nFeatures = featureNum;

    pool_restore_checkpoint(checkpoint);
    return YES;
}


/**
 * 参考:interp2.m#linear(nargin=4: ExtrapVal, image, meshgridX, meshgridY)
 */
void image_interp2(const float* img, float* retImg, const int imgWidth, const int imgHeight,
                   const int kWinSize, const float xCenter, const float yCenter) {

    const int xCenterInt = (int)xCenter;  // 肯定为正数
    const int yCenterInt = (int)yCenter;  // 肯定为正数

    const float xCenterFrac = xCenter - xCenterInt;
    const float yCenterFrac = yCenter - yCenterInt;
    const float xCenterFrac_1 = 1 - xCenterFrac;
    const float yCenterFrac_1 = 1 - yCenterFrac;

    const int yStart = yCenterInt - kWinSize;
    const int yEnd = yCenterInt + kWinSize;
    const int xStart = xCenterInt - kWinSize;
    const int xEnd = xCenterInt + kWinSize;

    int destIndex = 0;
    int i, j;

    for (i = yStart; i < yEnd; i++) { // y
        for (j = xStart; j < xEnd; j++) {
            const int ndxi = i * imgWidth + j;
            // _row[i'] = i + yCenterFrac
            // _col[i'] = j + yCenterFrac
            // 通过４个元素来差值
            // i <==> (a, b)
            //        (a,     b) (a + 1,     b)
            //        (a, b + 1) (a + 1, b + 1)

            // 上下两行行间差值
            float tmp1 = img[ndxi]     * yCenterFrac_1 + img[ndxi + imgWidth    ] * yCenterFrac;
            float tmp2 = img[ndxi + 1] * yCenterFrac_1 + img[ndxi + imgWidth + 1] * yCenterFrac;

            // 同一行内部的差值
            retImg[destIndex++] = tmp1 * xCenterFrac_1 + tmp2 * xCenterFrac;
        }
    }
}

void image_interp2_color(const BGRAColor* const img, float* retImg,
                         const int imgWidth, const int imgHeight,           // 原始图片的大小
                         const int kWinSize, const float xCenter, const float yCenter) {


    const int xCenterInt = (int)xCenter;
    const int yCenterInt = (int)yCenter;
    const float xCenterFrac = xCenter - xCenterInt;
    const float yCenterFrac = yCenter - yCenterInt;
    const float xCenterFrac_1 = 1 - xCenterFrac;
    const float yCenterFrac_1 = 1 - yCenterFrac;

    const int yStart = yCenterInt - kWinSize;
    const int yEnd = yCenterInt + kWinSize;
    const int xStart = xCenterInt - kWinSize;
    const int xEnd = xCenterInt + kWinSize;

    int destIndex = 0;
    int i, j;
    for (i = yStart; i < yEnd; i++) { // y
        for (j = xStart; j < xEnd; j++) {
            const int ndxi = i * imgWidth + j;
            // 通过４个元素来差值
            // i <==> (a, b)
            //        (a,     b) (a + 1,     b)
            //        (a, b + 1) (a + 1, b + 1)

            // 上下两行行间差值
            float tmp1 = (int)img[ndxi].red     * yCenterFrac_1 + (int)img[ndxi + imgWidth    ].red * yCenterFrac;
            float tmp2 = (int)img[ndxi + 1].red * yCenterFrac_1 + (int)img[ndxi + imgWidth + 1].red * yCenterFrac;

            // 同一行内部的差值
            retImg[destIndex++] = tmp1 * xCenterFrac_1 + tmp2 * xCenterFrac;
        }
    }
}
