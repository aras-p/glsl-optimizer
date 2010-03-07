/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"

#include "svga_context.h"
#include "svga_screen_texture.h"

#include "svga_debug.h"

static INLINE unsigned
translate_wrap_mode(unsigned wrap)
{
   switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT: 
      return SVGA3D_TEX_ADDRESS_WRAP;

   case PIPE_TEX_WRAP_CLAMP: 
      return SVGA3D_TEX_ADDRESS_CLAMP;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE: 
      /* Unfortunately SVGA3D_TEX_ADDRESS_EDGE not respected by
       * hardware.
       */
      return SVGA3D_TEX_ADDRESS_CLAMP;

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER: 
      return SVGA3D_TEX_ADDRESS_BORDER;

   case PIPE_TEX_WRAP_MIRROR_REPEAT: 
      return SVGA3D_TEX_ADDRESS_MIRROR;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:  
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:   
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER: 
      return SVGA3D_TEX_ADDRESS_MIRRORONCE;

   default:
      assert(0);
      return SVGA3D_TEX_ADDRESS_WRAP;
   }
}

static INLINE unsigned translate_img_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST: return SVGA3D_TEX_FILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:  return SVGA3D_TEX_FILTER_LINEAR;
   default:
      assert(0);
      return SVGA3D_TEX_FILTER_NEAREST;
   }
}

static INLINE unsigned translate_mip_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_MIPFILTER_NONE:    return SVGA3D_TEX_FILTER_NONE;
   case PIPE_TEX_MIPFILTER_NEAREST: return SVGA3D_TEX_FILTER_NEAREST;
   case PIPE_TEX_MIPFILTER_LINEAR:  return SVGA3D_TEX_FILTER_LINEAR;
   default:
      assert(0);
      return SVGA3D_TEX_FILTER_NONE;
   }
}

static void *
svga_create_sampler_state(struct pipe_context *pipe,
                          const struct pipe_sampler_state *sampler)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_sampler_state *cso = CALLOC_STRUCT( svga_sampler_state );
   
   cso->mipfilter = translate_mip_filter(sampler->min_mip_filter);
   cso->magfilter = translate_img_filter( sampler->mag_img_filter );
   cso->minfilter = translate_img_filter( sampler->min_img_filter );
   cso->aniso_level = MAX2( sampler->max_anisotropy, 1 );
   if(sampler->max_anisotropy)
      cso->magfilter = cso->minfilter = SVGA3D_TEX_FILTER_ANISOTROPIC;
   cso->lod_bias = sampler->lod_bias;
   cso->addressu = translate_wrap_mode(sampler->wrap_s);
   cso->addressv = translate_wrap_mode(sampler->wrap_t);
   cso->addressw = translate_wrap_mode(sampler->wrap_r);
   cso->normalized_coords = sampler->normalized_coords;
   cso->compare_mode = sampler->compare_mode;
   cso->compare_func = sampler->compare_func;

   {
      uint32 r = float_to_ubyte(sampler->border_color[0]);
      uint32 g = float_to_ubyte(sampler->border_color[1]);
      uint32 b = float_to_ubyte(sampler->border_color[2]);
      uint32 a = float_to_ubyte(sampler->border_color[3]);

      cso->bordercolor = (a << 24) | (r << 16) | (g << 8) | b;
   }

   /* No SVGA3D support for:
    *    - min/max LOD clamping
    */
   cso->min_lod = 0;
   cso->view_min_lod = MAX2(sampler->min_lod, 0);
   cso->view_max_lod = MAX2(sampler->max_lod, 0);

   /* Use min_mipmap */
   if (svga->debug.use_min_mipmap) {
      if (cso->view_min_lod == cso->view_max_lod) {
         cso->min_lod = cso->view_min_lod;
         cso->view_min_lod = 0;
         cso->view_max_lod = 1000; /* Just a high number */
         cso->mipfilter = SVGA3D_TEX_FILTER_NONE;
      }
   }

   SVGA_DBG(DEBUG_VIEWS, "min %u, view(min %u, max %u) lod, mipfilter %s\n",
            cso->min_lod, cso->view_min_lod, cso->view_max_lod,
            cso->mipfilter == SVGA3D_TEX_FILTER_NONE ? "SVGA3D_TEX_FILTER_NONE" : "SOMETHING");

   return cso;
}

static void svga_bind_sampler_states(struct pipe_context *pipe,
                                     unsigned num, void **sampler)
{
   struct svga_context *svga = svga_context(pipe);
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == svga->curr.num_samplers &&
       !memcmp(svga->curr.sampler, sampler, num * sizeof(void *))) {
      debug_printf("sampler noop\n");
      return;
   }

   for (i = 0; i < num; i++)
      svga->curr.sampler[i] = sampler[i];

   for (i = num; i < svga->curr.num_samplers; i++)
      svga->curr.sampler[i] = NULL;

   svga->curr.num_samplers = num;
   svga->dirty |= SVGA_NEW_SAMPLER;
}

static void svga_delete_sampler_state(struct pipe_context *pipe,
                                      void *sampler)
{
   FREE(sampler);
}


static void svga_set_sampler_textures(struct pipe_context *pipe,
                                      unsigned num,
                                      struct pipe_texture **texture)
{
   struct svga_context *svga = svga_context(pipe);
   unsigned flag_1d = 0;
   unsigned flag_srgb = 0;
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == svga->curr.num_textures &&
       !memcmp(svga->curr.texture, texture, num * sizeof(struct pipe_texture *))) {
      if (0) debug_printf("texture noop\n");
      return;
   }

   for (i = 0; i < num; i++) {
      pipe_texture_reference(&svga->curr.texture[i],
                             texture[i]);

      if (!texture[i])
         continue;

      if (texture[i]->format == PIPE_FORMAT_B8G8R8A8_SRGB)
         flag_srgb |= 1 << i;

      if (texture[i]->target == PIPE_TEXTURE_1D)
         flag_1d |= 1 << i;
   }

   for (i = num; i < svga->curr.num_textures; i++)
      pipe_texture_reference(&svga->curr.texture[i],
                             NULL);

   svga->curr.num_textures = num;
   svga->dirty |= SVGA_NEW_TEXTURE_BINDING;

   if (flag_srgb != svga->curr.tex_flags.flag_srgb ||
       flag_1d != svga->curr.tex_flags.flag_1d) 
   {
      svga->dirty |= SVGA_NEW_TEXTURE_FLAGS;
      svga->curr.tex_flags.flag_1d = flag_1d;
      svga->curr.tex_flags.flag_srgb = flag_srgb;
   }  
}



void svga_init_sampler_functions( struct svga_context *svga )
{
   svga->pipe.create_sampler_state = svga_create_sampler_state;
   svga->pipe.bind_fragment_sampler_states = svga_bind_sampler_states;
   svga->pipe.delete_sampler_state = svga_delete_sampler_state;
   svga->pipe.set_fragment_sampler_textures = svga_set_sampler_textures;
}



