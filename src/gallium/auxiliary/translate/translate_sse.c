/*
 * Copyright 2003 Tungsten Graphics, inc.
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
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */


#include "pipe/p_config.h"
#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "translate.h"


#if defined(PIPE_ARCH_X86)

#include "rtasm/rtasm_cpu.h"
#include "rtasm/rtasm_x86sse.h"


#define X    0
#define Y    1
#define Z    2
#define W    3


typedef void (PIPE_CDECL *run_func)( struct translate *translate,
                                     unsigned start,
                                     unsigned count,
                                     void *output_buffer );

typedef void (PIPE_CDECL *run_elts_func)( struct translate *translate,
                                          const unsigned *elts,
                                          unsigned count,
                                          void *output_buffer );



struct translate_sse {
   struct translate translate;

   struct x86_function linear_func;
   struct x86_function elt_func;
   struct x86_function *func;

   boolean loaded_identity;
   boolean loaded_255;
   boolean loaded_inv_255;

   float identity[4];
   float float_255[4];
   float inv_255[4];

   struct {
      char *input_ptr;
      unsigned input_stride;
   } attrib[PIPE_MAX_ATTRIBS];

   run_func      gen_run;
   run_elts_func gen_run_elts;

};

static int get_offset( const void *a, const void *b )
{
   return (const char *)b - (const char *)a;
}



static struct x86_reg get_identity( struct translate_sse *p )
{
   struct x86_reg reg = x86_make_reg(file_XMM, 6);

   if (!p->loaded_identity) {
      /* Nasty: 
       */
      struct x86_reg translateESI = x86_make_reg(file_REG32, reg_SI);

      p->loaded_identity = TRUE;
      p->identity[0] = 0;
      p->identity[1] = 0;
      p->identity[2] = 0;
      p->identity[3] = 1;

      sse_movups(p->func, reg, 
		 x86_make_disp(translateESI, 
			       get_offset(p, &p->identity[0])));
   }

   return reg;
}

static struct x86_reg get_255( struct translate_sse *p )
{
   struct x86_reg reg = x86_make_reg(file_XMM, 6);

   if (!p->loaded_255) {
      struct x86_reg translateESI = x86_make_reg(file_REG32, reg_SI);

      p->loaded_255 = TRUE;
      p->float_255[0] =
	 p->float_255[1] =
	 p->float_255[2] =
	 p->float_255[3] = 255.0f;

      sse_movups(p->func, reg, 
		 x86_make_disp(translateESI, 
			       get_offset(p, &p->float_255[0])));
   }

   return reg;
   return x86_make_reg(file_XMM, 7);
}

static struct x86_reg get_inv_255( struct translate_sse *p )
{
   struct x86_reg reg = x86_make_reg(file_XMM, 5);

   if (!p->loaded_inv_255) {
      struct x86_reg translateESI = x86_make_reg(file_REG32, reg_SI);

      p->loaded_inv_255 = TRUE;
      p->inv_255[0] =
	 p->inv_255[1] =
	 p->inv_255[2] =
	 p->inv_255[3] = 1.0f / 255.0f;

      sse_movups(p->func, reg, 
		 x86_make_disp(translateESI, 
			       get_offset(p, &p->inv_255[0])));
   }

   return reg;
}


static void emit_load_R32G32B32A32( struct translate_sse *p, 			   
				    struct x86_reg data,
				    struct x86_reg arg0 )
{
   sse_movups(p->func, data, arg0);
}

static void emit_load_R32G32B32( struct translate_sse *p, 			   
				 struct x86_reg data,
				 struct x86_reg arg0 )
{
   /* Have to jump through some hoops:
    *
    * c 0 0 0
    * c 0 0 1
    * 0 0 c 1
    * a b c 1
    */
   sse_movss(p->func, data, x86_make_disp(arg0, 8));
   sse_shufps(p->func, data, get_identity(p), SHUF(X,Y,Z,W) );
   sse_shufps(p->func, data, data, SHUF(Y,Z,X,W) );
   sse_movlps(p->func, data, arg0);
}

static void emit_load_R32G32( struct translate_sse *p, 
			   struct x86_reg data,
			   struct x86_reg arg0 )
{
   /* 0 0 0 1
    * a b 0 1
    */
   sse_movups(p->func, data, get_identity(p) );
   sse_movlps(p->func, data, arg0);
}


static void emit_load_R32( struct translate_sse *p, 
			   struct x86_reg data,
			   struct x86_reg arg0 )
{
   /* a 0 0 0
    * a 0 0 1
    */
   sse_movss(p->func, data, arg0);
   sse_orps(p->func, data, get_identity(p) );
}


static void emit_load_R8G8B8A8_UNORM( struct translate_sse *p,
				       struct x86_reg data,
				       struct x86_reg src )
{

   /* Load and unpack twice:
    */
   sse_movss(p->func, data, src);
   sse2_punpcklbw(p->func, data, get_identity(p));
   sse2_punpcklbw(p->func, data, get_identity(p));

   /* Convert to float:
    */
   sse2_cvtdq2ps(p->func, data, data);


   /* Scale by 1/255.0
    */
   sse_mulps(p->func, data, get_inv_255(p));
}




static void emit_store_R32G32B32A32( struct translate_sse *p, 			   
				     struct x86_reg dest,
				     struct x86_reg dataXMM )
{
   sse_movups(p->func, dest, dataXMM);
}

static void emit_store_R32G32B32( struct translate_sse *p, 
				  struct x86_reg dest,
				  struct x86_reg dataXMM )
{
   /* Emit two, shuffle, emit one.
    */
   sse_movlps(p->func, dest, dataXMM);
   sse_shufps(p->func, dataXMM, dataXMM, SHUF(Z,Z,Z,Z) ); /* NOTE! destructive */
   sse_movss(p->func, x86_make_disp(dest,8), dataXMM);
}

static void emit_store_R32G32( struct translate_sse *p, 
			       struct x86_reg dest,
			       struct x86_reg dataXMM )
{
   sse_movlps(p->func, dest, dataXMM);
}

static void emit_store_R32( struct translate_sse *p, 
			    struct x86_reg dest,
			    struct x86_reg dataXMM )
{
   sse_movss(p->func, dest, dataXMM);
}



static void emit_store_R8G8B8A8_UNORM( struct translate_sse *p,
				       struct x86_reg dest,
				       struct x86_reg dataXMM )
{
   /* Scale by 255.0
    */
   sse_mulps(p->func, dataXMM, get_255(p));

   /* Pack and emit:
    */
   sse2_cvtps2dq(p->func, dataXMM, dataXMM);
   sse2_packssdw(p->func, dataXMM, dataXMM);
   sse2_packuswb(p->func, dataXMM, dataXMM);
   sse_movss(p->func, dest, dataXMM);
}





static void get_src_ptr( struct translate_sse *p,
			 struct x86_reg srcEAX,
			 struct x86_reg translateREG,
			 struct x86_reg eltREG,	
			 unsigned a )
{
   struct x86_reg input_ptr = 
      x86_make_disp(translateREG, 
		    get_offset(p, &p->attrib[a].input_ptr));

   struct x86_reg input_stride = 
      x86_make_disp(translateREG, 
		    get_offset(p, &p->attrib[a].input_stride));

   /* Calculate pointer to current attrib:
    */
   x86_mov(p->func, srcEAX, input_stride);
   x86_imul(p->func, srcEAX, eltREG);	
   x86_add(p->func, srcEAX, input_ptr);
}


/* Extended swizzles?  Maybe later.
 */  
static void emit_swizzle( struct translate_sse *p,
			  struct x86_reg dest,
			  struct x86_reg src,
			  unsigned char shuffle )
{
   sse_shufps(p->func, dest, src, shuffle);
}


static boolean translate_attr( struct translate_sse *p,
			       const struct translate_element *a,
			       struct x86_reg srcECX,
			       struct x86_reg dstEAX)
{
   struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);

   switch (a->input_format) {
   case PIPE_FORMAT_R32_FLOAT:
      emit_load_R32(p, dataXMM, srcECX);
      break;
   case PIPE_FORMAT_R32G32_FLOAT:
      emit_load_R32G32(p, dataXMM, srcECX);
      break;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      emit_load_R32G32B32(p, dataXMM, srcECX);
      break;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      emit_load_R32G32B32A32(p, dataXMM, srcECX);
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      emit_load_R8G8B8A8_UNORM(p, dataXMM, srcECX);
      emit_swizzle(p, dataXMM, dataXMM, SHUF(Z,Y,X,W));
      break;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      emit_load_R8G8B8A8_UNORM(p, dataXMM, srcECX);
      break;
   default:
      return FALSE;
   }

   switch (a->output_format) {
   case PIPE_FORMAT_R32_FLOAT:
      emit_store_R32(p, dstEAX, dataXMM);
      break;
   case PIPE_FORMAT_R32G32_FLOAT:
      emit_store_R32G32(p, dstEAX, dataXMM);
      break;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      emit_store_R32G32B32(p, dstEAX, dataXMM);
      break;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      emit_store_R32G32B32A32(p, dstEAX, dataXMM);
      break;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      emit_swizzle(p, dataXMM, dataXMM, SHUF(Z,Y,X,W));
      emit_store_R8G8B8A8_UNORM(p, dstEAX, dataXMM);
      break;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      emit_store_R8G8B8A8_UNORM(p, dstEAX, dataXMM);
      break;
   default:
      return FALSE;
   }

   return TRUE;
}

/* Build run( struct translate *translate,
 *            unsigned start,
 *            unsigned count,
 *            void *output_buffer )
 * or
 *  run_elts( struct translate *translate,
 *            unsigned *elts,
 *            unsigned count,
 *            void *output_buffer )
 *
 *  Lots of hardcoding
 *
 * EAX -- pointer to current output vertex
 * ECX -- pointer to current attribute 
 * 
 */
static boolean build_vertex_emit( struct translate_sse *p,
				  struct x86_function *func,
				  boolean linear )
{
   struct x86_reg vertexECX    = x86_make_reg(file_REG32, reg_AX);
   struct x86_reg idxEBX       = x86_make_reg(file_REG32, reg_BX);
   struct x86_reg srcEAX       = x86_make_reg(file_REG32, reg_CX);
   struct x86_reg countEBP     = x86_make_reg(file_REG32, reg_BP);
   struct x86_reg translateESI = x86_make_reg(file_REG32, reg_SI);
   int fixup, label;
   unsigned j;

   p->func = func;
   p->loaded_inv_255 = FALSE;
   p->loaded_255 = FALSE;
   p->loaded_identity = FALSE;

   x86_init_func(p->func);

   /* Push a few regs?
    */
   x86_push(p->func, countEBP);
   x86_push(p->func, translateESI);
   x86_push(p->func, idxEBX);

   /* Get vertex count, compare to zero
    */
   x86_xor(p->func, idxEBX, idxEBX);
   x86_mov(p->func, countEBP, x86_fn_arg(p->func, 3));
   x86_cmp(p->func, countEBP, idxEBX);
   fixup = x86_jcc_forward(p->func, cc_E);

   /* If linear, idx is the current element, otherwise it is a pointer
    * to the current element.
    */
   x86_mov(p->func, idxEBX, x86_fn_arg(p->func, 2));

   /* Initialize destination register. 
    */
   x86_mov(p->func, vertexECX, x86_fn_arg(p->func, 4));

   /* Move argument 1 (translate_sse pointer) into a reg:
    */
   x86_mov(p->func, translateESI, x86_fn_arg(p->func, 1));

   
   /* always load, needed or not:
    */

   /* Note address for loop jump */
   label = x86_get_label(p->func);


   for (j = 0; j < p->translate.key.nr_elements; j++) {
      const struct translate_element *a = &p->translate.key.element[j];

      struct x86_reg destEAX = x86_make_disp(vertexECX, 
					     a->output_offset);

      /* Figure out source pointer address:
       */
      if (linear) {
	 get_src_ptr(p, srcEAX, translateESI, idxEBX, j);
      }
      else {
	 get_src_ptr(p, srcEAX, translateESI, x86_deref(idxEBX), j);
      }

      if (!translate_attr( p, a, x86_deref(srcEAX), destEAX ))
	 return FALSE;
   }

   /* Next vertex:
    */
   x86_lea(p->func, vertexECX, x86_make_disp(vertexECX, p->translate.key.output_stride));

   /* Incr index
    */ 
   if (linear) {
      x86_inc(p->func, idxEBX);
   } 
   else {
      x86_lea(p->func, idxEBX, x86_make_disp(idxEBX, 4));
   }

   /* decr count, loop if not zero
    */
   x86_dec(p->func, countEBP);
   x86_test(p->func, countEBP, countEBP); 
   x86_jcc(p->func, cc_NZ, label);

   /* Exit mmx state?
    */
   if (p->func->need_emms)
      mmx_emms(p->func);

   /* Land forward jump here:
    */
   x86_fixup_fwd_jump(p->func, fixup);

   /* Pop regs and return
    */
   
   x86_pop(p->func, idxEBX);
   x86_pop(p->func, translateESI);
   x86_pop(p->func, countEBP);
   x86_ret(p->func);

   return TRUE;
}






			       
static void translate_sse_set_buffer( struct translate *translate,
				unsigned buf,
				const void *ptr,
				unsigned stride )
{
   struct translate_sse *p = (struct translate_sse *)translate;
   unsigned i;

   for (i = 0; i < p->translate.key.nr_elements; i++) {
      if (p->translate.key.element[i].input_buffer == buf) {
	 p->attrib[i].input_ptr = ((char *)ptr +
				    p->translate.key.element[i].input_offset);
	 p->attrib[i].input_stride = stride;
      }
   }
}


static void translate_sse_release( struct translate *translate )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   x86_release_func( &p->linear_func );
   x86_release_func( &p->elt_func );

   FREE(p);
}

static void PIPE_CDECL translate_sse_run_elts( struct translate *translate,
			      const unsigned *elts,
			      unsigned count,
			      void *output_buffer )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   p->gen_run_elts( translate,
		    elts,
		    count,
		    output_buffer );
}

static void PIPE_CDECL translate_sse_run( struct translate *translate,
			 unsigned start,
			 unsigned count,
			 void *output_buffer )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   p->gen_run( translate,
	       start,
	       count,
	       output_buffer );
}


struct translate *translate_sse2_create( const struct translate_key *key )
{
   struct translate_sse *p = NULL;

   if (!rtasm_cpu_has_sse() || !rtasm_cpu_has_sse2())
      goto fail;

   p = CALLOC_STRUCT( translate_sse );
   if (p == NULL) 
      goto fail;

   p->translate.key = *key;
   p->translate.release = translate_sse_release;
   p->translate.set_buffer = translate_sse_set_buffer;
   p->translate.run_elts = translate_sse_run_elts;
   p->translate.run = translate_sse_run;

   if (!build_vertex_emit(p, &p->linear_func, TRUE))
      goto fail;

   if (!build_vertex_emit(p, &p->elt_func, FALSE))
      goto fail;

   p->gen_run = (run_func)x86_get_func(&p->linear_func);
   if (p->gen_run == NULL)
      goto fail;

   p->gen_run_elts = (run_elts_func)x86_get_func(&p->elt_func);
   if (p->gen_run_elts == NULL)
      goto fail;

   return &p->translate;

 fail:
   if (p)
      translate_sse_release( &p->translate );

   return NULL;
}



#else

struct translate *translate_sse2_create( const struct translate_key *key )
{
   return NULL;
}

#endif
