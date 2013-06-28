#version 130
#extension GL_ARB_texture_cube_map_array : enable

ivec3 textureSize(samplerCubeArray sampler, int lod);
ivec3 textureSize(isamplerCubeArray sampler, int lod);
ivec3 textureSize(usamplerCubeArray sampler, int lod);
ivec3 textureSize(samplerCubeArrayShadow sampler, int lod);

 vec4 texture( samplerCubeArray sampler, vec4 coord);
ivec4 texture(isamplerCubeArray sampler, vec4 coord);
uvec4 texture(usamplerCubeArray sampler, vec4 coord);
float texture( samplerCubeArrayShadow sampler, vec4 P, float compare);

 vec4 textureGrad( samplerCubeArray sampler, vec4 P, vec3 dPdx, vec3 dPdy);
ivec4 textureGrad( isamplerCubeArray sampler, vec4 P, vec3 dPdx, vec3 dPdy);
uvec4 textureGrad( usamplerCubeArray sampler, vec4 P, vec3 dPdx, vec3 dPdy);

 vec4 textureLod( samplerCubeArray sampler, vec4 P, float lod);
ivec4 textureLod( isamplerCubeArray sampler, vec4 P, float lod);
uvec4 textureLod( usamplerCubeArray sampler, vec4 P, float lod);
