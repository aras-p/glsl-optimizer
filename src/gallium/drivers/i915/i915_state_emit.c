/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "i915_reg.h"
#include "i915_context.h"
#include "i915_batch.h"
#include "i915_reg.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

static unsigned translate_format( enum pipe_format format )
{
   switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return COLOR_BUF_ARGB8888;
   case PIPE_FORMAT_B5G6R5_UNORM:
      return COLOR_BUF_RGB565;
   default:
      assert(0);
      return 0;
   }
}

static unsigned translate_depth_format( enum pipe_format zformat )
{
   switch (zformat) {
   case PIPE_FORMAT_Z24S8_UNORM:
      return DEPTH_FRMT_24_FIXED_8_OTHER;
   case PIPE_FORMAT_Z16_UNORM:
      return DEPTH_FRMT_16_FIXED;
   default:
      assert(0);
      return 0;
   }
}


/**
 * Examine framebuffer state to determine width, height.
 */
static boolean
framebuffer_size(const struct pipe_framebuffer_state *fb,
                 uint *width, uint *height)
{
   if (fb->cbufs[0]) {
      *width = fb->cbufs[0]->width;
      *height = fb->cbufs[0]->height;
      return TRUE;
   }
   else if (fb->zsbuf) {
      *width = fb->zsbuf->width;
      *height = fb->zsbuf->height;
      return TRUE;
   }
   else {
      *width = *height = 0;
      return FALSE;
   }
}


/* Push the state into the sarea and/or texture memory.
 */
void
i915_emit_hardware_state(struct i915_context *i915 )
{
   /* XXX: there must be an easier way */
   const unsigned dwords = ( 14 + 
                             7 + 
                             I915_MAX_DYNAMIC + 
                             8 + 
                             2 + I915_TEX_UNITS*3 + 
                             2 + I915_TEX_UNITS*3 +
                             2 + I915_MAX_CONSTANT*4 + 
#if 0
                             i915->current.program_len + 
#else
                             i915->fs->program_len + 
#endif
                             6 
                           ) * 3/2; /* plus 50% margin */
   const unsigned relocs = ( I915_TEX_UNITS +
                             3
                           ) * 3/2; /* plus 50% margin */

#if 0
   debug_printf("i915_emit_hardware_state: %d dwords, %d relocs\n", dwords, relocs);
#endif
   
   if(!BEGIN_BATCH(dwords, relocs)) {
      FLUSH_BATCH(NULL);
      assert(BEGIN_BATCH(dwords, relocs));
   }

   /* 14 dwords, 0 relocs */
   if (i915->hardware_dirty & I915_HW_INVARIENT)
   {
      OUT_BATCH(_3DSTATE_AA_CMD |
                AA_LINE_ECAAR_WIDTH_ENABLE |
                AA_LINE_ECAAR_WIDTH_1_0 |
                AA_LINE_REGION_WIDTH_ENABLE | AA_LINE_REGION_WIDTH_1_0);

      OUT_BATCH(_3DSTATE_DFLT_DIFFUSE_CMD);
      OUT_BATCH(0);

      OUT_BATCH(_3DSTATE_DFLT_SPEC_CMD);
      OUT_BATCH(0);
      
      OUT_BATCH(_3DSTATE_DFLT_Z_CMD);
      OUT_BATCH(0);

      OUT_BATCH(_3DSTATE_COORD_SET_BINDINGS |
                CSB_TCB(0, 0) |
                CSB_TCB(1, 1) |
                CSB_TCB(2, 2) |
                CSB_TCB(3, 3) |
                CSB_TCB(4, 4) | 
                CSB_TCB(5, 5) | 
                CSB_TCB(6, 6) | 
                CSB_TCB(7, 7));

      OUT_BATCH(_3DSTATE_RASTER_RULES_CMD |
                ENABLE_POINT_RASTER_RULE |
                OGL_POINT_RASTER_RULE |
                ENABLE_LINE_STRIP_PROVOKE_VRTX |
                ENABLE_TRI_FAN_PROVOKE_VRTX |
                LINE_STRIP_PROVOKE_VRTX(1) |
                TRI_FAN_PROVOKE_VRTX(2) | 
                ENABLE_TEXKILL_3D_4D | 
                TEXKILL_4D);

      /* Need to initialize this to zero.
       */
      OUT_BATCH(_3DSTATE_LOAD_STATE_IMMEDIATE_1 | I1_LOAD_S(3) | (0));
      OUT_BATCH(0);

      OUT_BATCH(_3DSTATE_DEPTH_SUBRECT_DISABLE);

      /* disable indirect state for now
       */
      OUT_BATCH(_3DSTATE_LOAD_INDIRECT | 0);
      OUT_BATCH(0);
   }
   
   /* 7 dwords, 1 relocs */
   if (i915->hardware_dirty & I915_HW_IMMEDIATE)
   {
      OUT_BATCH(_3DSTATE_LOAD_STATE_IMMEDIATE_1 | 
                I1_LOAD_S(0) |
                I1_LOAD_S(1) |
                I1_LOAD_S(2) |
                I1_LOAD_S(4) |
                I1_LOAD_S(5) |
                I1_LOAD_S(6) | 
                (5));
      
      if(i915->vbo)
         OUT_RELOC(i915->vbo,
                   INTEL_USAGE_VERTEX,
                   i915->current.immediate[I915_IMMEDIATE_S0]);
      else
         /* FIXME: we should not do this */
         OUT_BATCH(0);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S1]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S2]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S4]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S5]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S6]);
   } 
   
   /* I915_MAX_DYNAMIC dwords, 0 relocs */
   if (i915->hardware_dirty & I915_HW_DYNAMIC) 
   {
      int i;
      for (i = 0; i < I915_MAX_DYNAMIC; i++) {
         OUT_BATCH(i915->current.dynamic[i]);
      }
   }
   
   /* 8 dwords, 2 relocs */
   if (i915->hardware_dirty & I915_HW_STATIC)
   {
      struct pipe_surface *cbuf_surface = i915->framebuffer.cbufs[0];
      struct pipe_surface *depth_surface = i915->framebuffer.zsbuf;

      if (cbuf_surface) {
         unsigned ctile = BUF_3D_USE_FENCE;
         struct i915_texture *tex = (struct i915_texture *)
                                    cbuf_surface->texture;
         assert(tex);

         if (tex && tex->sw_tiled) {
            ctile = BUF_3D_TILED_SURFACE;
         }

         OUT_BATCH(_3DSTATE_BUF_INFO_CMD);

         OUT_BATCH(BUF_3D_ID_COLOR_BACK |
                   BUF_3D_PITCH(tex->stride) |  /* pitch in bytes */
                   ctile);

         OUT_RELOC(tex->buffer,
                   INTEL_USAGE_RENDER,
                   cbuf_surface->offset);
      }

      /* What happens if no zbuf??
       */
      if (depth_surface) {
         unsigned ztile = BUF_3D_USE_FENCE;
         struct i915_texture *tex = (struct i915_texture *)
                                    depth_surface->texture;
         assert(tex);

         if (tex && tex->sw_tiled) {
            ztile = BUF_3D_TILED_SURFACE;
         }

         OUT_BATCH(_3DSTATE_BUF_INFO_CMD);

         assert(tex);
         OUT_BATCH(BUF_3D_ID_DEPTH |
                   BUF_3D_PITCH(tex->stride) |  /* pitch in bytes */
                   ztile);

         OUT_RELOC(tex->buffer,
                   INTEL_USAGE_RENDER,
                   depth_surface->offset);
      }
   
      {
         unsigned cformat, zformat = 0;
      
         if (cbuf_surface)
            cformat = cbuf_surface->format;
         else
            cformat = PIPE_FORMAT_B8G8R8A8_UNORM; /* arbitrary */
         cformat = translate_format(cformat);

         if (depth_surface) 
            zformat = translate_depth_format( i915->framebuffer.zsbuf->format );

         OUT_BATCH(_3DSTATE_DST_BUF_VARS_CMD);
         OUT_BATCH(DSTORG_HORT_BIAS(0x8) | /* .5 */
                   DSTORG_VERT_BIAS(0x8) | /* .5 */
                   LOD_PRECLAMP_OGL |
                   TEX_DEFAULT_COLOR_OGL |
                   cformat |
                   zformat );
      }
   }

#if 01
      /* texture images */
      /* 2 + I915_TEX_UNITS*3 dwords, I915_TEX_UNITS relocs */
      if (i915->hardware_dirty & (I915_HW_MAP | I915_HW_SAMPLER))
      {
         const uint nr = i915->current.sampler_enable_nr;
         if (nr) {
            const uint enabled = i915->current.sampler_enable_flags;
            uint unit;
            uint count = 0;
            OUT_BATCH(_3DSTATE_MAP_STATE | (3 * nr));
            OUT_BATCH(enabled);
            for (unit = 0; unit < I915_TEX_UNITS; unit++) {
               if (enabled & (1 << unit)) {
                  struct intel_buffer *buf = i915->texture[unit]->buffer;
                  uint offset = 0;
                  assert(buf);

                  count++;

                  OUT_RELOC(buf, INTEL_USAGE_SAMPLER, offset);
                  OUT_BATCH(i915->current.texbuffer[unit][0]); /* MS3 */
                  OUT_BATCH(i915->current.texbuffer[unit][1]); /* MS4 */
               }
            }
            assert(count == nr);
         }
      }
#endif

#if 01
   /* samplers */
   /* 2 + I915_TEX_UNITS*3 dwords, 0 relocs */
   if (i915->hardware_dirty & I915_HW_SAMPLER) 
   {
      if (i915->current.sampler_enable_nr) {
         int i;
         
         OUT_BATCH( _3DSTATE_SAMPLER_STATE | 
                    (3 * i915->current.sampler_enable_nr) );

         OUT_BATCH( i915->current.sampler_enable_flags );

         for (i = 0; i < I915_TEX_UNITS; i++) {
            if (i915->current.sampler_enable_flags & (1<<i)) {
               OUT_BATCH( i915->current.sampler[i][0] );
               OUT_BATCH( i915->current.sampler[i][1] );
               OUT_BATCH( i915->current.sampler[i][2] );
            }
         }
      }
   }
#endif

   /* constants */
   /* 2 + I915_MAX_CONSTANT*4 dwords, 0 relocs */
   if (i915->hardware_dirty & I915_HW_PROGRAM)
   {
      /* Collate the user-defined constants with the fragment shader's
       * immediates according to the constant_flags[] array.
       */
      const uint nr = i915->fs->num_constants;
      if (nr) {
         uint i;

         OUT_BATCH( _3DSTATE_PIXEL_SHADER_CONSTANTS | (nr * 4) );
         OUT_BATCH( (1 << (nr - 1)) | ((1 << (nr - 1)) - 1) );

         for (i = 0; i < nr; i++) {
            const uint *c;
            if (i915->fs->constant_flags[i] == I915_CONSTFLAG_USER) {
               /* grab user-defined constant */
               c = (uint *) i915->current.constants[PIPE_SHADER_FRAGMENT][i];
            }
            else {
               /* emit program constant */
               c = (uint *) i915->fs->constants[i];
            }
#if 0 /* debug */
            {
               float *f = (float *) c;
               printf("Const %2d: %f %f %f %f %s\n", i, f[0], f[1], f[2], f[3],
                      (i915->fs->constant_flags[i] == I915_CONSTFLAG_USER
                       ? "user" : "immediate"));
            }
#endif
            OUT_BATCH(*c++);
            OUT_BATCH(*c++);
            OUT_BATCH(*c++);
            OUT_BATCH(*c++);
         }
      }
   }

   /* Fragment program */
   /* i915->current.program_len dwords, 0 relocs */
   if (i915->hardware_dirty & I915_HW_PROGRAM)
   {
      uint i;
      /* we should always have, at least, a pass-through program */
      assert(i915->fs->program_len > 0);
      for (i = 0; i < i915->fs->program_len; i++) {
         OUT_BATCH(i915->fs->program[i]);
      }
   }

   /* drawing surface size */
   /* 6 dwords, 0 relocs */
   {
      uint w, h;
      boolean k = framebuffer_size(&i915->framebuffer, &w, &h);
      (void)k;
      assert(k);

      OUT_BATCH(_3DSTATE_DRAW_RECT_CMD);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(((w - 1) & 0xffff) | ((h - 1) << 16));
      OUT_BATCH(0);
      OUT_BATCH(0);
   }


   i915->hardware_dirty = 0;
}
