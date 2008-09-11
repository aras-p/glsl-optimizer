/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 2007  Tungsten Graphics   All Rights Reserved.
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


#include "main/glheader.h"
#include "main/ffvertex_prog.h"
#include "t_vp_build.h"


struct state_key {
   unsigned light_global_enabled:1;
   unsigned light_local_viewer:1;
   unsigned light_twoside:1;
   unsigned light_color_material:1;
   unsigned light_color_material_mask:12;
   unsigned light_material_mask:12;

   unsigned normalize:1;
   unsigned rescale_normals:1;
   unsigned fog_source_is_depth:1;
   unsigned tnl_do_vertex_fog:1;
   unsigned separate_specular:1;
   unsigned fog_mode:2;
   unsigned point_attenuated:1;
   unsigned texture_enabled_global:1;
   unsigned fragprog_inputs_read:12;

   struct {
      unsigned light_enabled:1;
      unsigned light_eyepos3_is_zero:1;
      unsigned light_spotcutoff_is_180:1;
      unsigned light_attenuated:1;      
      unsigned texunit_really_enabled:1;
      unsigned texmat_enabled:1;
      unsigned texgen_enabled:4;
      unsigned texgen_mode0:4;
      unsigned texgen_mode1:4;
      unsigned texgen_mode2:4;
      unsigned texgen_mode3:4;
   } unit[8];
};



#define FOG_NONE   0
#define FOG_LINEAR 1
#define FOG_EXP    2
#define FOG_EXP2   3

static GLuint translate_fog_mode( GLenum mode )
{
   switch (mode) {
   case GL_LINEAR: return FOG_LINEAR;
   case GL_EXP: return FOG_EXP;
   case GL_EXP2: return FOG_EXP2;
   default: return FOG_NONE;
   }
}

#define TXG_NONE           0
#define TXG_OBJ_LINEAR     1
#define TXG_EYE_LINEAR     2
#define TXG_SPHERE_MAP     3
#define TXG_REFLECTION_MAP 4
#define TXG_NORMAL_MAP     5

static GLuint translate_texgen( GLboolean enabled, GLenum mode )
{
   if (!enabled)
      return TXG_NONE;

   switch (mode) {
   case GL_OBJECT_LINEAR: return TXG_OBJ_LINEAR;
   case GL_EYE_LINEAR: return TXG_EYE_LINEAR;
   case GL_SPHERE_MAP: return TXG_SPHERE_MAP;
   case GL_REFLECTION_MAP_NV: return TXG_REFLECTION_MAP;
   case GL_NORMAL_MAP_NV: return TXG_NORMAL_MAP;
   default: return TXG_NONE;
   }
}

static struct state_key *make_state_key( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   const struct gl_fragment_program *fp = ctx->FragmentProgram._Current;
   struct state_key *key = CALLOC_STRUCT(state_key);
   GLuint i;

   /* This now relies on texenvprogram.c being active:
    */
   assert(fp);

   key->fragprog_inputs_read = fp->Base.InputsRead;

   key->separate_specular = (ctx->Light.Model.ColorControl ==
			     GL_SEPARATE_SPECULAR_COLOR);

   if (ctx->Light.Enabled) {
      key->light_global_enabled = 1;

      if (ctx->Light.Model.LocalViewer)
	 key->light_local_viewer = 1;

      if (ctx->Light.Model.TwoSide)
	 key->light_twoside = 1;

      if (ctx->Light.ColorMaterialEnabled) {
	 key->light_color_material = 1;
	 key->light_color_material_mask = ctx->Light.ColorMaterialBitmask;
      }

      for (i = _TNL_FIRST_MAT; i <= _TNL_LAST_MAT; i++) 
	 if (VB->AttribPtr[i]->stride) 
	    key->light_material_mask |= 1<<(i-_TNL_ATTRIB_MAT_FRONT_AMBIENT);

      for (i = 0; i < MAX_LIGHTS; i++) {
	 struct gl_light *light = &ctx->Light.Light[i];

	 if (light->Enabled) {
	    key->unit[i].light_enabled = 1;

	    if (light->EyePosition[3] == 0.0)
	       key->unit[i].light_eyepos3_is_zero = 1;
	    
	    if (light->SpotCutoff == 180.0)
	       key->unit[i].light_spotcutoff_is_180 = 1;

	    if (light->ConstantAttenuation != 1.0 ||
		light->LinearAttenuation != 0.0 ||
		light->QuadraticAttenuation != 0.0)
	       key->unit[i].light_attenuated = 1;
	 }
      }
   }

   if (ctx->Transform.Normalize)
      key->normalize = 1;

   if (ctx->Transform.RescaleNormals)
      key->rescale_normals = 1;

   key->fog_mode = translate_fog_mode(fp->FogOption);
   
   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT)
      key->fog_source_is_depth = 1;
   
   if (tnl->_DoVertexFog)
      key->tnl_do_vertex_fog = 1;

   if (ctx->Point._Attenuated)
      key->point_attenuated = 1;

   if (ctx->Texture._TexGenEnabled ||
       ctx->Texture._TexMatEnabled ||
       ctx->Texture._EnabledUnits)
      key->texture_enabled_global = 1;
      
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      if (texUnit->_ReallyEnabled)
	 key->unit[i].texunit_really_enabled = 1;

      if (ctx->Texture._TexMatEnabled & ENABLE_TEXMAT(i))      
	 key->unit[i].texmat_enabled = 1;
      
      if (texUnit->TexGenEnabled) {
	 key->unit[i].texgen_enabled = 1;
      
	 key->unit[i].texgen_mode0 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<0),
			      texUnit->GenModeS );
	 key->unit[i].texgen_mode1 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<1),
			      texUnit->GenModeT );
	 key->unit[i].texgen_mode2 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<2),
			      texUnit->GenModeR );
	 key->unit[i].texgen_mode3 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<3),
			      texUnit->GenModeQ );
      }
   }
   
   return key;
}


   
/* Very useful debugging tool - produces annotated listing of
 * generated program with line/function references for each
 * instruction back into this file:
 */
#define DISASSEM (MESA_VERBOSE&VERBOSE_DISASSEM)

/* Should be tunable by the driver - do we want to do matrix
 * multiplications with DP4's or with MUL/MAD's?  SSE works better
 * with the latter, drivers may differ.
 */
#define PREFER_DP4 0

#define MAX_INSN 350

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
   GLint idx:8;      /* relative addressing may be negative */
   GLuint negate:1;
   GLuint swz:12;
   GLuint pad:7;
};


struct tnl_program {
   const struct state_key *state;
   struct gl_vertex_program *program;
   
   GLuint temp_in_use;
   GLuint temp_reserved;
   
   struct ureg eye_position;
   struct ureg eye_position_normalized;
   struct ureg eye_normal;
   struct ureg identity;

   GLuint materials;
   GLuint color_materials;
};


static const struct ureg undef = { 
   PROGRAM_UNDEFINED,
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
static struct ureg make_ureg(GLuint file, GLint idx)
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
   int bit = _mesa_ffs( ~p->temp_in_use );
   if (!bit) {
      _mesa_problem(NULL, "%s: out of temporaries\n", __FILE__);
      _mesa_exit(1);
   }

   if ((GLuint) bit > p->program->Base.NumTemporaries)
      p->program->Base.NumTemporaries = bit;

   p->temp_in_use |= 1<<(bit-1);
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
      p->temp_in_use &= ~(1<<reg.idx);
      p->temp_in_use |= p->temp_reserved; /* can't release reserved temps */
   }
}

static void release_temps( struct tnl_program *p )
{
   p->temp_in_use = p->temp_reserved;
}



static struct ureg register_input( struct tnl_program *p, GLuint input )
{
   p->program->Base.InputsRead |= (1<<input);
   return make_ureg(PROGRAM_INPUT, input);
}

static struct ureg register_output( struct tnl_program *p, GLuint output )
{
   p->program->Base.OutputsWritten |= (1<<output);
   return make_ureg(PROGRAM_OUTPUT, output);
}

static struct ureg register_const4f( struct tnl_program *p, 
			      GLfloat s0,
			      GLfloat s1,
			      GLfloat s2,
			      GLfloat s3)
{
   GLfloat values[4];
   GLint idx;
   GLuint swizzle;
   values[0] = s0;
   values[1] = s1;
   values[2] = s2;
   values[3] = s3;
   idx = _mesa_add_unnamed_constant( p->program->Base.Parameters, values, 4,
                                     &swizzle );
   ASSERT(swizzle == SWIZZLE_NOOP);
   return make_ureg(PROGRAM_STATE_VAR, idx);
}

#define register_const1f(p, s0)         register_const4f(p, s0, 0, 0, 1)
#define register_scalar_const(p, s0)    register_const4f(p, s0, s0, s0, s0)
#define register_const2f(p, s0, s1)     register_const4f(p, s0, s1, 0, 1)
#define register_const3f(p, s0, s1, s2) register_const4f(p, s0, s1, s2, 1)

static GLboolean is_undef( struct ureg reg )
{
   return reg.file == PROGRAM_UNDEFINED;
}

static struct ureg get_identity_param( struct tnl_program *p )
{
   if (is_undef(p->identity)) 
      p->identity = register_const4f(p, 0,0,0,1);

   return p->identity;
}

static struct ureg register_param5(struct tnl_program *p, 
				   GLint s0,
				   GLint s1,
				   GLint s2,
				   GLint s3,
                                   GLint s4)
{
   gl_state_index tokens[STATE_LENGTH];
   GLint idx;
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   tokens[4] = s4;
   idx = _mesa_add_state_reference( p->program->Base.Parameters, tokens );
   return make_ureg(PROGRAM_STATE_VAR, idx);
}


#define register_param1(p,s0)          register_param5(p,s0,0,0,0,0)
#define register_param2(p,s0,s1)       register_param5(p,s0,s1,0,0,0)
#define register_param3(p,s0,s1,s2)    register_param5(p,s0,s1,s2,0,0)
#define register_param4(p,s0,s1,s2,s3) register_param5(p,s0,s1,s2,s3,0)


static void register_matrix_param5( struct tnl_program *p,
				    GLint s0, /* modelview, projection, etc */
				    GLint s1, /* texture matrix number */
				    GLint s2, /* first row */
				    GLint s3, /* last row */
				    GLint s4, /* inverse, transpose, etc */
				    struct ureg *matrix )
{
   GLint i;

   /* This is a bit sad as the support is there to pull the whole
    * matrix out in one go:
    */
   for (i = 0; i <= s3 - s2; i++) 
      matrix[i] = register_param5( p, s0, s1, i, i, s4 );
}


/**
 * Convert a ureg source register to a prog_src_register.
 */
static void emit_arg( struct prog_src_register *src,
		      struct ureg reg )
{
   assert(reg.file != PROGRAM_OUTPUT);
   src->File = reg.file;
   src->Index = reg.idx;
   src->Swizzle = reg.swz;
   src->NegateBase = reg.negate ? NEGATE_XYZW : 0;
   src->Abs = 0;
   src->NegateAbs = 0;
   src->RelAddr = 0;
}

/**
 * XXX This should go away someday, but still referenced by some drivers...
 */
void _tnl_UpdateFixedFunctionProgram( GLcontext *ctx )
{
   const struct gl_vertex_program *prev = ctx->VertexProgram._Current;

   if (!ctx->VertexProgram._Current ||
       ctx->VertexProgram._Current == ctx->VertexProgram._TnlProgram) {
      struct gl_vertex_program *newProg;

      newProg = _mesa_get_fixed_func_vertex_program(ctx);

      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._TnlProgram, newProg);
      _mesa_reference_vertprog(ctx, &ctx->VertexProgram._Current, newProg);
   }

   /* Tell the driver about the change.  Could define a new target for
    * this?
    */
   if (ctx->VertexProgram._Current != prev && ctx->Driver.BindProgram) {
      ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
                            (struct gl_program *) ctx->VertexProgram._Current);
   }
}
