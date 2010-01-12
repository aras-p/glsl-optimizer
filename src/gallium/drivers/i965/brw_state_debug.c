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
      


#include "brw_context.h"
#include "brw_state.h"


struct dirty_bit_map {
   uint32_t bit;
   char *name;
   uint32_t count;
};

#define DEFINE_BIT(name) {name, #name, 0}

static struct dirty_bit_map mesa_bits[] = {
   DEFINE_BIT(PIPE_NEW_DEPTH_STENCIL_ALPHA),
   DEFINE_BIT(PIPE_NEW_RAST),
   DEFINE_BIT(PIPE_NEW_BLEND),
   DEFINE_BIT(PIPE_NEW_VIEWPORT),
   DEFINE_BIT(PIPE_NEW_SAMPLERS),
   DEFINE_BIT(PIPE_NEW_VERTEX_BUFFER),
   DEFINE_BIT(PIPE_NEW_VERTEX_ELEMENT),
   DEFINE_BIT(PIPE_NEW_FRAGMENT_SHADER),
   DEFINE_BIT(PIPE_NEW_VERTEX_SHADER),
   DEFINE_BIT(PIPE_NEW_FRAGMENT_CONSTANTS),
   DEFINE_BIT(PIPE_NEW_VERTEX_CONSTANTS),
   DEFINE_BIT(PIPE_NEW_CLIP),
   DEFINE_BIT(PIPE_NEW_INDEX_BUFFER),
   DEFINE_BIT(PIPE_NEW_INDEX_RANGE),
   DEFINE_BIT(PIPE_NEW_BLEND_COLOR),
   DEFINE_BIT(PIPE_NEW_POLYGON_STIPPLE),
   DEFINE_BIT(PIPE_NEW_FRAMEBUFFER_DIMENSIONS),
   DEFINE_BIT(PIPE_NEW_DEPTH_BUFFER),
   DEFINE_BIT(PIPE_NEW_COLOR_BUFFERS),
   DEFINE_BIT(PIPE_NEW_QUERY),
   DEFINE_BIT(PIPE_NEW_SCISSOR),
   DEFINE_BIT(PIPE_NEW_BOUND_TEXTURES),
   DEFINE_BIT(PIPE_NEW_NR_CBUFS),
   {0, 0, 0}
};

static struct dirty_bit_map brw_bits[] = {
   DEFINE_BIT(BRW_NEW_URB_FENCE),
   DEFINE_BIT(BRW_NEW_FRAGMENT_PROGRAM),
   DEFINE_BIT(BRW_NEW_VERTEX_PROGRAM),
   DEFINE_BIT(BRW_NEW_INPUT_DIMENSIONS),
   DEFINE_BIT(BRW_NEW_CURBE_OFFSETS),
   DEFINE_BIT(BRW_NEW_REDUCED_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_CONTEXT),
   DEFINE_BIT(BRW_NEW_WM_INPUT_DIMENSIONS),
   DEFINE_BIT(BRW_NEW_PSP),
   DEFINE_BIT(BRW_NEW_WM_SURFACES),
   DEFINE_BIT(BRW_NEW_xxx),
   DEFINE_BIT(BRW_NEW_INDICES),
   {0, 0, 0}
};

static struct dirty_bit_map cache_bits[] = {
   DEFINE_BIT(CACHE_NEW_CC_VP),
   DEFINE_BIT(CACHE_NEW_CC_UNIT),
   DEFINE_BIT(CACHE_NEW_WM_PROG),
   DEFINE_BIT(CACHE_NEW_SAMPLER_DEFAULT_COLOR),
   DEFINE_BIT(CACHE_NEW_SAMPLER),
   DEFINE_BIT(CACHE_NEW_WM_UNIT),
   DEFINE_BIT(CACHE_NEW_SF_PROG),
   DEFINE_BIT(CACHE_NEW_SF_VP),
   DEFINE_BIT(CACHE_NEW_SF_UNIT),
   DEFINE_BIT(CACHE_NEW_VS_UNIT),
   DEFINE_BIT(CACHE_NEW_VS_PROG),
   DEFINE_BIT(CACHE_NEW_GS_UNIT),
   DEFINE_BIT(CACHE_NEW_GS_PROG),
   DEFINE_BIT(CACHE_NEW_CLIP_VP),
   DEFINE_BIT(CACHE_NEW_CLIP_UNIT),
   DEFINE_BIT(CACHE_NEW_CLIP_PROG),
   DEFINE_BIT(CACHE_NEW_SURFACE),
   DEFINE_BIT(CACHE_NEW_SURF_BIND),
   {0, 0, 0}
};


static void
brw_update_dirty_count(struct dirty_bit_map *bit_map, int32_t bits)
{
   int i;

   for (i = 0; i < 32; i++) {
      if (bit_map[i].bit == 0)
	 return;

      if (bit_map[i].bit & bits)
	 bit_map[i].count++;
   }
}

static void
brw_print_dirty_count(struct dirty_bit_map *bit_map, int32_t bits)
{
   int i;

   for (i = 0; i < 32; i++) {
      if (bit_map[i].bit == 0)
	 return;

      debug_printf("0x%08x: %12d (%s)\n",
	      bit_map[i].bit, bit_map[i].count, bit_map[i].name);
   }
}

void
brw_update_dirty_counts( unsigned mesa,
			 unsigned brw,
			 unsigned cache )
{
   static int dirty_count = 0;

   brw_update_dirty_count(mesa_bits, mesa);
   brw_update_dirty_count(brw_bits, brw);
   brw_update_dirty_count(cache_bits, cache);
      if (dirty_count++ % 1000 == 0) {
	 brw_print_dirty_count(mesa_bits, mesa);
	 brw_print_dirty_count(brw_bits, brw);
	 brw_print_dirty_count(cache_bits, cache);
	 debug_printf("\n");
      }
}
