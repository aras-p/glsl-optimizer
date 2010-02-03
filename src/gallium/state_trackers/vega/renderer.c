/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "renderer.h"

#include "vg_context.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_draw_quad.h"
#include "util/u_format.h"
#include "util/u_simple_shaders.h"
#include "util/u_memory.h"
#include "util/u_rect.h"

#include "cso_cache/cso_context.h"

struct renderer {
   struct pipe_context *pipe;
   struct vg_context *owner;

   struct cso_context *cso;

   void *fs;

   VGfloat vertices[4][2][4];
};

static void setup_shaders(struct renderer *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   /* fragment shader */
   ctx->fs = util_make_fragment_tex_shader(pipe, TGSI_TEXTURE_2D);
}

static struct pipe_buffer *
setup_vertex_data(struct renderer *ctx,
                  float x0, float y0, float x1, float y1, float z)
{
   ctx->vertices[0][0][0] = x0;
   ctx->vertices[0][0][1] = y0;
   ctx->vertices[0][0][2] = z;
   ctx->vertices[0][1][0] = 0.0f; /*s*/
   ctx->vertices[0][1][1] = 0.0f; /*t*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y0;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = 1.0f; /*s*/
   ctx->vertices[1][1][1] = 0.0f; /*t*/

   ctx->vertices[2][0][0] = x1;
   ctx->vertices[2][0][1] = y1;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = 1.0f;
   ctx->vertices[2][1][1] = 1.0f;

   ctx->vertices[3][0][0] = x0;
   ctx->vertices[3][0][1] = y1;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = 0.0f;
   ctx->vertices[3][1][1] = 1.0f;

   return pipe_user_buffer_create( ctx->pipe->screen,
                                   ctx->vertices,
                                   sizeof(ctx->vertices) );
}

static struct pipe_buffer *
setup_vertex_data_tex(struct renderer *ctx,
                      float x0, float y0, float x1, float y1,
                      float s0, float t0, float s1, float t1,
                      float z)
{
   ctx->vertices[0][0][0] = x0;
   ctx->vertices[0][0][1] = y0;
   ctx->vertices[0][0][2] = z;
   ctx->vertices[0][1][0] = s0; /*s*/
   ctx->vertices[0][1][1] = t0; /*t*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y0;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = s1; /*s*/
   ctx->vertices[1][1][1] = t0; /*t*/

   ctx->vertices[2][0][0] = x1;
   ctx->vertices[2][0][1] = y1;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = s1;
   ctx->vertices[2][1][1] = t1;

   ctx->vertices[3][0][0] = x0;
   ctx->vertices[3][0][1] = y1;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = s0;
   ctx->vertices[3][1][1] = t1;

   return pipe_user_buffer_create( ctx->pipe->screen,
                                   ctx->vertices,
                                   sizeof(ctx->vertices) );
}


static struct pipe_buffer *
setup_vertex_data_qtex(struct renderer *ctx,
                       float x0, float y0, float x1, float y1,
                       float x2, float y2, float x3, float y3,
                       float s0, float t0, float s1, float t1,
                       float z)
{
   ctx->vertices[0][0][0] = x0;
   ctx->vertices[0][0][1] = y0;
   ctx->vertices[0][0][2] = z;
   ctx->vertices[0][1][0] = s0; /*s*/
   ctx->vertices[0][1][1] = t0; /*t*/

   ctx->vertices[1][0][0] = x1;
   ctx->vertices[1][0][1] = y1;
   ctx->vertices[1][0][2] = z;
   ctx->vertices[1][1][0] = s1; /*s*/
   ctx->vertices[1][1][1] = t0; /*t*/

   ctx->vertices[2][0][0] = x2;
   ctx->vertices[2][0][1] = y2;
   ctx->vertices[2][0][2] = z;
   ctx->vertices[2][1][0] = s1;
   ctx->vertices[2][1][1] = t1;

   ctx->vertices[3][0][0] = x3;
   ctx->vertices[3][0][1] = y3;
   ctx->vertices[3][0][2] = z;
   ctx->vertices[3][1][0] = s0;
   ctx->vertices[3][1][1] = t1;

   return pipe_user_buffer_create( ctx->pipe->screen,
                                   ctx->vertices,
                                   sizeof(ctx->vertices) );
}

struct renderer * renderer_create(struct vg_context *owner)
{
   VGint i;
   struct renderer *renderer = CALLOC_STRUCT(renderer);

   if (!renderer)
      return NULL;

   renderer->owner = owner;
   renderer->pipe = owner->pipe;
   renderer->cso = owner->cso_context;

   setup_shaders(renderer);

   /* init vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      renderer->vertices[i][0][3] = 1.0f; /* w */
      renderer->vertices[i][1][2] = 0.0f; /* r */
      renderer->vertices[i][1][3] = 1.0f; /* q */
   }

   return renderer;
}

void renderer_destroy(struct renderer *ctx)
{
#if 0
   if (ctx->fs) {
      cso_delete_fragment_shader(ctx->cso, ctx->fs);
      ctx->fs = NULL;
   }
#endif
   free(ctx);
}

void renderer_draw_quad(struct renderer *r,
                        VGfloat x1, VGfloat y1,
                        VGfloat x2, VGfloat y2,
                        VGfloat depth)
{
   struct pipe_buffer *buf;

   buf = setup_vertex_data(r, x1, y1, x2, y2, depth);

   if (buf) {
      util_draw_vertex_buffer(r->pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference( &buf,
                             NULL );
   }
}

void renderer_draw_texture(struct renderer *r,
                           struct pipe_texture *tex,
                           VGfloat x1offset, VGfloat y1offset,
                           VGfloat x2offset, VGfloat y2offset,
                           VGfloat x1, VGfloat y1,
                           VGfloat x2, VGfloat y2)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_buffer *buf;
   VGfloat s0, t0, s1, t1;

   assert(tex->width0 != 0);
   assert(tex->height0 != 0);

   s0 = x1offset / tex->width0;
   s1 = x2offset / tex->width0;
   t0 = y1offset / tex->height0;
   t1 = y2offset / tex->height0;

   cso_save_vertex_shader(r->cso);
   /* shaders */
   cso_set_vertex_shader_handle(r->cso, vg_texture_vs(r->owner));

   /* draw quad */
   buf = setup_vertex_data_tex(r, x1, y1, x2, y2,
                               s0, t0, s1, t1, 0.0f);

   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

      pipe_buffer_reference( &buf,
                             NULL );
   }

   cso_restore_vertex_shader(r->cso);
}

void renderer_copy_texture(struct renderer *ctx,
                           struct pipe_texture *src,
                           VGfloat sx1, VGfloat sy1,
                           VGfloat sx2, VGfloat sy2,
                           struct pipe_texture *dst,
                           VGfloat dx1, VGfloat dy1,
                           VGfloat dx2, VGfloat dy2)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_buffer *buf;
   struct pipe_surface *dst_surf = screen->get_tex_surface(
      screen, dst, 0, 0, 0,
      PIPE_BUFFER_USAGE_GPU_WRITE);
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;

   assert(src->width0 != 0);
   assert(src->height0 != 0);
   assert(dst->width0 != 0);
   assert(dst->height0 != 0);

#if 0
   debug_printf("copy texture [%f, %f, %f, %f], [%f, %f, %f, %f]\n",
                sx1, sy1, sx2, sy2, dx1, dy1, dx2, dy2);
#endif

#if 1
   s0 = sx1 / src->width0;
   s1 = sx2 / src->width0;
   t0 = sy1 / src->height0;
   t1 = sy2 / src->height0;
#else
   s0 = 0;
   s1 = 1;
   t0 = 0;
   t1 = 1;
#endif

   assert(screen->is_format_supported(screen, dst_surf->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0));

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);

   cso_save_viewport(ctx->cso);


   /* set misc state we care about */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].colormask = PIPE_MASK_RGBA;
      cso_set_blend(ctx->cso, &blend);
   }

   /* sampler */
   {
      struct pipe_sampler_state sampler;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.normalized_coords = 1;
      cso_single_sampler(ctx->cso, 0, &sampler);
      cso_single_sampler_done(ctx->cso);
   }

   vg_set_viewport(ctx->owner, VEGA_Y0_TOP);

   /* texture */
   cso_set_sampler_textures(ctx->cso, 1, &src);

   /* shaders */
   cso_set_vertex_shader_handle(ctx->cso, vg_texture_vs(ctx->owner));
   cso_set_fragment_shader_handle(ctx->cso, ctx->fs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst_surf->width;
   fb.height = dst_surf->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst_surf;
   {
      VGint i;
      for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
         fb.cbufs[i] = 0;
   }
   cso_set_framebuffer(ctx->cso, &fb);

   /* draw quad */
   buf = setup_vertex_data_tex(ctx,
                         dx1, dy1,
                         dx2, dy2,
                         s0, t0, s1, t1,
                         0.0f);

   if (buf) {
      util_draw_vertex_buffer(ctx->pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference( &buf,
                             NULL );
   }

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_sampler_textures(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);

   pipe_surface_reference(&dst_surf, NULL);
}

void renderer_copy_surface(struct renderer *ctx,
                           struct pipe_surface *src,
                           int srcX0, int srcY0,
                           int srcX1, int srcY1,
                           struct pipe_surface *dst,
                           int dstX0, int dstY0,
                           int dstX1, int dstY1,
                           float z, unsigned filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_buffer *buf;
   struct pipe_texture texTemp, *tex;
   struct pipe_surface *texSurf;
   struct pipe_framebuffer_state fb;
   struct st_framebuffer *stfb = ctx->owner->draw_buffer;
   const int srcW = abs(srcX1 - srcX0);
   const int srcH = abs(srcY1 - srcY0);
   const int srcLeft = MIN2(srcX0, srcX1);
   const int srcTop = MIN2(srcY0, srcY1);

   assert(filter == PIPE_TEX_MIPFILTER_NEAREST ||
          filter == PIPE_TEX_MIPFILTER_LINEAR);

   if (srcLeft != srcX0) {
      /* left-right flip */
      int tmp = dstX0;
      dstX0 = dstX1;
      dstX1 = tmp;
   }

   if (srcTop != srcY0) {
      /* up-down flip */
      int tmp = dstY0;
      dstY0 = dstY1;
      dstY1 = tmp;
   }

   assert(screen->is_format_supported(screen, src->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_SAMPLER, 0));
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_SAMPLER, 0));
   assert(screen->is_format_supported(screen, dst->format, PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0));

   /*
    * XXX for now we're always creating a temporary texture.
    * Strictly speaking that's not always needed.
    */

   /* create temp texture */
   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = src->format;
   texTemp.last_level = 0;
   texTemp.width0 = srcW;
   texTemp.height0 = srcH;
   texTemp.depth0 = 1;

   tex = screen->texture_create(screen, &texTemp);
   if (!tex)
      return;

   texSurf = screen->get_tex_surface(screen, tex, 0, 0, 0,
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

   /* load temp texture */
   if (pipe->surface_copy) {
      pipe->surface_copy(pipe,
                         texSurf, 0, 0,   /* dest */
                         src, srcLeft, srcTop, /* src */
                         srcW, srcH);     /* size */
   } else {
      util_surface_copy(pipe, FALSE,
                        texSurf, 0, 0,   /* dest */
                        src, srcLeft, srcTop, /* src */
                        srcW, srcH);     /* size */
   }

   /* free the surface, update the texture if necessary.*/
   screen->tex_surface_destroy(texSurf);

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_viewport(ctx->cso);

   /* set misc state we care about */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].colormask = PIPE_MASK_RGBA;
      cso_set_blend(ctx->cso, &blend);
   }

   vg_set_viewport(ctx->owner, VEGA_Y0_TOP);

   /* sampler */
   {
      struct pipe_sampler_state sampler;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.normalized_coords = 1;
      cso_single_sampler(ctx->cso, 0, &sampler);
      cso_single_sampler_done(ctx->cso);
   }

   /* texture */
   cso_set_sampler_textures(ctx->cso, 1, &tex);

   /* shaders */
   cso_set_fragment_shader_handle(ctx->cso, ctx->fs);
   cso_set_vertex_shader_handle(ctx->cso, vg_texture_vs(ctx->owner));

   /* drawing dest */
   if (stfb->strb->surface != dst) {
      memset(&fb, 0, sizeof(fb));
      fb.width = dst->width;
      fb.height = dst->height;
      fb.nr_cbufs = 1;
      fb.cbufs[0] = dst;
      fb.zsbuf = stfb->dsrb->surface;
      cso_set_framebuffer(ctx->cso, &fb);
   }

   /* draw quad */
   buf = setup_vertex_data(ctx,
                           (float) dstX0, (float) dstY0,
                           (float) dstX1, (float) dstY1, z);

   if (buf) {
      util_draw_vertex_buffer(ctx->pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference( &buf,
                             NULL );
   }


   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_sampler_textures(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);

   pipe_texture_reference(&tex, NULL);
}

void renderer_texture_quad(struct renderer *r,
                           struct pipe_texture *tex,
                           VGfloat x1offset, VGfloat y1offset,
                           VGfloat x2offset, VGfloat y2offset,
                           VGfloat x1, VGfloat y1,
                           VGfloat x2, VGfloat y2,
                           VGfloat x3, VGfloat y3,
                           VGfloat x4, VGfloat y4)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_buffer *buf;
   VGfloat s0, t0, s1, t1;

   assert(tex->width0 != 0);
   assert(tex->height0 != 0);

   s0 = x1offset / tex->width0;
   s1 = x2offset / tex->width0;
   t0 = y1offset / tex->height0;
   t1 = y2offset / tex->height0;

   cso_save_vertex_shader(r->cso);
   /* shaders */
   cso_set_vertex_shader_handle(r->cso, vg_texture_vs(r->owner));

   /* draw quad */
   buf = setup_vertex_data_qtex(r, x1, y1, x2, y2, x3, y3, x4, y4,
                          s0, t0, s1, t1, 0.0f);

   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference(&buf,
                            NULL);
   }

   cso_restore_vertex_shader(r->cso);
}
