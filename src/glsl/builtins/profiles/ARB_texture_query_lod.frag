#version 130
#extension GL_ARB_texture_query_lod : enable
#extension GL_ARB_texture_cube_map_array : enable

vec2 textureQueryLOD( sampler1D sampler, float coord);
vec2 textureQueryLOD(isampler1D sampler, float coord);
vec2 textureQueryLOD(usampler1D sampler, float coord);

vec2 textureQueryLOD( sampler2D sampler, vec2 coord);
vec2 textureQueryLOD(isampler2D sampler, vec2 coord);
vec2 textureQueryLOD(usampler2D sampler, vec2 coord);

vec2 textureQueryLOD( sampler3D sampler, vec3 coord);
vec2 textureQueryLOD(isampler3D sampler, vec3 coord);
vec2 textureQueryLOD(usampler3D sampler, vec3 coord);

vec2 textureQueryLOD( samplerCube sampler, vec3 coord);
vec2 textureQueryLOD(isamplerCube sampler, vec3 coord);
vec2 textureQueryLOD(usamplerCube sampler, vec3 coord);

vec2 textureQueryLOD( sampler1DArray sampler, float coord);
vec2 textureQueryLOD(isampler1DArray sampler, float coord);
vec2 textureQueryLOD(usampler1DArray sampler, float coord);

vec2 textureQueryLOD( sampler2DArray sampler, vec2 coord);
vec2 textureQueryLOD(isampler2DArray sampler, vec2 coord);
vec2 textureQueryLOD(usampler2DArray sampler, vec2 coord);

vec2 textureQueryLOD( samplerCubeArray sampler, vec3 coord);
vec2 textureQueryLOD(isamplerCubeArray sampler, vec3 coord);
vec2 textureQueryLOD(usamplerCubeArray sampler, vec3 coord);

vec2 textureQueryLOD(sampler1DShadow sampler, float coord);
vec2 textureQueryLOD(sampler2DShadow sampler, vec2 coord);
vec2 textureQueryLOD(samplerCubeShadow sampler, vec3 coord);
vec2 textureQueryLOD(sampler1DArrayShadow sampler, float coord);
vec2 textureQueryLOD(sampler2DArrayShadow sampler, vec2 coord);
vec2 textureQueryLOD(samplerCubeArrayShadow sampler, vec3 coord);
