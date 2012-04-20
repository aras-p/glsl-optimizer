package
{
	import C_Run.initLib;
	
	import flash.display.Sprite;
	import flash.display.StageAlign;
	import flash.display.StageQuality;
	import flash.display.StageScaleMode;
	import flash.geom.Rectangle;
	import flash.system.Capabilities;
	
	import starling.core.Starling;
	
	[SWF(width="480", height="640", frameRate="60", backgroundColor="#ffffff")]
	public class BunnyMark_Starling extends Sprite
	{
		private var _width:Number = 480;
		private var _height:Number = 640;
		private  var mStarling:Starling;
		private var fps:FPS;
		
		public function BunnyMark_Starling()
		{
			C_Run.initLib(this);
				
			stage.scaleMode = StageScaleMode.NO_SCALE;
			stage.align = StageAlign.TOP_LEFT;
			stage.quality = StageQuality.LOW;
			if (Capabilities.manufacturer.toLowerCase().indexOf("ios") != -1 || 
				Capabilities.manufacturer.toLowerCase().indexOf("android") != -1)
			{
				_width = Capabilities.screenResolutionX;
				_height = Capabilities.screenResolutionY;
			}
			
			fps = new FPS();
			addChild(fps);
			
			Starling.multitouchEnabled = true;
			
			
			mStarling = new Starling(BunnySetup, stage, new Rectangle(0, 0, _width, _height));
			mStarling.simulateMultitouch = true;
			mStarling.enableErrorChecking = false;
			mStarling.start();
			
			//			addChild(new Stats());
		}
	}
}