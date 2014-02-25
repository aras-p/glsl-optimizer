/*
 * Copyright (c) 2014 Intel Corporation
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
 */


#include "main/context.h"
#include "main/blend.h"
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "main/texformat.h"
#include "program/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"
#include "intel_buffer_objects.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
gen6_update_renderbuffer_surface(struct brw_context *brw,
                                 struct gl_renderbuffer *rb,
                                 bool layered,
                                 unsigned int unit)
{
   struct gl_context *ctx = &brw->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_mipmap_tree *mt = irb->mt;
   uint32_t *surf;
   uint32_t format = 0;
   /* _NEW_BUFFERS */
   mesa_format rb_format = _mesa_get_render_format(ctx, intel_rb_format(irb));
   uint32_t surftype;
   int depth = MAX2(irb->layer_count, 1);
   const GLenum gl_target =
      rb->TexImage ? rb->TexImage->TexObject->Target : GL_TEXTURE_2D;

   uint32_t surf_index =
      brw->wm.prog_data->binding_table.render_target_start + unit;

   intel_miptree_used_for_rendering(irb->mt);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE, 6 * 4, 32,
                          &brw->wm.base.surf_offset[surf_index]);

   format = brw->render_target_format[rb_format];
   if (unlikely(!brw->format_supported_as_render_target[rb_format])) {
      _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
                    __FUNCTION__, _mesa_get_format_name(rb_format));
   }

   switch (gl_target) {
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
      surftype = BRW_SURFACE_2D;
      depth *= 6;
      break;
   case GL_TEXTURE_3D:
      depth = MAX2(irb->mt->logical_depth0, 1);
      /* fallthrough */
   default:
      surftype = translate_tex_target(gl_target);
      break;
   }

   const int min_array_element = layered ? 0 : irb->mt_layer;

   surf[0] = SET_FIELD(surftype, BRW_SURFACE_TYPE) |
             SET_FIELD(format, BRW_SURFACE_FORMAT);

   /* reloc */
   surf[1] = mt->bo->offset64;

   /* In the gen6 PRM Volume 1 Part 1: Graphics Core, Section 7.18.3.7.1
    * (Surface Arrays For all surfaces other than separate stencil buffer):
    *
    * "[DevSNB] Errata: Sampler MSAA Qpitch will be 4 greater than the value
    *  calculated in the equation above , for every other odd Surface Height
    *  starting from 1 i.e. 1,5,9,13"
    *
    * Since this Qpitch errata only impacts the sampler, we have to adjust the
    * input for the rendering surface to achieve the same qpitch. For the
    * affected heights, we increment the height by 1 for the rendering
    * surface.
    */
   int height0 = irb->mt->logical_height0;
   if (brw->gen == 6 && irb->mt->num_samples > 1 && (height0 % 4) == 1)
      height0++;

   surf[2] = SET_FIELD(mt->logical_width0 - 1, BRW_SURFACE_WIDTH) |
             SET_FIELD(height0 - 1, BRW_SURFACE_HEIGHT) |
             SET_FIELD(irb->mt_level - irb->mt->first_level, BRW_SURFACE_LOD);

   surf[3] = brw_get_surface_tiling_bits(mt->tiling) |
             SET_FIELD(depth - 1, BRW_SURFACE_DEPTH) |
             SET_FIELD(mt->pitch - 1, BRW_SURFACE_PITCH);

   surf[4] = brw_get_surface_num_multisamples(mt->num_samples) |
             SET_FIELD(min_array_element, BRW_SURFACE_MIN_ARRAY_ELEMENT) |
             SET_FIELD(depth - 1, BRW_SURFACE_RENDER_TARGET_VIEW_EXTENT);

   surf[5] = (mt->align_h == 4 ? BRW_SURFACE_VERTICAL_ALIGN_ENABLE : 0);

   drm_intel_bo_emit_reloc(brw->batch.bo,
			   brw->wm.base.surf_offset[surf_index] + 4,
			   mt->bo,
			   surf[1] - mt->bo->offset64,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);
}

void
gen6_init_vtable_surface_functions(struct brw_context *brw)
{
   gen4_init_vtable_surface_functions(brw);
   brw->vtbl.update_renderbuffer_surface = gen6_update_renderbuffer_surface;
}
