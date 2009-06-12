/*
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_network.h"
#include "util/u_tile.h"

static uint8_t* read(const char *filename, unsigned size);
static void dump(unsigned src_width, unsigned src_height,
                 unsigned src_stride, enum pipe_format src_format,
                 uint8_t *data, unsigned src_size);

int main(int argc, char** argv)
{
   /* change these */
   unsigned width = 64;
   unsigned height = 64;
   unsigned stride = width * 4;
   unsigned size = stride * height;
   const char *filename = "mybin.bin";
   enum pipe_format format = PIPE_FORMAT_A8R8G8B8_UNORM;

   dump(width, height, stride, format, read(filename, size), size);

   return 0;
}

static void dump(unsigned width, unsigned height,
                 unsigned src_stride, enum pipe_format src_format,
                 uint8_t *data, unsigned src_size)
{
   struct pipe_format_block src_block;

   enum pipe_format dst_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   struct pipe_format_block dst_block;
   unsigned dst_stride;
   unsigned dst_size;
   float *rgba;
   int i;
   char filename[512];

   {
      pf_get_block(src_format, &src_block);
      assert(src_stride >= pf_get_stride(&src_block, width));
      assert(src_size >= pf_get_2d_size(&src_block, src_stride, width));
   }
   {
      pf_get_block(dst_format, &dst_block);
      dst_stride = pf_get_stride(&dst_block, width);
      dst_size = pf_get_2d_size(&dst_block, dst_stride, width);
      rgba = MALLOC(dst_size);
   }

   util_snprintf(filename, 512, "%s.bmp", pf_name(src_format));

   if (pf_is_compressed(src_format)) {
      debug_printf("skipping: %s\n", filename);
      return;
   }

   debug_printf("saving: %s\n", filename);

   for (i = 0; i < height; i++) {
      pipe_tile_raw_to_rgba(src_format, data + src_stride * i,
                            width, 1,
                            &rgba[width*4*i], dst_stride);
   }

   debug_dump_float_rgba_bmp(filename, width, height, rgba, width);

   FREE(rgba);
}

static uint8_t* read(const char *filename, unsigned size)
{
   uint8_t *data;
   FILE *file = fopen(filename, "rb");

   data = MALLOC(size);

   fread(data, 1, size, file);
   fclose(file);

   return data;
}
