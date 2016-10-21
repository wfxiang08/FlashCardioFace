package com.chunyu.algorithm {
	
	import cmodule.cardioface.CLibInit;
	
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.events.IEventDispatcher;
	import flash.utils.ByteArray;
	import flash.utils.Endian;

	
	/** 通过alchemy来和C代码交互 */
	public class CardioFaceHandler {


		protected var __CARDIO_FACE_LIB:Object;
		protected var __alchemyRAM:ByteArray;
		protected var __handlerDelegate: CardioFaceHandlerDelegate;
		protected var __framerate: int;

		// TODO: 应该由FalshCardioFace来控制
		public var kStartX: int = 220;
		public var kStartY: int = 180;
		public var kRoiWidth: int = 200;
		public var kRoiHeight: int = 90;
		
		protected var __featurePoints:ByteArray = new ByteArray();
		protected var __realCurvePoints:ByteArray = new ByteArray();
		protected var __fftCurvePoints:ByteArray = new ByteArray();
		// isRunning属性的控制
		protected var _isRunning:Boolean = false;
		public function get isRunning():Boolean {
			return _isRunning;
		}
		public function set isRunning( value:Boolean ):void {
			_isRunning = value;
			__CARDIO_FACE_LIB.setIsRunning(value ? 1 : 0);
			// TODO: 
			__handlerDelegate.updateRunningStatus(value, false);
		}
		

		public function CardioFaceHandler(framerate:int, handlerDelegate: CardioFaceHandlerDelegate) {
			__handlerDelegate = handlerDelegate;
			__framerate = framerate;

			// Alchemy库的初始化
			__CARDIO_FACE_LIB = (new CLibInit()).init();
			var ns:Namespace = new Namespace( "cmodule.cardioface" );
			__alchemyRAM = (ns::gstate).ds;
			__CARDIO_FACE_LIB.handlerInitialize(__framerate, kStartX, kStartY, kRoiWidth, kRoiHeight);
			
			
		}

		public function handleVideoFrame(ba:ByteArray): void {
			// 将this传递给clib, 从而clib可以回调handler的函数
			var result: int = __CARDIO_FACE_LIB.handleVideoFrame(ba, this);
			// trace("Handle frame result: " + result);
		}
		
		
		// 经过归一化处理的数据
		public function callbackUpdateRealTimeCurve(dataPtr:int, count: int): void {
			__realCurvePoints.endian = Endian.LITTLE_ENDIAN;
			__realCurvePoints.position = 0;
			
			__realCurvePoints.writeBytes(__alchemyRAM, dataPtr, count << 2);
			
			__handlerDelegate.updateRealTimeCurve(__realCurvePoints, count);
		}
		public function callbackRunningStatusChanged(value:int): void {
			var newValue: Boolean = !!value;
			if (newValue == _isRunning) {
				return;
			}

			_isRunning = newValue;
			__handlerDelegate.updateRunningStatus(_isRunning, false);
		}
		
		public function callbackUpdateViewControllerUI():void {
			
		}
		
		public function callbackUpdateFFTCurve(fftPtr:int, psdNum: int): void {
			// 不知道为什么通过参数传递回来的featurePointPtr不正确
			__fftCurvePoints.endian = Endian.LITTLE_ENDIAN;
			__fftCurvePoints.position = 0;
			
			__fftCurvePoints.writeBytes(__alchemyRAM, fftPtr, psdNum << 2);
			__fftCurvePoints.position = 0;
			
			__handlerDelegate.updateFFTCurve(__fftCurvePoints, psdNum);
		}
		
		public function callbackUpdateHeartrate(hr:int):void {
			var hr1:Number = hr / 10.0;
			trace("heartrate: ", hr1);
			__handlerDelegate.updateHeartRate(hr1);
		}
		public function callbackUpdateFinalHeartrate(hr:int):void {
			var hr1:Number = hr / 10.0;
			trace("final heartrate: ", hr1);
			__handlerDelegate.updateFinalHeartRate(hr1);
			
			_isRunning = false;
			__handlerDelegate.updateRunningStatus(false, true);
		}
		public function callbackUpdateFeaturePoints(featurePointPtr:int):void {
			// 不知道为什么通过参数传递回来的featurePointPtr不正确
			__featurePoints.endian = Endian.LITTLE_ENDIAN;
			__featurePoints.position = 0;
			
			__featurePoints.writeBytes(__alchemyRAM, featurePointPtr, (2 + 12) << 2);
			__featurePoints.position = 0;

			__handlerDelegate.updateTrackingFeaturePoints(__featurePoints);
		}

	}
}