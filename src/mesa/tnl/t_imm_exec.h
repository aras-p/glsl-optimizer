/* $Id: t_imm_exec.h,v 1.6 2001/05/11 08:11:31 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef _T_VBXFORM_H
#define _T_VBXFORM_H

#include "mtypes.h"
#include "t_context.h"


/* Hook for ctx->Driver.FlushVertices:
 */
extern void _tnl_flush_vertices( GLcontext *ctx, GLuint flush_flags );

/* Called from imm_api.c and _tnl_flush_vertices:
 */
extern void _tnl_flush_immediate( struct immediate *IM );

/* Called from imm_dlist.c and _tnl_flush_immediate:
 */
extern void _tnl_run_cassette( GLcontext *ctx, struct immediate *IM );
extern void _tnl_copy_to_current( GLcontext *ctx, struct immediate *IM,
				  GLuint flag );

/* Initialize some stuff:
 */
extern void _tnl_imm_init( GLcontext *ctx );

extern void _tnl_imm_destroy( GLcontext *ctx );

extern void _tnl_reset_exec_input( GLcontext *ctx,
				   GLuint start,
				   GLuint beginstate,
				   GLuint savedbeginstate );

extern void _tnl_reset_compile_input( GLcontext *ctx,
				      GLuint start,
				      GLuint beginstate,
				      GLuint savedbeginstate );

extern void _tnl_compute_orflag( struct immediate *IM, GLuint start );
extern void _tnl_execute_cassette( GLcontext *ctx, struct immediate *IM );



#endif
