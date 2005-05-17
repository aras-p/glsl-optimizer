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

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "texenvprogram.h"

#include "shader/program.h"
#include "shader/nvfragprog.h"
#include "shader/arbfragparse.h"


#define DISASSEM (MESA_VERBOSE & VERBOSE_DISASSEM)

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

/* State used to build the fragment program:
 */
struct texenv_fragment_program {
   struct fragment_program *program;
   GLcontext *ctx;

   GLuint alu_temps;		/* Track texture indirections, see spec. */
   GLuint temps_output;		/* Track texture indirections, see spec. */

   GLuint temp_in_use;		/* Tracks temporary regs which are in
				 * use.
				 */


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
   int bit;
   
   /* First try and reuse temps which have been used already:
    */
   bit = ffs( ~p->temp_in_use & p->alu_temps );

   /* Then any unused temporary:
    */
   if (!bit)
      bit = ffs( ~p->temp_in_use );

   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}

static struct ureg get_tex_temp( struct texenv_fragment_program *p )
{
   int bit;
   
   /* First try to find availble temp not previously used (to avoid
    * starting a new texture indirection).  According to the spec, the
    * ~p->temps_output isn't necessary, but will keep it there for
    * now:
    */
   bit = ffs( ~p->temp_in_use & ~p->alu_temps & ~p->temps_output );

   /* Then any unused temporary:
    */
   if (!bit) 
      bit = ffs( ~p->temp_in_use );

   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}


static void release_temps( struct texenv_fragment_program *p )
{
   GLuint max_temp = p->ctx->Const.MaxFragmentProgramTemps;

   if (max_temp >= sizeof(int) * 8)
      p->temp_in_use = 0;
   else
      p->temp_in_use = ~((1<<max_temp)-1);
}


static struct ureg register_param6( struct texenv_fragment_program *p, 
				   GLint s0,
				   GLint s1,
				   GLint s2,
				   GLint s3,
				   GLint s4,
				   GLint s5)
{
   GLint tokens[6];
   GLuint idx;
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   tokens[4] = s4;
   tokens[5] = s5;
   idx = _mesa_add_state_reference( p->program->Parameters, tokens );
   return make_ureg(PROGRAM_STATE_VAR, idx);
}


#define register_param1(p,s0)          register_param6(p,s0,0,0,0,0,0)
#define register_param2(p,s0,s1)       register_param6(p,s0,s1,0,0,0,0)
#define register_param3(p,s0,s1,s2)    register_param6(p,s0,s1,s2,0,0,0)
#define register_param4(p,s0,s1,s2,s3) register_param6(p,s0,s1,s2,s3,0,0)


static struct ureg register_input( struct texenv_fragment_program *p, GLuint input )
{
   p->program->InputsRead |= (1<<input);
   return make_ureg(PROGRAM_INPUT, input);
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
   GLuint nr = p->program->Base.NumInstructions++;
   struct fp_instruction *inst = &p->program->Instructions[nr];
      
   memset(inst, 0, sizeof(*inst));
   inst->Opcode = op;
   
   emit_arg( &inst->SrcReg[0], src0 );
   emit_arg( &inst->SrcReg[1], src1 );
   emit_arg( &inst->SrcReg[2], src2 );
   
   inst->Saturate = saturate;

   emit_dst( &inst->DstReg, dest, mask );

   /* Accounting for indirection tracking:
    */
   if (dest.file == PROGRAM_TEMPORARY)
      p->temps_output |= 1 << dest.idx;

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
   
   /* Accounting for indirection tracking:
    */
   if (src0.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << src0.idx;

   if (!is_undef(src1) && src1.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << src1.idx;

   if (!is_undef(src2) && src2.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << src2.idx;

   if (dest.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << dest.idx;
       
   p->program->NumAluInstructions++;
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

   p->program->NumTexInstructions++;

   /* Is this a texture indirection?
    */
   if ((coord.file == PROGRAM_TEMPORARY &&
	(p->temps_output & (1<<coord.idx))) ||
       (dest.file == PROGRAM_TEMPORARY &&
	(p->alu_temps & (1<<dest.idx)))) {
      p->program->NumTexIndirections++;
      p->temps_output = 0;
      p->alu_temps = 0;
   }

   return dest;
}


static struct ureg register_const4f( struct texenv_fragment_program *p, 
			      GLfloat s0,
			      GLfloat s1,
			      GLfloat s2,
			      GLfloat s3)
{
   GLfloat values[4];
   GLuint idx;
   values[0] = s0;
   values[1] = s1;
   values[2] = s2;
   values[3] = s3;
   idx = _mesa_add_unnamed_constant( p->program->Parameters, values );
   return make_ureg(PROGRAM_STATE_VAR, idx);
}

#define register_scalar_const(p, s0)    register_const4f(p, s0, s0, s0, s0)
#define register_const1f(p, s0)         register_const4f(p, s0, 0, 0, 1)
#define register_const2f(p, s0, s1)     register_const4f(p, s0, s1, 0, 1)
#define register_const3f(p, s0, s1, s2) register_const4f(p, s0, s1, s2, 1)








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
	 struct ureg texcoord = register_input(p, FRAG_ATTRIB_TEX0+unit);
	 struct ureg tmp = get_tex_temp( p );

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
      return register_param2(p, STATE_TEXENV_COLOR, unit);
   case GL_PRIMARY_COLOR:
      return register_input(p, FRAG_ATTRIB_COL0);
   case GL_PREVIOUS:
   default: 
      if (is_undef(p->src_previous))
	 return register_input(p, FRAG_ATTRIB_COL0);
      else
	 return p->src_previous;
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
      one = register_scalar_const(p, 1.0);
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
      one = register_scalar_const(p, 1.0);
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
   struct ureg tmp, half;
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
       * result = tmp - .5
       */
      half = register_scalar_const(p, .5);
      emit_arith( p, FP_OPCODE_ADD, tmp, mask, 0, src[0], src[1], undef );
      emit_arith( p, FP_OPCODE_SUB, dest, mask, saturate, tmp, half, undef );
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
      struct ureg neg1 = register_scalar_const(p, -1);
      struct ureg two  = register_scalar_const(p, 2);

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



static struct ureg emit_texenv( struct texenv_fragment_program *p, int unit )
{
   struct gl_texture_unit *texUnit = &p->ctx->Texture.Unit[unit];
   GLuint saturate = (unit < p->last_tex_stage);
   GLuint rgb_shift, alpha_shift;
   struct ureg out, shift;
   struct ureg dest;

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

   /* If this is the very last calculation, emit direct to output reg:
    */
   if ((p->ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) ||
       unit != p->last_tex_stage ||
       alpha_shift ||
       rgb_shift)
      dest = get_temp( p );
   else
      dest = make_ureg(PROGRAM_OUTPUT, FRAG_OUTPUT_COLR);

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
	 shift = register_scalar_const(p, 1<<rgb_shift);
      }
      else {
	 shift = register_const2f(p, 1<<rgb_shift, 1<<alpha_shift);
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
   GLuint db_NumInstructions = 0;
   struct fp_instruction *db_Instructions = NULL;

   if (ctx->FragmentProgram._Enabled)
      return;

   if (!ctx->_TexEnvProgram)
      ctx->FragmentProgram._Current = ctx->_TexEnvProgram = 
	 (struct fragment_program *) 
	 ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);

   _mesa_memset(&p, 0, sizeof(p));
   p.ctx = ctx;
   p.program = ctx->_TexEnvProgram;

   if (ctx->Driver.ProgramStringNotify || DISASSEM) {
      db_Instructions = p.program->Instructions;
      db_NumInstructions = p.program->Base.NumInstructions;
      p.program->Instructions = NULL;
   }

   if (!p.program->Instructions)
      p.program->Instructions = MALLOC(sizeof(struct fp_instruction) * 100);

   p.program->Base.NumInstructions = 0;
   p.program->Base.Target = GL_FRAGMENT_PROGRAM_ARB;
   p.program->NumTexIndirections = 1;	/* correct? */
   p.program->NumTexInstructions = 0;
   p.program->NumAluInstructions = 0;
   p.program->Base.String = 0;
   p.program->Base.NumInstructions =
   p.program->Base.NumTemporaries =
   p.program->Base.NumParameters =
   p.program->Base.NumAttributes = p.program->Base.NumAddressRegs = 0;
   
   if (p.program->Parameters)
      _mesa_free_parameters(p.program->Parameters);
   else
      p.program->Parameters = _mesa_new_parameter_list();

   p.program->InputsRead = 0;
   p.program->OutputsWritten = 1 << FRAG_OUTPUT_COLR;

   p.src_texture = undef;
   p.src_previous = undef;
   p.last_tex_stage = 0;
   release_temps(&p);

   if (ctx->Texture._EnabledUnits) {
      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits ; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	    p.last_tex_stage = unit;
	 }

      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	    p.src_previous = emit_texenv( &p, unit );
	    p.src_texture = undef;
	    release_temps(&p);	/* release all temps */
	    if (p.src_previous.file == PROGRAM_TEMPORARY)
	       p.temp_in_use |= 1 << p.src_previous.idx; /* except for this one */
	 }
   }

   cf = get_source( &p, GL_PREVIOUS, 0 );
   out = make_ureg( PROGRAM_OUTPUT, FRAG_OUTPUT_COLR );

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
      /* Emit specular add.
       */
      struct ureg s = register_input(&p, FRAG_ATTRIB_COL1);
      emit_arith( &p, FP_OPCODE_ADD, out, WRITEMASK_XYZ, 0, cf, s, undef );
   }
   else if (memcmp(&cf, &out, sizeof(cf)) != 0) {
      /* Will wind up in here if no texture enabled or a couple of
       * other scenarios (GL_REPLACE for instance).
       */
      emit_arith( &p, FP_OPCODE_MOV, out, WRITEMASK_XYZW, 0, cf, undef, undef );
   }

   /* Finish up:
    */
   emit_arith( &p, FP_OPCODE_END, undef, WRITEMASK_XYZW, 0, undef, undef, undef);

   if (ctx->Fog.Enabled)
      p.program->FogOption = ctx->Fog.Mode;
   else
      p.program->FogOption = GL_NONE;

   if (p.program->NumTexIndirections > ctx->Const.MaxFragmentProgramTexIndirections) 
      program_error(&p, "Exceeded max nr indirect texture lookups");

   if (p.program->NumTexInstructions > ctx->Const.MaxFragmentProgramTexInstructions)
      program_error(&p, "Exceeded max TEX instructions");

   if (p.program->NumAluInstructions > ctx->Const.MaxFragmentProgramAluInstructions)
      program_error(&p, "Exceeded max ALU instructions");


   /* Notify driver the fragment program has (actually) changed.
    */
   if (ctx->Driver.ProgramStringNotify || DISASSEM) {
      if (db_Instructions == NULL ||
	  db_NumInstructions != p.program->Base.NumInstructions ||
	  memcmp(db_Instructions, p.program->Instructions, 
		 db_NumInstructions * sizeof(*db_Instructions)) != 0) {
	 
	 if (ctx->Driver.ProgramStringNotify)
	    ctx->Driver.ProgramStringNotify( ctx, GL_FRAGMENT_PROGRAM_ARB, 
					     &p.program->Base );

	 if (DISASSEM) {
	    _mesa_debug_fp_inst(p.program->NumTexInstructions + p.program->NumAluInstructions,
				p.program->Instructions);
	    _mesa_printf("\n");
	 }
      }
      
      FREE(db_Instructions);
   }
}


