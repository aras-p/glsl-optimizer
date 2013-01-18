/**
 * \file dlist.h
 * Display lists management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#include "main/mfeatures.h"
#include "main/mtypes.h"


#define _MESA_INIT_DLIST_VTXFMT(vfmt, impl)  \
   do {                                      \
      (vfmt)->CallList  = impl ## CallList;  \
      (vfmt)->CallLists = impl ## CallLists; \
   } while (0)

GLboolean GLAPIENTRY
_mesa_IsList(GLuint list);
void GLAPIENTRY
_mesa_DeleteLists(GLuint list, GLsizei range);
GLuint GLAPIENTRY
_mesa_GenLists(GLsizei range);
void GLAPIENTRY
_mesa_NewList(GLuint name, GLenum mode);
void GLAPIENTRY
_mesa_EndList(void);
void GLAPIENTRY
_mesa_CallList( GLuint list );
void GLAPIENTRY
_mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists );
void GLAPIENTRY
_mesa_ListBase(GLuint base);


extern void _mesa_compile_error( struct gl_context *ctx, GLenum error, const char *s );

extern void *_mesa_dlist_alloc(struct gl_context *ctx, GLuint opcode, GLuint sz);

extern GLint _mesa_dlist_alloc_opcode( struct gl_context *ctx, GLuint sz,
                                       void (*execute)( struct gl_context *, void * ),
                                       void (*destroy)( struct gl_context *, void * ),
                                       void (*print)( struct gl_context *, void * ) );

extern void _mesa_delete_list(struct gl_context *ctx, struct gl_display_list *dlist);

extern void _mesa_save_vtxfmt_init( GLvertexformat *vfmt );

extern void _mesa_initialize_save_table(const struct gl_context *);

extern void _mesa_install_dlist_vtxfmt(struct _glapi_table *disp,
                                       const GLvertexformat *vfmt);

extern void _mesa_init_display_list( struct gl_context * ctx );

extern void _mesa_free_display_list_data(struct gl_context *ctx);


#endif /* DLIST_H */
