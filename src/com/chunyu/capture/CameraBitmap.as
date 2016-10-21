package com.chunyu.capture {
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.events.IEventDispatcher;
	import flash.events.TimerEvent;
	import flash.filters.ColorMatrixFilter;
	import flash.geom.ColorTransform;
	import flash.geom.Matrix;
	import flash.geom.Point;
	import flash.media.Camera;
	import flash.media.Video;
	import flash.utils.Timer;
	import flash.utils.getTimer;
	import flash.utils.setTimeout;
	
	public class CameraBitmap extends EventDispatcher {
		[Event(name="Event.RENDER", type="flash.events.Event")]
		
		public var bitmapData:BitmapData;
		
		private var __width:int;
		private var __height:int;
		
		private var __cam:Camera;
		private var __video:Video;
		
		private var __refreshRate:int;
		private var __timer:Timer;
		private var __paintMatrix:Matrix;
		private var __smooth:Boolean;
		private var __colorTransform:ColorTransform;
		private var __colorMatrix:Array;
		private var __colorMatrixFilter:ColorMatrixFilter = new ColorMatrixFilter();
		
		private const CAMERA_DELAY:int = 100;
		
		private const origin:Point = new Point();
		
		public function CameraBitmap( width:int, height:int, refreshRate:int = 15, 
									  cameraWidth:int = -1, cameraHeight:int = -1 ) {
			__width  = width;
			__height = height;
			refreshRate = refreshRate;

			// 内存一次分配，多次使用
			bitmapData = new BitmapData( width, height, false, 0 );
			
			// 首先要求获取Camera
			// 设置Camera的大小?
			__cam = Camera.getCamera();
			
			if (__cam) {
				if ( cameraWidth == -1 || cameraHeight == -1 ) {
					__cam.setMode(width, height, refreshRate, true );

				} else {
					__cam.setMode(cameraWidth, cameraHeight, refreshRate, true );
				}
				__refreshRate = refreshRate;
				
				cameraInit();

			} else {
				// 如果没有Camera, 暂时什么事情也不做				
			}
		}

		public function isCameraAvailable():Boolean {
			return !!__cam;
		}

		public function print_camera(): void {
			trace("max fps: " + __cam.fps + ", current fps: " + __cam.currentFPS);
		}
		
		public function getCurrentFps(): Number {
			return __cam.currentFPS;
		}

		public function set active( value:Boolean ):void {
			// if ( value ) __timer.start() else __timer.stop();
		}
		
		public function close():void {
			active = false;
			__video.attachCamera(null);
			__video = null;
			__cam = null;
		}
		
		// 通过__timer来控制采样的间隔
		public function set refreshRate( value:int ):void {
			__refreshRate = value;
			// __timer.delay = 1000 / __refreshRate;
		}
		
		public function set cameraColorTransform( value:ColorTransform ):void {
			__colorTransform = value;
		}
		
		public function set colorMatrix( value:Array ):void {
			__colorMatrixFilter.matrix = __colorMatrix = value;
		}
		
		private function cameraInit():void {
			// 创建一个和video, 并且和Camera Attach
			// disattach
			__video = new Video( __cam.width, __cam.height );
			__video.attachCamera( __cam );
			
			// __width, __height的调整  (需要: 左右翻转)
			__paintMatrix = new Matrix( -1,  0,  // __width / __cam.width
									 	 0, 1, 
				                         __width, 0 );
			
			// 如果Scale不为1,  则需要smooth处理?
			// 这里我们假定Scale为1:
			__smooth = __paintMatrix.a != 1 || __paintMatrix.d != 1
		}
		
		public function sampleAndPaint(event:TimerEvent = null):void {
			var start:int = getTimer();
			// 将__video绘制到bitmapData中，并且生成一个RENDER EVENT
			bitmapData.lock();
			try {
				// 获取一桢数据
				bitmapData.draw ( __video, __paintMatrix, __colorTransform, "normal", null, __smooth );
				
				if ( __colorMatrix != null ) {
					bitmapData.applyFilter( bitmapData, bitmapData.rect, origin, __colorMatrixFilter );
				}				
			} finally {
				bitmapData.unlock();
			}

			// 通过事件来通信
			dispatchEvent( new Event( Event.RENDER ) );			
		}
	}
}