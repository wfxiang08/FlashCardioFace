package com.chunyu.ui {
	import flash.display.DisplayObject;
	import flash.display.Shape;
	import flash.display.SimpleButton;


	public class CustomSimpleButton extends SimpleButton {
		private var upColor:uint   = 0xFFCC00;
		private var overColor:uint = 0xCCFF00;
		private var downColor:uint = 0x00CCFF;
		private var size:uint      = 80;
		
		public function CustomSimpleButton() {
			this.downState      = new ButtonDisplayState(downColor, size);
			this.overState      = new ButtonDisplayState(overColor, size);
			this.upState        = new ButtonDisplayState(upColor, size);
			this.hitTestState   = new ButtonDisplayState(upColor, size * 2);
			
			hitTestState.x = -(size / 4);
			hitTestState.y = hitTestState.x;

			useHandCursor  = true;
		}
	}
}