/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/mtypes.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "main/fbobject.h"

#include "drivers/common/meta.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_fbo.h"
#include "intel_tex.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE


static bool
intel_copy_texsubimage(struct intel_context *intel,
                       struct intel_texture_image *intelImage,
                       GLint dstx, GLint dsty, GLint slice,
                       struct intel_renderbuffer *irb,
                       GLint x, GLint y, GLsizei width, GLsizei height)
{
   const GLenum internalFormat = intelImage->base.Base.InternalFormat;

   intel_prepare_render(intel);

   if (!intelImage->mt || !irb || !irb->mt) {
      if (unlikely(INTEL_DEBUG & DEBUG_PERF))
	 fprintf(stderr, "%s fail %p %p (0x%08x)\n",
		 __FUNCTION__, intelImage->mt, irb, internalFormat);
      return false;
   }

   /* blit from src buffer to texture */
   if (!intel_miptree_blit(intel,
                           irb->mt, irb->mt_level, irb->mt_layer,
                           x, y, irb->Base.Base.Name == 0,
                           intelImage->mt, intelImage->base.Base.Level,
                           intelImage->base.Base.Face + slice,
                           dstx, dsty, false,
                           width, height, GL_COPY)) {
      return false;
   }

   return true;
}


static void
intelCopyTexSubImage(struct gl_context *ctx, GLuint dims,
                     struct gl_texture_image *texImage,
                     GLint xoffset, GLint yoffset, GLint slice,
                     struct gl_renderbuffer *rb,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height)
{
   struct intel_context *intel = intel_context(ctx);

   /* Try the BLT engine. */
   if (intel_copy_texsubimage(intel,
                              intel_texture_image(texImage),
                              xoffset, yoffset, slice,
                              intel_renderbuffer(rb), x, y, width, height)) {
      return;
   }

   /* Otherwise, fall back to meta.  This will likely be slow. */
   perf_debug("%s - fallback to swrast\n", __FUNCTION__);
   _mesa_meta_CopyTexSubImage(ctx, dims, texImage,
                              xoffset, yoffset, slice,
                              rb, x, y, width, height);
}


void
intelInitTextureCopyImageFuncs(struct dd_function_table *functions)
{
   functions->CopyTexSubImage = intelCopyTexSubImage;
}
