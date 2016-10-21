//
//  handler.h
//  MacCardioFace
//
//  Created by Fei Wang on 12-6-3.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#ifndef MacCardioFace_handler_h
#define MacCardioFace_handler_h
#include "AlgUtils.h"
#include "algorithm.h"

extern float _accumOffsetX;
extern float _accumOffsetY;
extern float _detrendedSignal[kFFTPointsNum];

//inline float get_accum_offsetX() {
//	return _accumOffsetX;
//}
//inline float get_accum_offsetY() {
//	return _accumOffsetY;
//}

BOOL handler_is_running();
void handler_set_framerate(int val);
void handler_start_running(BOOL val);

void handler_initialize(int _framerate, int startX, int startY, int roiWidth, int roiHeight);
void handler_reset();
void handler_after_configuration();
void handler_reset_check_condition();

BOOL handler_handle_frame_internal(uint32_t* const pixel, int width, int height);
float handler_get_green_sum_Of_ROI(int width, uint32_t * const pixel);
void handler_tracking(uint32_t* const pixel, int imageWidth, int imageHeight);

BOOL handler_fftAnalyzeSignalSegWithOrigData(uint32_t* const pixel);
void handler_FFTAndLocalMaxium(int count);
BOOL handler_check_stop();
#endif
