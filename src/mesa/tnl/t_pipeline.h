/* $Id: t_pipeline.h,v 1.2 2000/11/22 07:32:18 joukj Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 *
 *
 * Author:
 *    Keith Whitwell <keithw@valinux.com>
 */



#ifndef _T_PIPELINE_H_
#define _T_PIPELINE_H_

#include "mtypes.h"
#include "t_context.h"

extern void gl_update_materials( struct vertex_buffer *VB);

extern void _tnl_pipeline_init( GLcontext *ctx );
extern void gl_update_pipelines( GLcontext *ctx );

extern void gl_build_precalc_pipeline( GLcontext *ctx );
extern void gl_build_immediate_pipeline( GLcontext *ctx );

extern void gl_print_vert_flags( const char *name, GLuint flags );
extern void gl_print_pipeline( GLcontext *ctx, struct gl_pipeline *p );
extern void gl_print_active_pipeline( GLcontext *ctx, struct gl_pipeline *p );

extern void gl_run_pipeline( struct vertex_buffer *VB );

extern void gl_clean_color( struct vertex_buffer *VB );

extern void gl_reset_cva_vb( struct vertex_buffer *VB, GLuint stages );

extern void gl_print_pipe_ops( const char *msg, GLuint flags );

#endif
