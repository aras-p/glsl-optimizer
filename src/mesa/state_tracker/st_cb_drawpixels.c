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

#include "imports.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_cb_drawpixels.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "vf/vf.h"


static void
draw_quad(struct st_context *st, GLfloat x, GLfloat y, GLfloat z,
          GLsizei width, GLsizei height)
{
   static const GLuint attribs[2] = {
      VF_ATTRIB_POS,
      VF_ATTRIB_TEX0
   };
   GLfloat verts[4][2][4]; /* four verts, two attribs, XYZW */
   GLuint i;

   /* lower-left */
   verts[0][0][0] = x;
   verts[0][0][1] = y;
   verts[0][1][0] = 0.0;
   verts[0][1][1] = 0.0;

   /* lower-right */
   verts[1][0][0] = x + width;
   verts[1][0][1] = y;
   verts[1][1][0] = 1.0;
   verts[1][1][1] = 0.0;

   /* upper-right */
   verts[2][0][0] = x + width;
   verts[2][0][1] = y + height;
   verts[2][1][0] = 1.0;
   verts[2][1][1] = 1.0;

   /* upper-left */
   verts[3][0][0] = x;
   verts[3][0][1] = y + height;
   verts[3][1][0] = 0.0;
   verts[3][1][1] = 11.0;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      verts[i][0][2] = z;   /*Z*/
      verts[i][0][3] = 1.0; /*W*/
      verts[i][1][2] = 0.0; /*R*/
      verts[i][1][3] = 1.0; /*Q*/
   }

   st->pipe->draw_vertices(st->pipe, GL_QUADS,
                           4, (GLfloat *) verts, 2, attribs);
}


static void
draw_textured_quad(struct st_context *st, GLfloat x, GLfloat y, GLfloat z,
                   GLsizei width, GLsizei height, GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid *pixels)
{
   struct pipe_setup_state setup;

   /* setup state: just scissor */
   memset(&setup, 0, sizeof(setup));
   if (st->ctx->Scissor.Enabled)
      setup.scissor = 1;
   st->pipe->set_setup_state(st->pipe, &setup);


   /* XXX upload the texture image */

   draw_quad(st, x, y, z, width, height);

   /* restore GL state */
   st->pipe->set_setup_state(st->pipe, &st->state.setup);
}


/**
 * Check if a GL format/type combination is a match to the given pipe format.
 * XXX probably move this to a re-usable place.
 */
static GLboolean
compatible_formats(GLenum format, GLenum type, GLuint pipeFormat)
{
   static const GLuint one = 1;
   GLubyte littleEndian = *((GLubyte *) one);

   if (pipeFormat == PIPE_FORMAT_U_R8_G8_B8_A8 &&
       format == GL_RGBA &&
       type == GL_UNSIGNED_BYTE &&
       !littleEndian) {
      return GL_TRUE;
   }
   else if (pipeFormat == PIPE_FORMAT_U_R8_G8_B8_A8 &&
            format == GL_ABGR_EXT &&
            type == GL_UNSIGNED_BYTE &&
            littleEndian) {
      return GL_TRUE;
   }
   else if (pipeFormat == PIPE_FORMAT_U_A8_R8_G8_B8 &&
            format == GL_BGRA &&
            type == GL_UNSIGNED_BYTE &&
            littleEndian) {
      return GL_TRUE;
   }
   else if (pipeFormat == PIPE_FORMAT_U_R5_G6_B5 &&
            format == GL_RGB &&
            type == GL_UNSIGNED_SHORT_5_6_5) {
      /* endian don't care */
      return GL_TRUE;
   }
   else if (pipeFormat == PIPE_FORMAT_U_R5_G6_B5 &&
            format == GL_BGR &&
            type == GL_UNSIGNED_SHORT_5_6_5_REV) {
      /* endian don't care */
      return GL_TRUE;
   }
   else if (pipeFormat == PIPE_FORMAT_U_S8 &&
            format == GL_STENCIL_INDEX &&
            type == GL_UNSIGNED_BYTE) {
      return GL_TRUE;
   }
   else if (pipeFormat == PIPE_FORMAT_U_Z32 &&
            format == GL_DEPTH_COMPONENT &&
            type == GL_UNSIGNED_INT) {
      return GL_TRUE;
   }
   /* XXX add more cases */
   else {
      return GL_FALSE;
   }
}


/**
 * Check if any per-fragment ops are enabled.
 * XXX probably move this to a re-usable place.
 */
static GLboolean
any_fragment_ops(const struct st_context *st)
{
   if (st->state.alpha_test.enabled ||
       st->state.blend.blend_enable ||
       st->state.blend.logicop_enable ||
       st->state.depth.enabled)
      /* XXX more checks */
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 * Check if any pixel transfer ops are enabled.
 * XXX probably move this to a re-usable place.
 */
static GLboolean
any_pixel_transfer_ops(const struct st_context *st)
{
   if (st->ctx->Pixel.RedScale != 1.0 ||
       st->ctx->Pixel.RedBias != 0.0 ||
       st->ctx->Pixel.GreenScale != 1.0 ||
       st->ctx->Pixel.GreenBias != 0.0 ||
       st->ctx->Pixel.BlueScale != 1.0 ||
       st->ctx->Pixel.BlueBias != 0.0 ||
       st->ctx->Pixel.AlphaScale != 1.0 ||
       st->ctx->Pixel.AlphaBias != 0.0 ||
       st->ctx->Pixel.MapColorFlag)
      /* XXX more checks */
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 * Draw image with a blit, or other non-textured quad method.
 */
static void
draw_blit(struct st_context *st,
          GLsizei width, GLsizei height,
          GLenum format, GLenum type, const GLvoid *pixels)
{


}


/**
 * Called via ctx->Driver.DrawPixels()
 */
static void
st_drawpixels(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height,
              GLenum format, GLenum type,
              const struct gl_pixelstore_attrib *unpack, const GLvoid *pixels)
{
   struct st_context *st = ctx->st;
   struct pipe_surface *ps;
   GLuint bufferFormat;

   if (format == GL_DEPTH_COMPONENT) {
      ps = st->state.framebuffer.zbuf;
   }
   else if (format == GL_STENCIL_INDEX) {
      ps = st->state.framebuffer.sbuf;
   }
   else {
      ps = st->state.framebuffer.cbufs[0];
   }

   bufferFormat = ps->format;

   if (any_fragment_ops(st) ||
       any_pixel_transfer_ops(st) ||
       !compatible_formats(format, type, ps->format)) {
      /* textured quad */
      draw_textured_quad(st, x, y, ctx->Current.RasterPos[2], width, height,
                         format, type, unpack, pixels);
   }
   else {
      /* blit */
      draw_blit(st, width, height, format, type, pixels);
   }
}


void st_init_cb_drawpixels( struct st_context *st )
{
   struct dd_function_table *functions = &st->ctx->Driver;

   functions->DrawPixels = st_drawpixels;
}


void st_destroy_cb_drawpixels( struct st_context *st )
{
}
