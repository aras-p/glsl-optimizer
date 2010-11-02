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
#include "main/arbprogram.h"
#include "main/arrayobj.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/colortab.h"
#include "main/depth.h"
#include "main/enable.h"
#include "main/fbobject.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/matrix.h"
#include "main/mipmap.h"
#include "main/polygon.h"
#include "main/readpix.h"
#include "main/scissor.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/state.h"
#include "main/stencil.h"
#include "main/texobj.h"
#include "main/texenv.h"
#include "main/teximage.h"
#include "main/texparam.h"
#include "main/texstate.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "program/program.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"


/** Return offset in bytes of the field within a vertex struct */
#define OFFSET(FIELD) ((void *) offsetof(struct vertex, FIELD))


/**
 * Flags passed to _mesa_meta_begin().
 */
/*@{*/
#define META_ALL              ~0x0
#define META_ALPHA_TEST        0x1
#define META_BLEND             0x2  /**< includes logicop */
#define META_COLOR_MASK        0x4
#define META_DEPTH_TEST        0x8
#define META_FOG              0x10
#define META_PIXEL_STORE      0x20
#define META_PIXEL_TRANSFER   0x40
#define META_RASTERIZATION    0x80
#define META_SCISSOR         0x100
#define META_SHADER          0x200
#define META_STENCIL_TEST    0x400
#define META_TRANSFORM       0x800 /**< modelview, projection, clip planes */
#define META_TEXTURE        0x1000
#define META_VERTEX         0x2000
#define META_VIEWPORT       0x4000
/*@}*/


/**
 * State which we may save/restore across meta ops.
 * XXX this may be incomplete...
 */
struct save_state
{
   GLbitfield SavedState;  /**< bitmask of META_* flags */

   /** META_ALPHA_TEST */
   GLboolean AlphaEnabled;
   GLenum AlphaFunc;
   GLclampf AlphaRef;

   /** META_BLEND */
   GLbitfield BlendEnabled;
   GLboolean ColorLogicOpEnabled;

   /** META_COLOR_MASK */
   GLubyte ColorMask[MAX_DRAW_BUFFERS][4];

   /** META_DEPTH_TEST */
   struct gl_depthbuffer_attrib Depth;

   /** META_FOG */
   GLboolean Fog;

   /** META_PIXEL_STORE */
   struct gl_pixelstore_attrib Pack, Unpack;

   /** META_PIXEL_TRANSFER */
   GLfloat RedBias, RedScale;
   GLfloat GreenBias, GreenScale;
   GLfloat BlueBias, BlueScale;
   GLfloat AlphaBias, AlphaScale;
   GLfloat DepthBias, DepthScale;
   GLboolean MapColorFlag;

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
   struct gl_shader_program *VertexShader;
   struct gl_shader_program *GeometryShader;
   struct gl_shader_program *FragmentShader;
   struct gl_shader_program *ActiveShader;

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
 * Temporary texture used for glBlitFramebuffer, glDrawPixels, etc.
 * This is currently shared by all the meta ops.  But we could create a
 * separate one for each of glDrawPixel, glBlitFramebuffer, glCopyPixels, etc.
 */
struct temp_texture
{
   GLuint TexObj;
   GLenum Target;         /**< GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE */
   GLsizei MinSize;       /**< Min texture size to allocate */
   GLsizei MaxSize;       /**< Max possible texture size */
   GLboolean NPOT;        /**< Non-power of two size OK? */
   GLsizei Width, Height; /**< Current texture size */
   GLenum IntFormat;
   GLfloat Sright, Ttop;  /**< right, top texcoords */
};


/**
 * State for glBlitFramebufer()
 */
struct blit_state
{
   GLuint ArrayObj;
   GLuint VBO;
   GLuint DepthFP;
};


/**
 * State for glClear()
 */
struct clear_state
{
   GLuint ArrayObj;
   GLuint VBO;
};


/**
 * State for glCopyPixels()
 */
struct copypix_state
{
   GLuint ArrayObj;
   GLuint VBO;
};


/**
 * State for glDrawPixels()
 */
struct drawpix_state
{
   GLuint ArrayObj;

   GLuint StencilFP;  /**< Fragment program for drawing stencil images */
   GLuint DepthFP;  /**< Fragment program for drawing depth images */
};


/**
 * State for glBitmap()
 */
struct bitmap_state
{
   GLuint ArrayObj;
   GLuint VBO;
   struct temp_texture Tex;  /**< separate texture from other meta ops */
};


/**
 * State for _mesa_meta_generate_mipmap()
 */
struct gen_mipmap_state
{
   GLuint ArrayObj;
   GLuint VBO;
   GLuint FBO;
};


/**
 * All per-context meta state.
 */
struct gl_meta_state
{
   struct save_state Save;    /**< state saved during meta-ops */

   struct temp_texture TempTex;

   struct blit_state Blit;    /**< For _mesa_meta_BlitFramebuffer() */
   struct clear_state Clear;  /**< For _mesa_meta_Clear() */
   struct copypix_state CopyPix;  /**< For _mesa_meta_CopyPixels() */
   struct drawpix_state DrawPix;  /**< For _mesa_meta_DrawPixels() */
   struct bitmap_state Bitmap;    /**< For _mesa_meta_Bitmap() */
   struct gen_mipmap_state Mipmap;    /**< For _mesa_meta_GenerateMipmap() */
};


/**
 * Initialize meta-ops for a context.
 * To be called once during context creation.
 */
void
_mesa_meta_init(struct gl_context *ctx)
{
   ASSERT(!ctx->Meta);

   ctx->Meta = CALLOC_STRUCT(gl_meta_state);
}


/**
 * Free context meta-op state.
 * To be called once during context destruction.
 */
void
_mesa_meta_free(struct gl_context *ctx)
{
   /* Note: Any textures, VBOs, etc, that we allocate should get
    * freed by the normal context destruction code.  But this would be
    * the place to free other meta data someday.
    */
   free(ctx->Meta);
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
_mesa_meta_begin(struct gl_context *ctx, GLbitfield state)
{
   struct save_state *save = &ctx->Meta->Save;

   save->SavedState = state;

   if (state & META_ALPHA_TEST) {
      save->AlphaEnabled = ctx->Color.AlphaEnabled;
      save->AlphaFunc = ctx->Color.AlphaFunc;
      save->AlphaRef = ctx->Color.AlphaRef;
      if (ctx->Color.AlphaEnabled)
         _mesa_set_enable(ctx, GL_ALPHA_TEST, GL_FALSE);
   }

   if (state & META_BLEND) {
      save->BlendEnabled = ctx->Color.BlendEnabled;
      if (ctx->Color.BlendEnabled) {
         if (ctx->Extensions.EXT_draw_buffers2) {
            GLuint i;
            for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
               _mesa_set_enablei(ctx, GL_BLEND, i, GL_FALSE);
            }
         }
         else {
            _mesa_set_enable(ctx, GL_BLEND, GL_FALSE);
         }
      }
      save->ColorLogicOpEnabled = ctx->Color.ColorLogicOpEnabled;
      if (ctx->Color.ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, GL_FALSE);
   }

   if (state & META_COLOR_MASK) {
      memcpy(save->ColorMask, ctx->Color.ColorMask,
             sizeof(ctx->Color.ColorMask));
      if (!ctx->Color.ColorMask[0][0] ||
          !ctx->Color.ColorMask[0][1] ||
          !ctx->Color.ColorMask[0][2] ||
          !ctx->Color.ColorMask[0][3])
         _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   }

   if (state & META_DEPTH_TEST) {
      save->Depth = ctx->Depth; /* struct copy */
      if (ctx->Depth.Test)
         _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_FALSE);
   }

   if (state & META_FOG) {
      save->Fog = ctx->Fog.Enabled;
      if (ctx->Fog.Enabled)
         _mesa_set_enable(ctx, GL_FOG, GL_FALSE);
   }

   if (state & META_PIXEL_STORE) {
      save->Pack = ctx->Pack;
      save->Unpack = ctx->Unpack;
      ctx->Pack = ctx->DefaultPacking;
      ctx->Unpack = ctx->DefaultPacking;
   }

   if (state & META_PIXEL_TRANSFER) {
      save->RedScale = ctx->Pixel.RedScale;
      save->RedBias = ctx->Pixel.RedBias;
      save->GreenScale = ctx->Pixel.GreenScale;
      save->GreenBias = ctx->Pixel.GreenBias;
      save->BlueScale = ctx->Pixel.BlueScale;
      save->BlueBias = ctx->Pixel.BlueBias;
      save->AlphaScale = ctx->Pixel.AlphaScale;
      save->AlphaBias = ctx->Pixel.AlphaBias;
      save->MapColorFlag = ctx->Pixel.MapColorFlag;
      ctx->Pixel.RedScale = 1.0F;
      ctx->Pixel.RedBias = 0.0F;
      ctx->Pixel.GreenScale = 1.0F;
      ctx->Pixel.GreenBias = 0.0F;
      ctx->Pixel.BlueScale = 1.0F;
      ctx->Pixel.BlueBias = 0.0F;
      ctx->Pixel.AlphaScale = 1.0F;
      ctx->Pixel.AlphaBias = 0.0F;
      ctx->Pixel.MapColorFlag = GL_FALSE;
      /* XXX more state */
      ctx->NewState |=_NEW_PIXEL;
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
      _mesa_set_enable(ctx, GL_SCISSOR_TEST, GL_FALSE);
   }

   if (state & META_SHADER) {
      if (ctx->Extensions.ARB_vertex_program) {
         save->VertexProgramEnabled = ctx->VertexProgram.Enabled;
         _mesa_reference_vertprog(ctx, &save->VertexProgram,
				  ctx->VertexProgram.Current);
         _mesa_set_enable(ctx, GL_VERTEX_PROGRAM_ARB, GL_FALSE);
      }

      if (ctx->Extensions.ARB_fragment_program) {
         save->FragmentProgramEnabled = ctx->FragmentProgram.Enabled;
         _mesa_reference_fragprog(ctx, &save->FragmentProgram,
				  ctx->FragmentProgram.Current);
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_FALSE);
      }

      if (ctx->Extensions.ARB_shader_objects) {
	 _mesa_reference_shader_program(ctx, &save->VertexShader,
					ctx->Shader.CurrentVertexProgram);
	 _mesa_reference_shader_program(ctx, &save->GeometryShader,
					ctx->Shader.CurrentGeometryProgram);
	 _mesa_reference_shader_program(ctx, &save->FragmentShader,
					ctx->Shader.CurrentFragmentProgram);
	 _mesa_reference_shader_program(ctx, &save->ActiveShader,
					ctx->Shader.CurrentFragmentProgram);

         _mesa_UseProgramObjectARB(0);
      }
   }

   if (state & META_STENCIL_TEST) {
      save->Stencil = ctx->Stencil; /* struct copy */
      if (ctx->Stencil.Enabled)
         _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_FALSE);
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
      memcpy(save->ModelviewMatrix, ctx->ModelviewMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      memcpy(save->ProjectionMatrix, ctx->ProjectionMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      memcpy(save->TextureMatrix, ctx->TextureMatrixStack[0].Top->m,
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
      _mesa_Ortho(0.0, ctx->DrawBuffer->Width,
                  0.0, ctx->DrawBuffer->Height,
                  -1.0, 1.0);
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
      /* save viewport state */
      save->ViewportX = ctx->Viewport.X;
      save->ViewportY = ctx->Viewport.Y;
      save->ViewportW = ctx->Viewport.Width;
      save->ViewportH = ctx->Viewport.Height;
      /* set viewport to match window size */
      if (ctx->Viewport.X != 0 ||
          ctx->Viewport.Y != 0 ||
          ctx->Viewport.Width != ctx->DrawBuffer->Width ||
          ctx->Viewport.Height != ctx->DrawBuffer->Height) {
         _mesa_set_viewport(ctx, 0, 0,
                            ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);
      }
      /* save depth range state */
      save->DepthNear = ctx->Viewport.Near;
      save->DepthFar = ctx->Viewport.Far;
      /* set depth range to default */
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
_mesa_meta_end(struct gl_context *ctx)
{
   struct save_state *save = &ctx->Meta->Save;
   const GLbitfield state = save->SavedState;

   if (state & META_ALPHA_TEST) {
      if (ctx->Color.AlphaEnabled != save->AlphaEnabled)
         _mesa_set_enable(ctx, GL_ALPHA_TEST, save->AlphaEnabled);
      _mesa_AlphaFunc(save->AlphaFunc, save->AlphaRef);
   }

   if (state & META_BLEND) {
      if (ctx->Color.BlendEnabled != save->BlendEnabled) {
         if (ctx->Extensions.EXT_draw_buffers2) {
            GLuint i;
            for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
               _mesa_set_enablei(ctx, GL_BLEND, i, (save->BlendEnabled >> i) & 1);
            }
         }
         else {
            _mesa_set_enable(ctx, GL_BLEND, (save->BlendEnabled & 1));
         }
      }
      if (ctx->Color.ColorLogicOpEnabled != save->ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, save->ColorLogicOpEnabled);
   }

   if (state & META_COLOR_MASK) {
      GLuint i;
      for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
         if (!TEST_EQ_4V(ctx->Color.ColorMask[i], save->ColorMask[i])) {
            if (i == 0) {
               _mesa_ColorMask(save->ColorMask[i][0], save->ColorMask[i][1],
                               save->ColorMask[i][2], save->ColorMask[i][3]);
            }
            else {
               _mesa_ColorMaskIndexed(i,
                                      save->ColorMask[i][0],
                                      save->ColorMask[i][1],
                                      save->ColorMask[i][2],
                                      save->ColorMask[i][3]);
            }
         }
      }
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

   if (state & META_PIXEL_STORE) {
      ctx->Pack = save->Pack;
      ctx->Unpack = save->Unpack;
   }

   if (state & META_PIXEL_TRANSFER) {
      ctx->Pixel.RedScale = save->RedScale;
      ctx->Pixel.RedBias = save->RedBias;
      ctx->Pixel.GreenScale = save->GreenScale;
      ctx->Pixel.GreenBias = save->GreenBias;
      ctx->Pixel.BlueScale = save->BlueScale;
      ctx->Pixel.BlueBias = save->BlueBias;
      ctx->Pixel.AlphaScale = save->AlphaScale;
      ctx->Pixel.AlphaBias = save->AlphaBias;
      ctx->Pixel.MapColorFlag = save->MapColorFlag;
      /* XXX more state */
      ctx->NewState |=_NEW_PIXEL;
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
	 _mesa_reference_vertprog(ctx, &save->VertexProgram, NULL);
      }

      if (ctx->Extensions.ARB_fragment_program) {
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB,
                          save->FragmentProgramEnabled);
         _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
                                  save->FragmentProgram);
	 _mesa_reference_fragprog(ctx, &save->FragmentProgram, NULL);
      }

      if (ctx->Extensions.ARB_vertex_shader)
	 _mesa_use_shader_program(ctx, GL_VERTEX_SHADER, save->VertexShader);

      if (ctx->Extensions.ARB_geometry_shader4)
	 _mesa_use_shader_program(ctx, GL_GEOMETRY_SHADER_ARB,
				  save->GeometryShader);

      if (ctx->Extensions.ARB_fragment_shader)
	 _mesa_use_shader_program(ctx, GL_FRAGMENT_SHADER,
				  save->FragmentShader);

      _mesa_reference_shader_program(ctx, &ctx->Shader.ActiveProgram,
				     save->ActiveShader);
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
         _mesa_reference_texobj(&save->CurrentTexture[tgt], NULL);
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
      if (save->ViewportX != ctx->Viewport.X ||
          save->ViewportY != ctx->Viewport.Y ||
          save->ViewportW != ctx->Viewport.Width ||
          save->ViewportH != ctx->Viewport.Height) {
         _mesa_set_viewport(ctx, save->ViewportX, save->ViewportY,
                            save->ViewportW, save->ViewportH);
      }
      _mesa_DepthRange(save->DepthNear, save->DepthFar);
   }

   /* misc */
   if (save->Lighting) {
      _mesa_set_enable(ctx, GL_LIGHTING, GL_TRUE);
   }
}


/**
 * Convert Z from a normalized value in the range [0, 1] to an object-space
 * Z coordinate in [-1, +1] so that drawing at the new Z position with the
 * default/identity ortho projection results in the original Z value.
 * Used by the meta-Clear, Draw/CopyPixels and Bitmap functions where the Z
 * value comes from the clear value or raster position.
 */
static INLINE GLfloat
invert_z(GLfloat normZ)
{
   GLfloat objZ = 1.0 - 2.0 * normZ;
   return objZ;
}


/**
 * One-time init for a temp_texture object.
 * Choose tex target, compute max tex size, etc.
 */
static void
init_temp_texture(struct gl_context *ctx, struct temp_texture *tex)
{
   /* prefer texture rectangle */
   if (ctx->Extensions.NV_texture_rectangle) {
      tex->Target = GL_TEXTURE_RECTANGLE;
      tex->MaxSize = ctx->Const.MaxTextureRectSize;
      tex->NPOT = GL_TRUE;
   }
   else {
      /* use 2D texture, NPOT if possible */
      tex->Target = GL_TEXTURE_2D;
      tex->MaxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      tex->NPOT = ctx->Extensions.ARB_texture_non_power_of_two;
   }
   tex->MinSize = 16;  /* 16 x 16 at least */
   assert(tex->MaxSize > 0);

   _mesa_GenTextures(1, &tex->TexObj);
}


/**
 * Return pointer to temp_texture info for non-bitmap ops.
 * This does some one-time init if needed.
 */
static struct temp_texture *
get_temp_texture(struct gl_context *ctx)
{
   struct temp_texture *tex = &ctx->Meta->TempTex;

   if (!tex->TexObj) {
      init_temp_texture(ctx, tex);
   }

   return tex;
}


/**
 * Return pointer to temp_texture info for _mesa_meta_bitmap().
 * We use a separate texture for bitmaps to reduce texture
 * allocation/deallocation.
 */
static struct temp_texture *
get_bitmap_temp_texture(struct gl_context *ctx)
{
   struct temp_texture *tex = &ctx->Meta->Bitmap.Tex;

   if (!tex->TexObj) {
      init_temp_texture(ctx, tex);
   }

   return tex;
}


/**
 * Compute the width/height of texture needed to draw an image of the
 * given size.  Return a flag indicating whether the current texture
 * can be re-used (glTexSubImage2D) or if a new texture needs to be
 * allocated (glTexImage2D).
 * Also, compute s/t texcoords for drawing.
 *
 * \return GL_TRUE if new texture is needed, GL_FALSE otherwise
 */
static GLboolean
alloc_texture(struct temp_texture *tex,
              GLsizei width, GLsizei height, GLenum intFormat)
{
   GLboolean newTex = GL_FALSE;

   ASSERT(width <= tex->MaxSize);
   ASSERT(height <= tex->MaxSize);

   if (width > tex->Width ||
       height > tex->Height ||
       intFormat != tex->IntFormat) {
      /* alloc new texture (larger or different format) */

      if (tex->NPOT) {
         /* use non-power of two size */
         tex->Width = MAX2(tex->MinSize, width);
         tex->Height = MAX2(tex->MinSize, height);
      }
      else {
         /* find power of two size */
         GLsizei w, h;
         w = h = tex->MinSize;
         while (w < width)
            w *= 2;
         while (h < height)
            h *= 2;
         tex->Width = w;
         tex->Height = h;
      }

      tex->IntFormat = intFormat;

      newTex = GL_TRUE;
   }

   /* compute texcoords */
   if (tex->Target == GL_TEXTURE_RECTANGLE) {
      tex->Sright = (GLfloat) width;
      tex->Ttop = (GLfloat) height;
   }
   else {
      tex->Sright = (GLfloat) width / tex->Width;
      tex->Ttop = (GLfloat) height / tex->Height;
   }

   return newTex;
}


/**
 * Setup/load texture for glCopyPixels or glBlitFramebuffer.
 */
static void
setup_copypix_texture(struct temp_texture *tex,
                      GLboolean newTex,
                      GLint srcX, GLint srcY,
                      GLsizei width, GLsizei height, GLenum intFormat,
                      GLenum filter)
{
   _mesa_BindTexture(tex->Target, tex->TexObj);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MIN_FILTER, filter);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MAG_FILTER, filter);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   /* copy framebuffer image to texture */
   if (newTex) {
      /* create new tex image */
      if (tex->Width == width && tex->Height == height) {
         /* create new tex with framebuffer data */
         _mesa_CopyTexImage2D(tex->Target, 0, tex->IntFormat,
                              srcX, srcY, width, height, 0);
      }
      else {
         /* create empty texture */
         _mesa_TexImage2D(tex->Target, 0, tex->IntFormat,
                          tex->Width, tex->Height, 0,
                          intFormat, GL_UNSIGNED_BYTE, NULL);
         /* load image */
         _mesa_CopyTexSubImage2D(tex->Target, 0,
                                 0, 0, srcX, srcY, width, height);
      }
   }
   else {
      /* replace existing tex image */
      _mesa_CopyTexSubImage2D(tex->Target, 0,
                              0, 0, srcX, srcY, width, height);
   }
}


/**
 * Setup/load texture for glDrawPixels.
 */
static void
setup_drawpix_texture(struct gl_context *ctx,
		      struct temp_texture *tex,
                      GLboolean newTex,
                      GLenum texIntFormat,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const GLvoid *pixels)
{
   _mesa_BindTexture(tex->Target, tex->TexObj);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   /* copy pixel data to texture */
   if (newTex) {
      /* create new tex image */
      if (tex->Width == width && tex->Height == height) {
         /* create new tex and load image data */
         _mesa_TexImage2D(tex->Target, 0, tex->IntFormat,
                          tex->Width, tex->Height, 0, format, type, pixels);
      }
      else {
	 struct gl_buffer_object *save_unpack_obj = NULL;

	 _mesa_reference_buffer_object(ctx, &save_unpack_obj,
				       ctx->Unpack.BufferObj);
	 _mesa_BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
         /* create empty texture */
         _mesa_TexImage2D(tex->Target, 0, tex->IntFormat,
                          tex->Width, tex->Height, 0, format, type, NULL);
	 if (save_unpack_obj != NULL)
	    _mesa_BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
				save_unpack_obj->Name);
         /* load image */
         _mesa_TexSubImage2D(tex->Target, 0,
                             0, 0, width, height, format, type, pixels);
      }
   }
   else {
      /* replace existing tex image */
      _mesa_TexSubImage2D(tex->Target, 0,
                          0, 0, width, height, format, type, pixels);
   }
}



/**
 * One-time init for drawing depth pixels.
 */
static void
init_blit_depth_pixels(struct gl_context *ctx)
{
   static const char *program =
      "!!ARBfp1.0\n"
      "TEX result.depth, fragment.texcoord[0], texture[0], %s; \n"
      "END \n";
   char program2[200];
   struct blit_state *blit = &ctx->Meta->Blit;
   struct temp_texture *tex = get_temp_texture(ctx);
   const char *texTarget;

   assert(blit->DepthFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenPrograms(1, &blit->DepthFP);
   _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, blit->DepthFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}


/**
 * Try to do a glBlitFramebuffer using no-copy texturing.
 * We can do this when the src renderbuffer is actually a texture.
 * But if the src buffer == dst buffer we cannot do this.
 *
 * \return new buffer mask indicating the buffers left to blit using the
 *         normal path.
 */
static GLbitfield
blitframebuffer_texture(struct gl_context *ctx,
                        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter)
{
   if (mask & GL_COLOR_BUFFER_BIT) {
      const struct gl_framebuffer *drawFb = ctx->DrawBuffer;
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      const struct gl_renderbuffer_attachment *drawAtt =
         &drawFb->Attachment[drawFb->_ColorDrawBufferIndexes[0]];
      const struct gl_renderbuffer_attachment *readAtt =
         &readFb->Attachment[readFb->_ColorReadBufferIndex];

      if (readAtt && readAtt->Texture) {
         const struct gl_texture_object *texObj = readAtt->Texture;
         const GLuint srcLevel = readAtt->TextureLevel;
         const GLenum minFilterSave = texObj->MinFilter;
         const GLenum magFilterSave = texObj->MagFilter;
         const GLint baseLevelSave = texObj->BaseLevel;
         const GLint maxLevelSave = texObj->MaxLevel;
         const GLenum wrapSSave = texObj->WrapS;
         const GLenum wrapTSave = texObj->WrapT;
         const GLenum target = texObj->Target;

         if (drawAtt->Texture == readAtt->Texture) {
            /* Can't use same texture as both the source and dest.  We need
             * to handle overlapping blits and besides, some hw may not
             * support this.
             */
            return mask;
         }

         if (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_ARB) {
            /* Can't handle other texture types at this time */
            return mask;
         }

         /*
         printf("Blit from texture!\n");
         printf("  srcAtt %p  dstAtt %p\n", readAtt, drawAtt);
         printf("  srcTex %p  dstText %p\n", texObj, drawAtt->Texture);
         */

         /* Prepare src texture state */
         _mesa_BindTexture(target, texObj->Name);
         _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
         _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
         if (target != GL_TEXTURE_RECTANGLE_ARB) {
            _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, srcLevel);
            _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, srcLevel);
         }
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
         _mesa_set_enable(ctx, target, GL_TRUE);

         /* Prepare vertex data (the VBO was previously created and bound) */
         {
            struct vertex {
               GLfloat x, y, s, t;
            };
            struct vertex verts[4];
            GLfloat s0, t0, s1, t1;

            if (target == GL_TEXTURE_2D) {
               const struct gl_texture_image *texImage
                   = _mesa_select_tex_image(ctx, texObj, target, srcLevel);
               s0 = srcX0 / (float) texImage->Width;
               s1 = srcX1 / (float) texImage->Width;
               t0 = srcY0 / (float) texImage->Height;
               t1 = srcY1 / (float) texImage->Height;
            }
            else {
               assert(target == GL_TEXTURE_RECTANGLE_ARB);
               s0 = srcX0;
               s1 = srcX1;
               t0 = srcY0;
               t1 = srcY1;
            }

            verts[0].x = (GLfloat) dstX0;
            verts[0].y = (GLfloat) dstY0;
            verts[1].x = (GLfloat) dstX1;
            verts[1].y = (GLfloat) dstY0;
            verts[2].x = (GLfloat) dstX1;
            verts[2].y = (GLfloat) dstY1;
            verts[3].x = (GLfloat) dstX0;
            verts[3].y = (GLfloat) dstY1;

            verts[0].s = s0;
            verts[0].t = t0;
            verts[1].s = s1;
            verts[1].t = t0;
            verts[2].s = s1;
            verts[2].t = t1;
            verts[3].s = s0;
            verts[3].t = t1;

            _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
         }

         _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

         /* Restore texture object state, the texture binding will
          * be restored by _mesa_meta_end().
          */
         _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterSave);
         _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilterSave);
         if (target != GL_TEXTURE_RECTANGLE_ARB) {
            _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, baseLevelSave);
            _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
         }
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, wrapSSave);
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, wrapTSave);

         /* Done with color buffer */
         mask &= ~GL_COLOR_BUFFER_BIT;
      }
   }

   return mask;
}


/**
 * Meta implementation of ctx->Driver.BlitFramebuffer() in terms
 * of texture mapping and polygon rendering.
 */
void
_mesa_meta_BlitFramebuffer(struct gl_context *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter)
{
   struct blit_state *blit = &ctx->Meta->Blit;
   struct temp_texture *tex = get_temp_texture(ctx);
   const GLsizei maxTexSize = tex->MaxSize;
   const GLint srcX = MIN2(srcX0, srcX1);
   const GLint srcY = MIN2(srcY0, srcY1);
   const GLint srcW = abs(srcX1 - srcX0);
   const GLint srcH = abs(srcY1 - srcY0);
   const GLboolean srcFlipX = srcX1 < srcX0;
   const GLboolean srcFlipY = srcY1 < srcY0;
   struct vertex {
      GLfloat x, y, s, t;
   };
   struct vertex verts[4];
   GLboolean newTex;

   if (srcW > maxTexSize || srcH > maxTexSize) {
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

   if (blit->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &blit->ArrayObj);
      _mesa_BindVertexArray(blit->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &blit->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, blit->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(blit->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, blit->VBO);
   }

   /* Try faster, direct texture approach first */
   mask = blitframebuffer_texture(ctx, srcX0, srcY0, srcX1, srcY1,
                                  dstX0, dstY0, dstX1, dstY1, mask, filter);
   if (mask == 0x0) {
      _mesa_meta_end(ctx);
      return;
   }

   /* Continue with "normal" approach which involves copying the src rect
    * into a temporary texture and is "blitted" by drawing a textured quad.
    */

   newTex = alloc_texture(tex, srcW, srcH, GL_RGBA);

   /* vertex positions/texcoords (after texture allocation!) */
   {
      verts[0].x = (GLfloat) dstX0;
      verts[0].y = (GLfloat) dstY0;
      verts[1].x = (GLfloat) dstX1;
      verts[1].y = (GLfloat) dstY0;
      verts[2].x = (GLfloat) dstX1;
      verts[2].y = (GLfloat) dstY1;
      verts[3].x = (GLfloat) dstX0;
      verts[3].y = (GLfloat) dstY1;

      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;

      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   if (mask & GL_COLOR_BUFFER_BIT) {
      setup_copypix_texture(tex, newTex, srcX, srcY, srcW, srcH,
                            GL_RGBA, filter);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      GLuint *tmp = (GLuint *) malloc(srcW * srcH * sizeof(GLuint));
      if (tmp) {
         if (!blit->DepthFP)
            init_blit_depth_pixels(ctx);

         /* maybe change tex format here */
         newTex = alloc_texture(tex, srcW, srcH, GL_DEPTH_COMPONENT);

         _mesa_ReadPixels(srcX, srcY, srcW, srcH,
                          GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, tmp);

         setup_drawpix_texture(ctx, tex, newTex, GL_DEPTH_COMPONENT, srcW, srcH,
                               GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, tmp);

         _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, blit->DepthFP);
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);
         _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
         _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
         _mesa_DepthFunc(GL_ALWAYS);
         _mesa_DepthMask(GL_TRUE);

         _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
         mask &= ~GL_DEPTH_BUFFER_BIT;

         free(tmp);
      }
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      /* XXX can't easily do stencil */
   }

   _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_meta_end(ctx);

   if (mask) {
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
   }
}


/**
 * Meta implementation of ctx->Driver.Clear() in terms of polygon rendering.
 */
void
_mesa_meta_Clear(struct gl_context *ctx, GLbitfield buffers)
{
   struct clear_state *clear = &ctx->Meta->Clear;
   struct vertex {
      GLfloat x, y, z, r, g, b, a;
   };
   struct vertex verts[4];
   /* save all state but scissor, pixel pack/unpack */
   GLbitfield metaSave = META_ALL - META_SCISSOR - META_PIXEL_STORE;

   if (buffers & BUFFER_BITS_COLOR) {
      /* if clearing color buffers, don't save/restore colormask */
      metaSave -= META_COLOR_MASK;
   }

   _mesa_meta_begin(ctx, metaSave);

   if (clear->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &clear->ArrayObj);
      _mesa_BindVertexArray(clear->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &clear->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_ColorPointer(4, GL_FLOAT, sizeof(struct vertex), OFFSET(r));
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
      ASSERT(metaSave & META_COLOR_MASK);
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

   /* vertex positions/colors */
   {
      const GLfloat x0 = (GLfloat) ctx->DrawBuffer->_Xmin;
      const GLfloat y0 = (GLfloat) ctx->DrawBuffer->_Ymin;
      const GLfloat x1 = (GLfloat) ctx->DrawBuffer->_Xmax;
      const GLfloat y1 = (GLfloat) ctx->DrawBuffer->_Ymax;
      const GLfloat z = invert_z(ctx->Depth.Clear);
      GLuint i;

      verts[0].x = x0;
      verts[0].y = y0;
      verts[0].z = z;
      verts[1].x = x1;
      verts[1].y = y0;
      verts[1].z = z;
      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;
      verts[3].x = x0;
      verts[3].y = y1;
      verts[3].z = z;

      /* vertex colors */
      for (i = 0; i < 4; i++) {
         verts[i].r = ctx->Color.ClearColor[0];
         verts[i].g = ctx->Color.ClearColor[1];
         verts[i].b = ctx->Color.ClearColor[2];
         verts[i].a = ctx->Color.ClearColor[3];
      }

      /* upload new vertex data */
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts), verts,
			  GL_DYNAMIC_DRAW_ARB);
   }

   /* draw quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_meta_end(ctx);
}


/**
 * Meta implementation of ctx->Driver.CopyPixels() in terms
 * of texture mapping and polygon rendering.
 */
void
_mesa_meta_CopyPixels(struct gl_context *ctx, GLint srcX, GLint srcY,
                      GLsizei width, GLsizei height,
                      GLint dstX, GLint dstY, GLenum type)
{
   struct copypix_state *copypix = &ctx->Meta->CopyPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   struct vertex {
      GLfloat x, y, z, s, t;
   };
   struct vertex verts[4];
   GLboolean newTex;
   GLenum intFormat = GL_RGBA;

   if (type != GL_COLOR ||
       ctx->_ImageTransferState ||
       ctx->Fog.Enabled ||
       width > tex->MaxSize ||
       height > tex->MaxSize) {
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

   if (copypix->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &copypix->ArrayObj);
      _mesa_BindVertexArray(copypix->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &copypix->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, copypix->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(copypix->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, copypix->VBO);
   }

   newTex = alloc_texture(tex, width, height, intFormat);

   /* vertex positions, texcoords (after texture allocation!) */
   {
      const GLfloat dstX0 = (GLfloat) dstX;
      const GLfloat dstY0 = (GLfloat) dstY;
      const GLfloat dstX1 = dstX + width * ctx->Pixel.ZoomX;
      const GLfloat dstY1 = dstY + height * ctx->Pixel.ZoomY;
      const GLfloat z = invert_z(ctx->Current.RasterPos[2]);

      verts[0].x = dstX0;
      verts[0].y = dstY0;
      verts[0].z = z;
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].x = dstX1;
      verts[1].y = dstY0;
      verts[1].z = z;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].x = dstX1;
      verts[2].y = dstY1;
      verts[2].z = z;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].x = dstX0;
      verts[3].y = dstY1;
      verts[3].z = z;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;

      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   /* Alloc/setup texture */
   setup_copypix_texture(tex, newTex, srcX, srcY, width, height,
                         GL_RGBA, GL_NEAREST);

   _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   /* draw textured quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_meta_end(ctx);
}



/**
 * When the glDrawPixels() image size is greater than the max rectangle
 * texture size we use this function to break the glDrawPixels() image
 * into tiles which fit into the max texture size.
 */
static void
tiled_draw_pixels(struct gl_context *ctx,
                  GLint tileSize,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLvoid *pixels)
{
   struct gl_pixelstore_attrib tileUnpack = *unpack;
   GLint i, j;

   if (tileUnpack.RowLength == 0)
      tileUnpack.RowLength = width;

   for (i = 0; i < width; i += tileSize) {
      const GLint tileWidth = MIN2(tileSize, width - i);
      const GLint tileX = (GLint) (x + i * ctx->Pixel.ZoomX);

      tileUnpack.SkipPixels = unpack->SkipPixels + i;

      for (j = 0; j < height; j += tileSize) {
         const GLint tileHeight = MIN2(tileSize, height - j);
         const GLint tileY = (GLint) (y + j * ctx->Pixel.ZoomY);

         tileUnpack.SkipRows = unpack->SkipRows + j;

         _mesa_meta_DrawPixels(ctx, tileX, tileY, tileWidth, tileHeight,
                               format, type, &tileUnpack, pixels);
      }
   }
}


/**
 * One-time init for drawing stencil pixels.
 */
static void
init_draw_stencil_pixels(struct gl_context *ctx)
{
   /* This program is run eight times, once for each stencil bit.
    * The stencil values to draw are found in an 8-bit alpha texture.
    * We read the texture/stencil value and test if bit 'b' is set.
    * If the bit is not set, use KIL to kill the fragment.
    * Finally, we use the stencil test to update the stencil buffer.
    *
    * The basic algorithm for checking if a bit is set is:
    *   if (is_odd(value / (1 << bit)))
    *      result is one (or non-zero).
    *   else
    *      result is zero.
    * The program parameter contains three values:
    *   parm.x = 255 / (1 << bit)
    *   parm.y = 0.5
    *   parm.z = 0.0
    */
   static const char *program =
      "!!ARBfp1.0\n"
      "PARAM parm = program.local[0]; \n"
      "TEMP t; \n"
      "TEX t, fragment.texcoord[0], texture[0], %s; \n"   /* NOTE %s here! */
      "# t = t * 255 / bit \n"
      "MUL t.x, t.a, parm.x; \n"
      "# t = (int) t \n"
      "FRC t.y, t.x; \n"
      "SUB t.x, t.x, t.y; \n"
      "# t = t * 0.5 \n"
      "MUL t.x, t.x, parm.y; \n"
      "# t = fract(t.x) \n"
      "FRC t.x, t.x; # if t.x != 0, then the bit is set \n"
      "# t.x = (t.x == 0 ? 1 : 0) \n"
      "SGE t.x, -t.x, parm.z; \n"
      "KIL -t.x; \n"
      "# for debug only \n"
      "#MOV result.color, t.x; \n"
      "END \n";
   char program2[1000];
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   const char *texTarget;

   assert(drawpix->StencilFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenPrograms(1, &drawpix->StencilFP);
   _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->StencilFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}


/**
 * One-time init for drawing depth pixels.
 */
static void
init_draw_depth_pixels(struct gl_context *ctx)
{
   static const char *program =
      "!!ARBfp1.0\n"
      "PARAM color = program.local[0]; \n"
      "TEX result.depth, fragment.texcoord[0], texture[0], %s; \n"
      "MOV result.color, color; \n"
      "END \n";
   char program2[200];
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   const char *texTarget;

   assert(drawpix->DepthFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenPrograms(1, &drawpix->DepthFP);
   _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->DepthFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}


/**
 * Meta implementation of ctx->Driver.DrawPixels() in terms
 * of texture mapping and polygon rendering.
 */
void
_mesa_meta_DrawPixels(struct gl_context *ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid *pixels)
{
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   const struct gl_pixelstore_attrib unpackSave = ctx->Unpack;
   const GLuint origStencilMask = ctx->Stencil.WriteMask[0];
   struct vertex {
      GLfloat x, y, z, s, t;
   };
   struct vertex verts[4];
   GLenum texIntFormat;
   GLboolean fallback, newTex;
   GLbitfield metaExtraSave = 0x0;
   GLuint vbo;

   /*
    * Determine if we can do the glDrawPixels with texture mapping.
    */
   fallback = GL_FALSE;
   if (ctx->_ImageTransferState ||
       ctx->Fog.Enabled) {
      fallback = GL_TRUE;
   }

   if (_mesa_is_color_format(format)) {
      /* use more compact format when possible */
      /* XXX disable special case for GL_LUMINANCE for now to work around
       * apparent i965 driver bug (see bug #23670).
       */
      if (/*format == GL_LUMINANCE ||*/ format == GL_LUMINANCE_ALPHA)
         texIntFormat = format;
      else
         texIntFormat = GL_RGBA;
   }
   else if (_mesa_is_stencil_format(format)) {
      if (ctx->Extensions.ARB_fragment_program &&
          ctx->Pixel.IndexShift == 0 &&
          ctx->Pixel.IndexOffset == 0 &&
          type == GL_UNSIGNED_BYTE) {
         /* We'll store stencil as alpha.  This only works for GLubyte
          * image data because of how incoming values are mapped to alpha
          * in [0,1].
          */
         texIntFormat = GL_ALPHA;
         metaExtraSave = (META_COLOR_MASK |
                          META_DEPTH_TEST |
                          META_SHADER |
                          META_STENCIL_TEST);
      }
      else {
         fallback = GL_TRUE;
      }
   }
   else if (_mesa_is_depth_format(format)) {
      if (ctx->Extensions.ARB_depth_texture &&
          ctx->Extensions.ARB_fragment_program) {
         texIntFormat = GL_DEPTH_COMPONENT;
         metaExtraSave = (META_SHADER);
      }
      else {
         fallback = GL_TRUE;
      }
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
   if (width > tex->MaxSize || height > tex->MaxSize) {
      tiled_draw_pixels(ctx, tex->MaxSize, x, y, width, height,
                        format, type, unpack, pixels);
      return;
   }

   /* Most GL state applies to glDrawPixels (like blending, stencil, etc),
    * but a there's a few things we need to override:
    */
   _mesa_meta_begin(ctx, (META_RASTERIZATION |
                          META_SHADER |
                          META_TEXTURE |
                          META_TRANSFORM |
                          META_VERTEX |
                          META_VIEWPORT |
                          metaExtraSave));

   newTex = alloc_texture(tex, width, height, texIntFormat);

   /* vertex positions, texcoords (after texture allocation!) */
   {
      const GLfloat x0 = (GLfloat) x;
      const GLfloat y0 = (GLfloat) y;
      const GLfloat x1 = x + width * ctx->Pixel.ZoomX;
      const GLfloat y1 = y + height * ctx->Pixel.ZoomY;
      const GLfloat z = invert_z(ctx->Current.RasterPos[2]);

      verts[0].x = x0;
      verts[0].y = y0;
      verts[0].z = z;
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].x = x1;
      verts[1].y = y0;
      verts[1].z = z;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].x = x0;
      verts[3].y = y1;
      verts[3].z = z;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;
   }

   if (drawpix->ArrayObj == 0) {
      /* one-time setup: create vertex array object */
      _mesa_GenVertexArrays(1, &drawpix->ArrayObj);
   }
   _mesa_BindVertexArray(drawpix->ArrayObj);

   /* create vertex array buffer */
   _mesa_GenBuffersARB(1, &vbo);
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
   _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                       verts, GL_DYNAMIC_DRAW_ARB);

   /* setup vertex arrays */
   _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
   _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
   _mesa_EnableClientState(GL_VERTEX_ARRAY);
   _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);

   /* set given unpack params */
   ctx->Unpack = *unpack;

   _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   if (_mesa_is_stencil_format(format)) {
      /* Drawing stencil */
      GLint bit;

      if (!drawpix->StencilFP)
         init_draw_stencil_pixels(ctx);

      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            GL_ALPHA, type, pixels);

      _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_TRUE);

      /* set all stencil bits to 0 */
      _mesa_StencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      _mesa_StencilFunc(GL_ALWAYS, 0, 255);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
  
      /* set stencil bits to 1 where needed */
      _mesa_StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

      _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->StencilFP);
      _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);

      for (bit = 0; bit < ctx->DrawBuffer->Visual.stencilBits; bit++) {
         const GLuint mask = 1 << bit;
         if (mask & origStencilMask) {
            _mesa_StencilFunc(GL_ALWAYS, mask, mask);
            _mesa_StencilMask(mask);

            _mesa_ProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
                                             255.0 / mask, 0.5, 0.0, 0.0);

            _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
         }
      }
   }
   else if (_mesa_is_depth_format(format)) {
      /* Drawing depth */
      if (!drawpix->DepthFP)
         init_draw_depth_pixels(ctx);

      _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->DepthFP);
      _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);

      /* polygon color = current raster color */
      _mesa_ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0,
                                        ctx->Current.RasterColor);

      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            format, type, pixels);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }
   else {
      /* Drawing RGBA */
      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            format, type, pixels);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }

   _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_DeleteBuffersARB(1, &vbo);

   /* restore unpack params */
   ctx->Unpack = unpackSave;

   _mesa_meta_end(ctx);
}


/**
 * Do glBitmap with a alpha texture quad.  Use the alpha test to
 * cull the 'off' bits.  If alpha test is already enabled, fall back
 * to swrast (should be a rare case).
 * A bitmap cache as in the gallium/mesa state tracker would
 * improve performance a lot.
 */
void
_mesa_meta_Bitmap(struct gl_context *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLubyte *bitmap1)
{
   struct bitmap_state *bitmap = &ctx->Meta->Bitmap;
   struct temp_texture *tex = get_bitmap_temp_texture(ctx);
   const GLenum texIntFormat = GL_ALPHA;
   const struct gl_pixelstore_attrib unpackSave = *unpack;
   GLubyte fg, bg;
   struct vertex {
      GLfloat x, y, z, s, t, r, g, b, a;
   };
   struct vertex verts[4];
   GLboolean newTex;
   GLubyte *bitmap8;

   /*
    * Check if swrast fallback is needed.
    */
   if (ctx->_ImageTransferState ||
       ctx->Color.AlphaEnabled ||
       ctx->Fog.Enabled ||
       ctx->Texture._EnabledUnits ||
       width > tex->MaxSize ||
       height > tex->MaxSize) {
      _swrast_Bitmap(ctx, x, y, width, height, unpack, bitmap1);
      return;
   }

   /* Most GL state applies to glBitmap (like blending, stencil, etc),
    * but a there's a few things we need to override:
    */
   _mesa_meta_begin(ctx, (META_ALPHA_TEST |
                          META_PIXEL_STORE |
                          META_RASTERIZATION |
                          META_SHADER |
                          META_TEXTURE |
                          META_TRANSFORM |
                          META_VERTEX |
                          META_VIEWPORT));

   if (bitmap->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArraysAPPLE(1, &bitmap->ArrayObj);
      _mesa_BindVertexArrayAPPLE(bitmap->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &bitmap->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, bitmap->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_ColorPointer(4, GL_FLOAT, sizeof(struct vertex), OFFSET(r));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
      _mesa_EnableClientState(GL_COLOR_ARRAY);
   }
   else {
      _mesa_BindVertexArray(bitmap->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, bitmap->VBO);
   }

   newTex = alloc_texture(tex, width, height, texIntFormat);

   /* vertex positions, texcoords, colors (after texture allocation!) */
   {
      const GLfloat x0 = (GLfloat) x;
      const GLfloat y0 = (GLfloat) y;
      const GLfloat x1 = (GLfloat) (x + width);
      const GLfloat y1 = (GLfloat) (y + height);
      const GLfloat z = invert_z(ctx->Current.RasterPos[2]);
      GLuint i;

      verts[0].x = x0;
      verts[0].y = y0;
      verts[0].z = z;
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].x = x1;
      verts[1].y = y0;
      verts[1].z = z;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].x = x0;
      verts[3].y = y1;
      verts[3].z = z;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;

      for (i = 0; i < 4; i++) {
         verts[i].r = ctx->Current.RasterColor[0];
         verts[i].g = ctx->Current.RasterColor[1];
         verts[i].b = ctx->Current.RasterColor[2];
         verts[i].a = ctx->Current.RasterColor[3];
      }

      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   /* choose different foreground/background alpha values */
   CLAMPED_FLOAT_TO_UBYTE(fg, ctx->Current.RasterColor[ACOMP]);
   bg = (fg > 127 ? 0 : 255);

   bitmap1 = _mesa_map_pbo_source(ctx, &unpackSave, bitmap1);
   if (!bitmap1) {
      _mesa_meta_end(ctx);
      return;
   }

   bitmap8 = (GLubyte *) malloc(width * height);
   if (bitmap8) {
      memset(bitmap8, bg, width * height);
      _mesa_expand_bitmap(width, height, &unpackSave, bitmap1,
                          bitmap8, width, fg);

      _mesa_set_enable(ctx, tex->Target, GL_TRUE);

      _mesa_set_enable(ctx, GL_ALPHA_TEST, GL_TRUE);
      _mesa_AlphaFunc(GL_NOTEQUAL, UBYTE_TO_FLOAT(bg));

      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            GL_ALPHA, GL_UNSIGNED_BYTE, bitmap8);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

      _mesa_set_enable(ctx, tex->Target, GL_FALSE);

      free(bitmap8);
   }

   _mesa_unmap_pbo_source(ctx, &unpackSave);

   _mesa_meta_end(ctx);
}


/**
 * Check if the call to _mesa_meta_GenerateMipmap() will require a
 * software fallback.  The fallback path will require that the texture
 * images are mapped.
 * \return GL_TRUE if a fallback is needed, GL_FALSE otherwise
 */
GLboolean
_mesa_meta_check_generate_mipmap_fallback(struct gl_context *ctx, GLenum target,
                                          struct gl_texture_object *texObj)
{
   const GLuint fboSave = ctx->DrawBuffer->Name;
   struct gen_mipmap_state *mipmap = &ctx->Meta->Mipmap;
   struct gl_texture_image *baseImage;
   GLuint srcLevel;
   GLenum status;

   /* check for fallbacks */
   if (!ctx->Extensions.EXT_framebuffer_object ||
       target == GL_TEXTURE_3D) {
      return GL_TRUE;
   }

   srcLevel = texObj->BaseLevel;
   baseImage = _mesa_select_tex_image(ctx, texObj, target, srcLevel);
   if (!baseImage || _mesa_is_format_compressed(baseImage->TexFormat)) {
      return GL_TRUE;
   }

   /*
    * Test that we can actually render in the texture's format.
    */
   if (!mipmap->FBO)
      _mesa_GenFramebuffersEXT(1, &mipmap->FBO);
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, mipmap->FBO);

   if (target == GL_TEXTURE_1D) {
      _mesa_FramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel);
   }
#if 0
   /* other work is needed to enable 3D mipmap generation */
   else if (target == GL_TEXTURE_3D) {
      GLint zoffset = 0;
      _mesa_FramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel, zoffset);
   }
#endif
   else {
      /* 2D / cube */
      _mesa_FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel);
   }

   status = _mesa_CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboSave);

   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Called via ctx->Driver.GenerateMipmap()
 * Note: texture borders and 3D texture support not yet complete.
 */
void
_mesa_meta_GenerateMipmap(struct gl_context *ctx, GLenum target,
                          struct gl_texture_object *texObj)
{
   struct gen_mipmap_state *mipmap = &ctx->Meta->Mipmap;
   struct vertex {
      GLfloat x, y, s, t, r;
   };
   struct vertex verts[4];
   const GLuint baseLevel = texObj->BaseLevel;
   const GLuint maxLevel = texObj->MaxLevel;
   const GLenum minFilterSave = texObj->MinFilter;
   const GLenum magFilterSave = texObj->MagFilter;
   const GLint baseLevelSave = texObj->BaseLevel;
   const GLint maxLevelSave = texObj->MaxLevel;
   const GLboolean genMipmapSave = texObj->GenerateMipmap;
   const GLenum wrapSSave = texObj->WrapS;
   const GLenum wrapTSave = texObj->WrapT;
   const GLenum wrapRSave = texObj->WrapR;
   const GLuint fboSave = ctx->DrawBuffer->Name;
   const GLuint original_active_unit = ctx->Texture.CurrentUnit;
   GLenum faceTarget;
   GLuint dstLevel;
   GLuint border = 0;

   if (_mesa_meta_check_generate_mipmap_fallback(ctx, target, texObj)) {
      _mesa_generate_mipmap(ctx, target, texObj);
      return;
   }

   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
      faceTarget = target;
      target = GL_TEXTURE_CUBE_MAP;
   }
   else {
      faceTarget = target;
   }

   _mesa_meta_begin(ctx, META_ALL);

   if (original_active_unit != 0)
      _mesa_BindTexture(target, texObj->Name);

   if (mipmap->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArraysAPPLE(1, &mipmap->ArrayObj);
      _mesa_BindVertexArrayAPPLE(mipmap->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &mipmap->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, mipmap->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(mipmap->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, mipmap->VBO);
   }

   if (!mipmap->FBO) {
      _mesa_GenFramebuffersEXT(1, &mipmap->FBO);
   }
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, mipmap->FBO);

   _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   _mesa_TexParameteri(target, GL_GENERATE_MIPMAP, GL_FALSE);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   _mesa_set_enable(ctx, target, GL_TRUE);

   /* setup texcoords once (XXX what about border?) */
   switch (faceTarget) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[0].r = 0.0F;
      verts[1].s = 1.0F;
      verts[1].t = 0.0F;
      verts[1].r = 0.0F;
      verts[2].s = 1.0F;
      verts[2].t = 1.0F;
      verts[2].r = 0.0F;
      verts[3].s = 0.0F;
      verts[3].t = 1.0F;
      verts[3].r = 0.0F;
      break;
   case GL_TEXTURE_3D:
      abort();
      break;
   default:
      /* cube face */
      {
         static const GLfloat st[4][2] = {
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
         };
         GLuint i;

         /* loop over quad verts */
         for (i = 0; i < 4; i++) {
            /* Compute sc = +/-scale and tc = +/-scale.
             * Not +/-1 to avoid cube face selection ambiguity near the edges,
             * though that can still sometimes happen with this scale factor...
             */
            const GLfloat scale = 0.9999f;
            const GLfloat sc = (2.0f * st[i][0] - 1.0f) * scale;
            const GLfloat tc = (2.0f * st[i][1] - 1.0f) * scale;

            switch (faceTarget) {
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
               verts[i].s = 1.0f;
               verts[i].t = -tc;
               verts[i].r = -sc;
               break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
               verts[i].s = -1.0f;
               verts[i].t = -tc;
               verts[i].r = sc;
               break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
               verts[i].s = sc;
               verts[i].t = 1.0f;
               verts[i].r = tc;
               break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
               verts[i].s = sc;
               verts[i].t = -1.0f;
               verts[i].r = -tc;
               break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
               verts[i].s = sc;
               verts[i].t = -tc;
               verts[i].r = 1.0f;
               break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
               verts[i].s = -sc;
               verts[i].t = -tc;
               verts[i].r = -1.0f;
               break;
            default:
               assert(0);
            }
         }
      }
   }

   _mesa_set_enable(ctx, target, GL_TRUE);

   /* setup vertex positions */
   {
      verts[0].x = 0.0F;
      verts[0].y = 0.0F;
      verts[1].x = 1.0F;
      verts[1].y = 0.0F;
      verts[2].x = 1.0F;
      verts[2].y = 1.0F;
      verts[3].x = 0.0F;
      verts[3].y = 1.0F;
      
      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   /* setup projection matrix */
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_LoadIdentity();
   _mesa_Ortho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

   /* texture is already locked, unlock now */
   _mesa_unlock_texture(ctx, texObj);

   for (dstLevel = baseLevel + 1; dstLevel <= maxLevel; dstLevel++) {
      const struct gl_texture_image *srcImage;
      const GLuint srcLevel = dstLevel - 1;
      GLsizei srcWidth, srcHeight, srcDepth;
      GLsizei dstWidth, dstHeight, dstDepth;
      GLenum status;

      srcImage = _mesa_select_tex_image(ctx, texObj, faceTarget, srcLevel);
      assert(srcImage->Border == 0); /* XXX we can fix this */

      /* src size w/out border */
      srcWidth = srcImage->Width - 2 * border;
      srcHeight = srcImage->Height - 2 * border;
      srcDepth = srcImage->Depth - 2 * border;

      /* new dst size w/ border */
      dstWidth = MAX2(1, srcWidth / 2) + 2 * border;
      dstHeight = MAX2(1, srcHeight / 2) + 2 * border;
      dstDepth = MAX2(1, srcDepth / 2) + 2 * border;

      if (dstWidth == srcImage->Width &&
          dstHeight == srcImage->Height &&
          dstDepth == srcImage->Depth) {
         /* all done */
         break;
      }

      /* Set MaxLevel large enough to hold the new level when we allocate it  */
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, dstLevel);

      /* Create empty dest image */
      if (target == GL_TEXTURE_1D) {
         _mesa_TexImage1D(target, dstLevel, srcImage->InternalFormat,
                          dstWidth, border,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      }
      else if (target == GL_TEXTURE_3D) {
         _mesa_TexImage3D(target, dstLevel, srcImage->InternalFormat,
                          dstWidth, dstHeight, dstDepth, border,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      }
      else {
         /* 2D or cube */
         _mesa_TexImage2D(faceTarget, dstLevel, srcImage->InternalFormat,
                          dstWidth, dstHeight, border,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL);

         if (target == GL_TEXTURE_CUBE_MAP) {
            /* If texturing from a cube, we need to make sure all src faces
             * have been defined (even if we're not sampling from them.)
             * Otherwise the texture object will be 'incomplete' and
             * texturing from it will not be allowed.
             */
            GLuint face;
            for (face = 0; face < 6; face++) {
               if (!texObj->Image[face][srcLevel] ||
                   texObj->Image[face][srcLevel]->Width != srcWidth) {
                  _mesa_TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                   srcLevel, srcImage->InternalFormat,
                                   srcWidth, srcHeight, border,
                                   GL_RGBA, GL_UNSIGNED_BYTE, NULL);
               }
            }
         }
      }

      /* limit sampling to src level */
      _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, srcLevel);
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, srcLevel);

      /* Set to draw into the current dstLevel */
      if (target == GL_TEXTURE_1D) {
         _mesa_FramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       target,
                                       texObj->Name,
                                       dstLevel);
      }
      else if (target == GL_TEXTURE_3D) {
         GLint zoffset = 0; /* XXX unfinished */
         _mesa_FramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       target,
                                       texObj->Name,
                                       dstLevel, zoffset);
      }
      else {
         /* 2D / cube */
         _mesa_FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       faceTarget,
                                       texObj->Name,
                                       dstLevel);
      }

      _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

      /* sanity check */
      status = _mesa_CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         abort();
         break;
      }

      assert(dstWidth == ctx->DrawBuffer->Width);
      assert(dstHeight == ctx->DrawBuffer->Height);

      /* setup viewport */
      _mesa_set_viewport(ctx, 0, 0, dstWidth, dstHeight);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }

   _mesa_lock_texture(ctx, texObj); /* relock */

   _mesa_meta_end(ctx);

   _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterSave);
   _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilterSave);
   _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, baseLevelSave);
   _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
   _mesa_TexParameteri(target, GL_GENERATE_MIPMAP, genMipmapSave);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, wrapSSave);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, wrapTSave);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_R, wrapRSave);

   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboSave);
}


/**
 * Determine the GL data type to use for the temporary image read with
 * ReadPixels() and passed to Tex[Sub]Image().
 */
static GLenum
get_temp_image_type(struct gl_context *ctx, GLenum baseFormat)
{
   switch (baseFormat) {
   case GL_RGBA:
   case GL_RGB:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
      if (ctx->DrawBuffer->Visual.redBits <= 8)
         return GL_UNSIGNED_BYTE;
      else if (ctx->DrawBuffer->Visual.redBits <= 8)
         return GL_UNSIGNED_SHORT;
      else
         return GL_FLOAT;
   case GL_DEPTH_COMPONENT:
      return GL_UNSIGNED_INT;
   case GL_DEPTH_STENCIL:
      return GL_UNSIGNED_INT_24_8;
   default:
      _mesa_problem(ctx, "Unexpected format in get_temp_image_type()");
      return 0;
   }
}


/**
 * Helper for _mesa_meta_CopyTexImage1/2D() functions.
 * Have to be careful with locking and meta state for pixel transfer.
 */
static void
copy_tex_image(struct gl_context *ctx, GLuint dims, GLenum target, GLint level,
               GLenum internalFormat, GLint x, GLint y,
               GLsizei width, GLsizei height, GLint border)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   GLenum format, type;
   GLint bpp;
   void *buf;

   texObj = _mesa_get_current_tex_object(ctx, target);
   texImage = _mesa_get_tex_image(ctx, texObj, target, level);

   format = _mesa_base_tex_format(ctx, internalFormat);
   type = get_temp_image_type(ctx, format);
   bpp = _mesa_bytes_per_pixel(format, type);
   if (bpp <= 0) {
      _mesa_problem(ctx, "Bad bpp in meta copy_tex_image()");
      return;
   }

   /*
    * Alloc image buffer (XXX could use a PBO)
    */
   buf = malloc(width * height * bpp);
   if (!buf) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage%uD", dims);
      return;
   }

   _mesa_unlock_texture(ctx, texObj); /* need to unlock first */

   /*
    * Read image from framebuffer (disable pixel transfer ops)
    */
   _mesa_meta_begin(ctx, META_PIXEL_STORE | META_PIXEL_TRANSFER);
   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, buf);
   _mesa_meta_end(ctx);

   if (texImage->Data) {
      ctx->Driver.FreeTexImageData(ctx, texImage);
   }

   _mesa_init_teximage_fields(ctx, target, texImage,
                              postConvWidth, postConvHeight, 1,
                              border, internalFormat);

   _mesa_choose_texture_format(ctx, texObj, texImage, target, level,
                               internalFormat, GL_NONE, GL_NONE);

   /*
    * Store texture data (with pixel transfer ops)
    */
   _mesa_meta_begin(ctx, META_PIXEL_STORE);

   _mesa_update_state(ctx); /* to update pixel transfer state */

   if (target == GL_TEXTURE_1D) {
      ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                             width, border, format, type,
                             buf, &ctx->Unpack, texObj, texImage);
   }
   else {
      ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                             width, height, border, format, type,
                             buf, &ctx->Unpack, texObj, texImage);
   }
   _mesa_meta_end(ctx);

   _mesa_lock_texture(ctx, texObj); /* re-lock */

   free(buf);
}


void
_mesa_meta_CopyTexImage1D(struct gl_context *ctx, GLenum target, GLint level,
                          GLenum internalFormat, GLint x, GLint y,
                          GLsizei width, GLint border)
{
   copy_tex_image(ctx, 1, target, level, internalFormat, x, y,
                  width, 1, border);
}


void
_mesa_meta_CopyTexImage2D(struct gl_context *ctx, GLenum target, GLint level,
                          GLenum internalFormat, GLint x, GLint y,
                          GLsizei width, GLsizei height, GLint border)
{
   copy_tex_image(ctx, 2, target, level, internalFormat, x, y,
                  width, height, border);
}



/**
 * Helper for _mesa_meta_CopyTexSubImage1/2/3D() functions.
 * Have to be careful with locking and meta state for pixel transfer.
 */
static void
copy_tex_sub_image(struct gl_context *ctx, GLuint dims, GLenum target, GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLenum format, type;
   GLint bpp;
   void *buf;

   texObj = _mesa_get_current_tex_object(ctx, target);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   format = _mesa_get_format_base_format(texImage->TexFormat);
   type = get_temp_image_type(ctx, format);
   bpp = _mesa_bytes_per_pixel(format, type);
   if (bpp <= 0) {
      _mesa_problem(ctx, "Bad bpp in meta copy_tex_sub_image()");
      return;
   }

   /*
    * Alloc image buffer (XXX could use a PBO)
    */
   buf = malloc(width * height * bpp);
   if (!buf) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage%uD", dims);
      return;
   }

   _mesa_unlock_texture(ctx, texObj); /* need to unlock first */

   /*
    * Read image from framebuffer (disable pixel transfer ops)
    */
   _mesa_meta_begin(ctx, META_PIXEL_STORE | META_PIXEL_TRANSFER);
   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, buf);
   _mesa_meta_end(ctx);

   _mesa_update_state(ctx); /* to update pixel transfer state */

   /*
    * Store texture data (with pixel transfer ops)
    */
   _mesa_meta_begin(ctx, META_PIXEL_STORE);
   if (target == GL_TEXTURE_1D) {
      ctx->Driver.TexSubImage1D(ctx, target, level, xoffset,
                                width, format, type, buf,
                                &ctx->Unpack, texObj, texImage);
   }
   else if (target == GL_TEXTURE_3D) {
      ctx->Driver.TexSubImage3D(ctx, target, level, xoffset, yoffset, zoffset,
                                width, height, 1, format, type, buf,
                                &ctx->Unpack, texObj, texImage);
   }
   else {
      ctx->Driver.TexSubImage2D(ctx, target, level, xoffset, yoffset,
                                width, height, format, type, buf,
                                &ctx->Unpack, texObj, texImage);
   }
   _mesa_meta_end(ctx);

   _mesa_lock_texture(ctx, texObj); /* re-lock */

   free(buf);
}


void
_mesa_meta_CopyTexSubImage1D(struct gl_context *ctx, GLenum target, GLint level,
                             GLint xoffset,
                             GLint x, GLint y, GLsizei width)
{
   copy_tex_sub_image(ctx, 1, target, level, xoffset, 0, 0,
                      x, y, width, 1);
}


void
_mesa_meta_CopyTexSubImage2D(struct gl_context *ctx, GLenum target, GLint level,
                             GLint xoffset, GLint yoffset,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height)
{
   copy_tex_sub_image(ctx, 2, target, level, xoffset, yoffset, 0,
                      x, y, width, height);
}


void
_mesa_meta_CopyTexSubImage3D(struct gl_context *ctx, GLenum target, GLint level,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height)
{
   copy_tex_sub_image(ctx, 3, target, level, xoffset, yoffset, zoffset,
                      x, y, width, height);
}


void
_mesa_meta_CopyColorTable(struct gl_context *ctx,
                          GLenum target, GLenum internalformat,
                          GLint x, GLint y, GLsizei width)
{
   GLfloat *buf;

   buf = (GLfloat *) malloc(width * 4 * sizeof(GLfloat));
   if (!buf) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyColorTable");
      return;
   }

   /*
    * Read image from framebuffer (disable pixel transfer ops)
    */
   _mesa_meta_begin(ctx, META_PIXEL_STORE | META_PIXEL_TRANSFER);
   ctx->Driver.ReadPixels(ctx, x, y, width, 1,
                          GL_RGBA, GL_FLOAT, &ctx->Pack, buf);

   _mesa_ColorTable(target, internalformat, width, GL_RGBA, GL_FLOAT, buf);

   _mesa_meta_end(ctx);

   free(buf);
}


void
_mesa_meta_CopyColorSubTable(struct gl_context *ctx,GLenum target, GLsizei start,
                             GLint x, GLint y, GLsizei width)
{
   GLfloat *buf;

   buf = (GLfloat *) malloc(width * 4 * sizeof(GLfloat));
   if (!buf) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyColorSubTable");
      return;
   }

   /*
    * Read image from framebuffer (disable pixel transfer ops)
    */
   _mesa_meta_begin(ctx, META_PIXEL_STORE | META_PIXEL_TRANSFER);
   ctx->Driver.ReadPixels(ctx, x, y, width, 1,
                          GL_RGBA, GL_FLOAT, &ctx->Pack, buf);

   _mesa_ColorSubTable(target, start, width, GL_RGBA, GL_FLOAT, buf);

   _mesa_meta_end(ctx);

   free(buf);
}
