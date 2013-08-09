#version 300 es
precision highp float;

/* texture - bias variants */
 vec4 texture( sampler2D sampler, vec2 P, float bias);
ivec4 texture(isampler2D sampler, vec2 P, float bias);
uvec4 texture(usampler2D sampler, vec2 P, float bias);

 vec4 texture( sampler3D sampler, vec3 P, float bias);
ivec4 texture(isampler3D sampler, vec3 P, float bias);
uvec4 texture(usampler3D sampler, vec3 P, float bias);

 vec4 texture( samplerCube sampler, vec3 P, float bias);
ivec4 texture(isamplerCube sampler, vec3 P, float bias);
uvec4 texture(usamplerCube sampler, vec3 P, float bias);

float texture(sampler2DShadow   sampler, vec3 P, float bias);
float texture(samplerCubeShadow sampler, vec4 P, float bias);

 vec4 texture( sampler2DArray sampler, vec3 P, float bias);
ivec4 texture(isampler2DArray sampler, vec3 P, float bias);
uvec4 texture(usampler2DArray sampler, vec3 P, float bias);

float texture(sampler2DArrayShadow sampler, vec4 P, float bias);

/* textureProj - bias variants */
 vec4 textureProj( sampler2D sampler, vec3 P, float bias);
ivec4 textureProj(isampler2D sampler, vec3 P, float bias);
uvec4 textureProj(usampler2D sampler, vec3 P, float bias);
 vec4 textureProj( sampler2D sampler, vec4 P, float bias);
ivec4 textureProj(isampler2D sampler, vec4 P, float bias);
uvec4 textureProj(usampler2D sampler, vec4 P, float bias);

 vec4 textureProj( sampler3D sampler, vec4 P, float bias);
ivec4 textureProj(isampler3D sampler, vec4 P, float bias);
uvec4 textureProj(usampler3D sampler, vec4 P, float bias);

float textureProj(sampler2DShadow sampler, vec4 P, float bias);

/* textureOffset - bias variants */
 vec4 textureOffset( sampler2D sampler, vec2 P, ivec2 offset, float bias);
ivec4 textureOffset(isampler2D sampler, vec2 P, ivec2 offset, float bias);
uvec4 textureOffset(usampler2D sampler, vec2 P, ivec2 offset, float bias);

 vec4 textureOffset( sampler3D sampler, vec3 P, ivec3 offset, float bias);
ivec4 textureOffset(isampler3D sampler, vec3 P, ivec3 offset, float bias);
uvec4 textureOffset(usampler3D sampler, vec3 P, ivec3 offset, float bias);

float textureOffset(sampler2DShadow sampler, vec3 P, ivec2 offset, float bias);

 vec4 textureOffset( sampler2DArray sampler, vec3 P, ivec2 offset, float bias);
ivec4 textureOffset(isampler2DArray sampler, vec3 P, ivec2 offset, float bias);
uvec4 textureOffset(usampler2DArray sampler, vec3 P, ivec2 offset, float bias);

/* textureProjOffsetOffset - bias variants */
 vec4 textureProjOffset( sampler2D sampler, vec3 P, ivec2 offset, float bias);
ivec4 textureProjOffset(isampler2D sampler, vec3 P, ivec2 offset, float bias);
uvec4 textureProjOffset(usampler2D sampler, vec3 P, ivec2 offset, float bias);
 vec4 textureProjOffset( sampler2D sampler, vec4 P, ivec2 offset, float bias);
ivec4 textureProjOffset(isampler2D sampler, vec4 P, ivec2 offset, float bias);
uvec4 textureProjOffset(usampler2D sampler, vec4 P, ivec2 offset, float bias);

 vec4 textureProjOffset( sampler3D sampler, vec4 P, ivec3 offset, float bias);
ivec4 textureProjOffset(isampler3D sampler, vec4 P, ivec3 offset, float bias);
uvec4 textureProjOffset(usampler3D sampler, vec4 P, ivec3 offset, float bias);

float textureProjOffset(sampler2DShadow s, vec4 P, ivec2 offset, float bias);

/*
 * 8.9 - Fragment Processing Functions
 */
float dFdx(float p);
vec2  dFdx(vec2  p);
vec3  dFdx(vec3  p);
vec4  dFdx(vec4  p);

float dFdy(float p);
vec2  dFdy(vec2  p);
vec3  dFdy(vec3  p);
vec4  dFdy(vec4  p);

float fwidth(float p);
vec2  fwidth(vec2  p);
vec3  fwidth(vec3  p);
vec4  fwidth(vec4  p);
