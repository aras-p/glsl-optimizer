/* $Id: texstate.c,v 1.69 2002/04/25 22:11:33 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "extensions.h"
#include "macros.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "mtypes.h"
#include "math/m_xform.h"
#include "math/m_matrix.h"
#endif



#ifdef SPECIALCAST
/* Needed for an Amiga compiler */
#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))
#else
/* all other compilers */
#define ENUM_TO_FLOAT(X) ((GLfloat)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(X))
#endif



/**********************************************************************/
/*                       Texture Environment                          */
/**********************************************************************/


void
_mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

#define TE_ERROR(errCode, msg, value)				\
   {								\
      char s[100];						\
      sprintf(s, msg, _mesa_lookup_enum_by_nr(value));		\
      _mesa_error(ctx, errCode, s);				\
   }

   if (target==GL_TEXTURE_ENV) {
      switch (pname) {
      case GL_TEXTURE_ENV_MODE: {
	 GLenum mode = (GLenum) (GLint) *param;

	 switch (mode) {
	 case GL_ADD:
	    if (!ctx->Extensions.EXT_texture_env_add) {
	       TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
	    break;
	 case GL_COMBINE_EXT:
	    if (!ctx->Extensions.EXT_texture_env_combine &&
                !ctx->Extensions.ARB_texture_env_combine) {
	       TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
	    break;
	 case GL_MODULATE:
	 case GL_BLEND:
	 case GL_DECAL:
	 case GL_REPLACE:
	    break;
	 default:
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	    return;
	 }

	 if (texUnit->EnvMode == mode)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	 texUnit->EnvMode = mode;
	 break;
      }
      case GL_TEXTURE_ENV_COLOR: {
	 GLfloat tmp[4];
	 tmp[0] = CLAMP( param[0], 0.0F, 1.0F );
	 tmp[1] = CLAMP( param[1], 0.0F, 1.0F );
	 tmp[2] = CLAMP( param[2], 0.0F, 1.0F );
	 tmp[3] = CLAMP( param[3], 0.0F, 1.0F );
	 if (TEST_EQ_4V(tmp, texUnit->EnvColor))
	    return;
	 FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	 COPY_4FV(texUnit->EnvColor, tmp);
	 break;
      }
      case GL_COMBINE_RGB_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum mode = (GLenum) (GLint) *param;
	    switch (mode) {
	    case GL_REPLACE:
	    case GL_MODULATE:
	    case GL_ADD:
	    case GL_ADD_SIGNED_EXT:
	    case GL_INTERPOLATE_EXT:
               /* OK */
	       break;
            case GL_SUBTRACT_ARB:
               if (!ctx->Extensions.ARB_texture_env_combine) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
                  return;
               }
               break;
	    case GL_DOT3_RGB_EXT:
	    case GL_DOT3_RGBA_EXT:
	       if (!ctx->Extensions.EXT_texture_env_dot3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_DOT3_RGB_ARB:
	    case GL_DOT3_RGBA_ARB:
	       if (!ctx->Extensions.ARB_texture_env_dot3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
	    if (texUnit->CombineModeRGB == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineModeRGB = mode;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
         break;
      case GL_COMBINE_ALPHA_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum mode = (GLenum) (GLint) *param;
	    switch (mode) {
	    case GL_REPLACE:
	    case GL_MODULATE:
	    case GL_ADD:
	    case GL_ADD_SIGNED_EXT:
	    case GL_INTERPOLATE_EXT:
               /* OK */
	       break;
            case GL_SUBTRACT_ARB:
               if (!ctx->Extensions.ARB_texture_env_combine) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
                  return;
               }
               break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
            if (texUnit->CombineModeA == mode)
               return;
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texUnit->CombineModeA = mode;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_SOURCE0_RGB_EXT:
      case GL_SOURCE1_RGB_EXT:
      case GL_SOURCE2_RGB_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
	     ctx->Extensions.ARB_texture_env_combine) {
	    GLenum source = (GLenum) (GLint) *param;
	    GLuint s = pname - GL_SOURCE0_RGB_EXT;
	    switch (source) {
	    case GL_TEXTURE:
	    case GL_CONSTANT_EXT:
	    case GL_PRIMARY_COLOR_EXT:
	    case GL_PREVIOUS_EXT:
	       if (texUnit->CombineSourceRGB[s] == source)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineSourceRGB[s] = source;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", source);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_SOURCE0_ALPHA_EXT:
      case GL_SOURCE1_ALPHA_EXT:
      case GL_SOURCE2_ALPHA_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLenum source = (GLenum) (GLint) *param;
	    GLuint s = pname - GL_SOURCE0_ALPHA_EXT;
	    switch (source) {
	    case GL_TEXTURE:
	    case GL_CONSTANT_EXT:
	    case GL_PRIMARY_COLOR_EXT:
	    case GL_PREVIOUS_EXT:
	       if (texUnit->CombineSourceA[s] == source) return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineSourceA[s] = source;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", source);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND0_RGB_EXT:
      case GL_OPERAND1_RGB_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
	     ctx->Extensions.ARB_texture_env_combine) {
	    GLenum operand = (GLenum) (GLint) *param;
	    GLuint s = pname - GL_OPERAND0_RGB_EXT;
	    switch (operand) {
	    case GL_SRC_COLOR:
	    case GL_ONE_MINUS_SRC_COLOR:
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       if (texUnit->CombineOperandRGB[s] == operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandRGB[s] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND0_ALPHA_EXT:
      case GL_OPERAND1_ALPHA_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLenum operand = (GLenum) (GLint) *param;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       if (texUnit->CombineOperandA[pname-GL_OPERAND0_ALPHA_EXT] ==
		   operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandA[pname-GL_OPERAND0_ALPHA_EXT] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND2_RGB_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLenum operand = (GLenum) (GLint) *param;
	    switch (operand) {
	    case GL_SRC_COLOR:           /* ARB combine only */
	    case GL_ONE_MINUS_SRC_COLOR: /* ARB combine only */
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       if (texUnit->CombineOperandRGB[2] == operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandRGB[2] = operand;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND2_ALPHA_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLenum operand = (GLenum) (GLint) *param;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       if (texUnit->CombineOperandA[2] == operand)
		  return;
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->CombineOperandA[2] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_RGB_SCALE_EXT:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLuint newshift;
	    if (*param == 1.0) {
	       newshift = 0;
	    }
	    else if (*param == 2.0) {
	       newshift = 1;
	    }
	    else if (*param == 4.0) {
	       newshift = 2;
	    }
	    else {
	       _mesa_error( ctx, GL_INVALID_VALUE,
                            "glTexEnv(GL_RGB_SCALE not 1, 2 or 4)" );
	       return;
	    }
	    if (texUnit->CombineScaleShiftRGB == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineScaleShiftRGB = newshift;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_ALPHA_SCALE:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLuint newshift;
	    if (*param == 1.0) {
	       newshift = 0;
	    }
	    else if (*param == 2.0) {
	       newshift = 1;
	    }
	    else if (*param == 4.0) {
	       newshift = 2;
	    }
	    else {
	       _mesa_error( ctx, GL_INVALID_VALUE,
                            "glTexEnv(GL_ALPHA_SCALE not 1, 2 or 4)" );
	       return;
	    }
	    if (texUnit->CombineScaleShiftA == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->CombineScaleShiftA = newshift;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
	 return;
      }
   }
   else if (target==GL_TEXTURE_FILTER_CONTROL_EXT) {
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(param)" );
	 return;
      }
      switch (pname) {
      case GL_TEXTURE_LOD_BIAS_EXT:
	 if (texUnit->LodBias == param[0])
	    return;
	 FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->LodBias = CLAMP(param[0], -ctx->Const.MaxTextureLodBias,
                                  ctx->Const.MaxTextureLodBias);
	 break;
      default:
         TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	 return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glTexEnv %s %s %.1f(%s) ...\n",
	      _mesa_lookup_enum_by_nr(target),
	      _mesa_lookup_enum_by_nr(pname),
	      *param,
	      _mesa_lookup_enum_by_nr((GLenum) (GLint) *param));

   /* Tell device driver about the new texture environment */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, target, pname, param );
   }
}


void
_mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexEnvfv( target, pname, &param );
}



void
_mesa_TexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
   _mesa_TexEnvfv( target, pname, p );
}


void
_mesa_TexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT( param[0] );
      p[1] = INT_TO_FLOAT( param[1] );
      p[2] = INT_TO_FLOAT( param[2] );
      p[3] = INT_TO_FLOAT( param[3] );
   }
   else {
      p[0] = (GLint) param[0];
      p[1] = p[2] = p[3] = 0;  /* init to zero, just to be safe */
   }
   _mesa_TexEnvfv( target, pname, p );
}


void
_mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target!=GL_TEXTURE_ENV) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }

   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = ENUM_TO_FLOAT(texUnit->EnvMode);
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 COPY_4FV( params, texUnit->EnvColor );
	 break;
      case GL_COMBINE_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineModeRGB;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_COMBINE_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineModeA;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_SOURCE0_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineSourceRGB[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_SOURCE1_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineSourceRGB[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_SOURCE2_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineSourceRGB[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_SOURCE0_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineSourceA[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_SOURCE1_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineSourceA[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_SOURCE2_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineSourceA[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_OPERAND0_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineOperandRGB[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_OPERAND1_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineOperandRGB[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_OPERAND2_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineOperandRGB[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_OPERAND0_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineOperandA[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_OPERAND1_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineOperandA[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
         break;
      case GL_OPERAND2_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLfloat) texUnit->CombineOperandA[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
         }
	 break;
      case GL_RGB_SCALE_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            if (texUnit->CombineScaleShiftRGB == 0)
               *params = 1.0;
            else if (texUnit->CombineScaleShiftRGB == 1)
               *params = 2.0;
            else
               *params = 4.0;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            return;
         }
         break;
      case GL_ALPHA_SCALE:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            if (texUnit->CombineScaleShiftA == 0)
               *params = 1.0;
            else if (texUnit->CombineScaleShiftA == 1)
               *params = 2.0;
            else
               *params = 4.0;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            return;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
   }
}


void
_mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_TEXTURE_ENV) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
      return;
   }

   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = (GLint) texUnit->EnvMode;
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = FLOAT_TO_INT( texUnit->EnvColor[0] );
	 params[1] = FLOAT_TO_INT( texUnit->EnvColor[1] );
	 params[2] = FLOAT_TO_INT( texUnit->EnvColor[2] );
	 params[3] = FLOAT_TO_INT( texUnit->EnvColor[3] );
         break;
      case GL_COMBINE_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineModeRGB;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_COMBINE_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineModeA;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_SOURCE0_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineSourceRGB[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_SOURCE1_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineSourceRGB[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_SOURCE2_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineSourceRGB[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_SOURCE0_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineSourceA[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_SOURCE1_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineSourceA[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_SOURCE2_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineSourceA[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_OPERAND0_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineOperandRGB[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_OPERAND1_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineOperandRGB[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_OPERAND2_RGB_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineOperandRGB[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_OPERAND0_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineOperandA[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_OPERAND1_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineOperandA[1];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
         break;
      case GL_OPERAND2_ALPHA_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            *params = (GLint) texUnit->CombineOperandA[2];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
         }
	 break;
      case GL_RGB_SCALE_EXT:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            if (texUnit->CombineScaleShiftRGB == 0)
               *params = 1;
            else if (texUnit->CombineScaleShiftRGB == 1)
               *params = 2;
            else
               *params = 4;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            return;
         }
         break;
      case GL_ALPHA_SCALE:
         if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
            if (texUnit->CombineScaleShiftA == 0)
               *params = 1;
            else if (texUnit->CombineScaleShiftA == 1)
               *params = 2;
            else
               *params = 4;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            return;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
   }
}




/**********************************************************************/
/*                       Texture Parameters                           */
/**********************************************************************/


void
_mesa_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexParameterfv(target, pname, &param);
}


void
_mesa_TexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   GLenum eparam = (GLenum) (GLint) params[0];
   struct gl_texture_object *texObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "texPARAM %s %s %d...\n",
	      _mesa_lookup_enum_by_nr(target),
	      _mesa_lookup_enum_by_nr(pname),
	      eparam);


   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D_EXT:
         texObj = texUnit->Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (ctx->Extensions.ARB_texture_cube_map) {
            texObj = texUnit->CurrentCubeMap;
            break;
         }
         /* fallthrough */
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         /* A small optimization */
         if (texObj->MinFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR
             || eparam==GL_NEAREST_MIPMAP_NEAREST
             || eparam==GL_LINEAR_MIPMAP_NEAREST
             || eparam==GL_NEAREST_MIPMAP_LINEAR
             || eparam==GL_LINEAR_MIPMAP_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         /* A small optimization */
         if (texObj->MagFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MagFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (texObj->WrapS == eparam)
            return;
         if (eparam==GL_CLAMP ||
             eparam==GL_REPEAT ||
             eparam==GL_CLAMP_TO_EDGE ||
             (eparam == GL_CLAMP_TO_BORDER_ARB &&
              ctx->Extensions.ARB_texture_border_clamp) ||
             (eparam == GL_MIRRORED_REPEAT_ARB &&
              ctx->Extensions.ARB_texture_mirrored_repeat)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapS = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (texObj->WrapT == eparam)
            return;
         if (eparam==GL_CLAMP ||
             eparam==GL_REPEAT ||
             eparam==GL_CLAMP_TO_EDGE ||
             (eparam == GL_CLAMP_TO_BORDER_ARB &&
              ctx->Extensions.ARB_texture_border_clamp) ||
             (eparam == GL_MIRRORED_REPEAT_ARB &&
              ctx->Extensions.ARB_texture_mirrored_repeat)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapT = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         if (texObj->WrapR == eparam)
            return;
         if (eparam==GL_CLAMP ||
             eparam==GL_REPEAT ||
             eparam==GL_CLAMP_TO_EDGE ||
             (eparam == GL_CLAMP_TO_BORDER_ARB &&
              ctx->Extensions.ARB_texture_border_clamp) ||
             (eparam == GL_MIRRORED_REPEAT_ARB &&
              ctx->Extensions.ARB_texture_mirrored_repeat)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapR = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
         }
         break;
#if 0 /* someday */
      case GL_TEXTUER_BORDER_VALUES_NV:
         /* don't clamp */
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         COPY_4V(texObj->BorderValues, params);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[0], params[0]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[1], params[1]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[2], params[2]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[3], params[3]);
         break;
#endif
      case GL_TEXTURE_BORDER_COLOR:
         /* clamp */
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BorderValues[0] = CLAMP(params[0], 0.0F, 1.0F);
         texObj->BorderValues[1] = CLAMP(params[1], 0.0F, 1.0F);
         texObj->BorderValues[2] = CLAMP(params[2], 0.0F, 1.0F);
         texObj->BorderValues[3] = CLAMP(params[3], 0.0F, 1.0F);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[0], texObj->BorderValues[0]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[1], texObj->BorderValues[1]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[2], texObj->BorderValues[2]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->BorderColor[3], texObj->BorderValues[3]);
         break;
      case GL_TEXTURE_MIN_LOD:
         if (texObj->MinLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MinLod = params[0];
         break;
      case GL_TEXTURE_MAX_LOD:
         if (texObj->MaxLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLod = params[0];
         break;
      case GL_TEXTURE_BASE_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BaseLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_MAX_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_PRIORITY:
         /* (keithh@netcomuk.co.uk) */
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->Priority = CLAMP( params[0], 0.0F, 1.0F );
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
	    if (params[0] < 1.0) {
	       _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
	       return;
	    }
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MaxAnisotropy = params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_MAX_TEXTURE_ANISOTROPY_EXT)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->CompareFlag = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_SGIX)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            GLenum op = (GLenum) params[0];
            if (op == GL_TEXTURE_LEQUAL_R_SGIX ||
                op == GL_TEXTURE_GEQUAL_R_SGIX) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareOperator = op;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(param)");
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                    "glTexParameter(pname=GL_TEXTURE_COMPARE_OPERATOR_SGIX)");
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            UNCLAMPED_FLOAT_TO_CHAN(texObj->ShadowAmbient, params[0]);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_SHADOW_AMBIENT_SGIX)");
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_GENERATE_MIPMAP_SGIS)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum mode = (GLenum) params[0];
            if (mode == GL_NONE || mode == GL_COMPARE_R_TO_TEXTURE_ARB) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareMode = mode;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_MODE_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum func = (GLenum) params[0];
            if (func == GL_LEQUAL || func == GL_GEQUAL) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else if (ctx->Extensions.EXT_shadow_funcs &&
                     (func == GL_EQUAL ||
                      func == GL_NOTEQUAL ||
                      func == GL_LESS ||
                      func == GL_GREATER ||
                      func == GL_ALWAYS ||
                      func == GL_NEVER)) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_FUNC_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_FUNC_ARB)");
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            const GLenum result = (GLenum) params[0];
            if (result == GL_LUMINANCE || result == GL_INTENSITY
                || result == GL_ALPHA) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->DepthMode = result;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                          "glTexParameter(bad GL_DEPTH_TEXTURE_MODE_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_DEPTH_TEXTURE_MODE_ARB)");
            return;
         }
         break;

      default:
         {
            char s[100];
            sprintf(s, "glTexParameter(pname=0x%x)", pname);
            _mesa_error( ctx, GL_INVALID_ENUM, s);
         }
         return;
   }

   texObj->Complete = GL_FALSE;

   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}


void
_mesa_TexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   _mesa_TexParameterfv(target, pname, fparam);
}

void
_mesa_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) params[0];
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   _mesa_TexParameterfv(target, pname, fparam);
}


void
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GLint iparam;
   _mesa_GetTexLevelParameteriv( target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}


static GLuint
tex_image_dimensions(GLcontext *ctx, GLenum target)
{
   switch (target) {
      case GL_TEXTURE_1D:
      case GL_PROXY_TEXTURE_1D:
         return 1;
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_2D:
         return 2;
      case GL_TEXTURE_3D:
      case GL_PROXY_TEXTURE_3D:
         return 3;
      case GL_TEXTURE_CUBE_MAP_ARB:
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
         return ctx->Extensions.ARB_texture_cube_map ? 2 : 0;
      default:
         _mesa_problem(ctx, "bad target in _mesa_tex_target_dimensions()");
         return 0;
   }
}


void
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   const struct gl_texture_image *img = NULL;
   GLuint dimensions;
   GLboolean isProxy;
   GLint maxLevels;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   dimensions = tex_image_dimensions(ctx, target);  /* 1, 2 or 3 */
   if (dimensions == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
      return;
   }

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      maxLevels = ctx->Const.MaxTextureLevels;
      break;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      maxLevels = ctx->Const.Max3DTextureLevels;
      break;
   default:
      maxLevels = ctx->Const.MaxCubeTextureLevels;
      break;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   img = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!img || !img->TexFormat) {
      /* undefined texture image */
      if (pname == GL_TEXTURE_COMPONENTS)
         *params = 1;
      else
         *params = 0;
      return;
   }

   isProxy = (target == GL_PROXY_TEXTURE_1D) ||
             (target == GL_PROXY_TEXTURE_2D) ||
             (target == GL_PROXY_TEXTURE_3D) ||
             (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB);

   switch (pname) {
      case GL_TEXTURE_WIDTH:
         *params = img->Width;
         return;
      case GL_TEXTURE_HEIGHT:
         *params = img->Height;
         return;
      case GL_TEXTURE_DEPTH:
         *params = img->Depth;
         return;
      case GL_TEXTURE_INTERNAL_FORMAT:
         *params = img->IntFormat;
         return;
      case GL_TEXTURE_BORDER:
         *params = img->Border;
         return;
      case GL_TEXTURE_RED_SIZE:
         if (img->Format == GL_RGB || img->Format == GL_RGBA)
            *params = img->TexFormat->RedBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_GREEN_SIZE:
         if (img->Format == GL_RGB || img->Format == GL_RGBA)
            *params = img->TexFormat->GreenBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_BLUE_SIZE:
         if (img->Format == GL_RGB || img->Format == GL_RGBA)
            *params = img->TexFormat->BlueBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_ALPHA_SIZE:
         if (img->Format == GL_ALPHA || img->Format == GL_LUMINANCE_ALPHA ||
             img->Format == GL_RGBA)
            *params = img->TexFormat->AlphaBits;
         else
            *params = 0;
         return;
      case GL_TEXTURE_INTENSITY_SIZE:
         if (img->Format != GL_INTENSITY)
            *params = 0;
         else if (img->TexFormat->IntensityBits > 0)
            *params = img->TexFormat->IntensityBits;
         else /* intensity probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         return;
      case GL_TEXTURE_LUMINANCE_SIZE:
         if (img->Format != GL_LUMINANCE &&
             img->Format != GL_LUMINANCE_ALPHA)
            *params = 0;
         else if (img->TexFormat->LuminanceBits > 0)
            *params = img->TexFormat->LuminanceBits;
         else /* luminance probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         return;
      case GL_TEXTURE_INDEX_SIZE_EXT:
         if (img->Format == GL_COLOR_INDEX)
            *params = img->TexFormat->IndexBits;
         else
            *params = 0;
         return;
      case GL_DEPTH_BITS:
         /* XXX this isn't in the GL_SGIX_depth_texture spec
          * but seems appropriate.
          */
         if (ctx->Extensions.SGIX_depth_texture)
            *params = img->TexFormat->DepthBits;
         else
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
         return;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
         if (ctx->Extensions.ARB_texture_compression) {
            if (img->IsCompressed && !isProxy)
               *params = img->CompressedSize;
            else
               _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
         }
         return;
      case GL_TEXTURE_COMPRESSED_ARB:
         if (ctx->Extensions.ARB_texture_compression) {
            *params = (GLint) img->IsCompressed;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
         }
         return;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
   }
}



void
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->MagFilter);
	 return;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->MinFilter);
         return;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->WrapS);
         return;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->WrapT);
         return;
      case GL_TEXTURE_WRAP_R_EXT:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         return;
#if 0 /* someday */
      case GL_TEXTURE_BORDER_VALUES_NV:
         /* unclamped */
         params[0] = obj->BorderValues[0];
         params[1] = obj->BorderValues[1];
         params[2] = obj->BorderValues[2];
         params[3] = obj->BorderValues[3];
         return;
#endif
      case GL_TEXTURE_BORDER_COLOR:
         /* clamped */
         params[0] = obj->BorderColor[0] / CHAN_MAXF;
         params[1] = obj->BorderColor[1] / CHAN_MAXF;
         params[2] = obj->BorderColor[2] / CHAN_MAXF;
         params[3] = obj->BorderColor[3] / CHAN_MAXF;
         return;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = ENUM_TO_FLOAT(resident);
         }
         return;
      case GL_TEXTURE_PRIORITY:
         *params = obj->Priority;
         return;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->MinLod;
         return;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->MaxLod;
         return;
      case GL_TEXTURE_BASE_LEVEL:
         *params = (GLfloat) obj->BaseLevel;
         return;
      case GL_TEXTURE_MAX_LEVEL:
         *params = (GLfloat) obj->MaxLevel;
         return;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = obj->MaxAnisotropy;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareFlag;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareOperator;
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = CHAN_TO_FLOAT(obj->ShadowAmbient);
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLfloat) obj->GenerateMipmap;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareMode;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareFunc;
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLfloat) obj->DepthMode;
            return;
         }
         break;
      default:
         ; /* silence warnings */
   }
   /* If we get here, pname was an unrecognized enum */
   _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname)" );
}


void
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
         return;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         return;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         return;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         return;
      case GL_TEXTURE_WRAP_R_EXT:
         *params = (GLint) obj->WrapR;
         return;
#if 0 /* someday */
      case GL_TEXTURE_BORDER_VALUES_NV:
         /* unclamped */
         params[0] = FLOAT_TO_INT(obj->BorderValues[0]);
         params[1] = FLOAT_TO_INT(obj->BorderValues[1]);
         params[2] = FLOAT_TO_INT(obj->BorderValues[2]);
         params[3] = FLOAT_TO_INT(obj->BorderValues[3]);
         return;
#endif
      case GL_TEXTURE_BORDER_COLOR:
         /* clamped */
         {
            GLfloat b[4];
            b[0] = CLAMP(obj->BorderValues[0], 0.0F, 1.0F);
            b[1] = CLAMP(obj->BorderValues[1], 0.0F, 1.0F);
            b[2] = CLAMP(obj->BorderValues[2], 0.0F, 1.0F);
            b[3] = CLAMP(obj->BorderValues[3], 0.0F, 1.0F);
            params[0] = FLOAT_TO_INT(b[0]);
            params[1] = FLOAT_TO_INT(b[1]);
            params[2] = FLOAT_TO_INT(b[2]);
            params[3] = FLOAT_TO_INT(b[3]);
         }
         return;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = (GLint) resident;
         }
         return;
      case GL_TEXTURE_PRIORITY:
         *params = (GLint) obj->Priority;
         return;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         return;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         return;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         return;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         return;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareFlag;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareOperator;
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            GLfloat a = CHAN_TO_FLOAT(obj->ShadowAmbient);
            *params = (GLint) FLOAT_TO_INT(a);
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLint) obj->GenerateMipmap;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareMode;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareFunc;
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLint) obj->DepthMode;
            return;
         }
         break;
      default:
         ; /* silence warnings */
   }
   /* If we get here, pname was an unrecognized enum */
   _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname)" );
}




/**********************************************************************/
/*                    Texture Coord Generation                        */
/**********************************************************************/


void
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "texGEN %s %s %x...\n",
	      _mesa_lookup_enum_by_nr(coord),
	      _mesa_lookup_enum_by_nr(pname),
	      *(int *)params);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bits;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bits = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       bits = TEXGEN_EYE_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       bits = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       bits = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_SPHERE_MAP:
	       bits = TEXGEN_SPHERE_MAP;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeS == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeS = mode;
	    texUnit->_GenBitS = bits;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneS, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneS[0] = params[0];
	    texUnit->ObjectPlaneS[1] = params[1];
	    texUnit->ObjectPlaneS[2] = params[2];
	    texUnit->ObjectPlaneS[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];

            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneS, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneS, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bitt;
	    switch (mode) {
               case GL_OBJECT_LINEAR:
                  bitt = TEXGEN_OBJ_LINEAR;
                  break;
               case GL_EYE_LINEAR:
                  bitt = TEXGEN_EYE_LINEAR;
                  break;
               case GL_REFLECTION_MAP_NV:
                  bitt = TEXGEN_REFLECTION_MAP_NV;
                  break;
               case GL_NORMAL_MAP_NV:
                  bitt = TEXGEN_NORMAL_MAP_NV;
                  break;
               case GL_SPHERE_MAP:
                  bitt = TEXGEN_SPHERE_MAP;
                  break;
               default:
                  _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
                  return;
	    }
	    if (texUnit->GenModeT == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeT = mode;
	    texUnit->_GenBitT = bitt;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneT, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneT[0] = params[0];
	    texUnit->ObjectPlaneT[1] = params[1];
	    texUnit->ObjectPlaneT[2] = params[2];
	    texUnit->ObjectPlaneT[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
	    if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneT, tmp))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneT, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bitr;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bitr = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       bitr = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       bitr = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_EYE_LINEAR:
	       bitr = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeR == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeR = mode;
	    texUnit->_GenBitR = bitr;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneR, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneR[0] = params[0];
	    texUnit->ObjectPlaneR[1] = params[1];
	    texUnit->ObjectPlaneR[2] = params[2];
	    texUnit->ObjectPlaneR[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneR, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneR, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLuint bitq;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bitq = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       bitq = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeQ == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeQ = mode;
	    texUnit->_GenBitQ = bitq;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneQ, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->ObjectPlaneQ[0] = params[0];
	    texUnit->ObjectPlaneQ[1] = params[1];
	    texUnit->ObjectPlaneQ[2] = params[2];
	    texUnit->ObjectPlaneQ[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelviewMatrixStack.Top->flags & MAT_DIRTY_INVERSE) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneQ, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneQ, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(coord)" );
	 return;
   }

   if (ctx->Driver.TexGen)
      ctx->Driver.TexGen( ctx, coord, pname, params );
}


void
_mesa_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
   _mesa_TexGenfv(coord, pname, p);
}


void
_mesa_TexGend(GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   _mesa_TexGenfv( coord, pname, &p );
}


void
_mesa_TexGendv(GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
   _mesa_TexGenfv( coord, pname, p );
}


void
_mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   _mesa_TexGenfv(coord, pname, &param);
}


void
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   _mesa_TexGeniv( coord, pname, &param );
}



void
_mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)" );
	 return;
   }
}



void
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)" );
	 return;
   }
}



void
_mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeS;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneS[0];
            params[1] = (GLint) texUnit->ObjectPlaneS[1];
            params[2] = (GLint) texUnit->ObjectPlaneS[2];
            params[3] = (GLint) texUnit->ObjectPlaneS[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneS[0];
            params[1] = (GLint) texUnit->EyePlaneS[1];
            params[2] = (GLint) texUnit->EyePlaneS[2];
            params[3] = (GLint) texUnit->EyePlaneS[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeT;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneT[0];
            params[1] = (GLint) texUnit->ObjectPlaneT[1];
            params[2] = (GLint) texUnit->ObjectPlaneT[2];
            params[3] = (GLint) texUnit->ObjectPlaneT[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneT[0];
            params[1] = (GLint) texUnit->EyePlaneT[1];
            params[2] = (GLint) texUnit->EyePlaneT[2];
            params[3] = (GLint) texUnit->EyePlaneT[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeR;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneR[0];
            params[1] = (GLint) texUnit->ObjectPlaneR[1];
            params[2] = (GLint) texUnit->ObjectPlaneR[2];
            params[3] = (GLint) texUnit->ObjectPlaneR[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneR[0];
            params[1] = (GLint) texUnit->EyePlaneR[1];
            params[2] = (GLint) texUnit->EyePlaneR[2];
            params[3] = (GLint) texUnit->EyePlaneR[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeQ;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneQ[0];
            params[1] = (GLint) texUnit->ObjectPlaneQ[1];
            params[2] = (GLint) texUnit->ObjectPlaneQ[2];
            params[3] = (GLint) texUnit->ObjectPlaneQ[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneQ[0];
            params[1] = (GLint) texUnit->EyePlaneQ[1];
            params[2] = (GLint) texUnit->EyePlaneQ[2];
            params[3] = (GLint) texUnit->EyePlaneQ[3];
         }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)" );
	 return;
   }
}


/* GL_ARB_multitexture */
void
_mesa_ActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint texUnit = target - GL_TEXTURE0_ARB;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glActiveTexture %s\n",
	      _mesa_lookup_enum_by_nr(target));

   if (texUnit > ctx->Const.MaxTextureUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glActiveTextureARB(target)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   ctx->Texture.CurrentUnit = texUnit;
   if (ctx->Driver.ActiveTexture) {
      (*ctx->Driver.ActiveTexture)( ctx, (GLuint) texUnit );
   }
}


/* GL_ARB_multitexture */
void
_mesa_ClientActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint texUnit = target - GL_TEXTURE0_ARB;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (texUnit > ctx->Const.MaxTextureUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClientActiveTextureARB(target)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ActiveTexture = texUnit;
}



/**********************************************************************/
/*                     Pixel Texgen Extensions                        */
/**********************************************************************/

void
_mesa_PixelTexGenSGIX(GLenum mode)
{
   GLenum newRgbSource, newAlphaSource;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (mode) {
      case GL_NONE:
         newRgbSource = GL_PIXEL_GROUP_COLOR_SGIS;
         newAlphaSource = GL_PIXEL_GROUP_COLOR_SGIS;
         break;
      case GL_ALPHA:
         newRgbSource = GL_PIXEL_GROUP_COLOR_SGIS;
         newAlphaSource = GL_CURRENT_RASTER_COLOR;
         break;
      case GL_RGB:
         newRgbSource = GL_CURRENT_RASTER_COLOR;
         newAlphaSource = GL_PIXEL_GROUP_COLOR_SGIS;
         break;
      case GL_RGBA:
         newRgbSource = GL_CURRENT_RASTER_COLOR;
         newAlphaSource = GL_CURRENT_RASTER_COLOR;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glPixelTexGenSGIX(mode)");
         return;
   }

   if (newRgbSource == ctx->Pixel.FragmentRgbSource &&
       newAlphaSource == ctx->Pixel.FragmentAlphaSource)
      return;

   FLUSH_VERTICES(ctx, _NEW_PIXEL);
   ctx->Pixel.FragmentRgbSource = newRgbSource;
   ctx->Pixel.FragmentAlphaSource = newAlphaSource;
}


void
_mesa_PixelTexGenParameterfSGIS(GLenum target, GLfloat value)
{
   _mesa_PixelTexGenParameteriSGIS(target, (GLint) value);
}


void
_mesa_PixelTexGenParameterfvSGIS(GLenum target, const GLfloat *value)
{
   _mesa_PixelTexGenParameteriSGIS(target, (GLint) *value);
}


void
_mesa_PixelTexGenParameteriSGIS(GLenum target, GLint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (value != GL_CURRENT_RASTER_COLOR && value != GL_PIXEL_GROUP_COLOR_SGIS) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelTexGenParameterSGIS(value)");
      return;
   }

   switch (target) {
   case GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS:
      if (ctx->Pixel.FragmentRgbSource == (GLenum) value)
	 return;
      FLUSH_VERTICES(ctx, _NEW_PIXEL);
      ctx->Pixel.FragmentRgbSource = (GLenum) value;
      break;
   case GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS:
      if (ctx->Pixel.FragmentAlphaSource == (GLenum) value)
	 return;
      FLUSH_VERTICES(ctx, _NEW_PIXEL);
      ctx->Pixel.FragmentAlphaSource = (GLenum) value;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelTexGenParameterSGIS(target)");
      return;
   }
}


void
_mesa_PixelTexGenParameterivSGIS(GLenum target, const GLint *value)
{
  _mesa_PixelTexGenParameteriSGIS(target, *value);
}


void
_mesa_GetPixelTexGenParameterfvSGIS(GLenum target, GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS) {
      *value = (GLfloat) ctx->Pixel.FragmentRgbSource;
   }
   else if (target == GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS) {
      *value = (GLfloat) ctx->Pixel.FragmentAlphaSource;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelTexGenParameterfvSGIS(target)");
   }
}


void
_mesa_GetPixelTexGenParameterivSGIS(GLenum target, GLint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS) {
      *value = (GLint) ctx->Pixel.FragmentRgbSource;
   }
   else if (target == GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS) {
      *value = (GLint) ctx->Pixel.FragmentAlphaSource;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelTexGenParameterivSGIS(target)");
   }
}
