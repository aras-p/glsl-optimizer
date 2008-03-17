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
#include "main/texformat.h"

#include "shader/prog_instruction.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_context.h"

#include "st_context.h"
#include "st_draw.h"
#include "st_gen_mipmap.h"
#include "st_program.h"
#include "st_texture.h"
#include "st_cb_drawpixels.h"
#include "st_cb_texture.h"



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

   st_translate_fragment_program(ctx->st, stfp, NULL);

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
   struct pipe_depth_stencil_alpha_state depthstencil;

   /* we don't use blending, but need to set valid values */
   memset(&blend, 0, sizeof(blend));
   blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.colormask = PIPE_MASK_RGBA;
   st->gen_mipmap.blend = blend;
   st->gen_mipmap.blend_cso = pipe->create_blend_state(pipe, &blend);

   memset(&depthstencil, 0, sizeof(depthstencil));
   st->gen_mipmap.depthstencil_cso = pipe->create_depth_stencil_alpha_state(pipe, &depthstencil);

   /* Note: we're assuming zero is valid for all non-specified fields */
   memset(&rasterizer, 0, sizeof(rasterizer));
   rasterizer.front_winding = PIPE_WINDING_CW;
   rasterizer.cull_mode = PIPE_WINDING_NONE;
   st->gen_mipmap.rasterizer_cso = pipe->create_rasterizer_state(pipe, &rasterizer);

   st->gen_mipmap.stfp = make_tex_fragment_program(st->ctx);
   st->gen_mipmap.stvp = st_make_passthrough_vertex_shader(st, GL_FALSE);
}


void
st_destroy_generate_mipmpap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;

   pipe->delete_blend_state(pipe, st->gen_mipmap.blend_cso);
   pipe->delete_depth_stencil_alpha_state(pipe, st->gen_mipmap.depthstencil_cso);
   pipe->delete_rasterizer_state(pipe, st->gen_mipmap.rasterizer_cso);

   /* XXX free stfp, stvp */
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
                 GLenum target,
                 struct pipe_texture *pt,
                 uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   struct pipe_sampler_state sampler;
   void *sampler_cso;
   const uint face = _mesa_tex_target_to_face(target), zslice = 0;
   /*const uint first_level_save = pt->first_level;*/
   uint dstLevel;

   assert(target != GL_TEXTURE_3D); /* not done yet */

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, pt->format, PIPE_SURFACE)) {
      return FALSE;
   }

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));
   fb.num_cbufs = 1;

   /* sampler state */
   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.normalized_coords = 1;


   /* bind state */
   cso_set_blend(st->cso_context, &st->gen_mipmap.blend);
   cso_set_depth_stencil_alpha(st->cso_context, &st->gen_mipmap.depthstencil);
   cso_set_rasterizer(st->cso_context, &st->gen_mipmap.rasterizer);

   /* bind shaders */
   pipe->bind_fs_state(pipe, st->gen_mipmap.stfp->driver_shader);
   pipe->bind_vs_state(pipe, st->gen_mipmap.stvp->driver_shader);

   /*
    * XXX for small mipmap levels, it may be faster to use the software
    * fallback path...
    */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;

      /*
       * Setup framebuffer / dest surface
       */
      fb.cbufs[0] = screen->get_tex_surface(screen, pt, face, dstLevel, zslice);
      pipe->set_framebuffer_state(pipe, &fb);

      /*
       * Setup sampler state
       * Note: we should only have to set the min/max LOD clamps to ensure
       * we grab texels from the right mipmap level.  But some hardware
       * has trouble with min clamping so we also setting the lod_bias to
       * try to work around that.
       */
      sampler.min_lod = sampler.max_lod = srcLevel;
      sampler.lod_bias = srcLevel;
      sampler_cso = pipe->create_sampler_state(pipe, &sampler);
      pipe->bind_sampler_states(pipe, 1, &sampler_cso);

      simple_viewport(pipe, pt->width[dstLevel], pt->height[dstLevel]);

      /*
       * Setup src texture, override pt->first_level so we sample from
       * the right mipmap level.
       */
      /*pt->first_level = srcLevel;*/
      pipe->set_sampler_textures(pipe, 1, &pt);

      draw_quad(st->ctx);

      pipe->flush(pipe, PIPE_FLUSH_WAIT);
      pipe->delete_sampler_state(pipe, sampler_cso);
   }

   /* restore first_level */
   /*pt->first_level = first_level_save;*/

   /* restore pipe state */
#if 0
   cso_set_rasterizer(st->cso_context, &st->state.rasterizer);
   cso_set_samplers(st->cso_context, st->state.samplers_list);
   pipe->bind_fs_state(pipe, st->fp->shader_program);
   pipe->bind_vs_state(pipe, st->vp->shader_program);
   pipe->set_sampler_textures(pipe, st->state.num_textures,
                              st->state.sampler_texture);
   pipe->set_viewport_state(pipe, &st->state.viewport);
#else
   /* XXX is this sufficient? */
   st_invalidate_state(st->ctx, _NEW_COLOR | _NEW_TEXTURE);
#endif

   return TRUE;
}


static void
fallback_generate_mipmap(GLcontext *ctx, GLenum target,
                         struct gl_texture_object *texObj)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_winsys *ws = pipe->winsys;
   struct pipe_texture *pt = st_get_texobj_texture(texObj);
   const uint baseLevel = texObj->BaseLevel;
   const uint lastLevel = pt->last_level;
   const uint face = _mesa_tex_target_to_face(target), zslice = 0;
   uint dstLevel;
   GLenum datatype;
   GLuint comps;

   assert(target != GL_TEXTURE_3D); /* not done yet */

   _mesa_format_to_type_and_comps(texObj->Image[face][baseLevel]->TexFormat,
                                  &datatype, &comps);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_surface *srcSurf, *dstSurf;
      const ubyte *srcData;
      ubyte *dstData;

      srcSurf = screen->get_tex_surface(screen, pt, face, srcLevel, zslice);
      dstSurf = screen->get_tex_surface(screen, pt, face, dstLevel, zslice);

      srcData = (ubyte *) ws->buffer_map(ws, srcSurf->buffer,
                                         PIPE_BUFFER_USAGE_CPU_READ)
              + srcSurf->offset;
      dstData = (ubyte *) ws->buffer_map(ws, dstSurf->buffer,
                                         PIPE_BUFFER_USAGE_CPU_WRITE)
              + dstSurf->offset;

      _mesa_generate_mipmap_level(target, datatype, comps,
                   0 /*border*/,
                   pt->width[srcLevel], pt->height[srcLevel], pt->depth[srcLevel],
                   srcSurf->pitch * srcSurf->cpp, /* stride in bytes */
                   srcData,
                   pt->width[dstLevel], pt->height[dstLevel], pt->depth[dstLevel],
                   dstSurf->pitch * dstSurf->cpp, /* stride in bytes */
                   dstData);

      ws->buffer_unmap(ws, srcSurf->buffer);
      ws->buffer_unmap(ws, dstSurf->buffer);

      pipe_surface_reference(&srcSurf, NULL);
      pipe_surface_reference(&dstSurf, NULL);
   }
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

   if (!st_render_mipmap(st, target, pt, baseLevel, lastLevel)) {
      fallback_generate_mipmap(ctx, target, texObj);
   }

   /* Fill in the Mesa gl_texture_image fields */
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
