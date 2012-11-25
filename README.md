GLSL To AGAL Compiler
=====================

This project provides a library and standalone tool for converting GLSL vertex and fragment shaders into AGAL shaders that can be used with the Stage3D API in the Adobe Flash runtime.

GLSL To AGAL Compiler is licensed according to the MIT License. See individual files for exact licensing, other code that doesn't form part of the glsl2agal compiler binary or SWC is licensed differently, please check individual files before reusing any code within this repository.

A live demo showing the glsl2agal compiler is running here: http://adobe.github.com/glsl2agal/

How does it work
----------------

The tool is based off the Mesa codebase and has full support for GLSL 1.3 syntax. After parsing the GLSL shader various optimizations are run on the code in order to generate efficient AGAL that fits within all of the constraints of the basic profile of the Stage3D API.

The general philosophy in the conversion process is that if it is possible to represent a given GLSL shader as AGAL then this tool should be able to perform the conversion without any problem. Conversly this tool cannot create AGAL shaders that you couldn't write by hand, so it cannot work around any inherrant limitations in the Stage3D API (texture reads in vertex shaders, for example).

Some things that are not available in AGAL, such as function calls and loops *will* work in cases where they are largely just syntactic sugar that the compiler can remove during optimization e.g. Function calls can all be inlined, and static loops can all be unrolled.

Some examples of conversion errors
- texture access in vertex shaders
- data-dependent loops (ones that can't be statically unrolled at compile time)
- functions that can't be implemented without gpu support (e.g. screen-space differentials)

Building
--------

A prebuilt copy of the standalone compiler (for OSX) and SWC is available in the bin directory.

To build the SWC and the standalone compiler with the AGAL optimizer:
<pre>
	make FLASCC=/path/to/flascc/sdk FLEX=/path/to/flexsdk
</pre>

To build the example SWF:

<pre>
	make FLASCC=/path/to/flascc/sdk FLEX=/path/to/flexsdk example
</pre>

Using the SWC
-------------

Look at the code in examples/Basic/GLSLCompiler.mxml for a simple example of how the SWC can be imported and used to compile GLSL into AGAL assembly.

Using the standalone tool
-------------------------

To demonstrate the tool run the following commands:

<pre>
./bin/glsl2agalopt -optimize -f tests/simple.fs
./bin/glsl2agalopt -optimize -v tests/simple.vs
</pre>

Option | Description
:-------:|------------:|
 -f/-v | Specify type (fragment / vertex shader)
 -e | Shader is for GLES rather than OpenGL |
 -d | dump out intermediate GLSL between optimization passes (useful when debugging) |
 -optimize | run the AGAL optimizer on the final AGAL |

The resulting ".out" files contain the generated AGAL asm along with the information needed to connect up the various inputs to the AGAL.

Handling the output
-------------------

The output from the tool is a JSON object containing several fields:

infolog - If there were any conversion errors or syntactic errors in the source glsl then this will contain the errors or warnings that were generated.

varnames - this dictionary maps from GLSL variable name to AGAL register name

storage - This dictionary maps from AGAL register name to glsl storage type (uniform, in/out, temp etc)

types - this maps from AGAL register name to the glsl type of the variable stored in that register (vec4, sampler2D etc)

consts - This maps from AGAL register name to an array of 4 floating point values that should be stored in that register before executing the shader.

agalasm (optional) - Contains the AGAL asm suitable for assembly by the AGALMiniAssembler

agalbin (optional) - Contains the binary AGAL shader code that can be uploaded directly to stage3D

The information in this JSON object should be sufficient to be able to hook up the necessary vertex attribute streams and set all of the required uniforms into the right register. Exactly how this integration is done will depend on the engine you are using.
