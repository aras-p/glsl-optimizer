/*
 * Copyright © 2012 Intel Corporation
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

#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_defines.h"
#include "gen6_blorp.h"
#include "gen7_blorp.h"

brw_blorp_mip_info::brw_blorp_mip_info()
   : mt(NULL),
     level(0),
     layer(0)
{
}

brw_blorp_surface_info::brw_blorp_surface_info()
   : map_stencil_as_y_tiled(false),
     num_samples(0)
{
}

void
brw_blorp_mip_info::set(struct intel_mipmap_tree *mt,
                        unsigned int level, unsigned int layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   this->mt = mt;
   this->level = level;
   this->layer = layer;
}

void
brw_blorp_surface_info::set(struct intel_mipmap_tree *mt,
                            unsigned int level, unsigned int layer)
{
   brw_blorp_mip_info::set(mt, level, layer);
   this->num_samples = mt->num_samples;
   this->array_spacing_lod0 = mt->array_spacing_lod0;

   if (mt->format == MESA_FORMAT_S8) {
      /* The miptree is a W-tiled stencil buffer.  Surface states can't be set
       * up for W tiling, so we'll need to use Y tiling and have the WM
       * program swizzle the coordinates.
       */
      this->map_stencil_as_y_tiled = true;
   } else {
      this->map_stencil_as_y_tiled = false;
   }
}

void
brw_blorp_mip_info::get_draw_offsets(uint32_t *draw_x, uint32_t *draw_y) const
{
   /* Construct a dummy renderbuffer just to extract tile offsets. */
   struct intel_renderbuffer rb;
   rb.mt = mt;
   rb.mt_level = level;
   rb.mt_layer = layer;
   intel_renderbuffer_set_draw_offset(&rb);
   *draw_x = rb.draw_x;
   *draw_y = rb.draw_y;
}

brw_blorp_params::brw_blorp_params()
   : x0(0),
     y0(0),
     x1(0),
     y1(0),
     depth_format(0),
     hiz_op(GEN6_HIZ_OP_NONE),
     num_samples(0),
     use_wm_prog(false)
{
}

extern "C" {
void
intel_hiz_exec(struct intel_context *intel, struct intel_mipmap_tree *mt,
	       unsigned int level, unsigned int layer, gen6_hiz_op op)
{
   brw_hiz_op_params params(mt, level, layer, op);
   brw_blorp_exec(intel, &params);
}

} /* extern "C" */

void
brw_blorp_exec(struct intel_context *intel, const brw_blorp_params *params)
{
   switch (intel->gen) {
   case 6:
      gen6_blorp_exec(intel, params);
      break;
   case 7:
      gen7_blorp_exec(intel, params);
      break;
   default:
      /* BLORP is not supported before Gen6. */
      assert(false);
      break;
   }
}

brw_hiz_op_params::brw_hiz_op_params(struct intel_mipmap_tree *mt,
                                     unsigned int level,
                                     unsigned int layer,
                                     gen6_hiz_op op)
{
   this->hiz_op = op;

   depth.set(mt, level, layer);
   depth.get_miplevel_dims(&x1, &y1);

   assert(mt->hiz_mt != NULL);

   switch (mt->format) {
   case MESA_FORMAT_Z16:       depth_format = BRW_DEPTHFORMAT_D16_UNORM; break;
   case MESA_FORMAT_Z32_FLOAT: depth_format = BRW_DEPTHFORMAT_D32_FLOAT; break;
   case MESA_FORMAT_X8_Z24:    depth_format = BRW_DEPTHFORMAT_D24_UNORM_X8_UINT; break;
   default:                    assert(0); break;
   }
}

uint32_t
brw_hiz_op_params::get_wm_prog(struct brw_context *brw,
                               brw_blorp_prog_data **prog_data) const
{
   return 0;
}
