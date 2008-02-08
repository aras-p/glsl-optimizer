/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "main/imports.h"
#include "main/mipmap.h"
#include "main/teximage.h"

#include "shader/prog_instruction.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/cso_cache/cso_cache.h"

#include "st_context.h"
#include "st_draw.h"
#include "st_gen_mipmap.h"
#include "st_program.h"
#include "st_cb_texture.h"



static void *blend_cso = NULL;
static void *depthstencil_cso = NULL;
static void *rasterizer_cso = NULL;
static void *sampler_cso = NULL;

static struct st_fragment_program *stfp = NULL;
static struct st_vertex_program *stvp = NULL;



static struct st_fragment_program *
make_tex_fragment_program(GLcontext *ctx)
{
   struct st_fragment_program *stfp;
   struct gl_program *p;
   GLuint ic = 0;

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 2;

   p->Instructions = _mesa_alloc_instructions(p->NumInstructions);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, p->NumInstructions);

   /* TEX result.color, fragment.texcoord[0], texture[0], 2D; */
   p->Instructions[ic].Opcode = OPCODE_TEX;
   p->Instructions[ic].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[ic].DstReg.Index = FRAG_RESULT_COLR;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   p->Instructions[ic].TexSrcUnit = 0;
   p->Instructions[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;

   /* END; */
   p->Instructions[ic++].Opcode = OPCODE_END;

   assert(ic == p->NumInstructions);

   p->InputsRead = FRAG_BIT_TEX0;
   p->OutputsWritten = (1 << FRAG_RESULT_COLR);

   stfp = (struct st_fragment_program *) p;

   st_translate_fragment_program(ctx->st, stfp, NULL,
                                 stfp->tokens, ST_MAX_SHADER_TOKENS);

   return stfp;
}




/**
 * one-time init for generate mipmap
 * XXX Note: there may be other times we need no-op/simple state like this.
 * In that case, some code refactoring would be good.
 */
void
st_init_generate_mipmap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_blend_state blend;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_depth_stencil_alpha_state depthstencil;

   assert(!blend_cso);

   memset(&blend, 0, sizeof(blend));
   blend.colormask = PIPE_MASK_RGBA;
   blend_cso = pipe->create_blend_state(pipe, &blend);

   memset(&depthstencil, 0, sizeof(depthstencil));
   depthstencil_cso = pipe->create_depth_stencil_alpha_state(pipe, &depthstencil);

   memset(&rasterizer, 0, sizeof(rasterizer));
   rasterizer_cso = pipe->create_rasterizer_state(pipe, &rasterizer);

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.normalized_coords = 1;
   sampler_cso = pipe->create_sampler_state(pipe, &sampler);

   stfp = make_tex_fragment_program(st->ctx);
   stvp = st_make_passthrough_vertex_shader(st, GL_FALSE);
}


void
st_destroy_generate_mipmpap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;

   pipe->delete_blend_state(pipe, blend_cso);
   pipe->delete_depth_stencil_alpha_state(pipe, depthstencil_cso);
   pipe->delete_rasterizer_state(pipe, rasterizer_cso);
   pipe->delete_sampler_state(pipe, sampler_cso);

   /* XXX free stfp, stvp */

   blend_cso = NULL;
   depthstencil_cso = NULL;
   rasterizer_cso = NULL;
   sampler_cso = NULL;
}


static void
simple_viewport(struct pipe_context *pipe, uint width, uint height)
{
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



/*
 * Draw simple [-1,1]x[-1,1] quad
 */
static void
draw_quad(GLcontext *ctx)
{
   GLfloat verts[4][2][4]; /* four verts, two attribs, XYZW */
   GLuint i;
   GLfloat sLeft = 0.0, sRight = 1.0;
   GLfloat tTop = 1.0, tBot = 0.0;
   GLfloat x0 = -1.0, x1 = 1.0;
   GLfloat y0 = -1.0, y1 = 1.0;

   /* upper-left */
   verts[0][0][0] = x0;    /* attr[0].x */
   verts[0][0][1] = y0;    /* attr[0].y */
   verts[0][1][0] = sLeft; /* attr[1].s */
   verts[0][1][1] = tTop;  /* attr[1].t */

   /* upper-right */
   verts[1][0][0] = x1;
   verts[1][0][1] = y0;
   verts[1][1][0] = sRight;
   verts[1][1][1] = tTop;

   /* lower-right */
   verts[2][0][0] = x1;
   verts[2][0][1] = y1;
   verts[2][1][0] = sRight;
   verts[2][1][1] = tBot;

   /* lower-left */
   verts[3][0][0] = x0;
   verts[3][0][1] = y1;
   verts[3][1][0] = sLeft;
   verts[3][1][1] = tBot;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      verts[i][0][2] = 0.0; /*Z*/
      verts[i][0][3] = 1.0; /*W*/
      verts[i][1][2] = 0.0; /*R*/
      verts[i][1][3] = 1.0; /*Q*/
   }

   st_draw_vertices(ctx, PIPE_PRIM_QUADS, 4, (float *) verts, 2, GL_TRUE);
}



/**
 * Generate mipmap levels using hardware rendering.
 * \return TRUE if successful, FALSE if not possible
 */
static boolean
st_render_mipmap(struct st_context *st,
                 struct pipe_texture *pt,
                 uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_framebuffer_state fb;
   const uint face = 0, zslice = 0;
   const uint first_level_save = pt->first_level;
   uint dstLevel;

   /* check if we can render in the texture's format */
   if (!pipe->is_format_supported(pipe, pt->format, PIPE_SURFACE)) {
      return FALSE;
   }

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));
   fb.num_cbufs = 1;

   /* bind CSOs */
   pipe->bind_blend_state(pipe, blend_cso);
   pipe->bind_depth_stencil_alpha_state(pipe, depthstencil_cso);
   pipe->bind_rasterizer_state(pipe, rasterizer_cso);
   pipe->bind_sampler_state(pipe, 0, sampler_cso);

   /* bind shaders */
   pipe->bind_fs_state(pipe, stfp->fs->data);
   pipe->bind_vs_state(pipe, stvp->cso->data);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;

      /*
       * Setup framebuffer / dest surface
       */
      fb.cbufs[0] = pipe->get_tex_surface(pipe, pt, face, dstLevel, zslice);
      pipe->set_framebuffer_state(pipe, &fb);

      simple_viewport(pipe, pt->width[dstLevel], pt->height[dstLevel]);

      /*
       * Setup src texture, override pt->first_level so we sample from
       * the right mipmap level.
       */
      pt->first_level = srcLevel;
      pipe->set_sampler_texture(pipe, 0, pt);

      draw_quad(st->ctx);
   }

   /* restore first_level */
   pt->first_level = first_level_save;

   /* restore pipe state */
   if (st->state.rasterizer)
      pipe->bind_rasterizer_state(pipe, st->state.rasterizer->data);
   if (st->state.fs)
      pipe->bind_fs_state(pipe, st->state.fs->data);
   if (st->state.vs)
      pipe->bind_vs_state(pipe, st->state.vs->cso->data);
   if (st->state.sampler[0])
      pipe->bind_sampler_state(pipe, 0, st->state.sampler[0]->data);
   pipe->set_sampler_texture(pipe, 0, st->state.sampler_texture[0]);
   pipe->set_viewport_state(pipe, &st->state.viewport);

   return TRUE;
}



void
st_generate_mipmap(GLcontext *ctx, GLenum target,
                   struct gl_texture_object *texObj)
{
   struct st_context *st = ctx->st;
   struct pipe_texture *pt = st_get_texobj_texture(texObj);
   const uint baseLevel = texObj->BaseLevel;
   const uint lastLevel = pt->last_level;
   uint dstLevel;

   if (!st_render_mipmap(st, pt, baseLevel, lastLevel)) {
      abort();
      /* XXX the following won't really work at this time */
      _mesa_generate_mipmap(ctx, target, texObj);
      return;
   }

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      const struct gl_texture_image *srcImage
         = _mesa_get_tex_image(ctx, texObj, target, srcLevel);
      struct gl_texture_image *dstImage;
      struct st_texture_image *stImage;
      uint dstWidth = pt->width[dstLevel];
      uint dstHeight = pt->height[dstLevel];
      uint dstDepth = pt->depth[dstLevel];
      uint border = srcImage->Border;


      dstImage = _mesa_get_tex_image(ctx, texObj, target, dstLevel);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      if (dstImage->ImageOffsets)
         _mesa_free(dstImage->ImageOffsets);

      /* Free old image data */
      if (dstImage->Data)
         ctx->Driver.FreeTexImageData(ctx, dstImage);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->InternalFormat);

      dstImage->TexFormat = srcImage->TexFormat;

      stImage = (struct st_texture_image *) dstImage;
      stImage->pt = pt;
   }

}
