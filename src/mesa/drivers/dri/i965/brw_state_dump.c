/*
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "main/mtypes.h"
#include "intel_batchbuffer.h"

#include "brw_context.h"
#include "brw_defines.h"

/**
 * Prints out a header, the contents, and the message associated with
 * the hardware state data given.
 *
 * \param name Name of the state object
 * \param data Pointer to the base of the state object
 * \param hw_offset Hardware offset of the base of the state data.
 * \param index Index of the DWORD being output.
 */
static void
state_out(const char *name, void *data, uint32_t hw_offset, int index,
	  char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%8s: 0x%08x: 0x%08x: ",
	    name, hw_offset + index * 4, ((uint32_t *)data)[index]);
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

/** Generic, undecoded state buffer debug printout */
static void
state_struct_out(const char *name, drm_intel_bo *buffer,
		 unsigned int offset, unsigned int size)
{
   int i;

   if (buffer == NULL)
      return;

   drm_intel_bo_map(buffer, GL_FALSE);
   for (i = 0; i < size / 4; i++) {
      state_out(name, buffer->virtual + offset, buffer->offset + offset, i,
		"dword %d\n", i);
   }
   drm_intel_bo_unmap(buffer);
}

static const char *
get_965_surfacetype(unsigned int surfacetype)
{
    switch (surfacetype) {
    case 0: return "1D";
    case 1: return "2D";
    case 2: return "3D";
    case 3: return "CUBE";
    case 4: return "BUFFER";
    case 7: return "NULL";
    default: return "unknown";
    }
}

static const char *
get_965_surface_format(unsigned int surface_format)
{
    switch (surface_format) {
    case 0x000: return "r32g32b32a32_float";
    case 0x0c1: return "b8g8r8a8_unorm";
    case 0x100: return "b5g6r5_unorm";
    case 0x102: return "b5g5r5a1_unorm";
    case 0x104: return "b4g4r4a4_unorm";
    default: return "unknown";
    }
}

static void dump_wm_surface_state(struct brw_context *brw)
{
   dri_bo *bo;
   GLubyte *base;
   int i;

   bo = brw->intel.batch.bo;
   drm_intel_bo_map(bo, GL_FALSE);
   base = bo->virtual;

   for (i = 0; i < brw->wm.nr_surfaces; i++) {
      unsigned int surfoff;
      uint32_t *surf;
      char name[20];

      if (brw->wm.surf_offset[i] == 0) {
	 fprintf(stderr, "WM SURF%d: NULL\n", i);
	 continue;
      }
      surfoff = bo->offset + brw->wm.surf_offset[i];
      surf = (uint32_t *)(base + brw->wm.surf_offset[i]);

      sprintf(name, "WM SURF%d", i);
      state_out(name, surf, surfoff, 0, "%s %s\n",
		get_965_surfacetype(GET_FIELD(surf[0], BRW_SURFACE_TYPE)),
		get_965_surface_format(GET_FIELD(surf[0], BRW_SURFACE_FORMAT)));
      state_out(name, surf, surfoff, 1, "offset\n");
      state_out(name, surf, surfoff, 2, "%dx%d size, %d mips\n",
		GET_FIELD(surf[2], BRW_SURFACE_WIDTH) + 1,
		GET_FIELD(surf[2], BRW_SURFACE_HEIGHT) + 1);
      state_out(name, surf, surfoff, 3, "pitch %d, %s tiled\n",
		GET_FIELD(surf[3], BRW_SURFACE_PITCH) + 1,
		(surf[3] & BRW_SURFACE_TILED) ?
		((surf[3] & BRW_SURFACE_TILED_Y) ? "Y" : "X") : "not");
      state_out(name, surf, surfoff, 4, "mip base %d\n",
		GET_FIELD(surf[4], BRW_SURFACE_MIN_LOD));
      state_out(name, surf, surfoff, 5, "x,y offset: %d,%d\n",
		GET_FIELD(surf[5], BRW_SURFACE_X_OFFSET),
		GET_FIELD(surf[5], BRW_SURFACE_Y_OFFSET));
   }
   drm_intel_bo_unmap(bo);
}

static void dump_gen7_surface_state(struct brw_context *brw)
{
   dri_bo *bo;
   GLubyte *base;
   int i;

   bo = brw->intel.batch.bo;
   drm_intel_bo_map(bo, GL_FALSE);
   base = bo->virtual;

   for (i = 0; i < brw->wm.nr_surfaces; i++) {
      unsigned int surfoff;
      struct gen7_surface_state *surf;
      char name[20];

      if (brw->wm.surf_offset[i] == 0) {
	 fprintf(stderr, "WM SURF%d: NULL\n", i);
	 continue;
      }
      surfoff = bo->offset + brw->wm.surf_offset[i];
      surf = (struct gen7_surface_state *) (base + brw->wm.surf_offset[i]);

      sprintf(name, "WM SURF%d", i);
      state_out(name, surf, surfoff, 0, "%s %s\n",
		get_965_surfacetype(surf->ss0.surface_type),
		get_965_surface_format(surf->ss0.surface_format));
      state_out(name, surf, surfoff, 1, "offset\n");
      state_out(name, surf, surfoff, 2, "%dx%d size, %d mips\n",
		surf->ss2.width + 1, surf->ss2.height + 1, surf->ss5.mip_count);
      state_out(name, surf, surfoff, 3, "pitch %d, %stiled\n",
		surf->ss3.pitch + 1, surf->ss0.tiled_surface ? "" : "not ");
      state_out(name, surf, surfoff, 4, "mip base %d\n",
		surf->ss5.min_lod);
      state_out(name, surf, surfoff, 5, "x,y offset: %d,%d\n",
		surf->ss5.x_offset, surf->ss5.y_offset);
   }
   drm_intel_bo_unmap(bo);
}

static void dump_wm_sampler_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   int i;

   assert(intel->gen < 7);

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      unsigned int offset;
      uint32_t sdc_offset;
      struct brw_sampler_state *samp;
      char name[20];

      if (!ctx->Texture.Unit[i]._ReallyEnabled) {
	 fprintf(stderr, "WM SAMP%d: disabled\n", i);
	 continue;
      }

      offset = (intel->batch.bo->offset +
		brw->wm.sampler_offset +
		i * sizeof(struct brw_sampler_state));
      samp = (struct brw_sampler_state *)(intel->batch.bo->virtual +
					  brw->wm.sampler_offset +
					  i * sizeof(struct brw_sampler_state));

      sprintf(name, "WM SAMP%d", i);
      state_out(name, samp, offset, 0, "filtering\n");
      state_out(name, samp, offset, 1, "wrapping, lod\n");
      state_out(name, samp, offset, 2, "default color pointer\n");
      state_out(name, samp, offset, 3, "chroma key, aniso\n");

      sprintf(name, " WM SDC%d", i);

      sdc_offset = intel->batch.bo->offset + brw->wm.sdc_offset[i];
      if (intel->gen >= 5) {
	 struct gen5_sampler_default_color *sdc = (intel->batch.bo->virtual +
						   brw->wm.sdc_offset[i]);
	 state_out(name, sdc, sdc_offset, 0, "unorm rgba\n");
	 state_out(name, sdc, sdc_offset, 1, "r %f\n", sdc->f[0]);
	 state_out(name, sdc, sdc_offset, 2, "b %f\n", sdc->f[1]);
	 state_out(name, sdc, sdc_offset, 3, "g %f\n", sdc->f[2]);
	 state_out(name, sdc, sdc_offset, 4, "a %f\n", sdc->f[3]);
	 state_out(name, sdc, sdc_offset, 5, "half float rg\n");
	 state_out(name, sdc, sdc_offset, 6, "half float ba\n");
	 state_out(name, sdc, sdc_offset, 7, "u16 rg\n");
	 state_out(name, sdc, sdc_offset, 8, "u16 ba\n");
	 state_out(name, sdc, sdc_offset, 9, "s16 rg\n");
	 state_out(name, sdc, sdc_offset, 10, "s16 ba\n");
	 state_out(name, sdc, sdc_offset, 11, "s8 rgba\n");
      } else {
	 struct brw_sampler_default_color *sdc = (intel->batch.bo->virtual +
						  brw->wm.sdc_offset[i]);
	 state_out(name, sdc, sdc_offset, 0, "r %f\n", sdc->color[0]);
	 state_out(name, sdc, sdc_offset, 1, "g %f\n", sdc->color[1]);
	 state_out(name, sdc, sdc_offset, 2, "b %f\n", sdc->color[2]);
	 state_out(name, sdc, sdc_offset, 3, "a %f\n", sdc->color[3]);
      }
   }
   drm_intel_bo_unmap(intel->batch.bo);
}

static void dump_gen7_sampler_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   int i;

   assert(intel->gen >= 7);

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      unsigned int offset;
      uint32_t sdc_offset;
      struct gen7_sampler_state *samp;
      char name[20];

      if (!ctx->Texture.Unit[i]._ReallyEnabled) {
	 fprintf(stderr, "WM SAMP%d: disabled\n", i);
	 continue;
      }

      offset = (intel->batch.bo->offset +
		brw->wm.sampler_offset +
		i * sizeof(struct gen7_sampler_state));
      samp = (struct gen7_sampler_state *)
	     (intel->batch.bo->virtual + brw->wm.sampler_offset +
	      i * sizeof(struct gen7_sampler_state));

      sprintf(name, "WM SAMP%d", i);
      state_out(name, samp, offset, 0, "filtering\n");
      state_out(name, samp, offset, 1, "wrapping, lod\n");
      state_out(name, samp, offset, 2, "default color pointer\n");
      state_out(name, samp, offset, 3, "chroma key, aniso\n");

      sprintf(name, " WM SDC%d", i);

      sdc_offset = intel->batch.bo->offset + brw->wm.sdc_offset[i];
      struct brw_sampler_default_color *sdc =
	 intel->batch.bo->virtual + brw->wm.sdc_offset[i];
      state_out(name, sdc, sdc_offset, 0, "r %f\n", sdc->color[0]);
      state_out(name, sdc, sdc_offset, 1, "g %f\n", sdc->color[1]);
      state_out(name, sdc, sdc_offset, 2, "b %f\n", sdc->color[2]);
      state_out(name, sdc, sdc_offset, 3, "a %f\n", sdc->color[3]);
   }
   drm_intel_bo_unmap(intel->batch.bo);
}


static void dump_sf_viewport_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "SF VP";
   struct brw_sf_viewport *vp;
   uint32_t vp_off;

   assert(intel->gen < 7);

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);

   vp = intel->batch.bo->virtual + brw->sf.vp_offset;
   vp_off = intel->batch.bo->offset + brw->sf.vp_offset;

   state_out(name, vp, vp_off, 0, "m00 = %f\n", vp->viewport.m00);
   state_out(name, vp, vp_off, 1, "m11 = %f\n", vp->viewport.m11);
   state_out(name, vp, vp_off, 2, "m22 = %f\n", vp->viewport.m22);
   state_out(name, vp, vp_off, 3, "m30 = %f\n", vp->viewport.m30);
   state_out(name, vp, vp_off, 4, "m31 = %f\n", vp->viewport.m31);
   state_out(name, vp, vp_off, 5, "m32 = %f\n", vp->viewport.m32);

   state_out(name, vp, vp_off, 6, "top left = %d,%d\n",
	     vp->scissor.xmin, vp->scissor.ymin);
   state_out(name, vp, vp_off, 7, "bottom right = %d,%d\n",
	     vp->scissor.xmax, vp->scissor.ymax);

   drm_intel_bo_unmap(intel->batch.bo);
}

static void dump_clip_viewport_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "CLIP VP";
   struct brw_clipper_viewport *vp;
   uint32_t vp_off;

   assert(intel->gen < 7);

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);

   vp = intel->batch.bo->virtual + brw->clip.vp_offset;
   vp_off = intel->batch.bo->offset + brw->clip.vp_offset;

   state_out(name, vp, vp_off, 0, "xmin = %f\n", vp->xmin);
   state_out(name, vp, vp_off, 1, "xmax = %f\n", vp->xmax);
   state_out(name, vp, vp_off, 2, "ymin = %f\n", vp->ymin);
   state_out(name, vp, vp_off, 3, "ymax = %f\n", vp->ymax);
   drm_intel_bo_unmap(intel->batch.bo);
}

static void dump_sf_clip_viewport_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "SF_CLIP VP";
   struct gen7_sf_clip_viewport *vp;
   uint32_t vp_off;

   assert(intel->gen >= 7);

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);

   vp = intel->batch.bo->virtual + brw->sf.vp_offset;
   vp_off = intel->batch.bo->offset + brw->sf.vp_offset;

   state_out(name, vp, vp_off, 0, "m00 = %f\n", vp->viewport.m00);
   state_out(name, vp, vp_off, 1, "m11 = %f\n", vp->viewport.m11);
   state_out(name, vp, vp_off, 2, "m22 = %f\n", vp->viewport.m22);
   state_out(name, vp, vp_off, 3, "m30 = %f\n", vp->viewport.m30);
   state_out(name, vp, vp_off, 4, "m31 = %f\n", vp->viewport.m31);
   state_out(name, vp, vp_off, 5, "m32 = %f\n", vp->viewport.m32);
   state_out(name, vp, vp_off, 6, "guardband xmin = %f\n", vp->guardband.xmin);
   state_out(name, vp, vp_off, 7, "guardband xmax = %f\n", vp->guardband.xmax);
   state_out(name, vp, vp_off, 8, "guardband ymin = %f\n", vp->guardband.ymin);
   state_out(name, vp, vp_off, 9, "guardband ymax = %f\n", vp->guardband.ymax);
   drm_intel_bo_unmap(intel->batch.bo);
}


static void dump_cc_viewport_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "CC VP";
   struct brw_cc_viewport *vp;
   uint32_t vp_off;

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);

   vp = intel->batch.bo->virtual + brw->cc.vp_offset;
   vp_off = intel->batch.bo->offset + brw->cc.vp_offset;

   state_out(name, vp, vp_off, 0, "min_depth = %f\n", vp->min_depth);
   state_out(name, vp, vp_off, 1, "max_depth = %f\n", vp->max_depth);
   drm_intel_bo_unmap(intel->batch.bo);
}

static void dump_depth_stencil_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "DEPTH STENCIL";
   struct gen6_depth_stencil_state *ds;
   uint32_t ds_off;

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);

   ds = intel->batch.bo->virtual + brw->cc.depth_stencil_state_offset;
   ds_off = intel->batch.bo->offset + brw->cc.depth_stencil_state_offset;

   state_out(name, ds, ds_off, 0, "stencil %sable, func %d, write %sable\n",
		ds->ds0.stencil_enable ? "en" : "dis",
		ds->ds0.stencil_func,
		ds->ds0.stencil_write_enable ? "en" : "dis");
   state_out(name, ds, ds_off, 1, "stencil test mask 0x%x, write mask 0x%x\n",
		ds->ds1.stencil_test_mask, ds->ds1.stencil_write_mask);
   state_out(name, ds, ds_off, 2, "depth test %sable, func %d, write %sable\n",
		ds->ds2.depth_test_enable ? "en" : "dis",
		ds->ds2.depth_test_func,
		ds->ds2.depth_write_enable ? "en" : "dis");
   drm_intel_bo_unmap(intel->batch.bo);
}

static void dump_cc_state(struct brw_context *brw)
{
   const char *name = "CC";
   struct gen6_color_calc_state *cc;
   uint32_t cc_off;
   dri_bo *bo = brw->intel.batch.bo;

   if (brw->cc.state_offset == 0)
	return;

   drm_intel_bo_map(bo, GL_FALSE);
   cc = bo->virtual + brw->cc.state_offset;
   cc_off = bo->offset + brw->cc.state_offset;

   state_out(name, cc, cc_off, 0, "alpha test format %s, round disable %d, stencil ref %d,"
		"bf stencil ref %d\n",
		cc->cc0.alpha_test_format ? "FLOAT32" : "UNORM8",
		cc->cc0.round_disable,
		cc->cc0.stencil_ref,
		cc->cc0.bf_stencil_ref);
   state_out(name, cc, cc_off, 1, "\n");
   state_out(name, cc, cc_off, 2, "constant red %f\n", cc->constant_r);
   state_out(name, cc, cc_off, 3, "constant green %f\n", cc->constant_g);
   state_out(name, cc, cc_off, 4, "constant blue %f\n", cc->constant_b);
   state_out(name, cc, cc_off, 5, "constant alpha %f\n", cc->constant_a);
   
   drm_intel_bo_unmap(bo);

}

static void dump_blend_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "BLEND";
   struct gen6_blend_state *blend;
   uint32_t blend_off;

   drm_intel_bo_map(intel->batch.bo, GL_FALSE);

   blend = intel->batch.bo->virtual + brw->cc.blend_state_offset;
   blend_off = intel->batch.bo->offset + brw->cc.blend_state_offset;

   state_out(name, blend, blend_off, 0, "\n");
   state_out(name, blend, blend_off, 1, "\n");

   drm_intel_bo_unmap(intel->batch.bo);

}

static void brw_debug_prog(struct brw_context *brw,
			   const char *name, uint32_t prog_offset)
{
   unsigned int i;
   uint32_t *data;

   drm_intel_bo_map(brw->cache.bo, false);

   data = brw->cache.bo->virtual + prog_offset;

   for (i = 0; i < brw->cache.bo->size / 4 / 4; i++) {
      fprintf(stderr, "%8s: 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	      name, (unsigned int)brw->cache.bo->offset + i * 4 * 4,
	      data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
      /* Stop at the end of the program.  It'd be nice to keep track of the actual
       * intended program size instead of guessing like this.
       */
      if (data[i * 4 + 0] == 0 &&
	  data[i * 4 + 1] == 0 &&
	  data[i * 4 + 2] == 0 &&
	  data[i * 4 + 3] == 0)
	 break;
   }

   drm_intel_bo_unmap(brw->cache.bo);
}


/**
 * Print additional debug information associated with the batchbuffer
 * when DEBUG_BATCH is set.
 *
 * For 965, this means mapping the state buffers that would have been referenced
 * by the batchbuffer and dumping them.
 *
 * The buffer offsets printed rely on the buffer containing the last offset
 * it was validated at.
 */
void brw_debug_batch(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);

   state_struct_out("WM bind",
		    brw->intel.batch.bo,
		    brw->wm.bind_bo_offset,
		    4 * brw->wm.nr_surfaces);
   if (intel->gen < 7) {
      dump_wm_surface_state(brw);
      dump_wm_sampler_state(brw);
   } else {
      dump_gen7_surface_state(brw);
      dump_gen7_sampler_state(brw);
   }

   if (intel->gen < 6)
       state_struct_out("VS", intel->batch.bo, brw->vs.state_offset,
			sizeof(struct brw_vs_unit_state));
   brw_debug_prog(brw, "VS prog", brw->vs.prog_offset);

   if (intel->gen < 6)
       state_struct_out("GS", intel->batch.bo, brw->gs.state_offset,
			sizeof(struct brw_gs_unit_state));
   if (brw->gs.prog_active) {
      brw_debug_prog(brw, "GS prog", brw->gs.prog_offset);
   }

   if (intel->gen < 6) {
      state_struct_out("SF", intel->batch.bo, brw->sf.state_offset,
		       sizeof(struct brw_sf_unit_state));
      brw_debug_prog(brw, "SF prog", brw->sf.prog_offset);
   }
   if (intel->gen >= 7)
      dump_sf_clip_viewport_state(brw);
   else
      dump_sf_viewport_state(brw);
   if (intel->gen == 6)
      dump_clip_viewport_state(brw);

   if (intel->gen < 6)
       state_struct_out("WM", intel->batch.bo, brw->wm.state_offset,
			sizeof(struct brw_wm_unit_state));
   brw_debug_prog(brw, "WM prog", brw->wm.prog_offset);

   if (intel->gen >= 6) {
	dump_cc_viewport_state(brw);
	dump_depth_stencil_state(brw);
	dump_cc_state(brw);
	dump_blend_state(brw);
   }
}
