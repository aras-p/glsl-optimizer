/* $Id: t_imm_fixup.h,v 1.1 2000/12/26 05:09:33 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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


#ifndef _T_IMM_FIXUP_H
#define _T_IMM_FIXUP_H

#include "mtypes.h"
#include "t_context.h"



extern void _tnl_fixup_input( GLcontext *ctx, struct immediate *IM );

extern void _tnl_fixup_compiled_cassette( GLcontext *ctx, 
					  struct immediate *IM );

extern void _tnl_restore_compiled_cassette( GLcontext *ctx, 
					    struct immediate *IM );


extern void _tnl_fixup_purged_eval( GLcontext *ctx,
				    GLuint fixup, GLuint purge );


extern void _tnl_copy_immediate_vertices( GLcontext *ctx, struct immediate *IM );
extern void _tnl_get_exec_copy_verts( GLcontext *ctx, struct immediate *IM );

#endif
