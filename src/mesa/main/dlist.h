/* $Id: dlist.h,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

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





#ifndef DLIST_H
#define DLIST_H


#include "types.h"

struct display_list {
   union node *nodes;	
   GLuint OrFlag;
   struct gl_current_attrib outputs;
};

struct display_list_compilation {
   struct display_list *list;
   union node *current_block;
   GLuint current_pos;
};


extern void gl_init_lists( void );

extern void gl_destroy_list( GLcontext *ctx, GLuint list );

extern void gl_CallList( GLcontext *ctx, GLuint list );

extern void gl_CallLists( GLcontext *ctx,
                          GLsizei n, GLenum type, const GLvoid *lists );

extern void gl_DeleteLists( GLcontext *ctx, GLuint list, GLsizei range );

extern void gl_EndList( GLcontext *ctx );

extern GLuint gl_GenLists( GLcontext *ctx, GLsizei range );

extern GLboolean gl_IsList( GLcontext *ctx, GLuint list );

extern void gl_ListBase( GLcontext *ctx, GLuint base );

extern void gl_NewList( GLcontext *ctx, GLuint list, GLenum mode );

extern void gl_init_dlist_pointers( struct gl_api_table *table );


extern void gl_compile_cassette( GLcontext *ctx );

extern void gl_save_error( GLcontext *ctx, GLenum error, const char *s );


#endif
