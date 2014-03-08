/*
 * Copyright © 2007 Intel Corporation
 * Copyright (C) 2012-2013 LunarG, Inc.
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
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "intel_winsys.h"

#include "ilo_cp.h"
#include "ilo_3d_pipeline.h"

#define PRINTFLIKE(f, a) _util_printf_format(f, a)
typedef short GLshort;
typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef float GLfloat;
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "brw_structs.h"
#include "brw_defines.h"

struct intel_context {
   int gen;

   struct {
      struct {
         void *virtual;
      } *bo, bo_dst;
   } batch;
};

struct brw_context {
   struct intel_context intel;
};

static void
batch_out(struct brw_context *brw, const char *name, uint32_t offset,
	  int index, char *fmt, ...) PRINTFLIKE(5, 6);

static void
batch_out(struct brw_context *brw, const char *name, uint32_t offset,
	  int index, char *fmt, ...)
{
   struct intel_context *intel = &brw->intel;
   uint32_t *data = intel->batch.bo->virtual + offset;
   va_list va;

   fprintf(stderr, "0x%08x:      0x%08x: %8s: ",
	   offset + index * 4, data[index], name);
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
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

static void dump_vs_state(struct brw_context *brw, uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "VS_STATE";
   struct brw_vs_unit_state *vs = intel->batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "thread0\n");
   batch_out(brw, name, offset, 1, "thread1\n");
   batch_out(brw, name, offset, 2, "thread2\n");
   batch_out(brw, name, offset, 3, "thread3\n");
   batch_out(brw, name, offset, 4, "thread4: %d threads\n",
	     vs->thread4.max_threads + 1);
   batch_out(brw, name, offset, 5, "vs5\n");
   batch_out(brw, name, offset, 6, "vs6\n");
}

static void dump_gs_state(struct brw_context *brw, uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "GS_STATE";
   struct brw_gs_unit_state *gs = intel->batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "thread0\n");
   batch_out(brw, name, offset, 1, "thread1\n");
   batch_out(brw, name, offset, 2, "thread2\n");
   batch_out(brw, name, offset, 3, "thread3\n");
   batch_out(brw, name, offset, 4, "thread4: %d threads\n",
	     gs->thread4.max_threads + 1);
   batch_out(brw, name, offset, 5, "vs5\n");
   batch_out(brw, name, offset, 6, "vs6\n");
}

static void dump_clip_state(struct brw_context *brw, uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "CLIP_STATE";
   struct brw_clip_unit_state *clip = intel->batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "thread0\n");
   batch_out(brw, name, offset, 1, "thread1\n");
   batch_out(brw, name, offset, 2, "thread2\n");
   batch_out(brw, name, offset, 3, "thread3\n");
   batch_out(brw, name, offset, 4, "thread4: %d threads\n",
	     clip->thread4.max_threads + 1);
   batch_out(brw, name, offset, 5, "clip5\n");
   batch_out(brw, name, offset, 6, "clip6\n");
   batch_out(brw, name, offset, 7, "vp xmin %f\n", clip->viewport_xmin);
   batch_out(brw, name, offset, 8, "vp xmax %f\n", clip->viewport_xmax);
   batch_out(brw, name, offset, 9, "vp ymin %f\n", clip->viewport_ymin);
   batch_out(brw, name, offset, 10, "vp ymax %f\n", clip->viewport_ymax);
}

static void dump_sf_state(struct brw_context *brw, uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "SF_STATE";
   struct brw_sf_unit_state *sf = intel->batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "thread0\n");
   batch_out(brw, name, offset, 1, "thread1\n");
   batch_out(brw, name, offset, 2, "thread2\n");
   batch_out(brw, name, offset, 3, "thread3\n");
   batch_out(brw, name, offset, 4, "thread4: %d threads\n",
	     sf->thread4.max_threads + 1);
   batch_out(brw, name, offset, 5, "sf5: viewport offset\n");
   batch_out(brw, name, offset, 6, "sf6\n");
   batch_out(brw, name, offset, 7, "sf7\n");
}

static void dump_wm_state(struct brw_context *brw, uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "WM_STATE";
   struct brw_wm_unit_state *wm = intel->batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "thread0\n");
   batch_out(brw, name, offset, 1, "thread1\n");
   batch_out(brw, name, offset, 2, "thread2\n");
   batch_out(brw, name, offset, 3, "thread3\n");
   batch_out(brw, name, offset, 4, "wm4\n");
   batch_out(brw, name, offset, 5, "wm5: %s%s%s%s%s%s, %d threads\n",
	     wm->wm5.enable_8_pix ? "8pix" : "",
	     wm->wm5.enable_16_pix ? "16pix" : "",
	     wm->wm5.program_uses_depth ? ", uses depth" : "",
	     wm->wm5.program_computes_depth ? ", computes depth" : "",
	     wm->wm5.program_uses_killpixel ? ", kills" : "",
	     wm->wm5.thread_dispatch_enable ? "" : ", no dispatch",
	     wm->wm5.max_threads + 1);
   batch_out(brw, name, offset, 6, "depth offset constant %f\n",
	     wm->global_depth_offset_constant);
   batch_out(brw, name, offset, 7, "depth offset scale %f\n",
	     wm->global_depth_offset_scale);
   batch_out(brw, name, offset, 8, "wm8: kernel 1 (gen5+)\n");
   batch_out(brw, name, offset, 9, "wm9: kernel 2 (gen5+)\n");
   batch_out(brw, name, offset, 10, "wm10: kernel 3 (gen5+)\n");
}

static void dump_surface_state(struct brw_context *brw, uint32_t offset)
{
   const char *name = "SURF";
   uint32_t *surf = brw->intel.batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "%s %s\n",
	     get_965_surfacetype(GET_FIELD(surf[0], BRW_SURFACE_TYPE)),
	     get_965_surface_format(GET_FIELD(surf[0], BRW_SURFACE_FORMAT)));
   batch_out(brw, name, offset, 1, "offset\n");
   batch_out(brw, name, offset, 2, "%dx%d size, %d mips\n",
	     GET_FIELD(surf[2], BRW_SURFACE_WIDTH) + 1,
	     GET_FIELD(surf[2], BRW_SURFACE_HEIGHT) + 1,
	     GET_FIELD(surf[2], BRW_SURFACE_LOD));
   batch_out(brw, name, offset, 3, "pitch %d, %s tiled\n",
	     GET_FIELD(surf[3], BRW_SURFACE_PITCH) + 1,
	     (surf[3] & BRW_SURFACE_TILED) ?
	     ((surf[3] & BRW_SURFACE_TILED_Y) ? "Y" : "X") : "not");
   batch_out(brw, name, offset, 4, "mip base %d\n",
	     GET_FIELD(surf[4], BRW_SURFACE_MIN_LOD));
   batch_out(brw, name, offset, 5, "x,y offset: %d,%d\n",
	     GET_FIELD(surf[5], BRW_SURFACE_X_OFFSET),
	     GET_FIELD(surf[5], BRW_SURFACE_Y_OFFSET));
}

static void dump_gen7_surface_state(struct brw_context *brw, uint32_t offset)
{
   const char *name = "SURF";
   uint32_t *surf = brw->intel.batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "%s %s\n",
             get_965_surfacetype(GET_FIELD(surf[0], BRW_SURFACE_TYPE)),
             get_965_surface_format(GET_FIELD(surf[0], BRW_SURFACE_FORMAT)));
   batch_out(brw, name, offset, 1, "offset\n");
   batch_out(brw, name, offset, 2, "%dx%d size, %d mips\n",
             GET_FIELD(surf[2], GEN7_SURFACE_WIDTH) + 1,
             GET_FIELD(surf[2], GEN7_SURFACE_HEIGHT) + 1,
             surf[5] & INTEL_MASK(3, 0));
   batch_out(brw, name, offset, 3, "pitch %d, %stiled\n",
	     (surf[3] & INTEL_MASK(17, 0)) + 1,
             (surf[0] & (1 << 14)) ? "" : "not ");
   batch_out(brw, name, offset, 4, "mip base %d\n",
             GET_FIELD(surf[5], GEN7_SURFACE_MIN_LOD));
   batch_out(brw, name, offset, 5, "x,y offset: %d,%d\n",
             GET_FIELD(surf[5], BRW_SURFACE_X_OFFSET),
             GET_FIELD(surf[5], BRW_SURFACE_Y_OFFSET));
}

static void
dump_sdc(struct brw_context *brw, uint32_t offset)
{
   const char *name = "SDC";
   struct intel_context *intel = &brw->intel;

   if (intel->gen >= 5 && intel->gen <= 6) {
      struct gen5_sampler_default_color *sdc = (intel->batch.bo->virtual +
						offset);
      batch_out(brw, name, offset, 0, "unorm rgba\n");
      batch_out(brw, name, offset, 1, "r %f\n", sdc->f[0]);
      batch_out(brw, name, offset, 2, "b %f\n", sdc->f[1]);
      batch_out(brw, name, offset, 3, "g %f\n", sdc->f[2]);
      batch_out(brw, name, offset, 4, "a %f\n", sdc->f[3]);
      batch_out(brw, name, offset, 5, "half float rg\n");
      batch_out(brw, name, offset, 6, "half float ba\n");
      batch_out(brw, name, offset, 7, "u16 rg\n");
      batch_out(brw, name, offset, 8, "u16 ba\n");
      batch_out(brw, name, offset, 9, "s16 rg\n");
      batch_out(brw, name, offset, 10, "s16 ba\n");
      batch_out(brw, name, offset, 11, "s8 rgba\n");
   } else {
      struct brw_sampler_default_color *sdc = (intel->batch.bo->virtual +
					       offset);
      batch_out(brw, name, offset, 0, "r %f\n", sdc->color[0]);
      batch_out(brw, name, offset, 1, "g %f\n", sdc->color[1]);
      batch_out(brw, name, offset, 2, "b %f\n", sdc->color[2]);
      batch_out(brw, name, offset, 3, "a %f\n", sdc->color[3]);
   }
}

static void dump_sampler_state(struct brw_context *brw,
			       uint32_t offset, uint32_t size)
{
   struct intel_context *intel = &brw->intel;
   int i;
   struct brw_sampler_state *samp = intel->batch.bo->virtual + offset;

   assert(intel->gen < 7);

   for (i = 0; i < size / sizeof(*samp); i++) {
      char name[20];

      sprintf(name, "WM SAMP%d", i);
      batch_out(brw, name, offset, 0, "filtering\n");
      batch_out(brw, name, offset, 1, "wrapping, lod\n");
      batch_out(brw, name, offset, 2, "default color pointer\n");
      batch_out(brw, name, offset, 3, "chroma key, aniso\n");

      samp++;
      offset += sizeof(*samp);
   }
}

static void dump_gen7_sampler_state(struct brw_context *brw,
				    uint32_t offset, uint32_t size)
{
   struct intel_context *intel = &brw->intel;
   struct gen7_sampler_state *samp = intel->batch.bo->virtual + offset;
   int i;

   assert(intel->gen >= 7);

   for (i = 0; i < size / sizeof(*samp); i++) {
      char name[20];

      sprintf(name, "WM SAMP%d", i);
      batch_out(brw, name, offset, 0, "filtering\n");
      batch_out(brw, name, offset, 1, "wrapping, lod\n");
      batch_out(brw, name, offset, 2, "default color pointer\n");
      batch_out(brw, name, offset, 3, "chroma key, aniso\n");

      samp++;
      offset += sizeof(*samp);
   }
}


static void dump_sf_viewport_state(struct brw_context *brw,
				   uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "SF VP";
   struct brw_sf_viewport *vp = intel->batch.bo->virtual + offset;

   assert(intel->gen < 7);

   batch_out(brw, name, offset, 0, "m00 = %f\n", vp->viewport.m00);
   batch_out(brw, name, offset, 1, "m11 = %f\n", vp->viewport.m11);
   batch_out(brw, name, offset, 2, "m22 = %f\n", vp->viewport.m22);
   batch_out(brw, name, offset, 3, "m30 = %f\n", vp->viewport.m30);
   batch_out(brw, name, offset, 4, "m31 = %f\n", vp->viewport.m31);
   batch_out(brw, name, offset, 5, "m32 = %f\n", vp->viewport.m32);

   batch_out(brw, name, offset, 6, "top left = %d,%d\n",
	     vp->scissor.xmin, vp->scissor.ymin);
   batch_out(brw, name, offset, 7, "bottom right = %d,%d\n",
	     vp->scissor.xmax, vp->scissor.ymax);
}

static void dump_clip_viewport_state(struct brw_context *brw,
				     uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "CLIP VP";
   struct brw_clipper_viewport *vp = intel->batch.bo->virtual + offset;

   assert(intel->gen < 7);

   batch_out(brw, name, offset, 0, "xmin = %f\n", vp->xmin);
   batch_out(brw, name, offset, 1, "xmax = %f\n", vp->xmax);
   batch_out(brw, name, offset, 2, "ymin = %f\n", vp->ymin);
   batch_out(brw, name, offset, 3, "ymax = %f\n", vp->ymax);
}

static void dump_sf_clip_viewport_state(struct brw_context *brw,
					uint32_t offset)
{
   struct intel_context *intel = &brw->intel;
   const char *name = "SF_CLIP VP";
   struct gen7_sf_clip_viewport *vp = intel->batch.bo->virtual + offset;

   assert(intel->gen >= 7);

   batch_out(brw, name, offset, 0, "m00 = %f\n", vp->viewport.m00);
   batch_out(brw, name, offset, 1, "m11 = %f\n", vp->viewport.m11);
   batch_out(brw, name, offset, 2, "m22 = %f\n", vp->viewport.m22);
   batch_out(brw, name, offset, 3, "m30 = %f\n", vp->viewport.m30);
   batch_out(brw, name, offset, 4, "m31 = %f\n", vp->viewport.m31);
   batch_out(brw, name, offset, 5, "m32 = %f\n", vp->viewport.m32);
   batch_out(brw, name, offset, 6, "guardband xmin = %f\n", vp->guardband.xmin);
   batch_out(brw, name, offset, 7, "guardband xmax = %f\n", vp->guardband.xmax);
   batch_out(brw, name, offset, 8, "guardband ymin = %f\n", vp->guardband.ymin);
   batch_out(brw, name, offset, 9, "guardband ymax = %f\n", vp->guardband.ymax);
}


static void dump_cc_viewport_state(struct brw_context *brw, uint32_t offset)
{
   const char *name = "CC VP";
   struct brw_cc_viewport *vp = brw->intel.batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "min_depth = %f\n", vp->min_depth);
   batch_out(brw, name, offset, 1, "max_depth = %f\n", vp->max_depth);
}

static void dump_depth_stencil_state(struct brw_context *brw, uint32_t offset)
{
   const char *name = "D_S";
   struct gen6_depth_stencil_state *ds = brw->intel.batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0,
	     "stencil %sable, func %d, write %sable\n",
	     ds->ds0.stencil_enable ? "en" : "dis",
	     ds->ds0.stencil_func,
	     ds->ds0.stencil_write_enable ? "en" : "dis");
   batch_out(brw, name, offset, 1,
	     "stencil test mask 0x%x, write mask 0x%x\n",
	     ds->ds1.stencil_test_mask, ds->ds1.stencil_write_mask);
   batch_out(brw, name, offset, 2,
	     "depth test %sable, func %d, write %sable\n",
	     ds->ds2.depth_test_enable ? "en" : "dis",
	     ds->ds2.depth_test_func,
	     ds->ds2.depth_write_enable ? "en" : "dis");
}

static void dump_cc_state_gen4(struct brw_context *brw, uint32_t offset)
{
   const char *name = "CC";

   batch_out(brw, name, offset, 0, "cc0\n");
   batch_out(brw, name, offset, 1, "cc1\n");
   batch_out(brw, name, offset, 2, "cc2\n");
   batch_out(brw, name, offset, 3, "cc3\n");
   batch_out(brw, name, offset, 4, "cc4: viewport offset\n");
   batch_out(brw, name, offset, 5, "cc5\n");
   batch_out(brw, name, offset, 6, "cc6\n");
   batch_out(brw, name, offset, 7, "cc7\n");
}

static void dump_cc_state_gen6(struct brw_context *brw, uint32_t offset)
{
   const char *name = "CC";
   struct gen6_color_calc_state *cc = brw->intel.batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0,
	     "alpha test format %s, round disable %d, stencil ref %d, "
	     "bf stencil ref %d\n",
	     cc->cc0.alpha_test_format ? "FLOAT32" : "UNORM8",
	     cc->cc0.round_disable,
	     cc->cc0.stencil_ref,
	     cc->cc0.bf_stencil_ref);
   batch_out(brw, name, offset, 1, "\n");
   batch_out(brw, name, offset, 2, "constant red %f\n", cc->constant_r);
   batch_out(brw, name, offset, 3, "constant green %f\n", cc->constant_g);
   batch_out(brw, name, offset, 4, "constant blue %f\n", cc->constant_b);
   batch_out(brw, name, offset, 5, "constant alpha %f\n", cc->constant_a);
}

static void dump_blend_state(struct brw_context *brw, uint32_t offset)
{
   const char *name = "BLEND";

   batch_out(brw, name, offset, 0, "\n");
   batch_out(brw, name, offset, 1, "\n");
}

static void
dump_scissor(struct brw_context *brw, uint32_t offset)
{
   const char *name = "SCISSOR";
   struct intel_context *intel = &brw->intel;
   struct gen6_scissor_rect *scissor = intel->batch.bo->virtual + offset;

   batch_out(brw, name, offset, 0, "xmin %d, ymin %d\n",
	     scissor->xmin, scissor->ymin);
   batch_out(brw, name, offset, 1, "xmax %d, ymax %d\n",
	     scissor->xmax, scissor->ymax);
}

static void
dump_vs_constants(struct brw_context *brw, uint32_t offset, uint32_t size)
{
   const char *name = "VS_CONST";
   struct intel_context *intel = &brw->intel;
   uint32_t *as_uint = intel->batch.bo->virtual + offset;
   float *as_float = intel->batch.bo->virtual + offset;
   int i;

   for (i = 0; i < size / 4; i += 4) {
      batch_out(brw, name, offset, i, "%3d: (% f % f % f % f) (0x%08x 0x%08x 0x%08x 0x%08x)\n",
		i / 4,
		as_float[i], as_float[i + 1], as_float[i + 2], as_float[i + 3],
		as_uint[i], as_uint[i + 1], as_uint[i + 2], as_uint[i + 3]);
   }
}

static void
dump_wm_constants(struct brw_context *brw, uint32_t offset, uint32_t size)
{
   const char *name = "WM_CONST";
   struct intel_context *intel = &brw->intel;
   uint32_t *as_uint = intel->batch.bo->virtual + offset;
   float *as_float = intel->batch.bo->virtual + offset;
   int i;

   for (i = 0; i < size / 4; i += 4) {
      batch_out(brw, name, offset, i, "%3d: (% f % f % f % f) (0x%08x 0x%08x 0x%08x 0x%08x)\n",
		i / 4,
		as_float[i], as_float[i + 1], as_float[i + 2], as_float[i + 3],
		as_uint[i], as_uint[i + 1], as_uint[i + 2], as_uint[i + 3]);
   }
}

static void dump_binding_table(struct brw_context *brw, uint32_t offset,
			       uint32_t size)
{
   char name[20];
   int i;
   uint32_t *data = brw->intel.batch.bo->virtual + offset;

   for (i = 0; i < size / 4; i++) {
      if (data[i] == 0)
	 continue;

      sprintf(name, "BIND%d", i);
      batch_out(brw, name, offset, i, "surface state address\n");
   }
}

static bool
init_brw(struct brw_context *brw, struct ilo_3d_pipeline *p)
{
   brw->intel.gen = ILO_GEN_GET_MAJOR(p->dev->gen);
   brw->intel.batch.bo = &brw->intel.batch.bo_dst;

   brw->intel.batch.bo_dst.virtual = intel_bo_map(p->cp->bo, false);
   if (!brw->intel.batch.bo_dst.virtual)
      return false;

   return true;
}

static void
dump_3d_state(struct ilo_3d_pipeline *p)
{
   struct brw_context brw;
   int num_states, i;

   if (!init_brw(&brw, p))
      return;

   if (brw.intel.gen >= 7) {
      dump_cc_viewport_state(&brw, p->state.CC_VIEWPORT);
      dump_sf_clip_viewport_state(&brw, p->state.SF_CLIP_VIEWPORT);
   }
   else {
      dump_clip_viewport_state(&brw, p->state.CLIP_VIEWPORT);
      dump_sf_viewport_state(&brw, p->state.SF_VIEWPORT);
      dump_cc_viewport_state(&brw, p->state.CC_VIEWPORT);
   }

   dump_blend_state(&brw, p->state.BLEND_STATE);
   dump_cc_state_gen6(&brw, p->state.COLOR_CALC_STATE);
   dump_depth_stencil_state(&brw, p->state.DEPTH_STENCIL_STATE);

   /* VS */
   num_states = p->state.vs.BINDING_TABLE_STATE_size;
   for (i = 0; i < num_states; i++) {
      if (brw.intel.gen < 7)
         dump_surface_state(&brw, p->state.vs.SURFACE_STATE[i]);
      else
         dump_gen7_surface_state(&brw, p->state.vs.SURFACE_STATE[i]);
   }
   dump_binding_table(&brw, p->state.vs.BINDING_TABLE_STATE, num_states * 4);

   num_states = 0;
   for (i = 0; i < Elements(p->state.vs.SAMPLER_BORDER_COLOR_STATE); i++) {
      if (!p->state.vs.SAMPLER_BORDER_COLOR_STATE[i])
         continue;

      dump_sdc(&brw, p->state.vs.SAMPLER_BORDER_COLOR_STATE[i]);
      num_states++;
   }
   if (brw.intel.gen < 7)
      dump_sampler_state(&brw, p->state.vs.SAMPLER_STATE, num_states * 16);
   else
      dump_gen7_sampler_state(&brw, p->state.vs.SAMPLER_STATE, num_states * 16);

   if (p->state.vs.PUSH_CONSTANT_BUFFER_size) {
      dump_vs_constants(&brw, p->state.vs.PUSH_CONSTANT_BUFFER,
            p->state.vs.PUSH_CONSTANT_BUFFER_size);
   }

   /* GS */
   num_states = p->state.gs.BINDING_TABLE_STATE_size;
   for (i = 0; i < num_states; i++) {
      if (!p->state.gs.SURFACE_STATE[i])
         continue;

      if (brw.intel.gen < 7)
         dump_surface_state(&brw, p->state.gs.SURFACE_STATE[i]);
      else
         dump_gen7_surface_state(&brw, p->state.gs.SURFACE_STATE[i]);
   }
   dump_binding_table(&brw, p->state.gs.BINDING_TABLE_STATE, num_states * 4);

   /* WM */
   num_states = p->state.wm.BINDING_TABLE_STATE_size;
   for (i = 0; i < num_states; i++) {
      if (!p->state.wm.SURFACE_STATE[i])
         continue;

      if (brw.intel.gen < 7)
         dump_surface_state(&brw, p->state.wm.SURFACE_STATE[i]);
      else
         dump_gen7_surface_state(&brw, p->state.wm.SURFACE_STATE[i]);
   }
   dump_binding_table(&brw, p->state.wm.BINDING_TABLE_STATE, num_states * 4);

   num_states = 0;
   for (i = 0; i < Elements(p->state.wm.SAMPLER_BORDER_COLOR_STATE); i++) {
      if (!p->state.wm.SAMPLER_BORDER_COLOR_STATE[i])
         continue;

      dump_sdc(&brw, p->state.wm.SAMPLER_BORDER_COLOR_STATE[i]);
      num_states++;
   }
   if (brw.intel.gen < 7)
      dump_sampler_state(&brw, p->state.wm.SAMPLER_STATE, num_states * 16);
   else
      dump_gen7_sampler_state(&brw, p->state.wm.SAMPLER_STATE, num_states * 16);

   if (p->state.wm.PUSH_CONSTANT_BUFFER_size) {
      dump_wm_constants(&brw, p->state.wm.PUSH_CONSTANT_BUFFER,
            p->state.wm.PUSH_CONSTANT_BUFFER_size);
   }

   dump_scissor(&brw, p->state.SCISSOR_RECT);

   (void) dump_vs_state;
   (void) dump_gs_state;
   (void) dump_clip_state;
   (void) dump_sf_state;
   (void) dump_wm_state;
   (void) dump_cc_state_gen4;

   intel_bo_unmap(p->cp->bo);
}

/**
 * Dump the pipeline.
 */
void
ilo_3d_pipeline_dump(struct ilo_3d_pipeline *p)
{
   ilo_cp_dump(p->cp);
   dump_3d_state(p);
}
