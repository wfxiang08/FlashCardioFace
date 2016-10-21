#ifndef SquareCam_Tracking_h
#define SquareCam_Tracking_h

#include "TrackingUtil.h"
#include "algorithm.h"

#define kMaxFeatureNum 15
#define kMaxAccumOffset 40

typedef struct _FeaturePoints {
    int32_t nFeatures;

    float row[kMaxFeatureNum];
    float column[kMaxFeatureNum];

    float value[kMaxFeatureNum];

} FeaturePoints;


// 非inline的版本不知道为什么总是有warning, So...
extern FeaturePoints _featurePoints;
inline static FeaturePoints* get_feature_points() {
    return &_featurePoints;
}

extern uint32_t* _pixels;
inline static uint32_t* get_pixels_buffer() {
	return _pixels;
}
extern uint32_t* _prevImage;
inline static uint32_t* get_prev_image_data() { // ignore this warning
    return _prevImage;
}

extern uint32_t* _currentImage;
inline static uint32_t* get_current_image_data() { // ignore this warning
    return _currentImage;
}

// 纪录(record)检测相关的信息，例如：图片的大小等
void tracking_roi_image_rec(const int x, const int y, const int width, const int height);

// 进行内存的预先分配
BOOL tracking_initialize(const int imageWidth, const int imgHeight);


BOOL tracking_find_features(uint8_t* roiImageData,
                   const int roiWidth, const int roiHeight, const int featureNum, const int xBound, const int yBound);

// 首先调用: tracking_start, 进行tracking的初始化
void tracking_start(uint32_t * const imageData, const int imgWidth, const int imgHeight, CGRect roiRect);

// 然后再调用: tracking_step来逐步更新tracking过程
BOOL tracking_step(uint32_t * const image, const int width, const int height,
                   float* trackedX, float* trackedY, const int frame_idx);

// 图片处理相关的东西
void image_interp2(const float* img, float* retImg, const int imgWidth, const int imgHeight,
                   const int kWinSize, const float xCenter, const float yCenter);

void image_interp2_color(const BGRAColor* const img, float* retImg, const int imgWidth, const int imgHeight,
                         const int kWinSize, const float xCenter, const float yCenter);

// 在图片上绘制一个点
void draw_point(uint32_t* inputImgData, const int imgWidth, const int imgHeight, const int x, const int y);
void draw_rect(uint32_t* inputImgData, const int imgWidth, const int imgHeight, CGRect rect);

#endif
