/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "glheader.h"
#include "enums.h"
#include "image.h"
#include "mtypes.h"
#include "macros.h"
#include "state.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"


static struct intel_region *
copypix_src_region(struct intel_context *intel, GLenum type)
{
   switch (type) {
   case GL_COLOR:
      return intel_readbuf_region(intel);
   case GL_DEPTH:
      /* Don't think this is really possible execpt at 16bpp, when we have no stencil.
       */
      if (intel->depth_region && intel->depth_region->cpp == 2)
         return intel->depth_region;
   case GL_STENCIL:
      /* Don't think this is really possible. 
       */
      break;
   case GL_DEPTH_STENCIL_EXT:
      /* Does it matter whether it is stencil/depth or depth/stencil?
       */
      return intel->depth_region;
   default:
      break;
   }

   return NULL;
}




/**
 * Check if any fragment operations are in effect which might effect
 * glDraw/CopyPixels.
 */
GLboolean
intel_check_blit_fragment_ops(GLcontext * ctx)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Could do logicop with the blitter: 
    */
   return !(ctx->_ImageTransferState ||
	    ctx->RenderMode != GL_RENDER ||
            ctx->Color.AlphaEnabled ||
            ctx->Depth.Test ||
            ctx->Fog.Enabled ||
            ctx->Stencil.Enabled ||
            !ctx->Color.ColorMask[0] ||
            !ctx->Color.ColorMask[1] ||
            !ctx->Color.ColorMask[2] ||
            !ctx->Color.ColorMask[3] ||	/* can do this! */
            ctx->Texture._EnabledUnits ||
	    ctx->FragmentProgram._Enabled ||
	    ctx->Color.BlendEnabled);
}



/**
 * CopyPixels with the blitter.  Don't support zooming, pixel transfer, etc.
 */
static GLboolean
do_blit_copypixels(GLcontext * ctx,
                   GLint srcx, GLint srcy,
                   GLsizei width, GLsizei height,
                   GLint dstx, GLint dsty, GLenum type)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst = intel_drawbuf_region(intel);
   struct intel_region *src = copypix_src_region(intel, type);

   /* Copypixels can be more than a straight copy.  Ensure all the
    * extra operations are disabled:
    */
   if (!intel_check_blit_fragment_ops(ctx) ||
       ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F)
      return GL_FALSE;

   if (!src || !dst)
      return GL_FALSE;



   intelFlush(&intel->ctx);

/*    intel->vtbl.render_start(intel); */
/*    intel->vtbl.emit_state(intel); */

   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t dest_rect;
      GLint nbox = dPriv->numClipRects;
      GLint delta_x = 0;
      GLint delta_y = 0;
      GLuint i;

      /* Do scissoring in GL coordinates:
       */
      if (ctx->Scissor.Enabled)
      {
	 GLint x = ctx->Scissor.X;
	 GLint y = ctx->Scissor.Y;
	 GLuint w = ctx->Scissor.Width;
	 GLuint h = ctx->Scissor.Height;
	 GLint dx = dstx - srcx;
         GLint dy = dsty - srcy;

         if (!_mesa_clip_to_region(x, y, x+w-1, y+h-1, &dstx, &dsty, &width, &height))
            goto out;
	 
         srcx = dstx - dx;
         srcy = dsty - dy;
      }

      /* Convert from GL to hardware coordinates:
       */
      dsty = dPriv->h - dsty - height;  
      srcy = dPriv->h - srcy - height;  
      dstx += dPriv->x;
      dsty += dPriv->y;
      srcx += dPriv->x;
      srcy += dPriv->y;

      /* Clip against the source region.  This is the only source
       * clipping we do.  Dst is clipped with cliprects below.
       */
      {
         delta_x = srcx - dstx;
         delta_y = srcy - dsty;

         if (!_mesa_clip_to_region(0, 0, src->pitch, src->height,
                                   &srcx, &srcy, &width, &height))
            goto out;

         dstx = srcx - delta_x;
         dsty = srcy - delta_y;
      }

      dest_rect.x1 = dstx;
      dest_rect.y1 = dsty;
      dest_rect.x2 = dstx + width;
      dest_rect.y2 = dsty + height;

/*       intel->vtbl.emit_flush(intel, 0); */

      /* Could do slightly more clipping: Eg, take the intersection of
       * the existing set of cliprects and those cliprects translated
       * by delta_x, delta_y:
       * 
       * This code will not overwrite other windows, but will
       * introduce garbage when copying from obscured window regions.
       */
      for (i = 0; i < nbox; i++) {
         drm_clip_rect_t rect;

         if (!intel_intersect_cliprects(&rect, &dest_rect, &box[i]))
            continue;


         intelEmitCopyBlit(intel, 
			   dst->cpp, 
			   src->pitch, src->buffer, 0, src->tiled,
			   dst->pitch, dst->buffer, 0, dst->tiled,
			   rect.x1 + delta_x, 
			   rect.y1 + delta_y,       /* srcx, srcy */
                           rect.x1, rect.y1,    /* dstx, dsty */
                           rect.x2 - rect.x1, rect.y2 - rect.y1,
			   ctx->Color.ColorLogicOpEnabled ?
			   ctx->Color.LogicOp : GL_COPY);
      }

      intel->need_flush = GL_TRUE;
   out:
      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);
   return GL_TRUE;
}

void
intelCopyPixels(GLcontext * ctx,
                GLint srcx, GLint srcy,
                GLsizei width, GLsizei height,
                GLint destx, GLint desty, GLenum type)
{
   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (do_blit_copypixels(ctx, srcx, srcy, width, height, destx, desty, type))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("fallback to _swrast_CopyPixels\n");

   _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type);
}
