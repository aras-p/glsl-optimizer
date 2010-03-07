/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

/** 
 * \file texparam.c
 *
 * glTexParameter-related functions
 */


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/macros.h"
#include "main/texcompress.h"
#include "main/texparam.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "shader/prog_instruction.h"


/**
 * Check if a coordinate wrap mode is supported for the texture target.
 * \return GL_TRUE if legal, GL_FALSE otherwise
 */
static GLboolean 
validate_texture_wrap_mode(GLcontext * ctx, GLenum target, GLenum wrap)
{
   const struct gl_extensions * const e = & ctx->Extensions;

   if (wrap == GL_CLAMP || wrap == GL_CLAMP_TO_EDGE ||
       (wrap == GL_CLAMP_TO_BORDER && e->ARB_texture_border_clamp)) {
      /* any texture target */
      return GL_TRUE;
   }
   else if (target != GL_TEXTURE_RECTANGLE_NV &&
	    (wrap == GL_REPEAT ||
	     (wrap == GL_MIRRORED_REPEAT &&
	      e->ARB_texture_mirrored_repeat) ||
	     (wrap == GL_MIRROR_CLAMP_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (wrap == GL_MIRROR_CLAMP_TO_EDGE_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (wrap == GL_MIRROR_CLAMP_TO_BORDER_EXT &&
	      (e->EXT_texture_mirror_clamp)))) {
      /* non-rectangle texture */
      return GL_TRUE;
   }

   _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(param=0x%x)", wrap );
   return GL_FALSE;
}


/**
 * Get current texture object for given target.
 * Return NULL if any error (and record the error).
 * Note that this is different from _mesa_select_tex_object() in that proxy
 * targets are not accepted.
 * Only the glGetTexLevelParameter() functions accept proxy targets.
 */
static struct gl_texture_object *
get_texobj(GLcontext *ctx, GLenum target, GLboolean get)
{
   struct gl_texture_unit *texUnit;

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "gl%sTexParameter(current unit)", get ? "Get" : "");
      return NULL;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   switch (target) {
   case GL_TEXTURE_1D:
      return texUnit->CurrentTex[TEXTURE_1D_INDEX];
   case GL_TEXTURE_2D:
      return texUnit->CurrentTex[TEXTURE_2D_INDEX];
   case GL_TEXTURE_3D:
      return texUnit->CurrentTex[TEXTURE_3D_INDEX];
   case GL_TEXTURE_CUBE_MAP:
      if (ctx->Extensions.ARB_texture_cube_map) {
         return texUnit->CurrentTex[TEXTURE_CUBE_INDEX];
      }
      break;
   case GL_TEXTURE_RECTANGLE_NV:
      if (ctx->Extensions.NV_texture_rectangle) {
         return texUnit->CurrentTex[TEXTURE_RECT_INDEX];
      }
      break;
   case GL_TEXTURE_1D_ARRAY_EXT:
      if (ctx->Extensions.MESA_texture_array) {
         return texUnit->CurrentTex[TEXTURE_1D_ARRAY_INDEX];
      }
      break;
   case GL_TEXTURE_2D_ARRAY_EXT:
      if (ctx->Extensions.MESA_texture_array) {
         return texUnit->CurrentTex[TEXTURE_2D_ARRAY_INDEX];
      }
      break;
   default:
      ;
   }

   _mesa_error(ctx, GL_INVALID_ENUM,
                  "gl%sTexParameter(target)", get ? "Get" : "");
   return NULL;
}


/**
 * Convert GL_RED/GREEN/BLUE/ALPHA/ZERO/ONE to SWIZZLE_X/Y/Z/W/ZERO/ONE.
 * \return -1 if error.
 */
static GLint
comp_to_swizzle(GLenum comp)
{
   switch (comp) {
   case GL_RED:
      return SWIZZLE_X;
   case GL_GREEN:
      return SWIZZLE_Y;
   case GL_BLUE:
      return SWIZZLE_Z;
   case GL_ALPHA:
      return SWIZZLE_W;
   case GL_ZERO:
      return SWIZZLE_ZERO;
   case GL_ONE:
      return SWIZZLE_ONE;
   default:
      return -1;
   }
}


static void
set_swizzle_component(GLuint *swizzle, GLuint comp, GLuint swz)
{
   ASSERT(comp < 4);
   ASSERT(swz <= SWIZZLE_NIL);
   {
      GLuint mask = 0x7 << (3 * comp);
      GLuint s = (*swizzle & ~mask) | (swz << (3 * comp));
      *swizzle = s;
   }
}


/**
 * This is called just prior to changing any texture object state.
 * Any pending rendering will be flushed out, we'll set the _NEW_TEXTURE
 * state flag and then mark the texture object as 'incomplete' so that any
 * per-texture derived state gets recomputed.
 */
static INLINE void
flush(GLcontext *ctx, struct gl_texture_object *texObj)
{
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   texObj->_Complete = GL_FALSE;
}


/**
 * Set an integer-valued texture parameter
 * \return GL_TRUE if legal AND the value changed, GL_FALSE otherwise
 */
static GLboolean
set_tex_parameteri(GLcontext *ctx,
                   struct gl_texture_object *texObj,
                   GLenum pname, const GLint *params)
{
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
      if (texObj->MinFilter == params[0])
         return GL_FALSE;
      switch (params[0]) {
      case GL_NEAREST:
      case GL_LINEAR:
         flush(ctx, texObj);
         texObj->MinFilter = params[0];
         return GL_TRUE;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
         if (texObj->Target != GL_TEXTURE_RECTANGLE_NV) {
            flush(ctx, texObj);
            texObj->MinFilter = params[0];
            return GL_TRUE;
         }
         /* fall-through */
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(param=0x%x)",
                      params[0] );
      }
      return GL_FALSE;

   case GL_TEXTURE_MAG_FILTER:
      if (texObj->MagFilter == params[0])
         return GL_FALSE;
      switch (params[0]) {
      case GL_NEAREST:
      case GL_LINEAR:
         flush(ctx, texObj);
         texObj->MagFilter = params[0];
         return GL_TRUE;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(param=0x%x)",
                      params[0]);
      }
      return GL_FALSE;

   case GL_TEXTURE_WRAP_S:
      if (texObj->WrapS == params[0])
         return GL_FALSE;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx, texObj);
         texObj->WrapS = params[0];
         return GL_TRUE;
      }
      return GL_FALSE;

   case GL_TEXTURE_WRAP_T:
      if (texObj->WrapT == params[0])
         return GL_FALSE;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx, texObj);
         texObj->WrapT = params[0];
         return GL_TRUE;
      }
      return GL_FALSE;

   case GL_TEXTURE_WRAP_R:
      if (texObj->WrapR == params[0])
         return GL_FALSE;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx, texObj);
         texObj->WrapR = params[0];
         return GL_TRUE;
      }
      return GL_FALSE;

   case GL_TEXTURE_BASE_LEVEL:
      if (texObj->BaseLevel == params[0])
         return GL_FALSE;
      if (params[0] < 0 ||
          (texObj->Target == GL_TEXTURE_RECTANGLE_ARB && params[0] != 0)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexParameter(param=%d)", params[0]);
         return GL_FALSE;
      }
      flush(ctx, texObj);
      texObj->BaseLevel = params[0];
      return GL_TRUE;

   case GL_TEXTURE_MAX_LEVEL:
      if (texObj->MaxLevel == params[0])
         return GL_FALSE;
      if (params[0] < 0 || texObj->Target == GL_TEXTURE_RECTANGLE_ARB) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexParameter(param=%d)", params[0]);
         return GL_FALSE;
      }
      flush(ctx, texObj);
      texObj->MaxLevel = params[0];
      return GL_TRUE;

   case GL_GENERATE_MIPMAP_SGIS:
      if (ctx->Extensions.SGIS_generate_mipmap) {
         if (texObj->GenerateMipmap != params[0]) {
            flush(ctx, texObj);
            texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
            return GL_TRUE;
         }
         return GL_FALSE;
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=GL_GENERATE_MIPMAP_SGIS)");
      }
      return GL_FALSE;

   case GL_TEXTURE_COMPARE_MODE_ARB:
      if (ctx->Extensions.ARB_shadow &&
          (params[0] == GL_NONE ||
           params[0] == GL_COMPARE_R_TO_TEXTURE_ARB)) {
         if (texObj->CompareMode != params[0]) {
            flush(ctx, texObj);
            texObj->CompareMode = params[0];
            return GL_TRUE;
         }
         return GL_FALSE;
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(GL_TEXTURE_COMPARE_MODE_ARB)");
      }
      return GL_FALSE;

   case GL_TEXTURE_COMPARE_FUNC_ARB:
      if (ctx->Extensions.ARB_shadow) {
         if (texObj->CompareFunc == params[0])
            return GL_FALSE;
         switch (params[0]) {
         case GL_LEQUAL:
         case GL_GEQUAL:
            flush(ctx, texObj);
            texObj->CompareFunc = params[0];
            return GL_TRUE;
         case GL_EQUAL:
         case GL_NOTEQUAL:
         case GL_LESS:
         case GL_GREATER:
         case GL_ALWAYS:
         case GL_NEVER:
            if (ctx->Extensions.EXT_shadow_funcs) {
               flush(ctx, texObj);
               texObj->CompareFunc = params[0];
               return GL_TRUE;
            }
            /* fall-through */
         default:
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(GL_TEXTURE_COMPARE_FUNC_ARB)");
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
      }
      return GL_FALSE;

   case GL_DEPTH_TEXTURE_MODE_ARB:
      if (ctx->Extensions.ARB_depth_texture &&
          (params[0] == GL_LUMINANCE ||
           params[0] == GL_INTENSITY ||
           params[0] == GL_ALPHA)) {
         if (texObj->DepthMode != params[0]) {
            flush(ctx, texObj);
            texObj->DepthMode = params[0];
            return GL_TRUE;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(GL_DEPTH_TEXTURE_MODE_ARB)");
      }
      return GL_FALSE;

#ifdef FEATURE_OES_draw_texture
   case GL_TEXTURE_CROP_RECT_OES:
      texObj->CropRect[0] = params[0];
      texObj->CropRect[1] = params[1];
      texObj->CropRect[2] = params[2];
      texObj->CropRect[3] = params[3];
      return GL_TRUE;
#endif

   case GL_TEXTURE_SWIZZLE_R_EXT:
   case GL_TEXTURE_SWIZZLE_G_EXT:
   case GL_TEXTURE_SWIZZLE_B_EXT:
   case GL_TEXTURE_SWIZZLE_A_EXT:
      if (ctx->Extensions.EXT_texture_swizzle) {
         const GLuint comp = pname - GL_TEXTURE_SWIZZLE_R_EXT;
         const GLint swz = comp_to_swizzle(params[0]);
         if (swz < 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glTexParameter(swizzle 0x%x)", params[0]);
            return GL_FALSE;
         }
         ASSERT(comp < 4);
         if (swz >= 0) {
            flush(ctx, texObj);
            texObj->Swizzle[comp] = params[0];
            set_swizzle_component(&texObj->_Swizzle, comp, swz);
            return GL_TRUE;
         }
      }
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
      return GL_FALSE;

   case GL_TEXTURE_SWIZZLE_RGBA_EXT:
      if (ctx->Extensions.EXT_texture_swizzle) {
         GLuint comp;
         flush(ctx, texObj);
         for (comp = 0; comp < 4; comp++) {
            const GLint swz = comp_to_swizzle(params[comp]);
            if (swz >= 0) {
               texObj->Swizzle[comp] = params[comp];
               set_swizzle_component(&texObj->_Swizzle, comp, swz);
            }
            else {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glTexParameter(swizzle 0x%x)", params[comp]);
               return GL_FALSE;
            }
         }
         return GL_TRUE;
      }
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
      return GL_FALSE;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
   }
   return GL_FALSE;
}


/**
 * Set a float-valued texture parameter
 * \return GL_TRUE if legal AND the value changed, GL_FALSE otherwise
 */
static GLboolean
set_tex_parameterf(GLcontext *ctx,
                   struct gl_texture_object *texObj,
                   GLenum pname, const GLfloat *params)
{
   switch (pname) {
   case GL_TEXTURE_MIN_LOD:
      if (texObj->MinLod == params[0])
         return GL_FALSE;
      flush(ctx, texObj);
      texObj->MinLod = params[0];
      return GL_TRUE;

   case GL_TEXTURE_MAX_LOD:
      if (texObj->MaxLod == params[0])
         return GL_FALSE;
      flush(ctx, texObj);
      texObj->MaxLod = params[0];
      return GL_TRUE;

   case GL_TEXTURE_PRIORITY:
      flush(ctx, texObj);
      texObj->Priority = CLAMP(params[0], 0.0F, 1.0F);
      return GL_TRUE;

   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      if (ctx->Extensions.EXT_texture_filter_anisotropic) {
         if (texObj->MaxAnisotropy == params[0])
            return GL_FALSE;
         if (params[0] < 1.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return GL_FALSE;
         }
         flush(ctx, texObj);
         /* clamp to max, that's what NVIDIA does */
         texObj->MaxAnisotropy = MIN2(params[0],
                                      ctx->Const.MaxTextureMaxAnisotropy);
         return GL_TRUE;
      }
      else {
         static GLuint count = 0;
         if (count++ < 10)
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_MAX_ANISOTROPY_EXT)");
      }
      return GL_FALSE;

   case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
      if (ctx->Extensions.ARB_shadow_ambient) {
         if (texObj->CompareFailValue != params[0]) {
            flush(ctx, texObj);
            texObj->CompareFailValue = CLAMP(params[0], 0.0F, 1.0F);
            return GL_TRUE;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                    "glTexParameter(pname=GL_TEXTURE_COMPARE_FAIL_VALUE_ARB)");
      }
      return GL_FALSE;

   case GL_TEXTURE_LOD_BIAS:
      /* NOTE: this is really part of OpenGL 1.4, not EXT_texture_lod_bias */
      if (ctx->Extensions.EXT_texture_lod_bias) {
         if (texObj->LodBias != params[0]) {
            flush(ctx, texObj);
            texObj->LodBias = params[0];
            return GL_TRUE;
         }
         return GL_FALSE;
      }
      break;

   case GL_TEXTURE_BORDER_COLOR:
      flush(ctx, texObj);
      texObj->BorderColor.f[RCOMP] = params[0];
      texObj->BorderColor.f[GCOMP] = params[1];
      texObj->BorderColor.f[BCOMP] = params[2];
      texObj->BorderColor.f[ACOMP] = params[3];
      return GL_TRUE;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
   }
   return GL_FALSE;
}


void GLAPIENTRY
_mesa_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_GENERATE_MIPMAP_SGIS:
   case GL_TEXTURE_COMPARE_MODE_ARB:
   case GL_TEXTURE_COMPARE_FUNC_ARB:
   case GL_DEPTH_TEXTURE_MODE_ARB:
      {
         /* convert float param to int */
         GLint p[4];
         p[0] = (GLint) param;
         p[1] = p[2] = p[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, p);
      }
      break;
   default:
      {
         /* this will generate an error if pname is illegal */
         GLfloat p[4];
         p[0] = param;
         p[1] = p[2] = p[3] = 0.0F;
         need_update = set_tex_parameterf(ctx, texObj, pname, p);
      }
   }

   if (ctx->Driver.TexParameter && need_update) {
      ctx->Driver.TexParameter(ctx, target, texObj, pname, &param);
   }
}


void GLAPIENTRY
_mesa_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_GENERATE_MIPMAP_SGIS:
   case GL_TEXTURE_COMPARE_MODE_ARB:
   case GL_TEXTURE_COMPARE_FUNC_ARB:
   case GL_DEPTH_TEXTURE_MODE_ARB:
      {
         /* convert float param to int */
         GLint p[4];
         p[0] = (GLint) params[0];
         p[1] = p[2] = p[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, p);
      }
      break;

#ifdef FEATURE_OES_draw_texture
   case GL_TEXTURE_CROP_RECT_OES:
      {
         /* convert float params to int */
         GLint iparams[4];
         iparams[0] = (GLint) params[0];
         iparams[1] = (GLint) params[1];
         iparams[2] = (GLint) params[2];
         iparams[3] = (GLint) params[3];
         need_update = set_tex_parameteri(ctx, texObj, pname, iparams);
      }
      break;
#endif

   default:
      /* this will generate an error if pname is illegal */
      need_update = set_tex_parameterf(ctx, texObj, pname, params);
   }

   if (ctx->Driver.TexParameter && need_update) {
      ctx->Driver.TexParameter(ctx, target, texObj, pname, params);
   }
}


void GLAPIENTRY
_mesa_TexParameteri(GLenum target, GLenum pname, GLint param)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
   case GL_TEXTURE_PRIORITY:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_LOD_BIAS:
   case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
      {
         GLfloat fparam[4];
         fparam[0] = (GLfloat) param;
         fparam[1] = fparam[2] = fparam[3] = 0.0F;
         /* convert int param to float */
         need_update = set_tex_parameterf(ctx, texObj, pname, fparam);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      {
         GLint iparam[4];
         iparam[0] = param;
         iparam[1] = iparam[2] = iparam[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, iparam);
      }
   }

   if (ctx->Driver.TexParameter && need_update) {
      GLfloat fparam = (GLfloat) param;
      ctx->Driver.TexParameter(ctx, target, texObj, pname, &fparam);
   }
}


void GLAPIENTRY
_mesa_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      {
         /* convert int params to float */
         GLfloat fparams[4];
         fparams[0] = INT_TO_FLOAT(params[0]);
         fparams[1] = INT_TO_FLOAT(params[1]);
         fparams[2] = INT_TO_FLOAT(params[2]);
         fparams[3] = INT_TO_FLOAT(params[3]);
         need_update = set_tex_parameterf(ctx, texObj, pname, fparams);
      }
      break;
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
   case GL_TEXTURE_PRIORITY:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_LOD_BIAS:
   case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
      {
         /* convert int param to float */
         GLfloat fparams[4];
         fparams[0] = (GLfloat) params[0];
         fparams[1] = fparams[2] = fparams[3] = 0.0F;
         need_update = set_tex_parameterf(ctx, texObj, pname, fparams);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      need_update = set_tex_parameteri(ctx, texObj, pname, params);
   }

   if (ctx->Driver.TexParameter && need_update) {
      GLfloat fparams[4];
      fparams[0] = INT_TO_FLOAT(params[0]);
      if (pname == GL_TEXTURE_BORDER_COLOR ||
          pname == GL_TEXTURE_CROP_RECT_OES) {
         fparams[1] = INT_TO_FLOAT(params[1]);
         fparams[2] = INT_TO_FLOAT(params[2]);
         fparams[3] = INT_TO_FLOAT(params[3]);
      }
      ctx->Driver.TexParameter(ctx, target, texObj, pname, fparams);
   }
}


/**
 * Set tex parameter to integer value(s).  Primarily intended to set
 * integer-valued texture border color (for integer-valued textures).
 * New in GL 3.0.
 */
void GLAPIENTRY
_mesa_TexParameterIiv(GLenum target, GLenum pname, const GLint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      /* set the integer-valued border color */
      COPY_4V(texObj->BorderColor.i, params);
      break;
   default:
      _mesa_TexParameteriv(target, pname, params);
      break;
   }
   /* XXX no driver hook for TexParameterIiv() yet */
}


/**
 * Set tex parameter to unsigned integer value(s).  Primarily intended to set
 * uint-valued texture border color (for integer-valued textures).
 * New in GL 3.0
 */
void GLAPIENTRY
_mesa_TexParameterIuiv(GLenum target, GLenum pname, const GLuint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      /* set the unsigned integer-valued border color */
      COPY_4V(texObj->BorderColor.ui, params);
      break;
   default:
      _mesa_TexParameteriv(target, pname, (const GLint *) params);
      break;
   }
   /* XXX no driver hook for TexParameterIuiv() yet */
}




void GLAPIENTRY
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GLint iparam;
   _mesa_GetTexLevelParameteriv( target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}


void GLAPIENTRY
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   const struct gl_texture_image *img = NULL;
   GLboolean isProxy;
   GLint maxLevels;
   gl_format texFormat;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexLevelParameteriv(current unit)");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   /* this will catch bad target values */
   maxLevels = _mesa_max_texture_levels(ctx, target);
   if (maxLevels == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetTexLevelParameter[if]v(target=0x%x)", target);
      return;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   _mesa_lock_texture(ctx, texObj);

   img = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!img || !img->TexFormat) {
      /* undefined texture image */
      if (pname == GL_TEXTURE_COMPONENTS)
         *params = 1;
      else
         *params = 0;
      goto out;
   }

   texFormat = img->TexFormat;

   isProxy = _mesa_is_proxy_texture(target);

   switch (pname) {
      case GL_TEXTURE_WIDTH:
         *params = img->Width;
         break;
      case GL_TEXTURE_HEIGHT:
         *params = img->Height;
         break;
      case GL_TEXTURE_DEPTH:
         *params = img->Depth;
         break;
      case GL_TEXTURE_INTERNAL_FORMAT:
         if (_mesa_is_format_compressed(img->TexFormat)) {
            /* need to return the actual compressed format */
            *params = _mesa_compressed_format_to_glenum(ctx, img->TexFormat);
         }
         else {
            /* return the user's requested internal format */
            *params = img->InternalFormat;
         }
         break;
      case GL_TEXTURE_BORDER:
         *params = img->Border;
         break;
      case GL_TEXTURE_RED_SIZE:
      case GL_TEXTURE_GREEN_SIZE:
      case GL_TEXTURE_BLUE_SIZE:
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = _mesa_get_format_bits(texFormat, pname);
         else
            *params = 0;
         break;
      case GL_TEXTURE_ALPHA_SIZE:
         if (img->_BaseFormat == GL_ALPHA ||
             img->_BaseFormat == GL_LUMINANCE_ALPHA ||
             img->_BaseFormat == GL_RGBA)
            *params = _mesa_get_format_bits(texFormat, pname);
         else
            *params = 0;
         break;
      case GL_TEXTURE_INTENSITY_SIZE:
         if (img->_BaseFormat != GL_INTENSITY)
            *params = 0;
         else {
            *params = _mesa_get_format_bits(texFormat, pname);
            if (*params == 0) {
               /* intensity probably stored as rgb texture */
               *params = MIN2(_mesa_get_format_bits(texFormat, GL_TEXTURE_RED_SIZE),
                              _mesa_get_format_bits(texFormat, GL_TEXTURE_GREEN_SIZE));
            }
         }
         break;
      case GL_TEXTURE_LUMINANCE_SIZE:
         if (img->_BaseFormat != GL_LUMINANCE &&
             img->_BaseFormat != GL_LUMINANCE_ALPHA)
            *params = 0;
         else {
            *params = _mesa_get_format_bits(texFormat, pname);
            if (*params == 0) {
               /* luminance probably stored as rgb texture */
               *params = MIN2(_mesa_get_format_bits(texFormat, GL_TEXTURE_RED_SIZE),
                              _mesa_get_format_bits(texFormat, GL_TEXTURE_GREEN_SIZE));
            }
         }
         break;
      case GL_TEXTURE_INDEX_SIZE_EXT:
         if (img->_BaseFormat == GL_COLOR_INDEX)
            *params = _mesa_get_format_bits(texFormat, pname);
         else
            *params = 0;
         break;
      case GL_TEXTURE_DEPTH_SIZE_ARB:
         if (ctx->Extensions.ARB_depth_texture)
            *params = _mesa_get_format_bits(texFormat, pname);
         else
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         break;
      case GL_TEXTURE_STENCIL_SIZE_EXT:
         if (ctx->Extensions.EXT_packed_depth_stencil ||
             ctx->Extensions.ARB_framebuffer_object) {
            *params = _mesa_get_format_bits(texFormat, pname);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE:
	 if (_mesa_is_format_compressed(img->TexFormat) && !isProxy) {
            *params = _mesa_format_image_size(texFormat, img->Width,
                                              img->Height, img->Depth);
	 }
	 else {
	    _mesa_error(ctx, GL_INVALID_OPERATION,
			"glGetTexLevelParameter[if]v(pname)");
	 }
         break;
      case GL_TEXTURE_COMPRESSED:
         *params = (GLint) _mesa_is_format_compressed(img->TexFormat);
         break;

      /* GL_ARB_texture_float */
      case GL_TEXTURE_RED_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_RED_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_GREEN_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_GREEN_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_BLUE_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_BLUE_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_ALPHA_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_ALPHA_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_LUMINANCE_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_LUMINANCE_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_INTENSITY_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_INTENSITY_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_DEPTH_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = _mesa_get_format_bits(texFormat, GL_TEXTURE_DEPTH_SIZE) ?
               _mesa_get_format_datatype(texFormat) : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetTexLevelParameter[if]v(pname)");
   }

 out:
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   struct gl_texture_object *obj;
   GLboolean error = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = get_texobj(ctx, target, GL_TRUE);
   if (!obj)
      return;

   _mesa_lock_texture(ctx, obj);
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
      case GL_TEXTURE_WRAP_R:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         break;
      case GL_TEXTURE_BORDER_COLOR:
         params[0] = CLAMP(obj->BorderColor.f[0], 0.0F, 1.0F);
         params[1] = CLAMP(obj->BorderColor.f[1], 0.0F, 1.0F);
         params[2] = CLAMP(obj->BorderColor.f[2], 0.0F, 1.0F);
         params[3] = CLAMP(obj->BorderColor.f[3], 0.0F, 1.0F);
         break;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = ENUM_TO_FLOAT(resident);
         }
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
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = obj->MaxAnisotropy;
         }
	 else
	    error = GL_TRUE;
         break;
      case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
         if (ctx->Extensions.ARB_shadow_ambient) {
            *params = obj->CompareFailValue;
         }
	 else 
	    error = GL_TRUE;
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLfloat) obj->GenerateMipmap;
         }
	 else 
	    error = GL_TRUE;
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareMode;
         }
	 else 
	    error = GL_TRUE;
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareFunc;
         }
	 else 
	    error = GL_TRUE;
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLfloat) obj->DepthMode;
         }
	 else 
	    error = GL_TRUE;
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = obj->LodBias;
         }
	 else 
	    error = GL_TRUE;
         break;
#ifdef FEATURE_OES_draw_texture
      case GL_TEXTURE_CROP_RECT_OES:
         params[0] = obj->CropRect[0];
         params[1] = obj->CropRect[1];
         params[2] = obj->CropRect[2];
         params[3] = obj->CropRect[3];
         break;
#endif

      case GL_TEXTURE_SWIZZLE_R_EXT:
      case GL_TEXTURE_SWIZZLE_G_EXT:
      case GL_TEXTURE_SWIZZLE_B_EXT:
      case GL_TEXTURE_SWIZZLE_A_EXT:
         if (ctx->Extensions.EXT_texture_swizzle) {
            GLuint comp = pname - GL_TEXTURE_SWIZZLE_R_EXT;
            *params = (GLfloat) obj->Swizzle[comp];
         }
         else {
            error = GL_TRUE;
         }
         break;

      case GL_TEXTURE_SWIZZLE_RGBA_EXT:
         if (ctx->Extensions.EXT_texture_swizzle) {
            GLuint comp;
            for (comp = 0; comp < 4; comp++) {
               params[comp] = (GLfloat) obj->Swizzle[comp];
            }
         }
         else {
            error = GL_TRUE;
         }
         break;

      default:
	 error = GL_TRUE;
	 break;
   }

   if (error)
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname=0x%x)",
		  pname);

   _mesa_unlock_texture(ctx, obj);
}


void GLAPIENTRY
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   struct gl_texture_object *obj;
   GLboolean error = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

    obj = get_texobj(ctx, target, GL_TRUE);
    if (!obj)
       return;

   _mesa_lock_texture(ctx, obj);
   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
         break;;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         break;;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         break;;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         break;;
      case GL_TEXTURE_WRAP_R:
         *params = (GLint) obj->WrapR;
         break;;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat b[4];
            b[0] = CLAMP(obj->BorderColor.f[0], 0.0F, 1.0F);
            b[1] = CLAMP(obj->BorderColor.f[1], 0.0F, 1.0F);
            b[2] = CLAMP(obj->BorderColor.f[2], 0.0F, 1.0F);
            b[3] = CLAMP(obj->BorderColor.f[3], 0.0F, 1.0F);
            params[0] = FLOAT_TO_INT(b[0]);
            params[1] = FLOAT_TO_INT(b[1]);
            params[2] = FLOAT_TO_INT(b[2]);
            params[3] = FLOAT_TO_INT(b[3]);
         }
         break;;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = (GLint) resident;
         }
         break;;
      case GL_TEXTURE_PRIORITY:
         *params = FLOAT_TO_INT(obj->Priority);
         break;;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         break;;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         break;;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         break;;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         break;;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = (GLint) obj->MaxAnisotropy;
         }
         else {
            error = GL_TRUE;
         }
         break;
      case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
         if (ctx->Extensions.ARB_shadow_ambient) {
            *params = (GLint) FLOAT_TO_INT(obj->CompareFailValue);
         }
         else {
            error = GL_TRUE;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLint) obj->GenerateMipmap;
         }
         else {
            error = GL_TRUE;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareMode;
         }
         else {
            error = GL_TRUE;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareFunc;
         }
         else {
            error = GL_TRUE;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLint) obj->DepthMode;
         }
         else {
            error = GL_TRUE;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = (GLint) obj->LodBias;
         }
         else {
            error = GL_TRUE;
         }
         break;
#ifdef FEATURE_OES_draw_texture
      case GL_TEXTURE_CROP_RECT_OES:
         params[0] = obj->CropRect[0];
         params[1] = obj->CropRect[1];
         params[2] = obj->CropRect[2];
         params[3] = obj->CropRect[3];
         break;
#endif
      case GL_TEXTURE_SWIZZLE_R_EXT:
      case GL_TEXTURE_SWIZZLE_G_EXT:
      case GL_TEXTURE_SWIZZLE_B_EXT:
      case GL_TEXTURE_SWIZZLE_A_EXT:
         if (ctx->Extensions.EXT_texture_swizzle) {
            GLuint comp = pname - GL_TEXTURE_SWIZZLE_R_EXT;
            *params = obj->Swizzle[comp];
         }
         else {
            error = GL_TRUE;
         }
         break;

      case GL_TEXTURE_SWIZZLE_RGBA_EXT:
         if (ctx->Extensions.EXT_texture_swizzle) {
            COPY_4V(params, obj->Swizzle);
         }
         else {
            error = GL_TRUE;
         }
         break;

      default:
         ; /* silence warnings */
   }

   if (error)
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname=0x%x)",
		  pname);

   _mesa_unlock_texture(ctx, obj);
}


/** New in GL 3.0 */
void GLAPIENTRY
_mesa_GetTexParameterIiv(GLenum target, GLenum pname, GLint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_TRUE);
   
   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      COPY_4V(params, texObj->BorderColor.i);
      break;
   default:
      _mesa_GetTexParameteriv(target, pname, params);
   }
}


/** New in GL 3.0 */
void GLAPIENTRY
_mesa_GetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_TRUE);
   
   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      COPY_4V(params, texObj->BorderColor.i);
      break;
   default:
      {
         GLint ip[4];
         _mesa_GetTexParameteriv(target, pname, ip);
         params[0] = ip[0];
         if (pname == GL_TEXTURE_SWIZZLE_RGBA_EXT || 
             pname == GL_TEXTURE_CROP_RECT_OES) {
            params[1] = ip[1];
            params[2] = ip[2];
            params[3] = ip[3];
         }
      }
   }
}
