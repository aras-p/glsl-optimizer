// GLSL image format names, and Metal texture type names for image dimensions.
#pragma once

#include "util/format/u_formats.h"
#include "compiler/glsl_types.h"

inline const char* glslopt_image_format_name(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_R32G32B32A32_FLOAT: return "rgba32f";
   case PIPE_FORMAT_R16G16B16A16_FLOAT: return "rgba16f";
   case PIPE_FORMAT_R32G32_FLOAT: return "rg32f";
   case PIPE_FORMAT_R16G16_FLOAT: return "rg16f";
   case PIPE_FORMAT_R11G11B10_FLOAT: return "r11f_g11f_b10f";
   case PIPE_FORMAT_R32_FLOAT: return "r32f";
   case PIPE_FORMAT_R16_FLOAT: return "r16f";
   case PIPE_FORMAT_R32G32B32A32_UINT: return "rgba32ui";
   case PIPE_FORMAT_R16G16B16A16_UINT: return "rgba16ui";
   case PIPE_FORMAT_R10G10B10A2_UINT: return "rgb10_a2ui";
   case PIPE_FORMAT_R8G8B8A8_UINT: return "rgba8ui";
   case PIPE_FORMAT_R32G32_UINT: return "rg32ui";
   case PIPE_FORMAT_R16G16_UINT: return "rg16ui";
   case PIPE_FORMAT_R8G8_UINT: return "rg8ui";
   case PIPE_FORMAT_R32_UINT: return "r32ui";
   case PIPE_FORMAT_R16_UINT: return "r16ui";
   case PIPE_FORMAT_R8_UINT: return "r8ui";
   case PIPE_FORMAT_R32G32B32A32_SINT: return "rgba32i";
   case PIPE_FORMAT_R16G16B16A16_SINT: return "rgba16i";
   case PIPE_FORMAT_R8G8B8A8_SINT: return "rgba8i";
   case PIPE_FORMAT_R32G32_SINT: return "rg32i";
   case PIPE_FORMAT_R16G16_SINT: return "rg16i";
   case PIPE_FORMAT_R8G8_SINT: return "rg8i";
   case PIPE_FORMAT_R32_SINT: return "r32i";
   case PIPE_FORMAT_R16_SINT: return "r16i";
   case PIPE_FORMAT_R8_SINT: return "r8i";
   case PIPE_FORMAT_R16G16B16A16_UNORM: return "rgba16";
   case PIPE_FORMAT_R10G10B10A2_UNORM: return "rgb10_a2";
   case PIPE_FORMAT_R8G8B8A8_UNORM: return "rgba8";
   case PIPE_FORMAT_R16G16_UNORM: return "rg16";
   case PIPE_FORMAT_R8G8_UNORM: return "rg8";
   case PIPE_FORMAT_R16_UNORM: return "r16";
   case PIPE_FORMAT_R8_UNORM: return "r8";
   case PIPE_FORMAT_R16G16B16A16_SNORM: return "rgba16_snorm";
   case PIPE_FORMAT_R8G8B8A8_SNORM: return "rgba8_snorm";
   case PIPE_FORMAT_R16G16_SNORM: return "rg16_snorm";
   case PIPE_FORMAT_R8G8_SNORM: return "rg8_snorm";
   case PIPE_FORMAT_R16_SNORM: return "r16_snorm";
   case PIPE_FORMAT_R8_SNORM: return "r8_snorm";
   default: return nullptr;
   }
}

inline const char* glslopt_metal_image_type_name(const glsl_type* type)
{
   bool array = glsl_sampler_type_is_array(type);
   switch (glsl_get_sampler_dim(type)) {
   case GLSL_SAMPLER_DIM_1D: return array ? "texture1d_array" : "texture1d";
   case GLSL_SAMPLER_DIM_2D: return array ? "texture2d_array" : "texture2d";
   case GLSL_SAMPLER_DIM_3D: return "texture3d";
   case GLSL_SAMPLER_DIM_CUBE: return array ? "texturecube_array" : "texturecube";
   case GLSL_SAMPLER_DIM_BUF: return "texture_buffer";
   case GLSL_SAMPLER_DIM_MS: return array ? "texture2d_ms_array" : "texture2d_ms";
   default: return nullptr;
   }
}
