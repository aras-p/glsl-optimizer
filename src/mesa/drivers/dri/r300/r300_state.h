/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_STATE_H__
#define __R300_STATE_H__

#include "r300_context.h"

#define R300_NEWPRIM( rmesa )			\
  do {						\
    if ( rmesa->dma.flush )			\
      rmesa->dma.flush( rmesa );		\
  } while (0)

#define R300_STATECHANGE(r300, atom) \
	do {						\
	  R300_NEWPRIM(r300);				\
		r300->hw.atom.dirty = GL_TRUE;		\
		r300->hw.is_dirty = GL_TRUE;		\
	} while(0)

#define R300_PRINT_STATE(r300, atom) \
		r300PrintStateAtom(r300, &r300->hw.atom)

/* Fire the buffered vertices no matter what.
   TODO: This has not been implemented yet
 */
#define R300_FIREVERTICES( r300 )			\
do {							\
    \
   if ( (r300)->cmdbuf.count_used || (r300)->dma.flush ) {	\
      r300Flush( (r300)->radeon.glCtx );		\
   }							\
    \
} while (0)

// r300_state.c
extern int future_hw_tcl_on;
void _tnl_UpdateFixedFunctionProgram (GLcontext * ctx);
void r300UpdateViewportOffset (GLcontext * ctx);
void r300UpdateDrawBuffer (GLcontext * ctx);
void r300UpdateStateParameters (GLcontext * ctx, GLuint new_state);
void r300UpdateShaders (r300ContextPtr rmesa);
void r300UpdateShaderStates (r300ContextPtr rmesa);
void r300InitState (r300ContextPtr r300);
void r300UpdateClipPlanes (GLcontext * ctx);
void r300InitStateFuncs (struct dd_function_table *functions);

#endif				/* __R300_STATE_H__ */
