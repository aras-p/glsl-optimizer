/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/** 
 * \file texstate.c
 *
 * Texture state handling.
 */

#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "colortab.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "shaderimage.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "mtypes.h"
#include "bitset.h"


/**
 * Default texture combine environment state.  This is used to initialize
 * a context's texture units and as the basis for converting "classic"
 * texture environmnets to ARB_texture_env_combine style values.
 */
static const struct gl_tex_env_combine_state default_combine_state = {
   GL_MODULATE, GL_MODULATE,
   { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT, GL_CONSTANT },
   { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT, GL_CONSTANT },
   { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_ALPHA, GL_SRC_ALPHA },
   { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA },
   0, 0,
   2, 2
};



/**
 * Used by glXCopyContext to copy texture state from one context to another.
 */
void
_mesa_copy_texture_state( const struct gl_context *src, struct gl_context *dst )
{
   GLuint u, tex;

   ASSERT(src);
   ASSERT(dst);

   dst->Texture.CurrentUnit = src->Texture.CurrentUnit;
   dst->Texture._GenFlags = src->Texture._GenFlags;
   dst->Texture._TexGenEnabled = src->Texture._TexGenEnabled;
   dst->Texture._TexMatEnabled = src->Texture._TexMatEnabled;

   /* per-unit state */
   for (u = 0; u < src->Const.MaxCombinedTextureImageUnits; u++) {
      dst->Texture.Unit[u].Enabled = src->Texture.Unit[u].Enabled;
      dst->Texture.Unit[u].EnvMode = src->Texture.Unit[u].EnvMode;
      COPY_4V(dst->Texture.Unit[u].EnvColor, src->Texture.Unit[u].EnvColor);
      dst->Texture.Unit[u].TexGenEnabled = src->Texture.Unit[u].TexGenEnabled;
      dst->Texture.Unit[u].GenS = src->Texture.Unit[u].GenS;
      dst->Texture.Unit[u].GenT = src->Texture.Unit[u].GenT;
      dst->Texture.Unit[u].GenR = src->Texture.Unit[u].GenR;
      dst->Texture.Unit[u].GenQ = src->Texture.Unit[u].GenQ;
      dst->Texture.Unit[u].LodBias = src->Texture.Unit[u].LodBias;

      /* GL_EXT_texture_env_combine */
      dst->Texture.Unit[u].Combine = src->Texture.Unit[u].Combine;

      /* GL_ATI_envmap_bumpmap - need this? */
      dst->Texture.Unit[u].BumpTarget = src->Texture.Unit[u].BumpTarget;
      COPY_4V(dst->Texture.Unit[u].RotMatrix, src->Texture.Unit[u].RotMatrix);

      /*
       * XXX strictly speaking, we should compare texture names/ids and
       * bind textures in the dest context according to id.  For now, only
       * copy bindings if the contexts share the same pool of textures to
       * avoid refcounting bugs.
       */
      if (dst->Shared == src->Shared) {
         /* copy texture object bindings, not contents of texture objects */
         _mesa_lock_context_textures(dst);

         for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
            _mesa_reference_texobj(&dst->Texture.Unit[u].CurrentTex[tex],
                                   src->Texture.Unit[u].CurrentTex[tex]);
            if (src->Texture.Unit[u].CurrentTex[tex]) {
               dst->Texture.NumCurrentTexUsed =
                  MAX2(dst->Texture.NumCurrentTexUsed, u + 1);
            }
         }
         dst->Texture.Unit[u]._BoundTextures = src->Texture.Unit[u]._BoundTextures;
         _mesa_unlock_context_textures(dst);
      }
   }
}


/*
 * For debugging
 */
void
_mesa_print_texunit_state( struct gl_context *ctx, GLuint unit )
{
   const struct gl_texture_unit *texUnit = ctx->Texture.Unit + unit;
   printf("Texture Unit %d\n", unit);
   printf("  GL_TEXTURE_ENV_MODE = %s\n", _mesa_lookup_enum_by_nr(texUnit->EnvMode));
   printf("  GL_COMBINE_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeRGB));
   printf("  GL_COMBINE_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeA));
   printf("  GL_SOURCE0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[0]));
   printf("  GL_SOURCE1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[1]));
   printf("  GL_SOURCE2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[2]));
   printf("  GL_SOURCE0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[0]));
   printf("  GL_SOURCE1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[1]));
   printf("  GL_SOURCE2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[2]));
   printf("  GL_OPERAND0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[0]));
   printf("  GL_OPERAND1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[1]));
   printf("  GL_OPERAND2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[2]));
   printf("  GL_OPERAND0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[0]));
   printf("  GL_OPERAND1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[1]));
   printf("  GL_OPERAND2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[2]));
   printf("  GL_RGB_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftRGB);
   printf("  GL_ALPHA_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftA);
   printf("  GL_TEXTURE_ENV_COLOR = (%f, %f, %f, %f)\n", texUnit->EnvColor[0], texUnit->EnvColor[1], texUnit->EnvColor[2], texUnit->EnvColor[3]);
}



/**********************************************************************/
/*                       Texture Environment                          */
/**********************************************************************/

/**
 * Convert "classic" texture environment to ARB_texture_env_combine style
 * environments.
 * 
 * \param state  texture_env_combine state vector to be filled-in.
 * \param mode   Classic texture environment mode (i.e., \c GL_REPLACE,
 *               \c GL_BLEND, \c GL_DECAL, etc.).
 * \param texBaseFormat  Base format of the texture associated with the
 *               texture unit.
 */
static void
calculate_derived_texenv( struct gl_tex_env_combine_state *state,
			  GLenum mode, GLenum texBaseFormat )
{
   GLenum mode_rgb;
   GLenum mode_a;

   *state = default_combine_state;

   switch (texBaseFormat) {
   case GL_ALPHA:
      state->SourceRGB[0] = GL_PREVIOUS;
      break;

   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_RGBA:
      break;

   case GL_LUMINANCE:
   case GL_RED:
   case GL_RG:
   case GL_RGB:
   case GL_YCBCR_MESA:
   case GL_DUDV_ATI:
      state->SourceA[0] = GL_PREVIOUS;
      break;
      
   default:
      _mesa_problem(NULL,
                    "Invalid texBaseFormat 0x%x in calculate_derived_texenv",
                    texBaseFormat);
      return;
   }

   if (mode == GL_REPLACE_EXT)
      mode = GL_REPLACE;

   switch (mode) {
   case GL_REPLACE:
   case GL_MODULATE:
      mode_rgb = (texBaseFormat == GL_ALPHA) ? GL_REPLACE : mode;
      mode_a   = mode;
      break;
   
   case GL_DECAL:
      mode_rgb = GL_INTERPOLATE;
      mode_a   = GL_REPLACE;

      state->SourceA[0] = GL_PREVIOUS;

      /* Having alpha / luminance / intensity textures replace using the
       * incoming fragment color matches the definition in NV_texture_shader.
       * The 1.5 spec simply marks these as "undefined".
       */
      switch (texBaseFormat) {
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_INTENSITY:
	 state->SourceRGB[0] = GL_PREVIOUS;
	 break;
      case GL_RED:
      case GL_RG:
      case GL_RGB:
      case GL_YCBCR_MESA:
      case GL_DUDV_ATI:
	 mode_rgb = GL_REPLACE;
	 break;
      case GL_RGBA:
	 state->SourceRGB[2] = GL_TEXTURE;
	 break;
      }
      break;

   case GL_BLEND:
      mode_rgb = GL_INTERPOLATE;
      mode_a   = GL_MODULATE;

      switch (texBaseFormat) {
      case GL_ALPHA:
	 mode_rgb = GL_REPLACE;
	 break;
      case GL_INTENSITY:
	 mode_a = GL_INTERPOLATE;
	 state->SourceA[0] = GL_CONSTANT;
	 state->OperandA[2] = GL_SRC_ALPHA;
	 /* FALLTHROUGH */
      case GL_LUMINANCE:
      case GL_RED:
      case GL_RG:
      case GL_RGB:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
      case GL_YCBCR_MESA:
      case GL_DUDV_ATI:
	 state->SourceRGB[2] = GL_TEXTURE;
	 state->SourceA[2]   = GL_TEXTURE;
	 state->SourceRGB[0] = GL_CONSTANT;
	 state->OperandRGB[2] = GL_SRC_COLOR;
	 break;
      }
      break;

   case GL_ADD:
      mode_rgb = (texBaseFormat == GL_ALPHA) ? GL_REPLACE : GL_ADD;
      mode_a   = (texBaseFormat == GL_INTENSITY) ? GL_ADD : GL_MODULATE;
      break;

   default:
      _mesa_problem(NULL,
                    "Invalid texture env mode 0x%x in calculate_derived_texenv",
                    mode);
      return;
   }
   
   state->ModeRGB = (state->SourceRGB[0] != GL_PREVIOUS)
       ? mode_rgb : GL_REPLACE;
   state->ModeA   = (state->SourceA[0]   != GL_PREVIOUS)
       ? mode_a   : GL_REPLACE;
}




/* GL_ARB_multitexture */
void GLAPIENTRY
_mesa_ActiveTexture(GLenum texture)
{
   const GLuint texUnit = texture - GL_TEXTURE0;
   GLuint k;
   GET_CURRENT_CONTEXT(ctx);

   /* See OpenGL spec for glActiveTexture: */
   k = MAX2(ctx->Const.MaxCombinedTextureImageUnits,
            ctx->Const.MaxTextureCoordUnits);

   ASSERT(k <= Elements(ctx->Texture.Unit));

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glActiveTexture %s\n",
                  _mesa_lookup_enum_by_nr(texture));

   if (texUnit >= k) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glActiveTexture(texture=%s)",
                  _mesa_lookup_enum_by_nr(texture));
      return;
   }

   if (ctx->Texture.CurrentUnit == texUnit)
      return;

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   ctx->Texture.CurrentUnit = texUnit;
   if (ctx->Transform.MatrixMode == GL_TEXTURE) {
      /* update current stack pointer */
      ctx->CurrentStack = &ctx->TextureMatrixStack[texUnit];
   }
}


/* GL_ARB_multitexture */
void GLAPIENTRY
_mesa_ClientActiveTexture(GLenum texture)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint texUnit = texture - GL_TEXTURE0;

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glClientActiveTexture %s\n",
                  _mesa_lookup_enum_by_nr(texture));

   if (texUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClientActiveTexture(texture)");
      return;
   }

   if (ctx->Array.ActiveTexture == texUnit)
      return;

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ActiveTexture = texUnit;
}



/**********************************************************************/
/*****                    State management                        *****/
/**********************************************************************/


/**
 * \note This routine refers to derived texture attribute values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE_MATRIX.  On changes to _NEW_TEXTURE, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_textures(), below.
 *
 * \param ctx GL context.
 */
static void
update_texture_matrices( struct gl_context *ctx )
{
   GLuint u;

   ctx->Texture._TexMatEnabled = 0x0;

   for (u = 0; u < ctx->Const.MaxTextureCoordUnits; u++) {
      ASSERT(u < Elements(ctx->TextureMatrixStack));
      if (_math_matrix_is_dirty(ctx->TextureMatrixStack[u].Top)) {
	 _math_matrix_analyse( ctx->TextureMatrixStack[u].Top );

	 if (ctx->Texture.Unit[u]._Current &&
	     ctx->TextureMatrixStack[u].Top->type != MATRIX_IDENTITY)
	    ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(u);
      }
   }
}


/**
 * Examine texture unit's combine/env state to update derived state.
 */
static void
update_tex_combine(struct gl_context *ctx, struct gl_texture_unit *texUnit)
{
   struct gl_tex_env_combine_state *combine;

   /* No combiners will apply to this. */
   if (texUnit->_Current->Target == GL_TEXTURE_BUFFER)
      return;

   /* Set the texUnit->_CurrentCombine field to point to the user's combiner
    * state, or the combiner state which is derived from traditional texenv
    * mode.
    */
   if (texUnit->EnvMode == GL_COMBINE ||
       texUnit->EnvMode == GL_COMBINE4_NV) {
      texUnit->_CurrentCombine = & texUnit->Combine;
   }
   else {
      const struct gl_texture_object *texObj = texUnit->_Current;
      GLenum format = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;

      if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL_EXT) {
         format = texObj->DepthMode;
      }
      calculate_derived_texenv(&texUnit->_EnvMode, texUnit->EnvMode, format);
      texUnit->_CurrentCombine = & texUnit->_EnvMode;
   }

   combine = texUnit->_CurrentCombine;

   /* Determine number of source RGB terms in the combiner function */
   switch (combine->ModeRGB) {
   case GL_REPLACE:
      combine->_NumArgsRGB = 1;
      break;
   case GL_ADD:
   case GL_ADD_SIGNED:
      if (texUnit->EnvMode == GL_COMBINE4_NV)
         combine->_NumArgsRGB = 4;
      else
         combine->_NumArgsRGB = 2;
      break;
   case GL_MODULATE:
   case GL_SUBTRACT:
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGBA_EXT:
      combine->_NumArgsRGB = 2;
      break;
   case GL_INTERPOLATE:
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      combine->_NumArgsRGB = 3;
      break;
   case GL_BUMP_ENVMAP_ATI:
      /* no real arguments for this case */
      combine->_NumArgsRGB = 0;
      break;
   default:
      combine->_NumArgsRGB = 0;
      _mesa_problem(ctx, "invalid RGB combine mode in update_texture_state");
      return;
   }

   /* Determine number of source Alpha terms in the combiner function */
   switch (combine->ModeA) {
   case GL_REPLACE:
      combine->_NumArgsA = 1;
      break;
   case GL_ADD:
   case GL_ADD_SIGNED:
      if (texUnit->EnvMode == GL_COMBINE4_NV)
         combine->_NumArgsA = 4;
      else
         combine->_NumArgsA = 2;
      break;
   case GL_MODULATE:
   case GL_SUBTRACT:
      combine->_NumArgsA = 2;
      break;
   case GL_INTERPOLATE:
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      combine->_NumArgsA = 3;
      break;
   default:
      combine->_NumArgsA = 0;
      _mesa_problem(ctx, "invalid Alpha combine mode in update_texture_state");
      break;
   }
}

static void
update_texgen(struct gl_context *ctx)
{
   GLuint unit;

   /* Setup texgen for those texture coordinate sets that are in use */
   for (unit = 0; unit < ctx->Const.MaxTextureCoordUnits; unit++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

      texUnit->_GenFlags = 0x0;

      if (!(ctx->Texture._EnabledCoordUnits & (1 << unit)))
	 continue;

      if (texUnit->TexGenEnabled) {
	 if (texUnit->TexGenEnabled & S_BIT) {
	    texUnit->_GenFlags |= texUnit->GenS._ModeBit;
	 }
	 if (texUnit->TexGenEnabled & T_BIT) {
	    texUnit->_GenFlags |= texUnit->GenT._ModeBit;
	 }
	 if (texUnit->TexGenEnabled & R_BIT) {
	    texUnit->_GenFlags |= texUnit->GenR._ModeBit;
	 }
	 if (texUnit->TexGenEnabled & Q_BIT) {
	    texUnit->_GenFlags |= texUnit->GenQ._ModeBit;
	 }

	 ctx->Texture._TexGenEnabled |= ENABLE_TEXGEN(unit);
	 ctx->Texture._GenFlags |= texUnit->_GenFlags;
      }

      ASSERT(unit < Elements(ctx->TextureMatrixStack));
      if (ctx->TextureMatrixStack[unit].Top->type != MATRIX_IDENTITY)
	 ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(unit);
   }
}

static struct gl_texture_object *
update_single_program_texture(struct gl_context *ctx, struct gl_program *prog,
                              int s)
{
   gl_texture_index target_index;
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_sampler_object *sampler;
   int unit;

   if (!(prog->SamplersUsed & (1 << s)))
      return NULL;

   unit = prog->SamplerUnits[s];
   texUnit = &ctx->Texture.Unit[unit];

   /* Note: If more than one bit was set in TexturesUsed[unit], then we should
    * have had the draw call rejected already.  From the GL 4.4 specification,
    * section 7.10 ("Samplers"):
    *
    *     "It is not allowed to have variables of different sampler types
    *      pointing to the same texture image unit within a program
    *      object. This situation can only be detected at the next rendering
    *      command issued which triggers shader invocations, and an
    *      INVALID_OPERATION error will then be generated."
    */
   target_index = ffs(prog->TexturesUsed[unit]) - 1;
   texObj = texUnit->CurrentTex[target_index];

   sampler = texUnit->Sampler ?
      texUnit->Sampler : &texObj->Sampler;

   if (likely(texObj)) {
      if (_mesa_is_texture_complete(texObj, sampler))
         return texObj;

      _mesa_test_texobj_completeness(ctx, texObj);
      if (_mesa_is_texture_complete(texObj, sampler))
         return texObj;
   }

   /* If we've reached this point, we didn't find a complete texture of the
    * shader's target.  From the GL 4.4 core specification, section 11.1.3.5
    * ("Texture Access"):
    *
    *     "If a sampler is used in a shader and the sampler’s associated
    *      texture is not complete, as defined in section 8.17, (0, 0, 0, 1)
    *      will be returned for a non-shadow sampler and 0 for a shadow
    *      sampler."
    *
    * Mesa implements this by creating a hidden texture object with a pixel of
    * that value.
    */
   texObj = _mesa_get_fallback_texture(ctx, target_index);
   assert(texObj);

   return texObj;
}

static void
update_program_texture_state(struct gl_context *ctx, struct gl_program **prog,
                             BITSET_WORD *enabled_texture_units)
{
   int i;

   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      int s;

      if (!prog[i])
         continue;

      /* We can't only do the shifting trick as the loop condition because if
       * sampler 31 is active, the next iteration tries to shift by 32, which is
       * undefined.
       */
      for (s = 0; s < MAX_SAMPLERS && (1 << s) <= prog[i]->SamplersUsed; s++) {
         struct gl_texture_object *texObj;

         texObj = update_single_program_texture(ctx, prog[i], s);
         if (texObj) {
            int unit = prog[i]->SamplerUnits[s];
            _mesa_reference_texobj(&ctx->Texture.Unit[unit]._Current, texObj);
            BITSET_SET(enabled_texture_units, unit);
            ctx->Texture._MaxEnabledTexImageUnit =
               MAX2(ctx->Texture._MaxEnabledTexImageUnit, (int)unit);
         }
      }
   }

   if (prog[MESA_SHADER_FRAGMENT]) {
      const GLuint coordMask = (1 << MAX_TEXTURE_COORD_UNITS) - 1;
      ctx->Texture._EnabledCoordUnits |=
         (prog[MESA_SHADER_FRAGMENT]->InputsRead >> VARYING_SLOT_TEX0) &
         coordMask;
   }
}

static void
update_ff_texture_state(struct gl_context *ctx,
                        BITSET_WORD *enabled_texture_units)
{
   int unit;

   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
      GLuint texIndex;

      if (texUnit->Enabled == 0x0)
         continue;

      /* If a shader already dictated what texture target was used for this
       * unit, just go along with it.
       */
      if (BITSET_TEST(enabled_texture_units, unit))
         continue;

      /* From the GL 4.4 compat specification, section 16.2 ("Texture Application"):
       *
       *     "Texturing is enabled or disabled using the generic Enable and
       *      Disable commands, respectively, with the symbolic constants
       *      TEXTURE_1D, TEXTURE_2D, TEXTURE_RECTANGLE, TEXTURE_3D, or
       *      TEXTURE_CUBE_MAP to enable the one-, two-, rectangular,
       *      three-dimensional, or cube map texture, respectively. If more
       *      than one of these textures is enabled, the first one enabled
       *      from the following list is used:
       *
       *      • cube map texture
       *      • three-dimensional texture
       *      • rectangular texture
       *      • two-dimensional texture
       *      • one-dimensional texture"
       *
       * Note that the TEXTURE_x_INDEX values are in high to low priority.
       * Also:
       *
       *     "If a texture unit is disabled or has an invalid or incomplete
       *      texture (as defined in section 8.17) bound to it, then blending
       *      is disabled for that texture unit. If the texture environment
       *      for a given enabled texture unit references a disabled texture
       *      unit, or an invalid or incomplete texture that is bound to
       *      another unit, then the results of texture blending are
       *      undefined."
       */
      for (texIndex = 0; texIndex < NUM_TEXTURE_TARGETS; texIndex++) {
         if (texUnit->Enabled & (1 << texIndex)) {
            struct gl_texture_object *texObj = texUnit->CurrentTex[texIndex];
            struct gl_sampler_object *sampler = texUnit->Sampler ?
               texUnit->Sampler : &texObj->Sampler;

            if (!_mesa_is_texture_complete(texObj, sampler)) {
               _mesa_test_texobj_completeness(ctx, texObj);
            }
            if (_mesa_is_texture_complete(texObj, sampler)) {
               _mesa_reference_texobj(&texUnit->_Current, texObj);
               break;
            }
         }
      }

      if (texIndex == NUM_TEXTURE_TARGETS)
         continue;

      /* if we get here, we know this texture unit is enabled */
      BITSET_SET(enabled_texture_units, unit);
      ctx->Texture._MaxEnabledTexImageUnit =
         MAX2(ctx->Texture._MaxEnabledTexImageUnit, (int)unit);

      ctx->Texture._EnabledCoordUnits |= 1 << unit;

      update_tex_combine(ctx, texUnit);
   }
}

/**
 * \note This routine refers to derived texture matrix values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE.  On changes to _NEW_TEXTURE_MATRIX, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_texture_matrices, above.
 *
 * \param ctx GL context.
 */
static void
update_texture_state( struct gl_context *ctx )
{
   struct gl_program *prog[MESA_SHADER_STAGES];
   int i;
   int old_max_unit = ctx->Texture._MaxEnabledTexImageUnit;
   BITSET_DECLARE(enabled_texture_units, MAX_COMBINED_TEXTURE_IMAGE_UNITS);

   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (ctx->_Shader->CurrentProgram[i] &&
          ctx->_Shader->CurrentProgram[i]->LinkStatus) {
         prog[i] = ctx->_Shader->CurrentProgram[i]->_LinkedShaders[i]->Program;
      } else {
         if (i == MESA_SHADER_FRAGMENT && ctx->FragmentProgram._Enabled)
            prog[i] = &ctx->FragmentProgram.Current->Base;
         else
            prog[i] = NULL;
      }
   }

   /* TODO: only set this if there are actual changes */
   ctx->NewState |= _NEW_TEXTURE;

   ctx->Texture._GenFlags = 0x0;
   ctx->Texture._TexMatEnabled = 0x0;
   ctx->Texture._TexGenEnabled = 0x0;
   ctx->Texture._MaxEnabledTexImageUnit = -1;
   ctx->Texture._EnabledCoordUnits = 0x0;

   memset(&enabled_texture_units, 0, sizeof(enabled_texture_units));

   /* First, walk over our programs pulling in all the textures for them.
    * Programs dictate specific texture targets to be enabled, and for a draw
    * call to be valid they can't conflict about which texture targets are
    * used.
    */
   update_program_texture_state(ctx, prog, enabled_texture_units);

   /* Also pull in any textures necessary for fixed function fragment shading.
    */
   if (!prog[MESA_SHADER_FRAGMENT])
      update_ff_texture_state(ctx, enabled_texture_units);

   /* Now, clear out the _Current of any disabled texture units. */
   for (i = 0; i <= ctx->Texture._MaxEnabledTexImageUnit; i++) {
      if (!BITSET_TEST(enabled_texture_units, i))
         _mesa_reference_texobj(&ctx->Texture.Unit[i]._Current, NULL);
   }
   for (i = ctx->Texture._MaxEnabledTexImageUnit + 1; i <= old_max_unit; i++) {
      _mesa_reference_texobj(&ctx->Texture.Unit[i]._Current, NULL);
   }

   if (!prog[MESA_SHADER_FRAGMENT] || !prog[MESA_SHADER_VERTEX])
      update_texgen(ctx);

   _mesa_validate_image_units(ctx);
}


/**
 * Update texture-related derived state.
 */
void
_mesa_update_texture( struct gl_context *ctx, GLuint new_state )
{
   if (new_state & _NEW_TEXTURE_MATRIX)
      update_texture_matrices( ctx );

   if (new_state & (_NEW_TEXTURE | _NEW_PROGRAM))
      update_texture_state( ctx );
}


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

/**
 * Allocate the proxy textures for the given context.
 * 
 * \param ctx the context to allocate proxies for.
 * 
 * \return GL_TRUE on success, or GL_FALSE on failure
 * 
 * If run out of memory part way through the allocations, clean up and return
 * GL_FALSE.
 */
static GLboolean
alloc_proxy_textures( struct gl_context *ctx )
{
   /* NOTE: these values must be in the same order as the TEXTURE_x_INDEX
    * values!
    */
   static const GLenum targets[] = {
      GL_TEXTURE_2D_MULTISAMPLE,
      GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
      GL_TEXTURE_CUBE_MAP_ARRAY,
      GL_TEXTURE_BUFFER,
      GL_TEXTURE_2D_ARRAY_EXT,
      GL_TEXTURE_1D_ARRAY_EXT,
      GL_TEXTURE_EXTERNAL_OES,
      GL_TEXTURE_CUBE_MAP_ARB,
      GL_TEXTURE_3D,
      GL_TEXTURE_RECTANGLE_NV,
      GL_TEXTURE_2D,
      GL_TEXTURE_1D,
   };
   GLint tgt;

   STATIC_ASSERT(Elements(targets) == NUM_TEXTURE_TARGETS);
   assert(targets[TEXTURE_2D_INDEX] == GL_TEXTURE_2D);
   assert(targets[TEXTURE_CUBE_INDEX] == GL_TEXTURE_CUBE_MAP);

   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      if (!(ctx->Texture.ProxyTex[tgt]
            = ctx->Driver.NewTextureObject(ctx, 0, targets[tgt]))) {
         /* out of memory, free what we did allocate */
         while (--tgt >= 0) {
            ctx->Driver.DeleteTexture(ctx, ctx->Texture.ProxyTex[tgt]);
         }
         return GL_FALSE;
      }
   }

   assert(ctx->Texture.ProxyTex[0]->RefCount == 1); /* sanity check */
   return GL_TRUE;
}


/**
 * Initialize a texture unit.
 *
 * \param ctx GL context.
 * \param unit texture unit number to be initialized.
 */
static void
init_texture_unit( struct gl_context *ctx, GLuint unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint tex;

   texUnit->EnvMode = GL_MODULATE;
   ASSIGN_4V( texUnit->EnvColor, 0.0, 0.0, 0.0, 0.0 );

   texUnit->Combine = default_combine_state;
   texUnit->_EnvMode = default_combine_state;
   texUnit->_CurrentCombine = & texUnit->_EnvMode;
   texUnit->BumpTarget = GL_TEXTURE0;

   texUnit->TexGenEnabled = 0x0;
   texUnit->GenS.Mode = GL_EYE_LINEAR;
   texUnit->GenT.Mode = GL_EYE_LINEAR;
   texUnit->GenR.Mode = GL_EYE_LINEAR;
   texUnit->GenQ.Mode = GL_EYE_LINEAR;
   texUnit->GenS._ModeBit = TEXGEN_EYE_LINEAR;
   texUnit->GenT._ModeBit = TEXGEN_EYE_LINEAR;
   texUnit->GenR._ModeBit = TEXGEN_EYE_LINEAR;
   texUnit->GenQ._ModeBit = TEXGEN_EYE_LINEAR;

   /* Yes, these plane coefficients are correct! */
   ASSIGN_4V( texUnit->GenS.ObjectPlane, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenT.ObjectPlane, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenR.ObjectPlane, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenQ.ObjectPlane, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenS.EyePlane, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenT.EyePlane, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenR.EyePlane, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenQ.EyePlane, 0.0, 0.0, 0.0, 0.0 );

   /* no mention of this in spec, but maybe id matrix expected? */
   ASSIGN_4V( texUnit->RotMatrix, 1.0, 0.0, 0.0, 1.0 );

   /* initialize current texture object ptrs to the shared default objects */
   for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
      _mesa_reference_texobj(&texUnit->CurrentTex[tex],
                             ctx->Shared->DefaultTex[tex]);
   }

   texUnit->_BoundTextures = 0;
}


/**
 * Initialize texture state for the given context.
 */
GLboolean
_mesa_init_texture(struct gl_context *ctx)
{
   GLuint u;

   /* Texture group */
   ctx->Texture.CurrentUnit = 0;      /* multitexture */

   /* Appendix F.2 of the OpenGL ES 3.0 spec says:
    *
    *     "OpenGL ES 3.0 requires that all cube map filtering be
    *     seamless. OpenGL ES 2.0 specified that a single cube map face be
    *     selected and used for filtering."
    */
   ctx->Texture.CubeMapSeamless = _mesa_is_gles3(ctx);

   for (u = 0; u < Elements(ctx->Texture.Unit); u++)
      init_texture_unit(ctx, u);

   /* After we're done initializing the context's texture state the default
    * texture objects' refcounts should be at least
    * MAX_COMBINED_TEXTURE_IMAGE_UNITS + 1.
    */
   assert(ctx->Shared->DefaultTex[TEXTURE_1D_INDEX]->RefCount
          >= MAX_COMBINED_TEXTURE_IMAGE_UNITS + 1);

   /* Allocate proxy textures */
   if (!alloc_proxy_textures( ctx ))
      return GL_FALSE;

   /* GL_ARB_texture_buffer_object */
   _mesa_reference_buffer_object(ctx, &ctx->Texture.BufferObject,
                                 ctx->Shared->NullBufferObj);

   ctx->Texture.NumCurrentTexUsed = 0;

   return GL_TRUE;
}


/**
 * Free dynamically-allocted texture data attached to the given context.
 */
void
_mesa_free_texture_data(struct gl_context *ctx)
{
   GLuint u, tgt;

   /* unreference current textures */
   for (u = 0; u < Elements(ctx->Texture.Unit); u++) {
      /* The _Current texture could account for another reference */
      _mesa_reference_texobj(&ctx->Texture.Unit[u]._Current, NULL);

      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
         _mesa_reference_texobj(&ctx->Texture.Unit[u].CurrentTex[tgt], NULL);
      }
   }

   /* Free proxy texture objects */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++)
      ctx->Driver.DeleteTexture(ctx, ctx->Texture.ProxyTex[tgt]);

   /* GL_ARB_texture_buffer_object */
   _mesa_reference_buffer_object(ctx, &ctx->Texture.BufferObject, NULL);

   for (u = 0; u < Elements(ctx->Texture.Unit); u++) {
      _mesa_reference_sampler_object(ctx, &ctx->Texture.Unit[u].Sampler, NULL);
   }
}


/**
 * Update the default texture objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_texture(struct gl_context *ctx)
{
   GLuint u, tex;

   for (u = 0; u < Elements(ctx->Texture.Unit); u++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[u];
      for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
         _mesa_reference_texobj(&texUnit->CurrentTex[tex],
                                ctx->Shared->DefaultTex[tex]);
      }
   }
}
