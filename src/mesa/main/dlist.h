/* $Id: dlist.h,v 1.4 2000/05/24 15:04:45 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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

extern void _mesa_CallList( GLuint list );

extern void _mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists );

extern void _mesa_DeleteLists( GLuint list, GLsizei range );

extern void _mesa_EndList( void );

extern GLuint _mesa_GenLists( GLsizei range );

extern GLboolean _mesa_IsList( GLuint list );

extern void _mesa_ListBase( GLuint base );

extern void _mesa_NewList( GLuint list, GLenum mode );

extern void _mesa_init_dlist_table( struct _glapi_table *table, GLuint tableSize );

extern void gl_compile_cassette( GLcontext *ctx );

extern void gl_save_error( GLcontext *ctx, GLenum error, const char *s );


#endif
