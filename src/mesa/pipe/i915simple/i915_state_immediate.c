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
   {
      LIS2 = i915->current.vertex_info.hwfmt[1];
      LIS4 = i915->current.vertex_info.hwfmt[0];
      /*
      printf("LIS2: 0x%x  LIS4: 0x%x\n", LIS2, LIS4);
      */
      assert(LIS4); /* should never be zero? */
   }

   LIS4 |= i915->rasterizer->LIS4;

   if (LIS2 != i915->current.immediate[I915_IMMEDIATE_S2] ||
       LIS4 != i915->current.immediate[I915_IMMEDIATE_S4]) {

      i915->current.immediate[I915_IMMEDIATE_S2] = LIS2;
      i915->current.immediate[I915_IMMEDIATE_S4] = LIS4;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}


const struct i915_tracked_state i915_upload_S2S4 = {
   .dirty = I915_NEW_RASTERIZER | I915_NEW_VERTEX_FORMAT,
   .update = upload_S2S4
};



/***********************************************************************
 * 
 */
static void upload_S5( struct i915_context *i915 )
{
   unsigned LIS5 = 0;

   LIS5 |= i915->depth_stencil->stencil_LIS5;

   LIS5 |= i915->blend->LIS5;

#if 0
   /* I915_NEW_RASTERIZER */
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
   .dirty = (I915_NEW_DEPTH_STENCIL | I915_NEW_BLEND | I915_NEW_RASTERIZER),
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
   LIS6 |= i915->alpha_test->LIS6;

   /* I915_NEW_BLEND
    */
   LIS6 |= i915->blend->LIS6;

   /* I915_NEW_DEPTH
    */
   LIS6 |= i915->depth_stencil->depth_LIS6;

   if (LIS6 != i915->current.immediate[I915_IMMEDIATE_S6]) {
      i915->current.immediate[I915_IMMEDIATE_S6] = LIS6;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}

const struct i915_tracked_state i915_upload_S6 = {
   .dirty = I915_NEW_ALPHA_TEST | I915_NEW_BLEND | I915_NEW_DEPTH_STENCIL,
   .update = upload_S6
};


/***********************************************************************
 */
static void upload_S7( struct i915_context *i915 )
{
   float LIS7;

   /* I915_NEW_RASTERIZER
    */
   LIS7 = i915->rasterizer->LIS7; /* probably incorrect */

   if (LIS7 != i915->current.immediate[I915_IMMEDIATE_S7]) {
      i915->current.immediate[I915_IMMEDIATE_S7] = LIS7;
      i915->hardware_dirty |= I915_HW_IMMEDIATE;
   }
}

const struct i915_tracked_state i915_upload_S7 = {
   .dirty = I915_NEW_RASTERIZER,
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
