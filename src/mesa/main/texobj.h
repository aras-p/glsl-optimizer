/* $Id: texobj.h,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

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





#ifndef TEXTOBJ_H
#define TEXTOBJ_H


#include "types.h"


#ifdef VMS
#define gl_test_texture_object_completeness gl_test_texture_object_complete
#endif

/*
 * Internal functions
 */

extern struct gl_texture_object *
gl_alloc_texture_object( struct gl_shared_state *shared, GLuint name,
                         GLuint dimensions );


extern void gl_free_texture_object( struct gl_shared_state *shared,
                                    struct gl_texture_object *t );


extern void gl_test_texture_object_completeness( const GLcontext *ctx, struct gl_texture_object *t );


/*
 * API functions
 */

extern void gl_GenTextures( GLcontext *ctx, GLsizei n, GLuint *textures );


extern void gl_DeleteTextures( GLcontext *ctx,
                               GLsizei n, const GLuint *textures);


extern void gl_BindTexture( GLcontext *ctx, GLenum target, GLuint texture );


extern void gl_PrioritizeTextures( GLcontext *ctx,
                                   GLsizei n, const GLuint *textures,
                                   const GLclampf *priorities );


extern GLboolean gl_AreTexturesResident( GLcontext *ctx, GLsizei n,
                                         const GLuint *textures,
                                         GLboolean *residences );


extern GLboolean gl_IsTexture( GLcontext *ctx, GLuint texture );


#endif
