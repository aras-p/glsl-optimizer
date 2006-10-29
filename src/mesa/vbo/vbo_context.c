/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "mtypes.h"
#include "vbo_context.h"
#include "imports.h"
#include "api_arrayelt.h"

/* Reach out and grab this to use as the default:
 */
extern void _tnl_draw_prims( GLcontext *ctx,
			     const struct gl_client_array *arrays[],
			     const struct _mesa_prim *prims,
			     GLuint nr_prims,
			     const struct _mesa_index_buffer *ib,
			     GLuint min_index,
			     GLuint max_index );

GLboolean _vbo_CreateContext( GLcontext *ctx )
{
   struct vbo_context *vbo = CALLOC_STRUCT(vbo_context);

   ctx->swtnl_im = (void *)vbo;

   /* Initialize the arrayelt helper
    */
   if (!ctx->aelt_context &&
       !_ae_create_context( ctx )) {
      return GL_FALSE;
   }

   /* Hook our functions into exec and compile dispatch tables.  These
    * will pretty much be permanently installed, which means that the
    * vtxfmt mechanism can be removed now.
    */
   vbo_exec_init( ctx );
   vbo_save_init( ctx );

   /* By default: 
    */
   vbo->draw_prims = _tnl_draw_prims;
   
   return GL_TRUE;
}

void vbo_save_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   _ae_invalidate_state(ctx, new_state);
}


void _vbo_DestroyContext( GLcontext *ctx )
{
   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   FREE(vbo_context(ctx));
   ctx->swtnl_im = NULL;

}
