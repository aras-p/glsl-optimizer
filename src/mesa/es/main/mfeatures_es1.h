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
 * \file mfeatures.h
 *
 * The #defines in this file enable/disable Mesa features needed
 * for OpenGL ES 1.1.
 */


#ifndef MFEATURES_ES1_H
#define MFEATURES_ES1_H

/* this file replaces main/mfeatures.h */
#ifdef FEATURES_H
#error "main/mfeatures.h was wrongly included"
#endif
#define FEATURES_H

#define ASSERT_NO_FEATURE() ASSERT(0)

/*
 * Enable/disable features (blocks of code) by setting FEATURE_xyz to 0 or 1.
 */
#ifndef _HAVE_FULL_GL
#define _HAVE_FULL_GL 1
#endif

#ifdef IN_DRI_DRIVER
#define FEATURE_remap_table 1
#else
#define FEATURE_remap_table 0
#endif

#define FEATURE_accum 0
#define FEATURE_arrayelt 0
#define FEATURE_attrib 0
#define FEATURE_beginend 0
#define FEATURE_colortable 0
#define FEATURE_convolve 0
#define FEATURE_dispatch 1
#define FEATURE_dlist 0
#define FEATURE_draw_read_buffer 0
#define FEATURE_drawpix 0
#define FEATURE_eval 0
#define FEATURE_feedback 0
#define FEATURE_fixedpt 1
#define FEATURE_histogram 0
#define FEATURE_pixel 0
#define FEATURE_point_size_array 1
#define FEATURE_queryobj 0
#define FEATURE_rastpos 0
#define FEATURE_texgen 1
#define FEATURE_texture_fxt1 0
#define FEATURE_texture_s3tc 0
#define FEATURE_userclip 1
#define FEATURE_vertex_array_byte 1
#define FEATURE_es2_glsl 0

#define FEATURE_ARB_fragment_program  _HAVE_FULL_GL
#define FEATURE_ARB_vertex_buffer_object  _HAVE_FULL_GL
#define FEATURE_ARB_vertex_program  _HAVE_FULL_GL

#define FEATURE_ARB_vertex_shader _HAVE_FULL_GL
#define FEATURE_ARB_fragment_shader _HAVE_FULL_GL
#define FEATURE_ARB_shader_objects (FEATURE_ARB_vertex_shader || FEATURE_ARB_fragment_shader)
#define FEATURE_ARB_shading_language_100 FEATURE_ARB_shader_objects
#define FEATURE_ARB_shading_language_120 FEATURE_ARB_shader_objects

#define FEATURE_EXT_framebuffer_blit 0
#define FEATURE_EXT_framebuffer_object _HAVE_FULL_GL
#define FEATURE_EXT_pixel_buffer_object  _HAVE_FULL_GL
#define FEATURE_EXT_texture_sRGB 0
#define FEATURE_ATI_fragment_shader 0
#define FEATURE_MESA_program_debug  _HAVE_FULL_GL
#define FEATURE_NV_fence 0
#define FEATURE_NV_fragment_program 0
#define FEATURE_NV_vertex_program 0

#define FEATURE_OES_framebuffer_object 1
#define FEATURE_OES_draw_texture 1
#define FEATURE_OES_mapbuffer 1

#define FEATURE_OES_EGL_image 1

#define FEATURE_extra_context_init 1

/*@}*/




#endif /* MFEATURES_ES1_H */
