//
//  handler.c
//  MacCardioFace
//
//  Created by Fei Wang on 12-6-3.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "handler.h"
#include "algorithm.h"
#include "AlgUtils.h"
#include "Tracking.h"

inline static CGRect CGRectMake(float x, float y, float width, float height) {
    CGRect rect;
    rect.origin.x = x; rect.origin.y = y;
    rect.size.width = width; rect.size.height = height;
    return rect;
}

#define kLast20HeartRates 20

double _maxVsSum;
double _maxVsSecond;
double _heartRate;

int32_t _roiImageStartX;
int32_t _roiImageStartY;
int _roiWidth;
int _roiHeight;

int _frameRate;

char msg[300];

// 主要用于处理光线不稳定的情况
volatile double _lastDetectionStartTime;

// 是否正在检测
volatile BOOL _isRunning = NO;

// 是否是用户要求停下来的(走检测成功流程，方便测试)
volatile BOOL _isAskedToEnd;



double _lastHandleFrameTime;
double _lastHandleFrameInterval;
double _lastHandleFrameStart;

int32_t _startIdx;
int32_t _endIdx;
int32_t _lastSampleNum;

int32_t _freqSampling;
double _fftFreqStep;
double _fftFreqStepInv;


double _lastMaxPower;
float _lastHeartRate;


// 意义: 当前在Buffer中的有意义的点的"个数", 不要一个变量两个用途
int32_t _currentSigNum;
int32_t _frameNumSinceStable;

// 滤波等处理所需要的Buffer
float _greenSignal[kFFTPointsNum];
float _filteredSignal[kFFTPointsNum];
float _detrendedSignal[kFFTPointsNum];

// FFT相关处理的Buffer
// FFT相关处理的Buffer
float _fft_f3[kFFTPointsNumHalf];


// 纪录过去一段时间内的靠谱心率
float _last20HeartRates[kLast20HeartRates]; // 5s左右
float _last20HeartRatesBuffer[kLast20HeartRates]; // 5s左右
int32_t _lastHeartRateIndex;
int32_t _lastHeartRateCount;
int32_t _lastHeartRateSucceedCount;
float _lastHeartRateAvg;

// FFT缓存
unsigned char* _buffer; // 用于存放_fftInput, _fftOuput中的数据
DSPSplitComplex _fftInput;
DSPSplitComplex _fftOutput;

// 局部极大值的处理
LocalMaxPoints _localMaxPoints[40];
int32_t _localMaxPointsNum;
float SNR;


float _roiImagePixelNumInv;



// 用于纪录从开始检测到目前位置，人脸的偏移距离(单次的偏移可以忽略，但是累积却很显著)
float _accumOffsetX;
float _accumOffsetY;

int32_t _frameIndex;
int32_t _lastStableHintIndex;
BOOL _lastStableStatus;

// 是否需要重启Tracking(FeaturePoints丢失，或者亮度变化太大)
BOOL _needRestartTracking;

// 和isRunning的变化一起使用
// isRunning = YES状态的结束分两种情况:
// 1. 成功检测
// 2. 检测超时，失败
BOOL _lastDetectionSucceed;

uint32_t* _640_480_buffer;


float lastDisplayableHeartRate;
double lastDetectionStartTime;

BOOL handler_is_running() {
	return _isRunning;
}
void handler_start_running(BOOL val) {
    _isRunning = val; 
    if (_isRunning) {
        lastDisplayableHeartRate = 0;
    }
    
    handler_reset();
    handler_reset_check_condition();

    // 通知Flash, 运行状态发生改变
    callbackRunningStatusChanged(_isRunning);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void handler_set_framerate(int val) {
    _frameRate = val;
}



static int point_compare(const void *a, const void *b) {
    LocalMaxPoints* pt1 = (LocalMaxPoints*)a;
    LocalMaxPoints* pt2 = (LocalMaxPoints*)b;
    
    // descending
    float result = pt2->maxValue - pt1->maxValue;
    if (result < 0) {
        return -1;
        
    } else if (result == 0) {
        return 0;
        
    } else {
        return 1;
    }
}


/**
 * 设置参数(在算法运转前必须设置参数)
 */
void handler_initialize(int framerate, int startX, int startY, int roiWidth, int roiHeight) {

	_roiImageStartX = startX;
	_roiImageStartY = startY;
    _roiWidth = roiWidth;
    _roiHeight = roiHeight;
    _frameRate = framerate;

	handler_after_configuration();

    _frameNumSinceStable = 0;

    // 初始化内存
    tracking_initialize(kVideoImageWidth, kVideoImageHeight);

    // 预先设置参数
    tracking_roi_image_rec(_roiImageStartX, _roiImageStartY, _roiWidth, _roiHeight);


    // 预先分配内存(注意顺序)
	_buffer = (unsigned char*)malloc(sizeof(float) * kFFTPointsNum * 4);
	float* tmp = (float*) _buffer;
	// 内存的布局固定，在后面可能利用这个关系来处理memcpy, memset等
	_fftInput.realp  = tmp;
	_fftInput.imagp  = _fftInput.realp  + kFFTPointsNum;
	_fftOutput.realp = _fftInput.imagp  + kFFTPointsNum;
	_fftOutput.imagp = _fftOutput.realp + kFFTPointsNum;

}



///////////////////////////////////////////////////////////////////////////////////////////////////
void handler_reset() {
    
    // 重新设置状态
    // memset(_greenSignal, 0, sizeof(_greenSignal));
    
    // 当前的有效的数据点数，以及最近稳定的数据点数
    _currentSigNum = 0;
    _frameNumSinceStable = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void handler_after_configuration() {
    // 设置所有的恶参数
    _roiImagePixelNumInv = 1.0 / (_roiWidth * _roiHeight);
    
    _fftFreqStep = ((float)_frameRate * 60) / kFFTPointsNum;
    _fftFreqStepInv = 1.0 / _fftFreqStep;
    
    _startIdx = (int)(55  * _fftFreqStepInv);
    _endIdx   = (int)(130 * _fftFreqStepInv + 0.5);  // 关键区间: [20, 65]
    
    // 红色框所在的位置
    tracking_roi_image_rec(_roiImageStartX, _roiImageStartY, _roiWidth, _roiHeight);
    
    // 重新从矩形框开始Tracking
    _accumOffsetX = 0;
    _accumOffsetY = 0;
    _frameIndex = 0;
    
    handler_reset_check_condition();
    

    sprintf(msg, "_roiW/H: %d, %d; _startIdx/E: %d, %d, _fftFreqStep: %.3f, _fftFreqStepInv: %.3f, _frameRate: %d ",
    		_roiWidth, _roiHeight, _startIdx, _endIdx, _fftFreqStep, _fftFreqStepInv, _frameRate);
    AS3_Trace(AS3_String(msg));

}

void handler_reset_check_condition() {
    _lastHeartRateIndex = -1;
    _lastHeartRateCount = 0;
    _lastHeartRateSucceedCount = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL handler_handle_frame_internal(uint32_t* const pixel, int width, int height) {
    
    // 只要正在工作，就说明没有没有成功
    _lastDetectionSucceed = NO;
    // 1. 实现Tracking
    handler_tracking(pixel, width, height);
    
    const int current_pos = _currentSigNum % kMaxSignalNum;
    _greenSignal[current_pos] = handler_get_green_sum_Of_ROI(width, pixel);
    
    // 状态立即改变
    _currentSigNum++;
    
    //    // View的坐标系：　原点在左上角(X1, Y1)
    //    // Video的坐标系:　原点在右上角，并且是转置过的 (X2, Y2)
    //    // 在算法中使用的是Video坐标系：宽度为640, 高度为: 480
//#ifdef SHOW_POI_IMAGE
//    if (_handlerDelegate && _currentSigNum % 10 == 0) {
//        // 避免不必要的时间开销, 调试OK后注释掉
//        // 更新facerect的数据
//        TT_RELEASE_SAFELY(_faceImage);
//        _faceImage = [[CardioFaceUtils convertBitmapRGBA8ToNSImage: (unsigned char *) _face_image
//                                                         withWidth: _roiWidth
//                                                        withHeight: _roiHeight] retain];
//        dispatch_async(dispatch_get_main_queue(), ^(void) {
//            [_handlerDelegate updateFaceImage];
//        });
//    }
//#endif
    
    
    // 稳定性检测。计算当前帧的稳定性。用绿色信号当前值减去k帧以前的值。如果绝对值小于M则认为这一帧是稳定的。
    // 在计算心率的时候，需要确保计算范围内每一帧都是稳定的。
    const int kStep = 5; // 5 / 15的意义 = 0.3 s
    float stabilityThreshold = 7.0;
    
    float diff = _greenSignal[current_pos] - _greenSignal[(current_pos + kMaxSignalNum - kStep) % kMaxSignalNum];
    
    // 数据是否稳定:
    BOOL stable = fabsf(diff) < stabilityThreshold;
    
    
    if (!stable) {
        // 如果数据不稳定，则重新开始计数
        _frameNumSinceStable = 0;
        
        // diff过大，则认为???
        if (fabsf(diff) > stabilityThreshold * 2) {
            // _lastDetectionStartTime = [[NSDate date] timeIntervalSince1970];
        }
        
        handler_reset_check_condition();

        AS3_Val msg1 = AS3_String("Not stable, reset....");
        AS3_Trace(msg1);
        AS3_Release(msg1);

        // 通知主线程??
//        if (_handlerDelegate) {
//            _lastSampleNum = 0;
//            // 暂时没有明确的作用:
//            dispatch_async(dispatch_get_main_queue(), ^(void) {
//                [_handlerDelegate updateViewControllerUINOGraph];
//            });
//        }
        
    } else {
        _frameNumSinceStable ++;
    }

    // 从当前帧往前数count帧。
    // _frameNumSinceStable > 5 && _frameNumSinceStable % 2 == 0
    if (_frameNumSinceStable > 5 && ((_frameNumSinceStable & 0x01) == 0)) {
        handler_fftAnalyzeSignalSegWithOrigData(pixel);
        
    } else if (_frameIndex % 5 == 0) {
        // 必须有一定的有效输入数据
        _lastSampleNum = 0;
        // AS3_Trace(AS3_String("Update input data curve...."));
//        [_handlerDelegate updateViewControllerUI];
//        dispatch_async(dispatch_get_main_queue(), ^(void) {
//            [_handlerDelegate updateViewControllerUI];
//        });
    }
    return YES;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
float handler_get_green_sum_Of_ROI(int width, uint32_t * const pixel) {
    
    int poiOffsetX = (int)(_roiImageStartX + _accumOffsetX + 0.5);
    int poiOffsetY = (int)(_roiImageStartY + _accumOffsetY + 0.5);
    
    int poiOffset1 = poiOffsetY * width + poiOffsetX;
    
    int sumGreen = 0;
    int i, k;

    // 100 * 200 * 255 < 2^24 int足够
    for (i = 0; i < _roiHeight; ++i) {
        for (k = 0; k < _roiWidth; k++) {
            
        	// ARGB (tmp从地位到高位分别为: A R G B)
            uint32_t tmp = pixel[poiOffset1 + k];
            uint32_t g1 = (tmp >> 16) & 0xFF;
            sumGreen += g1;
        }
        poiOffset1 += width;
    }
    float greenAvg = sumGreen * _roiImagePixelNumInv;
//    sprintf(msg, "greenAvg: %.2f", greenAvg);
//    AS3_Trace(AS3_String(msg));
    return greenAvg;
}



/**
 * @param pixel: 当前的video frame, BGRA格式
 * @param imageWidth, imageHeight: 为输入video frame的大小， imageWidth = 640, height = 480
 *
 * 处理结果更新到:
 *  1. _accumOffsetX, _accumOffsetY
 *  2. 如果_accumOffsetX, _accumOffsetY过大(Tracking考虑的数据可能会存在边界问题, 后面的算法为了速度放弃了边界处理)，则重新Tracking
 *    偏移过大，则重启计数
 *  3.
 */
void handler_tracking(uint32_t* const pixel, int imageWidth, int imageHeight) {
    

    // sprintf(msg, "_accumOffsetX: %.2f, _accumOffsetY: %.2f", _accumOffsetX, _accumOffsetY);
    // AS3_Trace(AS3_String(msg));

    // XXX: 保证不要移出屏幕(这个很关键, 后面的算法基本没有边界处理)
    if (fabs(_accumOffsetX) > kMaxAccumOffset || fabs(_accumOffsetY) > kMaxAccumOffset) {
        // 重置当前的Tracking状态
        _frameNumSinceStable = 0;
        _accumOffsetX = 0;
        _accumOffsetY = 0;
        
        // 重新开始
        // _lastDetectionStartTime = [[NSDate date] timeIntervalSince1970];
        
        if (_frameIndex - _lastStableHintIndex > 20 * 6) {
            _lastStableHintIndex = _frameIndex;
            _lastStableStatus = !_lastStableStatus;
            //                if (_lastStableStatus) {
            //                    dispatch_async(dispatch_get_main_queue(), ^(void) {
            //                        SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
            //                        msg.messages = [NSArray arrayWithObjects: @"请对准脸部，保持稳定", nil];
            //                        int durations[1] = { 10}; // 2s
            //                        [msg setDurations:durations  andLength: 1];
            //                        [_handlerDelegate showSimpleMsgs: msg];
            //                        
            //                    });
            //                    
            //                } else {
            //                    dispatch_async(dispatch_get_main_queue(), ^(void) {
            //                        SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
            //                        msg.messages = [NSArray arrayWithObjects: @"请对准脸部，不要手抖", nil];
            //                        int durations[1] = { 10}; // 2s
            //                        [msg setDurations:durations  andLength: 1];
            //                        [_handlerDelegate showSimpleMsgs: msg];
            //                        
            //                    });
            //                }
        }
        handler_reset_check_condition();
        
        
    }
    
    
    const float alertThreshold = 16;
    
    if (fabs(_accumOffsetX) > alertThreshold || fabs(_accumOffsetY) > alertThreshold) {
        if (_frameIndex - _lastStableHintIndex > 20 * 4) {
            _lastStableHintIndex = _frameIndex;
            
            _lastStableStatus = !_lastStableStatus;
            //                if (_lastStableStatus) {
            //                    dispatch_async(dispatch_get_main_queue(), ^(void) {
            //                        SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
            //                        msg.messages = [NSArray arrayWithObjects: @"请对准脸部，保持稳定", nil];
            //                        int durations[1] = { 10}; // 2s
            //                        [msg setDurations:durations  andLength: 1];
            //                        [_handlerDelegate showSimpleMsgs: msg];
            //                        
            //                    });
            //                    
            //                } else {
            //                    dispatch_async(dispatch_get_main_queue(), ^(void) {
            //                        SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
            //                        msg.messages = [NSArray arrayWithObjects: @"请对准脸部，不要手抖", nil];
            //                        int durations[1] = { 10}; // 2s
            //                        [msg setDurations:durations  andLength: 1];
            //                        [_handlerDelegate showSimpleMsgs: msg];
            //                        
            //                    });
            //                }
        }
        
    }
    
    // 每5帧重启一次
    if (_needRestartTracking || _frameNumSinceStable == 0) {
        _needRestartTracking = NO;
        
        // 重新开始Tracking
#ifdef IS_DEBUG
        // AS3_Trace(AS3_String("tracking_start...."));
#endif
        tracking_start(pixel, imageWidth, imageHeight,
        		CGRectMake(_roiImageStartX, _roiImageStartY, _roiWidth, _roiHeight));
        // _frameNumSinceStable != 0是进行特征点调整
        if (_frameNumSinceStable == 0) {
            _accumOffsetX = 0;
            _accumOffsetY = 0;
        }

        callbackUpdateFeaturePoints();
        // 如何通知FeaturePoints更新了呢?
        
        //            if (_handlerDelegate) {
        //                dispatch_async(dispatch_get_main_queue(), ^(void) {
        //                    [_handlerDelegate updateFeaturePoints];
        //                });
        //            }
        
    } else if (_frameNumSinceStable % 2 == 0) {
        // 每秒Track 10次，Tracking频率低会无法跟踪到脸部位移
#ifdef IS_DEBUG
        // AS3_Trace(AS3_String("tracking_step...."));
#endif
        float trackedX, trackedY;
        if (tracking_step(pixel, imageWidth, imageHeight, &trackedX, &trackedY, 0)) {
            _accumOffsetX += trackedX;
            _accumOffsetY += trackedY;
            callbackUpdateFeaturePoints();
            //                if (_handlerDelegate) {
            //                    dispatch_async(dispatch_get_main_queue(), ^(void) {
            //                        [_handlerDelegate updateFeaturePoints];
            //                    });
            //                }
            
        } else {
            // 需要重启
            _needRestartTracking = YES;
            
            //                dispatch_async(dispatch_get_main_queue(), ^(void) {
            //                    SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
            //                    msg.messages = [NSArray arrayWithObjects: @"请对准脸部，保持稳定", nil];
            //                    int durations[1] = { 10}; // 2s
            //                    [msg setDurations:durations  andLength: 1];
            //                    [_handlerDelegate showSimpleMsgs: msg];
            //                });
        }


        // sprintf(msg, "_needRestartTracking: %d, _accumOffsetX: %.2f,  _accumOffsetY: %.2f", _needRestartTracking, _accumOffsetX, _accumOffsetY);
        // AS3_Trace(AS3_String(msg));

    }
    
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @param pixel
 *   1. pixel 为当前的video frame, 数据格式为: RGBA
 *   2. 如果算法成功结束，pixel会被拷贝到_prevImage中，以供UI逻辑使用
 */
BOOL handler_fftAnalyzeSignalSegWithOrigData(uint32_t* const pixel) {
    // 必须有一定的有效输入数据
    TTDASSERT(_frameNumSinceStable >= 5);
    
    int count = MIN(_frameNumSinceStable, kMaxSignalNum);
    int greenSigStart = (_currentSigNum - count + kMaxSignalNum) % kMaxSignalNum;
    
    
    // Step 1: 5点均值绿波(OK)(不考虑边界效应，输入和输出的数据长度相同)
    average_filter(_greenSignal, count, greenSigStart, kMaxSignalNum, _detrendedSignal);
    
    // Step 2: 利用均值绿波的结果来Hanning
    hanning_filter_no_rotate(_detrendedSignal, count, _filteredSignal);
    _lastSampleNum = count;
    
    // AS3_Trace(AS3_String("handler_fftAnalyzeSignalSegWithOrigData....."));
    // 以上的_filteredSignal: 可以用于数据的展示
    // Step 3: 如果稳定的数据足够多，则可以开始FFT分析了
    if (_frameNumSinceStable >= kMinSigNum && _frameNumSinceStable % 4 == 0) {
        
    	// AS3_Trace(AS3_String("FFT Analyzer....."));

    	// FFT以及局部最大值分析
        handler_FFTAndLocalMaxium(count);

        
        float localMaxValue = _localMaxPointsNum > 0 ? _localMaxPoints[0].maxValue : 0;
        int localMaxIdx     = _localMaxPointsNum > 0 ? _localMaxPoints[0].index : 0;
        
        // sprintf(msg, "localMaxIdx: %d, localMaxValue: %.2f, count: %d", localMaxIdx, localMaxValue, count);
        // AS3_Trace(AS3_String(msg));

        // 3.6 功率谱有效检测
        _lastMaxPower = localMaxValue;
        if (localMaxIdx > 0 && localMaxValue > 1) {
            // 纪录上次采样的结果
            _lastSampleNum = count;
            _lastHeartRate = (float)(localMaxIdx * _fftFreqStep);
            
            if (_lastHeartRateCount < kLast20HeartRates) {
                _lastHeartRateCount++;
            }
            
            _lastHeartRateIndex = FAST_MOD(_lastHeartRateIndex + 1, kLast20HeartRates);
            _last20HeartRates[_lastHeartRateIndex] = _lastHeartRate;
            
            // Case 1: 30s内没有检测出来就停止检测(可能的原因：Camera不够稳定， 光线不够好)
            // 虽然有Msg提示，但是用户依然会晃动得比较厉害
            if (SNR > 0.4 && _lastHandleFrameStart - _lastDetectionStartTime > 30) {
                // 检测失败，停止检测
                _lastDetectionSucceed = NO;

                handler_start_running(NO);
                
                // sprintf(msg, "Failed to detect heartrate");
                AS3_Val msg1 = AS3_String("Failed to detect heartrate");
                AS3_Trace(msg1);
                AS3_Release(msg1);

//                dispatch_async(dispatch_get_main_queue(), ^(void) {
//                    SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
//                    if (self.hasTorch) {
//                        msg.messages = [NSArray arrayWithObjects: @"由于光照等原因未能识别", @"您可以切换到手指模式测量", nil];
//                        [msg setDurations: (int[]){ 10, 40}  andLength: 2]; // 2s 8s
//                        
//                    } else {
//                        msg.messages = [NSArray arrayWithObjects: @"由于光照等原因未能识别", nil];
//                        [msg setDurations: (int[]){ 10}  andLength: 1]; // 2s 8s
//                    }
//                    [_handlerDelegate showSimpleMsgs: msg];
//                    
//                });
                
                
                
            }  else if (handler_check_stop()) {
                
                // 更加当前得SNR, 以及过去20多次检测结果，作出最终得裁决
                // BOOL currentMode = _isTorchMode;
                // XXX：成功检测，设置状态，准备Video图片(BGRA格式，且是90度翻转的)
                _lastDetectionSucceed = YES;
                handler_start_running(NO);
                
                // 拷贝当前的图片
                uint32_t* prevImage = get_prev_image_data();
                memcpy(prevImage, pixel, sizeof(uint32_t) * kVideoImageWidth * kVideoImageHeight); // // 需要可配置
#ifdef DEBUG
                
                printf("Final Heartrate: %.2f\n", _lastHeartRate);
#endif  

                callbackUpdateFinalHeartrate(_lastHeartRateAvg);
//                sprintf(msg, "Final Heartrate: %.2f", _lastHeartRateAvg);
//                AS3_Trace(AS3_String(msg));
                
            } else {
                // 展示实时心跳
                if (SNR < 0.3) {
                	callbackUpdateHeartrate(_lastHeartRate);

                	// sprintf(msg, "Heartrate: %.2f", _lastHeartRate);
                	// AS3_Trace(AS3_String(msg));

                }
                
                if (_frameIndex - _lastStableHintIndex > 20 * 6) { // 6s间隔
                    _lastStableHintIndex = _frameIndex;
                    
                    _lastStableStatus = !_lastStableStatus;
                    // AS3_Trace(AS3_String("In detection, be patient"));

//                    dispatch_async(dispatch_get_main_queue(), ^(void) {
//                        SimpleMsg* msg = [[[SimpleMsg alloc] init] autorelease];
//                        msg.messages = [NSArray arrayWithObjects: @"正在识别中，请耐心等待", nil];
//                        [msg setDurations: (int[]){10}  andLength: 1];  // 2s
//                        [_handlerDelegate showSimpleMsgs: msg];
//                        
//                    });
                }
            }
            
        }  // End of if (localMaxIdx && localMaxValue > 1)
    } // End of if (_frameNumSinceStable >= kMinSigNum)
    
    
    // 刷新UI:
    callbackUpdateRealTimeCurve(_lastSampleNum);
    // [_handlerDelegate updateViewControllerUI];
    return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void handler_FFTAndLocalMaxium(int count) {
    int i, fftSpdNum;
    float lastPower, last2Power, tmpPower;

    // 3.1 准备FFT的输入数据
    memset(_fftInput.imagp, 0, sizeof(float) * kFFTPointsNum);
    memset(_fftInput.realp, 0, sizeof(float) * kFFTPointsNum);
    
    for (i = 0; i < count; i++) {
        _fftInput.realp[i] = _filteredSignal[i];
    }
    
    // AS3_Trace(AS3_String("Before FFT"));

    // 3.2 调用FFT, 可以处理512/1024两种情况
#ifdef USE_FFT_2048
    fft2048(&_fftInput, &_fftOutput);
#else
    fft1024(&_fftInput, &_fftOutput);
#endif
    
    
    // 3.3 将FFT结果转换称为频谱(PSD)  _fft_f3 = _fftOutput.x ^ 2 + _fftOutput.y ^ 2
    // memset(_fft_f3, 0, sizeof(_fft_f3));
    // FFT频谱中只有一半的信息有效
#ifdef USE_FFT_2048
    fftSpdNum = MIN(kFFTPointsNumHalf, MAX(_endIdx, 400));
#else
    fftSpdNum = MIN(kFFTPointsNumHalf, MAX(_endIdx, 200));
#endif
    for (i = 0; i < fftSpdNum; i++) {
        _fft_f3[i] = _fftOutput.imagp[i] * _fftOutput.imagp[i] + _fftOutput.realp[i] * _fftOutput.realp[i];
    }

    
    // 3.4 对PSD进行局部极大值分析
    lastPower  = _fft_f3[_startIdx - 1];
    last2Power = _fft_f3[_startIdx - 2];
    
    _localMaxPointsNum = 0;
    for (i = _startIdx; i < _endIdx; ++i) {
        tmpPower = _fft_f3[i];
        // 跟踪局部最大值（如果局部最大数值过多，则说明频谱存在问题，不稳定）
        if (_localMaxPointsNum < 40 && lastPower > last2Power && lastPower > tmpPower && i > _startIdx) {
            _localMaxPoints[_localMaxPointsNum].index = i - 1;
            _localMaxPoints[_localMaxPointsNum].maxValue = tmpPower;
            _localMaxPointsNum++;
            
        }
        
        // 更新lastPowers
        last2Power = lastPower;
        lastPower  = tmpPower;
        
    }
    // AS3_Trace(AS3_String("Local Max Points"));

    // 3.5 计算频谱的信噪比(SNR), 变量SNR实际表示为: 噪声和信号比值
    SNR = 100;
    if (_localMaxPointsNum > 1) {
        // 按照maxValue降序排列
        qsort (_localMaxPoints, _localMaxPointsNum, sizeof(LocalMaxPoints), &point_compare);
        SNR = _localMaxPoints[1].maxValue / _localMaxPoints[0].maxValue;
        callbackUpdateFFTCurve(_fft_f3, fftSpdNum, _localMaxPoints[0].maxValue);

        int maxLocalIndex = _localMaxPoints[0].index;
        float indexThreshold = kFFTPointsNumHalf / (float)count;

        // 如果低频较大，并且和当前得最大峰值挨得比较近，则结果也不可靠
        if (_fft_f3[_startIdx - 1] / _localMaxPoints[0].maxValue > 0.4
                && fabs(maxLocalIndex - _startIdx) < indexThreshold) {
            printf("Local Max is too close to low freq., unreliable, indexThreshold: %.1f, error: %d\n",
                   indexThreshold, maxLocalIndex - _startIdx);
            SNR = 0.5;
        }
    }
    
    
}



BOOL handler_check_stop() {
    // SNR: 0.15, 0.1
    int i;
    float error1, average, rootMean, v;

    float threshold = 0.2;
    if (SNR < threshold && _lastHeartRateCount >= kLast20HeartRates) {
        
        // 计算过去4s内的均值，以及方差
        average = 0;
        rootMean = 0;
        for (i = 0; i < kLast20HeartRates; i++) {
            average += _last20HeartRates[i];
        }
        average /= kLast20HeartRates;

        _lastHeartRateAvg = average;
        average = -average;
        
        for (i = 0; i < kLast20HeartRates; i++) {
            v = _last20HeartRates[i] + average;
            rootMean += v * v;
        }
        rootMean = sqrtf(rootMean / kLast20HeartRates);
        
        error1 = fabs(_last20HeartRates[_lastHeartRateIndex] + average);
        
        
        
        if (SNR <= 0.15) {
            // 频谱比较NB, 则检测结果的精度可以要求地一点
            if (error1 < _fftFreqStep && rootMean < _fftFreqStep * 2) {
                // printf("SNR quit OK, and result good....\n");
                // 更新心跳信息
//                dispatch_async(dispatch_get_main_queue(), ^(void) {
//                    [_handlerDelegate updateConfidentHeartRate];
//                });
                _lastHeartRateSucceedCount++;
                if (_lastHeartRateSucceedCount > 4) {
                    return YES;
                }
                
            } else {
                _lastHeartRateSucceedCount = 0;
            }
            
        } else if (SNR > 0.15) {
            // 频谱稍微差一些, 则如果检测结果的精度高，也没有问题
            if (error1 < _fftFreqStep && rootMean < _fftFreqStep) {
                // 更新心跳信息
//                dispatch_async(dispatch_get_main_queue(), ^(void) {
//                    [_handlerDelegate updateConfidentHeartRate];
//                });
                
                // printf("SNR not quit OK, but result perfect....\n");
                _lastHeartRateSucceedCount++;
                if (_lastHeartRateSucceedCount > 4) {
                    return YES;
                }
                
            } else {
                _lastHeartRateSucceedCount = 0;
            }
        }
        
        
        
    } else {
        _lastHeartRateSucceedCount = 0;
    }
    
    return NO;
}


