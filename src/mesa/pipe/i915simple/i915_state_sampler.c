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

//#include "mtypes.h"
//#include "enums.h"
//#include "texformat.h"
//#include "macros.h"
//#include "dri_bufmgr.h"


#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "i915_state_inlines.h"
#include "i915_context.h"
#include "i915_reg.h"
//#include "i915_cache.h"






/* The i915 (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static unsigned
translate_wrap_mode(unsigned wrap)
{
   switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:
      return TEXCOORDMODE_WRAP;
   case PIPE_TEX_WRAP_CLAMP:
      return TEXCOORDMODE_CLAMP_EDGE;   /* not quite correct */
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return TEXCOORDMODE_CLAMP_EDGE;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return TEXCOORDMODE_CLAMP_BORDER;
//   case PIPE_TEX_WRAP_MIRRORED_REPEAT:
//      return TEXCOORDMODE_MIRROR;
   default:
      return TEXCOORDMODE_WRAP;
   }
}

static unsigned translate_img_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
      return FILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:
      return FILTER_LINEAR;
   default:
      assert(0);
      return FILTER_NEAREST;
   }
}

static unsigned translate_mip_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_MIPFILTER_NONE:
      return MIPFILTER_NONE;
   case PIPE_TEX_MIPFILTER_NEAREST:
      return MIPFILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:
      return MIPFILTER_LINEAR;
   default:
      assert(0);
      return MIPFILTER_NONE;
   }
}


/* Recalculate all state from scratch.  Perhaps not the most
 * efficient, but this has gotten complex enough that we need
 * something which is understandable and reliable.
 */
static void update_sampler(struct i915_context *i915, 
			   const struct pipe_sampler_state *sampler,
			   const struct pipe_surface_state *surface,
			   unsigned surface_index,
			   unsigned target,
			   unsigned *state )
{

   /* Need to do this after updating the maps, which call the
    * intel_finalize_mipmap_tree and hence can update firstLevel:
    */
   unsigned minFilt, magFilt;
   unsigned mipFilt;

   state[0] = state[1] = state[2] = 0;

   mipFilt = translate_mip_filter( sampler->min_mip_filter );

   if (tObj->MaxAnisotropy > 1.0) {
      minFilt = FILTER_ANISOTROPIC;
      magFilt = FILTER_ANISOTROPIC;
   }
   else {
      minFilt = translate_img_filter( sampler->min_img_filter );
      magFilt = translate_img_filter( sampler->mag_img_filter );
   }

   {
      int b = sampler->lod_bias * 16.0;
      b = CLAMP(b, -256, 255);
      state[0] |= ((b << SS2_LOD_BIAS_SHIFT) & SS2_LOD_BIAS_MASK);
   }

   if (surface->format == MESA_FORMAT_YCBCR ||
       surface->format == MESA_FORMAT_YCBCR_REV)
      state[0] |= SS2_COLORSPACE_CONVERSION;


   /* Shadow:
    */
   if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) 
   {
      state[0] |= (SS2_SHADOW_ENABLE |
		   translate_compare_func(sampler->compare_func));

      minFilt = FILTER_4X4_FLAT;
      magFilt = FILTER_4X4_FLAT;
   }

   state[0] |= ((minFilt << SS2_MIN_FILTER_SHIFT) |
		(mipFilt << SS2_MIP_FILTER_SHIFT) |
		(magFilt << SS2_MAG_FILTER_SHIFT));

   
   if (0)
   {
      unsigned ws = tObj->WrapS;
      unsigned wt = tObj->WrapT;
      unsigned wr = tObj->WrapR;

      /* 3D textures don't seem to respect the border color.
       * Fallback if there's ever a danger that they might refer to
       * it.  
       * 
       * Effectively this means fallback on 3D clamp or
       * clamp_to_border.
       *
       * XXX: Check if this is true on i945.  
       * XXX: Check if this bug got fixed in release silicon.
       */
      if (target == PIPE_TEXTURE_3D &&
          (sampler->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
           sampler->mag_img_filter != PIPE_TEX_FILTER_NEAREST) &&
          (ws == GL_CLAMP ||
           wt == GL_CLAMP ||
           wr == GL_CLAMP ||
           ws == GL_CLAMP_TO_BORDER ||
           wt == GL_CLAMP_TO_BORDER || 
	   wr == GL_CLAMP_TO_BORDER)) {
	 
	 if (intel->strict_conformance) {
	    assert(0);
/* 	    sampler->fallback = true; */
	    /* TODO */
	 }
      }


      state[1] =
         ((translate_wrap_mode(ws) << SS3_TCX_ADDR_MODE_SHIFT) |
          (translate_wrap_mode(wt) << SS3_TCY_ADDR_MODE_SHIFT) |
          (translate_wrap_mode(wr) << SS3_TCZ_ADDR_MODE_SHIFT) |
	  (unit << SS3_TEXTUREMAP_INDEX_SHIFT));

     if (target != TEXTURE_RECT_BIT)
	 state[1] |= SS3_NORMALIZED_COORDS;

   state[2] = INTEL_PACKCOLOR8888(tObj->_BorderChan[0],
				  tObj->_BorderChan[1],
				  tObj->_BorderChan[2],
				  tObj->_BorderChan[3]);
}



void i915_update_samplers( struct i915_context *i915 )
{
   int i, dirty = 0, nr = 0;

   for (i = 0; i < I915_TEX_UNITS; i++) {
      if (i915->sampler[i].enabled) { /* ??? */
	 update_sampler( i915, i, 
			 i915->current.sampler[i] );
	 nr++;
	 dirty |= (1<<i);
      }
   }
}

