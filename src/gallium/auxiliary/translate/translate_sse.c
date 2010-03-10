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
#include "util/u_math.h"

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
                                     unsigned instance_id,
                                     void *output_buffer);

typedef void (PIPE_CDECL *run_elts_func)( struct translate *translate,
                                          const unsigned *elts,
                                          unsigned count,
                                          unsigned instance_id,
                                          void *output_buffer);

struct translate_buffer {
   const void *base_ptr;
   unsigned stride;
};

struct translate_buffer_varient {
   unsigned buffer_index;
   unsigned instance_divisor;
   void *ptr;                    /* updated either per vertex or per instance */
};


#define ELEMENT_BUFFER_INSTANCE_ID  1001


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

   struct translate_buffer buffer[PIPE_MAX_ATTRIBS];
   unsigned nr_buffers;

   /* Multiple buffer varients can map to a single buffer. */
   struct translate_buffer_varient buffer_varient[PIPE_MAX_ATTRIBS];
   unsigned nr_buffer_varients;

   /* Multiple elements can map to a single buffer varient. */
   unsigned element_to_buffer_varient[PIPE_MAX_ATTRIBS];

   boolean use_instancing;
   unsigned instance_id;

   run_func      gen_run;
   run_elts_func gen_run_elts;

   /* these are actually known values, but putting them in a struct
    * like this is helpful to keep them in sync across the file.
    */
   struct x86_reg tmp_EAX;
   struct x86_reg idx_EBX;     /* either start+i or &elt[i] */
   struct x86_reg outbuf_ECX;
   struct x86_reg machine_EDX;
   struct x86_reg count_ESI;    /* decrements to zero */
};

static int get_offset( const void *a, const void *b )
{
   return (const char *)b - (const char *)a;
}



static struct x86_reg get_identity( struct translate_sse *p )
{
   struct x86_reg reg = x86_make_reg(file_XMM, 6);

   if (!p->loaded_identity) {
      p->loaded_identity = TRUE;
      p->identity[0] = 0;
      p->identity[1] = 0;
      p->identity[2] = 0;
      p->identity[3] = 1;

      sse_movups(p->func, reg, 
		 x86_make_disp(p->machine_EDX, 
			       get_offset(p, &p->identity[0])));
   }

   return reg;
}

static struct x86_reg get_255( struct translate_sse *p )
{
   struct x86_reg reg = x86_make_reg(file_XMM, 7);

   if (!p->loaded_255) {
      p->loaded_255 = TRUE;
      p->float_255[0] =
	 p->float_255[1] =
	 p->float_255[2] =
	 p->float_255[3] = 255.0f;

      sse_movups(p->func, reg, 
		 x86_make_disp(p->machine_EDX, 
			       get_offset(p, &p->float_255[0])));
   }

   return reg;
}

static struct x86_reg get_inv_255( struct translate_sse *p )
{
   struct x86_reg reg = x86_make_reg(file_XMM, 5);

   if (!p->loaded_inv_255) {
      p->loaded_inv_255 = TRUE;
      p->inv_255[0] =
	 p->inv_255[1] =
	 p->inv_255[2] =
	 p->inv_255[3] = 1.0f / 255.0f;

      sse_movups(p->func, reg, 
		 x86_make_disp(p->machine_EDX, 
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
   case PIPE_FORMAT_A8R8G8B8_UNORM:
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
   case PIPE_FORMAT_A8R8G8B8_UNORM:
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


static boolean init_inputs( struct translate_sse *p,
                            boolean linear )
{
   unsigned i;
   struct x86_reg instance_id = x86_make_disp(p->machine_EDX,
                                              get_offset(p, &p->instance_id));

   for (i = 0; i < p->nr_buffer_varients; i++) {
      struct translate_buffer_varient *varient = &p->buffer_varient[i];
      struct translate_buffer *buffer = &p->buffer[varient->buffer_index];

      if (linear || varient->instance_divisor) {
         struct x86_reg buf_stride   = x86_make_disp(p->machine_EDX,
                                                     get_offset(p, &buffer->stride));
         struct x86_reg buf_ptr      = x86_make_disp(p->machine_EDX,
                                                     get_offset(p, &varient->ptr));
         struct x86_reg buf_base_ptr = x86_make_disp(p->machine_EDX,
                                                     get_offset(p, &buffer->base_ptr));
         struct x86_reg elt = p->idx_EBX;
         struct x86_reg tmp_EAX = p->tmp_EAX;

         /* Calculate pointer to first attrib:
          *   base_ptr + stride * index, where index depends on instance divisor
          */
         if (varient->instance_divisor) {
            /* Our index is instance ID divided by instance divisor.
             */
            x86_mov(p->func, tmp_EAX, instance_id);

            if (varient->instance_divisor != 1) {
               struct x86_reg tmp_EDX = p->machine_EDX;
               struct x86_reg tmp_ECX = p->outbuf_ECX;

               /* TODO: Add x86_shr() to rtasm and use it whenever
                *       instance divisor is power of two.
                */

               x86_push(p->func, tmp_EDX);
               x86_push(p->func, tmp_ECX);
               x86_xor(p->func, tmp_EDX, tmp_EDX);
               x86_mov_reg_imm(p->func, tmp_ECX, varient->instance_divisor);
               x86_div(p->func, tmp_ECX);    /* EAX = EDX:EAX / ECX */
               x86_pop(p->func, tmp_ECX);
               x86_pop(p->func, tmp_EDX);
            }
         } else {
            x86_mov(p->func, tmp_EAX, elt);
         }
         x86_imul(p->func, tmp_EAX, buf_stride);
         x86_add(p->func, tmp_EAX, buf_base_ptr);


         /* In the linear case, keep the buffer pointer instead of the
          * index number.
          */
         if (linear && p->nr_buffer_varients == 1)
            x86_mov(p->func, elt, tmp_EAX);
         else
            x86_mov(p->func, buf_ptr, tmp_EAX);
      }
   }

   return TRUE;
}


static struct x86_reg get_buffer_ptr( struct translate_sse *p,
                                      boolean linear,
                                      unsigned var_idx,
                                      struct x86_reg elt )
{
   if (var_idx == ELEMENT_BUFFER_INSTANCE_ID) {
      return x86_make_disp(p->machine_EDX,
                           get_offset(p, &p->instance_id));
   }
   if (linear && p->nr_buffer_varients == 1) {
      return p->idx_EBX;
   }
   else if (linear || p->buffer_varient[var_idx].instance_divisor) {
      struct x86_reg ptr = p->tmp_EAX;
      struct x86_reg buf_ptr = 
         x86_make_disp(p->machine_EDX, 
                       get_offset(p, &p->buffer_varient[var_idx].ptr));
      
      x86_mov(p->func, ptr, buf_ptr);
      return ptr;
   }
   else {
      struct x86_reg ptr = p->tmp_EAX;
      const struct translate_buffer_varient *varient = &p->buffer_varient[var_idx];

      struct x86_reg buf_stride = 
         x86_make_disp(p->machine_EDX, 
                       get_offset(p, &p->buffer[varient->buffer_index].stride));

      struct x86_reg buf_base_ptr = 
         x86_make_disp(p->machine_EDX, 
                       get_offset(p, &p->buffer[varient->buffer_index].base_ptr));



      /* Calculate pointer to current attrib:
       */
      x86_mov(p->func, ptr, buf_stride);
      x86_imul(p->func, ptr, elt);
      x86_add(p->func, ptr, buf_base_ptr);
      return ptr;
   }
}



static boolean incr_inputs( struct translate_sse *p, 
                            boolean linear )
{
   if (linear && p->nr_buffer_varients == 1) {
      struct x86_reg stride = x86_make_disp(p->machine_EDX,
                                            get_offset(p, &p->buffer[0].stride));

      if (p->buffer_varient[0].instance_divisor == 0) {
         x86_add(p->func, p->idx_EBX, stride);
         sse_prefetchnta(p->func, x86_make_disp(p->idx_EBX, 192));
      }
   }
   else if (linear) {
      unsigned i;

      /* Is this worthwhile??
       */
      for (i = 0; i < p->nr_buffer_varients; i++) {
         struct translate_buffer_varient *varient = &p->buffer_varient[i];
         struct x86_reg buf_ptr = x86_make_disp(p->machine_EDX,
                                                get_offset(p, &varient->ptr));
         struct x86_reg buf_stride = x86_make_disp(p->machine_EDX,
                                                   get_offset(p, &p->buffer[varient->buffer_index].stride));

         if (varient->instance_divisor == 0) {
            x86_mov(p->func, p->tmp_EAX, buf_ptr);
            x86_add(p->func, p->tmp_EAX, buf_stride);
            if (i == 0) sse_prefetchnta(p->func, x86_make_disp(p->tmp_EAX, 192));
            x86_mov(p->func, buf_ptr, p->tmp_EAX);
         }
      }
   } 
   else {
      x86_lea(p->func, p->idx_EBX, x86_make_disp(p->idx_EBX, 4));
   }
   
   return TRUE;
}


/* Build run( struct translate *machine,
 *            unsigned start,
 *            unsigned count,
 *            void *output_buffer )
 * or
 *  run_elts( struct translate *machine,
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
   int fixup, label;
   unsigned j;

   p->tmp_EAX       = x86_make_reg(file_REG32, reg_AX);
   p->idx_EBX       = x86_make_reg(file_REG32, reg_BX);
   p->outbuf_ECX    = x86_make_reg(file_REG32, reg_CX);
   p->machine_EDX   = x86_make_reg(file_REG32, reg_DX);
   p->count_ESI     = x86_make_reg(file_REG32, reg_SI);

   p->func = func;
   p->loaded_inv_255 = FALSE;
   p->loaded_255 = FALSE;
   p->loaded_identity = FALSE;

   x86_init_func(p->func);

   /* Push a few regs?
    */
   x86_push(p->func, p->idx_EBX);
   x86_push(p->func, p->count_ESI);

   /* Load arguments into regs:
    */
   x86_mov(p->func, p->machine_EDX, x86_fn_arg(p->func, 1));
   x86_mov(p->func, p->idx_EBX, x86_fn_arg(p->func, 2));
   x86_mov(p->func, p->count_ESI, x86_fn_arg(p->func, 3));
   x86_mov(p->func, p->outbuf_ECX, x86_fn_arg(p->func, 5));

   /* Load instance ID.
    */
   if (p->use_instancing) {
      x86_mov(p->func,
              p->tmp_EAX,
              x86_fn_arg(p->func, 4));
      x86_mov(p->func,
              x86_make_disp(p->machine_EDX, get_offset(p, &p->instance_id)),
              p->tmp_EAX);
   }

   /* Get vertex count, compare to zero
    */
   x86_xor(p->func, p->tmp_EAX, p->tmp_EAX);
   x86_cmp(p->func, p->count_ESI, p->tmp_EAX);
   fixup = x86_jcc_forward(p->func, cc_E);

   /* always load, needed or not:
    */
   init_inputs(p, linear);

   /* Note address for loop jump
    */
   label = x86_get_label(p->func);
   {
      struct x86_reg elt = linear ? p->idx_EBX : x86_deref(p->idx_EBX);
      int last_varient = -1;
      struct x86_reg vb;

      for (j = 0; j < p->translate.key.nr_elements; j++) {
         const struct translate_element *a = &p->translate.key.element[j];
         unsigned varient = p->element_to_buffer_varient[j];

         /* Figure out source pointer address:
          */
         if (varient != last_varient) {
            last_varient = varient;
            vb = get_buffer_ptr(p, linear, varient, elt);
         }
         
         if (!translate_attr( p, a, 
                              x86_make_disp(vb, a->input_offset), 
                              x86_make_disp(p->outbuf_ECX, a->output_offset)))
            return FALSE;
      }

      /* Next output vertex:
       */
      x86_lea(p->func, 
              p->outbuf_ECX, 
              x86_make_disp(p->outbuf_ECX, 
                            p->translate.key.output_stride));

      /* Incr index
       */ 
      incr_inputs( p, linear );
   }

   /* decr count, loop if not zero
    */
   x86_dec(p->func, p->count_ESI);
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
   
   x86_pop(p->func, p->count_ESI);
   x86_pop(p->func, p->idx_EBX);
   x86_ret(p->func);

   return TRUE;
}






			       
static void translate_sse_set_buffer( struct translate *translate,
				unsigned buf,
				const void *ptr,
				unsigned stride )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   if (buf < p->nr_buffers) {
      p->buffer[buf].base_ptr = (char *)ptr;
      p->buffer[buf].stride = stride;
   }

   if (0) debug_printf("%s %d/%d: %p %d\n", 
                       __FUNCTION__, buf, 
                       p->nr_buffers, 
                       ptr, stride);
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
                              unsigned instance_id,
			      void *output_buffer )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   p->gen_run_elts( translate,
		    elts,
		    count,
                    instance_id,
                    output_buffer);
}

static void PIPE_CDECL translate_sse_run( struct translate *translate,
			 unsigned start,
			 unsigned count,
                         unsigned instance_id,
			 void *output_buffer )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   p->gen_run( translate,
	       start,
	       count,
               instance_id,
               output_buffer);
}


struct translate *translate_sse2_create( const struct translate_key *key )
{
   struct translate_sse *p = NULL;
   unsigned i;

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

   for (i = 0; i < key->nr_elements; i++) {
      if (key->element[i].type == TRANSLATE_ELEMENT_NORMAL) {
         unsigned j;

         p->nr_buffers = MAX2(p->nr_buffers, key->element[i].input_buffer + 1);

         if (key->element[i].instance_divisor) {
            p->use_instancing = TRUE;
         }

         /*
          * Map vertex element to vertex buffer varient.
          */
         for (j = 0; j < p->nr_buffer_varients; j++) {
            if (p->buffer_varient[j].buffer_index == key->element[i].input_buffer &&
                p->buffer_varient[j].instance_divisor == key->element[i].instance_divisor) {
               break;
            }
         }
         if (j == p->nr_buffer_varients) {
            p->buffer_varient[j].buffer_index = key->element[i].input_buffer;
            p->buffer_varient[j].instance_divisor = key->element[i].instance_divisor;
            p->nr_buffer_varients++;
         }
         p->element_to_buffer_varient[i] = j;
      } else {
         assert(key->element[i].type == TRANSLATE_ELEMENT_INSTANCE_ID);

         p->element_to_buffer_varient[i] = ELEMENT_BUFFER_INSTANCE_ID;
      }
   }

   if (0) debug_printf("nr_buffers: %d\n", p->nr_buffers);

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
