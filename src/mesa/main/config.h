/* $Id: config.h,v 1.39 2002/06/15 02:38:15 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Tunable configuration parameters.
 */



#ifndef CONFIG_H
#define CONFIG_H

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif


/*
 * OpenGL implementation limits
 */

/* Maximum modelview matrix stack depth: */
#define MAX_MODELVIEW_STACK_DEPTH 32

/* Maximum projection matrix stack depth: */
#define MAX_PROJECTION_STACK_DEPTH 32

/* Maximum texture matrix stack depth: */
#define MAX_TEXTURE_STACK_DEPTH 10

/* Maximum color matrix stack depth: */
#define MAX_COLOR_STACK_DEPTH 4

/* Vertex program matrix stacks: */
#define MAX_PROGRAM_MATRICES 8
#define MAX_PROGRAM_STACK_DEPTH 4

/* Maximum attribute stack depth: */
#define MAX_ATTRIB_STACK_DEPTH 16

/* Maximum client attribute stack depth: */
#define MAX_CLIENT_ATTRIB_STACK_DEPTH 16

/* Maximum recursion depth of display list calls: */
#define MAX_LIST_NESTING 64

/* Maximum number of lights: */
#define MAX_LIGHTS 8

/* Maximum user-defined clipping planes: */
#define MAX_CLIP_PLANES 6

/* Maximum pixel map lookup table size: */
#define MAX_PIXEL_MAP_TABLE 256

/* Number of auxillary color buffers: */
#define NUM_AUX_BUFFERS 0

/* Maximum order (degree) of curves: */
#ifdef AMIGA
#   define MAX_EVAL_ORDER 12
#else
#   define MAX_EVAL_ORDER 30
#endif

/* Maximum Name stack depth */
#define MAX_NAME_STACK_DEPTH 64

/* Min and Max point sizes and granularity */
#define MIN_POINT_SIZE 1.0
#define MAX_POINT_SIZE 20.0
#define POINT_SIZE_GRANULARITY 0.1

/* Min and Max line widths and granularity */
#define MIN_LINE_WIDTH 1.0
#define MAX_LINE_WIDTH 10.0
#define LINE_WIDTH_GRANULARITY 0.1

/* Max texture palette / color table size */
#define MAX_COLOR_TABLE_SIZE 256

/* Number of 1D/2D texture mipmap levels */
#define MAX_TEXTURE_LEVELS 12

/* Number of 3D texture mipmap levels */
#define MAX_3D_TEXTURE_LEVELS 8

/* Number of cube texture mipmap levels */
#define MAX_CUBE_TEXTURE_LEVELS 12

/* Number of texture units - GL_ARB_multitexture */
#define MAX_TEXTURE_UNITS 8

/* Maximum viewport/image size: */
#define MAX_WIDTH 2048
#define MAX_HEIGHT 2048

/* Maxmimum size for CVA.  May be overridden by the drivers.  */
#define MAX_ARRAY_LOCK_SIZE 3000

/* Subpixel precision for antialiasing, window coordinate snapping */
#define SUB_PIXEL_BITS 4

/* Size of histogram tables */
#define HISTOGRAM_TABLE_SIZE 256

/* Max convolution filter sizes */
#define MAX_CONVOLUTION_WIDTH 9
#define MAX_CONVOLUTION_HEIGHT 9

/* GL_ARB_texture_compression */
#define MAX_COMPRESSED_TEXTURE_FORMATS 25

/* GL_EXT_texture_filter_anisotropic */
#define MAX_TEXTURE_MAX_ANISOTROPY 16.0

/* GL_EXT_texture_lod_bias */
#define MAX_TEXTURE_LOD_BIAS 4.0



/*
 * Mesa-specific parameters
 */


/*
 * Bits per accumulation buffer color component:  8, 16 or 32
 */
#define ACCUM_BITS 32


/*
 * Bits per depth buffer value.  Any reasonable value up to 31 will
 * work.  32 doesn't work because of integer overflow problems in the
 * rasterizer code.
 */
#ifndef DEFAULT_SOFTWARE_DEPTH_BITS
#define DEFAULT_SOFTWARE_DEPTH_BITS 16
#endif
#if DEFAULT_SOFTWARE_DEPTH_BITS <= 16
#define DEFAULT_SOFTWARE_DEPTH_TYPE GLushort
#else
#define DEFAULT_SOFTWARE_DEPTH_TYPE GLuint
#endif



/*
 * Bits per stencil value:  8
 */
#define STENCIL_BITS 8


/*
 * Bits per color channel:  8, 16 or 32
 */
#ifndef CHAN_BITS
#define CHAN_BITS 8
#endif


/*
 * Color channel component order
 * (changes will almost certainly cause problems at this time)
 */
#define RCOMP 0
#define GCOMP 1
#define BCOMP 2
#define ACOMP 3


#endif /* CONFIG_H */
