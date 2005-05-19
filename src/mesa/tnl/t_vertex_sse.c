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

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "t_context.h"
#include "t_vertex.h"
#include "simple_list.h"

#define X    0
#define Y    1
#define Z    2
#define W    3

#define DISASSEM 1

struct x86_reg {
   GLuint file:3;
   GLuint idx:3;
   GLuint mod:2;		/* mod_REG if this is just a register */
   GLint  disp:24;		/* only +/- 23bits of offset - should be enough... */
};

struct x86_program {
   GLcontext *ctx;
   
   GLubyte *store;
   GLubyte *csr;

   GLuint stack_offset;

   GLboolean inputs_safe;
   GLboolean outputs_safe;
   GLboolean have_sse2;
   GLboolean need_emms;
   
   struct x86_reg identity;
   struct x86_reg vp0;
   struct x86_reg vp1;
};


#define X86_TWOB 0x0f

/* There are more but these are all we'll use:
 */
enum x86_reg_file {
   file_REG32,
   file_MMX,
   file_XMM
};

/* Values for mod field of modr/m byte
 */
enum x86_reg_mod {
   mod_INDIRECT,
   mod_DISP8,
   mod_DISP32,
   mod_REG
};

enum x86_reg_name {
   reg_AX,
   reg_CX,
   reg_DX,
   reg_BX,
   reg_SP,
   reg_BP,
   reg_SI,
   reg_DI
};


enum x86_cc {
   cc_O,			/* overflow */
   cc_NO,			/* not overflow */
   cc_NAE,			/* not above or equal / carry */
   cc_AE,			/* above or equal / not carry */
   cc_E,			/* equal / zero */
   cc_NE			/* not equal / not zero */
};

#define cc_Z  cc_E
#define cc_NZ cc_NE


/* Create and manipulate registers and regmem values:
 */
static struct x86_reg make_reg( GLuint file,
				GLuint idx )
{
   struct x86_reg reg;

   reg.file = file;
   reg.idx = idx;
   reg.mod = mod_REG;
   reg.disp = 0;

   return reg;
}

static struct x86_reg make_disp( struct x86_reg reg,
				 GLint disp )
{
   assert(reg.file == file_REG32);

   if (reg.mod == mod_REG)
      reg.disp = disp;
   else
      reg.disp += disp;

   if (reg.disp == 0)
      reg.mod = mod_INDIRECT;
   else if (reg.disp <= 127 && reg.disp >= -128)
      reg.mod = mod_DISP8;
   else
      reg.mod = mod_DISP32;

   return reg;
}

static struct x86_reg deref( struct x86_reg reg )
{
   return make_disp(reg, 0);
}

static struct x86_reg get_base_reg( struct x86_reg reg )
{
   return make_reg( reg.file, reg.idx );
}


/* Retreive a reference to one of the function arguments, taking into
 * account any push/pop activity:
 */
static struct x86_reg make_fn_arg( struct x86_program *p,
				   GLuint arg )
{
   return make_disp(make_reg(file_REG32, reg_SP), 
		    p->stack_offset + arg * 4);	/* ??? */
}

static struct x86_reg get_identity( struct x86_program *p )
{
   return p->identity;
}


/* Emit bytes to the instruction stream:
 */
static void emit_1b( struct x86_program *p, GLbyte b0 )
{
   *(GLbyte *)(p->csr++) = b0;
}

static void emit_1i( struct x86_program *p, GLint i0 )
{
   *(GLint *)(p->csr) = i0;
   p->csr += 4;
}

static void disassem( struct x86_program *p, const char *fn )
{
#if DISASSEM
   static const char *last_fn;
   if (fn && fn != last_fn) {
      _mesa_printf("0x%x: %s\n", p->csr, fn);
      last_fn = fn;
   }
#endif
}

static void emit_1ub_fn( struct x86_program *p, GLubyte b0, const char *fn )
{
   disassem(p, fn);
   *(p->csr++) = b0;
}

static void emit_2ub_fn( struct x86_program *p, GLubyte b0, GLubyte b1, const char *fn )
{
   disassem(p, fn);
   *(p->csr++) = b0;
   *(p->csr++) = b1;
}

static void emit_3ub_fn( struct x86_program *p, GLubyte b0, GLubyte b1, GLubyte b2, const char *fn )
{
   disassem(p, fn);
   *(p->csr++) = b0;
   *(p->csr++) = b1;
   *(p->csr++) = b2;
}

#define emit_1ub(p, b0)         emit_1ub_fn(p, b0, __FUNCTION__)
#define emit_2ub(p, b0, b1)     emit_2ub_fn(p, b0, b1, __FUNCTION__)
#define emit_3ub(p, b0, b1, b2) emit_3ub_fn(p, b0, b1, b2, __FUNCTION__)


/* Labels, jumps and fixup:
 */
static GLubyte *get_label( struct x86_program *p )
{
   return p->csr;
}

static void x86_jcc( struct x86_program *p,
		      GLuint cc,
		      GLubyte *label )
{
   GLint offset = label - (get_label(p) + 2);
   
   if (offset <= 127 && offset >= -128) {
      emit_1ub(p, 0x70 + cc);
      emit_1b(p, (GLbyte) offset);
   }
   else {
      offset = label - (get_label(p) + 6);
      emit_2ub(p, 0x0f, 0x80 + cc);
      emit_1i(p, offset);
   }
}

/* Always use a 32bit offset for forward jumps:
 */
static GLubyte *x86_jcc_forward( struct x86_program *p,
				 GLuint cc )
{
   emit_2ub(p, 0x0f, 0x80 + cc);
   emit_1i(p, 0);
   return get_label(p);
}

/* Fixup offset from forward jump:
 */
static void do_fixup( struct x86_program *p,
		      GLubyte *fixup )
{
   *(int *)(fixup - 4) = get_label(p) - fixup;
}

static void x86_push( struct x86_program *p,
		       struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x50 + reg.idx);
   p->stack_offset += 4;
}

static void x86_pop( struct x86_program *p,
		       struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x58 + reg.idx);
   p->stack_offset -= 4;
}

static void x86_inc( struct x86_program *p,
		       struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x40 + reg.idx);
}

static void x86_dec( struct x86_program *p,
		       struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x48 + reg.idx);
}

static void x86_ret( struct x86_program *p )
{
   emit_1ub(p, 0xc3);
}

static void mmx_emms( struct x86_program *p )
{
   assert(p->need_emms);
   emit_2ub(p, 0x0f, 0x0e);
   p->need_emms = 0;
}




/* Build a modRM byte + possible displacement.  No treatment of SIB
 * indexing.  BZZT - no way to encode an absolute address.
 */
static void emit_modrm( struct x86_program *p, 
			struct x86_reg reg, 
			struct x86_reg regmem )
{
   GLubyte val = 0;
   
   assert(reg.mod == mod_REG);
   
   val |= regmem.mod << 6;     	/* mod field */
   val |= reg.idx << 3;		/* reg field */
   val |= regmem.idx;		/* r/m field */
   
   emit_1ub_fn(p, val, 0);

   /* Oh-oh we've stumbled into the SIB thing.
    */
   if (regmem.idx == reg_SP) {
      emit_1ub_fn(p, 0x24, 0);		/* simplistic! */
   }

   switch (regmem.mod) {
   case mod_REG:
   case mod_INDIRECT:
      break;
   case mod_DISP8:
      emit_1b(p, regmem.disp);
      break;
   case mod_DISP32:
      emit_1i(p, regmem.disp);
      break;
   }
}

/* Many x86 instructions have two opcodes to cope with the situations
 * where the destination is a register or memory reference
 * respectively.  This function selects the correct opcode based on
 * the arguments presented.
 */
static void emit_op_modrm( struct x86_program *p,
			   GLubyte op_dst_is_reg, 
			   GLubyte op_dst_is_mem,
			   struct x86_reg dst,
			   struct x86_reg src )
{
   switch (dst.mod) {
   case mod_REG:
      emit_1ub_fn(p, op_dst_is_reg, 0);
      emit_modrm(p, dst, src);
      break;
   case mod_INDIRECT:
   case mod_DISP32:
   case mod_DISP8:
      assert(src.mod == mod_REG);
      emit_1ub_fn(p, op_dst_is_mem, 0);
      emit_modrm(p, src, dst);
      break;
   }
}

static void x86_mov( struct x86_program *p,
		      struct x86_reg dst,
		      struct x86_reg src )
{
   emit_op_modrm( p, 0x8b, 0x89, dst, src );
}

static void x86_xor( struct x86_program *p,
		      struct x86_reg dst,
		      struct x86_reg src )
{
   emit_op_modrm( p, 0x33, 0x31, dst, src );
}

static void x86_cmp( struct x86_program *p,
		      struct x86_reg dst,
		      struct x86_reg src )
{
   emit_op_modrm( p, 0x3b, 0x39, dst, src );
}

static void sse2_movd( struct x86_program *p,
		       struct x86_reg dst,
		       struct x86_reg src )
{
   assert(p->have_sse2);
   emit_2ub(p, 0x66, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}

static void mmx_movd( struct x86_program *p,
		       struct x86_reg dst,
		       struct x86_reg src )
{
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}

static void mmx_movq( struct x86_program *p,
		       struct x86_reg dst,
		       struct x86_reg src )
{
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6f, 0x7f, dst, src );
}


static void sse_movss( struct x86_program *p,
		       struct x86_reg dst,
		       struct x86_reg src )
{
   emit_2ub(p, 0xF3, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

static void sse_movaps( struct x86_program *p,
			 struct x86_reg dst,
			 struct x86_reg src )
{
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x28, 0x29, dst, src );
}

static void sse_movups( struct x86_program *p,
			 struct x86_reg dst,
			 struct x86_reg src )
{
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

static void sse_movhps( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x16, 0x17, dst, src ); /* cf movlhps */
}

static void sse_movlps( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x12, 0x13, dst, src ); /* cf movhlps */
}

/* SSE operations often only have one format, with dest constrained to
 * be a register:
 */
static void sse_mulps( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x59);
   emit_modrm( p, dst, src );
}

static void sse_addps( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

static void sse_movhlps( struct x86_program *p,
			 struct x86_reg dst,
			 struct x86_reg src )
{
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x12);
   emit_modrm( p, dst, src );
}

static void sse_movlhps( struct x86_program *p,
			 struct x86_reg dst,
			 struct x86_reg src )
{
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x16);
   emit_modrm( p, dst, src );
}

static void sse2_cvtps2dq( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   assert(p->have_sse2);
   emit_3ub(p, 0x66, X86_TWOB, 0x5B);
   emit_modrm( p, dst, src );
}

static void sse2_packssdw( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   assert(p->have_sse2);
   emit_3ub(p, 0x66, X86_TWOB, 0x6B);
   emit_modrm( p, dst, src );
}

static void sse2_packsswb( struct x86_program *p,
			struct x86_reg dst,
			struct x86_reg src )
{
   assert(p->have_sse2);
   emit_3ub(p, 0x66, X86_TWOB, 0x63);
   emit_modrm( p, dst, src );
}

static void sse2_packuswb( struct x86_program *p,
			   struct x86_reg dst,
			   struct x86_reg src )
{
   assert(p->have_sse2);
   emit_3ub(p, 0x66, X86_TWOB, 0x67);
   emit_modrm( p, dst, src );
}

static void sse_cvtps2pi( struct x86_program *p,
			  struct x86_reg dst,
			  struct x86_reg src )
{
   assert(dst.file == file_MMX && 
	  (src.file == file_XMM || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x2d);
   emit_modrm( p, dst, src );
}

static void mmx_packssdw( struct x86_program *p,
			  struct x86_reg dst,
			  struct x86_reg src )
{
   assert(dst.file == file_MMX && 
	  (src.file == file_MMX || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x6b);
   emit_modrm( p, dst, src );
}

static void mmx_packuswb( struct x86_program *p,
			  struct x86_reg dst,
			  struct x86_reg src )
{
   assert(dst.file == file_MMX && 
	  (src.file == file_MMX || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x67);
   emit_modrm( p, dst, src );
}


/* Load effective address:
 */
static void x86_lea( struct x86_program *p,
		     struct x86_reg dst,
		     struct x86_reg src )
{
   emit_1ub(p, 0x8d);
   emit_modrm( p, dst, src );
}

static void x86_test( struct x86_program *p,
		      struct x86_reg dst,
		      struct x86_reg src )
{
   emit_1ub(p, 0x85);
   emit_modrm( p, dst, src );
}




/**
 * Perform a reduced swizzle:
 */
static void sse2_pshufd( struct x86_program *p,
			 struct x86_reg dest,
			 struct x86_reg arg0,
			 GLubyte x,
			 GLubyte y,
			 GLubyte z,
			 GLubyte w) 
{
   assert(p->have_sse2);
   emit_3ub(p, 0x66, X86_TWOB, 0x70);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, (x|(y<<2)|(z<<4)|w<<6)); 
}


/* Shufps can also be used to implement a reduced swizzle when dest ==
 * arg0.
 */
static void sse_shufps( struct x86_program *p,
			 struct x86_reg dest,
			 struct x86_reg arg0,
			 GLubyte x,
			 GLubyte y,
			 GLubyte z,
			 GLubyte w) 
{
   emit_2ub(p, X86_TWOB, 0xC6);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, (x|(y<<2)|(z<<4)|w<<6)); 
}


static void emit_load4f_4( struct x86_program *p, 			   
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   sse_movups(p, dest, arg0);
}

static void emit_load4f_3( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   /* Have to jump through some hoops:
    *
    * 0 0 0 1 -- skip if reg[3] preserved over loop iterations
    * c 0 0 1
    * 0 0 c 1
    * a b c 1 
    */
   sse_movaps(p, dest, get_identity(p));
   sse_movss(p, dest, make_disp(arg0, 8));
   sse_shufps(p, dest, dest, Y,Z,X,W );
   sse_movlps(p, dest, arg0);
}

static void emit_load4f_2( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   /* Pull in 2 dwords, then copy the top 2 dwords with 0,1 from id.
    */
   sse_movlps(p, dest, arg0);
   sse_movhps(p, dest, get_identity(p));
}

static void emit_load4f_1( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   /* Initialized with [0,0,0,1] from id, then pull in the single low
    * word.
    */
   sse_movaps(p, dest, get_identity(p));
   sse_movss(p, dest, arg0);
}



static void emit_load3f_3( struct x86_program *p, 			   
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   /* Over-reads by 1 dword - potential SEGV if input is a vertex
    * array.
    */
   if (p->inputs_safe) {
      sse_movups(p, dest, arg0);
   } 
   else {
      /* c . . .
       * c c c c
       * a b c c 
       */
      sse_movss(p, dest, make_disp(arg0, 8));
      sse_shufps(p, dest, dest, X,X,X,X);
      sse_movlps(p, dest, arg0);
   }
}

static void emit_load3f_2( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   emit_load4f_2(p, dest, arg0);
}

static void emit_load3f_1( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   emit_load4f_1(p, dest, arg0);
}

static void emit_load2f_2( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   sse_movlps(p, dest, arg0);
}

static void emit_load2f_1( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   emit_load4f_1(p, dest, arg0);
}

static void emit_load1f_1( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   sse_movss(p, dest, arg0);
}

static void (*load[4][4])( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 ) = {
   { emit_load1f_1, 
     emit_load1f_1, 
     emit_load1f_1, 
     emit_load1f_1 },

   { emit_load2f_1, 
     emit_load2f_2, 
     emit_load2f_2, 
     emit_load2f_2 },

   { emit_load3f_1, 
     emit_load3f_2, 
     emit_load3f_3, 
     emit_load3f_3 },

   { emit_load4f_1, 
     emit_load4f_2, 
     emit_load4f_3, 
     emit_load4f_4 } 
};

static void emit_load( struct x86_program *p,
		       struct x86_reg dest,
		       GLuint sz,
		       struct x86_reg src,
		       GLuint src_sz)
{
   if (DISASSEM)
      _mesa_printf("load %d/%d\n", sz, src_sz);
   load[sz-1][src_sz-1](p, dest, src);
}


static void emit_store4f( struct x86_program *p, 			   
			  struct x86_reg dest,
			  struct x86_reg arg0 )
{
   sse_movups(p, dest, arg0);
}

static void emit_store3f( struct x86_program *p, 
			  struct x86_reg dest,
			  struct x86_reg arg0 )
{
   if (p->outputs_safe) {
      /* Emit the extra dword anyway.  This may hurt writecombining,
       * may cause other problems.
       */
      sse_movups(p, dest, arg0);
   }
   else {
      /* Alternate strategy - emit two, shuffle, emit one.
       */
      sse_movlps(p, dest, arg0);
      sse_shufps(p, arg0, arg0, Z, Z, Z, Z ); /* NOTE! destructive */
      sse_movss(p, make_disp(dest,8), arg0);
   }
}

static void emit_store2f( struct x86_program *p, 
			   struct x86_reg dest,
			   struct x86_reg arg0 )
{
   sse_movlps(p, dest, arg0);
}

static void emit_store1f( struct x86_program *p, 
			  struct x86_reg dest,
			  struct x86_reg arg0 )
{
   sse_movss(p, dest, arg0);
}


static void (*store[4])( struct x86_program *p, 
			 struct x86_reg dest,
			 struct x86_reg arg0 ) = 
{
   emit_store1f, 
   emit_store2f, 
   emit_store3f, 
   emit_store4f 
};

static void emit_store( struct x86_program *p,
			struct x86_reg dest,
			GLuint sz,
			struct x86_reg temp )

{
   if (DISASSEM)
      _mesa_printf("store %d\n", sz);
   store[sz-1](p, dest, temp);
}

static void emit_pack_store_4ub( struct x86_program *p,
				 struct x86_reg dest,
				 struct x86_reg temp )
{
   if (p->have_sse2) {
      sse2_cvtps2dq(p, temp, temp);
      sse2_packssdw(p, temp, temp);
      sse2_packuswb(p, temp, temp);
      sse_movss(p, dest, temp);
   }
   else {
      struct x86_reg mmx0 = make_reg(file_MMX, 0);
      struct x86_reg mmx1 = make_reg(file_MMX, 1);
      sse_cvtps2pi(p, mmx0, temp);
      sse_movhlps(p, temp, temp);
      sse_cvtps2pi(p, mmx1, temp);
      mmx_packssdw(p, mmx0, mmx1);
      mmx_packuswb(p, mmx0, mmx0);
      mmx_movd(p, dest, mmx0);
   }
}

static GLint get_offset( const void *a, const void *b )
{
   return (const char *)b - (const char *)a;
}



/* Lots of hardcoding
 *
 * EAX -- pointer to current output vertex
 * ECX -- pointer to current attribute 
 * 
 */
static GLboolean build_vertex_emit( struct x86_program *p )
{
   GLcontext *ctx = p->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_attr *a = vtx->attr;
   GLuint j;

   struct x86_reg vertexEAX = make_reg(file_REG32, reg_AX);
   struct x86_reg srcEDI = make_reg(file_REG32, reg_CX);
   struct x86_reg countEBP = make_reg(file_REG32, reg_BP);
   struct x86_reg vtxESI = make_reg(file_REG32, reg_SI);
   struct x86_reg temp = make_reg(file_XMM, 0);
   struct x86_reg vp0 = make_reg(file_XMM, 1);
   struct x86_reg vp1 = make_reg(file_XMM, 2);
   struct x86_reg chan0 = make_reg(file_XMM, 3);
   GLubyte *fixup, *label;

   p->csr = p->store;
   
   /* Push a few regs?
    */
   x86_push(p, srcEDI);
   x86_push(p, countEBP);
   x86_push(p, vtxESI);


   /* Get vertex count, compare to zero
    */
   x86_xor(p, srcEDI, srcEDI);
   x86_mov(p, countEBP, make_fn_arg(p, 2));
   x86_cmp(p, countEBP, srcEDI);
   fixup = x86_jcc_forward(p, cc_E);

   /* Initialize destination register. 
    */
   x86_mov(p, vertexEAX, make_fn_arg(p, 3));

   /* Dereference ctx to get tnl, then vtx:
    */
   x86_mov(p, vtxESI, make_fn_arg(p, 1));
   x86_mov(p, vtxESI, make_disp(vtxESI, get_offset(ctx, &ctx->swtnl_context)));
   vtxESI = make_disp(vtxESI, get_offset(tnl, &tnl->clipspace));

   
   /* Possibly load vp0, vp1 for viewport calcs:
    */
   if (vtx->need_viewport) {
      sse_movups(p, vp0, make_disp(vtxESI, get_offset(vtx, &vtx->vp_scale[0])));
      sse_movups(p, vp1, make_disp(vtxESI, get_offset(vtx, &vtx->vp_xlate[0])));
   }

   /* always load, needed or not:
    */
   sse_movups(p, chan0, make_disp(vtxESI, get_offset(vtx, &vtx->chan_scale[0])));
   sse_movups(p, p->identity, make_disp(vtxESI, get_offset(vtx, &vtx->identity[0])));

   /* Note address for loop jump */
   label = get_label(p);

   /* Emit code for each of the attributes.  Currently routes
    * everything through SSE registers, even when it might be more
    * efficient to stick with regular old x86.  No optimization or
    * other tricks - enough new ground to cover here just getting
    * things working.
    */
   for (j = 0; j < vtx->attr_count; j++) {
      struct x86_reg dest = make_disp(vertexEAX, vtx->attr[j].vertoffset);
      struct x86_reg ptr_to_src = make_disp(vtxESI, get_offset(vtx, &vtx->attr[j].inputptr));

      /* Load current a[j].inputptr
       */
      x86_mov(p, srcEDI, ptr_to_src);

      /* Now, load an XMM reg from src, perhaps transform, then save.
       * Could be shortcircuited in specific cases:
       */
      switch (a[j].format) {
      case EMIT_1F:
	 emit_load(p, temp, 1, deref(srcEDI), vtx->attr[j].inputsize);
	 emit_store(p, dest, 1, temp);
	 break;
      case EMIT_2F:
	 emit_load(p, temp, 2, deref(srcEDI), vtx->attr[j].inputsize);
	 emit_store(p, dest, 2, temp);
	 break;
      case EMIT_3F:
	 /* Potentially the worst case - hardcode 2+1 copying:
	  */
	 emit_load(p, temp, 3, deref(srcEDI), vtx->attr[j].inputsize);
	 emit_store(p, dest, 3, temp);
	 break;
      case EMIT_4F:
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 emit_store(p, dest, 4, temp);
	 break;
      case EMIT_2F_VIEWPORT: 
	 emit_load(p, temp, 2, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_mulps(p, temp, vp0);
	 sse_addps(p, temp, vp1);
	 emit_store(p, dest, 2, temp);
	 break;
      case EMIT_3F_VIEWPORT: 
	 emit_load(p, temp, 3, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_mulps(p, temp, vp0);
	 sse_addps(p, temp, vp1);
	 emit_store(p, dest, 3, temp);
	 break;
      case EMIT_4F_VIEWPORT: 
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_mulps(p, temp, vp0);
	 sse_addps(p, temp, vp1);
	 emit_store(p, dest, 4, temp);
	 break;
      case EMIT_3F_XYW:
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_shufps(p, temp, temp, X, Y, W, Z);
	 emit_store(p, dest, 3, temp);
	 break;

	 /* Try and bond 3ub + 1ub pairs into a single 4ub operation?
	  */	 
      case EMIT_1UB_1F:	 
      case EMIT_3UB_3F_RGB:
      case EMIT_3UB_3F_BGR:
	 _mesa_printf("non-implemneted format %d\n", a[j].format);
	 return GL_FALSE;	/* add this later */

      case EMIT_4UB_4F_RGBA:
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_mulps(p, temp, chan0);
	 emit_pack_store_4ub(p, dest, temp);
	 break;
      case EMIT_4UB_4F_BGRA:
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_shufps(p, temp, temp, Z, Y, X, W);
	 sse_mulps(p, temp, chan0);
	 emit_pack_store_4ub(p, dest, temp);
	 break;
      case EMIT_4UB_4F_ARGB:
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_shufps(p, temp, temp, W, X, Y, Z);
	 sse_mulps(p, temp, chan0);
	 emit_pack_store_4ub(p, dest, temp);
	 break;
      case EMIT_4UB_4F_ABGR:
	 emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	 sse_shufps(p, temp, temp, W, Z, Y, X);
	 sse_mulps(p, temp, chan0);
	 emit_pack_store_4ub(p, dest, temp);
	 break;
      case EMIT_4CHAN_4F_RGBA:
	 switch (CHAN_TYPE) {
	 case GL_UNSIGNED_BYTE:
	    emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	    sse_mulps(p, temp, chan0);
	    emit_pack_store_4ub(p, dest, temp);
	    break;
	 case GL_FLOAT:
	    emit_load(p, temp, 4, deref(srcEDI), vtx->attr[j].inputsize);
	    emit_store(p, dest, 4, temp);
	    break;
	 case GL_UNSIGNED_SHORT:
	 default:
	    _mesa_printf("unknown CHAN_TYPE %s\n", _mesa_lookup_enum_by_nr(CHAN_TYPE));
	    return GL_FALSE;
	 }
	 break;
      default:
	 _mesa_printf("unknown a[%d].format %d\n", j, a[j].format);
	 return GL_FALSE;	/* catch any new opcodes */
      }
      
      /* add a[j].inputstride (hardcoded value - could just as easily
       * pull the stride value from memory each time).
       */
      x86_lea(p, srcEDI, make_disp(srcEDI, a[j].inputstride));
      
      /* save new value of a[j].inputptr 
       */
      x86_mov(p, ptr_to_src, srcEDI);

   }

   /* Next vertex:
    */
   x86_lea(p, vertexEAX, make_disp(vertexEAX, vtx->vertex_size));

   /* decr count, loop if not zero
    */
   x86_dec(p, countEBP);
   x86_test(p, countEBP, countEBP); 
   x86_jcc(p, cc_NZ, label);

   /* Exit mmx state?
    */
   if (p->need_emms)
      mmx_emms(p);

   /* Land forward jump here:
    */
   do_fixup(p, fixup);

   /* Pop regs and return
    */
   x86_pop(p, get_base_reg(vtxESI));
   x86_pop(p, countEBP);
   x86_pop(p, srcEDI);
   x86_ret(p);

   vtx->emit = (tnl_emit_func)p->store;
   return GL_TRUE;
}

void _tnl_generate_sse_emit( GLcontext *ctx )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct x86_program p;

   memset(&p, 0, sizeof(p));
   p.ctx = ctx;
   p.store = MALLOC(1024);

   p.inputs_safe = 1;		/* for now */
   p.outputs_safe = 1;		/* for now */
   p.have_sse2 = 0;		/* testing */
   p.identity = make_reg(file_XMM, 6);
   
   if (build_vertex_emit(&p)) {
      _tnl_register_fastpath( vtx, GL_TRUE );
      if (DISASSEM)
	 _mesa_printf("disassemble 0x%x 0x%x\n", p.store, p.csr);
   }
   else {
      /* Note the failure:
       */
      _tnl_register_fastpath( vtx, GL_FALSE );
      FREE(p.store);
   }

   (void)sse2_movd;
   (void)x86_inc;
   (void)x86_xor;
}
