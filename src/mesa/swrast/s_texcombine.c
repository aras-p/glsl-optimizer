/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/colormac.h"
#include "main/image.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/pixel.h"
#include "shader/prog_instruction.h"

#include "s_context.h"
#include "s_texcombine.h"


#define MAX_COMBINER_TERMS 4


/**
 * Do texture application for GL_ARB/EXT_texture_env_combine.
 * This function also supports GL_{EXT,ARB}_texture_env_dot3 and
 * GL_ATI_texture_env_combine3.  Since "classic" texture environments are
 * implemented using GL_ARB_texture_env_combine-like state, this same function
 * is used for classic texture environment application as well.
 *
 * \param ctx          rendering context
 * \param textureUnit  the texture unit to apply
 * \param n            number of fragments to process (span width)
 * \param primary_rgba incoming fragment color array
 * \param texelBuffer  pointer to texel colors for all texture units
 * 
 * \param rgba         incoming colors, which get modified here
 */
static void
texture_combine( const GLcontext *ctx, GLuint unit, GLuint n,
                 CONST GLfloat (*primary_rgba)[4],
                 CONST GLfloat *texelBuffer,
                 GLchan (*rgbaChan)[4] )
{
   const struct gl_texture_unit *textureUnit = &(ctx->Texture.Unit[unit]);
   const GLfloat (*argRGB [MAX_COMBINER_TERMS])[4];
   const GLfloat (*argA [MAX_COMBINER_TERMS])[4];
   const GLint RGBshift = textureUnit->_CurrentCombine->ScaleShiftRGB;
   const GLuint Ashift   = textureUnit->_CurrentCombine->ScaleShiftA;
   const GLfloat RGBmult = (GLfloat) (1 << RGBshift);
   const GLfloat Amult = (GLfloat) (1 << Ashift);
   static const GLfloat one[4] = { 1, 1, 1, 1 };
   static const GLfloat zero[4] = { 0, 0, 0, 0 };
   const GLuint numColorArgs = textureUnit->_CurrentCombine->_NumArgsRGB;
   const GLuint numAlphaArgs = textureUnit->_CurrentCombine->_NumArgsA;
   GLfloat ccolor[MAX_COMBINER_TERMS][MAX_WIDTH][4]; /* temp color buffers */
   GLfloat rgba[MAX_WIDTH][4];
   GLuint i, j;

   ASSERT(ctx->Extensions.EXT_texture_env_combine ||
          ctx->Extensions.ARB_texture_env_combine);
   ASSERT(CONST_SWRAST_CONTEXT(ctx)->_AnyTextureCombine);

   for (i = 0; i < n; i++) {
      rgba[i][RCOMP] = CHAN_TO_FLOAT(rgbaChan[i][RCOMP]);
      rgba[i][GCOMP] = CHAN_TO_FLOAT(rgbaChan[i][GCOMP]);
      rgba[i][BCOMP] = CHAN_TO_FLOAT(rgbaChan[i][BCOMP]);
      rgba[i][ACOMP] = CHAN_TO_FLOAT(rgbaChan[i][ACOMP]);
   }

   /*
   printf("modeRGB 0x%x  modeA 0x%x  srcRGB1 0x%x  srcA1 0x%x  srcRGB2 0x%x  srcA2 0x%x\n",
          textureUnit->_CurrentCombine->ModeRGB,
          textureUnit->_CurrentCombine->ModeA,
          textureUnit->_CurrentCombine->SourceRGB[0],
          textureUnit->_CurrentCombine->SourceA[0],
          textureUnit->_CurrentCombine->SourceRGB[1],
          textureUnit->_CurrentCombine->SourceA[1]);
   */

   /*
    * Do operand setup for up to 4 operands.  Loop over the terms.
    */
   for (j = 0; j < numColorArgs; j++) {
      const GLenum srcRGB = textureUnit->_CurrentCombine->SourceRGB[j];

      switch (srcRGB) {
         case GL_TEXTURE:
            argRGB[j] = (const GLfloat (*)[4])
               (texelBuffer + unit * (n * 4 * sizeof(GLfloat)));
            break;
         case GL_PRIMARY_COLOR:
            argRGB[j] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argRGB[j] = (const GLfloat (*)[4]) rgba;
            break;
         case GL_CONSTANT:
            {
               GLfloat (*c)[4] = ccolor[j];
               GLfloat red   = textureUnit->EnvColor[0];
               GLfloat green = textureUnit->EnvColor[1];
               GLfloat blue  = textureUnit->EnvColor[2];
               GLfloat alpha = textureUnit->EnvColor[3];
               for (i = 0; i < n; i++) {
                  ASSIGN_4V(c[i], red, green, blue, alpha);
               }
               argRGB[j] = (const GLfloat (*)[4]) ccolor[j];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            {
               GLfloat (*c)[4] = ccolor[j];
               for (i = 0; i < n; i++) {
                  ASSIGN_4V(c[i], 0.0F, 0.0F, 0.0F, 0.0F);
               }
               argRGB[j] = (const GLfloat (*)[4]) ccolor[j];
            }
            break;
	 case GL_ONE:
            {
               GLfloat (*c)[4] = ccolor[j];
               for (i = 0; i < n; i++) {
                  ASSIGN_4V(c[i], 1.0F, 1.0F, 1.0F, 1.0F);
               }
               argRGB[j] = (const GLfloat (*)[4]) ccolor[j];
            }
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcRGB - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  return;
               argRGB[j] = (const GLfloat (*)[4])
                  (texelBuffer + srcUnit * (n * 4 * sizeof(GLfloat)));
            }
      }

      if (textureUnit->_CurrentCombine->OperandRGB[j] != GL_SRC_COLOR) {
         const GLfloat (*src)[4] = argRGB[j];
         GLfloat (*dst)[4] = ccolor[j];

         /* point to new arg[j] storage */
         argRGB[j] = (const GLfloat (*)[4]) ccolor[j];

         if (textureUnit->_CurrentCombine->OperandRGB[j] == GL_ONE_MINUS_SRC_COLOR) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = 1.0F - src[i][RCOMP];
               dst[i][GCOMP] = 1.0F - src[i][GCOMP];
               dst[i][BCOMP] = 1.0F - src[i][BCOMP];
            }
         }
         else if (textureUnit->_CurrentCombine->OperandRGB[j] == GL_SRC_ALPHA) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = src[i][ACOMP];
               dst[i][GCOMP] = src[i][ACOMP];
               dst[i][BCOMP] = src[i][ACOMP];
            }
         }
         else {
            ASSERT(textureUnit->_CurrentCombine->OperandRGB[j] ==GL_ONE_MINUS_SRC_ALPHA);
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = 1.0F - src[i][ACOMP];
               dst[i][GCOMP] = 1.0F - src[i][ACOMP];
               dst[i][BCOMP] = 1.0F - src[i][ACOMP];
            }
         }
      }
   }

   /*
    * Set up the argA[i] pointers
    */
   for (j = 0; j < numAlphaArgs; j++) {
      const GLenum srcA = textureUnit->_CurrentCombine->SourceA[j];

      switch (srcA) {
         case GL_TEXTURE:
            argA[j] = (const GLfloat (*)[4])
               (texelBuffer + unit * (n * 4 * sizeof(GLfloat)));
            break;
         case GL_PRIMARY_COLOR:
            argA[j] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argA[j] = (const GLfloat (*)[4]) rgba;
            break;
         case GL_CONSTANT:
            {
               GLfloat alpha, (*c)[4] = ccolor[j];
               alpha = textureUnit->EnvColor[3];
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = alpha;
               argA[j] = (const GLfloat (*)[4]) ccolor[j];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            argA[j] = & zero;
            break;
	 case GL_ONE:
            argA[j] = & one;
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcA - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  return;
               argA[j] = (const GLfloat (*)[4])
                  (texelBuffer + srcUnit * (n * 4 * sizeof(GLfloat)));
            }
      }

      if (textureUnit->_CurrentCombine->OperandA[j] == GL_ONE_MINUS_SRC_ALPHA) {
         const GLfloat (*src)[4] = argA[j];
         GLfloat (*dst)[4] = ccolor[j];
         argA[j] = (const GLfloat (*)[4]) ccolor[j];
         for (i = 0; i < n; i++) {
            dst[i][ACOMP] = 1.0F - src[i][ACOMP];
         }
      }
   }

   /*
    * Do the texture combine.
    */
   switch (textureUnit->_CurrentCombine->ModeRGB) {
      case GL_REPLACE:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            if (RGBshift) {
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = arg0[i][RCOMP] * RGBmult;
                  rgba[i][GCOMP] = arg0[i][GCOMP] * RGBmult;
                  rgba[i][BCOMP] = arg0[i][BCOMP] * RGBmult;
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = arg0[i][RCOMP];
                  rgba[i][GCOMP] = arg0[i][GCOMP];
                  rgba[i][BCOMP] = arg0[i][BCOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = arg0[i][RCOMP] * arg1[i][RCOMP] * RGBmult;
               rgba[i][GCOMP] = arg0[i][GCOMP] * arg1[i][GCOMP] * RGBmult;
               rgba[i][BCOMP] = arg0[i][BCOMP] * arg1[i][BCOMP] * RGBmult;
            }
         }
         break;
      case GL_ADD:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argRGB[2];
            const GLfloat (*arg3)[4] = (const GLfloat (*)[4]) argRGB[3];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] * arg1[i][RCOMP] +
                                 arg2[i][RCOMP] * arg3[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] * arg1[i][GCOMP] +
                                 arg2[i][GCOMP] * arg3[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] * arg1[i][BCOMP] +
                                 arg2[i][BCOMP] * arg3[i][BCOMP]) * RGBmult;
            }
         }
         else {
            /* 2-term addition */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP]) * RGBmult;
            }
         }
         break;
      case GL_ADD_SIGNED:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) - 0.5 */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argRGB[2];
            const GLfloat (*arg3)[4] = (const GLfloat (*)[4]) argRGB[3];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP] *
                                 arg2[i][RCOMP] + arg3[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP] *
                                 arg2[i][GCOMP] + arg3[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP] *
                                 arg2[i][BCOMP] + arg3[i][BCOMP] - 0.5) * RGBmult;
            }
         }
         else {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP] - 0.5) * RGBmult;
            }
         }
         break;
      case GL_INTERPOLATE:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argRGB[2];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] * arg2[i][RCOMP] +
                      arg1[i][RCOMP] * (1.0F - arg2[i][RCOMP])) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] * arg2[i][GCOMP] +
                      arg1[i][GCOMP] * (1.0F - arg2[i][GCOMP])) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] * arg2[i][BCOMP] +
                      arg1[i][BCOMP] * (1.0F - arg2[i][BCOMP])) * RGBmult;
            }
         }
         break;
      case GL_SUBTRACT:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] - arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] - arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] - arg1[i][BCOMP]) * RGBmult;
            }
         }
         break;
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
         {
            /* Do not scale the result by 1 2 or 4 */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               GLfloat dot = ((arg0[i][RCOMP]-0.5F) * (arg1[i][RCOMP]-0.5F) +
                             (arg0[i][GCOMP]-0.5F) * (arg1[i][GCOMP]-0.5F) +
                             (arg0[i][BCOMP]-0.5F) * (arg1[i][BCOMP]-0.5F))
                            * 4.0F;
               dot = CLAMP(dot, 0.0F, 1.0F);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLfloat) dot;
            }
         }
         break;
      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
         {
            /* DO scale the result by 1 2 or 4 */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
               GLfloat dot = ((arg0[i][RCOMP]-0.5F) * (arg1[i][RCOMP]-0.5F) +
                             (arg0[i][GCOMP]-0.5F) * (arg1[i][GCOMP]-0.5F) +
                             (arg0[i][BCOMP]-0.5F) * (arg1[i][BCOMP]-0.5F))
                            * 4.0F * RGBmult;
               dot = CLAMP(dot, 0.0, 1.0F);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLfloat) dot;
            }
         }
         break;
      case GL_MODULATE_ADD_ATI:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argRGB[2];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) +
                                 arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) +
                                 arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) +
                                 arg1[i][BCOMP]) * RGBmult;
            }
	 }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argRGB[2];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) +
                                 arg1[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) +
                                 arg1[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) +
                                 arg1[i][BCOMP] - 0.5) * RGBmult;
            }
	 }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argRGB[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argRGB[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argRGB[2];
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) -
                                 arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) -
                                 arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) -
                                 arg1[i][BCOMP]) * RGBmult;
            }
	 }
         break;
      case GL_BUMP_ENVMAP_ATI:
         {
            /* this produces a fixed rgba color, and the coord calc is done elsewhere */
            for (i = 0; i < n; i++) {
            /* rgba result is 0,0,0,1 */
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = 0.0;
               rgba[i][GCOMP] = 0.0;
               rgba[i][BCOMP] = 0.0;
               rgba[i][ACOMP] = 1.0;
#else
               rgba[i][RCOMP] = 0;
               rgba[i][GCOMP] = 0;
               rgba[i][BCOMP] = 0;
               rgba[i][ACOMP] = CHAN_MAX;
#endif
            }
	 }
         return; /* no alpha processing */
      default:
         _mesa_problem(ctx, "invalid combine mode");
   }

   switch (textureUnit->_CurrentCombine->ModeA) {
      case GL_REPLACE:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            if (Ashift) {
               for (i = 0; i < n; i++) {
                  GLfloat a = arg0[i][ACOMP] * Amult;
                  rgba[i][ACOMP] = (GLfloat) MIN2(a, 1.0F);
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][ACOMP] = arg0[i][ACOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = arg0[i][ACOMP] * arg1[i][ACOMP] * Amult;
            }
         }
         break;
      case GL_ADD:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argA[2];
            const GLfloat (*arg3)[4] = (const GLfloat (*)[4]) argA[3];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg1[i][ACOMP] +
                                 arg2[i][ACOMP] * arg3[i][ACOMP]) * Amult;
            }
         }
         else {
            /* two-term add */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP]) * Amult;
            }
         }
         break;
      case GL_ADD_SIGNED:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) - 0.5 */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argA[2];
            const GLfloat (*arg3)[4] = (const GLfloat (*)[4]) argA[3];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg1[i][ACOMP] +
                                 arg2[i][ACOMP] * arg3[i][ACOMP] -
                                 0.5) * Amult;
            }
         }
         else {
            /* a + b - 0.5 */
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP] - 0.5F) * Amult;
            }
         }
         break;
      case GL_INTERPOLATE:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argA[2];
            for (i=0; i<n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg2[i][ACOMP] +
                                 arg1[i][ACOMP] * (1.0F - arg2[i][ACOMP]))
                                * Amult;
            }
         }
         break;
      case GL_SUBTRACT:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] - arg1[i][ACOMP]) * Amult;
            }
         }
         break;
      case GL_MODULATE_ADD_ATI:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argA[2];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP])
                                 + arg1[i][ACOMP]) * Amult;
            }
         }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argA[2];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) +
                                 arg1[i][ACOMP] - 0.5F) * Amult;
            }
         }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         {
            const GLfloat (*arg0)[4] = (const GLfloat (*)[4]) argA[0];
            const GLfloat (*arg1)[4] = (const GLfloat (*)[4]) argA[1];
            const GLfloat (*arg2)[4] = (const GLfloat (*)[4]) argA[2];
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP])
                                 - arg1[i][ACOMP]) * Amult;
            }
         }
         break;
      default:
         _mesa_problem(ctx, "invalid combine mode");
   }

   /* Fix the alpha component for GL_DOT3_RGBA_EXT/ARB combining.
    * This is kind of a kludge.  It would have been better if the spec
    * were written such that the GL_COMBINE_ALPHA value could be set to
    * GL_DOT3.
    */
   if (textureUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT ||
       textureUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) {
      for (i = 0; i < n; i++) {
	 rgba[i][ACOMP] = rgba[i][RCOMP];
      }
   }

   for (i = 0; i < n; i++) {
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][RCOMP], rgba[i][RCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][GCOMP], rgba[i][GCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][BCOMP], rgba[i][BCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][ACOMP], rgba[i][ACOMP]);
   }
}



/**
 * Apply X/Y/Z/W/0/1 swizzle to an array of colors/texels.
 * See GL_EXT_texture_swizzle.
 */
static void
swizzle_texels(GLuint swizzle, GLuint count, GLfloat (*texels)[4])
{
   const GLuint swzR = GET_SWZ(swizzle, 0);
   const GLuint swzG = GET_SWZ(swizzle, 1);
   const GLuint swzB = GET_SWZ(swizzle, 2);
   const GLuint swzA = GET_SWZ(swizzle, 3);
   GLfloat vector[6];
   GLuint i;

   vector[SWIZZLE_ZERO] = 0;
   vector[SWIZZLE_ONE] = 1.0F;

   for (i = 0; i < count; i++) {
      vector[SWIZZLE_X] = texels[i][0];
      vector[SWIZZLE_Y] = texels[i][1];
      vector[SWIZZLE_Z] = texels[i][2];
      vector[SWIZZLE_W] = texels[i][3];
      texels[i][RCOMP] = vector[swzR];
      texels[i][GCOMP] = vector[swzG];
      texels[i][BCOMP] = vector[swzB];
      texels[i][ACOMP] = vector[swzA];
   }
}


/**
 * Apply a conventional OpenGL texture env mode (REPLACE, ADD, BLEND,
 * MODULATE, or DECAL) to an array of fragments.
 * Input:  textureUnit - pointer to texture unit to apply
 *         format - base internal texture format
 *         n - number of fragments
 *         primary_rgba - primary colors (may alias rgba for single texture)
 *         texels - array of texel colors
 * InOut:  rgba - incoming fragment colors modified by texel colors
 *                according to the texture environment mode.
 */
static void
texture_apply( const GLcontext *ctx,
               const struct gl_texture_unit *texUnit,
               GLuint n,
               CONST GLfloat primary_rgba[][4], CONST GLfloat texel[][4],
               GLchan rgbaChan[][4] )
{
   GLint baseLevel;
   GLuint i;
   GLfloat Rc, Gc, Bc, Ac;
   GLenum format;
   GLfloat rgba[MAX_WIDTH][4];

   (void) primary_rgba;

   ASSERT(texUnit);
   ASSERT(texUnit->_Current);

   baseLevel = texUnit->_Current->BaseLevel;
   ASSERT(texUnit->_Current->Image[0][baseLevel]);

   format = texUnit->_Current->Image[0][baseLevel]->_BaseFormat;

   if (format == GL_COLOR_INDEX || format == GL_YCBCR_MESA) {
      format = GL_RGBA;  /* a bit of a hack */
   }
   else if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL_EXT) {
      format = texUnit->_Current->DepthMode;
   }

   if (texUnit->EnvMode != GL_REPLACE) {
      /* convert GLchan colors to GLfloat */
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = CHAN_TO_FLOAT(rgbaChan[i][RCOMP]);
         rgba[i][GCOMP] = CHAN_TO_FLOAT(rgbaChan[i][GCOMP]);
         rgba[i][BCOMP] = CHAN_TO_FLOAT(rgbaChan[i][BCOMP]);
         rgba[i][ACOMP] = CHAN_TO_FLOAT(rgbaChan[i][ACOMP]);
      }
   }

   switch (texUnit->EnvMode) {
      case GL_REPLACE:
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
                  /* Av = At */
                  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Lt */
                  GLfloat Lt = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
                  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
                  GLfloat Lt = texel[i][RCOMP];
		  /* Cv = Lt */
		  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = It */
                  GLfloat It = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = It;
                  /* Av = It */
                  rgba[i][ACOMP] = It;
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_REPLACE) in texture_apply");
               return;
	 }
	 break;

      case GL_MODULATE:
         switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
		  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = LtCf */
                  GLfloat Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = rgba[i][RCOMP] * Lt;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * Lt;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * Lt;
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfLt */
                  GLfloat Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = rgba[i][RCOMP] * Lt;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * Lt;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * Lt;
		  /* Av = AfAt */
		  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = CfIt */
                  GLfloat It = texel[i][RCOMP];
		  rgba[i][RCOMP] = rgba[i][RCOMP] * It;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * It;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * It;
		  /* Av = AfIt */
		  rgba[i][ACOMP] = rgba[i][ACOMP] * It;
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = rgba[i][RCOMP] * texel[i][RCOMP];
		  rgba[i][GCOMP] = rgba[i][GCOMP] * texel[i][GCOMP];
		  rgba[i][BCOMP] = rgba[i][BCOMP] * texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = rgba[i][RCOMP] * texel[i][RCOMP];
		  rgba[i][GCOMP] = rgba[i][GCOMP] * texel[i][GCOMP];
		  rgba[i][BCOMP] = rgba[i][BCOMP] * texel[i][BCOMP];
		  /* Av = AfAt */
		  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_MODULATE) in texture_apply");
               return;
	 }
	 break;

      case GL_DECAL:
         switch (format) {
            case GL_ALPHA:
            case GL_LUMINANCE:
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
               /* undefined */
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-At) + CtAt */
		  GLfloat t = texel[i][ACOMP], s = 1.0F - t;
		  rgba[i][RCOMP] = rgba[i][RCOMP] * s + texel[i][RCOMP] * t;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * s + texel[i][GCOMP] * t;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * s + texel[i][BCOMP] * t;
		  /* Av = Af */
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_DECAL) in texture_apply");
               return;
	 }
	 break;

      case GL_BLEND:
         Rc = texUnit->EnvColor[0];
         Gc = texUnit->EnvColor[1];
         Bc = texUnit->EnvColor[2];
         Ac = texUnit->EnvColor[3];
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
                  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
	       }
	       break;
            case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLfloat Lt = texel[i][RCOMP], s = 1.0F - Lt;
		  rgba[i][RCOMP] = rgba[i][RCOMP] * s + Rc * Lt;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * s + Gc * Lt;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * s + Bc * Lt;
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLfloat Lt = texel[i][RCOMP], s = 1.0F - Lt;
		  rgba[i][RCOMP] = rgba[i][RCOMP] * s + Rc * Lt;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * s + Gc * Lt;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * s + Bc * Lt;
		  /* Av = AfAt */
		  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
	       }
	       break;
            case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-It) + CcIt */
		  GLfloat It = texel[i][RCOMP], s = 1.0F - It;
		  rgba[i][RCOMP] = rgba[i][RCOMP] * s + Rc * It;
		  rgba[i][GCOMP] = rgba[i][GCOMP] * s + Gc * It;
		  rgba[i][BCOMP] = rgba[i][BCOMP] * s + Bc * It;
                  /* Av = Af(1-It) + Ac*It */
                  rgba[i][ACOMP] = rgba[i][ACOMP] * s + Ac * It;
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = rgba[i][RCOMP] * (1.0F - texel[i][RCOMP])
                     + Rc * texel[i][RCOMP];
		  rgba[i][GCOMP] = rgba[i][GCOMP] * (1.0F - texel[i][GCOMP])
                     + Gc * texel[i][GCOMP];
		  rgba[i][BCOMP] = rgba[i][BCOMP] * (1.0F - texel[i][BCOMP])
                     + Bc * texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = rgba[i][RCOMP] * (1.0F - texel[i][RCOMP])
                     + Rc * texel[i][RCOMP];
		  rgba[i][GCOMP] = rgba[i][GCOMP] * (1.0F - texel[i][GCOMP])
                     + Gc * texel[i][GCOMP];
		  rgba[i][BCOMP] = rgba[i][BCOMP] * (1.0F - texel[i][BCOMP])
                     + Bc * texel[i][BCOMP];
		  /* Av = AfAt */
		  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_BLEND) in texture_apply");
               return;
	 }
	 break;

     /* XXX don't clamp results if GLfloat is float??? */

      case GL_ADD:  /* GL_EXT_texture_add_env */
         switch (format) {
            case GL_ALPHA:
               for (i=0;i<n;i++) {
                  /* Rv = Rf */
                  /* Gv = Gf */
                  /* Bv = Bf */
                  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
               }
               break;
            case GL_LUMINANCE:
               for (i=0;i<n;i++) {
                  GLfloat Lt = texel[i][RCOMP];
                  GLfloat r = rgba[i][RCOMP] + Lt;
                  GLfloat g = rgba[i][GCOMP] + Lt;
                  GLfloat b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, 1.0F);
                  rgba[i][GCOMP] = MIN2(g, 1.0F);
                  rgba[i][BCOMP] = MIN2(b, 1.0F);
                  /* Av = Af */
               }
               break;
            case GL_LUMINANCE_ALPHA:
               for (i=0;i<n;i++) {
                  GLfloat Lt = texel[i][RCOMP];
                  GLfloat r = rgba[i][RCOMP] + Lt;
                  GLfloat g = rgba[i][GCOMP] + Lt;
                  GLfloat b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, 1.0F);
                  rgba[i][GCOMP] = MIN2(g, 1.0F);
                  rgba[i][BCOMP] = MIN2(b, 1.0F);
                  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
               }
               break;
            case GL_INTENSITY:
               for (i=0;i<n;i++) {
                  GLfloat It = texel[i][RCOMP];
                  GLfloat r = rgba[i][RCOMP] + It;
                  GLfloat g = rgba[i][GCOMP] + It;
                  GLfloat b = rgba[i][BCOMP] + It;
                  GLfloat a = rgba[i][ACOMP] + It;
                  rgba[i][RCOMP] = MIN2(r, 1.0F);
                  rgba[i][GCOMP] = MIN2(g, 1.0F);
                  rgba[i][BCOMP] = MIN2(b, 1.0F);
                  rgba[i][ACOMP] = MIN2(a, 1.0F);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
                  GLfloat r = rgba[i][RCOMP] + texel[i][RCOMP];
                  GLfloat g = rgba[i][GCOMP] + texel[i][GCOMP];
                  GLfloat b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, 1.0F);
		  rgba[i][GCOMP] = MIN2(g, 1.0F);
		  rgba[i][BCOMP] = MIN2(b, 1.0F);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
                  GLfloat r = rgba[i][RCOMP] + texel[i][RCOMP];
                  GLfloat g = rgba[i][GCOMP] + texel[i][GCOMP];
                  GLfloat b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, 1.0F);
		  rgba[i][GCOMP] = MIN2(g, 1.0F);
		  rgba[i][BCOMP] = MIN2(b, 1.0F);
                  rgba[i][ACOMP] = rgba[i][ACOMP] * texel[i][ACOMP];
               }
               break;
            default:
               _mesa_problem(ctx, "Bad format (GL_ADD) in texture_apply");
               return;
	 }
	 break;

      default:
         _mesa_problem(ctx, "Bad env mode in texture_apply");
         return;
   }

   /* convert GLfloat colors to GLchan */
   for (i = 0; i < n; i++) {
      CLAMPED_FLOAT_TO_CHAN(rgbaChan[i][RCOMP], rgba[i][RCOMP]);
      CLAMPED_FLOAT_TO_CHAN(rgbaChan[i][GCOMP], rgba[i][GCOMP]);
      CLAMPED_FLOAT_TO_CHAN(rgbaChan[i][BCOMP], rgba[i][BCOMP]);
      CLAMPED_FLOAT_TO_CHAN(rgbaChan[i][ACOMP], rgba[i][ACOMP]);
   }
}



/**
 * Apply texture mapping to a span of fragments.
 */
void
_swrast_texture_span( GLcontext *ctx, SWspan *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat primary_rgba[MAX_WIDTH][4];
   GLuint unit;

   ASSERT(span->end < MAX_WIDTH);

   /*
    * Save copy of the incoming fragment colors (the GL_PRIMARY_COLOR)
    */
   if (swrast->_AnyTextureCombine) {
      GLuint i;
      for (i = 0; i < span->end; i++) {
         primary_rgba[i][RCOMP] = CHAN_TO_FLOAT(span->array->rgba[i][RCOMP]);
         primary_rgba[i][GCOMP] = CHAN_TO_FLOAT(span->array->rgba[i][GCOMP]);
         primary_rgba[i][BCOMP] = CHAN_TO_FLOAT(span->array->rgba[i][BCOMP]);
         primary_rgba[i][ACOMP] = CHAN_TO_FLOAT(span->array->rgba[i][ACOMP]);
      }
   }

   /* First must sample all bump maps */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled &&
         ctx->Texture.Unit[unit]._CurrentCombine->ModeRGB == GL_BUMP_ENVMAP_ATI) {
         const GLfloat (*texcoords)[4]
            = (const GLfloat (*)[4])
            span->array->attribs[FRAG_ATTRIB_TEX0 + unit];
         GLfloat (*targetcoords)[4]
            = (GLfloat (*)[4])
            span->array->attribs[FRAG_ATTRIB_TEX0 +
               ctx->Texture.Unit[unit].BumpTarget - GL_TEXTURE0];

         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         const struct gl_texture_object *curObj = texUnit->_Current;
         GLfloat *lambda = span->array->lambda[unit];
         GLchan (*texels)[4] = (GLchan (*)[4])
            (swrast->TexelBuffer + unit * (span->end * 4 * sizeof(GLchan)));
         GLuint i;
         GLfloat rotMatrix00 = ctx->Texture.Unit[unit].RotMatrix[0];
         GLfloat rotMatrix01 = ctx->Texture.Unit[unit].RotMatrix[1];
         GLfloat rotMatrix10 = ctx->Texture.Unit[unit].RotMatrix[2];
         GLfloat rotMatrix11 = ctx->Texture.Unit[unit].RotMatrix[3];

         /* adjust texture lod (lambda) */
         if (span->arrayMask & SPAN_LAMBDA) {
            if (texUnit->LodBias + curObj->LodBias != 0.0F) {
               /* apply LOD bias, but don't clamp yet */
               const GLfloat bias = CLAMP(texUnit->LodBias + curObj->LodBias,
                                          -ctx->Const.MaxTextureLodBias,
                                          ctx->Const.MaxTextureLodBias);
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  lambda[i] += bias;
               }
            }

            if (curObj->MinLod != -1000.0 || curObj->MaxLod != 1000.0) {
               /* apply LOD clamping to lambda */
               const GLfloat min = curObj->MinLod;
               const GLfloat max = curObj->MaxLod;
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  GLfloat l = lambda[i];
                  lambda[i] = CLAMP(l, min, max);
               }
            }
         }

         /* Sample the texture (span->end = number of fragments) */
         swrast->TextureSample[unit]( ctx, texUnit->_Current, span->end,
                                      texcoords, lambda, texels );

         /* manipulate the span values of the bump target
            not sure this can work correctly even ignoring
            the problem that channel is unsigned */
         for (i = 0; i < span->end; i++) {
#if CHAN_TYPE == GL_FLOAT
            targetcoords[i][0] += (texels[i][0] * rotMatrix00 + texels[i][1] *
                                  rotMatrix01) / targetcoords[i][3];
            targetcoords[i][1] += (texels[i][0] * rotMatrix10 + texels[i][1] *
                                  rotMatrix11) / targetcoords[i][3];
#else
            targetcoords[i][0] += (CHAN_TO_FLOAT(texels[i][1]) * rotMatrix00 +
                                  CHAN_TO_FLOAT(texels[i][1]) * rotMatrix01) /
                                  targetcoords[i][3];
            targetcoords[i][1] += (CHAN_TO_FLOAT(texels[i][0]) * rotMatrix10 + 
                                  CHAN_TO_FLOAT(texels[i][1]) * rotMatrix11) /
                                  targetcoords[i][3];
#endif
         }
      }
   }

   /*
    * Must do all texture sampling before combining in order to
    * accomodate GL_ARB_texture_env_crossbar.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled &&
         ctx->Texture.Unit[unit]._CurrentCombine->ModeRGB != GL_BUMP_ENVMAP_ATI) {
         const GLfloat (*texcoords)[4]
            = (const GLfloat (*)[4])
            span->array->attribs[FRAG_ATTRIB_TEX0 + unit];
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         const struct gl_texture_object *curObj = texUnit->_Current;
         GLfloat *lambda = span->array->lambda[unit];
         GLfloat (*texels)[4] = (GLfloat (*)[4])
            (swrast->TexelBuffer + unit * (span->end * 4 * sizeof(GLfloat)));

         /* adjust texture lod (lambda) */
         if (span->arrayMask & SPAN_LAMBDA) {
            if (texUnit->LodBias + curObj->LodBias != 0.0F) {
               /* apply LOD bias, but don't clamp yet */
               const GLfloat bias = CLAMP(texUnit->LodBias + curObj->LodBias,
                                          -ctx->Const.MaxTextureLodBias,
                                          ctx->Const.MaxTextureLodBias);
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  lambda[i] += bias;
               }
            }

            if (curObj->MinLod != -1000.0 || curObj->MaxLod != 1000.0) {
               /* apply LOD clamping to lambda */
               const GLfloat min = curObj->MinLod;
               const GLfloat max = curObj->MaxLod;
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  GLfloat l = lambda[i];
                  lambda[i] = CLAMP(l, min, max);
               }
            }
         }

         /* Sample the texture (span->end = number of fragments) */
         swrast->TextureSample[unit]( ctx, texUnit->_Current, span->end,
                                      texcoords, lambda, texels );

         /* GL_SGI_texture_color_table */
         if (texUnit->ColorTableEnabled) {
            _mesa_lookup_rgba_float(&texUnit->ColorTable, span->end, texels);
         }

         /* GL_EXT_texture_swizzle */
         if (curObj->_Swizzle != SWIZZLE_NOOP) {
            swizzle_texels(curObj->_Swizzle, span->end, texels);
         }
      }
   }


   /*
    * OK, now apply the texture (aka texture combine/blend).
    * We modify the span->color.rgba values.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         if (texUnit->_CurrentCombine != &texUnit->_EnvMode ) {
            texture_combine( ctx, unit, span->end,
                             (CONST GLfloat (*)[4]) primary_rgba,
                             swrast->TexelBuffer,
                             span->array->rgba );
         }
         else {
            /* conventional texture blend */
            const GLfloat (*texels)[4] = (const GLfloat (*)[4])
               (swrast->TexelBuffer + unit *
                (span->end * 4 * sizeof(GLfloat)));


            texture_apply( ctx, texUnit, span->end,
                           (CONST GLfloat (*)[4]) primary_rgba, texels,
                           span->array->rgba );
         }
      }
   }
}
