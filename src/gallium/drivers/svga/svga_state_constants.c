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
#include "util/u_memory.h"
#include "pipe/p_defines.h"

#include "svga_screen.h"
#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_tgsi.h"
#include "svga_debug.h"
#include "svga_resource_buffer.h"

#include "svga_hw_reg.h"


/*
 * Don't try to send more than 4kb of successive constants.
 */
#define MAX_CONST_REG_COUNT 256  /**< number of float[4] constants */

/**
 * Extra space for svga-specific VS/PS constants (such as texcoord
 * scale factors, vertex transformation scale/translation).
 */
#define MAX_EXTRA_CONSTS 32

/** Guest-backed surface constant buffers must be this size */
#define GB_CONSTBUF_SIZE (SVGA3D_CONSTREG_MAX)

/**
 * Convert from PIPE_SHADER_* to SVGA3D_SHADERTYPE_*
 */
static unsigned
svga_shader_type(unsigned shader)
{
   switch (shader) {
   case PIPE_SHADER_VERTEX:
      return SVGA3D_SHADERTYPE_VS;
   case PIPE_SHADER_FRAGMENT:
      return SVGA3D_SHADERTYPE_PS;
   default:
      assert(!"Unexpected shader type");
      return SVGA3D_SHADERTYPE_VS;
   }
}


/**
 * Emit any extra fragment shader constants into the buffer pointed
 * to by 'dest'.
 * In particular, these would be the scaling factors needed for handling
 * unnormalized texture coordinates for texture rectangles.
 * \return number of float[4] constants put into the dest buffer
 */
static unsigned
svga_get_extra_fs_constants(struct svga_context *svga, float *dest)
{
   const struct svga_shader_variant *variant = svga->state.hw_draw.fs;
   const struct svga_fs_compile_key *key = &variant->key.fkey;
   unsigned count = 0;

   /* SVGA_NEW_VS_VARIANT
    */
   if (key->num_unnormalized_coords) {
      unsigned i;

      for (i = 0; i < key->num_textures; i++) {
         if (key->tex[i].unnormalized) {
            struct pipe_resource *tex = svga->curr.sampler_views[i]->texture;

            /* debug/sanity check */
            assert(key->tex[i].width_height_idx == count);

            *dest++ = 1.0 / (float)tex->width0;
            *dest++ = 1.0 / (float)tex->height0;
            *dest++ = 1.0;
            *dest++ = 1.0;

            count++;
         }
      }
   }

   assert(count <= MAX_EXTRA_CONSTS);

   return count;
}


/**
 * Emit any extra vertex shader constants into the buffer pointed
 * to by 'dest'.
 * In particular, these would be the scale and bias factors computed
 * from the framebuffer size which are used to copy with differences in
 * GL vs D3D coordinate spaces.  See svga_tgsi_insn.c for more info.
 * \return number of float[4] constants put into the dest buffer
 */
static unsigned
svga_get_extra_vs_constants(struct svga_context *svga, float *dest)
{
   const struct svga_shader_variant *variant = svga->state.hw_draw.vs;
   const struct svga_vs_compile_key *key = &variant->key.vkey;
   unsigned count = 0;

   /* SVGA_NEW_VS_VARIANT
    */
   if (key->need_prescale) {
      memcpy(dest, svga->state.hw_clear.prescale.scale, 4 * sizeof(float));
      dest += 4;

      memcpy(dest, svga->state.hw_clear.prescale.translate, 4 * sizeof(float));
      dest += 4;

      count = 2;
   }

   assert(count <= MAX_EXTRA_CONSTS);

   return count;
}


/**
 * Check and emit one shader constant register.
 * \param shader  PIPE_SHADER_FRAGMENT or PIPE_SHADER_VERTEX
 * \param i  which float[4] constant to change
 * \param value  the new float[4] value
 */
static enum pipe_error
emit_const(struct svga_context *svga, unsigned shader, unsigned i,
           const float *value)
{
   enum pipe_error ret = PIPE_OK;

   assert(shader < PIPE_SHADER_TYPES);
   assert(i < SVGA3D_CONSTREG_MAX);

   if (memcmp(svga->state.hw_draw.cb[shader][i], value,
              4 * sizeof(float)) != 0) {
      if (SVGA_DEBUG & DEBUG_CONSTS)
         debug_printf("%s %s %u: %f %f %f %f\n",
                      __FUNCTION__,
                      shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                      i,
                      value[0],
                      value[1],
                      value[2],
                      value[3]);

      ret = SVGA3D_SetShaderConst( svga->swc,
                                   i,
                                   svga_shader_type(shader),
                                   SVGA3D_CONST_TYPE_FLOAT,
                                   value );
      if (ret != PIPE_OK)
         return ret;

      memcpy(svga->state.hw_draw.cb[shader][i], value, 4 * sizeof(float));
   }

   return ret;
}


/*
 * Check and emit a range of shader constant registers, trying to coalesce
 * successive shader constant updates in a single command in order to save
 * space on the command buffer.  This is a HWv8 feature.
 */
static enum pipe_error
emit_const_range(struct svga_context *svga,
                 unsigned shader,
                 unsigned offset,
                 unsigned count,
                 const float (*values)[4])
{
   unsigned i, j;
   enum pipe_error ret;

#ifdef DEBUG
   if (offset + count > SVGA3D_CONSTREG_MAX) {
      debug_printf("svga: too many constants (offset %u + count %u = %u (max = %u))\n",
                   offset, count, offset + count, SVGA3D_CONSTREG_MAX);
   }
#endif

   if (offset > SVGA3D_CONSTREG_MAX) {
      /* This isn't OK, but if we propagate an error all the way up we'll
       * just get into more trouble.
       * XXX note that offset is always zero at this time so this is moot.
       */
      return PIPE_OK;
   }

   if (offset + count > SVGA3D_CONSTREG_MAX) {
      /* Just drop the extra constants for now.
       * Ideally we should not have allowed the app to create a shader
       * that exceeds our constant buffer size but there's no way to
       * express that in gallium at this time.
       */
      count = SVGA3D_CONSTREG_MAX - offset;
   }

   i = 0;
   while (i < count) {
      if (memcmp(svga->state.hw_draw.cb[shader][offset + i],
                 values[i],
                 4 * sizeof(float)) != 0) {
         /* Found one dirty constant
          */
         if (SVGA_DEBUG & DEBUG_CONSTS)
            debug_printf("%s %s %d: %f %f %f %f\n",
                         __FUNCTION__,
                         shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                         offset + i,
                         values[i][0],
                         values[i][1],
                         values[i][2],
                         values[i][3]);

         /* Look for more consecutive dirty constants.
          */
         j = i + 1;
         while (j < count &&
                j < i + MAX_CONST_REG_COUNT &&
                memcmp(svga->state.hw_draw.cb[shader][offset + j],
                       values[j],
                       4 * sizeof(float)) != 0) {

            if (SVGA_DEBUG & DEBUG_CONSTS)
               debug_printf("%s %s %d: %f %f %f %f\n",
                            __FUNCTION__,
                            shader == PIPE_SHADER_VERTEX ? "VERT" : "FRAG",
                            offset + j,
                            values[j][0],
                            values[j][1],
                            values[j][2],
                            values[j][3]);

            ++j;
         }

         assert(j >= i + 1);

         /* Send them all together.
          */
         if (svga_have_gb_objects(svga)) {
            ret = SVGA3D_SetGBShaderConstsInline(svga->swc,
                                                 offset + i, /* start */
                                                 j - i,  /* count */
                                                 svga_shader_type(shader),
                                                 SVGA3D_CONST_TYPE_FLOAT,
                                                 values + i);
         }
         else {
            ret = SVGA3D_SetShaderConsts(svga->swc,
                                         offset + i, j - i,
                                         svga_shader_type(shader),
                                         SVGA3D_CONST_TYPE_FLOAT,
                                         values + i);
         }
         if (ret != PIPE_OK) {
            return ret;
         }

         /*
          * Local copy of the hardware state.
          */
         memcpy(svga->state.hw_draw.cb[shader][offset + i],
                values[i],
                (j - i) * 4 * sizeof(float));

         i = j + 1;
      } else {
         ++i;
      }
   }

   return PIPE_OK;
}


/**
 * Emit all the constants in a constant buffer for a shader stage.
 */
static enum pipe_error
emit_consts(struct svga_context *svga, unsigned shader)
{
   struct svga_screen *ss = svga_screen(svga->pipe.screen);
   struct pipe_transfer *transfer = NULL;
   unsigned count;
   const float (*data)[4] = NULL;
   unsigned i;
   enum pipe_error ret = PIPE_OK;
   const unsigned offset = 0;

   assert(shader < PIPE_SHADER_TYPES);

   if (svga->curr.cbufs[shader].buffer == NULL)
      goto done;

   data = (const float (*)[4])pipe_buffer_map(&svga->pipe,
                                              svga->curr.cbufs[shader].buffer,
                                              PIPE_TRANSFER_READ,
					      &transfer);
   if (data == NULL) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto done;
   }

   /* sanity check */
   assert(svga->curr.cbufs[shader].buffer->width0 >=
          svga->curr.cbufs[shader].buffer_size);

   /* Use/apply the constant buffer size and offsets here */
   count = svga->curr.cbufs[shader].buffer_size / (4 * sizeof(float));
   data += svga->curr.cbufs[shader].buffer_offset / (4 * sizeof(float));

   if (ss->hw_version >= SVGA3D_HWVERSION_WS8_B1) {
      ret = emit_const_range( svga, shader, offset, count, data );
      if (ret != PIPE_OK) {
         goto done;
      }
   } else {
      for (i = 0; i < count; i++) {
         ret = emit_const( svga, shader, offset + i, data[i] );
         if (ret != PIPE_OK) {
            goto done;
         }
      }
   }

done:
   if (data)
      pipe_buffer_unmap(&svga->pipe, transfer);

   return ret;
}


static enum pipe_error
emit_fs_consts(struct svga_context *svga, unsigned dirty)
{
   struct svga_screen *ss = svga_screen(svga->pipe.screen);
   const struct svga_shader_variant *variant = svga->state.hw_draw.fs;
   enum pipe_error ret = PIPE_OK;

   /* SVGA_NEW_FS_VARIANT
    */
   if (variant == NULL)
      return PIPE_OK;

   /* SVGA_NEW_FS_CONST_BUFFER
    */
   ret = emit_consts( svga, PIPE_SHADER_FRAGMENT );
   if (ret != PIPE_OK)
      return ret;

   /* emit extra shader constants */
   {
      unsigned offset = variant->shader->info.file_max[TGSI_FILE_CONSTANT] + 1;
      float extras[MAX_EXTRA_CONSTS][4];
      unsigned count, i;

      count = svga_get_extra_fs_constants(svga, (float *) extras);

      if (ss->hw_version >= SVGA3D_HWVERSION_WS8_B1) {
         ret = emit_const_range(svga, PIPE_SHADER_FRAGMENT, offset, count,
                                (const float (*) [4])extras);
      } else {
         for (i = 0; i < count; i++) {
            ret = emit_const(svga, PIPE_SHADER_FRAGMENT, offset + i, extras[i]);
            if (ret != PIPE_OK)
               return ret;
         }
      }
   }

   return ret;
}


struct svga_tracked_state svga_hw_fs_constants =
{
   "hw fs params",
   (SVGA_NEW_FS_CONST_BUFFER |
    SVGA_NEW_FS_VARIANT |
    SVGA_NEW_TEXTURE_BINDING),
   emit_fs_consts
};



static enum pipe_error
emit_vs_consts(struct svga_context *svga, unsigned dirty)
{
   struct svga_screen *ss = svga_screen(svga->pipe.screen);
   const struct svga_shader_variant *variant = svga->state.hw_draw.vs;
   enum pipe_error ret = PIPE_OK;

   /* SVGA_NEW_VS_VARIANT
    */
   if (variant == NULL)
      return PIPE_OK;

   /* SVGA_NEW_VS_CONST_BUFFER
    */
   ret = emit_consts( svga, PIPE_SHADER_VERTEX );
   if (ret != PIPE_OK)
      return ret;

   /* emit extra shader constants */
   {
      unsigned offset = variant->shader->info.file_max[TGSI_FILE_CONSTANT] + 1;
      float extras[MAX_EXTRA_CONSTS][4];
      unsigned count, i;

      count = svga_get_extra_vs_constants(svga, (float *) extras);
      assert(count <= Elements(extras));

      if (ss->hw_version >= SVGA3D_HWVERSION_WS8_B1) {
         ret = emit_const_range(svga, PIPE_SHADER_VERTEX, offset, count,
                                (const float (*) [4]) extras);
      } else {
         for (i = 0; i < count; i++) {
            ret = emit_const(svga, PIPE_SHADER_VERTEX, offset + i, extras[i]);
            if (ret != PIPE_OK)
               return ret;
         }
      }
   }

   return ret;
}


struct svga_tracked_state svga_hw_vs_constants =
{
   "hw vs params",
   (SVGA_NEW_PRESCALE |
    SVGA_NEW_VS_CONST_BUFFER |
    SVGA_NEW_VS_VARIANT),
   emit_vs_consts
};
