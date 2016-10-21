package com.chunyu.ui {
	import flash.display.DisplayObject;
	import flash.display.Shape;
	import flash.display.SimpleButton;

	public class ButtonDisplayState extends Shape {
		private var bgColor:uint;
		private var size:uint;
		
		public function ButtonDisplayState(bgColor:uint, size:uint) {
			this.bgColor = bgColor;
			this.size    = size;
			draw();
			
//			var my_loader : Loader = new Loader();
//			my_loader.load(new URLRequest("car.jpg"));
//			addChild(my_loader);
		}
		
		private function draw():void {
			graphics.beginFill(bgColor);
			graphics.drawRect(0, 0, size, size);
			graphics.endFill();
		}
	}
}