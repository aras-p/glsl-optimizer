/* $Id: texobj.h,v 1.3 2000/05/23 17:14:49 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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



/*
 * Internal functions
 */

extern struct gl_texture_object *
gl_alloc_texture_object( struct gl_shared_state *shared, GLuint name,
                         GLuint dimensions );


extern void gl_free_texture_object( struct gl_shared_state *shared,
                                    struct gl_texture_object *t );


extern void
_mesa_test_texobj_completeness( const GLcontext *ctx,
                                struct gl_texture_object *t );


/*
 * API functions
 */

extern void
_mesa_GenTextures( GLsizei n, GLuint *textures );


extern void
_mesa_DeleteTextures( GLsizei n, const GLuint *textures );


extern void
_mesa_BindTexture( GLenum target, GLuint texture );


extern void
_mesa_PrioritizeTextures( GLsizei n, const GLuint *textures,
                          const GLclampf *priorities );


extern GLboolean
_mesa_AreTexturesResident( GLsizei n, const GLuint *textures,
                           GLboolean *residences );


extern GLboolean
_mesa_IsTexture( GLuint texture );


#endif
