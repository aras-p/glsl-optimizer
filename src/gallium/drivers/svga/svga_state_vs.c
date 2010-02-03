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
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_bitmask.h"
#include "translate/translate.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_tgsi.h"

#include "svga_hw_reg.h"

/***********************************************************************
 */


static INLINE int compare_vs_keys( const struct svga_vs_compile_key *a,
                                   const struct svga_vs_compile_key *b )
{
   unsigned keysize = svga_vs_key_size( a );
   return memcmp( a, b, keysize );
}


static struct svga_shader_result *search_vs_key( struct svga_vertex_shader *vs,
                                                 const struct svga_vs_compile_key *key )
{
   struct svga_shader_result *result = vs->base.results;

   assert(key);

   for ( ; result; result = result->next) {
      if (compare_vs_keys( key, &result->key.vkey ) == 0)
         return result;
   }
   
   return NULL;
}


static enum pipe_error compile_vs( struct svga_context *svga,
                                   struct svga_vertex_shader *vs,
                                   const struct svga_vs_compile_key *key,
                                   struct svga_shader_result **out_result )
{
   struct svga_shader_result *result;
   enum pipe_error ret = PIPE_ERROR;

   result = svga_translate_vertex_program( vs, key );
   if (result == NULL) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto fail;
   }

   result->id = util_bitmask_add(svga->vs_bm);
   if(result->id == UTIL_BITMASK_INVALID_INDEX) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto fail;
   }

   ret = SVGA3D_DefineShader(svga->swc, 
                             result->id,
                             SVGA3D_SHADERTYPE_VS,
                             result->tokens, 
                             result->nr_tokens * sizeof result->tokens[0]);
   if (ret)
      goto fail;

   *out_result = result;
   result->next = vs->base.results;
   vs->base.results = result;
   return PIPE_OK;

fail:
   if (result) {
      if (result->id != UTIL_BITMASK_INVALID_INDEX)
         util_bitmask_clear( svga->vs_bm, result->id );
      svga_destroy_shader_result( result );
   }
   return ret;
}

/* SVGA_NEW_PRESCALE, SVGA_NEW_RAST, SVGA_NEW_ZERO_STRIDE
 */
static int make_vs_key( struct svga_context *svga,
                        struct svga_vs_compile_key *key )
{
   memset(key, 0, sizeof *key);
   key->need_prescale = svga->state.hw_clear.prescale.enabled;
   key->allow_psiz = svga->curr.rast->templ.point_size_per_vertex;
   key->zero_stride_vertex_elements =
      svga->curr.zero_stride_vertex_elements;
   key->num_zero_stride_vertex_elements =
      svga->curr.num_zero_stride_vertex_elements;
   return 0;
}



static int emit_hw_vs( struct svga_context *svga,
                       unsigned dirty )
{
   struct svga_shader_result *result = NULL;
   unsigned id = SVGA3D_INVALID_ID;
   int ret = 0;

   /* SVGA_NEW_NEED_SWTNL */
   if (!svga->state.sw.need_swtnl) {
      struct svga_vertex_shader *vs = svga->curr.vs;
      struct svga_vs_compile_key key;

      ret = make_vs_key( svga, &key );
      if (ret)
         return ret;

      result = search_vs_key( vs, &key );
      if (!result) {
         ret = compile_vs( svga, vs, &key, &result );
         if (ret)
            return ret;
      }

      assert (result);
      id = result->id;
   }

   if (result != svga->state.hw_draw.vs) {
      ret = SVGA3D_SetShader(svga->swc,
                             SVGA3D_SHADERTYPE_VS,
                             id );
      if (ret)
         return ret;

      svga->dirty |= SVGA_NEW_VS_RESULT;
      svga->state.hw_draw.vs = result;      
   }

   return 0;
}

struct svga_tracked_state svga_hw_vs = 
{
   "vertex shader (hwtnl)",
   (SVGA_NEW_VS |
    SVGA_NEW_PRESCALE |
    SVGA_NEW_NEED_SWTNL |
    SVGA_NEW_ZERO_STRIDE),
   emit_hw_vs
};


/***********************************************************************
 */
static int update_zero_stride( struct svga_context *svga,
                               unsigned dirty )
{
   unsigned i;

   svga->curr.zero_stride_vertex_elements = 0;
   svga->curr.num_zero_stride_vertex_elements = 0;

   for (i = 0; i < svga->curr.num_vertex_elements; i++) {
      const struct pipe_vertex_element *vel = &svga->curr.ve[i];
      const struct pipe_vertex_buffer *vbuffer = &svga->curr.vb[
         vel->vertex_buffer_index];
      if (vbuffer->stride == 0) {
         unsigned const_idx =
            svga->curr.num_zero_stride_vertex_elements;
         struct translate *translate;
         struct translate_key key;
         void *mapped_buffer;

         svga->curr.zero_stride_vertex_elements |= (1 << i);
         ++svga->curr.num_zero_stride_vertex_elements;

         key.output_stride = 4 * sizeof(float);
         key.nr_elements = 1;
         key.element[0].type = TRANSLATE_ELEMENT_NORMAL;
         key.element[0].input_format = vel->src_format;
         key.element[0].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         key.element[0].input_buffer = vel->vertex_buffer_index;
         key.element[0].input_offset = vel->src_offset;
         key.element[0].instance_divisor = vel->instance_divisor;
         key.element[0].output_offset = const_idx * 4 * sizeof(float);

         translate_key_sanitize(&key);
         /* translate_generic_create is technically private but
          * we don't want to code-generate, just want generic
          * translation */
         translate = translate_generic_create(&key);

         assert(vel->src_offset == 0);
         
         mapped_buffer = pipe_buffer_map_range(svga->pipe.screen, 
                                               vbuffer->buffer,
                                               vel->src_offset,
                                               util_format_get_blocksize(vel->src_format),
                                               PIPE_BUFFER_USAGE_CPU_READ);
         translate->set_buffer(translate, vel->vertex_buffer_index,
                               mapped_buffer,
                               vbuffer->stride);
         translate->run(translate, 0, 1, 0,
                        svga->curr.zero_stride_constants);

         pipe_buffer_unmap(svga->pipe.screen,
                           vbuffer->buffer);
         translate->release(translate);
      }
   }

   if (svga->curr.num_zero_stride_vertex_elements)
      svga->dirty |= SVGA_NEW_ZERO_STRIDE;

   return 0;
}

struct svga_tracked_state svga_hw_update_zero_stride =
{
   "update zero_stride",
   ( SVGA_NEW_VELEMENT |
     SVGA_NEW_VBUFFER ),
   update_zero_stride
};
