/*
 * (C) Copyright IBM Corporation 2008
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
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <inttypes.h>
#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_format.h"

#include "../auxiliary/draw/draw_context.h"
#include "../auxiliary/draw/draw_private.h"

#include "cell_context.h"
#include "rtasm/rtasm_ppc_spe.h"


/**
 * Emit a 4x4 matrix transpose operation
 *
 * \param p         Function that the transpose operation is to be appended to
 * \param row0      Register containing row 0 of the source matrix
 * \param row1      Register containing row 1 of the source matrix
 * \param row2      Register containing row 2 of the source matrix
 * \param row3      Register containing row 3 of the source matrix
 * \param dest_ptr  Register containing the address of the destination matrix
 * \param shuf_ptr  Register containing the address of the shuffled data
 * \param count     Number of colums to actually be written to the destination
 *
 * \note
 * This function assumes that the registers named by \c row0, \c row1,
 * \c row2, and \c row3 are scratch and can be modified by the generated code.
 * Furthermore, these registers will be released, via calls to
 * \c release_register, by this function.
 * 
 * \note
 * This function requires that four temporary are available on entry.
 */
static void
emit_matrix_transpose(struct spe_function *p,
		      unsigned row0, unsigned row1, unsigned row2,
		      unsigned row3, unsigned dest_ptr,
		      unsigned shuf_ptr, unsigned count)
{
   int shuf_hi = spe_allocate_available_register(p);
   int shuf_lo = spe_allocate_available_register(p);
   int t1 = spe_allocate_available_register(p);
   int t2 = spe_allocate_available_register(p);
   int t3;
   int t4;
   int col0;
   int col1;
   int col2;
   int col3;


   spe_lqd(p, shuf_hi, shuf_ptr, 3*16);
   spe_lqd(p, shuf_lo, shuf_ptr, 4*16);
   spe_shufb(p, t1, row0, row2, shuf_hi);
   spe_shufb(p, t2, row0, row2, shuf_lo);


   /* row0 and row2 are now no longer needed.  Re-use those registers as
    * temporaries.
    */
   t3 = row0;
   t4 = row2;

   spe_shufb(p, t3, row1, row3, shuf_hi);
   spe_shufb(p, t4, row1, row3, shuf_lo);


   /* row1 and row3 are now no longer needed.  Re-use those registers as
    * temporaries.
    */
   col0 = row1;
   col1 = row3;

   spe_shufb(p, col0, t1, t3, shuf_hi);
   if (count > 1) {
      spe_shufb(p, col1, t1, t3, shuf_lo);
   }

   /* t1 and t3 are now no longer needed.  Re-use those registers as
    * temporaries.
    */
   col2 = t1;
   col3 = t3;

   if (count > 2) {
      spe_shufb(p, col2, t2, t4, shuf_hi);
   }

   if (count > 3) {
      spe_shufb(p, col3, t2, t4, shuf_lo);
   }


   /* Store the results.  Remember that the stqd instruction is encoded using
    * the qword offset (stand-alone assemblers to the byte-offset to
    * qword-offset conversion for you), so the byte-offset needs be divided by
    * 16.
    */
   switch (count) {
   case 4:
      spe_stqd(p, col3, dest_ptr, 3 * 16);
   case 3:
      spe_stqd(p, col2, dest_ptr, 2 * 16);
   case 2:
      spe_stqd(p, col1, dest_ptr, 1 * 16);
   case 1:
      spe_stqd(p, col0, dest_ptr, 0 * 16);
   }


   /* Release all of the temporary registers used.
    */
   spe_release_register(p, col0);
   spe_release_register(p, col1);
   spe_release_register(p, col2);
   spe_release_register(p, col3);
   spe_release_register(p, shuf_hi);
   spe_release_register(p, shuf_lo);
   spe_release_register(p, t2);
   spe_release_register(p, t4);
}


#if 0
/* This appears to not be used currently */
static void
emit_fetch(struct spe_function *p,
	   unsigned in_ptr, unsigned *offset,
	   unsigned out_ptr, unsigned shuf_ptr,
	   enum pipe_format format)
{
   const unsigned count = (pf_size_x(format) != 0) + (pf_size_y(format) != 0)
       + (pf_size_z(format) != 0) + (pf_size_w(format) != 0);
   const unsigned type = pf_type(format);
   const unsigned bytes = pf_size_x(format);

   int v0 = spe_allocate_available_register(p);
   int v1 = spe_allocate_available_register(p);
   int v2 = spe_allocate_available_register(p);
   int v3 = spe_allocate_available_register(p);
   int tmp = spe_allocate_available_register(p);
   int float_zero = -1;
   int float_one = -1;
   float scale_signed = 0.0;
   float scale_unsigned = 0.0;

   spe_lqd(p, v0, in_ptr, (0 + offset[0]) * 16);
   spe_lqd(p, v1, in_ptr, (1 + offset[0]) * 16);
   spe_lqd(p, v2, in_ptr, (2 + offset[0]) * 16);
   spe_lqd(p, v3, in_ptr, (3 + offset[0]) * 16);
   offset[0] += 4;
   
   switch (bytes) {
   case 1:
      scale_signed = 1.0f / 127.0f;
      scale_unsigned = 1.0f / 255.0f;
      spe_lqd(p, tmp, shuf_ptr, 1 * 16);
      spe_shufb(p, v0, v0, v0, tmp);
      spe_shufb(p, v1, v1, v1, tmp);
      spe_shufb(p, v2, v2, v2, tmp);
      spe_shufb(p, v3, v3, v3, tmp);
      break;
   case 2:
      scale_signed = 1.0f / 32767.0f;
      scale_unsigned = 1.0f / 65535.0f;
      spe_lqd(p, tmp, shuf_ptr, 2 * 16);
      spe_shufb(p, v0, v0, v0, tmp);
      spe_shufb(p, v1, v1, v1, tmp);
      spe_shufb(p, v2, v2, v2, tmp);
      spe_shufb(p, v3, v3, v3, tmp);
      break;
   case 4:
      scale_signed = 1.0f / 2147483647.0f;
      scale_unsigned = 1.0f / 4294967295.0f;
      break;
   default:
      assert(0);
      break;
   }

   switch (type) {
   case PIPE_FORMAT_TYPE_FLOAT:
      break;
   case PIPE_FORMAT_TYPE_UNORM:
      spe_ilhu(p, tmp, ((unsigned) scale_unsigned) >> 16);
      spe_iohl(p, tmp, ((unsigned) scale_unsigned) & 0x0ffff);
      spe_cuflt(p, v0, v0, 0);
      spe_fm(p, v0, v0, tmp);
      break;
   case PIPE_FORMAT_TYPE_SNORM:
      spe_ilhu(p, tmp, ((unsigned) scale_signed) >> 16);
      spe_iohl(p, tmp, ((unsigned) scale_signed) & 0x0ffff);
      spe_csflt(p, v0, v0, 0);
      spe_fm(p, v0, v0, tmp);
      break;
   case PIPE_FORMAT_TYPE_USCALED:
      spe_cuflt(p, v0, v0, 0);
      break;
   case PIPE_FORMAT_TYPE_SSCALED:
      spe_csflt(p, v0, v0, 0);
      break;
   }


   if (count < 4) {
      float_one = spe_allocate_available_register(p);
      spe_il(p, float_one, 1);
      spe_cuflt(p, float_one, float_one, 0);
      
      if (count < 3) {
	 float_zero = spe_allocate_available_register(p);
	 spe_il(p, float_zero, 0);
      }
   }

   spe_release_register(p, tmp);

   emit_matrix_transpose(p, v0, v1, v2, v3, out_ptr, shuf_ptr, count);

   switch (count) {
   case 1:
      spe_stqd(p, float_zero, out_ptr, 1 * 16);
   case 2:
      spe_stqd(p, float_zero, out_ptr, 2 * 16);
   case 3:
      spe_stqd(p, float_one, out_ptr, 3 * 16);
   }

   if (float_zero != -1) {
      spe_release_register(p, float_zero);
   }

   if (float_one != -1) {
      spe_release_register(p, float_one);
   }
}
#endif


void cell_update_vertex_fetch(struct draw_context *draw)
{
#if 0
   struct cell_context *const cell =
       (struct cell_context *) draw->driver_private;
   struct spe_function *p = &cell->attrib_fetch;
   unsigned function_index[PIPE_MAX_ATTRIBS];
   unsigned unique_attr_formats;
   int out_ptr;
   int in_ptr;
   int shuf_ptr;
   unsigned i;
   unsigned j;


   /* Determine how many unique input attribute formats there are.  At the
    * same time, store the index of the lowest numbered attribute that has
    * the same format as any non-unique format.
    */
   unique_attr_formats = 1;
   function_index[0] = 0;
   for (i = 1; i < draw->vertex_fetch.nr_attrs; i++) {
      const enum pipe_format curr_fmt = draw->vertex_element[i].src_format;

      for (j = 0; j < i; j++) {
	 if (curr_fmt == draw->vertex_element[j].src_format) {
	    break;
	 }
      }
      
      if (j == i) {
	 unique_attr_formats++;
      }

      function_index[i] = j;
   }


   /* Each fetch function can be a maximum of 34 instructions (note: this is
    * actually a slight over-estimate).
    */
   spe_init_func(p, 34 * SPE_INST_SIZE * unique_attr_formats);


   /* Allocate registers for the function's input parameters.
    */
   out_ptr = spe_allocate_register(p, 3);
   in_ptr = spe_allocate_register(p, 4);
   shuf_ptr = spe_allocate_register(p, 5);


   /* Generate code for the individual attribute fetch functions.
    */
   for (i = 0; i < draw->vertex_fetch.nr_attrs; i++) {
      unsigned offset;

      if (function_index[i] == i) {
	 cell->attrib_fetch_offsets[i] = (unsigned) ((void *) p->csr 
						     - (void *) p->store);

	 offset = 0;
	 emit_fetch(p, in_ptr, &offset, out_ptr, shuf_ptr,
		    draw->vertex_element[i].src_format);
	 spe_bi(p, 0, 0, 0);

	 /* Round up to the next 16-byte boundary.
	  */
	 if ((((unsigned) p->store) & 0x0f) != 0) {
	    const unsigned align = ((unsigned) p->store) & 0x0f;
	    p->store = (uint32_t *) (((void *) p->store) + align);
	 }
      } else {
	 /* Use the same function entry-point as a previously seen attribute
	  * with the same format.
	  */
	 cell->attrib_fetch_offsets[i] = 
	     cell->attrib_fetch_offsets[function_index[i]];
      }
   }
#else
   assert(0);
#endif
}
