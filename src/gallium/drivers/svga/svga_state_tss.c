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

#include "svga_screen_texture.h"
#include "svga_winsys.h"
#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"


void svga_cleanup_tss_binding(struct svga_context *svga)
{
   int i;
   unsigned count = MAX2( svga->curr.num_textures,
                          svga->state.hw_draw.num_views );

   for (i = 0; i < count; i++) {
      struct svga_hw_view_state *view = &svga->state.hw_draw.views[i];

      svga_sampler_view_reference(&view->v, NULL);
      pipe_texture_reference( &svga->curr.texture[i], NULL );
      pipe_texture_reference( &view->texture, NULL );

      view->dirty = 1;
   }
}


static int
update_tss_binding(struct svga_context *svga, 
                   unsigned dirty )
{
   unsigned i;
   unsigned count = MAX2( svga->curr.num_textures,
                          svga->state.hw_draw.num_views );
   unsigned min_lod;
   unsigned max_lod;


   struct {
      struct {
         unsigned unit;
         struct svga_hw_view_state *view;
      } bind[PIPE_MAX_SAMPLERS];

      unsigned bind_count;
   } queue;

   queue.bind_count = 0;
   
   for (i = 0; i < count; i++) {
      const struct svga_sampler_state *s = svga->curr.sampler[i];
      struct svga_hw_view_state *view = &svga->state.hw_draw.views[i];

      /* get min max lod */
      if (svga->curr.texture[i]) {
         min_lod = MAX2(s->view_min_lod, 0);
         max_lod = MIN2(s->view_max_lod, svga->curr.texture[i]->last_level);
      } else {
         min_lod = 0;
         max_lod = 0;
      }

      if (view->texture != svga->curr.texture[i] ||
          view->min_lod != min_lod ||
          view->max_lod != max_lod) {

         svga_sampler_view_reference(&view->v, NULL);
         pipe_texture_reference( &view->texture, svga->curr.texture[i] );

         view->dirty = TRUE;
         view->min_lod = min_lod;
         view->max_lod = max_lod;

         if (svga->curr.texture[i])
            view->v = svga_get_tex_sampler_view(&svga->pipe, 
                                                svga->curr.texture[i], 
                                                min_lod,
                                                max_lod);
      }

      if (view->dirty) {
         queue.bind[queue.bind_count].unit = i;
         queue.bind[queue.bind_count].view = view;
         queue.bind_count++;
      } 
      else if (view->v) {
         svga_validate_sampler_view(svga, view->v);
      }
   }

   svga->state.hw_draw.num_views = svga->curr.num_textures;

   if (queue.bind_count) {
      SVGA3dTextureState *ts;

      if (SVGA3D_BeginSetTextureState( svga->swc,
                                       &ts,
                                       queue.bind_count ) != PIPE_OK)
         goto fail;

      for (i = 0; i < queue.bind_count; i++) {
         ts[i].stage = queue.bind[i].unit;
         ts[i].name = SVGA3D_TS_BIND_TEXTURE;

         if (queue.bind[i].view->v) {
            svga->swc->surface_relocation(svga->swc,
                                          &ts[i].value,
                                          queue.bind[i].view->v->handle,
                                          PIPE_BUFFER_USAGE_GPU_READ);
         }
         else {
            ts[i].value = SVGA3D_INVALID_ID;
         }
         
         queue.bind[i].view->dirty = FALSE;
      }

      SVGA_FIFOCommitAll( svga->swc );
   }

   return 0;

fail:
   return PIPE_ERROR_OUT_OF_MEMORY;
}


struct svga_tracked_state svga_hw_tss_binding = {
   "texture binding emit",
   SVGA_NEW_TEXTURE_BINDING |
   SVGA_NEW_SAMPLER,
   update_tss_binding
};


/***********************************************************************
 */

struct ts_queue {
   unsigned ts_count;
   SVGA3dTextureState ts[PIPE_MAX_SAMPLERS*SVGA3D_TS_MAX];
};


#define EMIT_TS(svga, unit, val, token, fail)                           \
do {                                                                    \
   if (svga->state.hw_draw.ts[unit][SVGA3D_TS_##token] != val) {        \
      svga_queue_tss( &queue, unit, SVGA3D_TS_##token, val );           \
      svga->state.hw_draw.ts[unit][SVGA3D_TS_##token] = val;            \
   }                                                                    \
} while (0)

#define EMIT_TS_FLOAT(svga, unit, fvalue, token, fail)                  \
do {                                                                    \
   unsigned val = fui(fvalue);                                          \
   if (svga->state.hw_draw.ts[unit][SVGA3D_TS_##token] != val) {        \
      svga_queue_tss( &queue, unit, SVGA3D_TS_##token, val );           \
      svga->state.hw_draw.ts[unit][SVGA3D_TS_##token] = val;            \
   }                                                                    \
} while (0)


static INLINE void 
svga_queue_tss( struct ts_queue *q,
                unsigned unit,
                unsigned tss,
                unsigned value )
{
   assert(q->ts_count < sizeof(q->ts)/sizeof(q->ts[0]));
   q->ts[q->ts_count].stage = unit;
   q->ts[q->ts_count].name = tss;
   q->ts[q->ts_count].value = value;
   q->ts_count++;
}


static int
update_tss(struct svga_context *svga, 
           unsigned dirty )
{
   unsigned i;
   struct ts_queue queue;

   queue.ts_count = 0;
   for (i = 0; i < svga->curr.num_samplers; i++) {
      if (svga->curr.sampler[i]) {
         const struct svga_sampler_state *curr = svga->curr.sampler[i];

         EMIT_TS(svga, i, curr->mipfilter, MIPFILTER, fail);
         EMIT_TS(svga, i, curr->min_lod, TEXTURE_MIPMAP_LEVEL, fail);
         EMIT_TS(svga, i, curr->magfilter, MAGFILTER, fail);
         EMIT_TS(svga, i, curr->minfilter, MINFILTER, fail);
         EMIT_TS(svga, i, curr->aniso_level, TEXTURE_ANISOTROPIC_LEVEL, fail);
         EMIT_TS_FLOAT(svga, i, curr->lod_bias, TEXTURE_LOD_BIAS, fail);
         EMIT_TS(svga, i, curr->addressu, ADDRESSU, fail);
         EMIT_TS(svga, i, curr->addressw, ADDRESSW, fail);
         EMIT_TS(svga, i, curr->bordercolor, BORDERCOLOR, fail);
         // TEXCOORDINDEX -- hopefully not needed

         if (svga->curr.tex_flags.flag_1d & (1 << i)) {
            debug_printf("wrap 1d tex %d\n", i);
            EMIT_TS(svga, i, SVGA3D_TEX_ADDRESS_WRAP, ADDRESSV, fail);
         }
         else
            EMIT_TS(svga, i, curr->addressv, ADDRESSV, fail);

         if (svga->curr.tex_flags.flag_srgb & (1 << i))
            EMIT_TS_FLOAT(svga, i, 2.2f, GAMMA, fail);
         else
            EMIT_TS_FLOAT(svga, i, 1.0f, GAMMA, fail);

      }
   }
 
   if (queue.ts_count) {
      SVGA3dTextureState *ts;

      if (SVGA3D_BeginSetTextureState( svga->swc,
                                       &ts,
                                       queue.ts_count ) != PIPE_OK)
         goto fail;

      memcpy( ts,
              queue.ts,
              queue.ts_count * sizeof queue.ts[0]);
      
      SVGA_FIFOCommitAll( svga->swc );
   }

   return 0;

fail:
   /* XXX: need to poison cached hardware state on failure to ensure
    * dirty state gets re-emitted.  Fix this by re-instating partial
    * FIFOCommit command and only updating cached hw state once the
    * initial allocation has succeeded.
    */
   memset(svga->state.hw_draw.ts, 0xcd, sizeof(svga->state.hw_draw.ts));

   return PIPE_ERROR_OUT_OF_MEMORY;
}


struct svga_tracked_state svga_hw_tss = {
   "texture state emit",
   (SVGA_NEW_SAMPLER |
    SVGA_NEW_TEXTURE_FLAGS),
   update_tss
};

