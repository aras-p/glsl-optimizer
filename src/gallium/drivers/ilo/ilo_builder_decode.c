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

#include <stdio.h>
#include <stdarg.h>
#include "genhw/genhw.h"
#include "shader/toy_compiler.h"
#include "intel_winsys.h"
#include "ilo_builder.h"

static const uint32_t *
writer_pointer(const struct ilo_builder *builder,
               enum ilo_builder_writer_type which,
               unsigned offset)
{
   const struct ilo_builder_writer *writer = &builder->writers[which];
   return (const uint32_t *) ((const char *) writer->ptr + offset);
}

static uint32_t _util_printf_format(5, 6)
writer_dw(const struct ilo_builder *builder,
          enum ilo_builder_writer_type which,
          unsigned offset, unsigned dw_index,
          const char *format, ...)
{
   const uint32_t *dw = writer_pointer(builder, which, offset);
   va_list ap;
   char desc[16];
   int len;

   ilo_printf("0x%08x:      0x%08x: ",
         offset + (dw_index << 2), dw[dw_index]);

   va_start(ap, format);
   len = vsnprintf(desc, sizeof(desc), format, ap);
   va_end(ap);

   if (len >= sizeof(desc)) {
      len = sizeof(desc) - 1;
      desc[len] = '\0';
   }

   if (desc[len - 1] == '\n') {
      desc[len - 1] = '\0';
      ilo_printf("%8s: \n", desc);
   } else {
      ilo_printf("%8s: ", desc);
   }

   return dw[dw_index];
}

static void
writer_decode_blob(const struct ilo_builder *builder,
                   enum ilo_builder_writer_type which,
                   const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 4;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      const uint32_t *dw = writer_pointer(builder, which, offset);

      writer_dw(builder, which, offset, 0, "BLOB%d", i);
      /* output a single line for all four DWords */
      ilo_printf("(% f, % f, % f, % f) (0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
               uif(dw[0]), uif(dw[1]), uif(dw[2]), uif(dw[3]),
               dw[0], dw[1], dw[2], dw[3]);

      offset += state_size;
   }
}

static void
writer_decode_clip_viewport(const struct ilo_builder *builder,
                            enum ilo_builder_writer_type which,
                            const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 4;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      uint32_t dw;

      dw = writer_dw(builder, which, offset, 0, "CLIP VP%d", i);
      ilo_printf("xmin = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 1, "CLIP VP%d", i);
      ilo_printf("xmax = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 2, "CLIP VP%d", i);
      ilo_printf("ymin = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 3, "CLIP VP%d", i);
      ilo_printf("ymax = %f\n", uif(dw));

      offset += state_size;
   }
}

static void
writer_decode_sf_clip_viewport_gen7(const struct ilo_builder *builder,
                                    enum ilo_builder_writer_type which,
                                    const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 16;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      uint32_t dw;

      dw = writer_dw(builder, which, offset, 0, "SF_CLIP VP%d", i);
      ilo_printf("m00 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 1, "SF_CLIP VP%d", i);
      ilo_printf("m11 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 2, "SF_CLIP VP%d", i);
      ilo_printf("m22 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 3, "SF_CLIP VP%d", i);
      ilo_printf("m30 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 4, "SF_CLIP VP%d", i);
      ilo_printf("m31 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 5, "SF_CLIP VP%d", i);
      ilo_printf("m32 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 8, "SF_CLIP VP%d", i);
      ilo_printf("guardband xmin = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 9, "SF_CLIP VP%d", i);
      ilo_printf("guardband xmax = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 10, "SF_CLIP VP%d", i);
      ilo_printf("guardband ymin = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 11, "SF_CLIP VP%d", i);
      ilo_printf("guardband ymax = %f\n", uif(dw));

      offset += state_size;
   }
}

static void
writer_decode_sf_viewport_gen6(const struct ilo_builder *builder,
                               enum ilo_builder_writer_type which,
                               const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 8;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      uint32_t dw;

      dw = writer_dw(builder, which, offset, 0, "SF VP%d", i);
      ilo_printf("m00 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 1, "SF VP%d", i);
      ilo_printf("m11 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 2, "SF VP%d", i);
      ilo_printf("m22 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 3, "SF VP%d", i);
      ilo_printf("m30 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 4, "SF VP%d", i);
      ilo_printf("m31 = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 5, "SF VP%d", i);
      ilo_printf("m32 = %f\n", uif(dw));

      offset += state_size;
   }
}

static void
writer_decode_sf_viewport(const struct ilo_builder *builder,
                          enum ilo_builder_writer_type which,
                          const struct ilo_builder_item *item)
{
   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7))
      writer_decode_sf_clip_viewport_gen7(builder, which, item);
   else
      writer_decode_sf_viewport_gen6(builder, which, item);
}

static void
writer_decode_scissor_rect(const struct ilo_builder *builder,
                           enum ilo_builder_writer_type which,
                           const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 2;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      uint32_t dw;

      dw = writer_dw(builder, which, offset, 0, "SCISSOR%d", i);
      ilo_printf("xmin %d, ymin %d\n",
            GEN_EXTRACT(dw, GEN6_SCISSOR_DW0_MIN_X),
            GEN_EXTRACT(dw, GEN6_SCISSOR_DW0_MIN_Y));

      dw = writer_dw(builder, which, offset, 1, "SCISSOR%d", i);
      ilo_printf("xmax %d, ymax %d\n",
            GEN_EXTRACT(dw, GEN6_SCISSOR_DW1_MAX_X),
            GEN_EXTRACT(dw, GEN6_SCISSOR_DW1_MAX_Y));

      offset += state_size;
   }
}

static void
writer_decode_cc_viewport(const struct ilo_builder *builder,
                          enum ilo_builder_writer_type which,
                          const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 2;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      uint32_t dw;

      dw = writer_dw(builder, which, offset, 0, "CC VP%d", i);
      ilo_printf("min_depth = %f\n", uif(dw));

      dw = writer_dw(builder, which, offset, 1, "CC VP%d", i);
      ilo_printf("max_depth = %f\n", uif(dw));

      offset += state_size;
   }
}

static void
writer_decode_color_calc(const struct ilo_builder *builder,
                         enum ilo_builder_writer_type which,
                         const struct ilo_builder_item *item)
{
   uint32_t dw;

   dw = writer_dw(builder, which, item->offset, 0, "CC");
   ilo_printf("alpha test format %s, round disable %d, "
              "stencil ref %d, bf stencil ref %d\n",
	      GEN_EXTRACT(dw, GEN6_CC_DW0_ALPHATEST) ? "FLOAT32" : "UNORM8",
	      (bool) (dw & GEN6_CC_DW0_ROUND_DISABLE_DISABLE),
	      GEN_EXTRACT(dw, GEN6_CC_DW0_STENCIL0_REF),
	      GEN_EXTRACT(dw, GEN6_CC_DW0_STENCIL1_REF));

   writer_dw(builder, which, item->offset, 1, "CC\n");

   dw = writer_dw(builder, which, item->offset, 2, "CC");
   ilo_printf("constant red %f\n", uif(dw));

   dw = writer_dw(builder, which, item->offset, 3, "CC");
   ilo_printf("constant green %f\n", uif(dw));

   dw = writer_dw(builder, which, item->offset, 4, "CC");
   ilo_printf("constant blue %f\n", uif(dw));

   dw = writer_dw(builder, which, item->offset, 5, "CC");
   ilo_printf("constant alpha %f\n", uif(dw));
}

static void
writer_decode_depth_stencil(const struct ilo_builder *builder,
                            enum ilo_builder_writer_type which,
                            const struct ilo_builder_item *item)
{
   uint32_t dw;

   dw = writer_dw(builder, which, item->offset, 0, "D_S");
   ilo_printf("stencil %sable, func %d, write %sable\n",
         (dw & GEN6_ZS_DW0_STENCIL_TEST_ENABLE) ? "en" : "dis",
         GEN_EXTRACT(dw, GEN6_ZS_DW0_STENCIL0_FUNC),
         (dw & GEN6_ZS_DW0_STENCIL_WRITE_ENABLE) ? "en" : "dis");

   dw = writer_dw(builder, which, item->offset, 1, "D_S");
   ilo_printf("stencil test mask 0x%x, write mask 0x%x\n",
         GEN_EXTRACT(dw, GEN6_ZS_DW1_STENCIL0_VALUEMASK),
         GEN_EXTRACT(dw, GEN6_ZS_DW1_STENCIL0_WRITEMASK));

   dw = writer_dw(builder, which, item->offset, 2, "D_S");
   ilo_printf("depth test %sable, func %d, write %sable\n",
         (dw & GEN6_ZS_DW2_DEPTH_TEST_ENABLE) ? "en" : "dis",
         GEN_EXTRACT(dw, GEN6_ZS_DW2_DEPTH_FUNC),
         (dw & GEN6_ZS_DW2_DEPTH_WRITE_ENABLE) ? "en" : "dis");
}

static void
writer_decode_blend(const struct ilo_builder *builder,
                    enum ilo_builder_writer_type which,
                    const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 2;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      writer_dw(builder, which, offset, 0, "BLEND%d\n", i);
      writer_dw(builder, which, offset, 1, "BLEND%d\n", i);

      offset += state_size;
   }
}

static void
writer_decode_sampler(const struct ilo_builder *builder,
                      enum ilo_builder_writer_type which,
                      const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 4;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      writer_dw(builder, which, offset, 0, "WM SAMP%d", i);
      ilo_printf("filtering\n");

      writer_dw(builder, which, offset, 1, "WM SAMP%d", i);
      ilo_printf("wrapping, lod\n");

      writer_dw(builder, which, offset, 2, "WM SAMP%d", i);
      ilo_printf("default color pointer\n");

      writer_dw(builder, which, offset, 3, "WM SAMP%d", i);
      ilo_printf("chroma key, aniso\n");

      offset += state_size;
   }
}

static void
writer_decode_surface_gen7(const struct ilo_builder *builder,
                           enum ilo_builder_writer_type which,
                           const struct ilo_builder_item *item)
{
   uint32_t dw;

   dw = writer_dw(builder, which, item->offset, 0, "SURF");
   ilo_printf("type 0x%x, format 0x%x, tiling %d, %s array\n",
         GEN_EXTRACT(dw, GEN7_SURFACE_DW0_TYPE),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW0_FORMAT),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW0_TILING),
         (dw & GEN7_SURFACE_DW0_IS_ARRAY) ? "is" : "not");

   writer_dw(builder, which, item->offset, 1, "SURF");
   ilo_printf("offset\n");

   dw = writer_dw(builder, which, item->offset, 2, "SURF");
   ilo_printf("%dx%d size\n",
         GEN_EXTRACT(dw, GEN7_SURFACE_DW2_WIDTH),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW2_HEIGHT));

   dw = writer_dw(builder, which, item->offset, 3, "SURF");
   ilo_printf("depth %d, pitch %d\n",
         GEN_EXTRACT(dw, GEN7_SURFACE_DW3_DEPTH),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW3_PITCH));

   dw = writer_dw(builder, which, item->offset, 4, "SURF");
   ilo_printf("min array element %d, array extent %d\n",
         GEN_EXTRACT(dw, GEN7_SURFACE_DW4_MIN_ARRAY_ELEMENT),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW4_RT_VIEW_EXTENT));

   dw = writer_dw(builder, which, item->offset, 5, "SURF");
   ilo_printf("mip base %d, mips %d, x,y offset: %d,%d\n",
         GEN_EXTRACT(dw, GEN7_SURFACE_DW5_MIN_LOD),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW5_MIP_COUNT_LOD),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW5_X_OFFSET),
         GEN_EXTRACT(dw, GEN7_SURFACE_DW5_Y_OFFSET));

   writer_dw(builder, which, item->offset, 6, "SURF\n");
   writer_dw(builder, which, item->offset, 7, "SURF\n");
}

static void
writer_decode_surface_gen6(const struct ilo_builder *builder,
                           enum ilo_builder_writer_type which,
                           const struct ilo_builder_item *item)
{
   uint32_t dw;

   dw = writer_dw(builder, which, item->offset, 0, "SURF");
   ilo_printf("type 0x%x, format 0x%x\n",
         GEN_EXTRACT(dw, GEN6_SURFACE_DW0_TYPE),
         GEN_EXTRACT(dw, GEN6_SURFACE_DW0_FORMAT));

   writer_dw(builder, which, item->offset, 1, "SURF");
   ilo_printf("offset\n");

   dw = writer_dw(builder, which, item->offset, 2, "SURF");
   ilo_printf("%dx%d size, %d mips\n",
         GEN_EXTRACT(dw, GEN6_SURFACE_DW2_WIDTH),
         GEN_EXTRACT(dw, GEN6_SURFACE_DW2_HEIGHT),
         GEN_EXTRACT(dw, GEN6_SURFACE_DW2_MIP_COUNT_LOD));

   dw = writer_dw(builder, which, item->offset, 3, "SURF");
   ilo_printf("pitch %d, tiling %d\n",
         GEN_EXTRACT(dw, GEN6_SURFACE_DW3_PITCH),
         GEN_EXTRACT(dw, GEN6_SURFACE_DW3_TILING));

   dw = writer_dw(builder, which, item->offset, 4, "SURF");
   ilo_printf("mip base %d\n",
         GEN_EXTRACT(dw, GEN6_SURFACE_DW4_MIN_LOD));

   dw = writer_dw(builder, which, item->offset, 5, "SURF");
   ilo_printf("x,y offset: %d,%d\n",
         GEN_EXTRACT(dw, GEN6_SURFACE_DW5_X_OFFSET),
         GEN_EXTRACT(dw, GEN6_SURFACE_DW5_Y_OFFSET));
}

static void
writer_decode_surface(const struct ilo_builder *builder,
                      enum ilo_builder_writer_type which,
                      const struct ilo_builder_item *item)
{
   if (ilo_dev_gen(builder->dev) >= ILO_GEN(7))
      writer_decode_surface_gen7(builder, which, item);
   else
      writer_decode_surface_gen6(builder, which, item);
}

static void
writer_decode_binding_table(const struct ilo_builder *builder,
                            enum ilo_builder_writer_type which,
                            const struct ilo_builder_item *item)
{
   const unsigned state_size = sizeof(uint32_t) * 1;
   const unsigned count = item->size / state_size;
   unsigned offset = item->offset;
   unsigned i;

   for (i = 0; i < count; i++) {
      writer_dw(builder, which, offset, 0, "BIND");
      ilo_printf("BINDING_TABLE_STATE[%d]\n", i);

      offset += state_size;
   }
}

static void
writer_decode_kernel(const struct ilo_builder *builder,
                     enum ilo_builder_writer_type which,
                     const struct ilo_builder_item *item)
{
   const void *kernel;

   ilo_printf("0x%08x:\n", item->offset);
   kernel = (const void *) writer_pointer(builder, which, item->offset);
   toy_compiler_disassemble(builder->dev, kernel, item->size, true);
}

static const struct {
   void (*func)(const struct ilo_builder *builder,
                enum ilo_builder_writer_type which,
                const struct ilo_builder_item *item);
} writer_decode_table[ILO_BUILDER_ITEM_COUNT] = {
   [ILO_BUILDER_ITEM_BLOB]                = { writer_decode_blob },
   [ILO_BUILDER_ITEM_CLIP_VIEWPORT]       = { writer_decode_clip_viewport },
   [ILO_BUILDER_ITEM_SF_VIEWPORT]         = { writer_decode_sf_viewport },
   [ILO_BUILDER_ITEM_SCISSOR_RECT]        = { writer_decode_scissor_rect },
   [ILO_BUILDER_ITEM_CC_VIEWPORT]         = { writer_decode_cc_viewport },
   [ILO_BUILDER_ITEM_COLOR_CALC]          = { writer_decode_color_calc },
   [ILO_BUILDER_ITEM_DEPTH_STENCIL]       = { writer_decode_depth_stencil },
   [ILO_BUILDER_ITEM_BLEND]               = { writer_decode_blend },
   [ILO_BUILDER_ITEM_SAMPLER]             = { writer_decode_sampler },
   [ILO_BUILDER_ITEM_SURFACE]             = { writer_decode_surface },
   [ILO_BUILDER_ITEM_BINDING_TABLE]       = { writer_decode_binding_table },
   [ILO_BUILDER_ITEM_KERNEL]              = { writer_decode_kernel },
};

static void
ilo_builder_writer_decode_items(struct ilo_builder *builder,
                                enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];
   int i;

   if (!writer->item_used)
      return;

   writer->ptr = intel_bo_map(writer->bo, false);
   if (!writer->ptr)
      return;

   for (i = 0; i < writer->item_used; i++) {
      const struct ilo_builder_item *item = &writer->items[i];

      writer_decode_table[item->type].func(builder, which, item);
   }

   intel_bo_unmap(writer->bo);
}

static void
ilo_builder_writer_decode(struct ilo_builder *builder,
                          enum ilo_builder_writer_type which)
{
   struct ilo_builder_writer *writer = &builder->writers[which];

   assert(writer->bo && !writer->ptr);

   switch (which) {
   case ILO_BUILDER_WRITER_BATCH:
      ilo_printf("decoding batch buffer: %d bytes\n", writer->used);
      if (writer->used)
         intel_winsys_decode_bo(builder->winsys, writer->bo, writer->used);

      ilo_printf("decoding state buffer: %d states\n",
            writer->item_used);
      ilo_builder_writer_decode_items(builder, which);
      break;
   case ILO_BUILDER_WRITER_INSTRUCTION:
      if (true) {
         ilo_printf("skipping instruction buffer: %d kernels\n",
               writer->item_used);
      } else {
         ilo_printf("decoding instruction buffer: %d kernels\n",
               writer->item_used);

         ilo_builder_writer_decode_items(builder, which);
      }
      break;
   default:
      break;
   }
}

/**
 * Decode the builder according to the recorded items.  This can be called
 * only after a successful ilo_builder_end().
 */
void
ilo_builder_decode(struct ilo_builder *builder)
{
   int i;

   assert(!builder->unrecoverable_error);

   for (i = 0; i < ILO_BUILDER_WRITER_COUNT; i++)
      ilo_builder_writer_decode(builder, i);
}
