/* $Id: clip.h,v 1.2 1999/09/18 20:41:22 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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





#ifndef CLIP_H
#define CLIP_H


#include "types.h"



#ifdef DEBUG
#  define GL_VIEWCLIP_POINT( V )   gl_viewclip_point( V )
#else
#  define GL_VIEWCLIP_POINT( V )			\
     (    (V)[0] <= (V)[3] && (V)[0] >= -(V)[3]		\
       && (V)[1] <= (V)[3] && (V)[1] >= -(V)[3]		\
       && (V)[2] <= (V)[3] && (V)[2] >= -(V)[3] )
#endif




#define CLIP_TAB_EDGEFLAG 1

extern void gl_init_clip(void);


extern GLuint gl_viewclip_point( const GLfloat v[] );



extern GLuint gl_userclip_point( GLcontext* ctx, const GLfloat v[] );


extern void gl_user_cliptest( struct vertex_buffer *VB );


extern void gl_ClipPlane( GLcontext* ctx,
                          GLenum plane, const GLfloat *equation );

extern void gl_GetClipPlane( GLcontext* ctx,
                             GLenum plane, GLdouble *equation );


/*
 * Clipping interpolation functions
 */

extern void gl_clip_interp_all( struct vertex_buffer *VB, 
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_color_tex( struct vertex_buffer *VB,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_tex( struct vertex_buffer *VB, 
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_color( struct vertex_buffer *VB,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_none( struct vertex_buffer *VB,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );


extern void gl_update_userclip( GLcontext *ctx );

extern void gl_update_clipmask( GLcontext *ctx );

#endif
