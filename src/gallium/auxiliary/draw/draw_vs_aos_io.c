/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_exec.h"
#include "draw_vs.h"
#include "draw_vs_aos.h"
#include "draw_vertex.h"

#include "rtasm/rtasm_x86sse.h"

#ifdef PIPE_ARCH_X86

/* Note - don't yet have to worry about interacting with the code in
 * draw_vs_aos.c as there is no intermingling of generated code...
 * That may have to change, we'll see.
 */
static void emit_load_R32G32B32A32( struct aos_compilation *cp, 			   
				    struct x86_reg data,
				    struct x86_reg src_ptr )
{
   sse_movups(cp->func, data, src_ptr);
}

static void emit_load_R32G32B32( struct aos_compilation *cp, 			   
				 struct x86_reg data,
				 struct x86_reg src_ptr )
{
#if 1
   sse_movss(cp->func, data, x86_make_disp(src_ptr, 8));
   /* data = z ? ? ? */
   sse_shufps(cp->func, data, aos_get_internal_xmm( cp, IMM_IDENTITY ), SHUF(X,Y,Z,W) );
   /* data = z ? 0 1 */
   sse_shufps(cp->func, data, data, SHUF(Y,Z,X,W) );
   /* data = ? 0 z 1 */
   sse_movlps(cp->func, data, src_ptr);
   /* data = x y z 1 */
#else
   sse_movups(cp->func, data, src_ptr);
   /* data = x y z ? */
   sse2_pshufd(cp->func, data, data, SHUF(W,X,Y,Z) );
   /* data = ? x y z */
   sse_movss(cp->func, data, aos_get_internal_xmm( cp, IMM_ONES ) );
   /* data = 1 x y z */
   sse2_pshufd(cp->func, data, data, SHUF(Y,Z,W,X) );
   /* data = x y z 1 */
#endif
}

static void emit_load_R32G32( struct aos_compilation *cp, 
			   struct x86_reg data,
			   struct x86_reg src_ptr )
{
   sse_movups(cp->func, data, aos_get_internal_xmm( cp, IMM_IDENTITY ) );
   sse_movlps(cp->func, data, src_ptr);
}


static void emit_load_R32( struct aos_compilation *cp, 
			   struct x86_reg data,
			   struct x86_reg src_ptr )
{
   sse_movss(cp->func, data, src_ptr);
   sse_orps(cp->func, data, aos_get_internal_xmm( cp, IMM_IDENTITY ) );
}


static void emit_load_R8G8B8A8_UNORM( struct aos_compilation *cp,
				       struct x86_reg data,
				       struct x86_reg src_ptr )
{
   sse_movss(cp->func, data, src_ptr);
   sse2_punpcklbw(cp->func, data, aos_get_internal_xmm( cp, IMM_IDENTITY ));
   sse2_punpcklbw(cp->func, data, aos_get_internal_xmm( cp, IMM_IDENTITY ));
   sse2_cvtdq2ps(cp->func, data, data);
   sse_mulps(cp->func, data, aos_get_internal(cp, IMM_INV_255));
}



/* Extended swizzles?  Maybe later.
 */  
static void emit_swizzle( struct aos_compilation *cp,
			  struct x86_reg dest,
			  struct x86_reg src,
			  ubyte shuffle )
{
   sse_shufps(cp->func, dest, src, shuffle);
}



static boolean get_buffer_ptr( struct aos_compilation *cp,
                               boolean linear,
                               unsigned buf_idx,
                               struct x86_reg elt,
                               struct x86_reg ptr)
{
   struct x86_reg buf = x86_make_disp(aos_get_x86( cp, 0, X86_BUFFERS ), 
                                      buf_idx * sizeof(struct aos_buffer));

   struct x86_reg buf_stride = x86_make_disp(buf, 
                                             Offset(struct aos_buffer, stride));
   if (linear) {
      struct x86_reg buf_ptr = x86_make_disp(buf, 
                                             Offset(struct aos_buffer, ptr));


      /* Calculate pointer to current attrib:
       */
      x86_mov(cp->func, ptr, buf_ptr);
      x86_mov(cp->func, elt, buf_stride);
      x86_add(cp->func, elt, ptr);
      if (buf_idx == 0) sse_prefetchnta(cp->func, x86_make_disp(elt, 192));
      x86_mov(cp->func, buf_ptr, elt);
   }
   else {
      struct x86_reg buf_base_ptr = x86_make_disp(buf, 
                                                  Offset(struct aos_buffer, base_ptr));


      /* Calculate pointer to current attrib:
       */
      x86_mov(cp->func, ptr, buf_stride);
      x86_imul(cp->func, ptr, elt);
      x86_add(cp->func, ptr, buf_base_ptr);
   }

   cp->insn_counter++;

   return TRUE;
}


static boolean load_input( struct aos_compilation *cp,
                           unsigned idx,
                           struct x86_reg bufptr )
{
   unsigned format = cp->vaos->base.key.element[idx].in.format;
   unsigned offset = cp->vaos->base.key.element[idx].in.offset;
   struct x86_reg dataXMM = aos_get_xmm_reg(cp);

   /* Figure out source pointer address:
    */
   struct x86_reg src = x86_make_disp(bufptr, offset);

   aos_adopt_xmm_reg( cp,
                      dataXMM,
                      TGSI_FILE_INPUT,
                      idx,
                      TRUE );

   switch (format) {
   case PIPE_FORMAT_R32_FLOAT:
      emit_load_R32(cp, dataXMM, src);
      break;
   case PIPE_FORMAT_R32G32_FLOAT:
      emit_load_R32G32(cp, dataXMM, src);
      break;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      emit_load_R32G32B32(cp, dataXMM, src);
      break;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      emit_load_R32G32B32A32(cp, dataXMM, src);
      break;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      emit_load_R8G8B8A8_UNORM(cp, dataXMM, src);
      emit_swizzle(cp, dataXMM, dataXMM, SHUF(Z,Y,X,W));
      break;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      emit_load_R8G8B8A8_UNORM(cp, dataXMM, src);
      break;
   default:
      AOS_ERROR(cp, "unhandled input format");
      return FALSE;
   }

   return TRUE;
}

static boolean load_inputs( struct aos_compilation *cp,
                            unsigned buffer,
                            struct x86_reg ptr )
{
   unsigned i;

   for (i = 0; i < cp->vaos->base.key.nr_inputs; i++) {
      if (cp->vaos->base.key.element[i].in.buffer == buffer) {

         if (!load_input( cp, i, ptr ))
            return FALSE;

         cp->insn_counter++;
      }
   }
   
   return TRUE;
}

boolean aos_init_inputs( struct aos_compilation *cp, boolean linear )
{
   unsigned i;
   for (i = 0; i < cp->vaos->nr_vb; i++) {
      struct x86_reg buf = x86_make_disp(aos_get_x86( cp, 0, X86_BUFFERS ), 
                                         i * sizeof(struct aos_buffer));

      struct x86_reg buf_base_ptr = x86_make_disp(buf, 
                                                  Offset(struct aos_buffer, base_ptr));

      if (cp->vaos->base.key.const_vbuffers & (1<<i)) {
         struct x86_reg ptr = cp->tmp_EAX;

         x86_mov(cp->func, ptr, buf_base_ptr);

         /* Load all inputs for this constant vertex buffer
          */
         load_inputs( cp, i, x86_deref(ptr) );
         
         /* Then just force them out to aos_machine.input[]
          */
         aos_spill_all( cp );

      }
      else if (linear) {

         struct x86_reg elt = cp->idx_EBX;
         struct x86_reg ptr = cp->tmp_EAX;

         struct x86_reg buf_stride = x86_make_disp(buf, 
                                                   Offset(struct aos_buffer, stride));

         struct x86_reg buf_ptr = x86_make_disp(buf, 
                                                Offset(struct aos_buffer, ptr));


         /* Calculate pointer to current attrib:
          */
         x86_mov(cp->func, ptr, buf_stride);
         x86_imul(cp->func, ptr, elt);
         x86_add(cp->func, ptr, buf_base_ptr);


         /* In the linear case, keep the buffer pointer instead of the
          * index number.
          */
         if (cp->vaos->nr_vb == 1) 
            x86_mov( cp->func, elt, ptr );
         else
            x86_mov( cp->func, buf_ptr, ptr );

         cp->insn_counter++;
      }
   }

   return TRUE;
}

boolean aos_fetch_inputs( struct aos_compilation *cp, boolean linear )
{
   unsigned j;

   for (j = 0; j < cp->vaos->nr_vb; j++) {
      if (cp->vaos->base.key.const_vbuffers & (1<<j)) {
         /* just retreive pre-transformed input */
      }
      else if (linear && cp->vaos->nr_vb == 1) {
         load_inputs( cp, 0, cp->idx_EBX );
      }
      else {
         struct x86_reg elt = linear ? cp->idx_EBX : x86_deref(cp->idx_EBX);
         struct x86_reg ptr = cp->tmp_EAX;

         if (!get_buffer_ptr( cp, linear, j, elt, ptr ))
            return FALSE;

         if (!load_inputs( cp, j, ptr ))
            return FALSE;
      }
   }

   return TRUE;
}

boolean aos_incr_inputs( struct aos_compilation *cp, boolean linear )
{
   if (linear && cp->vaos->nr_vb == 1) {
      struct x86_reg stride = x86_make_disp(aos_get_x86( cp, 0, X86_BUFFERS ), 
                                            (0 * sizeof(struct aos_buffer) + 
                                             Offset(struct aos_buffer, stride)));

      x86_add(cp->func, cp->idx_EBX, stride);
      sse_prefetchnta(cp->func, x86_make_disp(cp->idx_EBX, 192));
   }
   else if (linear) {
      /* Nothing to do */
   } 
   else {
      x86_lea(cp->func, cp->idx_EBX, x86_make_disp(cp->idx_EBX, 4));
   }

   return TRUE;
}






static void emit_store_R32G32B32A32( struct aos_compilation *cp, 			   
				     struct x86_reg dst_ptr,
				     struct x86_reg dataXMM )
{
   sse_movups(cp->func, dst_ptr, dataXMM);
}

static void emit_store_R32G32B32( struct aos_compilation *cp, 
				  struct x86_reg dst_ptr,
				  struct x86_reg dataXMM )
{
   sse_movlps(cp->func, dst_ptr, dataXMM);
   sse_shufps(cp->func, dataXMM, dataXMM, SHUF(Z,Z,Z,Z) ); /* NOTE! destructive */
   sse_movss(cp->func, x86_make_disp(dst_ptr,8), dataXMM);
}

static void emit_store_R32G32( struct aos_compilation *cp, 
			       struct x86_reg dst_ptr,
			       struct x86_reg dataXMM )
{
   sse_movlps(cp->func, dst_ptr, dataXMM);
}

static void emit_store_R32( struct aos_compilation *cp, 
			    struct x86_reg dst_ptr,
			    struct x86_reg dataXMM )
{
   sse_movss(cp->func, dst_ptr, dataXMM);
}



static void emit_store_R8G8B8A8_UNORM( struct aos_compilation *cp,
				       struct x86_reg dst_ptr,
				       struct x86_reg dataXMM )
{
   sse_mulps(cp->func, dataXMM, aos_get_internal(cp, IMM_255));
   sse2_cvtps2dq(cp->func, dataXMM, dataXMM);
   sse2_packssdw(cp->func, dataXMM, dataXMM);
   sse2_packuswb(cp->func, dataXMM, dataXMM);
   sse_movss(cp->func, dst_ptr, dataXMM);
}





static boolean emit_output( struct aos_compilation *cp,
                            struct x86_reg ptr,
                            struct x86_reg dataXMM, 
                            unsigned format )
{
   switch (format) {
   case EMIT_1F:
   case EMIT_1F_PSIZE:
      emit_store_R32(cp, ptr, dataXMM);
      break;
   case EMIT_2F:
      emit_store_R32G32(cp, ptr, dataXMM);
      break;
   case EMIT_3F:
      emit_store_R32G32B32(cp, ptr, dataXMM);
      break;
   case EMIT_4F:
      emit_store_R32G32B32A32(cp, ptr, dataXMM);
      break;
   case EMIT_4UB:
      if (1) {
         emit_swizzle(cp, dataXMM, dataXMM, SHUF(Z,Y,X,W));
         emit_store_R8G8B8A8_UNORM(cp, ptr, dataXMM);
      }
      else {
         emit_store_R8G8B8A8_UNORM(cp, ptr, dataXMM);
      }
      break;
   default:
      AOS_ERROR(cp, "unhandled output format");
      return FALSE;
   }

   return TRUE;
}



boolean aos_emit_outputs( struct aos_compilation *cp )
{
   unsigned i;
   
   for (i = 0; i < cp->vaos->base.key.nr_outputs; i++) {
      unsigned format = cp->vaos->base.key.element[i].out.format;
      unsigned offset = cp->vaos->base.key.element[i].out.offset;
      unsigned vs_output = cp->vaos->base.key.element[i].out.vs_output;

      struct x86_reg data;

      if (format == EMIT_1F_PSIZE) {
         data = aos_get_internal_xmm( cp, IMM_PSIZE );
      }
      else {
         data = aos_get_shader_reg( cp, 
                                    TGSI_FILE_OUTPUT,
                                    vs_output );
      }

      if (data.file != file_XMM) {
         struct x86_reg tmp = aos_get_xmm_reg( cp );
         sse_movaps(cp->func, tmp, data);
         data = tmp;
      }
      
      if (!emit_output( cp, 
                        x86_make_disp( cp->outbuf_ECX, offset ),
                        data, 
                        format ))
         return FALSE;

      aos_release_xmm_reg( cp, data.idx );

      cp->insn_counter++;
   }

   return TRUE;
}

#endif
