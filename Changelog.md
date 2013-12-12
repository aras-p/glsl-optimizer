GLSL optimizer Change Log
=========================

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
