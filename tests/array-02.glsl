#version 120
/* FAIL - array size type must be scalar */
uniform vec4 [ivec4(3)] a;
