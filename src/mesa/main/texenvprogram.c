/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
#include "imports.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_cache.h"
#include "shader/prog_instruction.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "shader/programopt.h"
#include "texenvprogram.h"


/*
 * Note on texture units:
 *
 * The number of texture units supported by fixed-function fragment
 * processing is MAX_TEXTURE_COORD_UNITS, not MAX_TEXTURE_IMAGE_UNITS.
 * That's because there's a one-to-one correspondence between texture
 * coordinates and samplers in fixed-function processing.
 *
 * Since fixed-function vertex processing is limited to MAX_TEXTURE_COORD_UNITS
 * sets of texcoords, so is fixed-function fragment processing.
 *
 * We can safely use ctx->Const.MaxTextureUnits for loop bounds.
 */


struct texenvprog_cache_item
{
   GLuint hash;
   void *key;
   struct gl_fragment_program *data;
   struct texenvprog_cache_item *next;
};

static GLboolean
texenv_doing_secondary_color(GLcontext *ctx)
{
   if (ctx->Light.Enabled &&
       (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR))
      return GL_TRUE;

   if (ctx->Fog.ColorSumEnabled)
      return GL_TRUE;

   return GL_FALSE;
}

/**
 * Up to nine instructions per tex unit, plus fog, specular color.
 */
#define MAX_INSTRUCTIONS ((MAX_TEXTURE_COORD_UNITS * 9) + 12)

#define DISASSEM (MESA_VERBOSE & VERBOSE_DISASSEM)

struct mode_opt {
#ifdef __GNUC__
   __extension__ GLubyte Source:4;  /**< SRC_x */
   __extension__ GLubyte Operand:3; /**< OPR_x */
#else
   GLubyte Source;  /**< SRC_x */
   GLubyte Operand; /**< OPR_x */
#endif
};

struct state_key {
   GLuint nr_enabled_units:8;
   GLuint enabled_units:8;
   GLuint separate_specular:1;
   GLuint fog_enabled:1;
   GLuint fog_mode:2;          /**< FOG_x */
   GLuint inputs_available:12;

   /* NOTE: This array of structs must be last! (see "keySize" below) */
   struct {
      GLuint enabled:1;
      GLuint source_index:3;   /**< TEXTURE_x_INDEX */
      GLuint shadow:1;
      GLuint ScaleShiftRGB:2;
      GLuint ScaleShiftA:2;

      GLuint NumArgsRGB:3;  /**< up to MAX_COMBINER_TERMS */
      GLuint ModeRGB:5;     /**< MODE_x */

      GLuint NumArgsA:3;  /**< up to MAX_COMBINER_TERMS */
      GLuint ModeA:5;     /**< MODE_x */

      GLuint texture_cyl_wrap:1; /**< For gallium test/debug only */

      struct mode_opt OptRGB[MAX_COMBINER_TERMS];
      struct mode_opt OptA[MAX_COMBINER_TERMS];
   } unit[MAX_TEXTURE_UNITS];
};

#define FOG_LINEAR  0
#define FOG_EXP     1
#define FOG_EXP2    2
#define FOG_UNKNOWN 3

static GLuint translate_fog_mode( GLenum mode )
{
   switch (mode) {
   case GL_LINEAR: return FOG_LINEAR;
   case GL_EXP: return FOG_EXP;
   case GL_EXP2: return FOG_EXP2;
   default: return FOG_UNKNOWN;
   }
}

#define OPR_SRC_COLOR           0
#define OPR_ONE_MINUS_SRC_COLOR 1
#define OPR_SRC_ALPHA           2
#define OPR_ONE_MINUS_SRC_ALPHA	3
#define OPR_ZERO                4
#define OPR_ONE                 5
#define OPR_UNKNOWN             7

static GLuint translate_operand( GLenum operand )
{
   switch (operand) {
   case GL_SRC_COLOR: return OPR_SRC_COLOR;
   case GL_ONE_MINUS_SRC_COLOR: return OPR_ONE_MINUS_SRC_COLOR;
   case GL_SRC_ALPHA: return OPR_SRC_ALPHA;
   case GL_ONE_MINUS_SRC_ALPHA: return OPR_ONE_MINUS_SRC_ALPHA;
   case GL_ZERO: return OPR_ZERO;
   case GL_ONE: return OPR_ONE;
   default:
      assert(0);
      return OPR_UNKNOWN;
   }
}

#define SRC_TEXTURE  0
#define SRC_TEXTURE0 1
#define SRC_TEXTURE1 2
#define SRC_TEXTURE2 3
#define SRC_TEXTURE3 4
#define SRC_TEXTURE4 5
#define SRC_TEXTURE5 6
#define SRC_TEXTURE6 7
#define SRC_TEXTURE7 8
#define SRC_CONSTANT 9
#define SRC_PRIMARY_COLOR 10
#define SRC_PREVIOUS 11
#define SRC_ZERO     12
#define SRC_UNKNOWN  15

static GLuint translate_source( GLenum src )
{
   switch (src) {
   case GL_TEXTURE: return SRC_TEXTURE;
   case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7: return SRC_TEXTURE0 + (src - GL_TEXTURE0);
   case GL_CONSTANT: return SRC_CONSTANT;
   case GL_PRIMARY_COLOR: return SRC_PRIMARY_COLOR;
   case GL_PREVIOUS: return SRC_PREVIOUS;
   case GL_ZERO:
      return SRC_ZERO;
   default:
      assert(0);
      return SRC_UNKNOWN;
   }
}

#define MODE_REPLACE                     0  /* r = a0 */
#define MODE_MODULATE                    1  /* r = a0 * a1 */
#define MODE_ADD                         2  /* r = a0 + a1 */
#define MODE_ADD_SIGNED                  3  /* r = a0 + a1 - 0.5 */
#define MODE_INTERPOLATE                 4  /* r = a0 * a2 + a1 * (1 - a2) */
#define MODE_SUBTRACT                    5  /* r = a0 - a1 */
#define MODE_DOT3_RGB                    6  /* r = a0 . a1 */
#define MODE_DOT3_RGB_EXT                7  /* r = a0 . a1 */
#define MODE_DOT3_RGBA                   8  /* r = a0 . a1 */
#define MODE_DOT3_RGBA_EXT               9  /* r = a0 . a1 */
#define MODE_MODULATE_ADD_ATI           10  /* r = a0 * a2 + a1 */
#define MODE_MODULATE_SIGNED_ADD_ATI    11  /* r = a0 * a2 + a1 - 0.5 */
#define MODE_MODULATE_SUBTRACT_ATI      12  /* r = a0 * a2 - a1 */
#define MODE_ADD_PRODUCTS               13  /* r = a0 * a1 + a2 * a3 */
#define MODE_ADD_PRODUCTS_SIGNED        14  /* r = a0 * a1 + a2 * a3 - 0.5 */
#define MODE_BUMP_ENVMAP_ATI            15  /* special */
#define MODE_UNKNOWN                    16

/**
 * Translate GL combiner state into a MODE_x value
 */
static GLuint translate_mode( GLenum envMode, GLenum mode )
{
   switch (mode) {
   case GL_REPLACE: return MODE_REPLACE;
   case GL_MODULATE: return MODE_MODULATE;
   case GL_ADD:
      if (envMode == GL_COMBINE4_NV)
         return MODE_ADD_PRODUCTS;
      else
         return MODE_ADD;
   case GL_ADD_SIGNED:
      if (envMode == GL_COMBINE4_NV)
         return MODE_ADD_PRODUCTS_SIGNED;
      else
         return MODE_ADD_SIGNED;
   case GL_INTERPOLATE: return MODE_INTERPOLATE;
   case GL_SUBTRACT: return MODE_SUBTRACT;
   case GL_DOT3_RGB: return MODE_DOT3_RGB;
   case GL_DOT3_RGB_EXT: return MODE_DOT3_RGB_EXT;
   case GL_DOT3_RGBA: return MODE_DOT3_RGBA;
   case GL_DOT3_RGBA_EXT: return MODE_DOT3_RGBA_EXT;
   case GL_MODULATE_ADD_ATI: return MODE_MODULATE_ADD_ATI;
   case GL_MODULATE_SIGNED_ADD_ATI: return MODE_MODULATE_SIGNED_ADD_ATI;
   case GL_MODULATE_SUBTRACT_ATI: return MODE_MODULATE_SUBTRACT_ATI;
   case GL_BUMP_ENVMAP_ATI: return MODE_BUMP_ENVMAP_ATI;
   default:
      assert(0);
      return MODE_UNKNOWN;
   }
}


/**
 * Do we need to clamp the results of the given texture env/combine mode?
 * If the inputs to the mode are in [0,1] we don't always have to clamp
 * the results.
 */
static GLboolean
need_saturate( GLuint mode )
{
   switch (mode) {
   case MODE_REPLACE:
   case MODE_MODULATE:
   case MODE_INTERPOLATE:
      return GL_FALSE;
   case MODE_ADD:
   case MODE_ADD_SIGNED:
   case MODE_SUBTRACT:
   case MODE_DOT3_RGB:
   case MODE_DOT3_RGB_EXT:
   case MODE_DOT3_RGBA:
   case MODE_DOT3_RGBA_EXT:
   case MODE_MODULATE_ADD_ATI:
   case MODE_MODULATE_SIGNED_ADD_ATI:
   case MODE_MODULATE_SUBTRACT_ATI:
   case MODE_ADD_PRODUCTS:
   case MODE_ADD_PRODUCTS_SIGNED:
   case MODE_BUMP_ENVMAP_ATI:
      return GL_TRUE;
   default:
      assert(0);
      return GL_FALSE;
   }
}



/**
 * Translate TEXTURE_x_BIT to TEXTURE_x_INDEX.
 */
static GLuint translate_tex_src_bit( GLbitfield bit )
{
   ASSERT(bit);
   return _mesa_ffs(bit) - 1;
}


#define VERT_BIT_TEX_ANY    (0xff << VERT_ATTRIB_TEX0)
#define VERT_RESULT_TEX_ANY (0xff << VERT_RESULT_TEX0)

/**
 * Identify all possible varying inputs.  The fragment program will
 * never reference non-varying inputs, but will track them via state
 * constants instead.
 *
 * This function figures out all the inputs that the fragment program
 * has access to.  The bitmask is later reduced to just those which
 * are actually referenced.
 */
static GLbitfield get_fp_input_mask( GLcontext *ctx )
{
   /* _NEW_PROGRAM */
   const GLboolean vertexShader = (ctx->Shader.CurrentProgram &&
				   ctx->Shader.CurrentProgram->LinkStatus &&
                                   ctx->Shader.CurrentProgram->VertexProgram);
   const GLboolean vertexProgram = ctx->VertexProgram._Enabled;
   GLbitfield fp_inputs = 0x0;

   if (ctx->VertexProgram._Overriden) {
      /* Somebody's messing with the vertex program and we don't have
       * a clue what's happening.  Assume that it could be producing
       * all possible outputs.
       */
      fp_inputs = ~0;
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      /* _NEW_RENDERMODE */
      fp_inputs = (FRAG_BIT_COL0 | FRAG_BIT_TEX0);
   }
   else if (!(vertexProgram || vertexShader) ||
            !ctx->VertexProgram._Current) {
      /* Fixed function vertex logic */
      /* _NEW_ARRAY */
      GLbitfield varying_inputs = ctx->varying_vp_inputs;

      /* These get generated in the setup routine regardless of the
       * vertex program:
       */
      /* _NEW_POINT */
      if (ctx->Point.PointSprite)
         varying_inputs |= FRAG_BITS_TEX_ANY;

      /* First look at what values may be computed by the generated
       * vertex program:
       */
      /* _NEW_LIGHT */
      if (ctx->Light.Enabled) {
         fp_inputs |= FRAG_BIT_COL0;

         if (texenv_doing_secondary_color(ctx))
            fp_inputs |= FRAG_BIT_COL1;
      }

      /* _NEW_TEXTURE */
      fp_inputs |= (ctx->Texture._TexGenEnabled |
                    ctx->Texture._TexMatEnabled) << FRAG_ATTRIB_TEX0;

      /* Then look at what might be varying as a result of enabled
       * arrays, etc:
       */
      if (varying_inputs & VERT_BIT_COLOR0)
         fp_inputs |= FRAG_BIT_COL0;
      if (varying_inputs & VERT_BIT_COLOR1)
         fp_inputs |= FRAG_BIT_COL1;

      fp_inputs |= (((varying_inputs & VERT_BIT_TEX_ANY) >> VERT_ATTRIB_TEX0) 
                    << FRAG_ATTRIB_TEX0);

   }
   else {
      /* calculate from vp->outputs */
      struct gl_vertex_program *vprog;
      GLbitfield64 vp_outputs;

      /* Choose GLSL vertex shader over ARB vertex program.  Need this
       * since vertex shader state validation comes after fragment state
       * validation (see additional comments in state.c).
       */
      if (vertexShader)
         vprog = ctx->Shader.CurrentProgram->VertexProgram;
      else
         vprog = ctx->VertexProgram.Current;

      vp_outputs = vprog->Base.OutputsWritten;

      /* These get generated in the setup routine regardless of the
       * vertex program:
       */
      /* _NEW_POINT */
      if (ctx->Point.PointSprite)
         vp_outputs |= FRAG_BITS_TEX_ANY;

      if (vp_outputs & (1 << VERT_RESULT_COL0))
         fp_inputs |= FRAG_BIT_COL0;
      if (vp_outputs & (1 << VERT_RESULT_COL1))
         fp_inputs |= FRAG_BIT_COL1;

      fp_inputs |= (((vp_outputs & VERT_RESULT_TEX_ANY) >> VERT_RESULT_TEX0) 
                    << FRAG_ATTRIB_TEX0);
   }
   
   return fp_inputs;
}


/**
 * Examine current texture environment state and generate a unique
 * key to identify it.
 */
static GLuint make_state_key( GLcontext *ctx,  struct state_key *key )
{
   GLuint i, j;
   GLbitfield inputs_referenced = FRAG_BIT_COL0;
   const GLbitfield inputs_available = get_fp_input_mask( ctx );
   GLuint keySize;

   memset(key, 0, sizeof(*key));

   /* _NEW_TEXTURE */
   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const struct gl_texture_object *texObj = texUnit->_Current;
      const struct gl_tex_env_combine_state *comb = texUnit->_CurrentCombine;
      GLenum format;

      if (!texUnit->_ReallyEnabled || !texUnit->Enabled)
         continue;

      format = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;

      key->unit[i].enabled = 1;
      key->enabled_units |= (1<<i);
      key->nr_enabled_units = i + 1;
      inputs_referenced |= FRAG_BIT_TEX(i);

      key->unit[i].source_index =
         translate_tex_src_bit(texUnit->_ReallyEnabled);

      key->unit[i].shadow = ((texObj->CompareMode == GL_COMPARE_R_TO_TEXTURE) &&
                             ((format == GL_DEPTH_COMPONENT) || 
                              (format == GL_DEPTH_STENCIL_EXT)));

      key->unit[i].NumArgsRGB = comb->_NumArgsRGB;
      key->unit[i].NumArgsA = comb->_NumArgsA;

      key->unit[i].ModeRGB =
	 translate_mode(texUnit->EnvMode, comb->ModeRGB);
      key->unit[i].ModeA =
	 translate_mode(texUnit->EnvMode, comb->ModeA);

      key->unit[i].ScaleShiftRGB = comb->ScaleShiftRGB;
      key->unit[i].ScaleShiftA = comb->ScaleShiftA;

      for (j = 0; j < MAX_COMBINER_TERMS; j++) {
         key->unit[i].OptRGB[j].Operand = translate_operand(comb->OperandRGB[j]);
         key->unit[i].OptA[j].Operand = translate_operand(comb->OperandA[j]);
         key->unit[i].OptRGB[j].Source = translate_source(comb->SourceRGB[j]);
         key->unit[i].OptA[j].Source = translate_source(comb->SourceA[j]);
      }

      if (key->unit[i].ModeRGB == MODE_BUMP_ENVMAP_ATI) {
         /* requires some special translation */
         key->unit[i].NumArgsRGB = 2;
         key->unit[i].ScaleShiftRGB = 0;
         key->unit[i].OptRGB[0].Operand = OPR_SRC_COLOR;
         key->unit[i].OptRGB[0].Source = SRC_TEXTURE;
         key->unit[i].OptRGB[1].Operand = OPR_SRC_COLOR;
         key->unit[i].OptRGB[1].Source = texUnit->BumpTarget - GL_TEXTURE0 + SRC_TEXTURE0;
       }

      /* this is a back-door for enabling cylindrical texture wrap mode */
      if (texObj->Priority == 0.125)
         key->unit[i].texture_cyl_wrap = 1;
   }

   /* _NEW_LIGHT | _NEW_FOG */
   if (texenv_doing_secondary_color(ctx)) {
      key->separate_specular = 1;
      inputs_referenced |= FRAG_BIT_COL1;
   }

   /* _NEW_FOG */
   if (ctx->Fog.Enabled) {
      key->fog_enabled = 1;
      key->fog_mode = translate_fog_mode(ctx->Fog.Mode);
      inputs_referenced |= FRAG_BIT_FOGC; /* maybe */
   }

   key->inputs_available = (inputs_available & inputs_referenced);

   /* compute size of state key, ignoring unused texture units */
   keySize = sizeof(*key) - sizeof(key->unit)
      + key->nr_enabled_units * sizeof(key->unit[0]);

   return keySize;
}


/**
 * Use uregs to represent registers internally, translate to Mesa's
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
   GLuint swz:12;
   GLuint pad:7;
};

static const struct ureg undef = { 
   PROGRAM_UNDEFINED,
   ~0,
   0,
   0,
   0
};


/** State used to build the fragment program:
 */
struct texenv_fragment_program {
   struct gl_fragment_program *program;
   struct state_key *state;

   GLbitfield alu_temps;	/**< Track texture indirections, see spec. */
   GLbitfield temps_output;	/**< Track texture indirections, see spec. */
   GLbitfield temp_in_use;	/**< Tracks temporary regs which are in use. */
   GLboolean error;

   struct ureg src_texture[MAX_TEXTURE_COORD_UNITS];   
   /* Reg containing each texture unit's sampled texture color,
    * else undef.
    */

   struct ureg texcoord_tex[MAX_TEXTURE_COORD_UNITS];
   /* Reg containing texcoord for a texture unit,
    * needed for bump mapping, else undef.
    */

   struct ureg src_previous;	/**< Reg containing color from previous 
				 * stage.  May need to be decl'd.
				 */

   GLuint last_tex_stage;	/**< Number of last enabled texture unit */

   struct ureg half;
   struct ureg one;
   struct ureg zero;
};



static struct ureg make_ureg(GLuint file, GLuint idx)
{
   struct ureg reg;
   reg.file = file;
   reg.idx = idx;
   reg.negatebase = 0;
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

static struct ureg negate( struct ureg reg )
{
   reg.negatebase ^= 1;
   return reg;
}

static GLboolean is_undef( struct ureg reg )
{
   return reg.file == PROGRAM_UNDEFINED;
}


static struct ureg get_temp( struct texenv_fragment_program *p )
{
   GLint bit;
   
   /* First try and reuse temps which have been used already:
    */
   bit = _mesa_ffs( ~p->temp_in_use & p->alu_temps );

   /* Then any unused temporary:
    */
   if (!bit)
      bit = _mesa_ffs( ~p->temp_in_use );

   if (!bit) {
      _mesa_problem(NULL, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   if ((GLuint) bit > p->program->Base.NumTemporaries)
      p->program->Base.NumTemporaries = bit;

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}

static struct ureg get_tex_temp( struct texenv_fragment_program *p )
{
   int bit;
   
   /* First try to find available temp not previously used (to avoid
    * starting a new texture indirection).  According to the spec, the
    * ~p->temps_output isn't necessary, but will keep it there for
    * now:
    */
   bit = _mesa_ffs( ~p->temp_in_use & ~p->alu_temps & ~p->temps_output );

   /* Then any unused temporary:
    */
   if (!bit) 
      bit = _mesa_ffs( ~p->temp_in_use );

   if (!bit) {
      _mesa_problem(NULL, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   if ((GLuint) bit > p->program->Base.NumTemporaries)
      p->program->Base.NumTemporaries = bit;

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}


/** Mark a temp reg as being no longer allocatable. */
static void reserve_temp( struct texenv_fragment_program *p, struct ureg r )
{
   if (r.file == PROGRAM_TEMPORARY)
      p->temps_output |= (1 << r.idx);
}


static void release_temps(GLcontext *ctx, struct texenv_fragment_program *p )
{
   GLuint max_temp = ctx->Const.FragmentProgram.MaxTemps;

   /* KW: To support tex_env_crossbar, don't release the registers in
    * temps_output.
    */
   if (max_temp >= sizeof(int) * 8)
      p->temp_in_use = p->temps_output;
   else
      p->temp_in_use = ~((1<<max_temp)-1) | p->temps_output;
}


static struct ureg register_param5( struct texenv_fragment_program *p, 
				    GLint s0,
				    GLint s1,
				    GLint s2,
				    GLint s3,
				    GLint s4)
{
   gl_state_index tokens[STATE_LENGTH];
   GLuint idx;
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

static GLuint frag_to_vert_attrib( GLuint attrib )
{
   switch (attrib) {
   case FRAG_ATTRIB_COL0: return VERT_ATTRIB_COLOR0;
   case FRAG_ATTRIB_COL1: return VERT_ATTRIB_COLOR1;
   default:
      assert(attrib >= FRAG_ATTRIB_TEX0);
      assert(attrib <= FRAG_ATTRIB_TEX7);
      return attrib - FRAG_ATTRIB_TEX0 + VERT_ATTRIB_TEX0;
   }
}


static struct ureg register_input( struct texenv_fragment_program *p, GLuint input )
{
   if (p->state->inputs_available & (1<<input)) {
      p->program->Base.InputsRead |= (1 << input);
      return make_ureg(PROGRAM_INPUT, input);
   }
   else {
      GLuint idx = frag_to_vert_attrib( input );
      return register_param3( p, STATE_INTERNAL, STATE_CURRENT_ATTRIB, idx );
   }
}


static void emit_arg( struct prog_src_register *reg,
		      struct ureg ureg )
{
   reg->File = ureg.file;
   reg->Index = ureg.idx;
   reg->Swizzle = ureg.swz;
   reg->Negate = ureg.negatebase ? NEGATE_XYZW : NEGATE_NONE;
   reg->Abs = GL_FALSE;
}

static void emit_dst( struct prog_dst_register *dst,
		      struct ureg ureg, GLuint mask )
{
   dst->File = ureg.file;
   dst->Index = ureg.idx;
   dst->WriteMask = mask;
   dst->CondMask = COND_TR;  /* always pass cond test */
   dst->CondSwizzle = SWIZZLE_NOOP;
}

static struct prog_instruction *
emit_op(struct texenv_fragment_program *p,
	enum prog_opcode op,
	struct ureg dest,
	GLuint mask,
	GLboolean saturate,
	struct ureg src0,
	struct ureg src1,
	struct ureg src2 )
{
   const GLuint nr = p->program->Base.NumInstructions++;
   struct prog_instruction *inst = &p->program->Base.Instructions[nr];

   assert(nr < MAX_INSTRUCTIONS);

   _mesa_init_instructions(inst, 1);
   inst->Opcode = op;
   
   emit_arg( &inst->SrcReg[0], src0 );
   emit_arg( &inst->SrcReg[1], src1 );
   emit_arg( &inst->SrcReg[2], src2 );
   
   inst->SaturateMode = saturate ? SATURATE_ZERO_ONE : SATURATE_OFF;

   emit_dst( &inst->DstReg, dest, mask );

#if 0
   /* Accounting for indirection tracking:
    */
   if (dest.file == PROGRAM_TEMPORARY)
      p->temps_output |= 1 << dest.idx;
#endif

   return inst;
}
   

static struct ureg emit_arith( struct texenv_fragment_program *p,
			       enum prog_opcode op,
			       struct ureg dest,
			       GLuint mask,
			       GLboolean saturate,
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
       
   p->program->Base.NumAluInstructions++;
   return dest;
}

static struct ureg emit_texld( struct texenv_fragment_program *p,
			       enum prog_opcode op,
			       struct ureg dest,
			       GLuint destmask,
			       GLuint tex_unit,
			       GLuint tex_idx,
                               GLuint tex_shadow,
			       struct ureg coord )
{
   struct prog_instruction *inst = emit_op( p, op, 
					  dest, destmask, 
					  GL_FALSE,	/* don't saturate? */
					  coord, 	/* arg 0? */
					  undef,
					  undef);
   
   inst->TexSrcTarget = tex_idx;
   inst->TexSrcUnit = tex_unit;
   inst->TexShadow = tex_shadow;

   p->program->Base.NumTexInstructions++;

   /* Accounting for indirection tracking:
    */
   reserve_temp(p, dest);

#if 0
   /* Is this a texture indirection?
    */
   if ((coord.file == PROGRAM_TEMPORARY &&
	(p->temps_output & (1<<coord.idx))) ||
       (dest.file == PROGRAM_TEMPORARY &&
	(p->alu_temps & (1<<dest.idx)))) {
      p->program->Base.NumTexIndirections++;
      p->temps_output = 1<<coord.idx;
      p->alu_temps = 0;
      assert(0);		/* KW: texture env crossbar */
   }
#endif

   return dest;
}


static struct ureg register_const4f( struct texenv_fragment_program *p, 
				     GLfloat s0,
				     GLfloat s1,
				     GLfloat s2,
				     GLfloat s3)
{
   GLfloat values[4];
   GLuint idx, swizzle;
   struct ureg r;
   values[0] = s0;
   values[1] = s1;
   values[2] = s2;
   values[3] = s3;
   idx = _mesa_add_unnamed_constant( p->program->Base.Parameters, values, 4,
                                     &swizzle );
   r = make_ureg(PROGRAM_CONSTANT, idx);
   r.swz = swizzle;
   return r;
}

#define register_scalar_const(p, s0)    register_const4f(p, s0, s0, s0, s0)
#define register_const1f(p, s0)         register_const4f(p, s0, 0, 0, 1)
#define register_const2f(p, s0, s1)     register_const4f(p, s0, s1, 0, 1)
#define register_const3f(p, s0, s1, s2) register_const4f(p, s0, s1, s2, 1)


static struct ureg get_one( struct texenv_fragment_program *p )
{
   if (is_undef(p->one)) 
      p->one = register_scalar_const(p, 1.0);
   return p->one;
}

static struct ureg get_half( struct texenv_fragment_program *p )
{
   if (is_undef(p->half)) 
      p->half = register_scalar_const(p, 0.5);
   return p->half;
}

static struct ureg get_zero( struct texenv_fragment_program *p )
{
   if (is_undef(p->zero)) 
      p->zero = register_scalar_const(p, 0.0);
   return p->zero;
}


static void program_error( struct texenv_fragment_program *p, const char *msg )
{
   _mesa_problem(NULL, msg);
   p->error = 1;
}

static struct ureg get_source( struct texenv_fragment_program *p, 
			       GLuint src, GLuint unit )
{
   switch (src) {
   case SRC_TEXTURE: 
      assert(!is_undef(p->src_texture[unit]));
      return p->src_texture[unit];

   case SRC_TEXTURE0:
   case SRC_TEXTURE1:
   case SRC_TEXTURE2:
   case SRC_TEXTURE3:
   case SRC_TEXTURE4:
   case SRC_TEXTURE5:
   case SRC_TEXTURE6:
   case SRC_TEXTURE7: 
      assert(!is_undef(p->src_texture[src - SRC_TEXTURE0]));
      return p->src_texture[src - SRC_TEXTURE0];

   case SRC_CONSTANT:
      return register_param2(p, STATE_TEXENV_COLOR, unit);

   case SRC_PRIMARY_COLOR:
      return register_input(p, FRAG_ATTRIB_COL0);

   case SRC_ZERO:
      return get_zero(p);

   case SRC_PREVIOUS:
      if (is_undef(p->src_previous))
	 return register_input(p, FRAG_ATTRIB_COL0);
      else
	 return p->src_previous;

   default:
      assert(0);
      return undef;
   }
}

static struct ureg emit_combine_source( struct texenv_fragment_program *p, 
					GLuint mask,
					GLuint unit,
					GLuint source, 
					GLuint operand )
{
   struct ureg arg, src, one;

   src = get_source(p, source, unit);

   switch (operand) {
   case OPR_ONE_MINUS_SRC_COLOR: 
      /* Get unused tmp,
       * Emit tmp = 1.0 - arg.xyzw
       */
      arg = get_temp( p );
      one = get_one( p );
      return emit_arith( p, OPCODE_SUB, arg, mask, 0, one, src, undef);

   case OPR_SRC_ALPHA: 
      if (mask == WRITEMASK_W)
	 return src;
      else
	 return swizzle1( src, SWIZZLE_W );
   case OPR_ONE_MINUS_SRC_ALPHA: 
      /* Get unused tmp,
       * Emit tmp = 1.0 - arg.wwww
       */
      arg = get_temp(p);
      one = get_one(p);
      return emit_arith(p, OPCODE_SUB, arg, mask, 0,
			one, swizzle1(src, SWIZZLE_W), undef);
   case OPR_ZERO:
      return get_zero(p);
   case OPR_ONE:
      return get_one(p);
   case OPR_SRC_COLOR: 
      return src;
   default:
      assert(0);
      return src;
   }
}

/**
 * Check if the RGB and Alpha sources and operands match for the given
 * texture unit's combinder state.  When the RGB and A sources and
 * operands match, we can emit fewer instructions.
 */
static GLboolean args_match( const struct state_key *key, GLuint unit )
{
   GLuint i, numArgs = key->unit[unit].NumArgsRGB;

   for (i = 0; i < numArgs; i++) {
      if (key->unit[unit].OptA[i].Source != key->unit[unit].OptRGB[i].Source) 
	 return GL_FALSE;

      switch (key->unit[unit].OptA[i].Operand) {
      case OPR_SRC_ALPHA: 
	 switch (key->unit[unit].OptRGB[i].Operand) {
	 case OPR_SRC_COLOR: 
	 case OPR_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      case OPR_ONE_MINUS_SRC_ALPHA: 
	 switch (key->unit[unit].OptRGB[i].Operand) {
	 case OPR_ONE_MINUS_SRC_COLOR: 
	 case OPR_ONE_MINUS_SRC_ALPHA: 
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
				 GLboolean saturate,
				 GLuint unit,
				 GLuint nr,
				 GLuint mode,
				 const struct mode_opt *opt)
{
   struct ureg src[MAX_COMBINER_TERMS];
   struct ureg tmp, half;
   GLuint i;

   assert(nr <= MAX_COMBINER_TERMS);

   for (i = 0; i < nr; i++)
      src[i] = emit_combine_source( p, mask, unit, opt[i].Source, opt[i].Operand );

   switch (mode) {
   case MODE_REPLACE: 
      if (mask == WRITEMASK_XYZW && !saturate)
	 return src[0];
      else
	 return emit_arith( p, OPCODE_MOV, dest, mask, saturate, src[0], undef, undef );
   case MODE_MODULATE: 
      return emit_arith( p, OPCODE_MUL, dest, mask, saturate,
			 src[0], src[1], undef );
   case MODE_ADD: 
      return emit_arith( p, OPCODE_ADD, dest, mask, saturate, 
			 src[0], src[1], undef );
   case MODE_ADD_SIGNED:
      /* tmp = arg0 + arg1
       * result = tmp - .5
       */
      half = get_half(p);
      tmp = get_temp( p );
      emit_arith( p, OPCODE_ADD, tmp, mask, 0, src[0], src[1], undef );
      emit_arith( p, OPCODE_SUB, dest, mask, saturate, tmp, half, undef );
      return dest;
   case MODE_INTERPOLATE: 
      /* Arg0 * (Arg2) + Arg1 * (1-Arg2) -- note arguments are reordered:
       */
      return emit_arith( p, OPCODE_LRP, dest, mask, saturate, src[2], src[0], src[1] );

   case MODE_SUBTRACT: 
      return emit_arith( p, OPCODE_SUB, dest, mask, saturate, src[0], src[1], undef );

   case MODE_DOT3_RGBA:
   case MODE_DOT3_RGBA_EXT: 
   case MODE_DOT3_RGB_EXT:
   case MODE_DOT3_RGB: {
      struct ureg tmp0 = get_temp( p );
      struct ureg tmp1 = get_temp( p );
      struct ureg neg1 = register_scalar_const(p, -1);
      struct ureg two  = register_scalar_const(p, 2);

      /* tmp0 = 2*src0 - 1
       * tmp1 = 2*src1 - 1
       *
       * dst = tmp0 dot3 tmp1 
       */
      emit_arith( p, OPCODE_MAD, tmp0, WRITEMASK_XYZW, 0, 
		  two, src[0], neg1);

      if (memcmp(&src[0], &src[1], sizeof(struct ureg)) == 0)
	 tmp1 = tmp0;
      else
	 emit_arith( p, OPCODE_MAD, tmp1, WRITEMASK_XYZW, 0, 
		     two, src[1], neg1);
      emit_arith( p, OPCODE_DP3, dest, mask, saturate, tmp0, tmp1, undef);
      return dest;
   }
   case MODE_MODULATE_ADD_ATI:
      /* Arg0 * Arg2 + Arg1 */
      return emit_arith( p, OPCODE_MAD, dest, mask, saturate,
			 src[0], src[2], src[1] );
   case MODE_MODULATE_SIGNED_ADD_ATI: {
      /* Arg0 * Arg2 + Arg1 - 0.5 */
      struct ureg tmp0 = get_temp(p);
      half = get_half(p);
      emit_arith( p, OPCODE_MAD, tmp0, mask, 0, src[0], src[2], src[1] );
      emit_arith( p, OPCODE_SUB, dest, mask, saturate, tmp0, half, undef );
      return dest;
   }
   case MODE_MODULATE_SUBTRACT_ATI:
      /* Arg0 * Arg2 - Arg1 */
      emit_arith( p, OPCODE_MAD, dest, mask, 0, src[0], src[2], negate(src[1]) );
      return dest;
   case MODE_ADD_PRODUCTS:
      /* Arg0 * Arg1 + Arg2 * Arg3 */
      {
         struct ureg tmp0 = get_temp(p);
         emit_arith( p, OPCODE_MUL, tmp0, mask, 0, src[0], src[1], undef );
         emit_arith( p, OPCODE_MAD, dest, mask, saturate, src[2], src[3], tmp0 );
      }
      return dest;
   case MODE_ADD_PRODUCTS_SIGNED:
      /* Arg0 * Arg1 + Arg2 * Arg3 - 0.5 */
      {
         struct ureg tmp0 = get_temp(p);
         half = get_half(p);
         emit_arith( p, OPCODE_MUL, tmp0, mask, 0, src[0], src[1], undef );
         emit_arith( p, OPCODE_MAD, tmp0, mask, 0, src[2], src[3], tmp0 );
         emit_arith( p, OPCODE_SUB, dest, mask, saturate, tmp0, half, undef );
      }
      return dest;
   case MODE_BUMP_ENVMAP_ATI:
      /* special - not handled here */
      assert(0);
      return src[0];
   default: 
      assert(0);
      return src[0];
   }
}


/**
 * Generate instructions for one texture unit's env/combiner mode.
 */
static struct ureg
emit_texenv(struct texenv_fragment_program *p, GLuint unit)
{
   const struct state_key *key = p->state;
   GLboolean rgb_saturate, alpha_saturate;
   GLuint rgb_shift, alpha_shift;
   struct ureg out, dest;

   if (!key->unit[unit].enabled) {
      return get_source(p, SRC_PREVIOUS, 0);
   }
   if (key->unit[unit].ModeRGB == MODE_BUMP_ENVMAP_ATI) {
      /* this isn't really a env stage delivering a color and handled elsewhere */
      return get_source(p, SRC_PREVIOUS, 0);
   }
   
   switch (key->unit[unit].ModeRGB) {
   case MODE_DOT3_RGB_EXT:
      alpha_shift = key->unit[unit].ScaleShiftA;
      rgb_shift = 0;
      break;
   case MODE_DOT3_RGBA_EXT:
      alpha_shift = 0;
      rgb_shift = 0;
      break;
   default:
      rgb_shift = key->unit[unit].ScaleShiftRGB;
      alpha_shift = key->unit[unit].ScaleShiftA;
      break;
   }
   
   /* If we'll do rgb/alpha shifting don't saturate in emit_combine().
    * We don't want to clamp twice.
    */
   if (rgb_shift)
      rgb_saturate = GL_FALSE;  /* saturate after rgb shift */
   else if (need_saturate(key->unit[unit].ModeRGB))
      rgb_saturate = GL_TRUE;
   else
      rgb_saturate = GL_FALSE;

   if (alpha_shift)
      alpha_saturate = GL_FALSE;  /* saturate after alpha shift */
   else if (need_saturate(key->unit[unit].ModeA))
      alpha_saturate = GL_TRUE;
   else
      alpha_saturate = GL_FALSE;

   /* If this is the very last calculation, emit direct to output reg:
    */
   if (key->separate_specular ||
       unit != p->last_tex_stage ||
       alpha_shift ||
       rgb_shift)
      dest = get_temp( p );
   else
      dest = make_ureg(PROGRAM_OUTPUT, FRAG_RESULT_COLOR);

   /* Emit the RGB and A combine ops
    */
   if (key->unit[unit].ModeRGB == key->unit[unit].ModeA &&
       args_match(key, unit)) {
      out = emit_combine( p, dest, WRITEMASK_XYZW, rgb_saturate,
			  unit,
			  key->unit[unit].NumArgsRGB,
			  key->unit[unit].ModeRGB,
			  key->unit[unit].OptRGB);
   }
   else if (key->unit[unit].ModeRGB == MODE_DOT3_RGBA_EXT ||
	    key->unit[unit].ModeRGB == MODE_DOT3_RGBA) {
      out = emit_combine( p, dest, WRITEMASK_XYZW, rgb_saturate,
			  unit,
			  key->unit[unit].NumArgsRGB,
			  key->unit[unit].ModeRGB,
			  key->unit[unit].OptRGB);
   }
   else {
      /* Need to do something to stop from re-emitting identical
       * argument calculations here:
       */
      out = emit_combine( p, dest, WRITEMASK_XYZ, rgb_saturate,
			  unit,
			  key->unit[unit].NumArgsRGB,
			  key->unit[unit].ModeRGB,
			  key->unit[unit].OptRGB);
      out = emit_combine( p, dest, WRITEMASK_W, alpha_saturate,
			  unit,
			  key->unit[unit].NumArgsA,
			  key->unit[unit].ModeA,
			  key->unit[unit].OptA);
   }

   /* Deal with the final shift:
    */
   if (alpha_shift || rgb_shift) {
      struct ureg shift;
      GLboolean saturate = GL_TRUE;  /* always saturate at this point */

      if (rgb_shift == alpha_shift) {
	 shift = register_scalar_const(p, (GLfloat)(1<<rgb_shift));
      }
      else {
	 shift = register_const4f(p, 
				  (GLfloat)(1<<rgb_shift),
				  (GLfloat)(1<<rgb_shift),
				  (GLfloat)(1<<rgb_shift),
				  (GLfloat)(1<<alpha_shift));
      }
      return emit_arith( p, OPCODE_MUL, dest, WRITEMASK_XYZW, 
			 saturate, out, shift, undef );
   }
   else
      return out;
}


/**
 * Generate instruction for getting a texture source term.
 */
static void load_texture( struct texenv_fragment_program *p, GLuint unit )
{
   if (is_undef(p->src_texture[unit])) {
      const GLuint texTarget = p->state->unit[unit].source_index;
      struct ureg texcoord;
      struct ureg tmp = get_tex_temp( p );

      if (is_undef(p->texcoord_tex[unit])) {
         texcoord = register_input(p, FRAG_ATTRIB_TEX0+unit);
      }
      else {
         /* might want to reuse this reg for tex output actually */
         texcoord = p->texcoord_tex[unit];
      }

      /* TODO: Use D0_MASK_XY where possible.
       */
      if (p->state->unit[unit].enabled) {
         GLboolean shadow = GL_FALSE;

	 if (p->state->unit[unit].shadow) {
	    p->program->Base.ShadowSamplers |= 1 << unit;
            shadow = GL_TRUE;
         }

	 p->src_texture[unit] = emit_texld( p, OPCODE_TXP,
					    tmp, WRITEMASK_XYZW, 
					    unit, texTarget, shadow,
                                            texcoord );

         p->program->Base.SamplersUsed |= (1 << unit);
         /* This identity mapping should already be in place
          * (see _mesa_init_program_struct()) but let's be safe.
          */
         p->program->Base.SamplerUnits[unit] = unit;
      }
      else
	 p->src_texture[unit] = get_zero(p);

      if (p->state->unit[unit].texture_cyl_wrap) {
         /* set flag which is checked by Mesa->Gallium program translation */
         p->program->Base.InputFlags[0] |= PROG_PARAM_BIT_CYL_WRAP;
      }

   }
}

static GLboolean load_texenv_source( struct texenv_fragment_program *p, 
				     GLuint src, GLuint unit )
{
   switch (src) {
   case SRC_TEXTURE:
      load_texture(p, unit);
      break;

   case SRC_TEXTURE0:
   case SRC_TEXTURE1:
   case SRC_TEXTURE2:
   case SRC_TEXTURE3:
   case SRC_TEXTURE4:
   case SRC_TEXTURE5:
   case SRC_TEXTURE6:
   case SRC_TEXTURE7:       
      load_texture(p, src - SRC_TEXTURE0);
      break;
      
   default:
      /* not a texture src - do nothing */
      break;
   }
 
   return GL_TRUE;
}


/**
 * Generate instructions for loading all texture source terms.
 */
static GLboolean
load_texunit_sources( struct texenv_fragment_program *p, GLuint unit )
{
   const struct state_key *key = p->state;
   GLuint i;

   for (i = 0; i < key->unit[unit].NumArgsRGB; i++) {
      load_texenv_source( p, key->unit[unit].OptRGB[i].Source, unit );
   }

   for (i = 0; i < key->unit[unit].NumArgsA; i++) {
      load_texenv_source( p, key->unit[unit].OptA[i].Source, unit );
   }

   return GL_TRUE;
}

/**
 * Generate instructions for loading bump map textures.
 */
static GLboolean
load_texunit_bumpmap( struct texenv_fragment_program *p, GLuint unit )
{
   const struct state_key *key = p->state;
   GLuint bumpedUnitNr = key->unit[unit].OptRGB[1].Source - SRC_TEXTURE0;
   struct ureg texcDst, bumpMapRes;
   struct ureg constdudvcolor = register_const4f(p, 0.0, 0.0, 0.0, 1.0);
   struct ureg texcSrc = register_input(p, FRAG_ATTRIB_TEX0 + bumpedUnitNr);
   struct ureg rotMat0 = register_param3( p, STATE_INTERNAL, STATE_ROT_MATRIX_0, unit );
   struct ureg rotMat1 = register_param3( p, STATE_INTERNAL, STATE_ROT_MATRIX_1, unit );

   load_texenv_source( p, unit + SRC_TEXTURE0, unit );

   bumpMapRes = get_source(p, key->unit[unit].OptRGB[0].Source, unit);
   texcDst = get_tex_temp( p );
   p->texcoord_tex[bumpedUnitNr] = texcDst;

   /* Apply rot matrix and add coords to be available in next phase.
    * dest = (Arg0.xxxx * rotMat0 + Arg1) + (Arg0.yyyy * rotMat1)
    * note only 2 coords are affected the rest are left unchanged (mul by 0)
    */
   emit_arith( p, OPCODE_MAD, texcDst, WRITEMASK_XYZW, 0,
               swizzle1(bumpMapRes, SWIZZLE_X), rotMat0, texcSrc );
   emit_arith( p, OPCODE_MAD, texcDst, WRITEMASK_XYZW, 0,
               swizzle1(bumpMapRes, SWIZZLE_Y), rotMat1, texcDst );

   /* Move 0,0,0,1 into bumpmap src if someone (crossbar) is foolish
    * enough to access this later, should optimize away.
    */
   emit_arith( p, OPCODE_MOV, bumpMapRes, WRITEMASK_XYZW, 0,
               constdudvcolor, undef, undef );

   return GL_TRUE;
}

/**
 * Generate a new fragment program which implements the context's
 * current texture env/combine mode.
 */
static void
create_new_program(GLcontext *ctx, struct state_key *key,
                   struct gl_fragment_program *program)
{
   struct prog_instruction instBuffer[MAX_INSTRUCTIONS];
   struct texenv_fragment_program p;
   GLuint unit;
   struct ureg cf, out;
   int i;

   memset(&p, 0, sizeof(p));
   p.state = key;
   p.program = program;

   /* During code generation, use locally-allocated instruction buffer,
    * then alloc dynamic storage below.
    */
   p.program->Base.Instructions = instBuffer;
   p.program->Base.Target = GL_FRAGMENT_PROGRAM_ARB;
   p.program->Base.String = NULL;
   p.program->Base.NumTexIndirections = 1; /* is this right? */
   p.program->Base.NumTexInstructions = 0;
   p.program->Base.NumAluInstructions = 0;
   p.program->Base.NumInstructions = 0;
   p.program->Base.NumTemporaries = 0;
   p.program->Base.NumParameters = 0;
   p.program->Base.NumAttributes = 0;
   p.program->Base.NumAddressRegs = 0;
   p.program->Base.Parameters = _mesa_new_parameter_list();
   p.program->Base.InputsRead = 0x0;

   if (ctx->DrawBuffer->_NumColorDrawBuffers == 1)
      p.program->Base.OutputsWritten = 1 << FRAG_RESULT_COLOR;
   else {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++)
	 p.program->Base.OutputsWritten |= (1 << (FRAG_RESULT_DATA0 + i));
   }

   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      p.src_texture[unit] = undef;
      p.texcoord_tex[unit] = undef;
   }

   p.src_previous = undef;
   p.half = undef;
   p.zero = undef;
   p.one = undef;

   p.last_tex_stage = 0;
   release_temps(ctx, &p);

   if (key->enabled_units) {
      GLboolean needbumpstage = GL_FALSE;

      /* Zeroth pass - bump map textures first */
      for (unit = 0; unit < key->nr_enabled_units; unit++)
	 if (key->unit[unit].enabled &&
             key->unit[unit].ModeRGB == MODE_BUMP_ENVMAP_ATI) {
	    needbumpstage = GL_TRUE;
	    load_texunit_bumpmap( &p, unit );
	 }
      if (needbumpstage)
	 p.program->Base.NumTexIndirections++;

      /* First pass - to support texture_env_crossbar, first identify
       * all referenced texture sources and emit texld instructions
       * for each:
       */
      for (unit = 0; unit < key->nr_enabled_units; unit++)
	 if (key->unit[unit].enabled) {
	    load_texunit_sources( &p, unit );
	    p.last_tex_stage = unit;
	 }

      /* Second pass - emit combine instructions to build final color:
       */
      for (unit = 0; unit < key->nr_enabled_units; unit++)
	 if (key->unit[unit].enabled) {
	    p.src_previous = emit_texenv( &p, unit );
            reserve_temp(&p, p.src_previous); /* don't re-use this temp reg */
	    release_temps(ctx, &p);	/* release all temps */
	 }
   }

   cf = get_source( &p, SRC_PREVIOUS, 0 );

   for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
      if (ctx->DrawBuffer->_NumColorDrawBuffers == 1)
	 out = make_ureg( PROGRAM_OUTPUT, FRAG_RESULT_COLOR );
      else {
	 out = make_ureg( PROGRAM_OUTPUT, FRAG_RESULT_DATA0 + i );
      }

      if (key->separate_specular) {
	 /* Emit specular add.
	  */
	 struct ureg s = register_input(&p, FRAG_ATTRIB_COL1);
	 emit_arith( &p, OPCODE_ADD, out, WRITEMASK_XYZ, 0, cf, s, undef );
	 emit_arith( &p, OPCODE_MOV, out, WRITEMASK_W, 0, cf, undef, undef );
      }
      else if (memcmp(&cf, &out, sizeof(cf)) != 0) {
	 /* Will wind up in here if no texture enabled or a couple of
	  * other scenarios (GL_REPLACE for instance).
	  */
	 emit_arith( &p, OPCODE_MOV, out, WRITEMASK_XYZW, 0, cf, undef, undef );
      }
   }
   /* Finish up:
    */
   emit_arith( &p, OPCODE_END, undef, WRITEMASK_XYZW, 0, undef, undef, undef);

   if (key->fog_enabled) {
      /* Pull fog mode from GLcontext, the value in the state key is
       * a reduced value and not what is expected in FogOption
       */
      p.program->FogOption = ctx->Fog.Mode;
      p.program->Base.InputsRead |= FRAG_BIT_FOGC;
   }
   else {
      p.program->FogOption = GL_NONE;
   }

   if (p.program->Base.NumTexIndirections > ctx->Const.FragmentProgram.MaxTexIndirections) 
      program_error(&p, "Exceeded max nr indirect texture lookups");

   if (p.program->Base.NumTexInstructions > ctx->Const.FragmentProgram.MaxTexInstructions)
      program_error(&p, "Exceeded max TEX instructions");

   if (p.program->Base.NumAluInstructions > ctx->Const.FragmentProgram.MaxAluInstructions)
      program_error(&p, "Exceeded max ALU instructions");

   ASSERT(p.program->Base.NumInstructions <= MAX_INSTRUCTIONS);

   /* Allocate final instruction array */
   p.program->Base.Instructions
      = _mesa_alloc_instructions(p.program->Base.NumInstructions);
   if (!p.program->Base.Instructions) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "generating tex env program");
      return;
   }
   _mesa_copy_instructions(p.program->Base.Instructions, instBuffer,
                           p.program->Base.NumInstructions);

   if (p.program->FogOption) {
      _mesa_append_fog_code(ctx, p.program);
      p.program->FogOption = GL_NONE;
   }


   /* Notify driver the fragment program has (actually) changed.
    */
   if (ctx->Driver.ProgramStringNotify) {
      GLboolean ok = ctx->Driver.ProgramStringNotify(ctx,
                                                     GL_FRAGMENT_PROGRAM_ARB, 
                                                     &p.program->Base);
      /* Driver should be able to handle any texenv programs as long as
       * the driver correctly reported max number of texture units correctly,
       * etc.
       */
      ASSERT(ok);
      (void) ok; /* silence unused var warning */
   }

   if (DISASSEM) {
      _mesa_print_program(&p.program->Base);
      printf("\n");
   }
}


/**
 * Return a fragment program which implements the current
 * fixed-function texture, fog and color-sum operations.
 */
struct gl_fragment_program *
_mesa_get_fixed_func_fragment_program(GLcontext *ctx)
{
   struct gl_fragment_program *prog;
   struct state_key key;
   GLuint keySize;
	
   keySize = make_state_key(ctx, &key);
      
   prog = (struct gl_fragment_program *)
      _mesa_search_program_cache(ctx->FragmentProgram.Cache,
                                 &key, keySize);

   if (!prog) {
      prog = (struct gl_fragment_program *) 
         ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);

      create_new_program(ctx, &key, prog);

      _mesa_program_cache_insert(ctx, ctx->FragmentProgram.Cache,
                                 &key, keySize, &prog->Base);
   }

   return prog;
}
