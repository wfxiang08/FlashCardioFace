package com.chunyu.data {
	import flash.display.BitmapData;
	import flash.utils.ByteArray;
	
	// 学习如何高效地访问: BitmapData中的数据
	public class TargetImage{
		public  var _ii   :Array;	// IntegralImage
		public  var _ii2  :Array;	// IntegralImage of squared pixels
		public  var iiw   :int;
		public  var iih   :int;
		
		public var width:int;
		public var height:int;
		
		
		public function TargetImage( ){
		}
		
		public function set bitmapData(b:BitmapData):void {
			
			// 数组，稀疏数组: (和dict类似)
			if( (b.width+1)!=iiw || (b.height+1)!=iih ) {
				_ii  = [];
				_ii2 = [];
			}
			
			width = b.width;
			height = b.height;
			
			// build IntegralImages
			// IntegralImage is 1 size larger than image
			// all 0 for the 1st row,column
			iiw = width + 1;
			iih = height + 1;
			var singleII  :Number = 0;
			var singleII2 :Number = 0;
			var index:int;
			var ba_index:int = 1;
			
			var pix:Number;
			// 获取某个区域内部的pixels
			var ba:ByteArray = b.getPixels( b.rect );
			
			for( var i:int=0; i<iiw; i++ ) {
				_ii2[ i ] = _ii[  i ] = 0;
			}
			
			for( var j:int=1; j<iih; j++ ) {
				_ii2[ index = int(j*iiw) ] = _ii[index] = 0;
				for( i=1; i<iiw; i++ ) {
					pix = ba[ba_index];
					singleII  =  Number(_ii[int(index-iiw+1)]) + Number(_ii[index]) - Number(_ii[int(index-iiw)]) + pix;
					singleII2 = Number(_ii2[int(index-iiw+1)]) + Number(_ii2[index]) - Number(_ii2[int(index-iiw)]) + pix*pix ;
					_ii[ ++index ] = singleII;
					_ii2[ index ] = singleII2;
					ba_index+=4;
				}
			}
		}
		
		public function getSum(x:int,y:int,w:int,h:int):int{
			var y_iiw   :int = y     * iiw;
			var yh_iiw  :int = (y+h) * iiw;
			return int(_ii[int(y_iiw  + x    )]) +
				int(_ii[int(yh_iiw + x + w)]) -
				int(_ii[int(yh_iiw + x    )]) -
				int(_ii[int(y_iiw  + x + w)]);
		}
		
		// sum of squared pixel
		public function getSum2(x:int,y:int,w:int,h:int):int{
			var y_iiw   :int = y     * iiw;
			var yh_iiw  :int = (y+h) * iiw;
			return int(_ii2[int(y_iiw  + x    )]) +
				int(_ii2[int(yh_iiw + x + w)]) -
				int(_ii2[int(yh_iiw + x    )]) -
				int(_ii2[int(y_iiw  + x + w)]);
		}
		
		public function getII(x:int,y:int):int{
			return _ii[int(y*iiw+x)];
		}
		
		public function getII2(x:int,y:int):int{
			return _ii2[int(y*iiw+x)];
		}
		
	}
}