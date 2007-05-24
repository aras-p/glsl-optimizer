/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef SP_STATE_H
#define SP_STATE_H

#include "mtypes.h"
#include "vf/vf.h"

#define WINDING_NONE 0
#define WINDING_CW   1
#define WINDING_CCW  2
#define WINDING_BOTH (WINDING_CW | WINDING_CCW)

#define FILL_POINT 1
#define FILL_LINE  2
#define FILL_TRI   3

struct softpipe_setup_state {
   GLuint flatshade:1;
   GLuint light_twoside:1;

   GLuint front_winding:2;

   GLuint cull_mode:2;

   GLuint fill_cw:2;
   GLuint fill_ccw:2;

   GLuint offset_cw:1;
   GLuint offset_ccw:1;

   GLuint scissor:1;
   GLuint poly_stipple:1;

   GLuint pad:18;

   GLfloat offset_units;
   GLfloat offset_scale;
};

struct softpipe_poly_stipple {
   GLuint stipple[32];
};


struct softpipe_viewport {
   GLfloat scale[4];
   GLfloat translate[4];
};

struct softpipe_scissor_rect {
   GLshort minx;
   GLshort miny;
   GLshort maxx;
   GLshort maxy;
};


struct softpipe_clip_state {
   GLfloat ucp[6][4];
   GLuint nr;
};

struct softpipe_fs_state {
   struct gl_fragment_program *fp;
};

#define SP_MAX_CONSTANT 32

struct softpipe_constant_buffer {
   GLfloat constant[SP_MAX_CONSTANT][4];
   GLuint nr_constants;
};


struct softpipe_blend_state {   
   GLuint blend_enable:1; 
   GLuint ia_blend_enable:1;

   GLuint ia_dest_blend_factor:5; 
   GLuint ia_src_blend_factor:5; 
   GLuint ia_blend_function:3; 

   GLuint dest_blend_factor:5; 
   GLuint src_blend_factor:5; 
   GLuint blend_function:3; 

   GLuint logicop_enable:1; 
   GLuint logicop_func:4; 
};

struct softpipe_blend_color {
   GLfloat color[4];  
};


/* This will change for hardware softpipes...
 */
struct softpipe_surface {
   GLubyte *ptr;
   GLint stride;
   GLuint cpp;
   GLuint format;
};


#endif
