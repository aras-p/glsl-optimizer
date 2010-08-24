GLSL optimizer
==============

A C++ library that takes GLSL shaders, does some GPU-independent optimizations on them
and outputs GLSL back. Optimizations are function inlining, dead code removal, copy propagation,
constant folding, constant propagation, arithmetic optimizations and so on.

Apparently quite a few mobile platforms are pretty bad at optimizing GLSL shaders; and
unfortunately they *also* lack offline shader compilers. So using a GLSL optimizer offline
before can make the shader run much faster on a platform like that. I've seen shaders becoming
a dozen times faster on one mobile platform!

Almost all actual code is [Mesa 3D's GLSL2](http://cgit.freedesktop.org/mesa/mesa/log/?h=glsl2)
compiler; all this library does is spits out optimized GLSL back.

This GLSL optimizer is made for [Unity's](http://unity3d.com/) purposes and will be built-in
in upcoming [Unity 3.0](http://unity3d.com/unity/coming-soon/unity-3). If you have early beta
access then it is there in Unity 3.0.0 beta 6 build.


Usage
-----

Visual Studio 2008 (Windows, x86) and Xcode 3.2 (Mac, i386) project files for a static
library are provided in `src/glsl/msvc/mesaglsl2.vcproj` and `src/glsl/xcode/mesaglsl2`
respectively.

Interface for the library is `src/glsl/glsl_optimizer.h`. General usage is:
 
	ctx = glslopt_initialize();
	for (lots of shaders) {
		shader = glslopt_optimize (ctx, shaderType, shaderSource, options);
		if (glslopt_get_status (shader)) {
			newSource = glslopt_get_output (shader);
		} else {
			errorLog = glslopt_get_log (shader);
		}
		glslopt_shader_delete (shader);
	}
	glslopt_cleanup (ctx);

Notes
-----

* I haven't checked if/how it works with higher GLSL versions than the
  default (1.10?), or with GLSL ES syntax.
  