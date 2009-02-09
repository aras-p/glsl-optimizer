/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#include "util/u_math.h"
#include "util/u_memory.h"


#define COMPAREFUNC_ALWAYS		0
#define COMPAREFUNC_NEVER		0x1
#define COMPAREFUNC_LESS		0x2
#define COMPAREFUNC_EQUAL		0x3
#define COMPAREFUNC_LEQUAL		0x4
#define COMPAREFUNC_GREATER		0x5
#define COMPAREFUNC_NOTEQUAL		0x6
#define COMPAREFUNC_GEQUAL		0x7

/* Samplers aren't strictly wm state from the hardware's perspective,
 * but that is the only situation in which we use them in this driver.
 */

static int intel_translate_shadow_compare_func(unsigned func)
{
   switch(func) {
   case PIPE_FUNC_NEVER:
       return COMPAREFUNC_ALWAYS;
   case PIPE_FUNC_LESS:
       return COMPAREFUNC_LEQUAL;
   case PIPE_FUNC_LEQUAL:
       return COMPAREFUNC_LESS;
   case PIPE_FUNC_GREATER:
       return COMPAREFUNC_GEQUAL;
   case PIPE_FUNC_GEQUAL:
      return COMPAREFUNC_GREATER;
   case PIPE_FUNC_NOTEQUAL:
      return COMPAREFUNC_EQUAL;
   case PIPE_FUNC_EQUAL:
      return COMPAREFUNC_NOTEQUAL;
   case PIPE_FUNC_ALWAYS:
       return COMPAREFUNC_NEVER;
   }

   debug_printf("Unknown value in %s: %x\n", __FUNCTION__, func);
   return COMPAREFUNC_NEVER;
}

/* The brw (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static unsigned translate_wrap_mode( int wrap )
{
   switch( wrap ) {
   case PIPE_TEX_WRAP_REPEAT:
      return BRW_TEXCOORDMODE_WRAP;
   case PIPE_TEX_WRAP_CLAMP:
      return BRW_TEXCOORDMODE_CLAMP;
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return BRW_TEXCOORDMODE_CLAMP; /* conform likes it this way */
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return BRW_TEXCOORDMODE_CLAMP_BORDER;
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      return BRW_TEXCOORDMODE_MIRROR;
   default:
      return BRW_TEXCOORDMODE_WRAP;
   }
}


static unsigned U_FIXED(float value, unsigned frac_bits)
{
   value *= (1<<frac_bits);
   return value < 0 ? 0 : value;
}

static int S_FIXED(float value, unsigned frac_bits)
{
   return value * (1<<frac_bits);
}


static unsigned upload_default_color( struct brw_context *brw,
                                      const float *color )
{
   struct brw_sampler_default_color sdc;

   COPY_4V(sdc.color, color);

   return brw_cache_data( &brw->cache[BRW_SAMPLER_DEFAULT_COLOR], &sdc );
}


/*
 */
static void brw_update_sampler_state( const struct pipe_sampler_state *pipe_sampler,
				      unsigned sdc_gs_offset,
				      struct brw_sampler_state *sampler)
{
   memset(sampler, 0, sizeof(*sampler));

   switch (pipe_sampler->min_mip_filter) {
   case PIPE_TEX_FILTER_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      break;
   case PIPE_TEX_FILTER_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      break;
   case PIPE_TEX_FILTER_ANISO:
      sampler->ss0.min_filter = BRW_MAPFILTER_ANISOTROPIC;
      break;
   default:
      break;
   }

   switch (pipe_sampler->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NEAREST:
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case PIPE_TEX_MIPFILTER_LINEAR:
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   case PIPE_TEX_MIPFILTER_NONE:
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   default:
      break;
   }
   /* Set Anisotropy:
    */
   switch (pipe_sampler->mag_img_filter) {
   case PIPE_TEX_FILTER_NEAREST:
      sampler->ss0.mag_filter = BRW_MAPFILTER_NEAREST;
      break;
   case PIPE_TEX_FILTER_LINEAR:
      sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;
      break;
   case PIPE_TEX_FILTER_ANISO:
      sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;
      break;
   default:
      break;
   }

   if (pipe_sampler->max_anisotropy > 2.0) {
      sampler->ss3.max_aniso = MAX2((pipe_sampler->max_anisotropy - 2) / 2,
                                    BRW_ANISORATIO_16);
   }

   sampler->ss1.s_wrap_mode = translate_wrap_mode(pipe_sampler->wrap_s);
   sampler->ss1.r_wrap_mode = translate_wrap_mode(pipe_sampler->wrap_r);
   sampler->ss1.t_wrap_mode = translate_wrap_mode(pipe_sampler->wrap_t);

   /* Fulsim complains if I don't do this.  Hardware doesn't mind:
    */
#if 0
   if (texObj->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      sampler->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      sampler->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CUBE;
   }
#endif

   /* Set shadow function:
    */
   if (pipe_sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
      /* Shadowing is "enabled" by emitting a particular sampler
       * message (sample_c).  So need to recompile WM program when
       * shadow comparison is enabled on each/any texture unit.
       */
      sampler->ss0.shadow_function = intel_translate_shadow_compare_func(pipe_sampler->compare_func);
   }

   /* Set LOD bias:
    */
   sampler->ss0.lod_bias = S_FIXED(CLAMP(pipe_sampler->lod_bias, -16, 15), 6);

   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set BaseMipLevel, MaxLOD, MinLOD:
    *
    * XXX: I don't think that using firstLevel, lastLevel works,
    * because we always setup the surface state as if firstLevel ==
    * level zero.  Probably have to subtract firstLevel from each of
    * these:
    */
   sampler->ss0.base_level = U_FIXED(0, 1);

   sampler->ss1.max_lod = U_FIXED(MIN2(MAX2(pipe_sampler->max_lod, 0), 13), 6);
   sampler->ss1.min_lod = U_FIXED(MIN2(MAX2(pipe_sampler->min_lod, 0), 13), 6);

   sampler->ss2.default_color_pointer = sdc_gs_offset >> 5;
}



/* All samplers must be uploaded in a single contiguous array, which
 * complicates various things.  However, this is still too confusing -
 * FIXME: simplify all the different new texture state flags.
 */
static void upload_wm_samplers(struct brw_context *brw)
{
   unsigned unit;
   unsigned sampler_count = 0;

   /* BRW_NEW_SAMPLER */
   for (unit = 0; unit < brw->num_textures && unit < brw->num_samplers;
        unit++) {
      /* determine unit enable/disable by looking for a bound texture */
      if (brw->attribs.Texture[unit]) {
         const struct pipe_sampler_state *sampler = brw->attribs.Samplers[unit];
	 unsigned sdc_gs_offset = upload_default_color(brw, sampler->border_color);

	 brw_update_sampler_state(sampler,
				  sdc_gs_offset,
				  &brw->wm.sampler[unit]);

	 sampler_count = unit + 1;
      }
   }

   if (brw->wm.sampler_count != sampler_count) {
      brw->wm.sampler_count = sampler_count;
      brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
   }

   brw->wm.sampler_gs_offset = 0;

   if (brw->wm.sampler_count)
      brw->wm.sampler_gs_offset =
	 brw_cache_data_sz(&brw->cache[BRW_SAMPLER],
			   brw->wm.sampler,
			   sizeof(struct brw_sampler_state) * brw->wm.sampler_count);
}

const struct brw_tracked_state brw_wm_samplers = {
   .dirty = {
      .brw = BRW_NEW_SAMPLER,
      .cache = 0
   },
   .update = upload_wm_samplers
};

