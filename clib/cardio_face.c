#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "AlgUtils.h"
#include "Sin_Table.h"
#include "Tracking.h"
#include "TrackingUtil.h"
#include "handler.h"


float *realData;
float *imagData;
float *realFFTData;
float *imagFFTData;
float *amplFFTData;
float *phaseFFTData;
float *drawData;
float *shiftedData;

// 导出接口到action script中

// 接口需求:
// 1. 输入一个Bitmap, 或者是一个ByteArray
// 2. C库通过回调函数，将数据传递给Flash, 负责和C交互的代码是:  com.chunyu.algorithm.CardioFaceHandler

// 初始化handler
static AS3_Val handlerInitialize(void* self, AS3_Val args) {
	// 读取参数
	int framerate, startX, startY, roiWidth, roiHeight;
	AS3_ArrayValue(args, "IntType, IntType, IntType, IntType, IntType",
			&framerate, &startX, &startY, &roiWidth, &roiHeight );

	// 初始化: 传递参数，预先分配内存等等
	handler_initialize(framerate, startX, startY, roiWidth, roiHeight);

	// 必须返回一个Action Script的对象
	return AS3_Int(1);
}

// com.chunyu.algorithm.CardioFaceHandler
static AS3_Val _handler = NULL;


void callbackRunningStatusChanged(BOOL newStatus) {
	// 从C调用ActionScript
	AS3_CallTS("callbackRunningStatusChanged", _handler, "IntType", newStatus);
}
static float* featurePointsBuffer = NULL;
void callbackUpdateFeaturePoints() {
	// 将数据拷贝到: featurePointsBuffer, 然后再返回
	float location;
	extern float _accumOffsetX;
	extern float _accumOffsetY;

	if (featurePointsBuffer == NULL) {
		featurePointsBuffer = malloc((kFeatureNum + 1) * 2 * sizeof(float));
	}

	featurePointsBuffer[0] = _accumOffsetX;
	featurePointsBuffer[1] = _accumOffsetY;


	int i;
	FeaturePoints* points = get_feature_points();
	for (i = 0; i < kFeatureNum; i++) {
		int k = (i + 1)  << 1;
		featurePointsBuffer[k] = points->column[i];

		// AS3_Trace(AS3_Number(points->column[i]));
		featurePointsBuffer[k + 1] = points->row[i];
	}

	AS3_CallTS("callbackUpdateFeaturePoints", _handler, "IntType", featurePointsBuffer);
}

void callbackUpdateHeartrate(double hr) {
	// 直接传递native数据，不用wrap
	int hr1 = hr * 10 + 0.5;
	AS3_CallTS("callbackUpdateHeartrate", _handler, "IntType", hr1);
}
void callbackUpdateFinalHeartrate(double hr) {
	// 直接传递native数据，不用wrap
	int hr1 = hr * 10 + 0.5;
	AS3_CallTS("callbackUpdateFinalHeartrate", _handler, "IntType", hr1);
}
static float* fftCurveData = NULL;
void callbackUpdateFFTCurve(const float* psd, int psdNum, const float localMax) {
	if (fftCurveData == NULL) {
		fftCurveData = malloc(kFFTPointsNumHalf * sizeof(float));
	}

	float coefficent = 1.0 / localMax * 80;
	int i;
#ifdef USE_FFT_2048
	psdNum = psdNum >> 1;
	// 数据量加倍，但是展示不变
	for (i = 0; i < psdNum; i++) {
		fftCurveData[i] = psd[i << 1] * coefficent;
	}
#else
	for (i = 0; i < psdNum; i++) {
		fftCurveData[i] = psd[i] * coefficent;
	}
#endif

	AS3_CallTS("callbackUpdateFFTCurve", _handler, "IntType,IntType", fftCurveData, psdNum);
}
static float* realTimeCurveData = NULL;
void callbackUpdateRealTimeCurve(int count) {
	// 更新实时曲线
	extern float _detrendedSignal[kFFTPointsNum];
	if (realTimeCurveData == NULL) {
		realTimeCurveData = malloc(kFFTPointsNum * sizeof(float));
	}
	int i;
	float maxValue = -100000;
	float minValue = 1000000;
	for (i = 0; i < count; i++) {
		float tmp = _detrendedSignal[i];
		realTimeCurveData[i] = tmp;
		if (maxValue < tmp) {
			maxValue = tmp;
		}
		if (minValue > tmp) {
			minValue = tmp;
		}
	}
	maxValue = maxValue > 0 ? maxValue : -maxValue;
	minValue = minValue > 0 ? minValue : -minValue;
	maxValue = maxValue > minValue ? maxValue : minValue;
	if (maxValue > 0.02) {
		float invMaxValue = 1.0 / maxValue * 60;
		for (i = 0; i < count; i++) {
			realTimeCurveData[i] *= invMaxValue;
		}
	}
	AS3_CallTS("callbackUpdateRealTimeCurve", _handler, "IntType, IntType", realTimeCurveData, count);
}
static AS3_Val getFeaturePointsBuffer(void* self, AS3_Val args) {
	// Deprecated
	return AS3_Ptr(featurePointsBuffer);
}
// 处理每一桢数据
static AS3_Val handleVideoFrame(void* self, AS3_Val args) {
	// 只有处于Running状态，才处理数据
	if (!handler_is_running()) {
		return AS3_Int(0);
	}

	// 从args中读取byteArray
	// http://forums.adobe.com/thread/765694
	AS3_Val byteArray;
    AS3_ArrayValue(args, "AS3ValType, AS3ValType", &byteArray, &_handler);

    // 将byte array转换为pixels, 然后调用内部的handler_handle_frame_internal
    uint32_t* pixels = get_pixels_buffer();

    AS3_ByteArray_seek(byteArray, 0, 0);
    AS3_ByteArray_readBytes(pixels, byteArray, kVideoImageWidth * kVideoImageHeight * 4);


    // 调用内部的算法
	BOOL result = handler_handle_frame_internal(pixels, kVideoImageWidth, kVideoImageHeight);

	AS3_Release(byteArray);
	AS3_Release(_handler);
	_handler = NULL;

	return AS3_Int(result);
}

static AS3_Val setIsRunning(void* self, AS3_Val args) {
	int running = 0;
	AS3_ArrayValue(args, "IntType", &running);
	if (running) {
		handler_start_running(YES);
	} else {
		handler_start_running(NO);
	}
	return AS3_Int(running);
}

int main() {
    // AS3_Release: 在什么时候调用呢?

	// 1.0 Wrap
    AS3_Val handleVideoFrame_m = AS3_Function(NULL, handleVideoFrame);
    AS3_Val handlerInitialize_m = AS3_Function(NULL, handlerInitialize);
    AS3_Val handlerSetIsRunning_m = AS3_Function(NULL, setIsRunning);
    AS3_Val getFeaturePointsBuffer_m = AS3_Function(NULL, getFeaturePointsBuffer);
    // 1. 导出接口
    AS3_Val result = AS3_Object("handlerInitialize:AS3ValType, handleVideoFrame:AS3ValType, setIsRunning:AS3ValType, getFeaturePointsBuffer:AS3ValType",
    							handlerInitialize_m,
                                handleVideoFrame_m,  // handleVideoFrame: AS3ValType
                                handlerSetIsRunning_m,
                                getFeaturePointsBuffer_m
                                );
    // 1.1 Release
    AS3_Release(handleVideoFrame_m);
    AS3_Release(handlerInitialize_m);
    AS3_Release(handlerSetIsRunning_m);
    AS3_Release(getFeaturePointsBuffer_m);
    
    // 2. 通过result来出刷Lib
    AS3_LibInit(result);
    return 0;
}
