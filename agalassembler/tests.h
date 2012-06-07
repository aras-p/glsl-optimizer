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
				

static char test1[] = 
	"// fragment \n"
	"mov fo0, vi0.ww	\n";  

static char test2[] = 
	"// fragment \n" 
	"mov ft0, fc0		// ignored \n" 
	"add ft0, vi0, vi1	 \n" 
	"tex ft2, vi2, fs2 <cube,nearest,  clamp, mipnone> \n" 
	"kil ft2.y			\n"
	"mov ft1, ft0		\n" 	
	"mov fo0, ft0		\n"; 

static char test3[] = 
	"// vertex \n" 
	"mov vt2, vc0 \n"
	"mul vt2, vt2, vt2 \n"
	"mov vt0.xyzw, vt2 \n"
	"mov vt1.xyzw, vt2 \n"
	"mov vt2.xyzw, vt2 \n"
	"m33 vi0.xyz, va0, vt0 \n" 	

	"mov vt3.x, vc6.w \n"
	"dp4 vt2.x, va1, vc[va3.x].xyzw \n" 
	"dp4 vt2.w, va1, vc[vt3.x-23].xyzw    \n"
	"mov vt2.yz, vc5 \n" 
	"mov vo0, vt2 \n"; 

static char test4[] = 
	"// fragment \n" 
	"mov ft0, fc0		\n" 
	"mov ft1, fc1		\n" 
	"mov ft2, fc2		\n" 
	"add ft3, ft1, ft2  \n"
	"mov fo0, ft3		\n"; 


static char test5[] = 
	"// fragment \n" 
	"mov ft1, fc0 \n"
	"add ft3.yz, ft1, fc2  \n"
	"add ft3.x,  ft1, fc2  \n"
	"add ft3.w,  ft1, fc2  \n"
	"mov fo0, ft3		\n"; 


static char complexrtt_1[] = 
				"// vertex \n" 
				"mov vt0, va0\n"
				"mov vt0, va1\n"
				"mov vt0, va2\n"  // dummy stream refs
				"m44 vt0, va0, vc0		\n" 		// 4x4 matrix transform position from stream 0 to world space
				"m33 vt3.xyz, va1, vc0		\n" 	// 3x3 matrix transform normal from stream 1 to world space
				"m44 vt1, vt0, vc4		\n"  		// 4x4 matrix transform from world to eye space
				"m44 vo0, vt1, vc8		\n"  		// 4x4 matrix transform from eye to clip space
				
				"m44 vt2, vt0, vc12		\n"  		// 4x4 matrix transform from world to light space position				
				"m33 vt4.xyz, vt3.xyz, vc12	\n"  	// 3x3 matrix transform normal from world space to light space
				"m44 vt5, vt2, vc16 \n"             // 4x4 matrix light space position to projected light space position 												
				
				"mov vi0, va2			\n"    		// copy uv to fragment
				"mov vi1, vt4.xyz		\n"     	// copy light space normal to fragment
				"mov vi2, vt2			\n"         // copy light space position to fragment
				"mov vi3, vt5			\n"         // copy projected light space position to fragment
;
				
static char complexrtt_2[] = 
				"// fragment \n" 
				"mov ft4, vi3\n"                  // project in light space
				"rcp ft4.w, ft4.w \n"  				
				"mul ft4.xyz, ft4, ft4.w \n"   			
				
				"kil ft4.w \n"  
				"mul ft3, ft4, ft4 \n"  
				"add ft3, ft3.xxxx, ft3.yyyy \n"  
				"sub ft3.y, fc0.x, ft3.y,  \n"  
				"kil ft3.y \n"   // clip
																
				"neg ft4.y, ft4.y\n"  
				"mul ft4.xy, ft4.xy, fc0.w \n"    				
				"add ft4.xy, ft4.xy, fc0.w \n"    // normalize to texture "screen space" x = x/2   .5 ... should be in vs
								
				// sample around
				"mov ft5, ft4 \n"  
				"add ft5.x, ft4.x, fc4.y \n"             //  dx
				"add ft5.y, ft4.y, fc4.y \n"             //  dy				
				"tex ft6, ft5, fs1, <2d,clamp,nearest,nomip>\n"    							
				"dp3 ft7.x, ft6, fc3 \n"   	     	    // color decode z
				
				"sub ft5.x, ft4.x, fc4.y \n"             // -dx
				"add ft5.y, ft4.y, fc4.y \n"             //  dy
				"tex ft6, ft5, fs1, <2d,clamp,nearest,nomip>\n"    							
				"dp3 ft7.y, ft6, fc3 \n"   	     	    // color decode z
				
				"add ft5.x, ft4.x, fc4.y \n"             //  dx
				"sub ft5.y, ft4.y, fc4.y \n"             // -dy
				"tex ft6, ft5, fs1, <2d,clamp,nearest,nomip>\n"    							
				"dp3 ft7.z, ft6, fc3 \n"   	     	    // color decode z

				"sub ft5.x, ft4.x, fc4.y \n"             // -dx
				"sub ft5.y, ft4.y, fc4.y \n"             // -dy
				"tex ft6, ft5, fs1, <2d,clamp,nearest,nomip>\n"    							
				"dp3 ft7.w, ft6, fc3 \n"   	     	    // color decode z				
								
				"slt ft7, ft4.z, ft7 \n"  			    // our light z - texture light z				
				"dp4 ft7.x, ft7, fc4.z \n"              // fc4.z = 1/4				
				
				// center sampler
				"tex ft6, ft4, fs1, <2d,clamp,nearest,nomip>\n"    											
				"dp3 ft7.y, ft6, fc3 \n"   	     	    // color decode z, center sample
				"slt ft7.y, ft4.z, ft7.y \n"  			    // our light z - texture light z
				
				"dp4 ft7.x, ft7.xxyy, fc4.z \n"  
								
				// sample base texture and modulate
				"tex ft5, vi0, fs0 <2d,clamp,linear,miplinear>\n" 
				"mul ft5, ft5, ft7.x \n"  
				"mul ft5.xyz, ft5, fc1 \n"    // light color
				"mul ft3.y, ft3.y, fc1.w\n"    // light falloff
				"sat ft3.y, ft3.y \n" 
				"mul ft5, ft5, ft3.y \n"   // modulate with falloff
				
				// dot lighting
				"mov ft0, vi0			\n"  
				"mov ft1, vi1			\n"  
				"nrm ft1.xyz, vi1		\n"  				
				"nrm ft2.xyz, vi2		\n"  
				"neg ft2.xyz, ft2.xyz   \n"  
				"dp3 ft3.x, ft2.xyz, ft1.xyz \n"  
				"sat ft3.x, ft3.x   \n"   
				"mul ft5, ft5, ft3.x \n"   // modulate				
								 													
				"mov fo0, ft5 \n" 
;
						
static char complexrtt_3[] = 			
				// way too many transforms, this is example code only to track what's happening!!!
				"// vertex \n" 
				"mov vt0, va0\nmov vt0, va1\nmov vt0, va2\n"   // dummy stream refs
				"m44 vt0, va0, vc0		\n"  		// 4x4 matrix transform position from stream 0 to world space
				"m44 vt1, vt0, vc4		\n"  		// 4x4 matrix transform from world to eye space
				"m44 vo0, vt1, vc8		\n"  		// 4x4 matrix transform from eye to clip space												 													
				"mov vi0, va2			\n"   		// copy uv to fragment
;
			
static char complexrtt_4[] =			
				"// fragment \n" 
				"tex ft0, vi0, fs0 <2d,clamp,linear,miplinear>\n" 
				"mul ft0, ft0, fc7 \n"  
				"mov fo0, ft0 \n" 
				// mul and output
;		
			
static char complexrtt_5[] = 		
				"// vertex \n" 
				"mov vt0, va0\nmov vt0, va1\nmov vt0, va2\n"   // dummy stream refs
				"m44 vt0, va0, vc0		\n"  		// 4x4 matrix transform position from stream 0 to world space
				"m44 vt2, vt0, vc12		\n"  		// 4x4 matrix transform from world to light space position
				"m44 vt5, vt2, vc16 \n"             // 4x4 matrix light space position to projected light space position
				"mov vi0, vt2 \n"                    // light space 
				"mov vi1, vt5 \n"                    // projected lightspace   
				"mov vo0, vt5 \n"                  // projected lightspace
;
							
static char complexrtt_6[] = 		
				"// fragment \n" 
				"mov ft0, vi1			\n"  
				"rcp ft0.w, ft0.w       \n"  
				"add ft0.z, ft0.z, fc4.x\n"     // bias
				"mul ft0, ft0, ft0.w    \n"     											
				
				"mul ft0, ft0.zzzz, fc2 \n"   	     // color encode 24 bit (1/255, 1, 255, 0, 0) 				
				"frc ft0, ft0 \n"  					
				"mul ft1, ft0, fc3 \n"   
				"sub ft0.xyz, ft0.xyz, ft1.yzw \n"       // adjust 
				"mov fo0, ft0          \n" 
;

static char complexrtt_7[] = 	
				"// vertex \n" 
				"mov vt0, va0\nmov vt0, va1\nmov vt0, va2\n"   // dummy stream refs				
				"mov vi0, va2 \n"   // copy uv
				"mov vt0, va0\n"  
				"neg vt0.y, vt0.y\n" 
				"mov vo0, vt0 \n" 
;
						
static char complexrtt_8[] = 		
				"// fragment \n" 
				"tex ft0, vi0, fs0 <2d,clamp,linear,mipnearest,0>\n" 	
				"dp3 ft4, ft0, ft0\n"   // brightness
				
				"mov ft1, vi0\n"  
				
				"add ft1.xy, vi0.xy, fc1.zz\n"   //  d, d
				"tex ft2, ft1, fs1 <2d,clamp,linear,mipnearest,2.0>\n" 
				"mov ft3, ft2\n"  
				
				"add ft1.xy, vi0.xy, fc1.zw\n"   //  d,-d
				"tex ft2, ft1, fs1 <2d,clamp,linear,mipnearest,2.0>\n" 
				"add ft3, ft3,ft2\n"  
				
				"add ft1.xy, vi0.xy, fc1.ww\n"   // -d,-d
				"tex ft2, ft1, fs1 <2d,clamp,linear,mipnearest,2.0>\n" 
				"add ft3, ft3, ft2\n"  
				
				"add ft1.xy, vi0.xy, fc1.wz\n"   //  d,-d
				"tex ft2, ft1, fs1 <2d,clamp,linear,mipnearest,2.0>\n" 
				"add ft3, ft3, ft2\n"  
								
				"mul ft3, ft3, fc0.w\n"   // scale back
				"dp3 ft3, ft3, ft3\n"   // brightness
				
				"sub ft3, ft3, ft4 \n"  
				"mul ft3, ft3, fc0.x \n"   // scale up a bit
				"abs ft3, ft3 \n"  
																		 							
				"mul ft0, ft0, fc0.x\n"   // br/cr adjust
				"add ft0, ft0, fc0.y\n"  
				
				"add ft0, ft3, ft0\n"  // add in				
				
				"mov fo0, ft0 \n" 
;

static char unitybug[] = 		
				"// vertex \n" 
				//" mov vt0, va0 \n" // fill with any trash (any non-temporary register) 
				" mul vt3.w, va1.x, vc12.x \n"
				" dp4 vt1.x, va0.xyzw, vc[vt3.w+4] \n"
				" dp4 vt1.y, va0.xyzw, vc[vt3.w+5] \n" 
				" dp4 vt1.z, va0.xyzw, vc[vt3.w+6] \n"
				" dp4 vt1.w, va0.xyzw, vc[vt3.w+7] \n"
				" dp4 vo0.x, vt1, vc0 \n"
				" dp4 vo0.y, vt1, vc1 \n"
				" dp4 vo0.z, vt1, vc2 \n"
				" dp4 vo0.w, vt1, vc3 \n"
;

static char square[] = 
	"//vertex program (agal 1)\n"
	" mov vi0, va0\n"
  " m43 vt1.xyz, va1, vc[va2.x+0].xyzw\n"
  " mul vt0, vt1.xyzz, va2.yyyy\n"
  " m43 vt1.xyz, va1, vc[va2.z+0].xyzw\n"
  " mul vt1.xyz, vt1.xyzz, va2.wwww\n"
  " add vt0, vt0, vt1.xyzz\n"
  " m43 vt1.xyz, va1, vc[va3.x+0].xyzw\n"
  " mul vt1.xyz, vt1.xyzz, va3.yyyy\n"
  " add vt0, vt0, vt1.xyzz\n"
  " m43 vt1.xyz, va1, vc[va3.z+0].xyzw\n"
  " mul vt1.xyz, vt1.xyzz, va3.wwww\n"
  " add vt0, vt0, vt1.xyzz\n"
  " m43 vt1.xyz, va1, vc[va4.x+0].xyzw\n"
  " mul vt1.xyz, vt1.xyzz, va4.yyyy\n"
  " add vt0, vt0, vt1.xyzz\n"
  " mov vt0.w, va1.wwww\n"
  " m44 vo0, vt0, vc54.xyzw\n"
  " mov vt1, vc58\n"
  " m33 vt3.xyz, va5, vc[va2.x+0].xyzw\n"
  " mul vt2, vt3.xyzz, va2.yyyy\n"
  "m33 vt3.xyz, va5, vc[va2.z+0].xyzw\n"
  " mul vt3.xyz, vt3.xyzz, va2.wwww\n"
  " add vt2, vt2, vt3.xyzz\n"
  " m33 vt3.xyz, va5, vc[va3.x+0].xyzw\n"
  " mul vt3.xyz, vt3.xyzz, va3.yyyy\n"
  " add vt2, vt2, vt3.xyzz\n"
  " m33 vt3.xyz, va5, vc[va3.z+0].xyzw\n"
  " mul vt3.xyz, vt3.xyzz, va3.wwww\n"
  " add vt2, vt2, vt3.xyzz\n"
  " m33 vt3.xyz, va5, vc[va4.x+0].xyzw\n"
  " mul vt3.xyz, vt3.xyzz, va4.yyyy\n"
  " add vt2, vt2, vt3.xyzz\n"
  " mov vt2.w, va5.wwww\n"
  " nrm vt2.xyz, vt2.xyzz\n"
  " sub vt3, vc59, vt0\n"
  " dp3 vt3.w, vt3, vt3\n"
  " nrm vt3.xyz, vt3.xyzz\n"
  " dp3 vt3.x, vt3, vt2\n"
  " sqt vt3.w, vt3.wwww\n"
  " sub vt3.w, vt3.wwww, vc61.zzzz\n"
  " div vt3.y, vt3.wwww, vc61.yyyy\n"
  " sub vt3.w, vc61.xxxx, vt3.yyyy\n"
  " sat vt3.xw, vt3.xwww\n"
  " mul vt3.xyz, vc60.xyzz, vt3.xxxx\n"
  " mul vt3.xyz, vt3.xyzz, vt3.wwww\n"
  " add vt1.xyz, vt1.xyzz, vt3.xyzz\n"
  " sub vt3, vc62, vt0\n"
  " dp3 vt3.w, vt3, vt3\n"
  " nrm vt3.xyz, vt3.xyzz\n"
  " dp3 vt3.x, vt3, vt2\n"
  " sqt vt3.w, vt3.wwww\n"
  " sub vt3.w, vt3.wwww, vc64.zzzz\n"
  " div vt3.y, vt3.wwww, vc64.yyyy\n"
  " sub vt3.w, vc64.xxxx, vt3.yyyy\n"
  "sat vt3.xw, vt3.xwww\n"
  "mul vt3.xyz, vc63.xyzz, vt3.xxxx\n"
  "mul vt3.xyz, vt3.xyzz, vt3.wwww\n"
  "add vt1.xyz, vt1.xyzz, vt3.xyzz\n"
  "dp3 vt3.x, vt2, vc65\n"
  "sat vt3.x, vt3.xxxx\n"
  "mul vt3, vc66, vt3.xxxx\n"
  "add vt1, vt1, vt3\n"
  "mov vi1, vt1\n";

static char simple1[] = 
	"//fragment program (agal 1)\n"
	"mov ft0, fi0\n"
	"tex ft1, ft0, fs1 <2d, linear, nomip, clamp>\n"
	"mov ft1.w, fc0\n"
	"mov fo0, ft1\n";

static char psmv[] = 
  "//;vertex program (agal 1)\n"
  "m44 vt0, va0, vc[va3.x+0].xyzw\n"
  "mul vt0, vt0, va3.yyyy\n"
  "m44 vt1, va0, vc[va3.z+0].xyzw\n"
  "mul vt1, vt1, va3.wwww\n"
  "add vt0, vt0, vt1\n"
  "m44 vt1, va0, vc[va4.x+0].xyzw\n"
  "mul vt1, vt1, va4.yyyy\n"
  "add vt0, vt0, vt1\n"
  "m44 vt1, va0, vc[va4.z+0].xyzw\n"
  "mul vt1, vt1, va4.wwww\n"
  "add vt0, vt0, vt1\n"
  "m44 vt0, vt0, vc1.xyzw\n"
  "mov vt7, vt0\n"
  "mov vi0, vt0\n"
  "m44 vt1, vt0, vc9.xyzw\n"
  "mov vo0, vt1\n"
  "mov vi3, vt1\n"
  "mov vt1, va1\n"
  "mov vt1.w, vc0.xxxx\n"
  "m33 vt1.xyz, va1, vc[va3.x+0].xyzw\n"
  "mul vt1.xyz, vt1, va3.yyyy\n"
  "m33 vt2.xyz, va1, vc[va3.z+0].xyzw\n"
  "mul vt2.xyz, vt2.xyzz, va3.wwww\n"
  "add vt1.xyz, vt1, vt2.xyzz\n"
  "m33 vt2.xyz, va1, vc[va4.x+0].xyzw\n"
  "mul vt2.xyz, vt2.xyzz, va4.yyyy\n"
  "add vt1.xyz, vt1, vt2.xyzz\n"
  "m33 vt2.xyz, va1, vc[va4.z+0].xyzw\n"
  "mul vt2.xyz, vt2.xyzz, va4.wwww\n"
  "add vt1.xyz, vt1, vt2.xyzz\n"
  "m33 vi1.xyz, vt1, vc5.xyzw\n"
  "mov vi1.w, vc0.xxxx\n"
  "mov vi2, va2\n"
  "m44 vt1, vt7, vc13.xyzw\n"
  "sub vt1.y, vc0.xxxx, vt1.yyyy\n"
  "mov vi4, vt1\n"
  "m44 vt1, vt7, vc17.xyzw\n"
  "sub vt1.y, vc0.xxxx, vt1.yyyy\n"
  "mov vi5, vt1\n";

static char psmf[] = 
  "//fragment program (agal 1)\n"
  "mov ft4, fc0.xxxy\n"
  "mov ft5, fc0.xxxy\n"
  "sub ft6, fc1, fi0\n"
  "nrm ft6.xyz, ft6\n"
  "mov ft6.w, fc0.xxxx\n"
  "nrm ft7.xyz, fi1\n"
  "sub ft1, fi0, fc14\n"
  "abs ft0.xyz, ft1.xyzz\n"
  "max ft0.w, ft0.yyyy, ft0.zzzz\n"
  "max ft0.w, ft0.xxxx, ft0.wwww\n"
  "sge ft3, ft0, ft0.wwww\n"
  "mov ft2.x, ft3.xxxx\n"
  "sub ft2.yz, ft3.xyzz, ft3.xxxx\n"
  "sub ft2.z, ft2.zzzz, ft3.yyyy\n"
  "sat ft2.xyz, ft2.xyzz\n"
  "div ft1.xyz, ft1.xyzz, ft0.wwww\n"
  "mul ft1.xyz, ft1.xyzz, fc0.zzzz\n"
  "mov ft1.w, ft0.wwww\n"
  "slt ft3.xyz, ft2.xyzz, ft2.zxyy\n"
  "dp3 ft0.x, ft1.xyzz, ft3.xyzz\n"
  "slt ft3.xyz, ft2.xyzz, ft2.yzxx\n"
  "dp3 ft0.y, ft1.xyzz, ft3.xyzz\n"
  "sub ft2.w, ft1.wwww, fc15.xxxx\n"
  "mul ft2.w, ft2.wwww, fc15.yyyy\n"
  "sat ft1.w, ft2.wwww\n"
  "mul ft0.xy, ft0.xyyy, fc15.zzzz\n"
  "frc ft0.xy, ft0.xyyy\n"
  "sub ft0.z, fc0.yyyy, ft0.xxxx\n"
  "sub ft0.w, fc0.yyyy, ft0.yyyy\n"
  "mul ft0.xz, ft0.xxzz, fc0.zzzz\n"
  "slt ft3.xyz, ft2.xyzz, fc0.zzzz\n"
  "mul ft3.xyz, ft3.xyzz, fc15.wwww\n"
  "sub ft1.xyz, ft1.xyzz, ft3.xyzz\n"
  "tex ft3.xyz, ft1, fs1 <cube, nearest, miplinear, clamp>\n"
  "dp3 ft3.x, ft3.xyzz, fc8.xyzz\n"
  "sge ft2.w, ft3.xxxx, ft1.wwww\n"
  "mul ft2.w, ft2.wwww, ft0.zzzz\n"
  "mul ft2.w, ft2.wwww, ft0.wwww\n"
  "slt ft3.xyz, ft2.xyzz, ft2.yzxx\n"
  "mul ft3.xyz, ft3.xyzz, fc15.wwww\n"
  "add ft1.xyz, ft1.xyzz, ft3.xyzz\n"
  "tex ft3.xyz, ft1, fs1 <cube, nearest, miplinear, clamp>\n"
  "dp3 ft3.x, ft3.xyzz, fc8.xyzz\n"
  "sge ft3.x, ft3.xxxx, ft1.wwww\n"
  "mul ft3.x, ft3.xxxx, ft0.zzzz\n"
  "mul ft3.x, ft3.xxxx, ft0.yyyy\n"
  "add ft2.w, ft2.wwww, ft3.xxxx\n"
  "slt ft3.xyz, ft2.xyzz, ft2.zxyy\n"
  "mul ft3.xyz, ft3.xyzz, fc15.wwww\n"
  "add ft1.xyz, ft1.xyzz, ft3.xyzz\n"
  "tex ft3.xyz, ft1, fs1 <cube, nearest, miplinear, clamp>\n"
  "dp3 ft3.x, ft3.xyzz, fc8.xyzz\n"
  "sge ft3.x, ft3.xxxx, ft1.wwww\n"
  "mul ft3.x, ft3.xxxx, ft0.xxxx\n"
  "mul ft3.x, ft3.xxxx, ft0.yyyy\n"
  "add ft2.w, ft2.wwww, ft3.xxxx\n"
  "slt ft3.xyz, ft2.xyzz, ft2.yzxx\n"
  "mul ft3.xyz, ft3.xyzz, fc15.wwww\n"
  "sub ft1.xyz, ft1.xyzz, ft3.xyzz\n"
  "tex ft3.xyz, ft1, fs1 <cube, nearest, miplinear, clamp>\n"
  "dp3 ft3.x, ft3.xyzz, fc8.xyzz\n"
  "sge ft3.x, ft3.xxxx, ft1.wwww\n"
  "mul ft3.x, ft3.xxxx, ft0.xxxx\n"
  "mul ft3.x, ft3.xxxx, ft0.wwww\n"
  "add ft3.z, ft2.wwww, ft3.xxxx\n"
  "sub ft0, fc14, fi0\n"
  "nrm ft0.xyz, ft0\n"
  "mov ft0.w, fc0.xxxx\n"
  "add ft1, ft0, ft6\n"
  "nrm ft1.xyz, ft1\n"
  "mov ft1.w, fc0.xxxx\n"
  "dp3 ft3.x, ft7.xyzz, ft0.xyzz\n"
  "sat ft3.x, ft3.xxxx\n"
  "dp3 ft3.y, ft7.xyzz, ft1.xyzz\n"
  "max ft3.y, ft3.yyyy, fc0.xxxx\n"
  "pow ft3.y, ft3.yyyy, fc6.yyyy\n"
  "sub ft0.x, fc0.yyyy, ft3.xxxx\n"
  "slt ft0.x, ft0.xxxx, fc0.yyyy\n"
  "mul ft3.y, ft3.yyyy, ft0.xxxx\n"
  "mul ft0.x, ft3.xxxx, fc7.wwww\n"
  "sat ft0.x, ft0.xxxx\n"
  "mul ft3.y, ft3.yyyy, ft0.xxxx\n"
  "mul ft2, fc13, ft3.zzzz\n"
  "mul ft0, ft2, ft3.xxxx\n"
  "mul ft1, ft2, ft3.yyyy\n"
  "add ft4, ft4, ft0\n"
  "add ft5, ft5, ft1\n"
  "mov ft0.xyz, fi5.xyzz\n"
  "add ft0.xy, ft0.xyyy, fc0.yyyy\n"
  "mul ft0.xy, ft0.xyyy, fc0.zzzz\n"
  "sat ft0.z, ft0.zzzz\n"
  "mul ft0.xy, ft0.xyyy, fc19.xyyy\n"
  "add ft0.xy, ft0.xyyy, fc0.zzzz\n"
  "frc ft2.xy, ft0.xyyy\n"
  "sub ft0.xy, ft0.xyyy, ft2.xyyy\n"
  "mul ft0.xy, ft0.xyyy, fc19.zwww\n"
  "mov ft1.zw, ft2.yyyy\n"
  "sub ft1.xy, fc0.yyyy, ft2.yyyy\n"
  "mul ft1.yw, ft1.yyww, ft2.xxxx\n"
  "sub ft0.w, fc0.yyyy, ft2.xxxx\n"
  "mul ft1.xz, ft1.xxzz, ft0.wwww\n"
  "mul ft1, ft1, fc0.zzzz\n"
  "sub ft0.xy, ft0.xyyy, fc19.zwww\n"
  "tex ft3, ft0, fs2 <2d, nearest, nomip, clamp>\n"
  "dp3 ft2.x, ft3, fc8\n"
  "add ft0.y, ft0.yyyy, fc19.zwww\n"
  "tex ft3, ft0, fs2 <2d, nearest, nomip, clamp>\n"
  "dp3 ft2.z, ft3, fc8\n"
  "add ft0.x, ft0.xxxx, fc19.zwww\n"
  "tex ft3, ft0, fs2 <2d, nearest, nomip, clamp>\n"
  "dp3 ft2.w, ft3, fc8\n"
  "sub ft0.y, ft0.yyyy, fc19.zwww\n"
  "tex ft3, ft0, fs2 <2d, nearest, nomip, clamp>\n"
  "dp3 ft2.y, ft3, fc8\n"
  "sge ft2, ft2, ft0.zzzz\n"
  "dp4 ft3.z, ft2, ft1\n"
  "add ft1, fc17, ft6\n"
  "nrm ft1.xyz, ft1\n"
  "mov ft1.w, fc0.xxxx\n"
  "dp3 ft3.x, ft7.xyzz, fc17\n"
  "sat ft3.x, ft3.xxxx\n"
  "dp3 ft3.y, ft7.xyzz, ft1.xyzz\n"
  "max ft3.y, ft3.yyyy, fc0.xxxx\n"
  "pow ft3.y, ft3.yyyy, fc6.yyyy\n"
  "sub ft0.x, fc0.yyyy, ft3.xxxx\n"
  "slt ft0.x, ft0.xxxx, fc0.yyyy\n"
  "mul ft3.y, ft3.yyyy, ft0.xxxx\n"
  "mul ft0.x, ft3.xxxx, fc7.wwww\n"
  "sat ft0.x, ft0.xxxx\n"
  "mul ft3.y, ft3.yyyy, ft0.xxxx\n"
  "mul ft2, fc16, ft3.zzzz\n"
  "mul ft0, ft2, ft3.xxxx\n"
  "mul ft1, ft2, ft3.yyyy\n"
  "add ft4, ft4, ft0\n"
  "add ft5, ft5, ft1\n"
  "mov ft0, fc3\n"
  "mov ft1, fc3\n"
  "mul ft1, ft1, fc7\n"
  "add ft0, ft0, ft1\n"
  "tex ft1, fi2.xyyy, fs0 <2d, nearest, nomip, repeat>\n"
  "sub ft2.x, fc0.yyyy, ft1.wwww\n"
  "mul ft2, fc4, ft2.xxxx\n"
  "add ft1, ft1, ft2\n"
  "mov ft1.w, fc0.yyyy\n"
  "mul ft4, ft4, ft1\n"
  "add ft0, ft0, ft4\n"
  "mov ft1, fc5\n"
  "mul ft5, ft5, ft1\n"
  "add ft0, ft0, ft5\n"
  "mov ft0.w, fc6.xxxx\n"
  "mul ft1.x, fi3.wwww, fc11.xxxx\n"
  "add ft1.x, ft1.xxxx, fc11.yyyy\n"
  "exp ft1.x, ft1.xxxx\n"
  "mul ft0.xyz, ft0.xyzz, ft1.xxxx\n"
  "sub ft1.y, fc0.yyyy, ft1.xxxx\n"
  "mul ft1.xyz, ft1.yyyy, fc10.xyzz\n"
  "add ft0.xyz, ft0.xyzz, ft1.xyzz\n"
  "mov fo0, ft0\n";

static char psmv_small[] = 
  "//;vertex program (agal 1)\n"
  "m44 vt0, va0, vc[va3.x+0].xyzw\n"
  "mul vt0, vt0, va3.yyyy\n"
  "m44 vt1, va0, vc[va3.z+0].xyzw\n"
  "mul vt1, vt1, va3.wwww\n"
  "add vt0, vt0, vt1\n"
  "mov vo0, vt0\n";