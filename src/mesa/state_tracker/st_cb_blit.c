/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/texformat.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "st_context.h"
#include "st_program.h"
#include "st_cb_drawpixels.h"
#include "st_cb_blit.h"
#include "st_cb_fbo.h"

#include "util/u_blit.h"

#include "cso_cache/cso_context.h"


void
st_init_blit(struct st_context *st)
{
   st->blit = util_create_blit(st->pipe);
}


void
st_destroy_blit(struct st_context *st)
{
   util_destroy_blit(st->blit);
   st->blit = NULL;
}


static void
st_BlitFramebuffer(GLcontext *ctx,
                   GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                   GLbitfield mask, GLenum filter)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;

   const uint pFilter = ((filter == GL_NEAREST)
                         ? PIPE_TEX_MIPFILTER_NEAREST
                         : PIPE_TEX_MIPFILTER_LINEAR);

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct st_renderbuffer *srcRb = 
         st_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
      struct st_renderbuffer *dstRb = 
         st_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0][0]);
      struct pipe_surface *srcSurf = srcRb->surface;
      struct pipe_surface *dstSurf = dstRb->surface;

      srcY0 = srcRb->Base.Height - srcY0;
      srcY1 = srcRb->Base.Height - srcY1;

      dstY0 = dstRb->Base.Height - dstY0;
      dstY1 = dstRb->Base.Height - dstY1;

      util_blit_pixels(st->blit,
                       srcSurf, srcX0, srcY0, srcX1, srcY1,
                       dstSurf, dstX0, dstY0, dstX1, dstY1,
                       0.0, pFilter);

   }

#if 0
   /* XXX is this sufficient? */
   st_invalidate_state(ctx, _NEW_COLOR | _NEW_TEXTURE);
#else
   /* need to "unset" cso state because we went behind the back of the cso
    * tracker.  Without unset, the _set_ calls would be no-ops.
    */
   cso_unset_blend(st->cso_context);
   cso_unset_depth_stencil_alpha(st->cso_context);
   cso_unset_rasterizer(st->cso_context);
   cso_set_blend(st->cso_context, &st->state.blend);
   cso_set_depth_stencil_alpha(st->cso_context, &st->state.depth_stencil);
   cso_set_rasterizer(st->cso_context, &st->state.rasterizer);
   pipe->bind_fs_state(pipe, st->fp->driver_shader);
   pipe->bind_vs_state(pipe, st->vp->driver_shader);
#endif
}



void
st_init_blit_functions(struct dd_function_table *functions)
{
   functions->BlitFramebuffer = st_BlitFramebuffer;
}
