/* $Id: light.h,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

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





#ifndef LIGHT_H
#define LIGHT_H


#include "types.h"

struct gl_shine_tab {
   struct gl_shine_tab *next, *prev;
   GLfloat tab[SHINE_TABLE_SIZE+1];
   GLfloat shininess;
   GLuint refcount;
};


extern void gl_ShadeModel( GLcontext *ctx, GLenum mode );

extern void gl_ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode );

extern void gl_Lightfv( GLcontext *ctx,
                        GLenum light, GLenum pname, const GLfloat *params,
                        GLint nparams );

extern void gl_LightModelfv( GLcontext *ctx,
                             GLenum pname, const GLfloat *params );


extern GLuint gl_material_bitmask( GLcontext *ctx, 
				   GLenum face, GLenum pname, 
				   GLuint legal,
				   const char * );

extern void gl_set_material( GLcontext *ctx, GLuint bitmask,
                             const GLfloat *params);

extern void gl_Materialfv( GLcontext *ctx,
                           GLenum face, GLenum pname, const GLfloat *params );



extern void gl_GetLightfv( GLcontext *ctx,
                           GLenum light, GLenum pname, GLfloat *params );

extern void gl_GetLightiv( GLcontext *ctx,
                           GLenum light, GLenum pname, GLint *params );


extern void gl_GetMaterialfv( GLcontext *ctx,
                              GLenum face, GLenum pname, GLfloat *params );

extern void gl_GetMaterialiv( GLcontext *ctx,
                              GLenum face, GLenum pname, GLint *params );


extern void gl_compute_spot_exp_table( struct gl_light *l );

extern void gl_compute_shine_table( GLcontext *ctx, GLuint i, 
				    GLfloat shininess );

extern void gl_update_lighting( GLcontext *ctx );

extern void gl_compute_light_positions( GLcontext *ctx );

extern void gl_update_normal_transform( GLcontext *ctx );

extern void gl_update_material( GLcontext *ctx, 
				struct gl_material *m, 
				GLuint bitmask );

extern void gl_update_color_material( GLcontext *ctx, const GLubyte rgba[4] );


#endif

