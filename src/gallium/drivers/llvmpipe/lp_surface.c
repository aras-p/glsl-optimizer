/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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

#include "util/u_rect.h"
#include "util/u_surface.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_limits.h"
#include "lp_surface.h"
#include "lp_texture.h"
#include "lp_query.h"


/**
 * Adjust x, y, width, height to lie on tile bounds.
 */
static void
adjust_to_tile_bounds(unsigned x, unsigned y, unsigned width, unsigned height,
                      unsigned *x_tile, unsigned *y_tile,
                      unsigned *w_tile, unsigned *h_tile)
{
   *x_tile = x & ~(TILE_SIZE - 1);
   *y_tile = y & ~(TILE_SIZE - 1);
   *w_tile = ((x + width + TILE_SIZE - 1) & ~(TILE_SIZE - 1)) - *x_tile;
   *h_tile = ((y + height + TILE_SIZE - 1) & ~(TILE_SIZE - 1)) - *y_tile;
}



static void
lp_resource_copy(struct pipe_context *pipe,
                 struct pipe_resource *dst, unsigned dst_level,
                 unsigned dstx, unsigned dsty, unsigned dstz,
                 struct pipe_resource *src, unsigned src_level,
                 const struct pipe_box *src_box)
{
   struct llvmpipe_resource *src_tex = llvmpipe_resource(src);
   struct llvmpipe_resource *dst_tex = llvmpipe_resource(dst);
   const enum pipe_format format = src_tex->base.format;
   unsigned width = src_box->width;
   unsigned height = src_box->height;
   unsigned depth = src_box->depth;
   unsigned z;

   llvmpipe_flush_resource(pipe,
                           dst, dst_level,
                           FALSE, /* read_only */
                           TRUE, /* cpu_access */
                           FALSE, /* do_not_block */
                           "blit dest");

   llvmpipe_flush_resource(pipe,
                           src, src_level,
                           TRUE, /* read_only */
                           TRUE, /* cpu_access */
                           FALSE, /* do_not_block */
                           "blit src");

   /* Fallback for buffers. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                                src, src_level, src_box);
      return;
   }

   /*
   printf("surface copy from %u lvl %u to %u lvl %u: %u,%u,%u to %u,%u,%u %u x %u x %u\n",
          src_tex->id, src_level, dst_tex->id, dst_level,
          src_box->x, src_box->y, src_box->z, dstx, dsty, dstz,
          src_box->width, src_box->height, src_box->depth);
   */

   for (z = 0; z < src_box->depth; z++){

      /* set src tiles to linear layout */
      {
         unsigned tx, ty, tw, th;
         unsigned x, y;

         adjust_to_tile_bounds(src_box->x, src_box->y, width, height,
                               &tx, &ty, &tw, &th);

         for (y = 0; y < th; y += TILE_SIZE) {
            for (x = 0; x < tw; x += TILE_SIZE) {
               (void) llvmpipe_get_texture_tile_linear(src_tex,
                                                       src_box->z + z, src_level,
                                                       LP_TEX_USAGE_READ,
                                                       tx + x, ty + y);
            }
         }
      }

      /* set dst tiles to linear layout */
      {
         unsigned tx, ty, tw, th;
         unsigned x, y;
         enum lp_texture_usage usage;

         adjust_to_tile_bounds(dstx, dsty, width, height, &tx, &ty, &tw, &th);

         for (y = 0; y < th; y += TILE_SIZE) {
            boolean contained_y = ty + y >= dsty &&
                                  ty + y + TILE_SIZE <= dsty + height ?
                                  TRUE : FALSE;

            for (x = 0; x < tw; x += TILE_SIZE) {
               boolean contained_x = tx + x >= dstx &&
                                     tx + x + TILE_SIZE <= dstx + width ?
                                     TRUE : FALSE;

               /*
                * Set the usage mode to WRITE_ALL for the tiles which are
                * completely contained by the dest rectangle.
                */
               if (contained_y && contained_x)
                  usage = LP_TEX_USAGE_WRITE_ALL;
               else
                  usage = LP_TEX_USAGE_READ_WRITE;

               (void) llvmpipe_get_texture_tile_linear(dst_tex,
                                                       dstz + z, dst_level,
                                                       usage,
                                                       tx + x, ty + y);
            }
         }
      }
   }

   /* copy */
   {
      const ubyte *src_linear_ptr
         = llvmpipe_get_texture_image_address(src_tex, src_box->z,
                                              src_level);
      ubyte *dst_linear_ptr
         = llvmpipe_get_texture_image_address(dst_tex, dstz,
                                              dst_level);

      if (dst_linear_ptr && src_linear_ptr) {
         util_copy_box(dst_linear_ptr, format,
                       llvmpipe_resource_stride(&dst_tex->base, dst_level),
                       dst_tex->img_stride[dst_level],
                       dstx, dsty, 0,
                       width, height, depth,
                       src_linear_ptr,
                       llvmpipe_resource_stride(&src_tex->base, src_level),
                       src_tex->img_stride[src_level],
                       src_box->x, src_box->y, 0);
      }
   }
}


static void lp_blit(struct pipe_context *pipe,
                    const struct pipe_blit_info *blit_info)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct pipe_blit_info info = *blit_info;

   if (info.src.resource->nr_samples > 1 &&
       info.dst.resource->nr_samples <= 1 &&
       !util_format_is_depth_or_stencil(info.src.resource->format) &&
       !util_format_is_pure_integer(info.src.resource->format)) {
      debug_printf("llvmpipe: color resolve unimplemented\n");
      return;
   }

   if (util_try_blit_via_copy_region(pipe, &info)) {
      return; /* done */
   }

   if (info.mask & PIPE_MASK_S) {
      debug_printf("llvmpipe: cannot blit stencil, skipping\n");
      info.mask &= ~PIPE_MASK_S;
   }

   if (!util_blitter_is_blit_supported(lp->blitter, &info)) {
      debug_printf("llvmpipe: blit unsupported %s -> %s\n",
                   util_format_short_name(info.src.resource->format),
                   util_format_short_name(info.dst.resource->format));
      return;
   }

   /* XXX turn off occlusion and streamout queries */

   util_blitter_save_vertex_buffer_slot(lp->blitter, lp->vertex_buffer);
   util_blitter_save_vertex_elements(lp->blitter, (void*)lp->velems);
   util_blitter_save_vertex_shader(lp->blitter, (void*)lp->vs);
   util_blitter_save_geometry_shader(lp->blitter, (void*)lp->gs);
   util_blitter_save_so_targets(lp->blitter, lp->num_so_targets,
                                (struct pipe_stream_output_target**)lp->so_targets);
   util_blitter_save_rasterizer(lp->blitter, (void*)lp->rasterizer);
   util_blitter_save_viewport(lp->blitter, &lp->viewports[0]);
   util_blitter_save_scissor(lp->blitter, &lp->scissors[0]);
   util_blitter_save_fragment_shader(lp->blitter, lp->fs);
   util_blitter_save_blend(lp->blitter, (void*)lp->blend);
   util_blitter_save_depth_stencil_alpha(lp->blitter, (void*)lp->depth_stencil);
   util_blitter_save_stencil_ref(lp->blitter, &lp->stencil_ref);
   /*util_blitter_save_sample_mask(sp->blitter, lp->sample_mask);*/
   util_blitter_save_framebuffer(lp->blitter, &lp->framebuffer);
   util_blitter_save_fragment_sampler_states(lp->blitter,
                     lp->num_samplers[PIPE_SHADER_FRAGMENT],
                     (void**)lp->samplers[PIPE_SHADER_FRAGMENT]);
   util_blitter_save_fragment_sampler_views(lp->blitter,
                     lp->num_sampler_views[PIPE_SHADER_FRAGMENT],
                     lp->sampler_views[PIPE_SHADER_FRAGMENT]);
   util_blitter_save_render_condition(lp->blitter, lp->render_cond_query,
                                      lp->render_cond_cond, lp->render_cond_mode);
   util_blitter_blit(lp->blitter, &info);
}


static void
lp_flush_resource(struct pipe_context *ctx, struct pipe_resource *resource)
{
}


static struct pipe_surface *
llvmpipe_create_surface(struct pipe_context *pipe,
                        struct pipe_resource *pt,
                        const struct pipe_surface *surf_tmpl)
{
   struct pipe_surface *ps;

   if (!(pt->bind & (PIPE_BIND_DEPTH_STENCIL | PIPE_BIND_RENDER_TARGET)))
      debug_printf("Illegal surface creation without bind flag\n");

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->context = pipe;
      ps->format = surf_tmpl->format;
      if (llvmpipe_resource_is_texture(pt)) {
         assert(surf_tmpl->u.tex.level <= pt->last_level);
         assert(surf_tmpl->u.tex.first_layer <= surf_tmpl->u.tex.last_layer);
         ps->width = u_minify(pt->width0, surf_tmpl->u.tex.level);
         ps->height = u_minify(pt->height0, surf_tmpl->u.tex.level);
         ps->u.tex.level = surf_tmpl->u.tex.level;
         ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
         ps->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
      }
      else {
         /* setting width as number of elements should get us correct renderbuffer width */
         ps->width = surf_tmpl->u.buf.last_element - surf_tmpl->u.buf.first_element + 1;
         ps->height = pt->height0;
         ps->u.buf.first_element = surf_tmpl->u.buf.first_element;
         ps->u.buf.last_element = surf_tmpl->u.buf.last_element;
         assert(ps->u.buf.first_element <= ps->u.buf.last_element);
         assert(util_format_get_blocksize(surf_tmpl->format) *
                (ps->u.buf.last_element + 1) <= pt->width0);
      }
   }
   return ps;
}


static void
llvmpipe_surface_destroy(struct pipe_context *pipe,
                         struct pipe_surface *surf)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert(surf->texture);
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


static void
llvmpipe_clear_render_target(struct pipe_context *pipe,
                             struct pipe_surface *dst,
                             const union pipe_color_union *color,
                             unsigned dstx, unsigned dsty,
                             unsigned width, unsigned height)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   if (!llvmpipe_check_render_cond(llvmpipe))
      return;

   util_clear_render_target(pipe, dst, color,
                            dstx, dsty, width, height);
}


static void
llvmpipe_clear_depth_stencil(struct pipe_context *pipe,
                             struct pipe_surface *dst,
                             unsigned clear_flags,
                             double depth,
                             unsigned stencil,
                             unsigned dstx, unsigned dsty,
                             unsigned width, unsigned height)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   if (!llvmpipe_check_render_cond(llvmpipe))
      return;

   util_clear_depth_stencil(pipe, dst, clear_flags,
                            depth, stencil,
                            dstx, dsty, width, height);
}


void
llvmpipe_init_surface_functions(struct llvmpipe_context *lp)
{
   lp->pipe.clear_render_target = llvmpipe_clear_render_target;
   lp->pipe.clear_depth_stencil = llvmpipe_clear_depth_stencil;
   lp->pipe.create_surface = llvmpipe_create_surface;
   lp->pipe.surface_destroy = llvmpipe_surface_destroy;
   /* These two are not actually functions dealing with surfaces */
   lp->pipe.resource_copy_region = lp_resource_copy;
   lp->pipe.blit = lp_blit;
   lp->pipe.flush_resource = lp_flush_resource;
}
