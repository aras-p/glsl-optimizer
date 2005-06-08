#if defined(USE_X86_ASM)

#include "x86sse.h"

#define DISASSEM 0
#define X86_TWOB 0x0f

/* Emit bytes to the instruction stream:
 */
static void emit_1b( struct x86_function *p, GLbyte b0 )
{
   *(GLbyte *)(p->csr++) = b0;
}

static void emit_1i( struct x86_function *p, GLint i0 )
{
   *(GLint *)(p->csr) = i0;
   p->csr += 4;
}

static void disassem( struct x86_function *p, const char *fn )
{
#if DISASSEM
   if (fn && fn != p->fn) {
      _mesa_printf("0x%x: %s\n", p->csr, fn);
      p->fn = fn;
   }
#endif
}

static void emit_1ub_fn( struct x86_function *p, GLubyte b0, const char *fn )
{
   disassem(p, fn);
   *(p->csr++) = b0;
}

static void emit_2ub_fn( struct x86_function *p, GLubyte b0, GLubyte b1, const char *fn )
{
   disassem(p, fn);
   *(p->csr++) = b0;
   *(p->csr++) = b1;
}

static void emit_3ub_fn( struct x86_function *p, GLubyte b0, GLubyte b1, GLubyte b2, const char *fn )
{
   disassem(p, fn);
   *(p->csr++) = b0;
   *(p->csr++) = b1;
   *(p->csr++) = b2;
}

#define emit_1ub(p, b0)         emit_1ub_fn(p, b0, __FUNCTION__)
#define emit_2ub(p, b0, b1)     emit_2ub_fn(p, b0, b1, __FUNCTION__)
#define emit_3ub(p, b0, b1, b2) emit_3ub_fn(p, b0, b1, b2, __FUNCTION__)



/* Build a modRM byte + possible displacement.  No treatment of SIB
 * indexing.  BZZT - no way to encode an absolute address.
 */
static void emit_modrm( struct x86_function *p, 
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
   if (regmem.file == file_REG32 &&
       regmem.idx == reg_SP) {
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
   default:
      assert(0);
      break;
   }
}

/* Many x86 instructions have two opcodes to cope with the situations
 * where the destination is a register or memory reference
 * respectively.  This function selects the correct opcode based on
 * the arguments presented.
 */
static void emit_op_modrm( struct x86_function *p,
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
   default:
      assert(0);
      break;
   }
}







/* Create and manipulate registers and regmem values:
 */
struct x86_reg x86_make_reg( GLuint file,
			     GLuint idx )
{
   struct x86_reg reg;

   reg.file = file;
   reg.idx = idx;
   reg.mod = mod_REG;
   reg.disp = 0;

   return reg;
}

struct x86_reg x86_make_disp( struct x86_reg reg,
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

struct x86_reg x86_deref( struct x86_reg reg )
{
   return x86_make_disp(reg, 0);
}

struct x86_reg x86_get_base_reg( struct x86_reg reg )
{
   return x86_make_reg( reg.file, reg.idx );
}



/* Labels, jumps and fixup:
 */
GLubyte *x86_get_label( struct x86_function *p )
{
   return p->csr;
}

void x86_jcc( struct x86_function *p,
	      GLuint cc,
	      GLubyte *label )
{
   GLint offset = label - (x86_get_label(p) + 2);
   
   if (offset <= 127 && offset >= -128) {
      emit_1ub(p, 0x70 + cc);
      emit_1b(p, (GLbyte) offset);
   }
   else {
      offset = label - (x86_get_label(p) + 6);
      emit_2ub(p, 0x0f, 0x80 + cc);
      emit_1i(p, offset);
   }
}

/* Always use a 32bit offset for forward jumps:
 */
GLubyte *x86_jcc_forward( struct x86_function *p,
			  GLuint cc )
{
   emit_2ub(p, 0x0f, 0x80 + cc);
   emit_1i(p, 0);
   return x86_get_label(p);
}

/* Fixup offset from forward jump:
 */
void x86_fixup_fwd_jump( struct x86_function *p,
			 GLubyte *fixup )
{
   *(int *)(fixup - 4) = x86_get_label(p) - fixup;
}

void x86_push( struct x86_function *p,
	       struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x50 + reg.idx);
   p->stack_offset += 4;
}

void x86_pop( struct x86_function *p,
	      struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x58 + reg.idx);
   p->stack_offset -= 4;
}

void x86_inc( struct x86_function *p,
	      struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x40 + reg.idx);
}

void x86_dec( struct x86_function *p,
	      struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x48 + reg.idx);
}

void x86_ret( struct x86_function *p )
{
   emit_1ub(p, 0xc3);
}

void mmx_emms( struct x86_function *p )
{
   assert(p->need_emms);
   emit_2ub(p, 0x0f, 0x77);
   p->need_emms = 0;
}




void x86_mov( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_op_modrm( p, 0x8b, 0x89, dst, src );
}

void x86_xor( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_op_modrm( p, 0x33, 0x31, dst, src );
}

void x86_cmp( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_op_modrm( p, 0x3b, 0x39, dst, src );
}

void sse2_movd( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, 0x66, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}

void mmx_movd( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}

void mmx_movq( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6f, 0x7f, dst, src );
}


void sse_movss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, 0xF3, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

void sse_movaps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x28, 0x29, dst, src );
}

void sse_movups( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

void sse_movhps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x16, 0x17, dst, src ); /* cf movlhps */
}

void sse_movlps( struct x86_function *p,
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
void sse_maxps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x5F);
   emit_modrm( p, dst, src );
}

void sse_divss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x5E);
   emit_modrm( p, dst, src );
}

void sse_minps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x5D);
   emit_modrm( p, dst, src );
}

void sse_subps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x5C);
   emit_modrm( p, dst, src );
}

void sse_mulps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x59);
   emit_modrm( p, dst, src );
}

void sse_addps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

void sse_addss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

void sse_andps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x54);
   emit_modrm( p, dst, src );
}

void sse2_rcpss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x53);
   emit_modrm( p, dst, src );
}

void sse_rsqrtss( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x52);
   emit_modrm( p, dst, src );

}

void sse_movhlps( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x12);
   emit_modrm( p, dst, src );
}

void sse_movlhps( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x16);
   emit_modrm( p, dst, src );
}

void sse2_cvtps2dq( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x5B);
   emit_modrm( p, dst, src );
}

void sse2_packssdw( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x6B);
   emit_modrm( p, dst, src );
}

void sse2_packsswb( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x63);
   emit_modrm( p, dst, src );
}

void sse2_packuswb( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x67);
   emit_modrm( p, dst, src );
}

void sse_cvtps2pi( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   assert(dst.file == file_MMX && 
	  (src.file == file_XMM || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x2d);
   emit_modrm( p, dst, src );
}

void mmx_packssdw( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   assert(dst.file == file_MMX && 
	  (src.file == file_MMX || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x6b);
   emit_modrm( p, dst, src );
}

void mmx_packuswb( struct x86_function *p,
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
void x86_lea( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_1ub(p, 0x8d);
   emit_modrm( p, dst, src );
}

void x86_test( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   emit_1ub(p, 0x85);
   emit_modrm( p, dst, src );
}


/**
 * Perform a reduced swizzle:
 */
void sse2_pshufd( struct x86_function *p,
		  struct x86_reg dest,
		  struct x86_reg arg0,
		  GLubyte shuf) 
{
   emit_3ub(p, 0x66, X86_TWOB, 0x70);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, shuf); 
}


/* Shufps can also be used to implement a reduced swizzle when dest ==
 * arg0.
 */
void sse_shufps( struct x86_function *p,
		 struct x86_reg dest,
		 struct x86_reg arg0,
		 GLubyte shuf) 
{
   emit_2ub(p, X86_TWOB, 0xC6);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, shuf); 
}

void sse_cmpps( struct x86_function *p,
		struct x86_reg dest,
		struct x86_reg arg0,
		GLubyte cc) 
{
   emit_2ub(p, X86_TWOB, 0xC2);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, cc); 
}


/* Retreive a reference to one of the function arguments, taking into
 * account any push/pop activity:
 */
struct x86_reg x86_fn_arg( struct x86_function *p,
			   GLuint arg )
{
   return x86_make_disp(x86_make_reg(file_REG32, reg_SP), 
			p->stack_offset + arg * 4);	/* ??? */
}


void x86_init_func( struct x86_function *p )
{
   p->store = malloc(1024);
   p->csr = p->store;
}

void x86_release_func( struct x86_function *p )
{
   free(p->store);
}


void (*x86_get_func( struct x86_function *p ))(void)
{
   if (DISASSEM)
      _mesa_printf("disassemble %p %p\n", p->store, p->csr);
   return (void (*)())p->store;
}

#else

void x86sse_dummy( void )
{
}

#endif
