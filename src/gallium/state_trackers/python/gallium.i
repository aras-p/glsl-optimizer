 /**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * SWIG interface definion for Gallium types.
 *
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

%module gallium;

%{

#include <stdio.h>
#include <Python.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h" 
#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h" 
#include "util/u_tile.h" 
#include "tgsi/tgsi_text.h" 
#include "tgsi/tgsi_dump.h" 

#include "st_device.h"
#include "st_sample.h"

%}

%include "typemaps.i"

%include "carrays.i"
%array_class(unsigned char, ByteArray);
%array_class(int, IntArray);
%array_class(unsigned, UnsignedArray);
%array_class(float, FloatArray);


%rename(Device) st_device;
%rename(Context) st_context;
%rename(Texture) pipe_texture;
%rename(Surface) pipe_surface;
%rename(Buffer) st_buffer;

%rename(BlendColor) pipe_blend_color;
%rename(Blend) pipe_blend_state;
%rename(Clip) pipe_clip_state;
%rename(ConstantBuffer) pipe_constant_buffer;
%rename(Depth) pipe_depth_state;
%rename(Stencil) pipe_stencil_state;
%rename(Alpha) pipe_alpha_state;
%rename(DepthStencilAlpha) pipe_depth_stencil_alpha_state;
%rename(FormatBlock) pipe_format_block;
%rename(Framebuffer) pipe_framebuffer_state;
%rename(PolyStipple) pipe_poly_stipple;
%rename(Rasterizer) pipe_rasterizer_state;
%rename(Sampler) pipe_sampler_state;
%rename(Scissor) pipe_scissor_state;
%rename(Shader) pipe_shader_state;
%rename(VertexBuffer) pipe_vertex_buffer;
%rename(VertexElement) pipe_vertex_element;
%rename(Viewport) pipe_viewport_state;


%include "p_compiler.i"
%include "pipe/p_defines.h";
%include "p_format.i"

%include "p_device.i"
%include "p_context.i"
%include "p_texture.i"
%include "p_state.i"

