/* $Id: texstate.c,v 1.13 2000/05/23 20:10:50 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#include "context.h"
#include "enums.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "texture.h"
#include "types.h"
#include "xform.h"
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

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexEnv");

   if (target==GL_TEXTURE_ENV) {

      if (pname==GL_TEXTURE_ENV_MODE) {
	 GLenum mode = (GLenum) (GLint) *param;
	 switch (mode) {
	 case GL_ADD:
	    if (!ctx->Extensions.HaveTextureEnvAdd) {
	       gl_error(ctx, GL_INVALID_ENUM, "glTexEnv(param)");
	       return;
	    }
	    /* FALL-THROUGH */
	 case GL_MODULATE:
	 case GL_BLEND:
	 case GL_DECAL:
	 case GL_REPLACE:
	    /* A small optimization for drivers */ 
	    if (texUnit->EnvMode == mode)
	       return;

	    if (MESA_VERBOSE & (VERBOSE_STATE|VERBOSE_TEXTURE))
	       fprintf(stderr, "glTexEnv: old mode %s, new mode %s\n",
		       gl_lookup_enum_by_nr(texUnit->EnvMode),
		       gl_lookup_enum_by_nr(mode));

	    texUnit->EnvMode = mode;
	    ctx->NewState |= NEW_TEXTURE_ENV;
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_VALUE, "glTexEnv(param)" );
	    return;
	 }
      }
      else if (pname==GL_TEXTURE_ENV_COLOR) {
	 texUnit->EnvColor[0] = CLAMP( param[0], 0.0F, 1.0F );
	 texUnit->EnvColor[1] = CLAMP( param[1], 0.0F, 1.0F );
	 texUnit->EnvColor[2] = CLAMP( param[2], 0.0F, 1.0F );
	 texUnit->EnvColor[3] = CLAMP( param[3], 0.0F, 1.0F );
      }
      else {
	 gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
	 return;
      }

   }
   else if (target==GL_TEXTURE_FILTER_CONTROL_EXT) {

      if (!ctx->Extensions.HaveTextureLodBias) {
	 gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(param)" );
	 return;
      }

      if (pname==GL_TEXTURE_LOD_BIAS_EXT) {
	 texUnit->LodBias = param[0];
      }
      else {
	 gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
	 return;
      }

   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(target)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glTexEnv %s %s %.1f(%s) ...\n",  
	      gl_lookup_enum_by_nr(target),
	      gl_lookup_enum_by_nr(pname),
	      *param,
	      gl_lookup_enum_by_nr((GLenum) (GLint) *param));

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
   p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
   p[3] = INT_TO_FLOAT( param[3] );
   _mesa_TexEnvfv( target, pname, p );
}


void
_mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexEnvfv");

   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }
   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = ENUM_TO_FLOAT(texUnit->EnvMode);
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 COPY_4FV( params, texUnit->EnvColor );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
   }
}


void
_mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexEnviv");

   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
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
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
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

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexParameterfv");

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "texPARAM %s %s %d...\n", 
	      gl_lookup_enum_by_nr(target),
	      gl_lookup_enum_by_nr(pname),
	      eparam);


   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->CurrentD[1];
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->CurrentD[2];
         break;
      case GL_TEXTURE_3D_EXT:
         texObj = texUnit->CurrentD[3];
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (ctx->Extensions.HaveTextureCubeMap) {
            texObj = texUnit->CurrentCubeMap;
            break;
         }
         /* fallthrough */
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
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
            texObj->MinFilter = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         /* A small optimization */
         if (texObj->MagFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            texObj->MagFilter = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (texObj->WrapS == eparam)
            return;

         if (eparam==GL_CLAMP || eparam==GL_REPEAT || eparam==GL_CLAMP_TO_EDGE) {
            texObj->WrapS = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (texObj->WrapT == eparam)
            return;

         if (eparam==GL_CLAMP || eparam==GL_REPEAT || eparam==GL_CLAMP_TO_EDGE) {
            texObj->WrapT = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         if (texObj->WrapR == eparam)
            return;

         if (eparam==GL_CLAMP || eparam==GL_REPEAT || eparam==GL_CLAMP_TO_EDGE) {
            texObj->WrapR = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
         }
         break;
      case GL_TEXTURE_BORDER_COLOR:
         texObj->BorderColor[0] = (GLubyte) CLAMP((GLint)(params[0]*255.0), 0, 255);
         texObj->BorderColor[1] = (GLubyte) CLAMP((GLint)(params[1]*255.0), 0, 255);
         texObj->BorderColor[2] = (GLubyte) CLAMP((GLint)(params[2]*255.0), 0, 255);
         texObj->BorderColor[3] = (GLubyte) CLAMP((GLint)(params[3]*255.0), 0, 255);
         break;
      case GL_TEXTURE_MIN_LOD:
         texObj->MinLod = params[0];
         ctx->NewState |= NEW_TEXTURING;
         break;
      case GL_TEXTURE_MAX_LOD:
         texObj->MaxLod = params[0];
         ctx->NewState |= NEW_TEXTURING;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         if (params[0] < 0.0) {
            gl_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         texObj->BaseLevel = (GLint) params[0];
         ctx->NewState |= NEW_TEXTURING;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         if (params[0] < 0.0) {
            gl_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         texObj->MaxLevel = (GLint) params[0];
         ctx->NewState |= NEW_TEXTURING;
         break;
      case GL_TEXTURE_PRIORITY:
         /* (keithh@netcomuk.co.uk) */
         texObj->Priority = CLAMP( params[0], 0.0F, 1.0F );
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexParameter(pname)" );
         return;
   }

   gl_put_texobj_on_dirty_list( ctx, texObj );

   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}


void
_mesa_TexParameteri( GLenum target, GLenum pname, const GLint param )
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
         return ctx->Extensions.HaveTextureCubeMap ? 2 : 0;
      default:
         gl_problem(ctx, "bad target in _mesa_tex_target_dimensions()");
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

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexLevelParameter");

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   dimensions = tex_image_dimensions(ctx, target);  /* 1, 2 or 3 */
   if (dimensions == 0) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
      return;
   }

   img = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!img) {
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
         if (dimensions > 1) {
            *params = img->Height;
         }
         else {
            gl_error( ctx, GL_INVALID_ENUM,
                      "glGetTexLevelParameter[if]v(pname=GL_TEXTURE_HEIGHT)" );
         }
         return;
      case GL_TEXTURE_DEPTH:
         if (dimensions > 2) {
            *params = img->Depth;
         }
         else {
            gl_error( ctx, GL_INVALID_ENUM,
                      "glGetTexLevelParameter[if]v(pname=GL_TEXTURE_DEPTH)" );
         }
         return;
      case GL_TEXTURE_COMPONENTS:
         *params = img->IntFormat;
         return;
      case GL_TEXTURE_BORDER:
         *params = img->Border;
         return;
      case GL_TEXTURE_RED_SIZE:
         *params = img->RedBits;
         return;
      case GL_TEXTURE_GREEN_SIZE:
         *params = img->GreenBits;
         return;
      case GL_TEXTURE_BLUE_SIZE:
         *params = img->BlueBits;
         return;
      case GL_TEXTURE_ALPHA_SIZE:
         *params = img->AlphaBits;
         return;
      case GL_TEXTURE_INTENSITY_SIZE:
         *params = img->IntensityBits;
         return;
      case GL_TEXTURE_LUMINANCE_SIZE:
         *params = img->LuminanceBits;
         return;
      case GL_TEXTURE_INDEX_SIZE_EXT:
         *params = img->IndexBits;
         return;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
         if (ctx->Extensions.HaveTextureCompression) {
            if (img->IsCompressed && !isProxy)
               *params = img->CompressedSize;
            else
               gl_error(ctx, GL_INVALID_OPERATION,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
         }
         return;
      case GL_TEXTURE_COMPRESSED_ARB:
         if (ctx->Extensions.HaveTextureCompression) {
            *params = (GLint) img->IsCompressed;
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
         }
         return;

      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(pname)");
   }
}



void
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexParameterfv");

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->MagFilter);
	 break;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->MinFilter);
         break;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->WrapS);
         break;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->WrapT);
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         break;
      case GL_TEXTURE_BORDER_COLOR:
         params[0] = obj->BorderColor[0] / 255.0F;
         params[1] = obj->BorderColor[1] / 255.0F;
         params[2] = obj->BorderColor[2] / 255.0F;
         params[3] = obj->BorderColor[3] / 255.0F;
         break;
      case GL_TEXTURE_RESIDENT:
         *params = ENUM_TO_FLOAT(GL_TRUE);
         break;
      case GL_TEXTURE_PRIORITY:
         *params = obj->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = (GLfloat) obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = (GLfloat) obj->MaxLevel;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname)" );
   }
}


void
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexParameteriv");

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      gl_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
         break;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         break;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         break;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         *params = (GLint) obj->WrapR;
         break;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat color[4];
            color[0] = obj->BorderColor[0] / 255.0F;
            color[1] = obj->BorderColor[1] / 255.0F;
            color[2] = obj->BorderColor[2] / 255.0F;
            color[3] = obj->BorderColor[3] / 255.0F;
            params[0] = FLOAT_TO_INT( color[0] );
            params[1] = FLOAT_TO_INT( color[1] );
            params[2] = FLOAT_TO_INT( color[2] );
            params[3] = FLOAT_TO_INT( color[3] );
         }
         break;
      case GL_TEXTURE_RESIDENT:
         *params = (GLint) GL_TRUE;
         break;
      case GL_TEXTURE_PRIORITY:
         *params = (GLint) obj->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname)" );
   }
}




/**********************************************************************/
/*                    Texture Coord Generation                        */
/**********************************************************************/


void
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexGenfv");

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "texGEN %s %s %x...\n", 
	      gl_lookup_enum_by_nr(coord),
	      gl_lookup_enum_by_nr(pname),
	      *(int *)params);

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       texUnit->GenModeS = mode;
	       texUnit->GenBitS = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       texUnit->GenModeS = mode;
	       texUnit->GenBitS = TEXGEN_EYE_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       texUnit->GenModeS = mode;
	       texUnit->GenBitS = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       texUnit->GenModeS = mode;
	       texUnit->GenBitS = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_SPHERE_MAP:
	       texUnit->GenModeS = mode;
	       texUnit->GenBitS = TEXGEN_SPHERE_MAP;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneS[0] = params[0];
	    texUnit->ObjectPlaneS[1] = params[1];
	    texUnit->ObjectPlaneS[2] = params[2];
	    texUnit->ObjectPlaneS[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneS, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    switch (mode) {
               case GL_OBJECT_LINEAR:
                  texUnit->GenModeT = GL_OBJECT_LINEAR;
                  texUnit->GenBitT = TEXGEN_OBJ_LINEAR;
                  break;
               case GL_EYE_LINEAR:
                  texUnit->GenModeT = GL_EYE_LINEAR;
                  texUnit->GenBitT = TEXGEN_EYE_LINEAR;
                  break;
               case GL_REFLECTION_MAP_NV:
                  texUnit->GenModeT = GL_REFLECTION_MAP_NV;
                  texUnit->GenBitT = TEXGEN_REFLECTION_MAP_NV;
                  break;
               case GL_NORMAL_MAP_NV:
                  texUnit->GenModeT = GL_NORMAL_MAP_NV;
                  texUnit->GenBitT = TEXGEN_NORMAL_MAP_NV;
                  break;
               case GL_SPHERE_MAP:
                  texUnit->GenModeT = GL_SPHERE_MAP;
                  texUnit->GenBitT = TEXGEN_SPHERE_MAP;
                  break;
               default:
                  gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
                  return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneT[0] = params[0];
	    texUnit->ObjectPlaneT[1] = params[1];
	    texUnit->ObjectPlaneT[2] = params[2];
	    texUnit->ObjectPlaneT[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneT, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       texUnit->GenModeR = GL_OBJECT_LINEAR;
	       texUnit->GenBitR = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       texUnit->GenModeR = GL_REFLECTION_MAP_NV;
	       texUnit->GenBitR = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       texUnit->GenModeR = GL_NORMAL_MAP_NV;
	       texUnit->GenBitR = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_EYE_LINEAR:
	       texUnit->GenModeR = GL_EYE_LINEAR;
	       texUnit->GenBitR = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneR[0] = params[0];
	    texUnit->ObjectPlaneR[1] = params[1];
	    texUnit->ObjectPlaneR[2] = params[2];
	    texUnit->ObjectPlaneR[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneR, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    switch (mode) {
	    case GL_OBJECT_LINEAR: 
	       texUnit->GenModeQ = GL_OBJECT_LINEAR;
	       texUnit->GenBitQ = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       texUnit->GenModeQ = GL_EYE_LINEAR;
	       texUnit->GenBitQ = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneQ[0] = params[0];
	    texUnit->ObjectPlaneQ[1] = params[1];
	    texUnit->ObjectPlaneQ[2] = params[2];
	    texUnit->ObjectPlaneQ[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneQ, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(coord)" );
	 return;
   }

   ctx->NewState |= NEW_TEXTURING;
}


void
_mesa_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
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
   p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
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
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexGendv");

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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)" );
	 return;
   }
}



void
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexGenfv");

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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)" );
	 return;
   }
}



void
_mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexGeniv");

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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
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
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)" );
	 return;
   }
}


/* GL_ARB_multitexture */
void
_mesa_ActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint maxUnits = ctx->Const.MaxTextureUnits;

   ASSERT_OUTSIDE_BEGIN_END( ctx, "glActiveTextureARB" );

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glActiveTexture %s\n", 
	      gl_lookup_enum_by_nr(target));

   if (target >= GL_TEXTURE0_ARB && target < GL_TEXTURE0_ARB + maxUnits) {
      GLint texUnit = target - GL_TEXTURE0_ARB;
      ctx->Texture.CurrentUnit = texUnit;
      ctx->Texture.CurrentTransformUnit = texUnit;
      if (ctx->Driver.ActiveTexture) {
         (*ctx->Driver.ActiveTexture)( ctx, (GLuint) texUnit );
      }
   }
   else {
      gl_error(ctx, GL_INVALID_OPERATION, "glActiveTextureARB(target)");
   }
}


/* GL_ARB_multitexture */
void
_mesa_ClientActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint maxUnits = ctx->Const.MaxTextureUnits;

   ASSERT_OUTSIDE_BEGIN_END( ctx, "glClientActiveTextureARB" );

   if (target >= GL_TEXTURE0_ARB && target < GL_TEXTURE0_ARB + maxUnits) {
      GLint texUnit = target - GL_TEXTURE0_ARB;
      ctx->Array.ActiveTexture = texUnit;
   }
   else {
      gl_error(ctx, GL_INVALID_OPERATION, "glActiveTextureARB(target)");
   }
}



/*
 * Put the given texture object into the list of dirty texture objects.
 * When a texture object is dirty we have to reexamine it for completeness
 * and perhaps choose a different texture sampling function.
 */
void gl_put_texobj_on_dirty_list( GLcontext *ctx, struct gl_texture_object *t )
{
   ASSERT(ctx);
   ASSERT(t);
   /* Only insert if not already in the dirty list.
    * The Dirty flag is only set iff the texture object is in the dirty list.
    */
   if (!t->Dirty) {
      ASSERT(t->NextDirty == NULL);
      t->Dirty = GL_TRUE;
      t->NextDirty = ctx->Shared->DirtyTexObjList;
      ctx->Shared->DirtyTexObjList = t;
   }
#ifdef DEBUG
   else {
      /* make sure t is in the list */
      struct gl_texture_object *obj = ctx->Shared->DirtyTexObjList;
      while (obj) {
         if (obj == t) {
            return;
         }
         obj = obj->NextDirty;
      }
      gl_problem(ctx, "Error in gl_put_texobj_on_dirty_list");
   }
#endif
}


/*
 * Remove a texture object from the dirty texture list.
 */
void gl_remove_texobj_from_dirty_list( struct gl_shared_state *shared,
                                       struct gl_texture_object *tObj )
{
   struct gl_texture_object *t, *prev = NULL;
   ASSERT(shared);
   ASSERT(tObj);
   for (t = shared->DirtyTexObjList; t; t = t->NextDirty) {
      if (t == tObj) {
         if (prev) {
            prev->NextDirty = t->NextDirty;
         }
         else {
            shared->DirtyTexObjList = t->NextDirty;
         }
         return;
      }
      prev = t;
   }
}


/*
 * This is called by gl_update_state() if the NEW_TEXTURING bit in
 * ctx->NewState is set.
 */
void gl_update_dirty_texobjs( GLcontext *ctx )
{
   struct gl_texture_object *t, *next;
   for (t = ctx->Shared->DirtyTexObjList; t; t = next) {
      next = t->NextDirty;
      _mesa_test_texobj_completeness(ctx, t);
      _mesa_set_texture_sampler(t);
      t->NextDirty = NULL;
      t->Dirty = GL_FALSE;
   }
   ctx->Shared->DirtyTexObjList = NULL;
}
