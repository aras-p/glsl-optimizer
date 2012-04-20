package
{
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import flash.display3D.*;
	import flash.utils.getTimer;
	import flash.text.TextField;
	import flash.text.TextFieldAutoSize;
	import flash.text.TextFormat;
	import flash.text.TextFormatAlign;
	import flash.events.MouseEvent;
	
	import starling.core.Starling;
	import starling.display.Image;
	import starling.display.Sprite;
	import starling.events.Event;
	import starling.textures.Texture;
	
	public class BunnySetup extends Sprite
	{
		[Embed(source="assets/wabbit_alpha.png")]
		private var BunnyImage : Class;
		[Embed(source="assets/pirate.png")]
		private var PirateImage : Class;
		
		private var bunnyBM:Bitmap;
		private var context3D:Context3D;
		private var bg:Background;
		private var _width:Number;// = 480;
		private var _height:Number;// = 640;
		private var bunnies:Vector.<Bunny> = new Vector.<Bunny>();
		private var pirate:Image;
		private var pirateHalfWidth:int;
		private var pirateHalfHeight:int;
		private var tf:TextField;	
		private var numBunnies:int = 100;	
		private var incBunnies:int = 100;
		
		private var gravity:Number = 0.5;
		private var maxX:int;
		private var minX:int;
		private var maxY:int;
		private var minY:int;
		
		public function BunnySetup()
		{
			super();
			bunnyBM = new BunnyImage();
			_width = Starling.current.viewPort.width;
			_height = Starling.current.viewPort.height;
			maxX = _width;
			minX = 0;
			maxY = _height;
			minY = 0;
			init();
		}
		private function init():void {
			//add the bunny counter
			createCounter();
			context3D = Starling.current.nativeStage.stage3Ds[0].context3D;		
			bg = new Background(context3D,_width,_height);
			addChild(bg);
			
			addBunnies(numBunnies);
			
			//add pirate
			pirate = new Image(Texture.fromBitmap(new PirateImage()));
			pirate.pivotX = pirateHalfWidth =  pirate.width/2;
			pirate.pivotY = pirateHalfHeight = pirate.height/2;
			addChild(pirate);
			//backgroundImage = new Image(Assets.getTexture("Background"));
			//backgroundImage.scaleX = BACKGROUND_SPRITE_SCALE;
			//backgroundImage.scaleY = BACKGROUND_SPRITE_SCALE;
			//addChild(backgroundImage);
			
			/*topOverlayImage = new Image(Assets.getTexture("TopOverlay"));
			topOverlayImage.y = topOverlayImage.height * -0.5;
			
			bottomOverlayImage = new Image(Assets.getTexture("BottomOverlay"));
			bottomOverlayImage.y = STAGE_HEIGHT - (bottomOverlayImage.height * 0.5);
			
			addFish(FISH_INCREMENTAL);
			
			addChild(topOverlayImage);
			addChild(bottomOverlayImage);*/
			
			this.addEventListener(Event.ENTER_FRAME,update);
		}
		private function createCounter():void
		{
			var format:TextFormat = new TextFormat("_sans", 20, 0, true);
			format.align = TextFormatAlign.RIGHT;
			
			tf = new TextField();
			tf.selectable = false;
			tf.defaultTextFormat = format;
			tf.text = "Bunnies:\n" + numBunnies;
			tf.autoSize = TextFieldAutoSize.LEFT;
			tf.x = _width - 100;
			tf.y = 10;
			
			Starling.current.nativeStage.addChild(tf);
			
			tf.addEventListener(MouseEvent.CLICK, counter_click);
		}
		private function counter_click(e:MouseEvent):void {
			/*if(numBunnies == 16250) {
				//we've reached the limit for vertex buffer length
				tf.text = "Bunnies \n(Limit):\n" + numBunnies;
			}
			else*/ {
				if (numBunnies >= 1500) incBunnies = 250;
				
				addBunnies(incBunnies);
				numBunnies += incBunnies;
				
				tf.text = "Bunnies:\n" + numBunnies;
			}
		}
		private function addBunnies(numBunnies:int):void {
			var currentBunnyCount:int = bunnies.length;
			var bunny:Bunny;
			var tex:Texture = Texture.fromBitmap(bunnyBM);
			for ( var i:uint = currentBunnyCount ; i < currentBunnyCount+numBunnies; i++ ) {
				
				bunny = new Bunny(tex);
				
				
				bunny.pivotX = bunny.width / 2;
				bunny.pivotY = bunny.height / 2;
				bunny.speedX = Math.random() * 5;
				bunny.speedY = (Math.random() * 5) - 2.5;
				//bunny.scaleX = FISH_SCALE;
				//newFish.scaleY = FISH_SCALE;
				//bunny.x = (Math.random() * FISH_MAXIMUM_X) + FISH_MINIMUM_X + newFish.width / 2;
				//newFish.y = (Math.random() * FISH_MAXIMUM_Y) + FISH_MINIMUM_Y;
				bunny.rotation = 15 - Math.random() * 30;
				
				bunnies.push(bunny);
				addChild(bunny);
			}
			if(pirate) {
				removeChild(pirate);
				addChild(pirate);
			}
		}
		private function update(event:Event):void 
		{
			var bunny:Bunny;
			for(var i:int=0; i<bunnies.length;i++)
			{
				bunny = bunnies[i];
				bunny.x += bunny.speedX;
				bunny.y += bunny.speedY;
				bunny.speedY += gravity;
				bunny.alpha = 0.3 + 0.7 * bunny.y / maxY;
				
				if (bunny.x > maxX)
				{
					bunny.speedX *= -1;
					bunny.x = maxX;
				}
				else if (bunny.x < minX)
				{
					bunny.speedX *= -1;
					bunny.x = minX;
				}
				if (bunny.y > maxY)
				{
					bunny.speedY *= -0.8;
					bunny.y = maxY;
					if (Math.random() > 0.5) bunny.speedY -= 3 + Math.random() * 4;
				} 
				else if (bunny.y < minY)
				{
					bunny.speedY = 0;
					bunny.y = minY;
				}
			}
			var currentTime:int = getTimer();
			pirate.x = (maxX - (pirateHalfWidth)) * (0.5 + 0.5 * Math.sin(currentTime / 3000));
			pirate.y = (maxY - (pirateHalfHeight) + 70 - 30 * Math.sin(currentTime / 100));
			
			//pirate.x = maxX * (0.5 + 0.5 * Math.sin( currentTime/ 3000));
			//pirate.y = maxY -  + 70 - 30 * Math.sin( currentTime/ 100);
		}
	}
}