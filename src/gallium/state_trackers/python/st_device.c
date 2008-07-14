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


#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_inlines.h"
#include "cso_cache/cso_context.h"
#include "util/u_simple_shaders.h"
#include "st_device.h"
#include "st_winsys.h"


static void
st_device_really_destroy(struct st_device *st_dev) 
{
   if(st_dev->screen)
      st_dev->st_ws->screen_destroy(st_dev->screen);
   
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
      !st_ws->screen_destroy ||
      !st_ws->context_create ||
      !st_ws->context_destroy)
      return NULL;
   
   st_dev = CALLOC_STRUCT(st_device);
   if(!st_dev)
      return NULL;
   
   st_dev->st_ws = st_ws;
   
   st_dev->screen = st_ws->screen_create();
   if(!st_dev->screen)
      st_device_destroy(st_dev);
   
   return st_dev;
}


struct st_device *
st_device_create(boolean hardware) {
#if 0
   if(hardware)
      return st_device_create_from_st_winsys(&st_hardware_winsys);
   else
#endif
      return st_device_create_from_st_winsys(&st_software_winsys);
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
         st_ctx->st_dev->st_ws->context_destroy(st_ctx->pipe);
      
      for(i = 0; i < PIPE_MAX_SAMPLERS; ++i)
         pipe_texture_reference(&st_ctx->sampler_textures[i], NULL);
   
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
   
   st_ctx->pipe = st_dev->st_ws->context_create(st_dev->screen);
   if(!st_ctx->pipe)
      st_context_destroy(st_ctx);
   
   st_ctx->cso = cso_create_context(st_ctx->pipe);
   if(!st_ctx->cso)
      st_context_destroy(st_ctx);
   
   /* vertex shader */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      st_ctx->vs = util_make_vertex_passthrough_shader(st_ctx->pipe, 
                                                       2, 
                                                       semantic_names,
                                                       semantic_indexes,
                                                       &st_ctx->vert_shader);
   }

   /* fragment shader */
   st_ctx->fs = util_make_fragment_passthrough_shader(st_ctx->pipe, 
                                                      &st_ctx->frag_shader);
   
   cso_set_fragment_shader_handle(st_ctx->cso, st_ctx->fs);
   cso_set_vertex_shader_handle(st_ctx->cso, st_ctx->vs);

   return st_ctx;
}
