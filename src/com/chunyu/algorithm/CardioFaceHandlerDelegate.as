package com.chunyu.algorithm {
	import flash.utils.ByteArray;

	public interface CardioFaceHandlerDelegate {
		/**
		 * featurePoints[0], featurePoints[1]表示_accumOffsetX和_accumOffsetY
		 * featurePoints[2...]表示特征点(x0, y0, x1, y1, x2, y2, ....) 所有的数字都是Number类型(float)
		 */
		function updateTrackingFeaturePoints(featurePoints:ByteArray): void;

		function updateRealTimeCurve(realData: ByteArray, len: int): void
		function updateFFTCurve(normalizedPsdData: ByteArray, psdNum: int):void;

		function updateHeartRate(hr:Number): void;
		function updateFinalHeartRate(hr: Number): void;
		
		function updateRunningStatus(isRunning:Boolean, isSucceed: Boolean): void;
		
	}
}