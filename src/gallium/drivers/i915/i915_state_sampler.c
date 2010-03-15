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

#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "i915_state_inlines.h"
#include "i915_context.h"
#include "i915_reg.h"
#include "i915_state.h"


/*
 * A note about min_lod & max_lod.
 *
 * There is a circular dependancy between the sampler state
 * and the map state to be submitted to hw.
 *
 * Two condition must be meet:
 * min_lod =< max_lod == true
 * max_lod =< last_level == true
 *
 *
 * This is all fine and dandy if it where for the fact that max_lod
 * is set on the map state instead of the sampler state. That is
 * the max_lod we submit on map is:
 * max_lod = MIN2(last_level, max_lod);
 *
 * So we need to update the map state when we change samplers and
 * we need to be change the sampler state when map state is changed.
 * The first part is done by calling i915_update_texture in
 * i915_update_samplers and the second part is done else where in
 * code tracking the state changes.
 */

static void
i915_update_texture(struct i915_context *i915,
                    uint unit,
                    const struct i915_texture *tex,
                    const struct i915_sampler_state *sampler,
                    uint state[6]);
/**
 * Compute i915 texture sampling state.
 *
 * Recalculate all state from scratch.  Perhaps not the most
 * efficient, but this has gotten complex enough that we need
 * something which is understandable and reliable.
 * \param state  returns the 3 words of compute state
 */
static void update_sampler(struct i915_context *i915,
                           uint unit,
			   const struct i915_sampler_state *sampler,
			   const struct i915_texture *tex,
			   unsigned state[3] )
{
   const struct pipe_texture *pt = &tex->base;
   unsigned minlod, lastlod;

   /* Need to do this after updating the maps, which call the
    * intel_finalize_mipmap_tree and hence can update firstLevel:
    */
   state[0] = sampler->state[0];
   state[1] = sampler->state[1];
   state[2] = sampler->state[2];

   if (pt->format == PIPE_FORMAT_UYVY ||
       pt->format == PIPE_FORMAT_YUYV)
      state[0] |= SS2_COLORSPACE_CONVERSION;

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
#if 0
   {
      const unsigned ws = sampler->templ->wrap_s;
      const unsigned wt = sampler->templ->wrap_t;
      const unsigned wr = sampler->templ->wrap_r;
      if (pt->target == PIPE_TEXTURE_3D &&
          (sampler->templ->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
           sampler->templ->mag_img_filter != PIPE_TEX_FILTER_NEAREST) &&
          (ws == PIPE_TEX_WRAP_CLAMP ||
           wt == PIPE_TEX_WRAP_CLAMP ||
           wr == PIPE_TEX_WRAP_CLAMP ||
           ws == PIPE_TEX_WRAP_CLAMP_TO_BORDER ||
           wt == PIPE_TEX_WRAP_CLAMP_TO_BORDER || 
           wr == PIPE_TEX_WRAP_CLAMP_TO_BORDER)) {
         if (i915->conformance_mode > 0) {
            assert(0);
            /* 	    sampler->fallback = true; */
            /* TODO */
         }
      }
   }
#endif

   /* See note at the top of file */
   minlod = sampler->minlod;
   lastlod = pt->last_level << 4;

   if (lastlod < minlod) {
      minlod = lastlod;
   }

   state[1] |= (sampler->minlod << SS3_MIN_LOD_SHIFT);
   state[1] |= (unit << SS3_TEXTUREMAP_INDEX_SHIFT);
}


void i915_update_samplers( struct i915_context *i915 )
{
   uint unit;

   i915->current.sampler_enable_nr = 0;
   i915->current.sampler_enable_flags = 0x0;

   for (unit = 0; unit < i915->num_textures && unit < i915->num_samplers;
        unit++) {
      /* determine unit enable/disable by looking for a bound texture */
      /* could also examine the fragment program? */
      if (i915->texture[unit]) {
	 update_sampler( i915,
	                 unit,
	                 i915->sampler[unit],       /* sampler state */
	                 i915->texture[unit],        /* texture */
	                 i915->current.sampler[unit] /* the result */
	                 );
	 i915_update_texture( i915,
	                      unit,
	                      i915->texture[unit],          /* texture */
	                      i915->sampler[unit],          /* sampler state */
	                      i915->current.texbuffer[unit] );

	 i915->current.sampler_enable_nr++;
	 i915->current.sampler_enable_flags |= (1 << unit);
      }
   }

   i915->hardware_dirty |= I915_HW_SAMPLER | I915_HW_MAP;
}


static uint
translate_texture_format(enum pipe_format pipeFormat)
{
   switch (pipeFormat) {
   case PIPE_FORMAT_L8_UNORM:
      return MAPSURF_8BIT | MT_8BIT_L8;
   case PIPE_FORMAT_I8_UNORM:
      return MAPSURF_8BIT | MT_8BIT_I8;
   case PIPE_FORMAT_A8_UNORM:
      return MAPSURF_8BIT | MT_8BIT_A8;
   case PIPE_FORMAT_L8A8_UNORM:
      return MAPSURF_16BIT | MT_16BIT_AY88;
   case PIPE_FORMAT_B5G6R5_UNORM:
      return MAPSURF_16BIT | MT_16BIT_RGB565;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return MAPSURF_16BIT | MT_16BIT_ARGB1555;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return MAPSURF_16BIT | MT_16BIT_ARGB4444;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return MAPSURF_32BIT | MT_32BIT_ARGB8888;
   case PIPE_FORMAT_YUYV:
      return (MAPSURF_422 | MT_422_YCRCB_NORMAL);
   case PIPE_FORMAT_UYVY:
      return (MAPSURF_422 | MT_422_YCRCB_SWAPY);
#if 0
   case PIPE_FORMAT_RGB_FXT1:
   case PIPE_FORMAT_RGBA_FXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_FXT1);
#endif
   case PIPE_FORMAT_Z16_UNORM:
      return (MAPSURF_16BIT | MT_16BIT_L16);
#if 0
   case PIPE_FORMAT_RGBA_DXT1:
   case PIPE_FORMAT_RGB_DXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
   case PIPE_FORMAT_RGBA_DXT3:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
   case PIPE_FORMAT_RGBA_DXT5:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
#endif
   case PIPE_FORMAT_Z24S8_UNORM:
      return (MAPSURF_32BIT | MT_32BIT_xI824);
   default:
      debug_printf("i915: translate_texture_format() bad image format %x\n",
              pipeFormat);
      assert(0);
      return 0;
   }
}


static void
i915_update_texture(struct i915_context *i915,
                    uint unit,
                    const struct i915_texture *tex,
                    const struct i915_sampler_state *sampler,
                    uint state[6])
{
   const struct pipe_texture *pt = &tex->base;
   uint format, pitch;
   const uint width = pt->width0, height = pt->height0, depth = pt->depth0;
   const uint num_levels = pt->last_level;
   unsigned max_lod = num_levels * 4;
   unsigned tiled = MS3_USE_FENCE_REGS;

   assert(tex);
   assert(width);
   assert(height);
   assert(depth);

   format = translate_texture_format(pt->format);
   pitch = tex->stride;

   assert(format);
   assert(pitch);

   if (tex->sw_tiled) {
      assert(!((pitch - 1) & pitch));
      tiled = MS3_TILED_SURFACE;
   }

   /* MS3 state */
   state[0] =
      (((height - 1) << MS3_HEIGHT_SHIFT)
       | ((width - 1) << MS3_WIDTH_SHIFT)
       | format
       | tiled);

   /*
    * XXX When min_filter != mag_filter and there's just one mipmap level,
    * set max_lod = 1 to make sure i915 chooses between min/mag filtering.
    */

   /* See note at the top of file */
   if (max_lod > (sampler->maxlod >> 2))
      max_lod = sampler->maxlod >> 2;

   /* MS4 state */
   state[1] =
      ((((pitch / 4) - 1) << MS4_PITCH_SHIFT)
       | MS4_CUBE_FACE_ENA_MASK
       | ((max_lod) << MS4_MAX_LOD_SHIFT)
       | ((depth - 1) << MS4_VOLUME_DEPTH_SHIFT));
}


void
i915_update_textures(struct i915_context *i915)
{
   uint unit;

   for (unit = 0; unit < i915->num_textures && unit < i915->num_samplers;
        unit++) {
      /* determine unit enable/disable by looking for a bound texture */
      /* could also examine the fragment program? */
      if (i915->texture[unit]) {
	 i915_update_texture( i915,
	                      unit,
	                      i915->texture[unit],          /* texture */
	                      i915->sampler[unit],          /* sampler state */
	                      i915->current.texbuffer[unit] );
      }
   }

   i915->hardware_dirty |= I915_HW_MAP;
}
