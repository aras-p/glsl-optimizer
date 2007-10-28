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

#include "st_context.h"
#include "st_atom.h"
#include "st_cache.h"
#include "st_draw.h"
#include "st_program.h"
#include "st_cb_drawpixels.h"
#include "st_cb_readpixels.h"
#include "st_cb_fbo.h"
#include "st_cb_texture.h"
#include "st_draw.h"
#include "st_format.h"
#include "st_mesa_to_tgsi.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "shader/prog_instruction.h"



/**
 * Create a simple fragment shader that does a TEX() instruction to get
 * the fragment color.
 * If bitmapMode, use KIL instruction to kill the "0-pixels".
 */
static struct st_fragment_program *
make_fragment_shader(struct st_context *st, GLboolean bitmapMode)
{
   /* only make programs once and re-use */
   static struct st_fragment_program *progs[2] = { NULL, NULL };
   GLcontext *ctx = st->ctx;
   struct st_fragment_program *stfp;
   struct gl_program *p;
   GLuint ic = 0;

   if (progs[bitmapMode])
      return progs[bitmapMode];

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   if (bitmapMode)
      p->NumInstructions = 6;
   else
      p->NumInstructions = 2;

   p->Instructions = _mesa_alloc_instructions(p->NumInstructions);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, p->NumInstructions);
   if (bitmapMode) {
      /*
       * XXX, we need to compose this fragment shader with the current
       * user-provided fragment shader so the fragment program is applied
       * to the fragments which aren't culled.
       */
      /* TEX tmp0, fragment.texcoord[0], texture[0], 2D; */
      p->Instructions[ic].Opcode = OPCODE_TEX;
      p->Instructions[ic].DstReg.File = PROGRAM_TEMPORARY;
      p->Instructions[ic].DstReg.Index = 0;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
      p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
      p->Instructions[ic].TexSrcUnit = 0;
      p->Instructions[ic].TexSrcTarget = TEXTURE_2D_INDEX;
      ic++;

      /* SWZ tmp0.x, tmp0.x, 1111; # tmp0.x = 1.0 */
      p->Instructions[ic].Opcode = OPCODE_SWZ;
      p->Instructions[ic].DstReg.File = PROGRAM_TEMPORARY;
      p->Instructions[ic].DstReg.Index = 0;
      p->Instructions[ic].DstReg.WriteMask = WRITEMASK_X;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      p->Instructions[ic].SrcReg[0].Index = 0;
      p->Instructions[ic].SrcReg[0].Swizzle
         = MAKE_SWIZZLE4(SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE );
      ic++;

      /* SUB tmp0, tmp0.wwww, tmp0.xxxx;  #  tmp0.w -= 1 */
      p->Instructions[ic].Opcode = OPCODE_SUB;
      p->Instructions[ic].DstReg.File = PROGRAM_TEMPORARY;
      p->Instructions[ic].DstReg.Index = 0;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      p->Instructions[ic].SrcReg[0].Index = 0;
      p->Instructions[ic].SrcReg[0].Swizzle = SWIZZLE_WWWW;
      p->Instructions[ic].SrcReg[1].File = PROGRAM_TEMPORARY;
      p->Instructions[ic].SrcReg[1].Index = 0;
      p->Instructions[ic].SrcReg[1].Swizzle = SWIZZLE_XXXX; /* 1.0 */
      ic++;

      /* KIL if tmp0 < 0 */
      p->Instructions[ic].Opcode = OPCODE_KIL;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      p->Instructions[ic].SrcReg[0].Index = 0;
      ic++;

      /* MOV result.color, fragment.color */
      p->Instructions[ic].Opcode = OPCODE_MOV;
      p->Instructions[ic].DstReg.File = PROGRAM_OUTPUT;
      p->Instructions[ic].DstReg.Index = FRAG_RESULT_COLR;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
      p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_COL0;
      ic++;
   }
   else {
      /* DrawPixels mode */
      /* TEX result.color, fragment.texcoord[0], texture[0], 2D; */
      p->Instructions[ic].Opcode = OPCODE_TEX;
      p->Instructions[ic].DstReg.File = PROGRAM_OUTPUT;
      p->Instructions[ic].DstReg.Index = FRAG_RESULT_COLR;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
      p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
      p->Instructions[ic].TexSrcUnit = 0;
      p->Instructions[ic].TexSrcTarget = TEXTURE_2D_INDEX;
      ic++;
   }
   /* END; */
   p->Instructions[ic++].Opcode = OPCODE_END;

   assert(ic == p->NumInstructions);

   p->InputsRead = FRAG_BIT_TEX0;
   p->OutputsWritten = (1 << FRAG_RESULT_COLR);
   if (bitmapMode) {
      p->InputsRead |= FRAG_BIT_COL0;
   }

   stfp = (struct st_fragment_program *) p;
   st_translate_fragment_program(st, stfp, NULL,
                                 stfp->tokens, ST_MAX_SHADER_TOKENS);

   progs[bitmapMode] = stfp;

   return stfp;
}


/**
 * Create fragment shader that does a TEX() instruction to get a Z
 * value, then writes to FRAG_RESULT_DEPR.
 * Pass fragment color through as-is.
 */
static struct st_fragment_program *
make_fragment_shader_z(struct st_context *st)
{
   GLcontext *ctx = st->ctx;
   /* only make programs once and re-use */
   static struct st_fragment_program *stfp = NULL;
   struct gl_program *p;
   GLuint ic = 0;

   if (stfp)
      return stfp;

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 3;

   p->Instructions = _mesa_alloc_instructions(p->NumInstructions);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, p->NumInstructions);

   /* TEX result.color, fragment.texcoord[0], texture[0], 2D; */
   p->Instructions[ic].Opcode = OPCODE_TEX;
   p->Instructions[ic].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[ic].DstReg.Index = FRAG_RESULT_DEPR;
   p->Instructions[ic].DstReg.WriteMask = WRITEMASK_Z;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   p->Instructions[ic].TexSrcUnit = 0;
   p->Instructions[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;

   /* MOV result.color, fragment.color */
   p->Instructions[ic].Opcode = OPCODE_MOV;
   p->Instructions[ic].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[ic].DstReg.Index = FRAG_RESULT_COLR;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_COL0;
   ic++;

   /* END; */
   p->Instructions[ic++].Opcode = OPCODE_END;

   assert(ic == p->NumInstructions);

   p->InputsRead = FRAG_BIT_TEX0 | FRAG_BIT_COL0;
   p->OutputsWritten = (1 << FRAG_RESULT_COLR) | (1 << FRAG_RESULT_DEPR);

   stfp = (struct st_fragment_program *) p;
   st_translate_fragment_program(st, stfp, NULL,
                                 stfp->tokens, ST_MAX_SHADER_TOKENS);

   return stfp;
}



/**
 * Create a simple vertex shader that just passes through the
 * vertex position and texcoord (and optionally, color).
 */
static struct st_vertex_program *
make_vertex_shader(struct st_context *st, GLboolean passColor)
{
   /* only make programs once and re-use */
   static struct st_vertex_program *progs[2] = { NULL, NULL };
   GLcontext *ctx = st->ctx;
   struct st_vertex_program *stvp;
   struct gl_program *p;
   GLuint ic = 0;

   if (progs[passColor])
      return progs[passColor];

   p = ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   if (passColor)
      p->NumInstructions = 4;
   else
      p->NumInstructions = 3;

   p->Instructions = _mesa_alloc_instructions(p->NumInstructions);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, p->NumInstructions);
   /* MOV result.pos, vertex.pos; */
   p->Instructions[0].Opcode = OPCODE_MOV;
   p->Instructions[0].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[0].DstReg.Index = VERT_RESULT_HPOS;
   p->Instructions[0].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[0].SrcReg[0].Index = VERT_ATTRIB_POS;
   /* MOV result.texcoord0, vertex.texcoord0; */
   p->Instructions[1].Opcode = OPCODE_MOV;
   p->Instructions[1].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[1].DstReg.Index = VERT_RESULT_TEX0;
   p->Instructions[1].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[1].SrcReg[0].Index = VERT_ATTRIB_TEX0;
   ic = 2;
   if (passColor) {
      /* MOV result.color0, vertex.color0; */
      p->Instructions[ic].Opcode = OPCODE_MOV;
      p->Instructions[ic].DstReg.File = PROGRAM_OUTPUT;
      p->Instructions[ic].DstReg.Index = VERT_RESULT_COL0;
      p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
      p->Instructions[ic].SrcReg[0].Index = VERT_ATTRIB_COLOR0;
      ic++;
   }

   /* END; */
   p->Instructions[ic].Opcode = OPCODE_END;
   ic++;

   assert(ic == p->NumInstructions);

   p->InputsRead = VERT_BIT_POS | VERT_BIT_TEX0;
   p->OutputsWritten = ((1 << VERT_RESULT_TEX0) |
                        (1 << VERT_RESULT_HPOS));
   if (passColor) {
      p->InputsRead |= VERT_BIT_COLOR0;
      p->OutputsWritten |= (1 << VERT_RESULT_COL0);
   }

   stvp = (struct st_vertex_program *) p;
   st_translate_vertex_program(st, stvp, NULL,
                               stvp->tokens, ST_MAX_SHADER_TOKENS);

   progs[passColor] = stvp;

   return stvp;
}


static GLenum
_mesa_base_format(GLenum format)
{
   switch (format) {
   case GL_DEPTH_COMPONENT:
      return GL_DEPTH_COMPONENT;
   case GL_STENCIL_INDEX:
      return GL_STENCIL_INDEX;
   default:
      return GL_RGBA;
   }
}



static struct pipe_mipmap_tree *
alloc_mipmap_tree(struct st_context *st,
                  GLsizei width, GLsizei height, uint pipeFormat)
{
   const GLbitfield flags = PIPE_SURFACE_FLAG_TEXTURE;
   struct pipe_mipmap_tree *mt;
   GLuint cpp;

   mt = CALLOC_STRUCT(pipe_mipmap_tree);
   if (!mt)
      return NULL;

   cpp = st_sizeof_format(pipeFormat);

   /* allocate texture region/storage */
   mt->region = st->pipe->winsys->region_alloc(st->pipe->winsys,
                                               cpp, width, height, flags);

   mt->target = PIPE_TEXTURE_2D;
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


/**
 * Make mipmap tree containing an image for glDrawPixels image.
 * If 'pixels' is NULL, leave the texture image data undefined.
 */
static struct pipe_mipmap_tree *
make_mipmap_tree(struct st_context *st,
                 GLsizei width, GLsizei height, GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *unpack,
                 const GLvoid *pixels)
{
   struct pipe_context *pipe = st->pipe;
   const struct gl_texture_format *mformat;
   struct pipe_mipmap_tree *mt;
   GLuint pipeFormat, cpp;
   GLenum baseFormat;

   baseFormat = _mesa_base_format(format);

   mformat = st_ChooseTextureFormat(st->ctx, baseFormat, format, type);
   assert(mformat);

   pipeFormat = st_mesa_format_to_pipe_format(mformat->MesaFormat);
   assert(pipeFormat);
   cpp = st_sizeof_format(pipeFormat);

   mt = alloc_mipmap_tree(st, width, height, pipeFormat);
   if (!mt)
      return NULL;

   if (unpack->BufferObj && unpack->BufferObj->Name) {
      /*
      mt->region = buffer_object_region(unpack->BufferObj);
      */
      printf("st_DrawPixels (sourcing from PBO not implemented yet)\n");
   }

   {
      static const GLuint dstImageOffsets = 0;
      GLboolean success;
      GLuint pitch = mt->region->pitch;
      GLubyte *dest;

      /* map texture region */
      dest = pipe->region_map(pipe, mt->region);

      /* Put image into texture region.
       * Note that the image is actually going to be upside down in
       * the texture.  We deal with that with texcoords.
       */
      success = mformat->StoreImage(st->ctx, 2,       /* dims */
                                    baseFormat,       /* baseInternalFormat */
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

#if 0
   mt->target = PIPE_TEXTURE_2D;
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
#endif
   return mt;
}


static void
free_mipmap_tree(struct pipe_context *pipe, struct pipe_mipmap_tree *mt)
{
   pipe->winsys->region_release(pipe->winsys, &mt->region);
   free(mt);
}


/**
 * Draw textured quad.
 * Coords are window coords with y=0=bottom.
 */
static void
draw_quad(GLcontext *ctx, GLfloat x0, GLfloat y0, GLfloat z,
          GLfloat x1, GLfloat y1, GLboolean invertTex)
{
   GLfloat verts[4][2][4]; /* four verts, two attribs, XYZW */
   GLuint i;
   GLfloat tTop = invertTex, tBot = 1.0 - tTop;

   /* upper-left */
   verts[0][0][0] = x0;    /* attr[0].x */
   verts[0][0][1] = y0;    /* attr[0].x */
   verts[0][1][0] = 0.0;   /* attr[1].s */
   verts[0][1][1] = tTop;  /* attr[1].t */

   /* upper-right */
   verts[1][0][0] = x1;
   verts[1][0][1] = y0;
   verts[1][1][0] = 1.0;
   verts[1][1][1] = tTop;

   /* lower-right */
   verts[2][0][0] = x1;
   verts[2][0][1] = y1;
   verts[2][1][0] = 1.0;
   verts[2][1][1] = tBot;

   /* lower-left */
   verts[3][0][0] = x0;
   verts[3][0][1] = y1;
   verts[3][1][0] = 0.0;
   verts[3][1][1] = tBot;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      verts[i][0][2] = z;   /*Z*/
      verts[i][0][3] = 1.0; /*W*/
      verts[i][1][2] = 0.0; /*R*/
      verts[i][1][3] = 1.0; /*Q*/
   }

   st_draw_vertices(ctx, PIPE_PRIM_QUADS, 4, (float *) verts, 2);
}


static void
draw_quad_colored(GLcontext *ctx, GLfloat x0, GLfloat y0, GLfloat z,
                  GLfloat x1, GLfloat y1, const GLfloat *color,
                  GLboolean invertTex)
{
   GLfloat verts[4][3][4]; /* four verts, three attribs, XYZW */
   GLuint i;
   GLfloat tTop = invertTex, tBot = 1.0 - tTop;

   /* upper-left */
   verts[0][0][0] = x0;    /* attr[0].x */
   verts[0][0][1] = y0;    /* attr[0].y */
   verts[0][2][0] = 0.0;   /* attr[2].s */
   verts[0][2][1] = tTop;  /* attr[2].t */

   /* upper-right */
   verts[1][0][0] = x1;
   verts[1][0][1] = y0;
   verts[1][2][0] = 1.0;
   verts[1][2][1] = tTop;

   /* lower-right */
   verts[2][0][0] = x1;
   verts[2][0][1] = y1;
   verts[2][2][0] = 1.0;
   verts[2][2][1] = tBot;

   /* lower-left */
   verts[3][0][0] = x0;
   verts[3][0][1] = y1;
   verts[3][2][0] = 0.0;
   verts[3][2][1] = tBot;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      verts[i][0][2] = z;   /*Z*/
      verts[i][0][3] = 1.0; /*W*/
      verts[i][1][0] = color[0];
      verts[i][1][1] = color[1];
      verts[i][1][2] = color[2];
      verts[i][1][3] = color[3];
      verts[i][2][2] = 0.0; /*R*/
      verts[i][2][3] = 1.0; /*Q*/
   }

   st_draw_vertices(ctx, PIPE_PRIM_QUADS, 4, (float *) verts, 3);
}



static void
draw_textured_quad(GLcontext *ctx, GLint x, GLint y, GLfloat z,
                   GLsizei width, GLsizei height,
                   GLfloat zoomX, GLfloat zoomY,
                   struct pipe_mipmap_tree *mt,
                   struct st_vertex_program *stvp,
                   struct st_fragment_program *stfp,
                   const GLfloat *color,
                   GLboolean invertTex)
{
   const GLuint unit = 0;
   struct pipe_context *pipe = ctx->st->pipe;
   GLfloat x0, y0, x1, y1;
   GLuint maxWidth, maxHeight;

   /* limit checks */
   /* XXX if DrawPixels image is larger than max texture size, break
    * it up into chunks.
    */
   pipe->max_texture_size(pipe, PIPE_TEXTURE_2D, &maxWidth, &maxHeight, NULL);
   assert(width <= maxWidth);
   assert(height <= maxHeight);

   /* setup state: just scissor */
   {
      struct pipe_rasterizer_state  setup;
      const struct cso_rasterizer  *cso;
      memset(&setup, 0, sizeof(setup));
      if (ctx->Scissor.Enabled)
         setup.scissor = 1;
      cso = st_cached_rasterizer_state(ctx->st, &setup);
      pipe->bind_rasterizer_state(pipe, cso->data);
   }

   /* fragment shader state: TEX lookup program */
   pipe->bind_fs_state(pipe, stfp->fs->data);

   /* vertex shader state: position + texcoord pass-through */
   pipe->bind_vs_state(pipe, stvp->vs->data);

   /* texture sampling state: */
   {
      struct pipe_sampler_state sampler;
      const struct cso_sampler *cso;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.normalized_coords = 1;
      cso = st_cached_sampler_state(ctx->st, &sampler);
      pipe->bind_sampler_state(pipe, unit, cso->data);
   }

   /* viewport state: viewport matching window dims */
   {
      const float width = ctx->DrawBuffer->Width;
      const float height = ctx->DrawBuffer->Height;
      struct pipe_viewport_state vp;
      vp.scale[0] =  0.5 * width;
      vp.scale[1] = -0.5 * height;
      vp.scale[2] = 1.0;
      vp.scale[3] = 1.0;
      vp.translate[0] = 0.5 * width;
      vp.translate[1] = 0.5 * height;
      vp.translate[2] = 0.0;
      vp.translate[3] = 0.0;
      pipe->set_viewport_state(pipe, &vp);
   }

   /* mipmap tree state: */
   {
      pipe->set_texture_state(pipe, unit, mt);
   }

   /* Compute window coords (y=0=bottom) with pixel zoom.
    * Recall that these coords are transformed by the current
    * vertex shader and viewport transformation.
    */
   x0 = x;
   x1 = x + width * ctx->Pixel.ZoomX;
   y0 = y;
   y1 = y + height * ctx->Pixel.ZoomY;

   /* draw textured quad */
   if (color)
      draw_quad_colored(ctx, x0, y0, z, x1, y1, color, invertTex);
   else
      draw_quad(ctx, x0, y0, z, x1, y1, invertTex);

   /* restore GL state */
   pipe->bind_rasterizer_state(pipe, ctx->st->state.rasterizer->data);
   pipe->bind_fs_state(pipe, ctx->st->state.fs->data);
   pipe->bind_vs_state(pipe, ctx->st->state.vs->data);
   pipe->set_texture_state(pipe, unit, ctx->st->state.texture[unit]);
   pipe->bind_sampler_state(pipe, unit, ctx->st->state.sampler[unit]->data);
   pipe->set_viewport_state(pipe, &ctx->st->state.viewport);
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
   if (st->state.alpha_test->state.enabled ||
       st->state.blend->state.blend_enable ||
       st->state.blend->state.logicop_enable ||
       st->state.depth_stencil->state.depth.enabled)
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


static void
draw_stencil_pixels(GLcontext *ctx, GLint x, GLint y,
                    GLsizei width, GLsizei height, GLenum type,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLvoid *pixels)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   struct pipe_surface *ps = st->state.framebuffer.sbuf;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;
   GLint skipPixels;
   ubyte *stmap;

   pipe->flush(pipe, 0);

   /* map the stencil buffer */
   stmap = pipe->region_map(pipe, ps->region);

   /* if width > MAX_WIDTH, have to process image in chunks */
   skipPixels = 0;
   while (skipPixels < width) {
      const GLint spanX = x + skipPixels;
      const GLint spanWidth = MIN2(width - skipPixels, MAX_WIDTH);
      GLint row;
      for (row = 0; row < height; row++) {
         GLint spanY = y + row;
         GLubyte values[MAX_WIDTH];
         GLenum destType = GL_UNSIGNED_BYTE;
         const GLvoid *source = _mesa_image_address2d(unpack, pixels,
                                                      width, height,
                                                      GL_COLOR_INDEX, type,
                                                      row, skipPixels);
         _mesa_unpack_stencil_span(ctx, spanWidth, destType, values,
                                   type, source, unpack,
                                   ctx->_ImageTransferState);
         if (zoom) {
            /*
            _swrast_write_zoomed_stencil_span(ctx, x, y, spanWidth,
                                              spanX, spanY, values);
            */
         }
         else {
            if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
               spanY = ctx->DrawBuffer->Height - spanY - 1;
            }

            switch (ps->format) {
            case PIPE_FORMAT_U_S8:
               {
                  ubyte *dest = stmap + spanY * ps->region->pitch + spanX;
                  memcpy(dest, values, spanWidth);
               }
               break;
            case PIPE_FORMAT_S8_Z24:
               {
                  uint *dest = (uint *) stmap + spanY * ps->region->pitch + spanX;
                  GLint k;
                  for (k = 0; k < spanWidth; k++) {
                     uint p = dest[k];
                     p = (p & 0xffffff) | (values[k] << 24);
                     dest[k] = p;
                  }
               }
               break;
            default:
               assert(0);
            }
         }
      }
      skipPixels += spanWidth;
   }

   /* unmap the stencil buffer */
   pipe->region_unmap(pipe, ps->region);
}


/**
 * Called via ctx->Driver.DrawPixels()
 */
static void
st_DrawPixels(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height,
              GLenum format, GLenum type,
              const struct gl_pixelstore_attrib *unpack, const GLvoid *pixels)
{
   struct st_fragment_program *stfp;
   struct st_vertex_program *stvp;
   struct st_context *st = ctx->st;
   struct pipe_surface *ps;
   GLuint bufferFormat;
   const GLfloat *color;

   if (format == GL_STENCIL_INDEX) {
      draw_stencil_pixels(ctx, x, y, width, height, type, unpack, pixels);
      return;
   }

   st_validate_state(st);

   if (format == GL_DEPTH_COMPONENT) {
      ps = st->state.framebuffer.zbuf;
      stfp = make_fragment_shader_z(ctx->st);
      stvp = make_vertex_shader(ctx->st, GL_TRUE);
      color = ctx->Current.RasterColor;
   }
   else if (format == GL_STENCIL_INDEX) {
      ps = st->state.framebuffer.sbuf;
      /* XXX special case - can't use texture map */
      color = NULL;
   }
   else {
      ps = st->state.framebuffer.cbufs[0];
      stfp = make_fragment_shader(ctx->st, GL_FALSE);
      stvp = make_vertex_shader(ctx->st, GL_FALSE);
      color = NULL;
   }

   bufferFormat = ps->format;

   if (any_fragment_ops(st) ||
       any_pixel_transfer_ops(st) ||
       !compatible_formats(format, type, ps->format)) {
      /* textured quad */
      struct pipe_mipmap_tree *mt
         = make_mipmap_tree(ctx->st, width, height, format, type,
                            unpack, pixels);
      if (mt) {
         draw_textured_quad(ctx, x, y, ctx->Current.RasterPos[2],
                            width, height, ctx->Pixel.ZoomX, ctx->Pixel.ZoomY,
                            mt, stvp, stfp, color, GL_FALSE);
         free_mipmap_tree(st->pipe, mt);
      }
   }
   else {
      /* blit */
      draw_blit(st, width, height, format, type, pixels);
   }
}



/**
 * Create a texture which represents a bitmap image.
 */
static struct pipe_mipmap_tree *
make_bitmap_texture(GLcontext *ctx, GLsizei width, GLsizei height,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLubyte *bitmap)
{
   struct pipe_context *pipe = ctx->st->pipe;
   const uint flags = PIPE_SURFACE_FLAG_TEXTURE;
   uint format = 0, cpp, comp, pitch;
   ubyte *dest;
   struct pipe_mipmap_tree *mt;
   int row, col;

   /* find a texture format we know */
   if (pipe->is_format_supported( pipe, PIPE_FORMAT_U_I8 )) {
      format = PIPE_FORMAT_U_I8;
      cpp = 1;
      comp = 0;
   }
   else if (pipe->is_format_supported( pipe, PIPE_FORMAT_U_A8_R8_G8_B8 )) {
      format = PIPE_FORMAT_U_A8_R8_G8_B8;
      cpp = 4;
      comp = 3; /* alpha channel */ /*XXX little-endian dependency */
   }
   else {
      /* XXX support more formats */
      assert( 0 );
   }

   /**
    * Create a mipmap tree.
    */
   mt = CALLOC_STRUCT(pipe_mipmap_tree);
   if (!mt)
      return NULL;

   if (unpack->BufferObj && unpack->BufferObj->Name) {
      /*
      mt->region = buffer_object_region(unpack->BufferObj);
      */
      printf("st_Bitmap (sourcing from PBO not implemented yet)\n");
   }


   /* allocate texture region/storage */
   mt->region = pipe->winsys->region_alloc(pipe->winsys,
                                           cpp, width, height, flags);
   pitch = mt->region->pitch;

   /* map texture region */
   dest = pipe->region_map(pipe, mt->region);
   if (!dest) {
      printf("st_Bitmap region_map() failed!?!");
      return NULL;
   }

   /* Put image into texture region.
    * Note that the image is actually going to be upside down in
    * the texture.  We deal with that with texcoords.
    */

   for (row = 0; row < height; row++) {
      const GLubyte *src = (const GLubyte *) _mesa_image_address2d(unpack,
                 bitmap, width, height, GL_COLOR_INDEX, GL_BITMAP, row, 0);
      ubyte *destRow = dest + row * pitch * cpp;

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            /* set texel to 255 if bit is set */
            destRow[comp] = (*src & mask) ? 255 : 0;
            destRow += cpp;

            if (mask == 128U) {
               src++;
               mask = 1U;
            }
            else {
               mask = mask << 1;
            }
         }

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            /* set texel to 255 if bit is set */
            destRow[comp] =(*src & mask) ? 255 : 0;
            destRow += cpp;

            if (mask == 1U) {
               src++;
               mask = 128U;
            }
            else {
               mask = mask >> 1;
            }
         }

         /* get ready for next row */
         if (mask != 128)
            src++;
      }

   } /* row */

   /* unmap */
   pipe->region_unmap(pipe, mt->region);

   mt->target = PIPE_TEXTURE_2D;
   mt->internal_format = GL_RGBA;
   mt->format = format;
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
st_Bitmap(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height,
          const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap )
{
   struct st_vertex_program *stvp;
   struct st_fragment_program *stfp;
   struct st_context *st = ctx->st;
   struct pipe_mipmap_tree *mt;

   /* create the fragment program */
   stfp = make_fragment_shader(ctx->st, GL_TRUE);

   /* and vertex program */
   stvp = make_vertex_shader(ctx->st, GL_TRUE);

   st_validate_state(st);

   mt = make_bitmap_texture(ctx, width, height, unpack, bitmap);
   if (mt) {
      draw_textured_quad(ctx, x, y, ctx->Current.RasterPos[2],
                         width, height, 1.0, 1.0,
                         mt, stvp, stfp,
                         ctx->Current.RasterColor, GL_FALSE);

      free_mipmap_tree(st->pipe, mt);
   }
}


static void
copy_stencil_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                    GLsizei width, GLsizei height,
                    GLint dstx, GLint dsty)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   struct st_renderbuffer *rbRead = st_renderbuffer(ctx->ReadBuffer->_StencilBuffer);
   struct st_renderbuffer *rbDraw = st_renderbuffer(ctx->DrawBuffer->_StencilBuffer);
   struct pipe_surface *psRead = rbRead->surface;
   struct pipe_surface *psDraw = rbDraw->surface;
   ubyte *readMap, *drawMap;
   ubyte *buffer;
   int i;

   buffer = malloc(width * height * sizeof(ubyte));
   if (!buffer) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels(stencil)");
      return;
   }

   /* map the stencil buffers */
   readMap = pipe->region_map(pipe, psRead->region);
   drawMap = pipe->region_map(pipe, psDraw->region);

   /* this will do stencil pixel transfer ops */
   st_read_stencil_pixels(ctx, srcx, srcy, width, height, GL_UNSIGNED_BYTE,
                          &ctx->DefaultPacking, buffer);

   /* draw */
   /* XXX PixelZoom not handled yet */
   for (i = 0; i < height; i++) {
      ubyte *dst;
      const ubyte *src;
      int y;

      y = dsty + i;

      if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
         y = ctx->DrawBuffer->Height - y - 1;
      }

      dst = drawMap + (y * psDraw->region->pitch + dstx) * psDraw->region->cpp;
      src = buffer + i * width;

      switch (psDraw->format) {
      case PIPE_FORMAT_S8_Z24:
         {
            uint *dst4 = (uint *) dst;
            int j;
            for (j = 0; j < width; j++) {
               *dst4 = (*dst4 & 0xffffff) | (src[j] << 24);
               dst4++;
            }
         }
         break;
      case PIPE_FORMAT_U_S8:
         memcpy(dst, src, width);
         break;
      default:
         assert(0);
      }
   }

   free(buffer);

   /* unmap the stencil buffers */
   pipe->region_unmap(pipe, psRead->region);
   pipe->region_unmap(pipe, psDraw->region);
}


static void
st_CopyPixels(GLcontext *ctx, GLint srcx, GLint srcy,
              GLsizei width, GLsizei height,
              GLint dstx, GLint dsty, GLenum type)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   struct st_renderbuffer *rbRead;
   struct st_vertex_program *stvp;
   struct st_fragment_program *stfp;
   struct pipe_surface *psRead;
   struct pipe_mipmap_tree *mt;
   GLfloat *color;
   uint format;

   st_validate_state(st);

   if (type == GL_STENCIL) {
      /* can't use texturing to do stencil */
      copy_stencil_pixels(ctx, srcx, srcy, width, height, dstx, dsty);
      return;
   }

   if (type == GL_COLOR) {
      rbRead = st_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
      color = NULL;
      stfp = make_fragment_shader(ctx->st, GL_FALSE);
      stvp = make_vertex_shader(ctx->st, GL_FALSE);
   }
   else {
      rbRead = st_renderbuffer(ctx->ReadBuffer->_DepthBuffer);
      color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
      stfp = make_fragment_shader_z(ctx->st);
      stvp = make_vertex_shader(ctx->st, GL_TRUE);
   }

   psRead = rbRead->surface;
   format = psRead->format;

   mt = alloc_mipmap_tree(ctx->st, width, height, format);
   if (!mt)
      return;

   if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
      srcy = ctx->DrawBuffer->Height - srcy - height;
   }

   /* For some drivers (like Xlib) it's not possible to treat the
    * front/back color buffers as regions (they're XImages and Pixmaps).
    * So, this var tells us if we can use region_copy here...
    */
   if (st->haveFramebufferRegions) {
      /* copy source framebuffer region into mipmap/texture */
      pipe->region_copy(pipe,
                        mt->region, /* dest */
                        0, /* dest_offset */
                        0, 0, /* destx/y */
                        psRead->region,
                        0, /* src_offset */
                        srcx, srcy, width, height);
   }
   else {
      /* alternate path using get/put_tile() */
      struct pipe_surface *psTex;
      GLfloat *buf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

      psTex = pipe->get_tex_surface(pipe, mt, 0, 0, 0);

      (void) pipe->region_map(pipe, psRead->region);
      (void) pipe->region_map(pipe, psTex->region);

      pipe->get_tile_rgba(pipe, psRead, srcx, srcy, width, height, buf);
      pipe->put_tile_rgba(pipe, psTex, 0, 0, width, height, buf);

      pipe->region_unmap(pipe, psRead->region);
      pipe->region_unmap(pipe, psTex->region);

      pipe_surface_reference(&psTex, NULL);

      free(buf);
   }


   /* draw textured quad */
   draw_textured_quad(ctx, dstx, dsty, ctx->Current.RasterPos[2],
                      width, height, ctx->Pixel.ZoomX, ctx->Pixel.ZoomY,
                      mt, stvp, stfp, color, GL_TRUE);

   free_mipmap_tree(st->pipe, mt);
}



void st_init_drawpixels_functions(struct dd_function_table *functions)
{
   functions->DrawPixels = st_DrawPixels;
   functions->CopyPixels = st_CopyPixels;
   functions->Bitmap = st_Bitmap;
}
