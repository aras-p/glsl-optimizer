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


#include "pipe/p_winsys.h"
#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_inlines.h"
#include "cso_cache/cso_context.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_shaders.h"
#include "trace/tr_screen.h"
#include "trace/tr_context.h"

#include "st_device.h"
#include "st_winsys.h"


static void
st_device_really_destroy(struct st_device *st_dev) 
{
   if(st_dev->screen)
      st_dev->screen->destroy(st_dev->screen);
   
   FREE(st_dev);
}


void
st_device_destroy(struct st_device *st_dev) 
{
   if(!--st_dev->refcount)
      st_device_really_destroy(st_dev);
}


static struct st_device *
st_device_create_from_st_winsys(const struct st_winsys *st_ws) 
{
   struct st_device *st_dev;
   
   if(!st_ws->screen_create ||
      !st_ws->context_create)
      return NULL;
   
   st_dev = CALLOC_STRUCT(st_device);
   if(!st_dev)
      return NULL;
   
   st_dev->refcount = 1;
   st_dev->st_ws = st_ws;
   
   st_dev->real_screen = st_ws->screen_create();
   if(!st_dev->real_screen) {
      st_device_destroy(st_dev);
      return NULL;
   }

   st_dev->screen = trace_screen_create(st_dev->real_screen);
   if(!st_dev->screen) {
      st_device_destroy(st_dev);
      return NULL;
   }
   
   return st_dev;
}


struct st_device *
st_device_create(boolean hardware) {
   if(hardware)
      return st_device_create_from_st_winsys(&st_hardpipe_winsys);
   else
      return st_device_create_from_st_winsys(&st_softpipe_winsys);
}


void
st_context_destroy(struct st_context *st_ctx) 
{
   unsigned i;
   
   if(st_ctx) {
      struct st_device *st_dev = st_ctx->st_dev;
      
      if(st_ctx->cso) {
         cso_delete_vertex_shader(st_ctx->cso, st_ctx->vs);
         cso_delete_fragment_shader(st_ctx->cso, st_ctx->fs);
         
         cso_destroy_context(st_ctx->cso);
      }
      
      if(st_ctx->pipe)
         st_ctx->pipe->destroy(st_ctx->pipe);
      
      for(i = 0; i < PIPE_MAX_SAMPLERS; ++i)
         pipe_texture_reference(&st_ctx->sampler_textures[i], NULL);
      pipe_texture_reference(&st_ctx->default_texture, NULL);

      FREE(st_ctx);
      
      if(!--st_dev->refcount)
         st_device_really_destroy(st_dev);
   }
}


struct st_context *
st_context_create(struct st_device *st_dev) 
{
   struct st_context *st_ctx;
   
   st_ctx = CALLOC_STRUCT(st_context);
   if(!st_ctx)
      return NULL;
   
   st_ctx->st_dev = st_dev;
   ++st_dev->refcount;
   
   st_ctx->real_pipe = st_dev->st_ws->context_create(st_dev->real_screen);
   if(!st_ctx->real_pipe) {
      st_context_destroy(st_ctx);
      return NULL;
   }
   
   st_ctx->pipe = trace_context_create(st_dev->screen, st_ctx->real_pipe);
   if(!st_ctx->pipe) {
      st_context_destroy(st_ctx);
      return NULL;
   }

   st_ctx->cso = cso_create_context(st_ctx->pipe);
   if(!st_ctx->cso) {
      st_context_destroy(st_ctx);
      return NULL;
   }
   
   /* disabled blending/masking */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.colormask = PIPE_MASK_RGBA;
      cso_set_blend(st_ctx->cso, &blend);
   }

   /* no-op depth/stencil/alpha */
   {
      struct pipe_depth_stencil_alpha_state depthstencil;
      memset(&depthstencil, 0, sizeof(depthstencil));
      cso_set_depth_stencil_alpha(st_ctx->cso, &depthstencil);
   }

   /* rasterizer */
   {
      struct pipe_rasterizer_state rasterizer;
      memset(&rasterizer, 0, sizeof(rasterizer));
      rasterizer.front_winding = PIPE_WINDING_CW;
      rasterizer.cull_mode = PIPE_WINDING_NONE;
      rasterizer.bypass_clipping = 1;
      /*rasterizer.bypass_vs = 1;*/
      cso_set_rasterizer(st_ctx->cso, &rasterizer);
   }

   /* identity viewport */
   {
      struct pipe_viewport_state viewport;
      viewport.scale[0] = 1.0;
      viewport.scale[1] = 1.0;
      viewport.scale[2] = 1.0;
      viewport.scale[3] = 1.0;
      viewport.translate[0] = 0.0;
      viewport.translate[1] = 0.0;
      viewport.translate[2] = 0.0;
      viewport.translate[3] = 0.0;
      cso_set_viewport(st_ctx->cso, &viewport);
   }

   /* samplers */
   {
      struct pipe_sampler_state sampler;
      unsigned i;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
      sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
      sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
      sampler.normalized_coords = 1;
      for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
         cso_single_sampler(st_ctx->cso, i, &sampler);
      cso_single_sampler_done(st_ctx->cso);
   }

   /* default textures */
   {
      struct pipe_screen *screen = st_dev->screen;
      struct pipe_texture templat;
      struct pipe_surface *surface;
      unsigned i;

      memset( &templat, 0, sizeof( templat ) );
      templat.target = PIPE_TEXTURE_2D;
      templat.format = PIPE_FORMAT_A8R8G8B8_UNORM;
      templat.block.size = 4;
      templat.block.width = 1;
      templat.block.height = 1;
      templat.width[0] = 1;
      templat.height[0] = 1;
      templat.depth[0] = 1;
      templat.last_level = 0;
   
      st_ctx->default_texture = screen->texture_create( screen, &templat );
      if(st_ctx->default_texture) {
         surface = screen->get_tex_surface( screen, 
                                            st_ctx->default_texture, 0, 0, 0,
                                            PIPE_BUFFER_USAGE_CPU_WRITE );
         if(surface) {
            uint32_t *map;
            map = (uint32_t *) pipe_surface_map(surface, PIPE_BUFFER_USAGE_CPU_WRITE );
            if(map) {
               *map = 0x00000000;
               pipe_surface_unmap( surface );
            }
            pipe_surface_reference(&surface, NULL);
         }
      }
   
      for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
         pipe_texture_reference(&st_ctx->sampler_textures[i], st_ctx->default_texture);
      
      cso_set_sampler_textures(st_ctx->cso, PIPE_MAX_SAMPLERS, st_ctx->sampler_textures);
   }
   
   /* vertex shader */
   {
      struct pipe_shader_state vert_shader;

      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      st_ctx->vs = util_make_vertex_passthrough_shader(st_ctx->pipe, 
                                                       2, 
                                                       semantic_names,
                                                       semantic_indexes,
                                                       &vert_shader);
      cso_set_vertex_shader_handle(st_ctx->cso, st_ctx->vs);
   }

   /* fragment shader */
   {
      struct pipe_shader_state frag_shader;
      st_ctx->fs = util_make_fragment_passthrough_shader(st_ctx->pipe, 
                                                         &frag_shader);
      cso_set_fragment_shader_handle(st_ctx->cso, st_ctx->fs);
   }

   return st_ctx;
}


void
st_buffer_destroy(struct st_buffer *st_buf)
{
   if(st_buf) {
      struct pipe_screen *screen = st_buf->st_dev->screen;
      pipe_buffer_reference(screen, &st_buf->buffer, NULL);
      FREE(st_buf);
   }
}


struct st_buffer *
st_buffer_create(struct st_device *st_dev,
                 unsigned alignment, unsigned usage, unsigned size)
{
   struct pipe_screen *screen = st_dev->screen;
   struct st_buffer *st_buf;
   
   st_buf = CALLOC_STRUCT(st_buffer);
   if(!st_buf)
      return NULL;

   st_buf->st_dev = st_dev;
   
   st_buf->buffer = pipe_buffer_create(screen, alignment, usage, size);
   if(!st_buf->buffer) {
      st_buffer_destroy(st_buf);
      return NULL;
   }
   
   return st_buf;
}

