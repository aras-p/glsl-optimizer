/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Tungsten Graphics   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file t_vp_build.c
 * Create a vertex program to execute the current fixed function T&L pipeline.
 * \author Keith Whitwell
 */


#include <strings.h>

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "t_context.h"
#include "t_vp_build.h"

#include "shader/program.h"
#include "shader/nvvertprog.h"
#include "shader/arbvertparse.h"


/* Very useful debugging tool - produces annotated listing of
 * generated program with line/function references for each
 * instruction back into this file:
 */
#define DISASSEM 0

#define MAX_INSN 200

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
   GLuint negate:1;
   GLuint swz:12;
   GLuint pad:7;
};


struct tnl_program {
   GLcontext *ctx;
   struct vertex_program *program;
   
   GLuint temp_flag;
   GLuint temp_reserved;
   
   struct ureg eye_position;
   struct ureg eye_position_normalized;
   struct ureg eye_normal;
   struct ureg identity;

   GLuint materials;
   GLuint color_materials;
};


const static struct ureg undef = { 
   ~0,
   ~0,
   0,
   0,
   0
};

/* Local shorthand:
 */
#define X    SWIZZLE_X
#define Y    SWIZZLE_Y
#define Z    SWIZZLE_Z
#define W    SWIZZLE_W


/* Construct a ureg:
 */
static struct ureg make_ureg(GLuint file, GLuint idx)
{
   struct ureg reg;
   reg.file = file;
   reg.idx = idx;
   reg.negate = 0;
   reg.swz = SWIZZLE_NOOP;
   reg.pad = 0;
   return reg;
}



static struct ureg negate( struct ureg reg )
{
   reg.negate ^= 1;
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

static struct ureg get_temp( struct tnl_program *p )
{
   int bit = ffs( ~p->temp_flag );
   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   p->temp_flag |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, bit-1);
}

static struct ureg reserve_temp( struct tnl_program *p )
{
   struct ureg temp = get_temp( p );
   p->temp_reserved |= 1<<temp.idx;
   return temp;
}

static void release_temp( struct tnl_program *p, struct ureg reg )
{
   if (reg.file == PROGRAM_TEMPORARY) {
      p->temp_flag &= ~(1<<reg.idx);
      p->temp_flag |= p->temp_reserved; /* can't release reserved temps */
   }
}

static void release_temps( struct tnl_program *p )
{
   p->temp_flag = p->temp_reserved;
}



static struct ureg register_input( struct tnl_program *p, GLuint input )
{
   p->program->InputsRead |= (1<<input);
   return make_ureg(PROGRAM_INPUT, input);
}

static struct ureg register_output( struct tnl_program *p, GLuint output )
{
   p->program->OutputsWritten |= (1<<output);
   return make_ureg(PROGRAM_OUTPUT, output);
}

static struct ureg register_const4f( struct tnl_program *p, 
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

#define register_const1f(p, s0)         register_const4f(p, s0, 0, 0, 1)
#define register_const2f(p, s0, s1)     register_const4f(p, s0, s1, 0, 1)
#define register_const3f(p, s0, s1, s2) register_const4f(p, s0, s1, s2, 1)

static GLboolean is_undef( struct ureg reg )
{
   return reg.file == 0xf;
}

static struct ureg get_identity_param( struct tnl_program *p )
{
   if (is_undef(p->identity)) 
      p->identity = register_const4f(p, 0,0,0,1);

   return p->identity;
}

static struct ureg register_param6( struct tnl_program *p, 
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


static void register_matrix_param6( struct tnl_program *p,
				    GLint s0,
				    GLint s1,
				    GLint s2,
				    GLint s3,
				    GLint s4,
				    GLint s5,
				    struct ureg *matrix )
{
   GLuint i;

   /* This is a bit sad as the support is there to pull the whole
    * matrix out in one go:
    */
   for (i = 0; i <= s4 - s3; i++) 
      matrix[i] = register_param6( p, s0, s1, s2, i, i, s5 );
}


static void emit_arg( struct vp_src_register *src,
		      struct ureg reg )
{
   src->File = reg.file;
   src->Index = reg.idx;
   src->Swizzle = reg.swz;
   src->Negate = reg.negate;
   src->RelAddr = 0;
   src->pad = 0;
}

static void emit_dst( struct vp_dst_register *dst,
		      struct ureg reg, GLuint mask )
{
   dst->File = reg.file;
   dst->Index = reg.idx;
   /* allow zero as a shorthand for xyzw */
   dst->WriteMask = mask ? mask : WRITEMASK_XYZW; 
   dst->pad = 0;
}

static void debug_insn( struct vp_instruction *inst, const char *fn,
			GLuint line )
{
#if DISASSEM
   static const char *last_fn;
   
   if (fn != last_fn) {
      last_fn = fn;
      _mesa_printf("%s:\n", fn);
   }
	 
   _mesa_printf("%d:\t", line);
   _mesa_debug_vp_inst(1, inst);
#endif
}


static void emit_op3fn(struct tnl_program *p,
		       GLuint op,
		       struct ureg dest,
		       GLuint mask,
		       struct ureg src0,
		       struct ureg src1,
		       struct ureg src2,
		       const char *fn,
		       GLuint line)
{
   GLuint nr = p->program->Base.NumInstructions++;
   struct vp_instruction *inst = &p->program->Instructions[nr];
      
   if (p->program->Base.NumInstructions > MAX_INSN) {
      _mesa_problem(p->ctx, "Out of instructions in emit_op3fn\n");
      return;
   }
      
   inst->Opcode = op; 
   inst->StringPos = 0;
   inst->Data = 0;
   
   emit_arg( &inst->SrcReg[0], src0 );
   emit_arg( &inst->SrcReg[1], src1 );
   emit_arg( &inst->SrcReg[2], src2 );   

   emit_dst( &inst->DstReg, dest, mask );

   debug_insn(inst, fn, line);
}

   

#define emit_op3(p, op, dst, mask, src0, src1, src2) \
   emit_op3fn(p, op, dst, mask, src0, src1, src2, __FUNCTION__, __LINE__)

#define emit_op2(p, op, dst, mask, src0, src1) \
    emit_op3fn(p, op, dst, mask, src0, src1, undef, __FUNCTION__, __LINE__)

#define emit_op1(p, op, dst, mask, src0) \
    emit_op3fn(p, op, dst, mask, src0, undef, undef, __FUNCTION__, __LINE__)


static struct ureg make_temp( struct tnl_program *p, struct ureg reg )
{
   if (reg.file == PROGRAM_TEMPORARY && 
       !(p->temp_reserved & (1<<reg.idx)))
      return reg;
   else {
      struct ureg temp = get_temp(p);
      emit_op1(p, VP_OPCODE_MOV, temp, 0, reg);
      return temp;
   }
}


/* Currently no tracking performed of input/output/register size or
 * active elements.  Could be used to reduce these operations, as
 * could the matrix type.
 */
static void emit_matrix_transform_vec4( struct tnl_program *p,
					struct ureg dest,
					const struct ureg *mat,
					struct ureg src)
{
   emit_op2(p, VP_OPCODE_DP4, dest, WRITEMASK_X, src, mat[0]);
   emit_op2(p, VP_OPCODE_DP4, dest, WRITEMASK_Y, src, mat[1]);
   emit_op2(p, VP_OPCODE_DP4, dest, WRITEMASK_Z, src, mat[2]);
   emit_op2(p, VP_OPCODE_DP4, dest, WRITEMASK_W, src, mat[3]);
}

/* This version is much easier to implement if writemasks are not
 * supported natively on the target or (like SSE), the target doesn't
 * have a clean/obvious dotproduct implementation.
 */
static void emit_transpose_matrix_transform_vec4( struct tnl_program *p,
						  struct ureg dest,
						  const struct ureg *mat,
						  struct ureg src)
{
   struct ureg tmp;

   if (dest.file != PROGRAM_TEMPORARY)
      tmp = get_temp(p);
   else
      tmp = dest;

   emit_op2(p, VP_OPCODE_MUL, tmp, 0, swizzle1(src,X), mat[0]);
   emit_op3(p, VP_OPCODE_MAD, tmp, 0, swizzle1(src,Y), mat[1], tmp);
   emit_op3(p, VP_OPCODE_MAD, tmp, 0, swizzle1(src,Z), mat[2], tmp);
   emit_op3(p, VP_OPCODE_MAD, dest, 0, swizzle1(src,W), mat[3], tmp);

   if (dest.file != PROGRAM_TEMPORARY)
      release_temp(p, tmp);
}

static void emit_matrix_transform_vec3( struct tnl_program *p,
					struct ureg dest,
					const struct ureg *mat,
					struct ureg src)
{
   emit_op2(p, VP_OPCODE_DP3, dest, WRITEMASK_X, src, mat[0]);
   emit_op2(p, VP_OPCODE_DP3, dest, WRITEMASK_Y, src, mat[1]);
   emit_op2(p, VP_OPCODE_DP3, dest, WRITEMASK_Z, src, mat[2]);
}


static void emit_normalize_vec3( struct tnl_program *p,
				 struct ureg dest,
				 struct ureg src )
{
   struct ureg tmp = get_temp(p);
   emit_op2(p, VP_OPCODE_DP3, tmp, 0, src, src);
   emit_op1(p, VP_OPCODE_RSQ, tmp, 0, tmp);
   emit_op2(p, VP_OPCODE_MUL, dest, 0, src, tmp);
   release_temp(p, tmp);
}

static struct ureg get_eye_position( struct tnl_program *p )
{
   if (is_undef(p->eye_position)) {
      struct ureg pos = register_input( p, VERT_ATTRIB_POS ); 
      struct ureg modelview[4];

      register_matrix_param6( p, STATE_MATRIX, STATE_MODELVIEW, 0, 0, 3, 
			      STATE_MATRIX_TRANSPOSE, modelview );
      p->eye_position = reserve_temp(p);

      emit_transpose_matrix_transform_vec4(p, p->eye_position, modelview, pos);
   }
   
   return p->eye_position;
}


static struct ureg get_eye_position_normalized( struct tnl_program *p )
{
   if (is_undef(p->eye_position_normalized)) {
      struct ureg eye = get_eye_position(p);
      p->eye_position_normalized = reserve_temp(p);
      emit_normalize_vec3(p, p->eye_position_normalized, eye);
   }
   
   return p->eye_position_normalized;
}


static struct ureg get_eye_normal( struct tnl_program *p )
{
   if (is_undef(p->eye_normal)) {
      struct ureg normal = register_input(p, VERT_ATTRIB_NORMAL );
      struct ureg mvinv[3];

      register_matrix_param6( p, STATE_MATRIX, STATE_MODELVIEW, 0, 0, 2,
			      STATE_MATRIX_INVTRANS, mvinv );

      p->eye_normal = reserve_temp(p);

      /* Transform to eye space:
       */
      emit_matrix_transform_vec3( p, p->eye_normal, mvinv, normal );

      /* Normalize/Rescale:
       */
      if (p->ctx->Transform.Normalize) {
	 emit_normalize_vec3( p, p->eye_normal, p->eye_normal );
      }
      else if (p->ctx->Transform.RescaleNormals) {
	 struct ureg rescale = register_param2(p, STATE_INTERNAL,
					       STATE_NORMAL_SCALE);

	 emit_op2( p, VP_OPCODE_MUL, p->eye_normal, 0, normal, 
		   swizzle1(rescale, X));
      }
   }

   return p->eye_normal;
}



static void build_hpos( struct tnl_program *p )
{
   struct ureg pos = register_input( p, VERT_ATTRIB_POS ); 
   struct ureg hpos = register_output( p, VERT_RESULT_HPOS );
   struct ureg mvp[4];

   register_matrix_param6( p, STATE_MATRIX, STATE_MVP, 0, 0, 3, 
			   STATE_MATRIX_TRANSPOSE, mvp );
   emit_transpose_matrix_transform_vec4( p, hpos, mvp, pos );
}


static GLuint material_attrib( GLuint side, GLuint property )
{
   return (_TNL_ATTRIB_MAT_FRONT_AMBIENT + 
	   (property - STATE_AMBIENT) * 2 + 
	   side);
}

static void set_material_flags( struct tnl_program *p )
{
   GLcontext *ctx = p->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;

   p->color_materials = 0;
   p->materials = 0;

   if (ctx->Light.ColorMaterialEnabled) {
      p->materials = 
	 p->color_materials = 
	 ctx->Light.ColorMaterialBitmask << _TNL_ATTRIB_MAT_FRONT_AMBIENT;
   }

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT ; i < _TNL_ATTRIB_INDEX ; i++) 
      if (tnl->vb.AttribPtr[i]->stride) 
	 p->materials |= 1<<i;
}


static struct ureg get_material( struct tnl_program *p, GLuint side, 
				 GLuint property )
{
   GLuint attrib = material_attrib(side, property);

   if (p->color_materials & (1<<attrib))
      return register_input(p, VERT_ATTRIB_COLOR0);
   else if (p->materials & (1<<attrib)) 
      return register_input( p, attrib );
   else
      return register_param3( p, STATE_MATERIAL, side, property );
}

#define SCENE_COLOR_BITS(side) (( _TNL_BIT_MAT_FRONT_EMISSION | \
				   _TNL_BIT_MAT_FRONT_AMBIENT | \
				   _TNL_BIT_MAT_FRONT_DIFFUSE) << (side))

/* Either return a precalculated constant value or emit code to
 * calculate these values dynamically in the case where material calls
 * are present between begin/end pairs.
 *
 * Probably want to shift this to the program compilation phase - if
 * we always emitted the calculation here, a smart compiler could
 * detect that it was constant (given a certain set of inputs), and
 * lift it out of the main loop.  That way the programs created here
 * would be independent of the vertex_buffer details.
 */
static struct ureg get_scenecolor( struct tnl_program *p, GLuint side )
{
   if (p->materials & SCENE_COLOR_BITS(side)) {
      struct ureg lm_ambient = register_param1(p, STATE_LIGHTMODEL_AMBIENT);
      struct ureg material_emission = get_material(p, side, STATE_EMISSION);
      struct ureg material_ambient = get_material(p, side, STATE_AMBIENT);
      struct ureg material_diffuse = get_material(p, side, STATE_DIFFUSE);
      struct ureg tmp = make_temp(p, material_diffuse);
      emit_op3(p, VP_OPCODE_MAD, tmp,  WRITEMASK_XYZ, lm_ambient, 
	       material_ambient, material_emission);
      return tmp;
   }
   else
      return register_param2( p, STATE_LIGHTMODEL_SCENECOLOR, side );
}


static struct ureg get_lightprod( struct tnl_program *p, GLuint light, 
				  GLuint side, GLuint property )
{
   GLuint attrib = material_attrib(side, property);
   if (p->materials & (1<<attrib)) {
      struct ureg light_value = 
	 register_param3(p, STATE_LIGHT, light, property);
      struct ureg material_value = get_material(p, side, property);
      struct ureg tmp = get_temp(p);
      emit_op2(p, VP_OPCODE_MUL, tmp,  0, light_value, material_value);
      return tmp;
   }
   else
      return register_param4(p, STATE_LIGHTPROD, light, side, property);
}

static struct ureg calculate_light_attenuation( struct tnl_program *p,
						GLuint i, 
						struct gl_light *light,
						struct ureg VPpli,
						struct ureg dist )
{
   struct ureg attenuation = register_param3(p, STATE_LIGHT, i,
					     STATE_ATTENUATION);
   struct ureg att = get_temp(p);

   /* Calculate spot attenuation:
    */
   if (light->SpotCutoff != 180.0F) {
      struct ureg spot_dir = register_param3(p, STATE_LIGHT, i,
					     STATE_SPOT_DIRECTION);
      struct ureg spot = get_temp(p);
      struct ureg slt = get_temp(p);
	       
      emit_normalize_vec3( p, spot, spot_dir ); /* XXX: precompute! */
      emit_op2(p, VP_OPCODE_DP3, spot, 0, negate(VPpli), spot_dir);
      emit_op2(p, VP_OPCODE_SLT, slt, 0, swizzle1(spot_dir,W), spot);
      emit_op2(p, VP_OPCODE_POW, spot, 0, spot, swizzle1(attenuation, W));
      emit_op2(p, VP_OPCODE_MUL, att, 0, slt, spot);

      release_temp(p, spot);
      release_temp(p, slt);
   }

   /* Calculate distance attenuation:
    */
   if (light->ConstantAttenuation != 1.0 ||
       light->LinearAttenuation != 1.0 ||
       light->QuadraticAttenuation != 1.0) {

      /* 1/d,d,d,1/d */
      emit_op1(p, VP_OPCODE_RCP, dist, WRITEMASK_YZ, dist); 
      /* 1,d,d*d,1/d */
      emit_op2(p, VP_OPCODE_MUL, dist, WRITEMASK_XZ, dist, swizzle1(dist,Y)); 
      /* 1/dist-atten */
      emit_op2(p, VP_OPCODE_DP3, dist, 0, attenuation, dist); 

      if (light->SpotCutoff != 180.0F) {
	 /* dist-atten */
	 emit_op1(p, VP_OPCODE_RCP, dist, 0, dist); 
	 /* spot-atten * dist-atten */
	 emit_op2(p, VP_OPCODE_MUL, att, 0, dist, att);	
      } else {
	 /* dist-atten */
	 emit_op1(p, VP_OPCODE_RCP, att, 0, dist); 
      }
   }

   return att;
}
						




/* Need to add some addtional parameters to allow lighting in object
 * space - STATE_SPOT_DIRECTION and STATE_HALF implicitly assume eye
 * space lighting.
 */
static void build_lighting( struct tnl_program *p )
{
   GLcontext *ctx = p->ctx;
   const GLboolean twoside = ctx->Light.Model.TwoSide;
   const GLboolean separate = (ctx->Light.Model.ColorControl ==
			       GL_SEPARATE_SPECULAR_COLOR);
   GLuint nr_lights = 0, count = 0;
   struct ureg normal = get_eye_normal(p);
   struct ureg lit = get_temp(p);
   struct ureg dots = get_temp(p);
   struct ureg _col0 = undef, _col1 = undef;
   struct ureg _bfc0 = undef, _bfc1 = undef;
   GLuint i;

   for (i = 0; i < MAX_LIGHTS; i++) 
      if (ctx->Light.Light[i].Enabled)
	 nr_lights++;
   
   set_material_flags(p);

   {
      struct ureg shininess = get_material(p, 0, STATE_SHININESS);
      emit_op1(p, VP_OPCODE_MOV, dots,  WRITEMASK_W, swizzle1(shininess,X));
      release_temp(p, shininess);

      _col0 = make_temp(p, get_scenecolor(p, 0));
      if (separate)
	 _col1 = make_temp(p, get_identity_param(p));
      else
	 _col1 = _col0;

   }

   if (twoside) {
      struct ureg shininess = get_material(p, 1, STATE_SHININESS);
      emit_op1(p, VP_OPCODE_MOV, dots, WRITEMASK_Z, 
	       negate(swizzle1(shininess,X)));
      release_temp(p, shininess);

      _bfc0 = make_temp(p, get_scenecolor(p, 1));
      if (separate)
	 _bfc1 = make_temp(p, get_identity_param(p));
      else
	 _bfc1 = _bfc0;
   }

   for (i = 0; i < MAX_LIGHTS; i++) {
      struct gl_light *light = &ctx->Light.Light[i];

      if (light->Enabled) {
	 struct ureg half = undef;
	 struct ureg att = undef, VPpli = undef;
	  
	 count++;

	 if (light->EyePosition[3] == 0) {
	    /* Can used precomputed constants in this case.
	     * Attenuation never applies to infinite lights.
	     */
	    VPpli = register_param3(p, STATE_LIGHT, i, 
				    STATE_POSITION_NORMALIZED); 
	    half = register_param3(p, STATE_LIGHT, i, STATE_HALF);
	 } 
	 else {
	    struct ureg Ppli = register_param3(p, STATE_LIGHT, i, 
					       STATE_POSITION); 
	    struct ureg V = get_eye_position(p);
	    struct ureg dist = get_temp(p);

	    VPpli = get_temp(p); 
	    half = get_temp(p);
 
	    /* Calulate VPpli vector
	     */
	    emit_op2(p, VP_OPCODE_SUB, VPpli, 0, Ppli, V); 

	    /* Normalize VPpli.  The dist value also used in
	     * attenuation below.
	     */
	    emit_op2(p, VP_OPCODE_DP3, dist, 0, VPpli, VPpli);
	    emit_op1(p, VP_OPCODE_RSQ, dist, 0, dist);
	    emit_op2(p, VP_OPCODE_MUL, VPpli, 0, VPpli, dist);


	    /* Calculate  attenuation:
	     */ 
	    if (light->SpotCutoff != 180.0 ||
		light->ConstantAttenuation != 1.0 ||
		light->LinearAttenuation != 1.0 ||
		light->QuadraticAttenuation != 1.0) {
	       att = calculate_light_attenuation(p, i, light, VPpli, dist);
	    }
	 
      
	    /* Calculate viewer direction, or use infinite viewer:
	     */
	    if (ctx->Light.Model.LocalViewer) {
	       struct ureg eye_hat = get_eye_position_normalized(p);
	       emit_op2(p, VP_OPCODE_SUB, half, 0, VPpli, eye_hat);
	    }
	    else {
	       struct ureg z_dir = swizzle(get_identity_param(p),X,Y,W,Z); 
	       emit_op2(p, VP_OPCODE_ADD, half, 0, VPpli, z_dir);
	    }

	    emit_normalize_vec3(p, half, half);

	    release_temp(p, dist);
	 }

	 /* Calculate dot products:
	  */
	 emit_op2(p, VP_OPCODE_DP3, dots, WRITEMASK_X, normal, VPpli);
	 emit_op2(p, VP_OPCODE_DP3, dots, WRITEMASK_Y, normal, half);

	
	 /* Front face lighting:
	  */
	 {
	    struct ureg ambient = get_lightprod(p, i, 0, STATE_AMBIENT);
	    struct ureg diffuse = get_lightprod(p, i, 0, STATE_DIFFUSE);
	    struct ureg specular = get_lightprod(p, i, 0, STATE_SPECULAR);
	    struct ureg res0, res1;

	    emit_op1(p, VP_OPCODE_LIT, lit, 0, dots);
   
	    if (!is_undef(att)) 
	       emit_op2(p, VP_OPCODE_MUL, lit, 0, lit, att);
   
	    
	    if (count == nr_lights) {
	       if (separate) {
		  res0 = register_output( p, VERT_RESULT_COL0 );
		  res1 = register_output( p, VERT_RESULT_COL1 );
	       }
	       else {
		  res0 = _col0;
		  res1 = register_output( p, VERT_RESULT_COL0 );
	       }
	    } else {
	       res0 = _col0;
	       res1 = _col1;
	    }

	    emit_op3(p, VP_OPCODE_MAD, _col0, 0, swizzle1(lit,X), ambient, _col0);
	    emit_op3(p, VP_OPCODE_MAD, res0, 0, swizzle1(lit,Y), diffuse, _col0);
	    emit_op3(p, VP_OPCODE_MAD, res1, 0, swizzle1(lit,Z), specular, _col1);
      
	    release_temp(p, ambient);
	    release_temp(p, diffuse);
	    release_temp(p, specular);
	 }

	 /* Back face lighting:
	  */
	 if (twoside) {
	    struct ureg ambient = get_lightprod(p, i, 1, STATE_AMBIENT);
	    struct ureg diffuse = get_lightprod(p, i, 1, STATE_DIFFUSE);
	    struct ureg specular = get_lightprod(p, i, 1, STATE_SPECULAR);
	    struct ureg res0, res1;
	       
	    emit_op1(p, VP_OPCODE_LIT, lit, 0, negate(swizzle(dots,X,Y,W,Z)));

	    if (!is_undef(att)) 
	       emit_op2(p, VP_OPCODE_MUL, lit, 0, lit, att);

	    if (count == nr_lights) {
	       if (separate) {
		  res0 = register_output( p, VERT_RESULT_BFC0 );
		  res1 = register_output( p, VERT_RESULT_BFC1 );
	       }
	       else {
		  res0 = _bfc0;
		  res1 = register_output( p, VERT_RESULT_BFC0 );
	       }
	    } else {
	       res0 = _bfc0;
	       res1 = _bfc1;
	    }


	    emit_op3(p, VP_OPCODE_MAD, _bfc0, 0, swizzle1(lit,X), ambient, _bfc0);
	    emit_op3(p, VP_OPCODE_MAD, res0, 0, swizzle1(lit,Y), diffuse, _bfc0);
	    emit_op3(p, VP_OPCODE_MAD, res1, 0, swizzle1(lit,Z), specular, _bfc1);

	    release_temp(p, ambient);
	    release_temp(p, diffuse);
	    release_temp(p, specular);
	 }

	 release_temp(p, half);
	 release_temp(p, VPpli);
	 release_temp(p, att);
      }
   }

   release_temps( p );
}


static void build_fog( struct tnl_program *p )
{
   GLcontext *ctx = p->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct ureg fog = register_output(p, VERT_RESULT_FOGC);
   struct ureg input;
   
   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT) {
      input = swizzle1(get_eye_position(p), Z);
   }
   else {
      input = swizzle1(register_input(p, VERT_ATTRIB_FOG), X);
   }

   if (tnl->_DoVertexFog) {
      struct ureg params = register_param1(p, STATE_FOG_PARAMS);
      struct ureg tmp = get_temp(p);

      switch (ctx->Fog.Mode) {
      case GL_LINEAR: {
	 struct ureg id = get_identity_param(p);
	 emit_op2(p, VP_OPCODE_SUB, tmp, 0, swizzle1(params,Z), input); 
	 emit_op2(p, VP_OPCODE_MUL, tmp, 0, tmp, swizzle1(params,W)); 
	 emit_op2(p, VP_OPCODE_MAX, tmp, 0, tmp, swizzle1(id,X)); /* saturate */
	 emit_op2(p, VP_OPCODE_MIN, fog, WRITEMASK_X, tmp, swizzle1(id,W));
	 break;
      }
      case GL_EXP:
	 emit_op1(p, VP_OPCODE_ABS, tmp, 0, input); 
	 emit_op2(p, VP_OPCODE_MUL, tmp, 0, tmp, swizzle1(params,X)); 
	 emit_op2(p, VP_OPCODE_POW, fog, WRITEMASK_X, 
		  register_const1f(p, M_E), negate(tmp)); 
	 break;
      case GL_EXP2:
	 emit_op2(p, VP_OPCODE_MUL, tmp, 0, input, swizzle1(params,X)); 
	 emit_op2(p, VP_OPCODE_MUL, tmp, 0, tmp, tmp); 
	 emit_op2(p, VP_OPCODE_POW, fog, WRITEMASK_X, 
		  register_const1f(p, M_E), negate(tmp)); 
	 break;
      }
      
      release_temp(p, tmp);
   }
   else {
      /* results = incoming fog coords (compute fog per-fragment later) 
       *
       * KW:  Is it really necessary to do anything in this case?
       */
      emit_op1(p, VP_OPCODE_MOV, fog, WRITEMASK_X, input);
   }
}
 
static void build_reflect_texgen( struct tnl_program *p,
				  struct ureg dest,
				  GLuint writemask )
{
   struct ureg normal = get_eye_normal(p);
   struct ureg eye_hat = get_eye_position_normalized(p);
   struct ureg tmp = get_temp(p);

   /* n.u */
   emit_op2(p, VP_OPCODE_DP3, tmp, 0, normal, eye_hat); 
   /* 2n.u */
   emit_op2(p, VP_OPCODE_ADD, tmp, 0, tmp, tmp); 
   /* (-2n.u)n + u */
   emit_op3(p, VP_OPCODE_MAD, dest, writemask, negate(tmp), normal, eye_hat);
}

static void build_sphere_texgen( struct tnl_program *p,
				 struct ureg dest,
				 GLuint writemask )
{
   struct ureg normal = get_eye_normal(p);
   struct ureg eye_hat = get_eye_position_normalized(p);
   struct ureg tmp = get_temp(p);
   struct ureg half = register_const1f(p, .5);
   struct ureg r = get_temp(p);
   struct ureg inv_m = get_temp(p);
   struct ureg id = get_identity_param(p);

   /* Could share the above calculations, but it would be
    * a fairly odd state for someone to set (both sphere and
    * reflection active for different texture coordinate
    * components.  Of course - if two texture units enable
    * reflect and/or sphere, things start to tilt in favour
    * of seperating this out:
    */

   /* n.u */
   emit_op2(p, VP_OPCODE_DP3, tmp, 0, normal, eye_hat); 
   /* 2n.u */
   emit_op2(p, VP_OPCODE_ADD, tmp, 0, tmp, tmp); 
   /* (-2n.u)n + u */
   emit_op3(p, VP_OPCODE_MAD, r, 0, negate(tmp), normal, eye_hat); 
   /* r + 0,0,1 */
   emit_op2(p, VP_OPCODE_ADD, tmp, 0, r, swizzle(id,X,Y,W,Z)); 
   /* rx^2 + ry^2 + (rz+1)^2 */
   emit_op2(p, VP_OPCODE_DP3, tmp, 0, tmp, tmp); 
   /* 2/m */
   emit_op1(p, VP_OPCODE_RSQ, tmp, 0, tmp); 
   /* 1/m */
   emit_op2(p, VP_OPCODE_MUL, inv_m, 0, tmp, swizzle1(half,X)); 
   /* r/m + 1/2 */
   emit_op3(p, VP_OPCODE_MAD, dest, writemask, r, inv_m, swizzle1(half,X)); 
	       
   release_temp(p, tmp);
   release_temp(p, r);
   release_temp(p, inv_m);
}


static void build_texture_transform( struct tnl_program *p )
{
   GLcontext *ctx = p->ctx;
   GLuint i, j;

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      GLuint texmat_enabled = ctx->Texture._TexMatEnabled & ENABLE_TEXMAT(i);
      struct ureg out = register_output(p, VERT_RESULT_TEX0 + i);

      if (texUnit->TexGenEnabled || texmat_enabled) {
	 struct ureg out_texgen = undef;

	 if (texUnit->TexGenEnabled) {
	    GLuint copy_mask = 0;
	    GLuint sphere_mask = 0;
	    GLuint reflect_mask = 0;
	    GLuint normal_mask = 0;
	    GLuint modes[4];
	 
	    if (texmat_enabled) 
	       out_texgen = get_temp(p);
	    else
	       out_texgen = out;

	    modes[0] = texUnit->GenModeS;
	    modes[1] = texUnit->GenModeT;
	    modes[2] = texUnit->GenModeR;
	    modes[3] = texUnit->GenModeQ;

	    for (j = 0; j < 4; j++) {
	       if (texUnit->TexGenEnabled & (1<<j)) {
		  switch (modes[j]) {
		  case GL_OBJECT_LINEAR: {
		     struct ureg obj = register_input(p, VERT_ATTRIB_POS);
		     struct ureg plane = 
			register_param3(p, STATE_TEXGEN, i,
					STATE_TEXGEN_OBJECT_S + j);

		     emit_op2(p, VP_OPCODE_DP4, out_texgen, WRITEMASK_X << j, 
			      obj, plane );
		     break;
		  }
		  case GL_EYE_LINEAR: {
		     struct ureg eye = get_eye_position(p);
		     struct ureg plane = 
			register_param3(p, STATE_TEXGEN, i, 
					STATE_TEXGEN_EYE_S + j);

		     emit_op2(p, VP_OPCODE_DP4, out_texgen, WRITEMASK_X << j, 
			      eye, plane );
		     break;
		  }
		  case GL_SPHERE_MAP: 
		     sphere_mask |= WRITEMASK_X << j;
		     break;
		  case GL_REFLECTION_MAP_NV:
		     reflect_mask |= WRITEMASK_X << j;
		     break;
		  case GL_NORMAL_MAP_NV: 
		     normal_mask |= WRITEMASK_X << j;
		     break;
		  }
	       }
	       else 
		  copy_mask |= WRITEMASK_X << j;
	    }

	 
	    if (sphere_mask) {
	       build_sphere_texgen(p, out_texgen, sphere_mask);
	    }

	    if (reflect_mask) {
	       build_reflect_texgen(p, out_texgen, reflect_mask);
	    }

	    if (normal_mask) {
	       struct ureg normal = get_eye_normal(p);
	       emit_op1(p, VP_OPCODE_MOV, out_texgen, normal_mask, normal );
	    }

	    if (copy_mask) {
	       struct ureg in = register_input(p, VERT_ATTRIB_TEX0+i);
	       emit_op1(p, VP_OPCODE_MOV, out_texgen, copy_mask, in );
	    }
	 }

	 if (texmat_enabled) {
	    struct ureg texmat[4];
	    struct ureg in = (!is_undef(out_texgen) ? 
			      out_texgen : 
			      register_input(p, VERT_ATTRIB_TEX0+i));
	    register_matrix_param6( p, STATE_MATRIX, STATE_TEXTURE, i, 
				    0, 3, 0, texmat );
	    emit_matrix_transform_vec4( p, out, texmat, in );
	 }

	 release_temps(p);
      }
   }
}


/* Seems like it could be tighter:
 */
static void build_pointsize( struct tnl_program *p )
{
   struct ureg eye = get_eye_position(p);
   struct ureg state_size = register_param1(p, STATE_POINT_SIZE);
   struct ureg state_attenuation = register_param1(p, STATE_POINT_ATTENUATION);
   struct ureg out = register_output(p, VERT_RESULT_PSIZ);
   struct ureg ut = get_temp(p);

   /* 1, -Z, Z * Z, 1 */      
   emit_op1(p, VP_OPCODE_MOV, ut, 0, swizzle1(get_identity_param(p), W));
   emit_op2(p, VP_OPCODE_MUL, ut, WRITEMASK_YZ, ut, negate(swizzle1(eye, Z)));
   emit_op2(p, VP_OPCODE_MUL, ut, WRITEMASK_Z, ut, negate(swizzle1(eye, Z)));


   /* p1 +  p2 * dist + p3 * dist * dist, 0 */
   emit_op2(p, VP_OPCODE_DP3, ut, 0, ut, state_attenuation);

   /* 1 / factor */
   emit_op1(p, VP_OPCODE_RCP, ut, 0, ut ); 

   /* out = pointSize / factor */
   emit_op2(p, VP_OPCODE_MUL, out, WRITEMASK_X, ut, state_size); 

   release_temp(p, ut);
}


static void build_passthrough( struct tnl_program *p, GLuint inputs )
{
}


static GLboolean programs_eq( struct vertex_program *a,
			      struct vertex_program *b )
{
   if (!a || !b)
      return GL_FALSE;

   if (a->Base.NumInstructions != b->Base.NumInstructions ||
       a->Parameters->NumParameters != b->Parameters->NumParameters)
      return GL_FALSE;

   if (memcmp(a->Instructions, b->Instructions, 
	      a->Base.NumInstructions * sizeof(struct vp_instruction)) != 0)
      return GL_FALSE;

   if (memcmp(a->Parameters->Parameters, b->Parameters->Parameters,
	      a->Parameters->NumParameters * 
	      sizeof(struct program_parameter)) != 0)
      return GL_FALSE;

   return GL_TRUE;
}


void _tnl_UpdateFixedFunctionProgram( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_program p;

   if (ctx->VertexProgram._Enabled)
      return;
   

   memset(&p, 0, sizeof(p));
   p.ctx = ctx;
   p.program = (struct vertex_program *)ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0);
   p.eye_position = undef;
   p.eye_position_normalized = undef;
   p.eye_normal = undef;
   p.identity = undef;

   p.temp_flag = 0;
   p.temp_reserved = ~((1<<MAX_NV_VERTEX_PROGRAM_TEMPS)-1);
   p.program->Instructions = MALLOC(sizeof(struct vp_instruction) * MAX_INSN);

   /* Initialize the arb_program struct */
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
   p.program->OutputsWritten = 0;

   /* Emit the program, starting with modelviewproject:
    */
   build_hpos(&p);

   /* Lighting calculations:
    */
   if (ctx->Light.Enabled)
      build_lighting(&p);

   if (ctx->Fog.Enabled)
      build_fog(&p);

   if (ctx->Texture._TexGenEnabled || ctx->Texture._TexMatEnabled)
      build_texture_transform(&p);

   if (ctx->Point._Attenuated)
      build_pointsize(&p);

   /* Is there a need to copy inputs to outputs?  The software
    * implementation might do this more efficiently by just assigning
    * the missing results to point at input arrays.
    */
   if (/* tnl->vp_copy_inputs &&  */
       (tnl->render_inputs & ~p.program->OutputsWritten)) {
      build_passthrough(&p, tnl->render_inputs);
   }


   /* Finish up:
    */
   emit_op1(&p, VP_OPCODE_END, undef, 0, undef);

   /* Disassemble:
    */
   if (DISASSEM) {
      _mesa_printf ("\n");
   }


   /* Notify driver the fragment program has (actually) changed.
    */
   if (!programs_eq(ctx->_TnlProgram, p.program) != 0) {
      if (ctx->_TnlProgram)
	 ctx->Driver.DeleteProgram( ctx, &ctx->_TnlProgram->Base );
      ctx->_TnlProgram = p.program;
   }
   else if (p.program) {
      /* Nothing changed...
       */
      ctx->Driver.DeleteProgram( ctx, &p.program->Base );
   }
}
