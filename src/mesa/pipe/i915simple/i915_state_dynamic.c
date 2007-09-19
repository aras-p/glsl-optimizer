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

#include "i915_batch.h"
#include "i915_state_inlines.h"
#include "i915_context.h"
#include "i915_reg.h"
#include "i915_state.h"
#include "pipe/p_util.h"

#define FILE_DEBUG_FLAG DEBUG_STATE

/* State that we have chosen to store in the DYNAMIC segment of the
 * i915 indirect state mechanism.  
 *
 * Can't cache these in the way we do the static state, as there is no
 * start/size in the command packet, instead an 'end' value that gets
 * incremented.
 *
 * Additionally, there seems to be a requirement to re-issue the full
 * (active) state every time a 4kb boundary is crossed.
 */

static inline void set_dynamic_indirect( struct i915_context *i915,
					 unsigned offset,
					 const unsigned *src,
					 unsigned dwords )
{
   int i;

   for (i = 0; i < dwords; i++)
      i915->current.dynamic[offset + i] = src[i];

   i915->hardware_dirty |= I915_HW_DYNAMIC;
}


/***********************************************************************
 * Modes4: stencil masks and logicop 
 */
static void upload_MODES4( struct i915_context *i915 )
{
   unsigned modes4 = 0;

   /* I915_NEW_STENCIL */
   {
      int testmask = i915->depth_stencil->stencil.value_mask[0] & 0xff;
      int writemask = i915->depth_stencil->stencil.write_mask[0] & 0xff;

      modes4 |= (_3DSTATE_MODES_4_CMD |
		 ENABLE_STENCIL_TEST_MASK |
		 STENCIL_TEST_MASK(testmask) |
		 ENABLE_STENCIL_WRITE_MASK |
		 STENCIL_WRITE_MASK(writemask));
   }

   /* I915_NEW_BLEND */
   modes4 |= i915->blend->modes4;

   /* Always, so that we know when state is in-active: 
    */
   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_MODES4,
			 &modes4,
			 1 );
}

const struct i915_tracked_state i915_upload_MODES4 = {
   .dirty = I915_NEW_BLEND | I915_NEW_DEPTH_STENCIL,
   .update = upload_MODES4
};




/***********************************************************************
 */

static void upload_BFO( struct i915_context *i915 )
{
   unsigned bf[2];

   memset( bf, 0, sizeof(bf) );

   /* _NEW_STENCIL 
    */
   if (i915->depth_stencil->stencil.back_enabled) {
      int test  = i915_translate_compare_func(i915->depth_stencil->stencil.back_func);
      int fop   = i915_translate_stencil_op(i915->depth_stencil->stencil.back_fail_op);
      int dfop  = i915_translate_stencil_op(i915->depth_stencil->stencil.back_zfail_op);
      int dpop  = i915_translate_stencil_op(i915->depth_stencil->stencil.back_zpass_op);
      int ref   = i915->depth_stencil->stencil.ref_value[1] & 0xff;
      int tmask = i915->depth_stencil->stencil.value_mask[1] & 0xff;
      int wmask = i915->depth_stencil->stencil.write_mask[1] & 0xff;
      
      bf[0] = (_3DSTATE_BACKFACE_STENCIL_OPS |
	       BFO_ENABLE_STENCIL_FUNCS |
	       BFO_ENABLE_STENCIL_TWO_SIDE |
	       BFO_ENABLE_STENCIL_REF |
	       BFO_STENCIL_TWO_SIDE |
	       (ref  << BFO_STENCIL_REF_SHIFT) |
	       (test << BFO_STENCIL_TEST_SHIFT) |
	       (fop  << BFO_STENCIL_FAIL_SHIFT) |
	       (dfop << BFO_STENCIL_PASS_Z_FAIL_SHIFT) |
	       (dpop << BFO_STENCIL_PASS_Z_PASS_SHIFT));

      bf[1] = (_3DSTATE_BACKFACE_STENCIL_MASKS |
	       BFM_ENABLE_STENCIL_TEST_MASK |
	       BFM_ENABLE_STENCIL_WRITE_MASK |
	       (tmask << BFM_STENCIL_TEST_MASK_SHIFT) |
	       (wmask << BFM_STENCIL_WRITE_MASK_SHIFT));
   }
   else {
      /* This actually disables two-side stencil: The bit set is a
       * modify-enable bit to indicate we are changing the two-side
       * setting.  Then there is a symbolic zero to show that we are
       * setting the flag to zero/off.
       */
      bf[0] = (_3DSTATE_BACKFACE_STENCIL_OPS |
	       BFO_ENABLE_STENCIL_TWO_SIDE |
	       0);
      bf[1] = 0;
   }      

   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_BFO_0,
			 &bf[0],
			 2 );
}

const struct i915_tracked_state i915_upload_BFO = {
   .dirty = I915_NEW_DEPTH_STENCIL,
   .update = upload_BFO
};


/***********************************************************************
 */


static void upload_BLENDCOLOR( struct i915_context *i915 )
{
   unsigned bc[2];

   memset( bc, 0, sizeof(bc) );

   /* I915_NEW_BLEND {_COLOR} 
    */
   {
      const float *color = i915->blend_color.color;

      bc[0] = _3DSTATE_CONST_BLEND_COLOR_CMD;
      bc[1] = pack_ui32_float4( color[0],
				color[1],
				color[2], 
				color[3] );
   }

   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_BC_0,
			 bc,
			 2 );
}

const struct i915_tracked_state i915_upload_BLENDCOLOR = {
   .dirty = I915_NEW_BLEND,
   .update = upload_BLENDCOLOR
};

/***********************************************************************
 */


static void upload_IAB( struct i915_context *i915 )
{
   unsigned iab = i915->blend->iab;


   set_dynamic_indirect( i915,
			 I915_DYNAMIC_IAB,
			 &iab,
			 1 );
}

const struct i915_tracked_state i915_upload_IAB = {
   .dirty = I915_NEW_BLEND,
   .update = upload_IAB
};


/***********************************************************************
 */



static void upload_DEPTHSCALE( struct i915_context *i915 )
{
   union { float f; unsigned u; } ds[2];

   memset( ds, 0, sizeof(ds) );
   
   /* I915_NEW_RASTERIZER
    */
   ds[0].u = _3DSTATE_DEPTH_OFFSET_SCALE;
   ds[1].f = i915->rasterizer->offset_scale;

   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_DEPTHSCALE_0,
			 &ds[0].u,
			 2 );
}

const struct i915_tracked_state i915_upload_DEPTHSCALE = {
   .dirty = I915_NEW_RASTERIZER,
   .update = upload_DEPTHSCALE
};



/***********************************************************************
 * Polygon stipple
 *
 * The i915 supports a 4x4 stipple natively, GL wants 32x32.
 * Fortunately stipple is usually a repeating pattern.
 *
 * XXX: does stipple pattern need to be adjusted according to
 * the window position?
 *
 * XXX: possibly need workaround for conform paths test. 
 */

static void upload_STIPPLE( struct i915_context *i915 )
{
   unsigned st[2];

   st[0] = _3DSTATE_STIPPLE;
   st[1] = 0;
   
   /* I915_NEW_RASTERIZER 
    */
   if (i915->rasterizer->poly_stipple_enable) {
      st[1] |= ST1_ENABLE;
   }


   /* I915_NEW_STIPPLE
    */
   {
      const ubyte *mask = (const ubyte *)i915->poly_stipple.stipple;
      ubyte p[4];

      p[0] = mask[12] & 0xf;
      p[1] = mask[8] & 0xf;
      p[2] = mask[4] & 0xf;
      p[3] = mask[0] & 0xf;

      /* Not sure what to do about fallbacks, so for now just dont:
       */
      st[1] |= ((p[0] << 0) |
		(p[1] << 4) |
		(p[2] << 8) | 
		(p[3] << 12));
   }


   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_STP_0,
			 &st[0],
			 2 );
}


const struct i915_tracked_state i915_upload_STIPPLE = {
   .dirty = I915_NEW_RASTERIZER | I915_NEW_STIPPLE,
   .update = upload_STIPPLE
};



/***********************************************************************
 * Scissor.  
 */
static void upload_SCISSOR_ENABLE( struct i915_context *i915 )
{
   unsigned sc[1];

   if (i915->rasterizer->scissor) 
      sc[0] = _3DSTATE_SCISSOR_ENABLE_CMD | ENABLE_SCISSOR_RECT;
   else
      sc[0] = _3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT;

   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_SC_ENA_0,
			 &sc[0],
			 1 );
}

const struct i915_tracked_state i915_upload_SCISSOR_ENABLE = {
   .dirty = I915_NEW_RASTERIZER,
   .update = upload_SCISSOR_ENABLE
};



static void upload_SCISSOR_RECT( struct i915_context *i915 )
{
   unsigned x1 = i915->scissor.minx;
   unsigned y1 = i915->scissor.miny;
   unsigned x2 = i915->scissor.maxx;
   unsigned y2 = i915->scissor.maxy;
   unsigned sc[3];
 
   sc[0] = _3DSTATE_SCISSOR_RECT_0_CMD;
   sc[1] = (y1 << 16) | (x1 & 0xffff);
   sc[2] = (y2 << 16) | (x2 & 0xffff);

   set_dynamic_indirect( i915, 
			 I915_DYNAMIC_SC_RECT_0,
			 &sc[0],
			 3 );
}


const struct i915_tracked_state i915_upload_SCISSOR_RECT = {
   .dirty = I915_NEW_SCISSOR,
   .update = upload_SCISSOR_RECT
};






static const struct i915_tracked_state *atoms[] = {
   &i915_upload_MODES4,
   &i915_upload_BFO,
   &i915_upload_BLENDCOLOR,
   &i915_upload_IAB,
   &i915_upload_DEPTHSCALE,
   &i915_upload_STIPPLE,
   &i915_upload_SCISSOR_ENABLE,
   &i915_upload_SCISSOR_RECT
};

/* These will be dynamic indirect state commands, but for now just end
 * up on the batch buffer with everything else.
 */
void i915_update_dynamic( struct i915_context *i915 )
{
   int i;

   for (i = 0; i < Elements(atoms); i++)
      if (i915->dirty & atoms[i]->dirty)
	 atoms[i]->update( i915 );
}

