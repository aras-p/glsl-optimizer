#version 130
#extension GL_ARB_texture_cube_map_array : enable

 vec4 texture( samplerCubeArray sampler, vec4 coord, float bias);
ivec4 texture(isamplerCubeArray sampler, vec4 coord, float bias);
uvec4 texture(usamplerCubeArray sampler, vec4 coord, float bias);
