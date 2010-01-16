/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * (C) Copyright IBM Corporation 2008
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Ian Romanick <idr@us.ibm.com>
  */

#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "spu_exec.h"
#include "spu_vertex_shader.h"
#include "spu_main.h"
#include "spu_dcache.h"

typedef void (*spu_fetch_func)(qword *out, const qword *in,
			       const qword *shuffle_data);


PIPE_ALIGN_VAR(16) static const qword
fetch_shuffle_data[5] = {
   /* Shuffle used by CVT_64_FLOAT
    */
   {
      0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13,
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   },

   /* Shuffle used by CVT_8_USCALED and CVT_8_SSCALED
    */
   {
      0x00, 0x80, 0x80, 0x80, 0x01, 0x80, 0x80, 0x80,
      0x02, 0x80, 0x80, 0x80, 0x03, 0x80, 0x80, 0x80,
   },
   
   /* Shuffle used by CVT_16_USCALED and CVT_16_SSCALED
    */
   {
      0x00, 0x01, 0x80, 0x80, 0x02, 0x03, 0x80, 0x80,
      0x04, 0x05, 0x80, 0x80, 0x06, 0x07, 0x80, 0x80,
   },
   
   /* High value shuffle used by trans4x4.
    */
   {
      0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13,
      0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17
   },

   /* Low value shuffle used by trans4x4.
    */
   {
      0x08, 0x09, 0x0A, 0x0B, 0x18, 0x19, 0x1A, 0x1B,
      0x0C, 0x0D, 0x0E, 0x0F, 0x1C, 0x1D, 0x1E, 0x1F
   }
};


/**
 * Fetch vertex attributes for 'count' vertices.
 */
static void generic_vertex_fetch(struct spu_vs_context *draw,
                                 struct spu_exec_machine *machine,
                                 const unsigned *elts,
                                 unsigned count)
{
   unsigned nr_attrs = draw->vertex_fetch.nr_attrs;
   unsigned attr;

   ASSERT(count <= 4);

#if DRAW_DBG
   printf("SPU: %s count = %u, nr_attrs = %u\n", 
          __FUNCTION__, count, nr_attrs);
#endif

   /* loop over vertex attributes (vertex shader inputs)
    */
   for (attr = 0; attr < nr_attrs; attr++) {
      const unsigned pitch = draw->vertex_fetch.pitch[attr];
      const uint64_t src = draw->vertex_fetch.src_ptr[attr];
      const spu_fetch_func fetch = (spu_fetch_func)
	  (draw->vertex_fetch.code + draw->vertex_fetch.code_offset[attr]);
      unsigned i;
      unsigned idx;
      const unsigned bytes_per_entry = draw->vertex_fetch.size[attr];
      const unsigned quads_per_entry = (bytes_per_entry + 15) / 16;
      PIPE_ALIGN_VAR(16) qword in[2 * 4];


      /* Fetch four attributes for four vertices.  
       */
      idx = 0;
      for (i = 0; i < count; i++) {
         const uint64_t addr = src + (elts[i] * pitch);

#if DRAW_DBG
         printf("SPU: fetching = 0x%llx\n", addr);
#endif

         spu_dcache_fetch_unaligned(& in[idx], addr, bytes_per_entry);
         idx += quads_per_entry;
      }

      /* Be nice and zero out any missing vertices.
       */
      (void) memset(& in[idx], 0, (8 - idx) * sizeof(qword));


      /* Convert all 4 vertices to vectors of float.
       */
      (*fetch)(&machine->Inputs[attr].xyzw[0].q, in, fetch_shuffle_data);
   }
}


void spu_update_vertex_fetch( struct spu_vs_context *draw )
{
   draw->vertex_fetch.fetch_func = generic_vertex_fetch;
}
