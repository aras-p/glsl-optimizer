GLSL optimizer Change Log
=========================


2026 11
-------

Added a whole bunch of stuff that could be summarized as "compute shader support":

* Compute shaders are supported, including work group sizes, various built-in variables,
  shared memory, workgroup synchronization. Reflection API was extended to query work group sizes.
* Uniform buffers (UBO) and storage buffers (SSBOs) are actually parsed and get through
  to output correctly. Reflection API was extended to be able to query buffers and
  their member data.
* Atomic operations, barriers and atomic counters.
* Image load, store and atomics.
* Stage interface blocks.
* Texture gather operations.
* Recognize `GL_OES_EGL_image_external_essl3` in GLES3 inputs.

Changed:

* Removed OpenGL ES 2.0 language input/output support; for GLES only `#version 300 es` and later
  shaders are accepted and produced now.
* `lowp` and `mediump` precision are treated as the same precision now. Last time any GPU
  had _actual_ `lowp` was, like, 15 years ago.
* Updated underlying Mesa compiler/optimizer codebase from 10.4-dev (2014 Oct) to Mesa 20.3.5 (2021 Mar).
  This slightly improves constant folding, dead-code elimination, vector assignment
  simplification, and arithmetic optimization. It also seems to have improved performance
  of the optimizer itself (20-30% faster across tests). I did not pick more recent Mesa since it moved
  a lot of optimization to their new "NIR" architecture, which would have been a lot more work. Maybe someday!

Build:

* Moved to CMake build and C++17 version, updates for recent compilers/toolchains.
* Compilation and tests are now ran on Github Actions.
* Linux build now uses `-fPIC` by default (#52).

Fixed:

* Fixed `glsl_types::interface_types` memory leak (#141).
* Fixed missing precision on `sampler2DArray` (#146).
* Fixed GLSL generation for `OES_EGL_image_external` (#96).
* Fixed variables with double underscores (`__`) being generated in some cases (#61).
* Fixed Metal output being incorrect for multiple `gl_FragData` outputs in some cases (#114).
* Fixed broadcast of scalar bool not being correct in some cases (#73).
* Fixed precision for constant literals being dropped in some cases (#90).
* Fixed GLSL explicit varying locations not being respected.
* Metal output now preserves explicit vertex attribute locations.

2016 10
-------

* Metal/GLES: Fixed bad optimization (all code removed) when framebuffer fetch extension is used, but
  fragment shader does not actually read the incoming color value.
* Fixed translation of texelSize().
* Fixed translation of texelFetch().


2016 09
-------

* Metal: Fixed constant precision propagation in some cases.
* Metal: Fixed shadowmap sampling when reference Z value is outside of 0..1 range (now clamps to match GLES specs).


2016 06
-------

Fixed:

* Fixed Metal translation in some cases having wrong precision on constants or constant arrays.


2016 05
-------

Fixed:

* Fixed Metal translation in some cases having wrong precision on struct members.
* Fixed Metal translation in some cases emitting struct declarations vs. constant initializers in wrong order.


2016 03
-------

Fixed:

* Fixed translation performance regression in loop analysis (regressed in 2015 06 fixes).


2015 08
-------

Changes:

* 2D shadow and 2D array uniforms got their own glslopt_basic_type entries.

Fixes:

* Fixed translation of 2D texture arrays (GLSL with EXT_texture_array, GLES3 and Metal).


2015 06
-------

Fixes:

* Fixed some cases of different precision matrix assignments being miscompiled on Metal.
* Fixed yet more issues with translation of weird loops.
* Fixed translation of matrix+scalar, matrix-scalar, matrix/scalar operations on Metal.


2015 05
-------

Fixes:

* Fixes some cases of highp/mediump sampler sampling resulting in resulting temporaries wrongly being lowp.


2015 04
-------

Goodies:

* GLES2: support EXT_draw_instanced / gl_InstanceIDEXT.
* Support gl_VertexID in GLSL < 1.30 when EXT_gpu_shader4 is used.

Fixes:

* Metal: fixed some bugs with translation of weird loops.


2015 02
-------

Tweaks:

* Texture LOD sampling functions on GLES2.0 now produce a wrapper call, that does approximation
  (mip bias) on devices that don't support GL_EXT_shader_texture_lod.
* Undefined precision integers on GLES now default to highp.


2015 01
-------

Fixes:

* Float literals are printed with 7 significant digits now.
* Metal: GLSL mod() is properly translated into Metal's fmod().
* Metal: Fixed some cases of reciprocal (1/x) printing missing half precision cast.
* GLES3: textureOffset with a mipmap bias is printed correctly now.
* Fixed a bug with loop inductor detection if the inductor was used before the loop for some things.
* Fixed printing of int/float bitcast operations.
* Improved precision determination of some constructs.


2014 10
-------

Goodies:

* Support for translating GLSL shaders into Apple Metal shading language.
  Exactly same process; GLSL in (preferably ES3 variant), optimization passes, Metal out.
  All uniforms currently will be put into one constant buffer.
  Pass kGlslTargetMetal target to get Metal.
* Shader reflection API. See glslopt_shader_get_* functions. Binding indices
  are only automatically assigned on Metal now; on GL/ES targets the reflection API is only
  useful to get list of inputs/uniforms, their names and types.
* Improved dead code elimation: some cases of swizzled/masked assignments where same variable was on both sides
  were not eliminated if that variable was totally unused later.
* Merged with upstream Mesa, comes with new optimizations (min/max pruning, tree rebalancing, vector_insert lowering).

Fixes:

* sampler3D declarations were missing precision qualifier on GLES3.0.


2014 09
-------

Goodies:

* Supports GL_EXT_draw_buffers in ES2.0, for MRT.


2014 08
-------

Goodies:

* Supports GL_EXT_shader_framebuffer_fetch now, in both ES2.0 & 3.0.

Fixes:

* Fixed printing of infinities & NaNs.
* Fixed vectorization pass in some cases going wrong on texture lookups.


2014 06
-------

Goodies:

* Optimization: split vectors with only some used components into scalars.

Fixes:

* Fixed more issues with for-loop printing.
* Fixed printing of unsigned integer swizzled & constants.

2014 03
-------

Fixes:

* Fixed missing precision qualifier in some ES shaders (mostly due to expansion of ternary ?: check).

2014 02
-------

Fixes:

* Fixed vectorize pass introduced last month going wrong with dot products.

2014 01
-------

Goodies:

* Better optimization: vectorize assignments to individual vector channels.
* More OpenGL ES 2.0 conformant printing of complex for-loops (loop inductions printed
  as += or ++; loop initializers inside loop body).

Fixes:

* Fixed array assignments sometimes appearing in pre-GLSL1.20 versions, especially with
  complex loops that couldn't be unrolled.
* Fixed output of textureOffset and texelFetch.
* Fixed error messages on MRT outputs on GL & GLES3 (now supports 4 MRTs).

2013 12
-------

Goodies:

* Optimized performance; was spending half of the time in stupid string code.
* Added glslopt_shader_get_stats to get *very* approximate shader complexity stats.
* Nicer printing of complicated for-loops.

Fixes:

* Fixed printing of struct initializers.


2013 11
-------

Goodies:

* Better optimizations: CSE; `A+(-B) => A-B`; `!A || !B => !(A && B)`.
* Merged with upstream Mesa.

Fixes:

* Fixed location qualifiers, ES3.0 version printing, samplerCubeShadow sampling operations.


2013 10
-------

Goodies:

* Initial OpenGL ES 3.0 support
* API to query shader input names; glslopt_shader_get_input_count and glslopt_shader_get_input_name

Changes:

* Xcode project files updated to Xcode 5

Fixes:

* VS2013 fixes
