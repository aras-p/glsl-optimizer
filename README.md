GLSL To AGAL Compiler
=====================

This project provides a library and standalone tool for converting GLSL vertex and fragment shaders into AGAL shaders that can be used with the Stage3D API in the Adobe Flash runtime.

GLSL To AGAL Compiler Is licensed according to the MIT License. See individual files for exact licensing.

How does it work
----------------

The tool is based off the Mesa codebase and has full support for GLSL 1.3 syntax. After parsing the GLSL shader various optimizations are run on the code in order to generate efficient AGAL that fits within all of the constraints of the basic profile of the Stage3D API.

The general philosophy in the conversion process is that if it is possible to represent a given GLSL shader as AGAL then this tool should be able to perform the conversion without any problem. Conversly this tool cannot create AGAL shaders that you couldn't write by hand, so it cannot work around any inherrant limitations in the Stage3D API (texture reads in vertex shaders, for example).

Some things that are not available in AGAL, such as function calls and loops *will* work in cases where they are largely just syntactic sugar that the compiler can remove during optimization e.g. Function calls can all be inlined, and static loops can all be unrolled.

Some examples of conversion errors
- texture access in vertex shaders
- data-dependent loops (ones that can't be statically unrolled at compile time)
- functions that can't be implemented without gpu support (e.g. screen-space differentials)

Using the SWC
-------------

TBD

Using the standalone tool
-------------------------

TBD

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

Proscenium Example
------------------

The proscenium example displays a spinning teapot that can be rendered using a user supplied GLSL vertex/fragment shader pair. The HTML ui allows you to modify the shader at runtime and an overlaid flex UI allows you to interactively modify any uniform variables exposed by the programs.


Starling Example
----------------

The Starling example uses the BunnyMark demo but replaces the agal shader used to render the background element with an embedded GLSL shader that is compiled at runtime using the glsl2agal swc.

Take a look in Background.as to see how you invoke the compiler on some GLSL source and then how you inspect the resulting json dictionary to figure out where to map the uniform values using the standard stage3D APIs.