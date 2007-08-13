/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 
//#include "macros.h"

#include "i915_state_inlines.h"
#include "i915_context.h"
#include "i915_state.h"
#include "i915_reg.h"
#include "p_util.h"


/* All state expressable with the LOAD_STATE_IMMEDIATE_1 packet.
 * Would like to opportunistically recombine all these fragments into
 * a single packet containing only what has changed, but for now emit
 * as multiple packets.
 */




/***********************************************************************
 * S4: Vertex format, rasterization state
 */
static void upload_S2S4(struct i915_context *i915)
{
   unsigned LIS2, LIS4;
   
   /* I915_NEW_VERTEX_FORMAT */
   LIS2 = 0xffffffff;
   LIS4 = (S4_VFMT_XYZ | S4_VFMT_COLOR);


   /* I915_NEW_SETUP */
   switch (i915->setup.cull_mode) {
   case PIPE_WINDING_NONE:
      LIS4 |= S4_CULLMODE_NONE;
      break;
   case PIPE_WINDING_CW:
      LIS4 |= S4_CULLMODE_CW;
      break;
   case PIPE_WINDING_CCW:
      LIS4 |= S4_CULLMODE_CCW;
      break;
   case PIPE_WINDING_BOTH:
      LIS4 |= S4_CULLMODE_BOTH;
      break;
   }

   /* I915_NEW_SETUP */
   {
      int line_width = CLAMP((int)(i915->setup.line_width * 2), 1, 0xf);

      LIS4 |= line_width << S4_LINE_WIDTH_SHIFT;

      if (i915->setup.line_smooth)
	 LIS4 |= S4_LINE_ANTIALIAS_ENABLE;
   }

   /* I915_NEW_SETUP */
   {
      int point_size = CLAMP((int) i915->setup.point_size, 1, 0xff);

      LIS4 |= point_size << S4_POINT_WIDTH_SHIFT;
   }

   /* I915_NEW_SETUP */
   if (i915->setup.flatshade) {
      LIS4 |= (S4_FLATSHADE_ALPHA |
	       S4_FLATSHADE_COLOR |
	       S4_FLATSHADE_SPECULAR);
   }


   if (LIS2 != i915->current.immediate[I915_IMMEDIATE_S2] ||
       LIS4 != i915->current.immediate[I915_IMMEDIATE_S4]) {

      i915->current.immediate[I915_IMMEDIATE_S2] = LIS2;
      i915->current.immediate[I915_IMMEDIATE_S4] = LIS4;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}


const struct i915_tracked_state i915_upload_S2S4 = {
   .dirty = I915_NEW_SETUP | I915_NEW_VERTEX_FORMAT,
   .update = upload_S2S4
};



/***********************************************************************
 * 
 */
static void upload_S5( struct i915_context *i915 )
{
   unsigned LIS5 = 0;

   /* I915_NEW_STENCIL */
   if (i915->stencil.front_enabled) {
      int test = i915_translate_compare_func(i915->stencil.front_func);
      int fop = i915_translate_stencil_op(i915->stencil.front_fail_op);
      int dfop = i915_translate_stencil_op(i915->stencil.front_zfail_op);
      int dpop = i915_translate_stencil_op(i915->stencil.front_zpass_op);
      int ref = i915->stencil.ref_value[0] & 0xff;
      
      LIS5 |= (S5_STENCIL_TEST_ENABLE |
	       S5_STENCIL_WRITE_ENABLE |
	       (ref  << S5_STENCIL_REF_SHIFT) |
	       (test << S5_STENCIL_TEST_FUNC_SHIFT) |
	       (fop  << S5_STENCIL_FAIL_SHIFT) |
	       (dfop << S5_STENCIL_PASS_Z_FAIL_SHIFT) |
	       (dpop << S5_STENCIL_PASS_Z_PASS_SHIFT));
   }

   /* I915_NEW_BLEND */
   if (i915->blend.logicop_enable) 
      LIS5 |= S5_LOGICOP_ENABLE;

   if (i915->blend.dither) 
      LIS5 |= S5_COLOR_DITHER_ENABLE;

   if ((i915->blend.colormask & PIPE_MASK_R) == 0)
      LIS5 |= S5_WRITEDISABLE_RED;

   if ((i915->blend.colormask & PIPE_MASK_G) == 0)
      LIS5 |= S5_WRITEDISABLE_GREEN;
   
   if ((i915->blend.colormask & PIPE_MASK_B) == 0)
      LIS5 |= S5_WRITEDISABLE_BLUE;

   if ((i915->blend.colormask & PIPE_MASK_A) == 0)
      LIS5 |= S5_WRITEDISABLE_ALPHA;


#if 0
   /* I915_NEW_SETUP */
   if (i915->state.Polygon->OffsetFill) {
      LIS5 |= S5_GLOBAL_DEPTH_OFFSET_ENABLE;
   }
#endif


   if (LIS5 != i915->current.immediate[I915_IMMEDIATE_S5]) {
      i915->current.immediate[I915_IMMEDIATE_S5] = LIS5;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}

const struct i915_tracked_state i915_upload_S5 = {
   .dirty = (I915_NEW_STENCIL | I915_NEW_BLEND | I915_NEW_SETUP),
   .update = upload_S5
};


/***********************************************************************
 */
static void upload_S6( struct i915_context *i915 )
{
   unsigned LIS6 = (S6_COLOR_WRITE_ENABLE |
		  (2 << S6_TRISTRIP_PV_SHIFT));

   /* I915_NEW_ALPHA_TEST
    */
   if (i915->alpha_test.enabled) {
      int test = i915_translate_compare_func(i915->alpha_test.func);
      ubyte refByte = float_to_ubyte(i915->alpha_test.ref);
      

      LIS6 |= (S6_ALPHA_TEST_ENABLE |
	       (test << S6_ALPHA_TEST_FUNC_SHIFT) |
	       (((unsigned) refByte) << S6_ALPHA_REF_SHIFT));
   }

   /* I915_NEW_BLEND
    */
   if (i915->blend.blend_enable)
   {
      unsigned funcRGB = i915->blend.rgb_func;
      unsigned srcRGB = i915->blend.rgb_src_factor;
      unsigned dstRGB = i915->blend.rgb_dst_factor;
      
      LIS6 |= (S6_CBUF_BLEND_ENABLE |
	       SRC_BLND_FACT(i915_translate_blend_factor(srcRGB)) |
	       DST_BLND_FACT(i915_translate_blend_factor(dstRGB)) |
	       (i915_translate_blend_func(funcRGB) << S6_CBUF_BLEND_FUNC_SHIFT));
   }

   /* I915_NEW_DEPTH 
    */
   if (i915->depth_test.enabled) {
      int func = i915_translate_compare_func(i915->depth_test.func);

      LIS6 |= (S6_DEPTH_TEST_ENABLE |
	       (func << S6_DEPTH_TEST_FUNC_SHIFT));

      if (i915->depth_test.writemask)
	 LIS6 |= S6_DEPTH_WRITE_ENABLE;
   }

   if (LIS6 != i915->current.immediate[I915_IMMEDIATE_S6]) {
      i915->current.immediate[I915_IMMEDIATE_S6] = LIS6;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}

const struct i915_tracked_state i915_upload_S6 = {
   .dirty = I915_NEW_ALPHA_TEST | I915_NEW_BLEND | I915_NEW_DEPTH_TEST,
   .update = upload_S6
};


/***********************************************************************
 */
static void upload_S7( struct i915_context *i915 )
{
   float LIS7;

   /* I915_NEW_SETUP
    */
   LIS7 = i915->setup.offset_units; /* probably incorrect */

   if (LIS7 != i915->current.immediate[I915_IMMEDIATE_S7]) {
      i915->current.immediate[I915_IMMEDIATE_S7] = LIS7;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}

const struct i915_tracked_state i915_upload_S7 = {
   .dirty = I915_NEW_SETUP,
   .update = upload_S7
};


static const struct i915_tracked_state *atoms[] = {
   &i915_upload_S2S4,
   &i915_upload_S5,
   &i915_upload_S6,
   &i915_upload_S7
};

/* 
 */
void i915_update_immediate( struct i915_context *i915 )
{
   int i;

   for (i = 0; i < Elements(atoms); i++)
      if (i915->dirty & atoms[i]->dirty)
	 atoms[i]->update( i915 );
}
