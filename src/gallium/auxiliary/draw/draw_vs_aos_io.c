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
   sse_movss(cp->func, data, x86_make_disp(src_ptr, 8));
   /* data = z ? ? ? */
   sse_shufps(cp->func, data, aos_get_internal_xmm( cp, IMM_IDENTITY ), SHUF(X,Y,Z,W) );
   /* data = z ? 0 1 */
   sse_shufps(cp->func, data, data, SHUF(Y,Z,X,W) );
   /* data = ? 0 z 1 */
   sse_movlps(cp->func, data, src_ptr);
   /* data = x y z 1 */
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



static void get_src_ptr( struct aos_compilation *cp,
                         struct x86_reg src,
                         struct x86_reg elt,
                         unsigned a )
{
   struct x86_reg attrib = x86_make_disp(aos_get_x86( cp, 0, X86_ATTRIBS ), 
                                         a * sizeof(struct aos_attrib));

   struct x86_reg input_ptr = x86_make_disp(attrib, 
                                            Offset(struct aos_attrib, input_ptr));

   struct x86_reg input_stride = x86_make_disp(attrib, 
                                               Offset(struct aos_attrib, input_stride));

   /* Calculate pointer to current attrib:
    */
   x86_mov(cp->func, src, input_stride);
   x86_imul(cp->func, src, elt);
   x86_add(cp->func, src, input_ptr);
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


static boolean load_input( struct aos_compilation *cp,
                           unsigned idx,
                           boolean linear )
{
   unsigned format = cp->vaos->base.key.element[idx].in.format;
   struct x86_reg src = cp->tmp_EAX;
   struct x86_reg dataXMM = aos_get_xmm_reg(cp);

   /* Figure out source pointer address:
    */
   get_src_ptr(cp, 
               src, 
               linear ? cp->idx_EBX : x86_deref(cp->idx_EBX),
               idx);

   src = x86_deref(src);

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
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      emit_load_R8G8B8A8_UNORM(cp, dataXMM, src);
      emit_swizzle(cp, dataXMM, dataXMM, SHUF(Z,Y,X,W));
      break;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      emit_load_R8G8B8A8_UNORM(cp, dataXMM, src);
      break;
   default:
      ERROR(cp, "unhandled input format");
      return FALSE;
   }

   return TRUE;
}


boolean aos_fetch_inputs( struct aos_compilation *cp, boolean linear )
{
   unsigned i;
   
   for (i = 0; i < cp->vaos->base.key.nr_inputs; i++) {
      if (!load_input( cp, i, linear ))
         return FALSE;
      cp->insn_counter++;
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
      ERROR(cp, "unhandled output format");
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
         sse_movups(cp->func, tmp, data);
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
