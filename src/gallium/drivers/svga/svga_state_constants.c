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

#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_tgsi.h"
#include "svga_debug.h"

#include "svga_hw_reg.h"

/***********************************************************************
 * Hardware update 
 */

/* Convert from PIPE_SHADER_* to SVGA3D_SHADERTYPE_*
 */
static int svga_shader_type( int unit )
{
   return unit + 1;
}


static int emit_const( struct svga_context *svga,
                       int unit,
                       int i,
                       const float *value )
{
   int ret = PIPE_OK;

   if (memcmp(svga->state.hw_draw.cb[unit][i], value, 4 * sizeof(float)) != 0) {
      if (SVGA_DEBUG & DEBUG_CONSTS)
         debug_printf("%s %s %d: %f %f %f %f\n",
                      __FUNCTION__,
                      unit == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                      i,
                      value[0],
                      value[1],
                      value[2],
                      value[3]);

      ret = SVGA3D_SetShaderConst( svga->swc, 
                                   i,
                                   svga_shader_type(unit),
                                   SVGA3D_CONST_TYPE_FLOAT,
                                   value );
      if (ret)
         return ret;

      memcpy(svga->state.hw_draw.cb[unit][i], value, 4 * sizeof(float));
   }
   
   return ret;
}

static int emit_consts( struct svga_context *svga,
                        int offset,
                        int unit )
{
   struct pipe_screen *screen = svga->pipe.screen;
   unsigned count;
   const float (*data)[4] = NULL;
   unsigned i;
   int ret = PIPE_OK;

   if (svga->curr.cb[unit] == NULL)
      goto done;

   count = svga->curr.cb[unit]->size / (4 * sizeof(float));

   data = (const float (*)[4])pipe_buffer_map(screen,
                                              svga->curr.cb[unit],
                                              PIPE_BUFFER_USAGE_CPU_READ);
   if (data == NULL) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto done;
   }

   for (i = 0; i < count; i++) {
      ret = emit_const( svga, unit, offset + i, data[i] );
      if (ret)
         goto done;
   }

done:
   if (data)
      pipe_buffer_unmap(screen, svga->curr.cb[unit]);

   return ret;
}
   
static int emit_fs_consts( struct svga_context *svga,
                           unsigned dirty )
{
   const struct svga_shader_result *result = svga->state.hw_draw.fs;
   const struct svga_fs_compile_key *key = &result->key.fkey;
   int ret = 0;

   ret = emit_consts( svga, 0, PIPE_SHADER_FRAGMENT );
   if (ret)
      return ret;

   /* The internally generated fragment shader for xor blending
    * doesn't have a 'result' struct.  It should be fixed to avoid
    * this special case, but work around it with a NULL check:
    */
   if (result != NULL &&
       key->num_unnormalized_coords)
   {
      unsigned offset = result->shader->info.file_max[TGSI_FILE_CONSTANT] + 1;
      int i;

      for (i = 0; i < key->num_textures; i++) {
         if (key->tex[i].unnormalized) {
            struct pipe_texture *tex = svga->curr.texture[i];
            float data[4];

            data[0] = 1.0 / (float)tex->width0;
            data[1] = 1.0 / (float)tex->height0;
            data[2] = 1.0;
            data[3] = 1.0;

            ret = emit_const( svga,
                              PIPE_SHADER_FRAGMENT,
                              key->tex[i].width_height_idx + offset,
                              data );
            if (ret)
               return ret;
         }
      }

      offset += key->num_unnormalized_coords;
   }

   return 0;
}


struct svga_tracked_state svga_hw_fs_parameters = 
{
   "hw fs params",
   (SVGA_NEW_FS_CONST_BUFFER |
    SVGA_NEW_FS_RESULT |
    SVGA_NEW_TEXTURE_BINDING),
   emit_fs_consts
};

/***********************************************************************
 */

static int emit_vs_consts( struct svga_context *svga,
                           unsigned dirty )
{
   const struct svga_shader_result *result = svga->state.hw_draw.vs;
   const struct svga_vs_compile_key *key = &result->key.vkey;
   int ret = 0;
   unsigned offset;

   /* SVGA_NEW_VS_RESULT
    */
   if (result == NULL) 
      return 0;

   /* SVGA_NEW_VS_CONST_BUFFER 
    */
   ret = emit_consts( svga, 0, PIPE_SHADER_VERTEX );
   if (ret)
      return ret;

   offset = result->shader->info.file_max[TGSI_FILE_CONSTANT] + 1;

   /* SVGA_NEW_VS_RESULT
    */
   if (key->need_prescale) {
      ret = emit_const( svga, PIPE_SHADER_VERTEX, offset++,
                        svga->state.hw_clear.prescale.scale );
      if (ret)
         return ret;

      ret = emit_const( svga, PIPE_SHADER_VERTEX, offset++,
                        svga->state.hw_clear.prescale.translate );
      if (ret)
         return ret;
   }

   /* SVGA_NEW_ZERO_STRIDE
    */
   if (key->zero_stride_vertex_elements) {
      unsigned i, curr_zero_stride = 0;
      for (i = 0; i < PIPE_MAX_ATTRIBS; ++i) {
         if (key->zero_stride_vertex_elements & (1 << i)) {
            ret = emit_const( svga, PIPE_SHADER_VERTEX, offset++,
                              svga->curr.zero_stride_constants +
                              4 * curr_zero_stride );
            if (ret)
               return ret;
            ++curr_zero_stride;
         }
      }
   }

   return 0;
}


struct svga_tracked_state svga_hw_vs_parameters = 
{
   "hw vs params",
   (SVGA_NEW_PRESCALE |
    SVGA_NEW_VS_CONST_BUFFER |
    SVGA_NEW_ZERO_STRIDE |
    SVGA_NEW_VS_RESULT),
   emit_vs_consts
};

