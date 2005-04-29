/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <strings.h>

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "texenvprogram.h"

#include "shader/program.h"
#include "shader/nvfragprog.h"
#include "shader/arbfragparse.h"


#define DISASSEM 1

/* Use uregs to represent registers internally, translate to Mesa's
 * expected formats on emit.  
 *
 * NOTE: These are passed by value extensively in this file rather
 * than as usual by pointer reference.  If this disturbs you, try
 * remembering they are just 32bits in size.
 *
 * GCC is smart enough to deal with these dword-sized structures in
 * much the same way as if I had defined them as dwords and was using
 * macros to access and set the fields.  This is much nicer and easier
 * to evolve.
 */
struct ureg {
   GLuint file:4;
   GLuint idx:8;
   GLuint negatebase:1;
   GLuint abs:1;
   GLuint negateabs:1;
   GLuint swz:12;
   GLuint pad:5;
};

const static struct ureg undef = { 
   ~0,
   ~0,
   0,
   0,
   0,
   0,
   0
};

#define X    0
#define Y    1
#define Z    2
#define W    3

#define MAX_CONSTANT 32

/* State used to build the fragment program:
 */
struct texenv_fragment_program {
   struct fragment_program *prog;
   GLcontext *ctx;

   GLfloat constant[MAX_CONSTANT][4];
   GLuint constant_flags[MAX_CONSTANT];
   GLuint nr_constants;

   GLuint temp_flag;		/* Tracks temporary regs which are in
				 * use.
				 */


   struct { 
      GLuint reg;		/* Hardware constant idx */
      const GLfloat *values; 	/* Pointer to tracked values */
   } param[MAX_CONSTANT];
   GLuint nr_params;
      

   GLboolean error;

   struct ureg src_texture;   /* Reg containing sampled texture color,
			       * else undef.
			       */

   struct ureg src_previous;	/* Reg containing color from previous 
				 * stage.  May need to be decl'd.
				 */

   GLuint last_tex_stage;	/* Number of last enabled texture unit */
};



static struct ureg make_ureg(GLuint file, GLuint idx)
{
   struct ureg reg;
   reg.file = file;
   reg.idx = idx;
   reg.negatebase = 0;
   reg.abs = 0;
   reg.negateabs = 0;
   reg.swz = SWIZZLE_NOOP;
   reg.pad = 0;
   return reg;
}

static struct ureg swizzle( struct ureg reg, int x, int y, int z, int w )
{
   reg.swz = MAKE_SWIZZLE4(GET_SWZ(reg.swz, x),
			   GET_SWZ(reg.swz, y),
			   GET_SWZ(reg.swz, z),
			   GET_SWZ(reg.swz, w));

   return reg;
}

static struct ureg swizzle1( struct ureg reg, int x )
{
   return swizzle(reg, x, x, x, x);
}

static GLboolean is_undef( struct ureg reg )
{
   return reg.file == 0xf;
}

static struct ureg get_temp( struct texenv_fragment_program *p )
{
   int bit = ffs( ~p->temp_flag );
   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   p->temp_flag |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}


static void release_temps( struct texenv_fragment_program *p )
{
   p->temp_flag = ~0x7;
}


static struct ureg emit_decl( struct texenv_fragment_program *p,
			      GLuint type, GLuint nr )
{
   struct ureg reg = make_ureg(type, nr);

   if (type == PROGRAM_INPUT) {
      p->prog->InputsRead |= 1<<nr;
   }
   else {
      /* Other ???
       */
   }

   return reg;
}

static void emit_arg( struct fp_src_register *reg,
		      struct ureg ureg )
{
   reg->File = ureg.file;
   reg->Index = ureg.idx;
   reg->Swizzle = ureg.swz;
   reg->NegateBase = ureg.negatebase;
   reg->Abs = ureg.abs;
   reg->NegateAbs = ureg.negateabs;
}

static void emit_dst( struct fp_dst_register *dst,
		      struct ureg ureg, GLuint mask )
{
   dst->File = ureg.file;
   dst->Index = ureg.idx;
   dst->WriteMask = mask;
   dst->CondMask = 0;
   dst->CondSwizzle = 0;
}

static struct fp_instruction *
emit_op(struct texenv_fragment_program *p,
	GLuint op,
	struct ureg dest,
	GLuint mask,
	GLuint saturate,
	struct ureg src0,
	struct ureg src1,
	struct ureg src2 )
{
   GLuint nr = p->prog->Base.NumInstructions++;
   struct fp_instruction *inst = &p->prog->Instructions[nr];
      
   memset(inst, 0, sizeof(*inst));
   inst->Opcode = op;
   
   if (!is_undef(src0)) emit_arg( &inst->SrcReg[0], src0 );
   if (!is_undef(src1)) emit_arg( &inst->SrcReg[1], src1 );
   if (!is_undef(src2)) emit_arg( &inst->SrcReg[2], src2 );
   
   inst->Saturate = saturate;

   emit_dst( &inst->DstReg, dest, mask );

   return inst;
}
   

static struct ureg emit_arith( struct texenv_fragment_program *p,
			  GLuint op,
			  struct ureg dest,
			  GLuint mask,
			  GLuint saturate,
			  struct ureg src0,
			  struct ureg src1,
			  struct ureg src2 )
{
   emit_op(p, op, dest, mask, saturate, src0, src1, src2);
   
   p->prog->NumAluInstructions++;
   return dest;
}

static struct ureg emit_texld( struct texenv_fragment_program *p,
			  GLuint op,
			  struct ureg dest,
			  GLuint destmask,
			  GLuint tex_unit,
			  GLuint tex_idx,
			  struct ureg coord )
{
   struct fp_instruction *inst = emit_op( p, op, 
					  dest, destmask, 
					  0,		/* don't saturate? */
					  coord, 	/* arg 0? */
					  undef,
					  undef);
   
   inst->TexSrcIdx = tex_idx;
   inst->TexSrcUnit = tex_unit;

   p->prog->NumTexInstructions++;

   if (coord.file != PROGRAM_INPUT &&
       (coord.idx < VERT_ATTRIB_TEX0 ||
	coord.idx > VERT_ATTRIB_TEX7)) {
      p->prog->NumTexIndirections++;
   }

   return dest;
}


static struct ureg emit_const1f( struct texenv_fragment_program *p, GLfloat c0 )
{
   GLint reg, idx;

   for (reg = 0; reg < MAX_CONSTANT; reg++) {
      for (idx = 0; idx < 4; idx++) {
	 if (!(p->constant_flags[reg] & (1<<idx)) ||
	     p->constant[reg][idx] == c0) {
	    p->constant[reg][idx] = c0;
	    p->constant_flags[reg] |= 1<<idx;
	    if (reg+1 > p->nr_constants) p->nr_constants = reg+1;
	    return swizzle1(make_ureg(PROGRAM_LOCAL_PARAM, reg),idx);
	 }
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return undef;
}

static struct ureg emit_const2f( struct texenv_fragment_program *p, 
			    GLfloat c0, GLfloat c1 )
{
   GLint reg, idx;

   for (reg = 0; reg < MAX_CONSTANT; reg++) {
      if (p->constant_flags[reg] == 0xf)
	 continue;
      for (idx = 0; idx < 3; idx++) {
	 if (!(p->constant_flags[reg] & (3<<idx))) {
	    p->constant[reg][idx] = c0;
	    p->constant[reg][idx+1] = c1;
	    p->constant_flags[reg] |= 3<<idx;
	    if (reg+1 > p->nr_constants) p->nr_constants = reg+1;
	    return swizzle(make_ureg(PROGRAM_LOCAL_PARAM, reg),idx,idx+1,idx,idx+1);
	 }
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return undef;
}



static struct ureg emit_const4f( struct texenv_fragment_program *p, 
			    GLfloat c0, GLfloat c1, GLfloat c2, GLfloat c3 )
{
   GLint reg;

   for (reg = 0; reg < MAX_CONSTANT; reg++) {
      if (p->constant_flags[reg] == 0xf &&
	  p->constant[reg][0] == c0 &&
	  p->constant[reg][1] == c1 &&
	  p->constant[reg][2] == c2 &&
	  p->constant[reg][3] == c3) {
	 return make_ureg(PROGRAM_LOCAL_PARAM, reg);
      }
      else if (p->constant_flags[reg] == 0) {
	 p->constant[reg][0] = c0;
	 p->constant[reg][1] = c1;
	 p->constant[reg][2] = c2;
	 p->constant[reg][3] = c3;
	 p->constant_flags[reg] = 0xf;
	 if (reg+1 > p->nr_constants) p->nr_constants = reg+1;
	 return make_ureg(PROGRAM_LOCAL_PARAM, reg);
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return undef;
}


static struct ureg emit_const4fv( struct texenv_fragment_program *p, const GLfloat *c )
{
   return emit_const4f( p, c[0], c[1], c[2], c[3] );
}






static void program_error( struct texenv_fragment_program *p, const char *msg )
{
   fprintf(stderr, "%s\n", msg);
   p->error = 1;
}


static GLuint translate_tex_src_bit( struct texenv_fragment_program *p,
				     GLuint bit )
{
   switch (bit) {
   case TEXTURE_1D_BIT:   return TEXTURE_1D_INDEX;
   case TEXTURE_2D_BIT:   return TEXTURE_2D_INDEX;
   case TEXTURE_RECT_BIT: return TEXTURE_RECT_INDEX;
   case TEXTURE_3D_BIT:   return TEXTURE_3D_INDEX;
   case TEXTURE_CUBE_BIT: return TEXTURE_CUBE_INDEX;
   default: program_error(p, "TexSrcBit"); return 0;
   }
}


static struct ureg get_source( struct texenv_fragment_program *p, 
			  GLenum src, GLuint unit )
{
   switch (src) {
   case GL_TEXTURE: 
      if (is_undef(p->src_texture)) {

	 GLuint dim = translate_tex_src_bit( p, p->ctx->Texture.Unit[unit]._ReallyEnabled);
	 struct ureg texcoord = emit_decl(p, 
					  PROGRAM_INPUT, 
					  VERT_ATTRIB_TEX0+unit);
	 struct ureg tmp = get_temp( p );

	 /* TODO: Use D0_MASK_XY where possible.
	  */
	 p->src_texture = emit_texld( p, FP_OPCODE_TXP,
				      tmp, WRITEMASK_XYZW, 
				      unit, dim, texcoord );
      }

      return p->src_texture;

      /* Crossbar: */
   case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7: {
      return undef;
   }

   case GL_CONSTANT:
      return emit_const4fv( p, p->ctx->Texture.Unit[unit].EnvColor );
   case GL_PRIMARY_COLOR:
      return emit_decl(p, PROGRAM_INPUT, VERT_ATTRIB_COLOR0);
   case GL_PREVIOUS:
   default: 
      return emit_decl(p, 
		       p->src_previous.file,
		       p->src_previous.idx); 
   }
}
			

static struct ureg emit_combine_source( struct texenv_fragment_program *p, 
				   GLuint mask,
				   GLuint unit,
				   GLenum source, 
				   GLenum operand )
{
   struct ureg arg, src, one;

   src = get_source(p, source, unit);

   switch (operand) {
   case GL_ONE_MINUS_SRC_COLOR: 
      /* Get unused tmp,
       * Emit tmp = 1.0 - arg.xyzw
       */
      arg = get_temp( p );
      one = emit_const1f(p, 1);
      return emit_arith( p, FP_OPCODE_SUB, arg, mask, 0, one, src, undef);

   case GL_SRC_ALPHA: 
      if (mask == WRITEMASK_W)
	 return src;
      else
	 return swizzle1( src, W );
   case GL_ONE_MINUS_SRC_ALPHA: 
      /* Get unused tmp,
       * Emit tmp = 1.0 - arg.wwww
       */
      arg = get_temp( p );
      one = emit_const1f(p, 1);
      return emit_arith( p, FP_OPCODE_SUB, arg, mask, 0,
			 one, swizzle1(src, W), undef);
   case GL_SRC_COLOR: 
   default:
      return src;
   }
}



static int nr_args( GLenum mode )
{
   switch (mode) {
   case GL_REPLACE: return 1; 
   case GL_MODULATE: return 2;
   case GL_ADD: return 2;
   case GL_ADD_SIGNED: return 2;
   case GL_INTERPOLATE:	return 3;
   case GL_SUBTRACT: return 2;
   case GL_DOT3_RGB_EXT: return 2;
   case GL_DOT3_RGBA_EXT: return 2;
   case GL_DOT3_RGB: return 2;
   case GL_DOT3_RGBA: return 2;
   default: return 0;
   }
}


static GLboolean args_match( struct gl_texture_unit *texUnit )
{
   int i, nr = nr_args(texUnit->_CurrentCombine->ModeRGB);

   for (i = 0 ; i < nr ; i++) {
      if (texUnit->_CurrentCombine->SourceA[i] != texUnit->_CurrentCombine->SourceRGB[i]) 
	 return GL_FALSE;

      switch(texUnit->_CurrentCombine->OperandA[i]) {
      case GL_SRC_ALPHA: 
	 switch(texUnit->_CurrentCombine->OperandRGB[i]) {
	 case GL_SRC_COLOR: 
	 case GL_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      case GL_ONE_MINUS_SRC_ALPHA: 
	 switch(texUnit->_CurrentCombine->OperandRGB[i]) {
	 case GL_ONE_MINUS_SRC_COLOR: 
	 case GL_ONE_MINUS_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      default: 
	 return GL_FALSE;	/* impossible */
      }
   }

   return GL_TRUE;
}


static struct ureg emit_combine( struct texenv_fragment_program *p,
			    struct ureg dest,
			    GLuint mask,
			    GLuint saturate,
			    GLuint unit,
			    GLenum mode,
			    const GLenum *source,
			    const GLenum *operand)
{
   int nr = nr_args(mode);
   struct ureg src[3];
   struct ureg tmp;
   int i;

   for (i = 0; i < nr; i++)
      src[i] = emit_combine_source( p, mask, unit, source[i], operand[i] );

   switch (mode) {
   case GL_REPLACE: 
      if (mask == WRITEMASK_XYZW && !saturate)
	 return src[0];
      else
	 return emit_arith( p, FP_OPCODE_MOV, dest, mask, saturate, src[0], undef, undef );
   case GL_MODULATE: 
      return emit_arith( p, FP_OPCODE_MUL, dest, mask, saturate,
			     src[0], src[1], undef );
   case GL_ADD: 
      return emit_arith( p, FP_OPCODE_ADD, dest, mask, saturate, 
			     src[0], src[1], undef );
   case GL_ADD_SIGNED:
      /* tmp = arg0 + arg1
       * result = tmp + -.5
       */
      tmp = emit_const1f(p, .5);
      tmp = swizzle1(tmp,X);
      emit_arith( p, FP_OPCODE_ADD, dest, mask, 0, src[0], src[1], undef );
      emit_arith( p, FP_OPCODE_SUB, dest, mask, saturate, dest, tmp, undef );
      return dest;
   case GL_INTERPOLATE: 
      /* Arg0 * (Arg2) + Arg1 * (1-Arg2) -- note arguments are reordered:
       */
      return emit_arith( p, FP_OPCODE_LRP, dest, mask, saturate, src[2], src[0], src[1] );

   case GL_SUBTRACT: 
      return emit_arith( p, FP_OPCODE_SUB, dest, mask, saturate, src[0], src[1], undef );

   case GL_DOT3_RGBA:
   case GL_DOT3_RGBA_EXT: 
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGB: {
      struct ureg tmp0 = get_temp( p );
      struct ureg tmp1 = get_temp( p );
      struct ureg neg1 = emit_const1f(p, -1);
      struct ureg two  = emit_const1f(p, 2);

      /* tmp0 = 2*src0 - 1
       * tmp1 = 2*src1 - 1
       *
       * dst = tmp0 dot3 tmp1 
       */
      emit_arith( p, FP_OPCODE_MAD, tmp0, WRITEMASK_XYZW, 0, 
		      two, src[0], neg1);

      if (memcmp(&src[0], &src[1], sizeof(struct ureg)) == 0)
	 tmp1 = tmp0;
      else
	 emit_arith( p, FP_OPCODE_MAD, tmp1, WRITEMASK_XYZW, 0, 
			 two, src[1], neg1);
      emit_arith( p, FP_OPCODE_DP3, dest, mask, saturate, tmp0, tmp1, undef);
      return dest;
   }

   default: 
      return src[0];
   }
}

static struct ureg get_dest( struct texenv_fragment_program *p, int unit )
{
   if (p->ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      return get_temp( p );
   else if (unit != p->last_tex_stage)
      return get_temp( p );
   else
      return make_ureg(PROGRAM_OUTPUT, VERT_ATTRIB_COLOR0);
}
      


static struct ureg emit_texenv( struct texenv_fragment_program *p, int unit )
{
   struct gl_texture_unit *texUnit = &p->ctx->Texture.Unit[unit];
   GLuint saturate = (unit < p->last_tex_stage);
   GLuint rgb_shift, alpha_shift;
   struct ureg out, shift;
   struct ureg dest = get_dest(p, unit);

   if (!texUnit->_ReallyEnabled) {
      return get_source(p, GL_PREVIOUS, 0);
   }

   switch (texUnit->_CurrentCombine->ModeRGB) {
   case GL_DOT3_RGB_EXT:
      alpha_shift = texUnit->_CurrentCombine->ScaleShiftA;
      rgb_shift = 0;
      break;

   case GL_DOT3_RGBA_EXT:
      alpha_shift = 0;
      rgb_shift = 0;
      break;

   default:
      rgb_shift = texUnit->_CurrentCombine->ScaleShiftRGB;
      alpha_shift = texUnit->_CurrentCombine->ScaleShiftA;
      break;
   }


   /* Emit the RGB and A combine ops
    */
   if (texUnit->_CurrentCombine->ModeRGB == texUnit->_CurrentCombine->ModeA && 
       args_match( texUnit )) {
      out = emit_combine( p, dest, WRITEMASK_XYZW, saturate,
			  unit,
			  texUnit->_CurrentCombine->ModeRGB,
			  texUnit->_CurrentCombine->SourceRGB,
			  texUnit->_CurrentCombine->OperandRGB );
   }
   else if (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT ||
	    texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) {

      out = emit_combine( p, dest, WRITEMASK_XYZW, saturate,
			  unit,
			  texUnit->_CurrentCombine->ModeRGB,
			  texUnit->_CurrentCombine->SourceRGB,
			  texUnit->_CurrentCombine->OperandRGB );
   }
   else {
      /* Need to do something to stop from re-emitting identical
       * argument calculations here:
       */
      out = emit_combine( p, dest, WRITEMASK_XYZ, saturate,
			  unit,
			  texUnit->_CurrentCombine->ModeRGB,
			  texUnit->_CurrentCombine->SourceRGB,
			  texUnit->_CurrentCombine->OperandRGB );
      out = emit_combine( p, dest, WRITEMASK_W, saturate,
			  unit,
			  texUnit->_CurrentCombine->ModeA,
			  texUnit->_CurrentCombine->SourceA,
			  texUnit->_CurrentCombine->OperandA );
   }

   /* Deal with the final shift:
    */
   if (alpha_shift || rgb_shift) {
      if (rgb_shift == alpha_shift) {
	 shift = emit_const1f(p, 1<<rgb_shift);
	 shift = swizzle1(shift,X);
      }
      else {
	 shift = emit_const2f(p, 1<<rgb_shift, 1<<alpha_shift);
	 shift = swizzle(shift,X,X,X,Y);
      }
      return emit_arith( p, FP_OPCODE_MUL, dest, WRITEMASK_XYZW, 
			 saturate, out, shift, undef );
   }
   else
      return out;
}

void _mesa_UpdateTexEnvProgram( GLcontext *ctx )
{
   struct texenv_fragment_program p;
   GLuint unit;
   struct ureg cf, out;

   p.ctx = ctx;
   p.prog = &ctx->_TexEnvProgram;

   if (p.prog->Instructions == NULL) {
      p.prog->Instructions = MALLOC(sizeof(struct fp_instruction) * 100);
   }

   p.prog->Base.NumInstructions = 0;
   p.prog->NumTexIndirections = 1;	/* correct? */
   p.prog->NumTexInstructions = 0;
   p.prog->NumAluInstructions = 0;

   memset( p.constant_flags, 0, sizeof(p.constant_flags) );

   p.src_texture = undef;
   p.src_previous = make_ureg(PROGRAM_INPUT, VERT_ATTRIB_COLOR0);
   p.last_tex_stage = 0;

   if (ctx->Texture._EnabledUnits) {
      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits ; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	    p.last_tex_stage = unit;
	 }

      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	    p.src_previous = emit_texenv( &p, unit );
	    p.src_texture = undef;
	    p.temp_flag = 0xffff000;
	    p.temp_flag |= 1 << p.src_previous.idx;
	 }
   }

   cf = get_source( &p, GL_PREVIOUS, 0 );
   out = make_ureg( PROGRAM_OUTPUT, VERT_ATTRIB_COLOR0 );

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
      /* Emit specular add.
       */
      struct ureg s = emit_decl(&p, PROGRAM_INPUT, VERT_ATTRIB_COLOR1);
      emit_arith( &p, FP_OPCODE_ADD, out, WRITEMASK_XYZ, 0, cf, s, undef );
   }
   else if (memcmp(&cf, &out, sizeof(cf)) != 0) {
      /* Will wind up in here if no texture enabled or a couple of
       * other scenarios (GL_REPLACE for instance).
       */
      emit_arith( &p, FP_OPCODE_MOV, out, WRITEMASK_XYZW, 0, cf, undef, undef );
   }

   if (p.prog->NumTexIndirections > ctx->Const.MaxFragmentProgramTexIndirections) 
      program_error(&p, "Exceeded max nr indirect texture lookups");

   if (p.prog->NumTexInstructions > ctx->Const.MaxFragmentProgramTexInstructions)
      program_error(&p, "Exceeded max TEX instructions");

   if (p.prog->NumAluInstructions > ctx->Const.MaxFragmentProgramAluInstructions)
      program_error(&p, "Exceeded max ALU instructions");

#if DISASSEM
   _mesa_debug_fp_inst(p.prog->NumTexInstructions + p.prog->NumAluInstructions,
		       p.prog->Instructions);
   _mesa_printf("\n");
#endif
}


