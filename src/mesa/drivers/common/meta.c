/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
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
 * Meta operations.  Some GL operations can be expressed in terms of
 * other GL operations.  For example, glBlitFramebuffer() can be done
 * with texture mapping and glClear() can be done with polygon rendering.
 *
 * \author Brian Paul
 */


#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/arrayobj.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/depth.h"
#include "main/enable.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/matrix.h"
#include "main/polygon.h"
#include "main/scissor.h"
#include "main/shaders.h"
#include "main/stencil.h"
#include "main/texobj.h"
#include "main/texenv.h"
#include "main/teximage.h"
#include "main/texparam.h"
#include "main/texstate.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "shader/program.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"


/**
 * State which we may save/restore across meta ops.
 * XXX this may be incomplete...
 */
struct save_state
{
   GLbitfield SavedState;  /**< bitmask of META_* flags */

   /** META_ALPHA_TEST */
   GLboolean AlphaEnabled;

   /** META_BLEND */
   GLboolean BlendEnabled;
   GLboolean ColorLogicOpEnabled;

   /** META_COLOR_MASK */
   GLubyte ColorMask[4];

   /** META_DEPTH_TEST */
   struct gl_depthbuffer_attrib Depth;

   /** META_FOG */
   GLboolean Fog;

   /** META_PIXELSTORE */
   /* XXX / TO-DO */

   /** META_RASTERIZATION */
   GLenum FrontPolygonMode, BackPolygonMode;
   GLboolean PolygonOffset;
   GLboolean PolygonSmooth;
   GLboolean PolygonStipple;
   GLboolean PolygonCull;

   /** META_SCISSOR */
   struct gl_scissor_attrib Scissor;

   /** META_SHADER */
   GLboolean VertexProgramEnabled;
   struct gl_vertex_program *VertexProgram;
   GLboolean FragmentProgramEnabled;
   struct gl_fragment_program *FragmentProgram;
   GLuint Shader;

   /** META_STENCIL_TEST */
   struct gl_stencil_attrib Stencil;

   /** META_TRANSFORM */
   GLenum MatrixMode;
   GLfloat ModelviewMatrix[16];
   GLfloat ProjectionMatrix[16];
   GLfloat TextureMatrix[16];
   GLbitfield ClipPlanesEnabled;

   /** META_TEXTURE */
   GLuint ActiveUnit;
   GLuint ClientActiveUnit;
   /** for unit[0] only */
   struct gl_texture_object *CurrentTexture[NUM_TEXTURE_TARGETS];
   /** mask of TEXTURE_2D_BIT, etc */
   GLbitfield TexEnabled[MAX_TEXTURE_UNITS];
   GLbitfield TexGenEnabled[MAX_TEXTURE_UNITS];
   GLuint EnvMode;  /* unit[0] only */

   /** META_VERTEX */
   struct gl_array_object *ArrayObj;
   struct gl_buffer_object *ArrayBufferObj;

   /** META_VIEWPORT */
   GLint ViewportX, ViewportY, ViewportW, ViewportH;
   GLclampd DepthNear, DepthFar;

   /** Miscellaneous (always disabled) */
   GLboolean Lighting;
};


/**
 * State for glBlitFramebufer()
 */
struct blit_state
{
   GLuint TexObj;
   GLsizei TexWidth, TexHeight;
   GLenum TexType;
   GLuint ArrayObj;
   GLuint VBO;
   GLfloat verts[4][4]; /** four verts of X,Y,S,T */
};


/**
 * State for glClear()
 */
struct clear_state
{
   GLuint ArrayObj;
   GLuint VBO;
   GLfloat verts[4][7]; /** four verts of X,Y,Z,R,G,B,A */
};


/**
 * State for glCopyPixels()
 */
struct copypix_state
{
   GLuint TexObj;
   GLsizei TexWidth, TexHeight;
   GLenum TexType;
   GLuint ArrayObj;
   GLuint VBO;
   GLfloat verts[4][5]; /** four verts of X,Y,Z,S,T */
};


/**
 * State for glDrawPixels()
 */
struct drawpix_state
{
   GLuint TexObj;
   GLsizei TexWidth, TexHeight;
   GLenum TexIntFormat;
   GLuint ArrayObj;
   GLuint VBO;
   GLfloat verts[4][5]; /** four verts of X,Y,Z,S,T */
};



/**
 * All per-context meta state.
 */
struct gl_meta_state
{
   struct save_state Save;    /**< state saved during meta-ops */

   struct blit_state Blit;    /**< For _mesa_meta_blit_framebuffer() */
   struct clear_state Clear;  /**< For _mesa_meta_clear() */
   struct copypix_state CopyPix;  /**< For _mesa_meta_copy_pixels() */
   struct drawpix_state DrawPix;  /**< For _mesa_meta_draw_pixels() */

   /* other possible meta-ops:
    * glDrawPixels()
    * glBitmap()
    * glGenerateMipmap()
    */
};


/**
 * Initialize meta-ops for a context.
 * To be called once during context creation.
 */
void
_mesa_meta_init(GLcontext *ctx)
{
   ASSERT(!ctx->Meta);

   ctx->Meta = CALLOC_STRUCT(gl_meta_state);
}


/**
 * Free context meta-op state.
 * To be called once during context destruction.
 */
void
_mesa_meta_free(GLcontext *ctx)
{
   struct gl_meta_state *meta = ctx->Meta;

   if (meta->Blit.TexObj) {
      _mesa_DeleteTextures(1, &meta->Blit.TexObj);
      _mesa_DeleteBuffersARB(1, & meta->Blit.VBO);
      _mesa_DeleteVertexArraysAPPLE(1, &meta->Blit.ArrayObj);
   }

   if (meta->Clear.VBO) {
      _mesa_DeleteBuffersARB(1, & meta->Clear.VBO);
      _mesa_DeleteVertexArraysAPPLE(1, &meta->Clear.ArrayObj);
   }

   if (meta->CopyPix.TexObj) {
      _mesa_DeleteTextures(1, &meta->CopyPix.TexObj);
      _mesa_DeleteBuffersARB(1, & meta->CopyPix.VBO);
      _mesa_DeleteVertexArraysAPPLE(1, &meta->CopyPix.ArrayObj);
   }

   if (meta->DrawPix.TexObj) {
      _mesa_DeleteTextures(1, &meta->DrawPix.TexObj);
      _mesa_DeleteBuffersARB(1, & meta->DrawPix.VBO);
      _mesa_DeleteVertexArraysAPPLE(1, &meta->DrawPix.ArrayObj);
   }

   _mesa_free(ctx->Meta);
   ctx->Meta = NULL;
}


/**
 * Enter meta state.  This is like a light-weight version of glPushAttrib
 * but it also resets most GL state back to default values.
 *
 * \param state  bitmask of META_* flags indicating which attribute groups
 *               to save and reset to their defaults
 */
static void
_mesa_meta_begin(GLcontext *ctx, GLbitfield state)
{
   struct save_state *save = &ctx->Meta->Save;

   save->SavedState = state;

   if (state & META_ALPHA_TEST) {
      save->AlphaEnabled = ctx->Color.AlphaEnabled;
      if (ctx->Color.AlphaEnabled)
         _mesa_Disable(GL_ALPHA_TEST);
   }

   if (state & META_BLEND) {
      save->BlendEnabled = ctx->Color.BlendEnabled;
      if (ctx->Color.BlendEnabled)
         _mesa_Disable(GL_BLEND);
      save->ColorLogicOpEnabled = ctx->Color.ColorLogicOpEnabled;
      if (ctx->Color.ColorLogicOpEnabled)
         _mesa_Disable(GL_COLOR_LOGIC_OP);
   }

   if (state & META_COLOR_MASK) {
      COPY_4V(save->ColorMask, ctx->Color.ColorMask);
      if (!ctx->Color.ColorMask[0] ||
          !ctx->Color.ColorMask[1] ||
          !ctx->Color.ColorMask[2] ||
          !ctx->Color.ColorMask[3])
         _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   }

   if (state & META_DEPTH_TEST) {
      save->Depth = ctx->Depth; /* struct copy */
      if (ctx->Depth.Test)
         _mesa_Disable(GL_DEPTH_TEST);
   }

   if (state & META_FOG) {
      save->Fog = ctx->Fog.Enabled;
      if (ctx->Fog.Enabled)
         _mesa_set_enable(ctx, GL_FOG, GL_FALSE);
   }

   if (state & META_RASTERIZATION) {
      save->FrontPolygonMode = ctx->Polygon.FrontMode;
      save->BackPolygonMode = ctx->Polygon.BackMode;
      save->PolygonOffset = ctx->Polygon.OffsetFill;
      save->PolygonSmooth = ctx->Polygon.SmoothFlag;
      save->PolygonStipple = ctx->Polygon.StippleFlag;
      save->PolygonCull = ctx->Polygon.CullFlag;
      _mesa_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      _mesa_set_enable(ctx, GL_POLYGON_OFFSET_FILL, GL_FALSE);
      _mesa_set_enable(ctx, GL_POLYGON_SMOOTH, GL_FALSE);
      _mesa_set_enable(ctx, GL_POLYGON_STIPPLE, GL_FALSE);
      _mesa_set_enable(ctx, GL_CULL_FACE, GL_FALSE);
   }

   if (state & META_SCISSOR) {
      save->Scissor = ctx->Scissor; /* struct copy */
   }

   if (state & META_SHADER) {
      if (ctx->Extensions.ARB_vertex_program) {
         save->VertexProgramEnabled = ctx->VertexProgram.Enabled;
         save->VertexProgram = ctx->VertexProgram.Current;
         _mesa_set_enable(ctx, GL_VERTEX_PROGRAM_ARB, GL_FALSE);
      }

      if (ctx->Extensions.ARB_fragment_program) {
         save->FragmentProgramEnabled = ctx->FragmentProgram.Enabled;
         save->FragmentProgram = ctx->FragmentProgram.Current;
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_FALSE);
      }

      if (ctx->Extensions.ARB_shader_objects) {
         save->Shader = ctx->Shader.CurrentProgram ?
            ctx->Shader.CurrentProgram->Name : 0;
         _mesa_UseProgramObjectARB(0);
      }
   }

   if (state & META_STENCIL_TEST) {
      save->Stencil = ctx->Stencil; /* struct copy */
      if (ctx->Stencil.Enabled)
         _mesa_Disable(GL_STENCIL_TEST);
      /* NOTE: other stencil state not reset */
   }

   if (state & META_TEXTURE) {
      GLuint u, tgt;

      save->ActiveUnit = ctx->Texture.CurrentUnit;
      save->ClientActiveUnit = ctx->Array.ActiveTexture;
      save->EnvMode = ctx->Texture.Unit[0].EnvMode;

      /* Disable all texture units */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         save->TexEnabled[u] = ctx->Texture.Unit[u].Enabled;
         save->TexGenEnabled[u] = ctx->Texture.Unit[u].TexGenEnabled;
         if (ctx->Texture.Unit[u].Enabled ||
             ctx->Texture.Unit[u].TexGenEnabled) {
            _mesa_ActiveTextureARB(GL_TEXTURE0 + u);
            _mesa_set_enable(ctx, GL_TEXTURE_1D, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_2D, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_3D, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_CUBE_MAP, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_RECTANGLE, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_S, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_T, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_R, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_Q, GL_FALSE);
         }
      }

      /* save current texture objects for unit[0] only */
      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
         _mesa_reference_texobj(&save->CurrentTexture[tgt],
                                ctx->Texture.Unit[0].CurrentTex[tgt]);
      }

      /* set defaults for unit[0] */
      _mesa_ActiveTextureARB(GL_TEXTURE0);
      _mesa_ClientActiveTextureARB(GL_TEXTURE0);
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   }

   if (state & META_TRANSFORM) {
      GLuint activeTexture = ctx->Texture.CurrentUnit;
      _mesa_memcpy(save->ModelviewMatrix, ctx->ModelviewMatrixStack.Top->m,
                   16 * sizeof(GLfloat));
      _mesa_memcpy(save->ProjectionMatrix, ctx->ProjectionMatrixStack.Top->m,
                   16 * sizeof(GLfloat));
      _mesa_memcpy(save->TextureMatrix, ctx->TextureMatrixStack[0].Top->m,
                   16 * sizeof(GLfloat));
      save->MatrixMode = ctx->Transform.MatrixMode;
      /* set 1:1 vertex:pixel coordinate transform */
      _mesa_ActiveTextureARB(GL_TEXTURE0);
      _mesa_MatrixMode(GL_TEXTURE);
      _mesa_LoadIdentity();
      _mesa_ActiveTextureARB(GL_TEXTURE0 + activeTexture);
      _mesa_MatrixMode(GL_MODELVIEW);
      _mesa_LoadIdentity();
      _mesa_MatrixMode(GL_PROJECTION);
      _mesa_LoadIdentity();
      _mesa_Ortho(0.0F, ctx->DrawBuffer->Width,
                  0.0F, ctx->DrawBuffer->Height,
                  -1.0F, 1.0F);
      save->ClipPlanesEnabled = ctx->Transform.ClipPlanesEnabled;
      if (ctx->Transform.ClipPlanesEnabled) {
         GLuint i;
         for (i = 0; i < ctx->Const.MaxClipPlanes; i++) {
            _mesa_set_enable(ctx, GL_CLIP_PLANE0 + i, GL_FALSE);
         }
      }
   }

   if (state & META_VERTEX) {
      /* save vertex array object state */
      _mesa_reference_array_object(ctx, &save->ArrayObj,
                                   ctx->Array.ArrayObj);
      _mesa_reference_buffer_object(ctx, &save->ArrayBufferObj,
                                    ctx->Array.ArrayBufferObj);
      /* set some default state? */
   }

   if (state & META_VIEWPORT) {
      save->ViewportX = ctx->Viewport.X;
      save->ViewportY = ctx->Viewport.Y;
      save->ViewportW = ctx->Viewport.Width;
      save->ViewportH = ctx->Viewport.Height;
      _mesa_Viewport(0, 0, ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);
      save->DepthNear = ctx->Viewport.Near;
      save->DepthFar = ctx->Viewport.Far;
      _mesa_DepthRange(0.0, 1.0);
   }

   /* misc */
   {
      save->Lighting = ctx->Light.Enabled;
      if (ctx->Light.Enabled)
         _mesa_set_enable(ctx, GL_LIGHTING, GL_FALSE);
   }
}


/**
 * Leave meta state.  This is like a light-weight version of glPopAttrib().
 */
static void
_mesa_meta_end(GLcontext *ctx)
{
   struct save_state *save = &ctx->Meta->Save;
   const GLbitfield state = save->SavedState;

   if (state & META_ALPHA_TEST) {
      if (ctx->Color.AlphaEnabled != save->AlphaEnabled)
         _mesa_set_enable(ctx, GL_ALPHA_TEST, save->AlphaEnabled);
   }

   if (state & META_BLEND) {
      if (ctx->Color.BlendEnabled != save->BlendEnabled)
         _mesa_set_enable(ctx, GL_BLEND, save->BlendEnabled);
      if (ctx->Color.ColorLogicOpEnabled != save->ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, save->ColorLogicOpEnabled);
   }

   if (state & META_COLOR_MASK) {
      if (!TEST_EQ_4V(ctx->Color.ColorMask, save->ColorMask))
         _mesa_ColorMask(save->ColorMask[0], save->ColorMask[1],
                         save->ColorMask[2], save->ColorMask[3]);
   }

   if (state & META_DEPTH_TEST) {
      if (ctx->Depth.Test != save->Depth.Test)
         _mesa_set_enable(ctx, GL_DEPTH_TEST, save->Depth.Test);
      _mesa_DepthFunc(save->Depth.Func);
      _mesa_DepthMask(save->Depth.Mask);
   }

   if (state & META_FOG) {
      _mesa_set_enable(ctx, GL_FOG, save->Fog);
   }

   if (state & META_RASTERIZATION) {
      _mesa_PolygonMode(GL_FRONT, save->FrontPolygonMode);
      _mesa_PolygonMode(GL_BACK, save->BackPolygonMode);
      _mesa_set_enable(ctx, GL_POLYGON_STIPPLE, save->PolygonStipple);
      _mesa_set_enable(ctx, GL_POLYGON_OFFSET_FILL, save->PolygonOffset);
      _mesa_set_enable(ctx, GL_POLYGON_SMOOTH, save->PolygonSmooth);
      _mesa_set_enable(ctx, GL_CULL_FACE, save->PolygonCull);
   }

   if (state & META_SCISSOR) {
      _mesa_set_enable(ctx, GL_SCISSOR_TEST, save->Scissor.Enabled);
      _mesa_Scissor(save->Scissor.X, save->Scissor.Y,
                    save->Scissor.Width, save->Scissor.Height);
   }

   if (state & META_SHADER) {
      if (ctx->Extensions.ARB_vertex_program) {
         _mesa_set_enable(ctx, GL_VERTEX_PROGRAM_ARB,
                          save->VertexProgramEnabled);
         _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current, 
                                  save->VertexProgram);
      }

      if (ctx->Extensions.ARB_fragment_program) {
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB,
                          save->FragmentProgramEnabled);
         _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
                                  save->FragmentProgram);
      }

      if (ctx->Extensions.ARB_shader_objects) {
         _mesa_UseProgramObjectARB(save->Shader);
      }
   }

   if (state & META_STENCIL_TEST) {
      const struct gl_stencil_attrib *stencil = &save->Stencil;

      _mesa_set_enable(ctx, GL_STENCIL_TEST, stencil->Enabled);
      _mesa_ClearStencil(stencil->Clear);
      if (ctx->Extensions.EXT_stencil_two_side) {
         _mesa_set_enable(ctx, GL_STENCIL_TEST_TWO_SIDE_EXT,
                          stencil->TestTwoSide);
         _mesa_ActiveStencilFaceEXT(stencil->ActiveFace
                                    ? GL_BACK : GL_FRONT);
      }
      /* front state */
      _mesa_StencilFuncSeparate(GL_FRONT,
                                stencil->Function[0],
                                stencil->Ref[0],
                                stencil->ValueMask[0]);
      _mesa_StencilMaskSeparate(GL_FRONT, stencil->WriteMask[0]);
      _mesa_StencilOpSeparate(GL_FRONT, stencil->FailFunc[0],
                              stencil->ZFailFunc[0],
                              stencil->ZPassFunc[0]);
      /* back state */
      _mesa_StencilFuncSeparate(GL_BACK,
                                stencil->Function[1],
                                stencil->Ref[1],
                                stencil->ValueMask[1]);
      _mesa_StencilMaskSeparate(GL_BACK, stencil->WriteMask[1]);
      _mesa_StencilOpSeparate(GL_BACK, stencil->FailFunc[1],
                              stencil->ZFailFunc[1],
                              stencil->ZPassFunc[1]);
   }

   if (state & META_TEXTURE) {
      GLuint u, tgt;

      ASSERT(ctx->Texture.CurrentUnit == 0);

      /* restore texenv for unit[0] */
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, save->EnvMode);

      /* restore texture objects for unit[0] only */
      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
         _mesa_reference_texobj(&ctx->Texture.Unit[0].CurrentTex[tgt],
                                save->CurrentTexture[tgt]);
      }

      /* Re-enable textures, texgen */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         if (save->TexEnabled[u]) {
            _mesa_ActiveTextureARB(GL_TEXTURE0 + u);

            if (save->TexEnabled[u] & TEXTURE_1D_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_1D, GL_TRUE);
            if (save->TexEnabled[u] & TEXTURE_2D_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_2D, GL_TRUE);
            if (save->TexEnabled[u] & TEXTURE_3D_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_3D, GL_TRUE);
            if (save->TexEnabled[u] & TEXTURE_CUBE_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_CUBE_MAP, GL_TRUE);
            if (save->TexEnabled[u] & TEXTURE_RECT_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_RECTANGLE, GL_TRUE);
         }

         if (save->TexGenEnabled[u]) {
            _mesa_ActiveTextureARB(GL_TEXTURE0 + u);

            if (save->TexGenEnabled[u] & S_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_GEN_S, GL_TRUE);
            if (save->TexGenEnabled[u] & T_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_GEN_T, GL_TRUE);
            if (save->TexGenEnabled[u] & R_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_GEN_R, GL_TRUE);
            if (save->TexGenEnabled[u] & Q_BIT)
               _mesa_set_enable(ctx, GL_TEXTURE_GEN_Q, GL_TRUE);
         }
      }

      /* restore current unit state */
      _mesa_ActiveTextureARB(GL_TEXTURE0 + save->ActiveUnit);
      _mesa_ClientActiveTextureARB(GL_TEXTURE0 + save->ClientActiveUnit);
   }

   if (state & META_TRANSFORM) {
      GLuint activeTexture = ctx->Texture.CurrentUnit;
      _mesa_ActiveTextureARB(GL_TEXTURE0);
      _mesa_MatrixMode(GL_TEXTURE);
      _mesa_LoadMatrixf(save->TextureMatrix);
      _mesa_ActiveTextureARB(GL_TEXTURE0 + activeTexture);

      _mesa_MatrixMode(GL_MODELVIEW);
      _mesa_LoadMatrixf(save->ModelviewMatrix);

      _mesa_MatrixMode(GL_PROJECTION);
      _mesa_LoadMatrixf(save->ProjectionMatrix);

      _mesa_MatrixMode(save->MatrixMode);

      save->ClipPlanesEnabled = ctx->Transform.ClipPlanesEnabled;
      if (save->ClipPlanesEnabled) {
         GLuint i;
         for (i = 0; i < ctx->Const.MaxClipPlanes; i++) {
            if (save->ClipPlanesEnabled & (1 << i)) {
               _mesa_set_enable(ctx, GL_CLIP_PLANE0 + i, GL_TRUE);
            }
         }
      }
   }

   if (state & META_VERTEX) {
      /* restore vertex buffer object */
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, save->ArrayBufferObj->Name);
      _mesa_reference_buffer_object(ctx, &save->ArrayBufferObj, NULL);

      /* restore vertex array object */
      _mesa_BindVertexArray(save->ArrayObj->Name);
      _mesa_reference_array_object(ctx, &save->ArrayObj, NULL);
   }

   if (state & META_VIEWPORT) {
      _mesa_Viewport(save->ViewportX, save->ViewportY,
                     save->ViewportW, save->ViewportH);
      _mesa_DepthRange(save->DepthNear, save->DepthFar);
   }

   /* misc */
   if (save->Lighting) {
      _mesa_set_enable(ctx, GL_LIGHTING, GL_TRUE);
   }
   if (save->Fog) {
      _mesa_set_enable(ctx, GL_FOG, GL_TRUE);
   }
}


/**
 * Meta implementation of ctx->Driver.BlitFramebuffer() in terms
 * of texture mapping and polygon rendering.
 * Note: this function requires GL_ARB_texture_rectangle support.
 */
void
_mesa_meta_blit_framebuffer(GLcontext *ctx,
                            GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter)
{
   struct blit_state *blit = &ctx->Meta->Blit;
   const GLint srcX = MIN2(srcX0, srcX1);
   const GLint srcY = MIN2(srcY0, srcY1);
   const GLint srcW = abs(srcX1 - srcX0);
   const GLint srcH = abs(srcY1 - srcY0);
   GLboolean srcFlipX = srcX1 < srcX0;
   GLboolean srcFlipY = srcY1 < srcY0;

   ASSERT(ctx->Extensions.NV_texture_rectangle);

   if (srcW > ctx->Const.MaxTextureRectSize ||
       srcH > ctx->Const.MaxTextureRectSize) {
      /* XXX avoid this fallback */
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
      return;
   }


   if (srcFlipX) {
      GLint tmp = dstX0;
      dstX0 = dstX1;
      dstX1 = tmp;
   }

   if (srcFlipY) {
      GLint tmp = dstY0;
      dstY0 = dstY1;
      dstY1 = tmp;
   }

   /* only scissor effects blit so save/clear all other relevant state */
   _mesa_meta_begin(ctx, ~META_SCISSOR);

   if (blit->TexObj == 0) {
      /* one-time setup */

      /* create texture object */
      _mesa_GenTextures(1, &blit->TexObj);
      _mesa_BindTexture(GL_TEXTURE_RECTANGLE, blit->TexObj);
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   }
   else {
      _mesa_BindTexture(GL_TEXTURE_RECTANGLE, blit->TexObj);
   }

   _mesa_TexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, filter);
   _mesa_TexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, filter);

   if (blit->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &blit->ArrayObj);
      _mesa_BindVertexArray(blit->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &blit->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, blit->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(blit->verts),
                          blit->verts, GL_STREAM_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(2, GL_FLOAT, 4 * sizeof(GLfloat),
                          (void*) (0 * sizeof(GLfloat)));
      _mesa_TexCoordPointer(2, GL_FLOAT, 4 * sizeof(GLfloat),
                            (void *) (2 * sizeof(GLfloat)));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(blit->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, blit->VBO);
   }

   /* vertex positions */
   blit->verts[0][0] = (GLfloat) dstX0;
   blit->verts[0][1] = (GLfloat) dstY0;
   blit->verts[1][0] = (GLfloat) dstX1;
   blit->verts[1][1] = (GLfloat) dstY0;
   blit->verts[2][0] = (GLfloat) dstX1;
   blit->verts[2][1] = (GLfloat) dstY1;
   blit->verts[3][0] = (GLfloat) dstX0;
   blit->verts[3][1] = (GLfloat) dstY1;

   /* texcoords */
   blit->verts[0][2] = 0.0F;
   blit->verts[0][3] = 0.0F;
   blit->verts[1][2] = (GLfloat) srcW;
   blit->verts[1][3] = 0.0F;
   blit->verts[2][2] = (GLfloat) srcW;
   blit->verts[2][3] = (GLfloat) srcH;
   blit->verts[3][2] = 0.0F;
   blit->verts[3][3] = (GLfloat) srcH;

   /* upload new vertex data */
   _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0,
                          sizeof(blit->verts), blit->verts);

   /* copy framebuffer image to texture */
   if (mask & GL_COLOR_BUFFER_BIT) {
      if (blit->TexWidth == srcW &&
          blit->TexHeight == srcH &&
          blit->TexType == GL_RGBA) {
         /* replace existing tex image */
         _mesa_CopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0,
                                 0, 0, srcX, srcY, srcW, srcH);
      }
      else {
         /* create new tex image */
         _mesa_CopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA,
                              srcX, srcY, srcW, srcH, 0);
         blit->TexWidth = srcW;
         blit->TexHeight = srcH;
         blit->TexType = GL_RGBA;
      }

      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   _mesa_Enable(GL_TEXTURE_RECTANGLE);

   /* draw textured quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_Disable(GL_TEXTURE_RECTANGLE);

   _mesa_meta_end(ctx);

   /* XXX, TO-DO: try to handle these cases above! */
   if (mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
   }
}


/**
 * Meta implementation of ctx->Driver.Clear() in terms of polygon rendering.
 */
void
_mesa_meta_clear(GLcontext *ctx, GLbitfield buffers)
{
   struct clear_state *clear = &ctx->Meta->Clear;
   GLfloat z = 1.0 - 2.0 * ctx->Depth.Clear;
   GLuint i;

   /* only scissor and color mask effects clearing */
   _mesa_meta_begin(ctx, ~(META_SCISSOR | META_COLOR_MASK));

   if (clear->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &clear->ArrayObj);
      _mesa_BindVertexArray(clear->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &clear->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(clear->verts),
                          clear->verts, GL_STREAM_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, 7 * sizeof(GLfloat), (void *) 0);
      _mesa_ColorPointer(4, GL_FLOAT, 7 * sizeof(GLfloat),
                         (void *) (3 * sizeof(GLfloat)));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_COLOR_ARRAY);
   }
   else {
      _mesa_BindVertexArray(clear->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);
   }

   /* GL_COLOR_BUFFER_BIT */
   if (buffers & BUFFER_BITS_COLOR) {
      /* leave colormask, glDrawBuffer state as-is */
   }
   else {
      _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   }

   /* GL_DEPTH_BUFFER_BIT */
   if (buffers & BUFFER_BIT_DEPTH) {
      _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
      _mesa_DepthFunc(GL_ALWAYS);
      _mesa_DepthMask(GL_TRUE);
   }
   else {
      assert(!ctx->Depth.Test);
   }

   /* GL_STENCIL_BUFFER_BIT */
   if (buffers & BUFFER_BIT_STENCIL) {
      _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_TRUE);
      _mesa_StencilOpSeparate(GL_FRONT_AND_BACK,
                              GL_REPLACE, GL_REPLACE, GL_REPLACE);
      _mesa_StencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS,
                                ctx->Stencil.Clear & 0x7fffffff,
                                ctx->Stencil.WriteMask[0]);
   }
   else {
      assert(!ctx->Stencil.Enabled);
   }

   /* vertex positions */
   clear->verts[0][0] = (GLfloat) ctx->DrawBuffer->_Xmin;
   clear->verts[0][1] = (GLfloat) ctx->DrawBuffer->_Ymin;
   clear->verts[0][2] = z;
   clear->verts[1][0] = (GLfloat) ctx->DrawBuffer->_Xmax;
   clear->verts[1][1] = (GLfloat) ctx->DrawBuffer->_Ymin;
   clear->verts[1][2] = z;
   clear->verts[2][0] = (GLfloat) ctx->DrawBuffer->_Xmax;
   clear->verts[2][1] = (GLfloat) ctx->DrawBuffer->_Ymax;
   clear->verts[2][2] = z;
   clear->verts[3][0] = (GLfloat) ctx->DrawBuffer->_Xmin;
   clear->verts[3][1] = (GLfloat) ctx->DrawBuffer->_Ymax;
   clear->verts[3][2] = z;

   /* vertex colors */
   for (i = 0; i < 4; i++) {
      COPY_4FV(&clear->verts[i][3], ctx->Color.ClearColor);
   }

   /* upload new vertex data */
   _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0,
                          sizeof(clear->verts), clear->verts);

   /* draw quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_meta_end(ctx);
}


/**
 * Meta implementation of ctx->Driver.CopyPixels() in terms
 * of texture mapping and polygon rendering.
 * Note: this function requires GL_ARB_texture_rectangle support.
 */
void
_mesa_meta_copy_pixels(GLcontext *ctx, GLint srcX, GLint srcY,
                       GLsizei width, GLsizei height,
                       GLint dstX, GLint dstY, GLenum type)
{
   const GLenum filter = GL_NEAREST;
   struct copypix_state *copypix = &ctx->Meta->CopyPix;
   const GLfloat z = ctx->Current.RasterPos[2];
   const GLfloat dstX1 = dstX + width * ctx->Pixel.ZoomX;
   const GLfloat dstY1 = dstY + height * ctx->Pixel.ZoomY;

   ASSERT(ctx->Extensions.NV_texture_rectangle);

   if (type != GL_COLOR ||
       ctx->_ImageTransferState ||
       ctx->Fog.Enabled ||
       width > ctx->Const.MaxTextureRectSize ||
       height > ctx->Const.MaxTextureRectSize) {
      /* XXX avoid this fallback */
      _swrast_CopyPixels(ctx, srcX, srcY, width, height, dstX, dstY, type);
      return;
   }

   /* Most GL state applies to glCopyPixels, but a there's a few things
    * we need to override:
    */
   _mesa_meta_begin(ctx, (META_RASTERIZATION |
                          META_SHADER |
                          META_TEXTURE |
                          META_TRANSFORM |
                          META_VERTEX |
                          META_VIEWPORT));

   if (copypix->TexObj == 0) {
      /* one-time setup */

      /* create texture object */
      _mesa_GenTextures(1, &copypix->TexObj);
      _mesa_BindTexture(GL_TEXTURE_RECTANGLE, copypix->TexObj);
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      _mesa_TexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, filter);
      _mesa_TexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, filter);
   }
   else {
      _mesa_BindTexture(GL_TEXTURE_RECTANGLE, copypix->TexObj);
   }

   if (copypix->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &copypix->ArrayObj);
      _mesa_BindVertexArray(copypix->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &copypix->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, copypix->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(copypix->verts),
                          copypix->verts, GL_STREAM_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(copypix->verts[0]),
                          (void*) (0 * sizeof(GLfloat)));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(copypix->verts[0]),
                            (void *) (3 * sizeof(GLfloat)));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(copypix->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, copypix->VBO);
   }

   /* vertex positions, texcoords */
   copypix->verts[0][0] = (GLfloat) dstX;
   copypix->verts[0][1] = (GLfloat) dstY;
   copypix->verts[0][2] = z;
   copypix->verts[0][3] = 0.0F;
   copypix->verts[0][4] = 0.0F;
   copypix->verts[1][0] = (GLfloat) dstX1;
   copypix->verts[1][1] = (GLfloat) dstY;
   copypix->verts[1][2] = z;
   copypix->verts[1][3] = (GLfloat) width;
   copypix->verts[1][4] = 0.0F;
   copypix->verts[2][0] = (GLfloat) dstX1;
   copypix->verts[2][1] = (GLfloat) dstY1;
   copypix->verts[2][2] = z;
   copypix->verts[2][3] = (GLfloat) width;
   copypix->verts[2][4] = (GLfloat) height;
   copypix->verts[3][0] = (GLfloat) dstX;
   copypix->verts[3][1] = (GLfloat) dstY1;
   copypix->verts[3][2] = z;
   copypix->verts[3][3] = 0.0F;
   copypix->verts[3][4] = (GLfloat) height;

   /* upload new vertex data */
   _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0,
                          sizeof(copypix->verts), copypix->verts);

   /* copy framebuffer image to texture */
   if (type == GL_COLOR) {
      if (copypix->TexWidth == width &&
          copypix->TexHeight == height &&
          copypix->TexType == type) {
         /* replace existing tex image */
         _mesa_CopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0,
                                 0, 0, srcX, srcY, width, height);
      }
      else {
         /* create new tex image */
         _mesa_CopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA,
                              srcX, srcY, width, height, 0);
         copypix->TexWidth = width;
         copypix->TexHeight = height;
         copypix->TexType = type;
      }
   }
   else if (type == GL_DEPTH) {
      /* TO-DO: Use a GL_DEPTH_COMPONENT texture and a fragment program/shader
       * that replaces the fragment.z value.
       */
   }
   else {
      ASSERT(type == GL_STENCIL);
      /* have to use sw fallback */
   }

   _mesa_Enable(GL_TEXTURE_RECTANGLE);

   /* draw textured quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_Disable(GL_TEXTURE_RECTANGLE);

   _mesa_meta_end(ctx);
}



/**
 * When the glDrawPixels() image size is greater than the max rectangle
 * texture size we use this function to break the glDrawPixels() image
 * into tiles which fit into the max texture size.
 */
static void
tiled_draw_pixels(GLcontext *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLvoid *pixels)
{
   const GLint maxSize = ctx->Const.MaxTextureRectSize;
   struct gl_pixelstore_attrib tileUnpack = *unpack;
   GLint i, j;

   for (i = 0; i < width; i += maxSize) {
      const GLint tileWidth = MIN2(maxSize, width - i);
      const GLint tileX = (GLint) (x + i * ctx->Pixel.ZoomX);

      tileUnpack.SkipPixels = unpack->SkipPixels + i;

      for (j = 0; j < height; j += maxSize) {
         const GLint tileHeight = MIN2(maxSize, height - j);
         const GLint tileY = (GLint) (y + j * ctx->Pixel.ZoomY);

         tileUnpack.SkipRows = unpack->SkipRows + j;

         _mesa_meta_draw_pixels(ctx, tileX, tileY,
                                tileWidth, tileHeight,
                                format, type, &tileUnpack, pixels);
      }
   }
}


/**
 * Meta implementation of ctx->Driver.DrawPixels() in terms
 * of texture mapping and polygon rendering.
 * Note: this function requires GL_ARB_texture_rectangle support.
 */
void
_mesa_meta_draw_pixels(GLcontext *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       const GLvoid *pixels)
{
   const GLenum filter = GL_NEAREST;
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   const GLfloat z = ctx->Current.RasterPos[2];
   const GLfloat x1 = x + width * ctx->Pixel.ZoomX;
   const GLfloat y1 = y + height * ctx->Pixel.ZoomY;
   const struct gl_pixelstore_attrib unpackSave = ctx->Unpack;
   GLenum texIntFormat;
   GLboolean fallback;

   ASSERT(ctx->Extensions.NV_texture_rectangle);

   /*
    * Determine if we can do the glDrawPixels with texture mapping.
    */
   fallback = GL_FALSE;
   if (ctx->_ImageTransferState ||
       ctx->Fog.Enabled) {
      fallback = GL_TRUE;
   }

   if (_mesa_is_color_format(format)) {
      texIntFormat = GL_RGBA;
   }
   else {
      fallback = GL_TRUE;
   }

   if (fallback) {
      _swrast_DrawPixels(ctx, x, y, width, height,
                         format, type, unpack, pixels);
      return;
   }

   /*
    * Check image size against max texture size, draw as tiles if needed.
    */
   if (width > ctx->Const.MaxTextureRectSize ||
       height > ctx->Const.MaxTextureRectSize) {
      tiled_draw_pixels(ctx, x, y, width, height,
                        format, type, unpack, pixels);
      return;
   }

   /* Most GL state applies to glDrawPixels, but a there's a few things
    * we need to override:
    */
   _mesa_meta_begin(ctx, (META_RASTERIZATION |
                          META_SHADER |
                          META_TEXTURE |
                          META_TRANSFORM |
                          META_VERTEX |
                          META_VIEWPORT));

   if (drawpix->TexObj == 0) {
      /* one-time setup */

      /* create texture object */
      _mesa_GenTextures(1, &drawpix->TexObj);
      _mesa_BindTexture(GL_TEXTURE_RECTANGLE, drawpix->TexObj);
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      _mesa_TexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, filter);
      _mesa_TexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, filter);
   }
   else {
      _mesa_BindTexture(GL_TEXTURE_RECTANGLE, drawpix->TexObj);
   }

   if (drawpix->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &drawpix->ArrayObj);
      _mesa_BindVertexArray(drawpix->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &drawpix->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, drawpix->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(drawpix->verts),
                          drawpix->verts, GL_STREAM_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(drawpix->verts[0]),
                          (void*) (0 * sizeof(GLfloat)));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(drawpix->verts[0]),
                            (void *) (3 * sizeof(GLfloat)));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(drawpix->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, drawpix->VBO);
   }

   /* vertex positions, texcoords */
   drawpix->verts[0][0] = (GLfloat) x;
   drawpix->verts[0][1] = (GLfloat) y;
   drawpix->verts[0][2] = z;
   drawpix->verts[0][3] = 0.0F;
   drawpix->verts[0][4] = 0.0F;
   drawpix->verts[1][0] = (GLfloat) x1;
   drawpix->verts[1][1] = (GLfloat) y;
   drawpix->verts[1][2] = z;
   drawpix->verts[1][3] = (GLfloat) width;
   drawpix->verts[1][4] = 0.0F;
   drawpix->verts[2][0] = (GLfloat) x1;
   drawpix->verts[2][1] = (GLfloat) y1;
   drawpix->verts[2][2] = z;
   drawpix->verts[2][3] = (GLfloat) width;
   drawpix->verts[2][4] = (GLfloat) height;
   drawpix->verts[3][0] = (GLfloat) x;
   drawpix->verts[3][1] = (GLfloat) y1;
   drawpix->verts[3][2] = z;
   drawpix->verts[3][3] = 0.0F;
   drawpix->verts[3][4] = (GLfloat) height;

   /* upload new vertex data */
   _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0,
                          sizeof(drawpix->verts), drawpix->verts);

   /* set given unpack params */
   ctx->Unpack = *unpack; /* XXX bufobj */

   /* copy pixel data to texture */
   if (drawpix->TexWidth == width &&
       drawpix->TexHeight == height &&
       drawpix->TexIntFormat == texIntFormat) {
      /* replace existing tex image */
      _mesa_TexSubImage2D(GL_TEXTURE_RECTANGLE, 0,
                          0, 0, width, height, format, type, pixels);
   }
   else {
      /* create new tex image */
      _mesa_TexImage2D(GL_TEXTURE_RECTANGLE, 0, texIntFormat,
                       width, height, 0, format, type, pixels);
      drawpix->TexWidth = width;
      drawpix->TexHeight = height;
      drawpix->TexIntFormat = texIntFormat;
   }

   /* restore unpack params */
   ctx->Unpack = unpackSave; /* XXX bufobj */

   _mesa_Enable(GL_TEXTURE_RECTANGLE);

   /* draw textured quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_Disable(GL_TEXTURE_RECTANGLE);

   _mesa_meta_end(ctx);
}
