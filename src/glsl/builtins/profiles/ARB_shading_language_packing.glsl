#version 130
#extension GL_ARB_shading_language_packing : enable

highp   uint packSnorm2x16(        vec2 v);
highp   uint packUnorm2x16(        vec2 v);
highp   uint packSnorm4x8 (        vec4 v);
highp   uint packUnorm4x8 (        vec4 v);
highp   uint packHalf2x16 (mediump vec2 v);

highp   vec2 unpackSnorm2x16(highp uint p);
highp   vec2 unpackUnorm2x16(highp uint p);
highp   vec4 unpackSnorm4x8 (highp uint p);
highp   vec4 unpackUnorm4x8 (highp uint p);
mediump vec2 unpackHalf2x16 (highp uint p);
