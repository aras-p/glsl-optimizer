package
{
	import starling.display.Image;
	import starling.textures.Texture;
	
	public class Bunny extends Image
	{
		public var speedX:Number;
		public var speedY:Number;
		public function Bunny(texture:Texture)
		{
			super(texture);
			speedX = speedY = 0.0;
		}
	}
}