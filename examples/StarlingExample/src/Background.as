package
{
	import C_Run.compileShader;
	
	import com.adobe.utils.AGALMiniAssembler;
	
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import flash.display3D.*;
	import flash.display3D.textures.*;
	import flash.geom.Matrix3D;
	import flash.geom.Rectangle;
	import flash.utils.getTimer;
	
	import starling.core.RenderSupport;
	import starling.core.Starling;
	import starling.display.DisplayObject;
	import starling.utils.VertexData;
	
	public class Background extends DisplayObject
	{
		[Embed(source="assets/grass.png")]
		private var Grass : Class;
		private var context3D:Context3D;
		private var vb:VertexBuffer3D;
		private var uvb:VertexBuffer3D;
		private var ib:IndexBuffer3D;
		private var shader_program:Program3D;
		private var tex:Texture;
		private var _width:Number;
		private var _height:Number;
		private var texBM:Bitmap;
		private var _modelViewMatrix : Matrix3D;
		
		//variables for vertexBuffer manipulation
		private var vertices:Vector.<Number>;
		private var indices:Vector.<uint>;
		private var uvt:Vector.<Number>;
		
		//variables for starling
		private var mVertexData:VertexData;
		private var mRawData:Vector.<Number>;
		
		//haxe variable
		public var cols:int = 8;//2;//8;
		public var rows:int = 12;//2;//12;
		public var numTriangles:int;
		public var numVertices:int;
		public var numIndices:int;
		
		private var compiledVertexShader:Object;
		private var compiledFragmentShader:Object;
		
		private var va_vert_idx:int, va_tex_idx:int, tex_idx:int, mvpmat_idx:int, frag_time_idx:int;
		
		public function Background(ctx3D:Context3D,w:Number, h:Number)
		{
			//add this for starling port
			super();
			this.touchable = false;
			
			context3D = ctx3D;//Starling.current.nativeStage.stage3Ds[0].context3D;
			_width = w;
			_height = h;
				
			//create background texture
			texBM = new Grass();
			//tex = Texture.fromBitmapData(texBM.bitmapData,false,false);
			
			tex = context3D.createTexture(texBM.width, texBM.height, Context3DTextureFormat.BGRA,false);
			tex.uploadFromBitmapData(texBM.bitmapData,0);

			buildMesh();

			var glslVertexShaderSource:String = <![CDATA[
				varying vec2 TexCoords;
				
				void main()
				{
					TexCoords = gl_MultiTexCoord0.xy;
					gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
				}
			]]>

			var glslFragmentShaderSource:String = <![CDATA[
				varying vec2 TexCoords;
				
				uniform sampler2D BackgroundImage;
				uniform float time;
				
				vec2 wobbleTexCoords(in vec2 tc)
				{
					tc.x += (sin(tc.x*5)*0.5) + time;
					tc.y -= (sin(tc.y*5)*0.5) - time;
					return tc;
				}
				
				void main()
				{
					vec2 tc = wobbleTexCoords(TexCoords);
				
					gl_FragColor = texture2D(BackgroundImage, tc);
				}		
			]]>

			compiledVertexShader = JSON.parse(C_Run.compileShader(glslVertexShaderSource, 0, true));
			trace(compiledVertexShader.info)
			trace(compiledVertexShader.agalasm)
			
			compiledFragmentShader = JSON.parse(C_Run.compileShader(glslFragmentShaderSource, 1, true));
			trace(compiledFragmentShader.info)
			trace(compiledFragmentShader.agalasm)
			
			//build shaders
			var miniasm_vertex : AGALMiniAssembler = new AGALMiniAssembler ();
			miniasm_vertex.assemble( Context3DProgramType.VERTEX, compiledVertexShader.agalasm);
			
			var miniasm_fragment : AGALMiniAssembler = new AGALMiniAssembler (); 
			miniasm_fragment.assemble(Context3DProgramType.FRAGMENT, compiledFragmentShader.agalasm);
			 
			shader_program = context3D.createProgram();
			shader_program.upload( miniasm_vertex.agalcode, miniasm_fragment.agalcode );		
			
			//create projection matrix
			_modelViewMatrix = new Matrix3D();
			_modelViewMatrix.appendTranslation(-(_width)/2, -(_height)/2, 0);            
			_modelViewMatrix.appendScale(2.0/(_width-50), -2.0/(_height-50), 1);

			//set everything
			var c:String
			var constval:Array
			
			for(c in compiledFragmentShader.consts) {
				constval = compiledFragmentShader.consts[c];
				context3D.setProgramConstantsFromVector( Context3DProgramType.FRAGMENT, int(c.slice(2)), Vector.<Number>([constval[0], constval[1], constval[2], constval[3]]) )
			}
			
			for(c in compiledVertexShader.consts) {
				constval = compiledVertexShader.consts[c];
				context3D.setProgramConstantsFromVector( Context3DProgramType.VERTEX, int(c.slice(2)), Vector.<Number>([constval[0], constval[1], constval[2], constval[3]]) )
			}
			
			frag_time_idx = compiledFragmentShader.varnames["time"].slice(2)
				
			tex_idx = compiledFragmentShader.varnames["BackgroundImage"].slice(2)
			context3D.setTextureAt(tex_idx, tex);
			context3D.setProgram ( shader_program );
			
			va_vert_idx = compiledVertexShader.varnames["gl_Vertex"].slice(2)
			context3D.setVertexBufferAt( va_vert_idx, vb, 0, "float2" );
			
			va_tex_idx = compiledVertexShader.varnames["gl_MultiTexCoord0"].slice(2)
			context3D.setVertexBufferAt( va_tex_idx, uvb, 0, "float2" );
			
			mvpmat_idx = compiledVertexShader.varnames["gl_ModelViewProjectionMatrix"].slice(2)
			context3D.setProgramConstantsFromMatrix(Context3DProgramType.VERTEX, mvpmat_idx,_modelViewMatrix,true);
			
		}
		public override function getBounds(targetSpace:DisplayObject, resultRect:Rectangle=null):Rectangle {
			return null;
		}
		
		private function buildMesh():void 
		{
			var uw:Number = _width / texBM.width;
			var uh:Number = _height / texBM.height;
			var kx:Number, ky:Number;
			var ci:int, ci2:int, ri:int;
			
			//create vertices, use magic number for now for 8 cols 12 rows
			//mVertexData = new VertexData(117, false);
			//mRawData = mVertexData.rawData;
			
			vertices = new Vector.<Number>();
			uvt = new Vector.<Number>();
			indices = new Vector.<uint>();
			
			var i:int;
			var j:int;
			for(j = 0; j <= rows; j++)
			{
				ri = j * (cols + 1) * 2;
				ky = j / rows;
				for(i = 0; i <= cols; i++)
				{
					ci = ri + i * 2;
					kx = i / cols;
					vertices[ci] = _width * kx; 
					vertices[ci + 1] = _height * ky;
					uvt[ci] = uw * kx; 
					uvt[ci + 1] = uh * ky;
				}
			}
			for(j = 0; j < rows; j++)
			{
				ri = j * (cols + 1);
				for(i = 0; i < cols; i++)
				{
					ci = i + ri;
					ci2 = ci + cols + 1;
					indices.push(ci);
					indices.push(ci + 1);
					indices.push(ci2);
					indices.push(ci + 1);
					indices.push(ci2 + 1);
					indices.push(ci2);
				}
			}
			//now create the buffers
			numIndices = indices.length;
			numTriangles = numIndices / 3;
			numVertices = vertices.length / 2;
			vb = context3D.createVertexBuffer(numVertices,2);
			uvb = context3D.createVertexBuffer(numVertices,2);
			
			ib = context3D.createIndexBuffer(numIndices);
			vb.uploadFromVector(vertices,0,numVertices);
			ib.uploadFromVector(indices,0,numIndices);
			uvb.uploadFromVector(uvt,0,numVertices);
			
		}
		public override function render(support:RenderSupport, alpha:Number):void {
			if (_width == 0 || _height == 0) return;
			
			var t:Number = getTimer() / 1000.0;
			var sw:Number = _width;
			var sh:Number = _height;
			var kx:Number, ky:Number;
			var ci:int, ri:int;
			context3D.setBlendFactors(Context3DBlendFactor.ONE, Context3DBlendFactor.ONE_MINUS_SOURCE_ALPHA);
			context3D.setTextureAt(tex_idx, tex);  
			context3D.setProgram ( shader_program );
			context3D.setVertexBufferAt( va_vert_idx, vb, 0, "float2" );  
			context3D.setVertexBufferAt( va_tex_idx, uvb, 0, "float2" );
			context3D.setProgramConstantsFromMatrix(Context3DProgramType.VERTEX, mvpmat_idx,_modelViewMatrix,true);
			
			context3D.setProgramConstantsFromVector(Context3DProgramType.FRAGMENT, frag_time_idx , new <Number>[getTimer() / 5000, 0, 0, 0]);
			
			var i:int = 0;
			for(var j:int = 0; j <= rows; j++)
			{
				ri = j * (cols + 1) * 2;
				for (i=0; i <= cols; i++) 
				{
					ci = ri + i * 2;
					kx = i / cols ; //+ Math.cos(t + i) * 0.02;
					ky = j / rows ; // + Math.sin(t + j + i) * 0.02;
					vertices[ci] = sw * kx; 
					vertices[ci + 1] = sh * ky; 
				}
			}
			vb.uploadFromVector(vertices,0,numVertices);
			context3D.drawTriangles(ib,0,numTriangles);
		}
	}
}