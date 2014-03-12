/**************************************************************************
 *
 * Copyright 2008 VMware, Inc.
 * All Rights Reserved.
 * Copyright 2008  VMware, Inc.  All rights reserved.
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

/**
 * @file
 * Mipmap generation utility
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_draw_quad.h"
#include "util/u_gen_mipmap.h"
#include "util/u_simple_shaders.h"
#include "util/u_math.h"
#include "util/u_texture.h"
#include "util/u_half.h"
#include "util/u_surface.h"

#include "cso_cache/cso_context.h"


struct gen_mipmap_state
{
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend_keep_color, blend_write_color;
   struct pipe_depth_stencil_alpha_state dsa_keep_depth, dsa_write_depth;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_vertex_element velem[2];

   void *vs;

   /** Not all are used, but simplifies code */
   void *fs_color[TGSI_TEXTURE_COUNT];
   void *fs_depth[TGSI_TEXTURE_COUNT];

   struct pipe_resource *vbuf;  /**< quad vertices */
   unsigned vbuf_slot;

   float vertices[4][2][4];   /**< vertex/texcoords for quad */
};


/**
 * Create a mipmap generation context.
 * The idea is to create one of these and re-use it each time we need to
 * generate a mipmap.
 */
struct gen_mipmap_state *
util_create_gen_mipmap(struct pipe_context *pipe,
                       struct cso_context *cso)
{
   struct gen_mipmap_state *ctx;
   uint i;

   ctx = CALLOC_STRUCT(gen_mipmap_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;
   ctx->cso = cso;

   /* disabled blending/masking */
   memset(&ctx->blend_keep_color, 0, sizeof(ctx->blend_keep_color));
   memset(&ctx->blend_write_color, 0, sizeof(ctx->blend_write_color));
   ctx->blend_write_color.rt[0].colormask = PIPE_MASK_RGBA;

   /* no-op depth/stencil/alpha */
   memset(&ctx->dsa_keep_depth, 0, sizeof(ctx->dsa_keep_depth));
   memset(&ctx->dsa_write_depth, 0, sizeof(ctx->dsa_write_depth));
   ctx->dsa_write_depth.depth.enabled = 1;
   ctx->dsa_write_depth.depth.func = PIPE_FUNC_ALWAYS;
   ctx->dsa_write_depth.depth.writemask = 1;

   /* rasterizer */
   memset(&ctx->rasterizer, 0, sizeof(ctx->rasterizer));
   ctx->rasterizer.cull_face = PIPE_FACE_NONE;
   ctx->rasterizer.half_pixel_center = 1;
   ctx->rasterizer.bottom_edge_rule = 1;
   ctx->rasterizer.depth_clip = 1;

   /* sampler state */
   memset(&ctx->sampler, 0, sizeof(ctx->sampler));
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   ctx->sampler.normalized_coords = 1;

   /* vertex elements state */
   memset(&ctx->velem[0], 0, sizeof(ctx->velem[0]) * 2);
   for (i = 0; i < 2; i++) {
      ctx->velem[i].src_offset = i * 4 * sizeof(float);
      ctx->velem[i].instance_divisor = 0;
      ctx->velem[i].vertex_buffer_index = cso_get_aux_vertex_buffer_slot(cso);
      ctx->velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }

   /* vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][2] = 0.0f; /* z */
      ctx->vertices[i][0][3] = 1.0f; /* w */
      ctx->vertices[i][1][3] = 1.0f; /* q */
   }

   /* Note: the actual vertex buffer is allocated as needed below */

   return ctx;
}


/**
 * Helper function to set the fragment shaders.
 */
static INLINE void
set_fragment_shader(struct gen_mipmap_state *ctx, uint type,
                    boolean output_depth)
{
   if (output_depth) {
      if (!ctx->fs_depth[type])
         ctx->fs_depth[type] =
            util_make_fragment_tex_shader_writedepth(ctx->pipe, type,
                                                     TGSI_INTERPOLATE_LINEAR);

      cso_set_fragment_shader_handle(ctx->cso, ctx->fs_depth[type]);
   }
   else {
      if (!ctx->fs_color[type])
         ctx->fs_color[type] =
            util_make_fragment_tex_shader(ctx->pipe, type,
                                          TGSI_INTERPOLATE_LINEAR);

      cso_set_fragment_shader_handle(ctx->cso, ctx->fs_color[type]);
   }
}


/**
 * Helper function to set the vertex shader.
 */
static INLINE void
set_vertex_shader(struct gen_mipmap_state *ctx)
{
   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   if (!ctx->vs)
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(ctx->pipe, 2,
                                                    semantic_names,
                                                    semantic_indexes);
   }

   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);
}


/**
 * Get next "slot" of vertex space in the vertex buffer.
 * We're allocating one large vertex buffer and using it piece by piece.
 */
static unsigned
get_next_slot(struct gen_mipmap_state *ctx)
{
   const unsigned max_slots = 4096 / sizeof ctx->vertices;

   if (ctx->vbuf_slot >= max_slots) {
      pipe_resource_reference(&ctx->vbuf, NULL);
      ctx->vbuf_slot = 0;
   }

   if (!ctx->vbuf) {
      ctx->vbuf = pipe_buffer_create(ctx->pipe->screen,
                                     PIPE_BIND_VERTEX_BUFFER,
                                     PIPE_USAGE_STREAM,
                                     max_slots * sizeof ctx->vertices);
   }
   
   return ctx->vbuf_slot++ * sizeof ctx->vertices;
}


static unsigned
set_vertex_data(struct gen_mipmap_state *ctx,
                enum pipe_texture_target tex_target,
                uint face, float r)
{
   unsigned offset;

   /* vert[0].position */
   ctx->vertices[0][0][0] = -1.0f; /*x*/
   ctx->vertices[0][0][1] = -1.0f; /*y*/

   /* vert[1].position */
   ctx->vertices[1][0][0] = 1.0f;
   ctx->vertices[1][0][1] = -1.0f;

   /* vert[2].position */
   ctx->vertices[2][0][0] = 1.0f;
   ctx->vertices[2][0][1] = 1.0f;

   /* vert[3].position */
   ctx->vertices[3][0][0] = -1.0f;
   ctx->vertices[3][0][1] = 1.0f;

   /* Setup vertex texcoords.  This is a little tricky for cube maps. */
   if (tex_target == PIPE_TEXTURE_CUBE ||
       tex_target == PIPE_TEXTURE_CUBE_ARRAY) {
      static const float st[4][2] = {
         {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
      };

      util_map_texcoords2d_onto_cubemap(face, &st[0][0], 2,
                                        &ctx->vertices[0][1][0], 8,
                                        FALSE);

      /* set the layer for cube arrays */
      ctx->vertices[0][1][3] = r;
      ctx->vertices[1][1][3] = r;
      ctx->vertices[2][1][3] = r;
      ctx->vertices[3][1][3] = r;
   }
   else if (tex_target == PIPE_TEXTURE_1D_ARRAY) {
      /* 1D texture array  */
      ctx->vertices[0][1][0] = 0.0f; /*s*/
      ctx->vertices[0][1][1] = r; /*t*/
      ctx->vertices[0][1][2] = 0.0f;    /*r*/

      ctx->vertices[1][1][0] = 1.0f;
      ctx->vertices[1][1][1] = r;
      ctx->vertices[1][1][2] = 0.0f;

      ctx->vertices[2][1][0] = 1.0f;
      ctx->vertices[2][1][1] = r;
      ctx->vertices[2][1][2] = 0.0f;

      ctx->vertices[3][1][0] = 0.0f;
      ctx->vertices[3][1][1] = r;
      ctx->vertices[3][1][2] = 0.0f;
   } else {
      /* 1D/2D/3D/2D array */
      ctx->vertices[0][1][0] = 0.0f; /*s*/
      ctx->vertices[0][1][1] = 0.0f; /*t*/
      ctx->vertices[0][1][2] = r;    /*r*/

      ctx->vertices[1][1][0] = 1.0f;
      ctx->vertices[1][1][1] = 0.0f;
      ctx->vertices[1][1][2] = r;

      ctx->vertices[2][1][0] = 1.0f;
      ctx->vertices[2][1][1] = 1.0f;
      ctx->vertices[2][1][2] = r;

      ctx->vertices[3][1][0] = 0.0f;
      ctx->vertices[3][1][1] = 1.0f;
      ctx->vertices[3][1][2] = r;
   }

   offset = get_next_slot( ctx );

   pipe_buffer_write_nooverlap(ctx->pipe, ctx->vbuf,
                               offset, sizeof(ctx->vertices), ctx->vertices);

   return offset;
}



/**
 * Destroy a mipmap generation context
 */
void
util_destroy_gen_mipmap(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   unsigned i;

   for (i = 0; i < Elements(ctx->fs_color); i++)
      if (ctx->fs_color[i])
         pipe->delete_fs_state(pipe, ctx->fs_color[i]);

   for (i = 0; i < Elements(ctx->fs_depth); i++)
      if (ctx->fs_depth[i])
         pipe->delete_fs_state(pipe, ctx->fs_depth[i]);

   if (ctx->vs)
      pipe->delete_vs_state(pipe, ctx->vs);

   pipe_resource_reference(&ctx->vbuf, NULL);

   FREE(ctx);
}


/**
 * Generate mipmap images.  It's assumed all needed texture memory is
 * already allocated.
 *
 * \param psv  the sampler view to the texture to generate mipmap levels for
 * \param face  which cube face to generate mipmaps for (0 for non-cube maps)
 * \param baseLevel  the first mipmap level to use as a src
 * \param lastLevel  the last mipmap level to generate
 * \param filter  the minification filter used to generate mipmap levels with
 * \param filter  one of PIPE_TEX_FILTER_LINEAR, PIPE_TEX_FILTER_NEAREST
 */
void
util_gen_mipmap(struct gen_mipmap_state *ctx,
                struct pipe_sampler_view *psv,
                uint face, uint baseLevel, uint lastLevel, uint filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   struct pipe_resource *pt = psv->texture;
   uint dstLevel;
   uint offset;
   uint type;
   boolean is_depth = util_format_is_depth_or_stencil(psv->format);

   /* The texture object should have room for the levels which we're
    * about to generate.
    */
   assert(lastLevel <= pt->last_level);

   /* If this fails, why are we here? */
   assert(lastLevel > baseLevel);

   assert(filter == PIPE_TEX_FILTER_LINEAR ||
          filter == PIPE_TEX_FILTER_NEAREST);

   type = util_pipe_tex_to_tgsi_tex(pt->target, 1);

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, psv->format, pt->target,
                                    pt->nr_samples,
                                    is_depth ? PIPE_BIND_DEPTH_STENCIL :
                                               PIPE_BIND_RENDER_TARGET)) {
      /* The caller should check if the format is renderable. */
      assert(0);
      return;
   }

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_sample_mask(ctx->cso);
   cso_save_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_save_stream_outputs(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_geometry_shader(ctx->cso);
   cso_save_viewport(ctx->cso);
   cso_save_vertex_elements(ctx->cso);
   cso_save_aux_vertex_buffer_slot(ctx->cso);
   cso_save_render_condition(ctx->cso);

   /* bind our state */
   cso_set_blend(ctx->cso, is_depth ? &ctx->blend_keep_color :
                                      &ctx->blend_write_color);
   cso_set_depth_stencil_alpha(ctx->cso, is_depth ? &ctx->dsa_write_depth :
                                                    &ctx->dsa_keep_depth);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_sample_mask(ctx->cso, ~0);
   cso_set_vertex_elements(ctx->cso, 2, ctx->velem);
   cso_set_stream_outputs(ctx->cso, 0, NULL, NULL);
   cso_set_render_condition(ctx->cso, NULL, FALSE, 0);

   set_fragment_shader(ctx, type, is_depth);
   set_vertex_shader(ctx);
   cso_set_geometry_shader_handle(ctx->cso, NULL);

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));

   /* set min/mag to same filter for faster sw speed */
   ctx->sampler.mag_img_filter = filter;
   ctx->sampler.min_img_filter = filter;

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_viewport_state vp;
      unsigned nr_layers, layer, i;
      float rcoord = 0.0f;

      if (pt->target == PIPE_TEXTURE_3D)
         nr_layers = u_minify(pt->depth0, dstLevel);
      else if (pt->target == PIPE_TEXTURE_2D_ARRAY ||
               pt->target == PIPE_TEXTURE_1D_ARRAY ||
               pt->target == PIPE_TEXTURE_CUBE_ARRAY)
	 nr_layers = pt->array_size;
      else
         nr_layers = 1;

      for (i = 0; i < nr_layers; i++) {
         struct pipe_surface *surf, surf_templ;
         if (pt->target == PIPE_TEXTURE_3D) {
            /* in theory with geom shaders and driver with full layer support
               could do that in one go. */
            layer = i;
            /* XXX hmm really? */
            rcoord = (float)layer / (float)nr_layers + 1.0f / (float)(nr_layers * 2);
         } else if (pt->target == PIPE_TEXTURE_2D_ARRAY ||
                    pt->target == PIPE_TEXTURE_1D_ARRAY) {
	    layer = i;
	    rcoord = (float)layer;
         } else if (pt->target == PIPE_TEXTURE_CUBE_ARRAY) {
            layer = i;
            face = layer % 6;
            rcoord = layer / 6;
	 } else
            layer = face;

         u_surface_default_template(&surf_templ, pt);
         surf_templ.u.tex.level = dstLevel;
         surf_templ.u.tex.first_layer = layer;
         surf_templ.u.tex.last_layer = layer;
         surf = pipe->create_surface(pipe, pt, &surf_templ);

         /*
          * Setup framebuffer / dest surface
          */
         if (is_depth) {
            fb.nr_cbufs = 0;
            fb.zsbuf = surf;
         }
         else {
            fb.nr_cbufs = 1;
            fb.cbufs[0] = surf;
         }
         fb.width = u_minify(pt->width0, dstLevel);
         fb.height = u_minify(pt->height0, dstLevel);
         cso_set_framebuffer(ctx->cso, &fb);

         /* viewport */
         vp.scale[0] = 0.5f * fb.width;
         vp.scale[1] = 0.5f * fb.height;
         vp.scale[2] = 1.0f;
         vp.scale[3] = 1.0f;
         vp.translate[0] = 0.5f * fb.width;
         vp.translate[1] = 0.5f * fb.height;
         vp.translate[2] = 0.0f;
         vp.translate[3] = 0.0f;
         cso_set_viewport(ctx->cso, &vp);

         /*
          * Setup sampler state
          * Note: we should only have to set the min/max LOD clamps to ensure
          * we grab texels from the right mipmap level.  But some hardware
          * has trouble with min clamping so we also set the lod_bias to
          * try to work around that.
          */
         ctx->sampler.min_lod = ctx->sampler.max_lod = (float) srcLevel;
         ctx->sampler.lod_bias = (float) srcLevel;
         cso_single_sampler(ctx->cso, PIPE_SHADER_FRAGMENT, 0, &ctx->sampler);
         cso_single_sampler_done(ctx->cso, PIPE_SHADER_FRAGMENT);

         cso_set_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT, 1, &psv);

         /* quad coords in clip coords */
         offset = set_vertex_data(ctx,
                                  pt->target,
                                  face,
                                  rcoord);

         util_draw_vertex_buffer(ctx->pipe,
                                 ctx->cso,
                                 ctx->vbuf,
                                 cso_get_aux_vertex_buffer_slot(ctx->cso),
                                 offset,
                                 PIPE_PRIM_TRIANGLE_FAN,
                                 4,  /* verts */
                                 2); /* attribs/vert */

         /* need to signal that the texture has changed _after_ rendering to it */
         pipe_surface_reference( &surf, NULL );
      }
   }

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_sample_mask(ctx->cso);
   cso_restore_samplers(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_sampler_views(ctx->cso, PIPE_SHADER_FRAGMENT);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_geometry_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);
   cso_restore_vertex_elements(ctx->cso);
   cso_restore_stream_outputs(ctx->cso);
   cso_restore_aux_vertex_buffer_slot(ctx->cso);
   cso_restore_render_condition(ctx->cso);
}
