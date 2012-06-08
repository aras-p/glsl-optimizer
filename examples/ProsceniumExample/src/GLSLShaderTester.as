/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
package
{
	// ===========================================================================
	//	Imports
	// ---------------------------------------------------------------------------
	import C_Run.initLib;
	
	import com.adobe.alchemy.CModule;
	import com.adobe.scenegraph.*;
	import com.adobe.scenegraph.SceneSkyBox;
	import com.adobe.scenegraph.loaders.ModelLoader;
	import com.adobe.scenegraph.loaders.obj.OBJLoader;
	import com.adobe.utils.LoadTracker;
	
	import flash.display.*;
	import flash.display3D.*;
	import flash.events.Event;
	import flash.events.KeyboardEvent;
	import flash.events.MouseEvent;
	import flash.external.ExternalInterface;
	import flash.geom.*;
	import flash.system.Security;
	import flash.utils.ByteArray;
	import flash.utils.Dictionary;
	 
	// ===========================================================================
	//	Class
	// ---------------------------------------------------------------------------
	public class GLSLShaderTester extends BasicScene
	{
		// ======================================================================
		//	Constants
		// ----------------------------------------------------------------------
		protected static const CAMERA_ORIGIN:Vector3D				= new Vector3D( 0, 0, 20 );
		protected static const ORIGIN:Vector3D						= new Vector3D();
		protected static const PAN_AMOUNT:Number					= .25;
		protected static const ROTATE_AMOUNT:Number					= 4;
		
		public function GLSLShaderTester(_newShaderCallback:Function)
		{
			newShaderCallback = _newShaderCallback;
			super();
		}
		
		// ======================================================================
		//	Properties
		// ----------------------------------------------------------------------
		protected var _cubeInstanced:SceneMesh;
		
		protected var newShaderCallback:Function;
		 
		protected var vsglsl:String = <![CDATA[
			uniform vec3 LightPosition;
			
			const float SpecularContribution = 0.3;
			const float DiffuseContribution  = 1.0 - SpecularContribution;
			
			varying float LightIntensity;
			varying vec2  MCposition;
			uniform float time;
			
			void main()
			{
				vec3 ecPosition = vec3(gl_ModelViewMatrix * gl_Vertex);
				vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
				vec3 lightVec   = normalize(LightPosition - ecPosition);
				vec3 reflectVec = reflect(-lightVec, tnorm);
				vec3 viewVec    = normalize(-ecPosition);
				float diffuse   = max(dot(lightVec, tnorm), 0.0);
				float spec      = 0.0;
				
				if (diffuse > 0.0)
				{
					spec = max(dot(reflectVec, viewVec), 0.0);
					spec = pow(spec, 16.0);
				}
				
				LightIntensity  = max(0.2, DiffuseContribution * diffuse + SpecularContribution * spec);
				
				MCposition      = gl_Vertex.xy;
			
				vec4 inp = gl_Vertex;
				inp.y = inp.y + sin((inp.x + time) * 3)*.25;
				inp.z = inp.z + sin((inp.z + time) * 3)*.25;
			
			
				vec4 op = gl_ModelViewProjectionMatrix * inp;
				//op.y = op.y + sin((op.x + time) * 10)*0.25;
			
				gl_Position = op;
			}
		]]>;
		
		protected var fsglsl:String = <![CDATA[
			uniform vec3  BrickColor, MortarColor;
			uniform vec2  BrickSize;
			uniform vec2  BrickPct;
			
			varying vec2  MCposition;
			varying float LightIntensity;
			
			void main()
			{
				vec3  color;
				vec2  position, useBrick;
				
				position = MCposition / BrickSize;
				
				if (fract(position.y * 0.5) > 0.5)
					position.x += 0.5;
				
				position = fract(position);
				
				useBrick = step(position, BrickPct);
				
				color  = mix(MortarColor, BrickColor, useBrick.x * useBrick.y);
				color *= LightIntensity;
				gl_FragColor = vec4(color, 1.0);
			}
		]]>;
		
		protected static const SKYBOX_DIRECTORY:String				= "./";
		protected static const SKYBOX_FILENAMES:Vector.<String>		= new <String>[
			SKYBOX_DIRECTORY + "px.png",
			SKYBOX_DIRECTORY + "nx.png",
			SKYBOX_DIRECTORY + "py.png",
			SKYBOX_DIRECTORY + "ny.png",
			SKYBOX_DIRECTORY + "pz.png",
			SKYBOX_DIRECTORY + "nz.png",
			"stoneBlocks.png",
			"stoneBlocksNormal.png",
			"stoneBlocksSpecular.png",
		];
		
		protected var _modelLoader:ModelLoader;
		protected var testmat:MaterialCustomGLSL;
		protected var tp:SceneMesh;
		protected var _sky:SceneSkyBox;
		
		protected static const ROT_AXIS:Vector3D = new Vector3D( 0.1, 1, 0, 0 );
		protected static const LIGHT_POS:Vector3D = new Vector3D( 5.0, 5.0, 5.0, 0.0 );
		
		protected var fragConsts:Object = {}
		protected var vertConsts:Object = {}	
		
		// = =====================================================================
		//	Methods
		// ----------------------------------------------------------------------
		
		override protected function resetCamera():void
		{
			_camera = scene.activeCamera;
			_camera.identity();
			_camera.position = CAMERA_ORIGIN;
			_camera.appendRotation( -15, Vector3D.X_AXIS );
			_camera.appendRotation( -25, Vector3D.Y_AXIS, ORIGIN );
		}
		
		public function write(fd:int, buf:int, nbyte:int, errno_ptr:int):int
		{
			var str:String = CModule.readString(buf, nbyte);
			trace(str);
			return nbyte;
		}
		 
		protected function imageLoadComplete( bitmaps:Dictionary ):void
		{
			var bitmapDatas:Vector.<BitmapData> = new Vector.<BitmapData>( 6, true );
			var bitmap:Bitmap;
			for ( var i:uint = 0; i < 6; i++ )
				bitmapDatas[ i ] = bitmaps[ SKYBOX_FILENAMES[ i ] ].bitmapData;
			
			fragConsts["Stone"] = new TextureMap( bitmaps[ SKYBOX_FILENAMES[ 6 ] ].bitmapData);
			fragConsts["StoneNormal"] = new TextureMap( bitmaps[ SKYBOX_FILENAMES[ 7 ] ].bitmapData);
			fragConsts["StoneSpecular"] = new TextureMap( bitmaps[ SKYBOX_FILENAMES[ 8 ] ].bitmapData);
			 
			var light:SceneLight = new SceneLight( SceneLight.KIND_POINT, "point light" );
			light.color.set( .5, .6, .7 );
			light.appendTranslation( 20, 20, 20 );
			light.shadowMapEnabled = false;
			light.setShadowMapSize( 1024, 1024 );
			scene.addChild(light);
			
			// sky
			_sky = new SceneSkyBox( bitmapDatas, false );
			scene.addChild( _sky );	// skybox must be an immediate child of scene root
			_sky.name = "Sky"

			
			_modelLoader = new OBJLoader( "teapot.obj");
			_modelLoader.addEventListener( Event.COMPLETE, loadComplete);
		}
		
		protected function loadComplete( e:* ):void
		{
			var manifest:ModelManifest = _modelLoader.model.addTo( scene );
			trace(scene);
			tp = scene.getDescendantByNameAndType( "teapot", SceneMesh ) as SceneMesh; 
			tp.errorCallback = renderError;
			tp.material = testmat;
		}
		
		protected function renderError(sm:SceneMesh, e:*):void
		{
			//sm.hidden = true;
			sm.material = new MaterialStandard();
			if(ExternalInterface.available) {
				ExternalInterface.call("appendLog", e.toString() + " " + e.getStackTrace())
			}
		}
		
		override protected function initModels():void
		{
			stage.frameRate = 60
				
			initLib(this);
			
			fragConsts["BrickSize"]        = Vector.<Number>([0.30, 0.15, 0.0, 0.0])
			fragConsts["BrickPct"]         = Vector.<Number>([0.9, 0.85, 0.0, 0.5])
			fragConsts["BrickColor"]       = Vector.<Number>([1.0, 0.3, 0.2, 0])
			fragConsts["MortarColor"]      = Vector.<Number>([0.85, 0.86, 0.84, 0.0])
			
			fragConsts["AmbientColor"]     = Vector.<Number>([0.2, 0.2, 0.2, 0.0])
			fragConsts["DiffuseColor"]     = Vector.<Number>([0.2, 0.8, 0.1, 0.0])
			fragConsts["SpecularColor"]    = Vector.<Number>([0.8, 0.8, 0.8, 0.0])
			fragConsts["SpecularExponent"] = Vector.<Number>([200, 0, 0, 0.0])
			
			vertConsts["wobblefactor"] = Vector.<Number>([0.25, 0.25, 0.25, 0.0])
				
			if (ExternalInterface.available) {
				try {
					ExternalInterface.addCallback("newShaders", newShaders);
					Security.allowDomain("*") // Awwww yeah!
					trace("External interface callback added.");
				} catch (error:Error) {
					trace("An Error occurred: " + error.message + "\n");
				}
			} else {
				trace("External interface is not available for this container.");
			}
			
			LoadTracker.loadImages( SKYBOX_FILENAMES, imageLoadComplete );		
		}
		 
		private function newShaders(vs:String, fs:String):void
		{
			try { 
				ExternalInterface.call("clearLog");
				
				var vsjson:String = C_Run.compileShader(vs,0,true);
				trace("compiled v shader: " + vsjson);
				
				var fsjson:String = C_Run.compileShader(fs,1,true);
				trace("compiled f shader: " + fsjson);
				
				var vsj:Object = JSON.parse(vsjson);
				var fsj:Object = JSON.parse(fsjson);
				
				if(vsj.info.length > 0 || fsj.info.length > 0) {
					ExternalInterface.call("appendLog", "vshader error: \n" + vsj.info.toString() + "\n\n")
					ExternalInterface.call("appendLog", "fshader error: \n" + fsj.info.toString() + "\n")
					return;
				} else {
					ExternalInterface.call("appendLog", "vshader: \n" + vsj.agalasm.toString() + "\n\n")
					ExternalInterface.call("appendLog", "fshader: \n" + fsj.agalasm.toString() + "\n")
				}
				 
				testmat = new MaterialCustomGLSL(vsj, fsj, materialCallback, materialErrorCallback);	// material name is used
				testmat.fragShaderValues = fragConsts;
				testmat.vertShaderValues = vertConsts;
					
				if(tp)
					tp.material = testmat
						
				newShaderCallback(vsj, fsj, testmat);
			} catch (e:*) {
				trace(e)
				
				if(ExternalInterface.available) {
					ExternalInterface.call("appendLog", e.toString() + " " + e.getStackTrace())
				}
			}
		}
		
		protected function materialErrorCallback( e:* ):void
		{
			trace(e);
			if(ExternalInterface.available) {
				ExternalInterface.call("appendLog", e.toString() + " " + e.getStackTrace())
			}
		}
		
		protected function materialCallback( material:MaterialCustomGLSL, settings:RenderSettings, renderable:SceneRenderable, data:* = null ):void
		{
			var cam:SceneCamera = scene.activeCamera;
			var sl:SceneLight = scene.getActiveLight(0);

			var lp:Vector3D = cam.modelTransform.transformVector( sl.position );
			
			testmat.vertShaderValues["LightPosition"] = new <Number>[ lp.x, lp.y, lp.z, 0 ];
			
			testmat.vertShaderValues["time"] = new <Number>[ flash.utils.getTimer()*0.001, 0, 0, 0];
			testmat.fragShaderValues["time"] = new <Number>[ flash.utils.getTimer()*0.001, 0, 0, 0];
			
			material.setupProgramConstants(scene, instance, material, settings, renderable, data)
		}
		
		override protected function onAnimate( t:Number, dt:Number ):void
		{
			if(tp) {
				tp.setPosition( 0, 0, 0 );
				tp.appendRotation(1, ROT_AXIS);
			}
		}
		
		override protected function keyboardEventHandler( event:KeyboardEvent ):void
		{
			var dirty:Boolean = false;
			_camera = scene.activeCamera;
			
			switch( event.type )
			{
				case KeyboardEvent.KEY_DOWN:
				{
					dirty = true;
					
					switch( event.keyCode )
					{
						case 13:	// Enter
							animate = !animate;
							break;
						
						case 16:	// Shift
						case 17:	// Ctrl
						case 18:	// Alt
							dirty = false;
							break;
						
						case 32:	// Spacebar
							resetCamera();
							break;
						
						case 38:	// Up
							if ( event.ctrlKey )		_camera.interactiveRotateFirstPerson( 0, ROTATE_AMOUNT );
							else if ( event.shiftKey )	_camera.interactivePan( 0, -PAN_AMOUNT );
							else						_camera.interactiveForwardFirstPerson( PAN_AMOUNT );
							break;
						
						case 40:	// Down
							if ( event.ctrlKey )		_camera.interactiveRotateFirstPerson( 0, -ROTATE_AMOUNT );
							else if ( event.shiftKey )	_camera.interactivePan( 0, PAN_AMOUNT );
							else						_camera.interactiveForwardFirstPerson( -PAN_AMOUNT );
							break;
						
						case 37:	// Left
							if ( event.shiftKey )		_camera.interactivePan( -PAN_AMOUNT, 0 );
							else						_camera.interactiveRotateFirstPerson( ROTATE_AMOUNT, 0 );							
							break;
						
						case 39:	// Right
							if ( event.shiftKey )		_camera.interactivePan( PAN_AMOUNT, 0 );
							else						_camera.interactiveRotateFirstPerson( -ROTATE_AMOUNT, 0 );
							break;
						
						case 219:	_camera.fov -= 1;				break;	// "["						
						case 221:	_camera.fov += 1;				break;	// "]"						
						
						case 79:	// o
							SceneGraph.OIT_ENABLED = !SceneGraph.OIT_ENABLED;
							trace( "Stenciled Layer Peeling:", SceneGraph.OIT_ENABLED ? "enabled" : "disabled" );
							break;
						
						case 66:	// b
							instance.drawBoundingBox = !instance.drawBoundingBox;
							break;
						
						case 84:	// t
							instance.toneMappingEnabled = !instance.toneMappingEnabled;
							trace( "Tone mapping:", instance.toneMappingEnabled ? "enabled" : "disabled" );
							break;
						
						case 48:	// 0
						case 49:	// 1
						case 50:	// 2
						case 51:	// 3
						case 52:	// 4
						case 53:	// 5
						case 54:	// 6
						case 55:	// 7
						case 56:	// 8
						case 57:	// 9
							instance.toneMapScheme = event.keyCode - 48;
							trace( "Tone map scheme:", instance.toneMapScheme );
							break;
						
						default:
							//trace( event.keyCode );
							dirty = false;
					}	
				}
			}
			
			if ( dirty )
				_dirty = true;
		}
		
		override protected function mouseEventHandler( event:MouseEvent, target:InteractiveObject, offset:Point, data:* = undefined ):void
		{
			if ( offset.x == 0 && offset.y == 0 )
				return;
			
			_camera = scene.activeCamera;
			
			if ( event.ctrlKey )
			{
				if ( event.shiftKey )
					_camera.interactivePan( offset.x / 5, offset.y / 5 );
				else
					_camera.interactiveRotateFirstPerson( -offset.x, -offset.y );
			}
			else
			{
				if ( event.shiftKey )
					_camera.interactivePan( offset.x / 5, offset.y / 5 );
				else
				{
					_camera.interactiveRotateFirstPerson( -offset.x, 0 );
					_camera.interactiveForwardFirstPerson( -offset.y / 5 );
				}
			}
		}
	}
}