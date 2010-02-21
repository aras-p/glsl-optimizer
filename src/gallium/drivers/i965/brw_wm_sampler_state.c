/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
                   
#include "util/u_math.h"
#include "util/u_format.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_screen.h"


/* Samplers aren't strictly wm state from the hardware's perspective,
 * but that is the only situation in which we use them in this driver.
 */



static enum pipe_error
upload_default_color( struct brw_context *brw,
		      const GLfloat *color,
                      struct brw_winsys_buffer **bo_out )
{
   struct brw_sampler_default_color sdc;
   enum pipe_error ret;

   COPY_4V(sdc.color, color); 
   
   ret = brw_cache_data( &brw->cache, BRW_SAMPLER_DEFAULT_COLOR, &sdc,
                         NULL, 0, bo_out );
   if (ret)
      return ret;

   return PIPE_OK;
}


struct wm_sampler_key {
   int sampler_count;
   struct brw_sampler_state sampler[BRW_MAX_TEX_UNIT];
};


/** Sets up the cache key for sampler state for all texture units */
static void
brw_wm_sampler_populate_key(struct brw_context *brw,
			    struct wm_sampler_key *key)
{
   int i;

   memset(key, 0, sizeof(*key));

   key->sampler_count = MIN2(brw->curr.num_textures,
			    brw->curr.num_samplers);

   for (i = 0; i < key->sampler_count; i++) {
      const struct brw_texture *tex = brw_texture(brw->curr.texture[i]);
      const struct brw_sampler *sampler = brw->curr.sampler[i];
      struct brw_sampler_state *entry = &key->sampler[i];

      entry->ss0 = sampler->ss0;
      entry->ss1 = sampler->ss1;
      entry->ss2.default_color_pointer = 0; /* reloc */
      entry->ss3 = sampler->ss3;

      /* Cube-maps on 965 and later must use the same wrap mode for all 3
       * coordinate dimensions.  Futher, only CUBE and CLAMP are valid.
       */
      if (tex->base.target == PIPE_TEXTURE_CUBE) {
	 if (FALSE &&
	     (sampler->ss0.min_filter != BRW_MAPFILTER_NEAREST || 
	      sampler->ss0.mag_filter != BRW_MAPFILTER_NEAREST)) {
	    entry->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	    entry->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	    entry->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	 } else {
	    entry->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	    entry->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	    entry->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	 }
      } else if (tex->base.target == PIPE_TEXTURE_1D) {
	 /* There's a bug in 1D texture sampling - it actually pays
	  * attention to the wrap_t value, though it should not.
	  * Override the wrap_t value here to GL_REPEAT to keep
	  * any nonexistent border pixels from floating in.
	  */
	 entry->ss1.t_wrap_mode = BRW_TEXCOORDMODE_WRAP;
      }
   }
}


static enum pipe_error
brw_wm_sampler_update_default_colors(struct brw_context *brw)
{
   enum pipe_error ret;
   int nr = MIN2(brw->curr.num_textures,
		 brw->curr.num_samplers);
   int i;

   for (i = 0; i < nr; i++) {
      const struct brw_texture *tex = brw_texture(brw->curr.texture[i]);
      const struct brw_sampler *sampler = brw->curr.sampler[i];
      const float *bc;
      float bordercolor[4] = {
         sampler->border_color[0],
         sampler->border_color[0],
         sampler->border_color[0],
         sampler->border_color[0]
      };
      
      if (util_format_is_depth_or_stencil(tex->base.format)) {
         bc = bordercolor;
      }
      else {
         bc = sampler->border_color;
      }

      /* GL specs that border color for depth textures is taken from the
       * R channel, while the hardware uses A.  Spam R into all the
       * channels for safety.
       */
      ret = upload_default_color(brw, 
                                 bc,
                                 &brw->wm.sdc_bo[i]);
      if (ret) 
         return ret;
   }

   return PIPE_OK;
}



/* All samplers must be uploaded in a single contiguous array.  
 */
static int upload_wm_samplers( struct brw_context *brw )
{
   struct wm_sampler_key key;
   struct brw_winsys_reloc reloc[BRW_MAX_TEX_UNIT];
   enum pipe_error ret;
   int i;

   brw_wm_sampler_update_default_colors(brw);
   brw_wm_sampler_populate_key(brw, &key);

   if (brw->wm.sampler_count != key.sampler_count) {
      brw->wm.sampler_count = key.sampler_count;
      brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
   }

   if (brw->wm.sampler_count == 0) {
      bo_reference(&brw->wm.sampler_bo, NULL);
      return PIPE_OK;
   }

   /* Emit SDC relocations */
   for (i = 0; i < key.sampler_count; i++) {
      make_reloc( &reloc[i],
                  BRW_USAGE_SAMPLER,
                  0,
                  i * sizeof(struct brw_sampler_state) +
                  offsetof(struct brw_sampler_state, ss2),
                  brw->wm.sdc_bo[i]);
   }


   if (brw_search_cache(&brw->cache, BRW_SAMPLER,
                        &key, sizeof(key),
                        reloc, key.sampler_count,
                        NULL,
                        &brw->wm.sampler_bo))
      return PIPE_OK;

   /* If we didnt find it in the cache, compute the state and put it in the
    * cache.
    */
   ret = brw_upload_cache(&brw->cache, BRW_SAMPLER,
                          &key, sizeof(key),
                          reloc, key.sampler_count,
                          &key.sampler, sizeof(key.sampler),
                          NULL, NULL,
                          &brw->wm.sampler_bo);
   if (ret)
      return ret;


   return 0;
}

const struct brw_tracked_state brw_wm_samplers = {
   .dirty = {
      .mesa = PIPE_NEW_BOUND_TEXTURES | PIPE_NEW_SAMPLERS,
      .brw = 0,
      .cache = 0
   },
   .prepare = upload_wm_samplers,
};


