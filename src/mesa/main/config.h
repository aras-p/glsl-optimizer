/* $Id: config.h,v 1.20 2000/10/28 18:34:48 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#define MAX_POINT_SIZE 10.0
#define POINT_SIZE_GRANULARITY 0.1

/* Min and Max line widths and granularity */
#define MIN_LINE_WIDTH 1.0
#define MAX_LINE_WIDTH 10.0
#define LINE_WIDTH_GRANULARITY 0.1

/* Max texture palette / color table size */
#define MAX_COLOR_TABLE_SIZE 256

/* Number of texture levels */
#define MAX_TEXTURE_LEVELS 12

/* Number of texture units - GL_ARB_multitexture */
#define MAX_TEXTURE_UNITS 3

/* Maximum viewport/image size: */
#define MAX_WIDTH 2048
#define MAX_HEIGHT 1200

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



/*
 * Mesa-specific parameters
 */


/*
 * Bits per accumulation buffer color component:  8 or 16
 */
#define ACCUM_BITS 16


/*
 * Bits per depth buffer value.  Any reasonable value up to 31 will
 * work.  32 doesn't work because of integer overflow problems in the
 * rasterizer code.
 */
#define DEFAULT_SOFTWARE_DEPTH_BITS 16
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
 * Bits per color channel (must be 8 at this time!)
 */
#define CHAN_BITS 8
#define CHAN_MAX ((1 << CHAN_BITS) - 1)
#define CHAN_MAXF ((GLfloat) CHAN_MAX)
#if CHAN_BITS == 8
   typedef GLubyte GLchan;
#elif CHAN_BITS == 16
   typedef GLushort GLchan;
#else
#error  illegal number of color channel bits
#endif





/*
 * Color channel component order
 * (changes will almost certainly cause problems at this time)
 */
#define RCOMP 0
#define GCOMP 1
#define BCOMP 2
#define ACOMP 3



/* Vertex buffer size.  KW: no restrictions on the divisibility of
 * this number, though things may go better for you if you choose a
 * value of 12n + 3.  
 */
#define VB_START  3

#define VB_MAX (216 + VB_START)



/*
 * Actual vertex buffer size.
 *
 * Arrays must also accomodate new vertices from clipping, and
 * potential overflow from primitives which don't fit into neatly into
 * VB_MAX vertices.  (This only happens when mixed primitives are
 * sharing the vb).  
 */
#define VB_MAX_CLIPPED_VERTS ((2 * (6 + MAX_CLIP_PLANES))+1)
#define VB_SIZE  (VB_MAX + VB_MAX_CLIPPED_VERTS)


typedef struct __GLcontextRec GLcontext;

extern void
gl_read_config_file(GLcontext *ctx);

extern void
gl_register_config_var(const char *name, void (*notify)( const char *, int ));


#endif
