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
#include "i915_winsys.h"
#include "i915_batch.h"
#include "i915_reg.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

static unsigned translate_format( unsigned format )
{
   switch (format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      return COLOR_BUF_ARGB8888;
   case PIPE_FORMAT_U_R5_G6_B5:
      return COLOR_BUF_RGB565;
   default:
      assert(0);
      return 0;
   }
}

static unsigned translate_depth_format( unsigned zformat )
{
   assert(zformat == PIPE_FORMAT_S8_Z24);
   return DEPTH_FRMT_24_FIXED_8_OTHER;
}


/* Push the state into the sarea and/or texture memory.
 */
void
i915_emit_hardware_state(struct i915_context *i915 )
{
   BEGIN_BATCH(100, 10);

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
   

   if (i915->hardware_dirty & I915_HW_IMMEDIATE)
   {
      OUT_BATCH(_3DSTATE_LOAD_STATE_IMMEDIATE_1 | 
		I1_LOAD_S(2) |
		I1_LOAD_S(4) |
		I1_LOAD_S(5) |
		I1_LOAD_S(6) | 
		(3));
      
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S2]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S4]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S5]);
      OUT_BATCH(i915->current.immediate[I915_IMMEDIATE_S6]);
   }
   

   if (i915->hardware_dirty & I915_HW_DYNAMIC) 
   {
      int i;
      for (i = 0; i < I915_MAX_DYNAMIC; i++) {
	 OUT_BATCH(i915->current.dynamic[i]);
      }
   }

   if (i915->hardware_dirty & I915_HW_STATIC)
   {
      if (i915->framebuffer.cbufs[0]) {
	 struct pipe_region *cbuf_region = i915->framebuffer.cbufs[0]->region;
	 unsigned pitch = (cbuf_region->pitch *
			   cbuf_region->cpp);

	 OUT_BATCH(_3DSTATE_BUF_INFO_CMD);

	 OUT_BATCH(BUF_3D_ID_COLOR_BACK | 
		   BUF_3D_PITCH(pitch) |  /* pitch in bytes */
		   BUF_3D_USE_FENCE);

	 OUT_RELOC(cbuf_region->buffer,
		   I915_BUFFER_ACCESS_WRITE,
		   0);
      }

      /* What happens if no zbuf??
       */
      if (i915->framebuffer.zbuf) {
	 struct pipe_region *depth_region = i915->framebuffer.zbuf->region;
	 unsigned zpitch = (depth_region->pitch *
			    depth_region->cpp);
			 
	 OUT_BATCH(_3DSTATE_BUF_INFO_CMD);

	 OUT_BATCH(BUF_3D_ID_DEPTH |
		   BUF_3D_PITCH(zpitch) |  /* pitch in bytes */
		   BUF_3D_USE_FENCE);

	 OUT_RELOC(depth_region->buffer,
		   I915_BUFFER_ACCESS_WRITE,
		   0);
      }
   
      {
	 unsigned cformat = translate_format( i915->framebuffer.cbufs[0]->format );
	 unsigned zformat = 0;
      
	 if (i915->framebuffer.zbuf) 
	    zformat = translate_depth_format( i915->framebuffer.zbuf->format );

	 OUT_BATCH(_3DSTATE_DST_BUF_VARS_CMD);

	 OUT_BATCH(DSTORG_HORT_BIAS(0x8) | /* .5 */
		   DSTORG_VERT_BIAS(0x8) | /* .5 */
		   LOD_PRECLAMP_OGL |
		   TEX_DEFAULT_COLOR_OGL |
		   cformat |
		   zformat );
      }
   }

   /* constants */
   if (i915->hardware_dirty & I915_HW_PROGRAM)
   {
      const uint nr = i915->current.num_constants;
      if (nr > 0) {
         const uint *c = (const uint *) i915->current.constants;
         uint i;
         OUT_BATCH( _3DSTATE_PIXEL_SHADER_CONSTANTS | (nr * 4) );
         OUT_BATCH( (1 << (nr - 1)) | ((1 << (nr - 1)) - 1) );
         for (i = 0; i < nr; i++) {
            OUT_BATCH(*c++);
            OUT_BATCH(*c++);
            OUT_BATCH(*c++);
            OUT_BATCH(*c++);
         }
      }
   }

   /* Fragment program */
   if (i915->hardware_dirty & I915_HW_PROGRAM)
   {
      uint i;
      /* we should always have, at least, a pass-through program */
      assert(i915->current.program_len > 0);
      for (i = 0; i < i915->current.program_len; i++) {
         OUT_BATCH(i915->current.program[i]);
      }
   }

   /* drawing surface size */
   {
      int w = i915->framebuffer.cbufs[0]->width;
      int h = i915->framebuffer.cbufs[0]->height;

      OUT_BATCH(_3DSTATE_DRAW_RECT_CMD);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(((w - 1) & 0xffff) | ((h - 1) << 16));
      OUT_BATCH(0);
      OUT_BATCH(0);
   }


   i915->hardware_dirty = 0;
}

