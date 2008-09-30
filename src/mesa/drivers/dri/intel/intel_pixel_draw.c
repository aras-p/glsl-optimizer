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
#include "main/macros.h"
#include "main/bufferobj.h"
#include "main/teximage.h"
#include "main/texenv.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/texparam.h"
#include "main/matrix.h"
#include "main/varray.h"
#include "main/attrib.h"
#include "main/enable.h"
#include "glapi/dispatch.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_regions.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"
#include "intel_tris.h"


static GLboolean
intel_texture_drawpixels(GLcontext * ctx,
			 GLint x, GLint y,
			 GLsizei width, GLsizei height,
			 GLenum format,
			 GLenum type,
			 const struct gl_pixelstore_attrib *unpack,
			 const GLvoid *pixels)
{
   GLuint texname;
   GLfloat vertices[4][4];
   GLfloat texcoords[4][2];

   /* We're going to mess with texturing with no regard to existing texture
    * state, so if there is some set up we have to bail.
    */
   if (ctx->Texture._EnabledUnits != 0)
      return GL_FALSE;

   /* Can't do textured DrawPixels with a fragment program, unless we were
    * to generate a new program that sampled our texture and put the results
    * in the fragment color before the user's program started.
    */
   if (ctx->FragmentProgram.Enabled)
      return GL_FALSE;

   /* Don't even want to think about it */
   if (format == GL_COLOR_INDEX)
      return GL_FALSE;

   /* We don't have a way to generate fragments with stencil values which *
    * will set the resulting stencil value.
    */
   if (format == GL_STENCIL_INDEX)
      return GL_FALSE;

   /* To do DEPTH_COMPONENT, we would need to change our setup to not draw to
    * the color buffer, and sample the texture values into the fragment depth
    * in a program.
    */
   if (format == GL_DEPTH_COMPONENT)
      return GL_FALSE;

   _mesa_PushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_TEXTURE_BIT |
		    GL_CURRENT_BIT);
   _mesa_PushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   /* XXX: pixel store stuff */
   _mesa_Disable(GL_POLYGON_STIPPLE);

   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB);
   _mesa_Enable(GL_TEXTURE_2D);
   _mesa_GenTextures(1, &texname);
   _mesa_BindTexture(GL_TEXTURE_2D, texname);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   _mesa_TexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   /*
   _mesa_TexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
   _mesa_TexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
   */
   _mesa_TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format,
		    type, pixels);

   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
   _mesa_Ortho(0, ctx->DrawBuffer->Width, 0, ctx->DrawBuffer->Height, 1, -1);

   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();

   vertices[0][0] = x;
   vertices[0][1] = y;
   vertices[0][2] = ctx->Current.RasterPos[2];
   vertices[0][3] = ctx->Current.RasterPos[3];
   vertices[1][0] = x + width * ctx->Pixel.ZoomX;
   vertices[1][1] = y;
   vertices[1][2] = ctx->Current.RasterPos[2];
   vertices[1][3] = ctx->Current.RasterPos[3];
   vertices[2][0] = x + width * ctx->Pixel.ZoomX;
   vertices[2][1] = y + height * ctx->Pixel.ZoomY;
   vertices[2][2] = ctx->Current.RasterPos[2];
   vertices[2][3] = ctx->Current.RasterPos[3];
   vertices[3][0] = x;
   vertices[3][1] = y + height * ctx->Pixel.ZoomY;
   vertices[3][2] = ctx->Current.RasterPos[2];
   vertices[3][3] = ctx->Current.RasterPos[3];

   texcoords[0][0] = 0.0;
   texcoords[0][1] = 0.0;
   texcoords[1][0] = 1.0;
   texcoords[1][1] = 0.0;
   texcoords[2][0] = 1.0;
   texcoords[2][1] = 1.0;
   texcoords[3][0] = 0.0;
   texcoords[3][1] = 1.0;

   _mesa_VertexPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), &vertices);
   _mesa_TexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), &texcoords);
   _mesa_Enable(GL_VERTEX_ARRAY);
   _mesa_Enable(GL_TEXTURE_COORD_ARRAY);
   CALL_DrawArrays(ctx->Exec, (GL_TRIANGLE_FAN, 0, 4));

   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PopMatrix();
   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PopMatrix();
   _mesa_PopClientAttrib();
   _mesa_PopAttrib();

   _mesa_DeleteTextures(1, &texname);

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
   if (intel_texture_drawpixels(ctx, x, y, width, height, format, type,
				unpack, pixels))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: fallback to swrast\n", __FUNCTION__);

   if (ctx->FragmentProgram._Current == ctx->FragmentProgram._TexEnvProgram) {
      /*
       * We don't want the i915 texenv program to be applied to DrawPixels.
       * This is really just a performance optimization (mesa will other-
       * wise happily run the fragment program on each pixel in the image).
       */
      struct gl_fragment_program *fpSave = ctx->FragmentProgram._Current;
   /* can't just set current frag prog to 0 here as on buffer resize
      we'll get new state checks which will segfault. Remains a hack. */
      ctx->FragmentProgram._Current = NULL;
      ctx->FragmentProgram._UseTexEnvProgram = GL_FALSE;
      ctx->FragmentProgram._Active = GL_FALSE;
      _swrast_DrawPixels( ctx, x, y, width, height, format, type,
                          unpack, pixels );
      ctx->FragmentProgram._Current = fpSave;
      ctx->FragmentProgram._UseTexEnvProgram = GL_TRUE;
      ctx->FragmentProgram._Active = GL_TRUE;
      _swrast_InvalidateState(ctx, _NEW_PROGRAM);
   }
   else {
      _swrast_DrawPixels( ctx, x, y, width, height, format, type,
                          unpack, pixels );
   }
}
