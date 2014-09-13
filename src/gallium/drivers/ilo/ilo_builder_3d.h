/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_BUILDER_3D_H
#define ILO_BUILDER_3D_H

#include "genhw/genhw.h"

#include "ilo_common.h"
#include "ilo_builder_3d_top.h"
#include "ilo_builder_3d_bottom.h"

/**
 * Translate a pipe primitive type to the matching hardware primitive type.
 */
static inline int
gen6_3d_translate_pipe_prim(unsigned prim)
{
   static const int prim_mapping[ILO_PRIM_MAX] = {
      [PIPE_PRIM_POINTS]                     = GEN6_3DPRIM_POINTLIST,
      [PIPE_PRIM_LINES]                      = GEN6_3DPRIM_LINELIST,
      [PIPE_PRIM_LINE_LOOP]                  = GEN6_3DPRIM_LINELOOP,
      [PIPE_PRIM_LINE_STRIP]                 = GEN6_3DPRIM_LINESTRIP,
      [PIPE_PRIM_TRIANGLES]                  = GEN6_3DPRIM_TRILIST,
      [PIPE_PRIM_TRIANGLE_STRIP]             = GEN6_3DPRIM_TRISTRIP,
      [PIPE_PRIM_TRIANGLE_FAN]               = GEN6_3DPRIM_TRIFAN,
      [PIPE_PRIM_QUADS]                      = GEN6_3DPRIM_QUADLIST,
      [PIPE_PRIM_QUAD_STRIP]                 = GEN6_3DPRIM_QUADSTRIP,
      [PIPE_PRIM_POLYGON]                    = GEN6_3DPRIM_POLYGON,
      [PIPE_PRIM_LINES_ADJACENCY]            = GEN6_3DPRIM_LINELIST_ADJ,
      [PIPE_PRIM_LINE_STRIP_ADJACENCY]       = GEN6_3DPRIM_LINESTRIP_ADJ,
      [PIPE_PRIM_TRIANGLES_ADJACENCY]        = GEN6_3DPRIM_TRILIST_ADJ,
      [PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY]   = GEN6_3DPRIM_TRISTRIP_ADJ,
      [ILO_PRIM_RECTANGLES]                  = GEN6_3DPRIM_RECTLIST,
   };

   assert(prim_mapping[prim]);

   return prim_mapping[prim];
}

static inline void
gen6_3DPRIMITIVE(struct ilo_builder *builder,
                 const struct pipe_draw_info *info,
                 const struct ilo_ib_state *ib)
{
   const uint8_t cmd_len = 6;
   const int prim = gen6_3d_translate_pipe_prim(info->mode);
   const int vb_access = (info->indexed) ?
      GEN6_3DPRIM_DW0_ACCESS_RANDOM : GEN6_3DPRIM_DW0_ACCESS_SEQUENTIAL;
   const uint32_t vb_start = info->start +
      ((info->indexed) ? ib->draw_start_offset : 0);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 6, 6);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) |
           vb_access |
           prim << GEN6_3DPRIM_DW0_TYPE__SHIFT |
           (cmd_len - 2);
   dw[1] = info->count;
   dw[2] = vb_start;
   dw[3] = info->instance_count;
   dw[4] = info->start_instance;
   dw[5] = info->index_bias;
}

static inline void
gen7_3DPRIMITIVE(struct ilo_builder *builder,
                 const struct pipe_draw_info *info,
                 const struct ilo_ib_state *ib)
{
   const uint8_t cmd_len = 7;
   const int prim = gen6_3d_translate_pipe_prim(info->mode);
   const int vb_access = (info->indexed) ?
      GEN7_3DPRIM_DW1_ACCESS_RANDOM : GEN7_3DPRIM_DW1_ACCESS_SEQUENTIAL;
   const uint32_t vb_start = info->start +
      ((info->indexed) ? ib->draw_start_offset : 0);
   uint32_t *dw;

   ILO_DEV_ASSERT(builder->dev, 7, 7.5);

   ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) | (cmd_len - 2);
   dw[1] = vb_access | prim;
   dw[2] = info->count;
   dw[3] = vb_start;
   dw[4] = info->instance_count;
   dw[5] = info->start_instance;
   dw[6] = info->index_bias;
}

#endif /* ILO_BUILDER_3D_H */
