/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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

#include "main/glheader.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/mtypes.h"
#include "main/teximage.h"
#include "main/texenv.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/texparam.h"
#include "main/varray.h"
#include "main/attrib.h"
#include "main/enable.h"
#include "main/buffers.h"
#include "main/fbobject.h"
#include "main/depth.h"
#include "main/hash.h"
#include "main/blend.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "intel_context.h"
#include "intel_pixel.h"
#include "intel_fbo.h"


/** XXX compare perf of this vs. _mesa_meta_DrawPixels(STENCIL) */
static GLboolean
intel_stencil_drawpixels(GLcontext * ctx,
			 GLint x, GLint y,
			 GLsizei width, GLsizei height,
			 GLenum format,
			 GLenum type,
			 const struct gl_pixelstore_attrib *unpack,
			 const GLvoid *pixels)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint texname, rb_name, fb_name, old_fb_name;
   GLfloat vertices[4][2];
   struct intel_renderbuffer *irb;
   struct intel_renderbuffer *depth_irb;
   struct gl_pixelstore_attrib old_unpack;
   GLstencil *stencil_pixels;
   int row, y1, y2;
   GLint old_active_texture;
   GLboolean rendering_to_fbo = ctx->DrawBuffer->Name != 0;

   if (format != GL_STENCIL_INDEX)
      return GL_FALSE;

   /* If there's nothing to write, we're done. */
   if (ctx->Stencil.WriteMask[0] == 0)
      return GL_TRUE;

   /* Can't do a per-bit writemask while treating stencil as rgba data. */
   if ((ctx->Stencil.WriteMask[0] & 0xff) != 0xff) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glDrawPixels(STENCIL_INDEX) fallback: "
		 "stencil mask enabled\n");
      return GL_FALSE;
   }

   /* We don't support stencil testing/ops here */
   if (ctx->Stencil._Enabled)
      return GL_FALSE;

   /* We use FBOs for our wrapping of the depthbuffer into a color
    * destination.
    */
   if (!ctx->Extensions.EXT_framebuffer_object)
      return GL_FALSE;

   /* We're going to mess with texturing with no regard to existing texture
    * state, so if there is some set up we have to bail.
    */
   if (ctx->Texture._EnabledUnits != 0) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glDrawPixels(STENCIL_INDEX) fallback: "
		 "texturing enabled\n");
      return GL_FALSE;
   }

   /* Can't do textured DrawPixels with a fragment program, unless we were
    * to generate a new program that sampled our texture and put the results
    * in the fragment color before the user's program started.
    */
   if (ctx->FragmentProgram.Enabled) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glDrawPixels(STENCIL_INDEX) fallback: "
		 "fragment program enabled\n");
      return GL_FALSE;
   }

   /* Check that we can load in a texture this big. */
   if (width > (1 << (ctx->Const.MaxTextureLevels - 1)) ||
       height > (1 << (ctx->Const.MaxTextureLevels - 1))) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glDrawPixels(STENCIL_INDEX) fallback: "
		 "bitmap too large (%dx%d)\n",
		 width, height);
      return GL_FALSE;
   }

   if (!ctx->Extensions.ARB_texture_non_power_of_two &&
       (!is_power_of_two(width) || !is_power_of_two(height))) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr,
		 "glDrawPixels(GL_STENCIL_INDEX) fallback: NPOT texture\n");
      return GL_FALSE;
   }

   _mesa_PushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT |
		    GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   _mesa_PushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   old_fb_name = ctx->DrawBuffer->Name;
   old_active_texture = ctx->Texture.CurrentUnit;

   _mesa_Disable(GL_POLYGON_STIPPLE);
   _mesa_Disable(GL_DEPTH_TEST);
   _mesa_Disable(GL_STENCIL_TEST);

   /* Unpack the supplied stencil values into a ubyte buffer. */
   assert(sizeof(GLstencil) == sizeof(GLubyte));
   stencil_pixels = malloc(width * height * sizeof(GLstencil));
   for (row = 0; row < height; row++) {
      GLvoid *source = _mesa_image_address2d(unpack, pixels,
					     width, height,
					     GL_COLOR_INDEX, type,
					     row, 0);
      _mesa_unpack_stencil_span(ctx, width, GL_UNSIGNED_BYTE,
				stencil_pixels +
				row * width * sizeof(GLstencil),
				type, source, unpack, ctx->_ImageTransferState);
   }

   /* Take the current depth/stencil renderbuffer, and make a new one wrapping
    * it which will be treated as GL_RGBA8 so we can render to it as a color
    * buffer.
    */
   depth_irb = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);
   irb = intel_create_renderbuffer(MESA_FORMAT_ARGB8888);
   irb->Base.Width = depth_irb->Base.Width;
   irb->Base.Height = depth_irb->Base.Height;
   intel_renderbuffer_set_region(intel, irb, depth_irb->region);

   /* Create a name for our renderbuffer, which lets us use other mesa
    * rb functions for convenience.
    */
   _mesa_GenRenderbuffersEXT(1, &rb_name);
   irb->Base.RefCount++;
   _mesa_HashInsert(ctx->Shared->RenderBuffers, rb_name, &irb->Base);

   /* Bind the new renderbuffer to the color attachment point. */
   _mesa_GenFramebuffersEXT(1, &fb_name);
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_name);
   _mesa_FramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
				    GL_COLOR_ATTACHMENT0_EXT,
				    GL_RENDERBUFFER_EXT,
				    rb_name);
   /* Choose to render to the color attachment. */
   _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

   _mesa_DepthMask(GL_FALSE);
   _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB);
   _mesa_Enable(GL_TEXTURE_2D);
   _mesa_GenTextures(1, &texname);
   _mesa_BindTexture(GL_TEXTURE_2D, texname);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   _mesa_TexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   old_unpack = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   _mesa_TexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, width, height, 0,
		    GL_RED, GL_UNSIGNED_BYTE, stencil_pixels);
   ctx->Unpack = old_unpack;
   free(stencil_pixels);

   meta_set_passthrough_transform(&intel->meta);

   /* Since we're rendering to the framebuffer as if it was an FBO,
    * if it's the window system we have to flip the coordinates.
    */
   if (rendering_to_fbo) {
      y1 = y;
      y2 = y + height * ctx->Pixel.ZoomY;
   } else {
      y1 = irb->Base.Height - (y + height * ctx->Pixel.ZoomY);
      y2 = irb->Base.Height - y;
   }
   vertices[0][0] = x;
   vertices[0][1] = y1;
   vertices[1][0] = x + width * ctx->Pixel.ZoomX;
   vertices[1][1] = y1;
   vertices[2][0] = x + width * ctx->Pixel.ZoomX;
   vertices[2][1] = y2;
   vertices[3][0] = x;
   vertices[3][1] = y2;

   _mesa_VertexPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), &vertices);
   _mesa_Enable(GL_VERTEX_ARRAY);
   meta_set_default_texrect(&intel->meta);

   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   meta_restore_texcoords(&intel->meta);
   meta_restore_transform(&intel->meta);

   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB + old_active_texture);
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fb_name);

   _mesa_PopClientAttrib();
   _mesa_PopAttrib();

   _mesa_DeleteTextures(1, &texname);
   _mesa_DeleteFramebuffersEXT(1, &fb_name);
   _mesa_DeleteRenderbuffersEXT(1, &rb_name);

   return GL_TRUE;
}

void
intelDrawPixels(GLcontext * ctx,
                GLint x, GLint y,
                GLsizei width, GLsizei height,
                GLenum format,
                GLenum type,
                const struct gl_pixelstore_attrib *unpack,
                const GLvoid * pixels)
{
#if 0
   /* XXX this function doesn't seem to work reliably even when all
    * the pre-requisite conditions are met.
    * Note that this function is never hit with conform.
    * Fall back to swrast because even the _mesa_meta_DrawPixels() approach
    * isn't working because of an apparent stencil bug.
    */
   if (intel_stencil_drawpixels(ctx, x, y, width, height, format, type,
				unpack, pixels))
      return;
#else
   (void) intel_stencil_drawpixels; /* silence warning */
   if (format == GL_STENCIL_INDEX) {
      _swrast_DrawPixels(ctx, x, y, width, height, format, type,
                         unpack, pixels);
      return;
   }
#endif

   _mesa_meta_DrawPixels(ctx, x, y, width, height, format, type,
                         unpack, pixels);
}
