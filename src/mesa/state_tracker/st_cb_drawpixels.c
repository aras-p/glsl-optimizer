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
#include "st_program.h"
#include "st_cb_drawpixels.h"
#include "st_cb_texture.h"
#include "st_format.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "shader/prog_instruction.h"
#include "vf/vf.h"



/**
 * Create a simple fragment shader that passes the texture color through
 * to the fragment color.
 */
static struct st_fragment_program *
make_drawpixels_shader(struct st_context *st)
{
   GLcontext *ctx = st->ctx;
   struct st_fragment_program *stfp;
   struct gl_program *p;
   GLboolean b;

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 2;
   p->Instructions = _mesa_alloc_instructions(2);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, 2);
   /* TEX result.color, fragment.texcoord[0], texture[0], 2D; */
   p->Instructions[0].Opcode = OPCODE_TEX;
   p->Instructions[0].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[0].DstReg.Index = FRAG_RESULT_COLR;
   p->Instructions[0].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[0].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   p->Instructions[0].TexSrcUnit = 0;
   p->Instructions[0].TexSrcTarget = TEXTURE_2D_INDEX;
   /* END; */
   p->Instructions[1].Opcode = OPCODE_END;

   p->InputsRead = FRAG_BIT_TEX0;
   p->OutputsWritten = (1 << FRAG_RESULT_COLR);

   stfp = (struct st_fragment_program *) p;
   /* compile into tgsi format */
   b = tgsi_mesa_compile_fp_program(&stfp->Base,
                                    stfp->tokens, ST_FP_MAX_TOKENS);
   assert(b);

   return stfp;
}


static struct pipe_mipmap_tree *
make_mipmap_tree(struct st_context *st,
                 GLsizei width, GLsizei height, GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *unpack,
                 const GLvoid *pixels)
{
   struct pipe_context *pipe = st->pipe;
   const struct gl_texture_format *mformat;
   const GLbitfield flags = PIPE_SURFACE_FLAG_TEXTURE;
   struct pipe_mipmap_tree *mt;
   GLuint pipeFormat, cpp;

   mformat = st_ChooseTextureFormat(st->ctx, GL_RGBA, format, type);
   assert(mformat);

   pipeFormat = st_mesa_format_to_pipe_format(mformat->MesaFormat);
   assert(pipeFormat);
   cpp = st_sizeof_format(pipeFormat);

   mt = CALLOC_STRUCT(pipe_mipmap_tree);

   if (unpack->BufferObj && unpack->BufferObj->Name) {
      /*
      mt->region = buffer_object_region(unpack->BufferObj);
      */
   }
   else {
      static const GLuint dstImageOffsets = 0;
      GLboolean success;
      GLubyte *dest;
      GLuint pitch;

      /* allocate texture region/storage */
      mt->region = st->pipe->region_alloc(st->pipe, cpp, width, height, flags);
      pitch = mt->region->pitch;

      /* map texture region */
      dest = pipe->region_map(pipe, mt->region);

      /* put image into texture region */
      success = mformat->StoreImage(st->ctx, 2,       /* dims */
                                    GL_RGBA,          /* baseInternalFormat */
                                    mformat,          /* gl_texture_format */
                                    dest,             /* dest */
                                    0, 0, 0,          /* dstX/Y/Zoffset */
                                    pitch * cpp,      /* dstRowStride, bytes */
                                    &dstImageOffsets, /* dstImageOffsets */
                                    width, height, 1, /* size */
                                    format, type,     /* src format/type */
                                    pixels,           /* data source */
                                    unpack);

      /* unmap */
      pipe->region_unmap(pipe, mt->region);

      assert(success);
   }

   mt->target = GL_TEXTURE_2D;
   mt->internal_format = GL_RGBA;
   mt->format = pipeFormat;
   mt->first_level = 0;
   mt->last_level = 0;
   mt->width0 = width;
   mt->height0 = height;
   mt->depth0 = 1;
   mt->cpp = cpp;
   mt->compressed = 0;
   mt->pitch = mt->region->pitch;
   mt->depth_pitch = 0;
   mt->total_height = height;
   mt->level[0].level_offset = 0;
   mt->level[0].width = width;
   mt->level[0].height = height;
   mt->level[0].depth = 1;
   mt->level[0].nr_images = 1;
   mt->level[0].image_offset = NULL;
   mt->refcount = 1;

   return mt;
}


static void
free_mipmap_tree(struct pipe_context *pipe, struct pipe_mipmap_tree *mt)
{
   pipe->region_release(pipe, &mt->region);
   free(mt);
}


/**
 * Draw textured quad.
 * Y=0=top
 */
static void
draw_quad(struct st_context *st, GLfloat x0, GLfloat y0, GLfloat z,
          GLfloat x1, GLfloat y1)
{
   static const GLuint attribs[2] = {
      VF_ATTRIB_POS,
      VF_ATTRIB_TEX0
   };
   GLfloat verts[4][2][4]; /* four verts, two attribs, XYZW */
   GLuint i;

   /* upper-left */
   verts[0][0][0] = x0;
   verts[0][0][1] = y0;
   verts[0][1][0] = 0.0;
   verts[0][1][1] = 1.0;

   /* upper-right */
   verts[1][0][0] = x1;
   verts[1][0][1] = y0;
   verts[1][1][0] = 1.0;
   verts[1][1][1] = 1.0;

   /* lower-right */
   verts[2][0][0] = x1;
   verts[2][0][1] = y1;
   verts[2][1][0] = 1.0;
   verts[2][1][1] = 0.0;

   /* lower-left */
   verts[3][0][0] = x0;
   verts[3][0][1] = y1;
   verts[3][1][0] = 0.0;
   verts[3][1][1] = 0.0;

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
draw_textured_quad(GLcontext *ctx, GLint x, GLint y, GLfloat z,
                   GLsizei width, GLsizei height, GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid *pixels)
{
   const GLuint unit = 0;
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_mipmap_tree *mt;
   GLfloat x0, y0, x1, y1;

   /* setup state: just scissor */
   {
      struct pipe_setup_state setup;
      memset(&setup, 0, sizeof(setup));
      if (ctx->Scissor.Enabled)
         setup.scissor = 1;
      pipe->set_setup_state(pipe, &setup);
   }

   /* fragment shader state: color pass-through program */
   {
      static struct st_fragment_program *stfp = NULL;
      struct pipe_fs_state fs;
      if (!stfp) {
         stfp = make_drawpixels_shader(ctx->st);
      }
      memset(&fs, 0, sizeof(fs));
      fs.inputs_read = stfp->Base.Base.InputsRead;
      fs.tokens = &stfp->tokens[0];
      fs.constants = NULL;
      pipe->set_fs_state(pipe, &fs);
   }

   /* texture sampling state: */
   {
      struct pipe_sampler_state sampler;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      pipe->set_sampler_state(pipe, unit, &sampler);
   }

   /* mipmap tree state: */
   {
      mt = make_mipmap_tree(ctx->st, width, height, format, type,
                            unpack, pixels);
      pipe->set_texture_state(pipe, unit, mt);
   }

   /* compute window coords (y=0=top) with pixel zoom */
   x0 = x;
   y0 = ctx->DrawBuffer->Height - 1 - y;
   x1 = x + width * ctx->Pixel.ZoomX;
   y1 = ctx->DrawBuffer->Height - 1 - (y + height * ctx->Pixel.ZoomY);

   /* draw textured quad */
   draw_quad(ctx->st, x0, y0, z, x1, y1);

   /* restore GL state */
   pipe->set_setup_state(pipe, &ctx->st->state.setup);
   pipe->set_fs_state(pipe, &ctx->st->state.fs);
   /* XXX FIX: pipe->set_texture_state(pipe, unit, ???); */
   pipe->set_sampler_state(pipe, unit, &ctx->st->state.sampler[0]);

   free_mipmap_tree(pipe, mt);
}


/**
 * Check if a GL format/type combination is a match to the given pipe format.
 * XXX probably move this to a re-usable place.
 */
static GLboolean
compatible_formats(GLenum format, GLenum type, GLuint pipeFormat)
{
   static const GLuint one = 1;
   GLubyte littleEndian = *((GLubyte *) &one);

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
      draw_textured_quad(ctx, x, y, ctx->Current.RasterPos[2], width, height,
                         format, type, unpack, pixels);
   }
   else {
      /* blit */
      draw_blit(st, width, height, format, type, pixels);
   }
}


void st_init_drawpixels_functions(struct dd_function_table *functions)
{
   functions->DrawPixels = st_drawpixels;
}

