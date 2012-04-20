package
{
	import flash.events.Event;
	import flash.events.TimerEvent;
	import flash.text.TextField;
	import flash.text.TextFormat;
	import flash.utils.getTimer;
	
	public class FPS extends TextField
	{
		private var frameCount:int = 0;
		private var timer:int;
		private var ms_prev:int;
		private var lastfps : Number = 60; 
		
		public function FPS(inX:Number=10.0, inY:Number=10.0, inCol:int = 0x000000)
		{
			super();
			x = inX;
			y = inY;
			selectable = false;
			defaultTextFormat = new TextFormat("_sans", 20, 0, true);
			text = "FPS:";
			textColor = inCol;
			this.addEventListener(Event.ADDED_TO_STAGE, onAddedHandler);
			
		}
		public function onAddedHandler(e:Event):void {
			stage.addEventListener(Event.ENTER_FRAME, onEnterFrame);
		}
		
		private function onEnterFrame(evt:Event):void
		{
			
			timer = getTimer();
			
			if( timer - 1000 > ms_prev )
			{
				lastfps = Math.round(frameCount/(timer-ms_prev)*1000);
				ms_prev = timer;
				text = "FPS:\n" + lastfps + "/" + stage.frameRate;
				frameCount = 0;
			}
			frameCount++;
				
		}
			
	}

}