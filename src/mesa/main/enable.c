/* $Id: enable.c,v 1.67 2002/06/18 16:53:46 brianp Exp $ */

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
#include "context.h"
#include "enable.h"
#include "light.h"
#include "macros.h"
#include "mmath.h"
#include "simple_list.h"
#include "mtypes.h"
#include "enums.h"
#include "math/m_matrix.h"
#include "math/m_xform.h"
#endif


#define CHECK_EXTENSION(EXTNAME, CAP)					\
   if (!ctx->Extensions.EXTNAME) {					\
      char s[100];							\
      sprintf(s, "gl%sClientState(0x%x)", state ? "Enable" : "Disable", CAP);\
      _mesa_error(ctx, GL_INVALID_ENUM,	s);				\
      return;								\
   }



static void
client_state( GLcontext *ctx, GLenum cap, GLboolean state )
{
   GLuint flag;
   GLuint *var;

   switch (cap) {
      case GL_VERTEX_ARRAY:
         var = &ctx->Array.Vertex.Enabled;
         flag = _NEW_ARRAY_VERTEX;
         break;
      case GL_NORMAL_ARRAY:
         var = &ctx->Array.Normal.Enabled;
         flag = _NEW_ARRAY_NORMAL;
         break;
      case GL_COLOR_ARRAY:
         var = &ctx->Array.Color.Enabled;
         flag = _NEW_ARRAY_COLOR0;
         break;
      case GL_INDEX_ARRAY:
         var = &ctx->Array.Index.Enabled;
         flag = _NEW_ARRAY_INDEX;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         var = &ctx->Array.TexCoord[ctx->Array.ActiveTexture].Enabled;
         flag = _NEW_ARRAY_TEXCOORD(ctx->Array.ActiveTexture);
         break;
      case GL_EDGE_FLAG_ARRAY:
         var = &ctx->Array.EdgeFlag.Enabled;
         flag = _NEW_ARRAY_EDGEFLAG;
         break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         var = &ctx->Array.FogCoord.Enabled;
         flag = _NEW_ARRAY_FOGCOORD;
         break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         var = &ctx->Array.SecondaryColor.Enabled;
         flag = _NEW_ARRAY_COLOR1;
         break;

      /* GL_NV_vertex_program */
      case GL_VERTEX_ATTRIB_ARRAY0_NV:
      case GL_VERTEX_ATTRIB_ARRAY1_NV:
      case GL_VERTEX_ATTRIB_ARRAY2_NV:
      case GL_VERTEX_ATTRIB_ARRAY3_NV:
      case GL_VERTEX_ATTRIB_ARRAY4_NV:
      case GL_VERTEX_ATTRIB_ARRAY5_NV:
      case GL_VERTEX_ATTRIB_ARRAY6_NV:
      case GL_VERTEX_ATTRIB_ARRAY7_NV:
      case GL_VERTEX_ATTRIB_ARRAY8_NV:
      case GL_VERTEX_ATTRIB_ARRAY9_NV:
      case GL_VERTEX_ATTRIB_ARRAY10_NV:
      case GL_VERTEX_ATTRIB_ARRAY11_NV:
      case GL_VERTEX_ATTRIB_ARRAY12_NV:
      case GL_VERTEX_ATTRIB_ARRAY13_NV:
      case GL_VERTEX_ATTRIB_ARRAY14_NV:
      case GL_VERTEX_ATTRIB_ARRAY15_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         {
            GLint n = (GLint) cap - GL_VERTEX_ATTRIB_ARRAY0_NV;
            var = &ctx->Array.VertexAttrib[n].Enabled;
            flag = _NEW_ARRAY_ATTRIB(n);
         }
         break;
      default:
         {
            char s[100];
            sprintf(s, "glEnable/DisableClientState(0x%x)", cap);
            _mesa_error( ctx, GL_INVALID_ENUM, s);
         }
         return;
   }

   if (*var == state)
      return;

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.NewState |= flag;
   *var = state;

   if (state)
      ctx->Array._Enabled |= flag;
   else
      ctx->Array._Enabled &= ~flag;

   if (ctx->Driver.Enable) {
      (*ctx->Driver.Enable)( ctx, cap, state );
   }
}



void
_mesa_EnableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   client_state( ctx, cap, GL_TRUE );
}



void
_mesa_DisableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   client_state( ctx, cap, GL_FALSE );
}


#undef CHECK_EXTENSION
#define CHECK_EXTENSION(EXTNAME, CAP)					\
   if (!ctx->Extensions.EXTNAME) {					\
      char s[100];							\
      sprintf(s, "gl%s(0x%x)", state ? "Enable" : "Disable", CAP);	\
      _mesa_error(ctx, GL_INVALID_ENUM, s);				\
      return;								\
   }


/*
 * Perform glEnable and glDisable calls.
 */
void _mesa_set_enable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "%s %s (newstate is %x)\n",
                  state ? "glEnable" : "glDisable",
                  _mesa_lookup_enum_by_nr(cap),
                  ctx->NewState);

   switch (cap) {
      case GL_ALPHA_TEST:
         if (ctx->Color.AlphaEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.AlphaEnabled = state;
         break;
      case GL_AUTO_NORMAL:
         if (ctx->Eval.AutoNormal == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.AutoNormal = state;
         break;
      case GL_BLEND:
         if (ctx->Color.BlendEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.BlendEnabled = state;
         /* The following needed to accomodate 1.0 RGB logic op blending */
         ctx->Color.ColorLogicOpEnabled =
            (ctx->Color.BlendEquation == GL_LOGIC_OP && state);
         break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
         {
            const GLuint p = cap - GL_CLIP_PLANE0;

            if ((ctx->Transform.ClipPlanesEnabled & (1 << p)) == ((GLuint) state << p))
               return;

            FLUSH_VERTICES(ctx, _NEW_TRANSFORM);

            if (state) {
               ctx->Transform.ClipPlanesEnabled |= (1 << p);

               if (ctx->ProjectionMatrixStack.Top->flags & MAT_DIRTY)
                  _math_matrix_analyse( ctx->ProjectionMatrixStack.Top );

               /* This derived state also calculated in clip.c and
                * from _mesa_update_state() on changes to EyeUserPlane
                * and ctx->ProjectionMatrix respectively.
                */
               _mesa_transform_vector( ctx->Transform._ClipUserPlane[p],
                                    ctx->Transform.EyeUserPlane[p],
                                    ctx->ProjectionMatrixStack.Top->inv );
            }
            else {
               ctx->Transform.ClipPlanesEnabled &= ~(1 << p);
            }               
         }
         break;
      case GL_COLOR_MATERIAL:
         if (ctx->Light.ColorMaterialEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT);
         ctx->Light.ColorMaterialEnabled = state;
         if (state) {
            FLUSH_CURRENT(ctx, 0);
            _mesa_update_color_material( ctx,
                                  ctx->Current.Attrib[VERT_ATTRIB_COLOR0] );
         }
         break;
      case GL_CULL_FACE:
         if (ctx->Polygon.CullFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.CullFlag = state;
         break;
      case GL_DEPTH_TEST:
         if (state && ctx->Visual.depthBits==0) {
            _mesa_warning(ctx,"glEnable(GL_DEPTH_TEST) but no depth buffer");
            return;
         }
         if (ctx->Depth.Test==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_DEPTH);
         ctx->Depth.Test = state;
         break;
      case GL_DITHER:
         if (ctx->NoDither) {
            state = GL_FALSE; /* MESA_NO_DITHER env var */
         }
         if (ctx->Color.DitherFlag==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.DitherFlag = state;
         break;
      case GL_FOG:
         if (ctx->Fog.Enabled==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.Enabled = state;
         break;
      case GL_HISTOGRAM:
         CHECK_EXTENSION(EXT_histogram, cap);
         if (ctx->Pixel.HistogramEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.HistogramEnabled = state;
         break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
         if (ctx->Light.Light[cap-GL_LIGHT0].Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT);
         ctx->Light.Light[cap-GL_LIGHT0].Enabled = state;
         if (state) {
            insert_at_tail(&ctx->Light.EnabledList,
                           &ctx->Light.Light[cap-GL_LIGHT0]);
         }
         else {
            remove_from_list(&ctx->Light.Light[cap-GL_LIGHT0]);
         }
         break;
      case GL_LIGHTING:
         if (ctx->Light.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT);
         ctx->Light.Enabled = state;

         if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
   	   ctx->_TriangleCaps |= DD_TRI_LIGHT_TWOSIDE;
         else
 	   ctx->_TriangleCaps &= ~DD_TRI_LIGHT_TWOSIDE;
 
         if ((ctx->Light.Enabled &&
              ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR)
             || ctx->Fog.ColorSumEnabled)
            ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
         else
            ctx->_TriangleCaps &= ~DD_SEPARATE_SPECULAR;

         break;
      case GL_LINE_SMOOTH:
         if (ctx->Line.SmoothFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LINE);
         ctx->Line.SmoothFlag = state;
         ctx->_TriangleCaps ^= DD_LINE_SMOOTH;
         break;
      case GL_LINE_STIPPLE:
         if (ctx->Line.StippleFlag == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_LINE);
         ctx->Line.StippleFlag = state;
         ctx->_TriangleCaps ^= DD_LINE_STIPPLE;
         break;
      case GL_INDEX_LOGIC_OP:
         if (ctx->Color.IndexLogicOpEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.IndexLogicOpEnabled = state;
         break;
      case GL_COLOR_LOGIC_OP:
         if (ctx->Color.ColorLogicOpEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_COLOR);
         ctx->Color.ColorLogicOpEnabled = state;
         break;
      case GL_MAP1_COLOR_4:
         if (ctx->Eval.Map1Color4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Color4 = state;
         break;
      case GL_MAP1_INDEX:
         if (ctx->Eval.Map1Index == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Index = state;
         break;
      case GL_MAP1_NORMAL:
         if (ctx->Eval.Map1Normal == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Normal = state;
         break;
      case GL_MAP1_TEXTURE_COORD_1:
         if (ctx->Eval.Map1TextureCoord1 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord1 = state;
         break;
      case GL_MAP1_TEXTURE_COORD_2:
         if (ctx->Eval.Map1TextureCoord2 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord2 = state;
         break;
      case GL_MAP1_TEXTURE_COORD_3:
         if (ctx->Eval.Map1TextureCoord3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord3 = state;
         break;
      case GL_MAP1_TEXTURE_COORD_4:
         if (ctx->Eval.Map1TextureCoord4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1TextureCoord4 = state;
         break;
      case GL_MAP1_VERTEX_3:
         if (ctx->Eval.Map1Vertex3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Vertex3 = state;
         break;
      case GL_MAP1_VERTEX_4:
         if (ctx->Eval.Map1Vertex4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map1Vertex4 = state;
         break;
      case GL_MAP2_COLOR_4:
         if (ctx->Eval.Map2Color4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Color4 = state;
         break;
      case GL_MAP2_INDEX:
         if (ctx->Eval.Map2Index == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Index = state;
         break;
      case GL_MAP2_NORMAL:
         if (ctx->Eval.Map2Normal == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Normal = state;
         break;
      case GL_MAP2_TEXTURE_COORD_1:
         if (ctx->Eval.Map2TextureCoord1 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord1 = state;
         break;
      case GL_MAP2_TEXTURE_COORD_2:
         if (ctx->Eval.Map2TextureCoord2 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord2 = state;
         break;
      case GL_MAP2_TEXTURE_COORD_3:
         if (ctx->Eval.Map2TextureCoord3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord3 = state;
         break;
      case GL_MAP2_TEXTURE_COORD_4:
         if (ctx->Eval.Map2TextureCoord4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2TextureCoord4 = state;
         break;
      case GL_MAP2_VERTEX_3:
         if (ctx->Eval.Map2Vertex3 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Vertex3 = state;
         break;
      case GL_MAP2_VERTEX_4:
         if (ctx->Eval.Map2Vertex4 == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_EVAL);
         ctx->Eval.Map2Vertex4 = state;
         break;
      case GL_MINMAX:
         if (ctx->Pixel.MinMaxEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.MinMaxEnabled = state;
         break;
      case GL_NORMALIZE:
         if (ctx->Transform.Normalize == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         ctx->Transform.Normalize = state;
         break;
      case GL_POINT_SMOOTH:
         if (ctx->Point.SmoothFlag==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.SmoothFlag = state;
         ctx->_TriangleCaps ^= DD_POINT_SMOOTH;
         break;
      case GL_POLYGON_SMOOTH:
         if (ctx->Polygon.SmoothFlag==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.SmoothFlag = state;
         ctx->_TriangleCaps ^= DD_TRI_SMOOTH;
         break;
      case GL_POLYGON_STIPPLE:
         if (ctx->Polygon.StippleFlag==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.StippleFlag = state;
         ctx->_TriangleCaps ^= DD_TRI_STIPPLE;
         break;
      case GL_POLYGON_OFFSET_POINT:
         if (ctx->Polygon.OffsetPoint==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.OffsetPoint = state;
         break;
      case GL_POLYGON_OFFSET_LINE:
         if (ctx->Polygon.OffsetLine==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.OffsetLine = state;
         break;
      case GL_POLYGON_OFFSET_FILL:
         /*case GL_POLYGON_OFFSET_EXT:*/
         if (ctx->Polygon.OffsetFill==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POLYGON);
         ctx->Polygon.OffsetFill = state;
         break;
      case GL_RESCALE_NORMAL_EXT:
         if (ctx->Transform.RescaleNormals == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         ctx->Transform.RescaleNormals = state;
         break;
      case GL_SCISSOR_TEST:
         if (ctx->Scissor.Enabled==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_SCISSOR);
         ctx->Scissor.Enabled = state;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         if (ctx->Texture.SharedPalette == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         ctx->Texture.SharedPalette = state;
         break;
      case GL_STENCIL_TEST:
         if (state && ctx->Visual.stencilBits==0) {
            _mesa_warning(ctx,
                          "glEnable(GL_STENCIL_TEST) but no stencil buffer");
            return;
         }
         if (ctx->Stencil.Enabled==state)
            return;
         FLUSH_VERTICES(ctx, _NEW_STENCIL);
         ctx->Stencil.Enabled = state;
         break;
      case GL_TEXTURE_1D: {
         const GLuint curr = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
         GLuint newenabled = texUnit->Enabled & ~TEXTURE_1D_BIT;
         if (state)
            newenabled |= TEXTURE_1D_BIT;
         if (!ctx->Visual.rgbMode || texUnit->Enabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->Enabled = newenabled;
         break;
      }
      case GL_TEXTURE_2D: {
         const GLuint curr = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
         GLuint newenabled = texUnit->Enabled & ~TEXTURE_2D_BIT;
         if (state)
            newenabled |= TEXTURE_2D_BIT;
         if (!ctx->Visual.rgbMode || texUnit->Enabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->Enabled = newenabled;
         break;
      }
      case GL_TEXTURE_3D: {
         const GLuint curr = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
         GLuint newenabled = texUnit->Enabled & ~TEXTURE_3D_BIT;
         if (state)
            newenabled |= TEXTURE_3D_BIT;
         if (!ctx->Visual.rgbMode || texUnit->Enabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->Enabled = newenabled;
         break;
      }
      case GL_TEXTURE_GEN_Q: {
         GLuint unit = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         GLuint newenabled = texUnit->TexGenEnabled & ~Q_BIT;
         if (state)
            newenabled |= Q_BIT;
         if (texUnit->TexGenEnabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->TexGenEnabled = newenabled;
         break;
      }
      case GL_TEXTURE_GEN_R: {
         GLuint unit = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         GLuint newenabled = texUnit->TexGenEnabled & ~R_BIT;
         if (state)
            newenabled |= R_BIT;
         if (texUnit->TexGenEnabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->TexGenEnabled = newenabled;
         break;
      }
      break;
      case GL_TEXTURE_GEN_S: {
         GLuint unit = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         GLuint newenabled = texUnit->TexGenEnabled & ~S_BIT;
         if (state)
            newenabled |= S_BIT;
         if (texUnit->TexGenEnabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->TexGenEnabled = newenabled;
         break;
      }
      break;
      case GL_TEXTURE_GEN_T: {
         GLuint unit = ctx->Texture.CurrentUnit;
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         GLuint newenabled = texUnit->TexGenEnabled & ~T_BIT;
         if (state)
            newenabled |= T_BIT;
         if (texUnit->TexGenEnabled == newenabled)
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->TexGenEnabled = newenabled;
         break;
      }
      break;

      /*
       * CLIENT STATE!!!
       */
      case GL_VERTEX_ARRAY:
      case GL_NORMAL_ARRAY:
      case GL_COLOR_ARRAY:
      case GL_INDEX_ARRAY:
      case GL_TEXTURE_COORD_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
      case GL_FOG_COORDINATE_ARRAY_EXT:
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         client_state( ctx, cap, state );
         return;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         CHECK_EXTENSION(HP_occlusion_test, cap);
         if (ctx->Depth.OcclusionTest == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_DEPTH);
         ctx->Depth.OcclusionTest = state;
         if (state)
            ctx->OcclusionResult = ctx->OcclusionResultSaved;
         else
            ctx->OcclusionResultSaved = ctx->OcclusionResult;
         break;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         CHECK_EXTENSION(SGIS_pixel_texture, cap);
         if (ctx->Pixel.PixelTextureEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PixelTextureEnabled = state;
         break;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         CHECK_EXTENSION(SGIX_pixel_texture, cap);
         if (ctx->Pixel.PixelTextureEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PixelTextureEnabled = state;
         break;

      /* GL_SGI_color_table */
      case GL_COLOR_TABLE_SGI:
         CHECK_EXTENSION(SGI_color_table, cap);
         if (ctx->Pixel.ColorTableEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.ColorTableEnabled = state;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         CHECK_EXTENSION(SGI_color_table, cap);
         if (ctx->Pixel.PostConvolutionColorTableEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionColorTableEnabled = state;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         CHECK_EXTENSION(SGI_color_table, cap);
         if (ctx->Pixel.PostColorMatrixColorTableEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixColorTableEnabled = state;
         break;

      /* GL_EXT_convolution */
      case GL_CONVOLUTION_1D:
         CHECK_EXTENSION(EXT_convolution, cap);
         if (ctx->Pixel.Convolution1DEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.Convolution1DEnabled = state;
         break;
      case GL_CONVOLUTION_2D:
         CHECK_EXTENSION(EXT_convolution, cap);
         if (ctx->Pixel.Convolution2DEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.Convolution2DEnabled = state;
         break;
      case GL_SEPARABLE_2D:
         CHECK_EXTENSION(EXT_convolution, cap);
         if (ctx->Pixel.Separable2DEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.Separable2DEnabled = state;
         break;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         {
            const GLuint curr = ctx->Texture.CurrentUnit;
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
            GLuint newenabled = texUnit->Enabled & ~TEXTURE_CUBE_BIT;
            CHECK_EXTENSION(ARB_texture_cube_map, cap);
            if (state)
               newenabled |= TEXTURE_CUBE_BIT;
            if (!ctx->Visual.rgbMode || texUnit->Enabled == newenabled)
               return;
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texUnit->Enabled = newenabled;
         }
         break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         CHECK_EXTENSION(EXT_secondary_color, cap);
         if (ctx->Fog.ColorSumEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.ColorSumEnabled = state;

         if ((ctx->Light.Enabled &&
              ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR)
             || ctx->Fog.ColorSumEnabled)
            ctx->_TriangleCaps |= DD_SEPARATE_SPECULAR;
         else
            ctx->_TriangleCaps &= ~DD_SEPARATE_SPECULAR;

         break;

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         CHECK_EXTENSION(ARB_multisample, cap);
         if (ctx->Multisample.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.Enabled = state;
         break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         CHECK_EXTENSION(ARB_multisample, cap);
         if (ctx->Multisample.SampleAlphaToCoverage == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleAlphaToCoverage = state;
         break;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         CHECK_EXTENSION(ARB_multisample, cap);
         if (ctx->Multisample.SampleAlphaToOne == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleAlphaToOne = state;
         break;
      case GL_SAMPLE_COVERAGE_ARB:
         CHECK_EXTENSION(ARB_multisample, cap);
         if (ctx->Multisample.SampleCoverage == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleCoverage = state;
         break;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         CHECK_EXTENSION(ARB_multisample, cap);
         if (ctx->Multisample.SampleCoverageInvert == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
         ctx->Multisample.SampleCoverageInvert = state;
         break;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION(IBM_rasterpos_clip, cap);
         if (ctx->Transform.RasterPositionUnclipped == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
         ctx->Transform.RasterPositionUnclipped = state;
         break;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         CHECK_EXTENSION(NV_point_sprite, cap);
         if (ctx->Point.PointSprite == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.PointSprite = state;
         break;

      /* GL_NV_vertex_program */
      case GL_VERTEX_PROGRAM_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         if (ctx->VertexProgram.Enabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM | _NEW_PROGRAM);  /* XXX OK? */
         ctx->VertexProgram.Enabled = state;
         break;
      case GL_VERTEX_PROGRAM_POINT_SIZE_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         if (ctx->VertexProgram.PointSizeEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT | _NEW_PROGRAM);
         ctx->VertexProgram.PointSizeEnabled = state;
         break;
      case GL_VERTEX_PROGRAM_TWO_SIDE_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         if (ctx->VertexProgram.TwoSideEnabled == state)
            return;
         FLUSH_VERTICES(ctx, _NEW_PROGRAM);  /* XXX OK? */
         ctx->VertexProgram.TwoSideEnabled = state;
         break;
      case GL_MAP1_VERTEX_ATTRIB0_4_NV:
      case GL_MAP1_VERTEX_ATTRIB1_4_NV:
      case GL_MAP1_VERTEX_ATTRIB2_4_NV:
      case GL_MAP1_VERTEX_ATTRIB3_4_NV:
      case GL_MAP1_VERTEX_ATTRIB4_4_NV:
      case GL_MAP1_VERTEX_ATTRIB5_4_NV:
      case GL_MAP1_VERTEX_ATTRIB6_4_NV:
      case GL_MAP1_VERTEX_ATTRIB7_4_NV:
      case GL_MAP1_VERTEX_ATTRIB8_4_NV:
      case GL_MAP1_VERTEX_ATTRIB9_4_NV:
      case GL_MAP1_VERTEX_ATTRIB10_4_NV:
      case GL_MAP1_VERTEX_ATTRIB11_4_NV:
      case GL_MAP1_VERTEX_ATTRIB12_4_NV:
      case GL_MAP1_VERTEX_ATTRIB13_4_NV:
      case GL_MAP1_VERTEX_ATTRIB14_4_NV:
      case GL_MAP1_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         {
            const GLuint map = (GLuint) (cap - GL_MAP1_VERTEX_ATTRIB0_4_NV);
            FLUSH_VERTICES(ctx, _NEW_EVAL);
            ctx->Eval.Map1Attrib[map] = state;
         }
         break;
      case GL_MAP2_VERTEX_ATTRIB0_4_NV:
      case GL_MAP2_VERTEX_ATTRIB1_4_NV:
      case GL_MAP2_VERTEX_ATTRIB2_4_NV:
      case GL_MAP2_VERTEX_ATTRIB3_4_NV:
      case GL_MAP2_VERTEX_ATTRIB4_4_NV:
      case GL_MAP2_VERTEX_ATTRIB5_4_NV:
      case GL_MAP2_VERTEX_ATTRIB6_4_NV:
      case GL_MAP2_VERTEX_ATTRIB7_4_NV:
      case GL_MAP2_VERTEX_ATTRIB8_4_NV:
      case GL_MAP2_VERTEX_ATTRIB9_4_NV:
      case GL_MAP2_VERTEX_ATTRIB10_4_NV:
      case GL_MAP2_VERTEX_ATTRIB11_4_NV:
      case GL_MAP2_VERTEX_ATTRIB12_4_NV:
      case GL_MAP2_VERTEX_ATTRIB13_4_NV:
      case GL_MAP2_VERTEX_ATTRIB14_4_NV:
      case GL_MAP2_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program, cap);
         {
            const GLuint map = (GLuint) (cap - GL_MAP2_VERTEX_ATTRIB0_4_NV);
            FLUSH_VERTICES(ctx, _NEW_EVAL);
            ctx->Eval.Map2Attrib[map] = state;
         }
         break;

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         {
            const GLuint curr = ctx->Texture.CurrentUnit;
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
            GLuint newenabled = texUnit->Enabled & ~TEXTURE_RECT_BIT;
            CHECK_EXTENSION(NV_texture_rectangle, cap);
            if (state)
               newenabled |= TEXTURE_RECT_BIT;
            if (!ctx->Visual.rgbMode || texUnit->Enabled == newenabled)
               return;
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texUnit->Enabled = newenabled;
         }
         break;

      default:
         {
            char s[100];
            sprintf(s, "%s(0x%x)", state ? "glEnable" : "glDisable", cap);
            _mesa_error(ctx, GL_INVALID_ENUM, s);
         }
         return;
   }

   if (ctx->Driver.Enable) {
      (*ctx->Driver.Enable)( ctx, cap, state );
   }
}


void
_mesa_Enable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_set_enable( ctx, cap, GL_TRUE );
}


void
_mesa_Disable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   _mesa_set_enable( ctx, cap, GL_FALSE );
}


#undef CHECK_EXTENSION
#define CHECK_EXTENSION(EXTNAME)			\
   if (!ctx->Extensions.EXTNAME) {			\
      _mesa_error(ctx, GL_INVALID_ENUM, "glIsEnabled");	\
      return GL_FALSE;					\
   }


GLboolean
_mesa_IsEnabled( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   switch (cap) {
      case GL_ALPHA_TEST:
         return ctx->Color.AlphaEnabled;
      case GL_AUTO_NORMAL:
	 return ctx->Eval.AutoNormal;
      case GL_BLEND:
         return ctx->Color.BlendEnabled;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
	 return (ctx->Transform.ClipPlanesEnabled >> (cap - GL_CLIP_PLANE0)) & 1;
      case GL_COLOR_MATERIAL:
	 return ctx->Light.ColorMaterialEnabled;
      case GL_CULL_FACE:
         return ctx->Polygon.CullFlag;
      case GL_DEPTH_TEST:
         return ctx->Depth.Test;
      case GL_DITHER:
	 return ctx->Color.DitherFlag;
      case GL_FOG:
	 return ctx->Fog.Enabled;
      case GL_LIGHTING:
         return ctx->Light.Enabled;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
         return ctx->Light.Light[cap-GL_LIGHT0].Enabled;
      case GL_LINE_SMOOTH:
	 return ctx->Line.SmoothFlag;
      case GL_LINE_STIPPLE:
	 return ctx->Line.StippleFlag;
      case GL_INDEX_LOGIC_OP:
	 return ctx->Color.IndexLogicOpEnabled;
      case GL_COLOR_LOGIC_OP:
	 return ctx->Color.ColorLogicOpEnabled;
      case GL_MAP1_COLOR_4:
	 return ctx->Eval.Map1Color4;
      case GL_MAP1_INDEX:
	 return ctx->Eval.Map1Index;
      case GL_MAP1_NORMAL:
	 return ctx->Eval.Map1Normal;
      case GL_MAP1_TEXTURE_COORD_1:
	 return ctx->Eval.Map1TextureCoord1;
      case GL_MAP1_TEXTURE_COORD_2:
	 return ctx->Eval.Map1TextureCoord2;
      case GL_MAP1_TEXTURE_COORD_3:
	 return ctx->Eval.Map1TextureCoord3;
      case GL_MAP1_TEXTURE_COORD_4:
	 return ctx->Eval.Map1TextureCoord4;
      case GL_MAP1_VERTEX_3:
	 return ctx->Eval.Map1Vertex3;
      case GL_MAP1_VERTEX_4:
	 return ctx->Eval.Map1Vertex4;
      case GL_MAP2_COLOR_4:
	 return ctx->Eval.Map2Color4;
      case GL_MAP2_INDEX:
	 return ctx->Eval.Map2Index;
      case GL_MAP2_NORMAL:
	 return ctx->Eval.Map2Normal;
      case GL_MAP2_TEXTURE_COORD_1:
	 return ctx->Eval.Map2TextureCoord1;
      case GL_MAP2_TEXTURE_COORD_2:
	 return ctx->Eval.Map2TextureCoord2;
      case GL_MAP2_TEXTURE_COORD_3:
	 return ctx->Eval.Map2TextureCoord3;
      case GL_MAP2_TEXTURE_COORD_4:
	 return ctx->Eval.Map2TextureCoord4;
      case GL_MAP2_VERTEX_3:
	 return ctx->Eval.Map2Vertex3;
      case GL_MAP2_VERTEX_4:
	 return ctx->Eval.Map2Vertex4;
      case GL_NORMALIZE:
	 return ctx->Transform.Normalize;
      case GL_POINT_SMOOTH:
	 return ctx->Point.SmoothFlag;
      case GL_POLYGON_SMOOTH:
	 return ctx->Polygon.SmoothFlag;
      case GL_POLYGON_STIPPLE:
	 return ctx->Polygon.StippleFlag;
      case GL_POLYGON_OFFSET_POINT:
	 return ctx->Polygon.OffsetPoint;
      case GL_POLYGON_OFFSET_LINE:
	 return ctx->Polygon.OffsetLine;
      case GL_POLYGON_OFFSET_FILL:
      /*case GL_POLYGON_OFFSET_EXT:*/
	 return ctx->Polygon.OffsetFill;
      case GL_RESCALE_NORMAL_EXT:
         return ctx->Transform.RescaleNormals;
      case GL_SCISSOR_TEST:
	 return ctx->Scissor.Enabled;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         return ctx->Texture.SharedPalette;
      case GL_STENCIL_TEST:
	 return ctx->Stencil.Enabled;
      case GL_TEXTURE_1D:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE_1D_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_2D:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE_2D_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_3D:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE_3D_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_Q:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & Q_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_R:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & R_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_S:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & S_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_T:
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & T_BIT) ? GL_TRUE : GL_FALSE;
         }

      /*
       * CLIENT STATE!!!
       */
      case GL_VERTEX_ARRAY:
         return (ctx->Array.Vertex.Enabled != 0);
      case GL_NORMAL_ARRAY:
         return (ctx->Array.Normal.Enabled != 0);
      case GL_COLOR_ARRAY:
         return (ctx->Array.Color.Enabled != 0);
      case GL_INDEX_ARRAY:
         return (ctx->Array.Index.Enabled != 0);
      case GL_TEXTURE_COORD_ARRAY:
         return (ctx->Array.TexCoord[ctx->Array.ActiveTexture].Enabled != 0);
      case GL_EDGE_FLAG_ARRAY:
         return (ctx->Array.EdgeFlag.Enabled != 0);
      case GL_FOG_COORDINATE_ARRAY_EXT:
         CHECK_EXTENSION(EXT_fog_coord);
         return (ctx->Array.FogCoord.Enabled != 0);
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         CHECK_EXTENSION(EXT_secondary_color);
         return (ctx->Array.SecondaryColor.Enabled != 0);

      /* GL_EXT_histogram */
      case GL_HISTOGRAM:
         CHECK_EXTENSION(EXT_histogram);
         return ctx->Pixel.HistogramEnabled;
      case GL_MINMAX:
         CHECK_EXTENSION(EXT_histogram);
         return ctx->Pixel.MinMaxEnabled;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         CHECK_EXTENSION(HP_occlusion_test);
         return ctx->Depth.OcclusionTest;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         CHECK_EXTENSION(SGIS_pixel_texture);
         return ctx->Pixel.PixelTextureEnabled;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         CHECK_EXTENSION(SGIX_pixel_texture);
         return ctx->Pixel.PixelTextureEnabled;

      /* GL_SGI_color_table */
      case GL_COLOR_TABLE_SGI:
         CHECK_EXTENSION(SGI_color_table);
         return ctx->Pixel.ColorTableEnabled;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         CHECK_EXTENSION(SGI_color_table);
         return ctx->Pixel.PostConvolutionColorTableEnabled;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         CHECK_EXTENSION(SGI_color_table);
         return ctx->Pixel.PostColorMatrixColorTableEnabled;

      /* GL_EXT_convolution */
      case GL_CONVOLUTION_1D:
         CHECK_EXTENSION(EXT_convolution);
         return ctx->Pixel.Convolution1DEnabled;
      case GL_CONVOLUTION_2D:
         CHECK_EXTENSION(EXT_convolution);
         return ctx->Pixel.Convolution2DEnabled;
      case GL_SEPARABLE_2D:
         CHECK_EXTENSION(EXT_convolution);
         return ctx->Pixel.Separable2DEnabled;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         CHECK_EXTENSION(ARB_texture_cube_map);
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE_CUBE_BIT) ? GL_TRUE : GL_FALSE;
         }

      /* GL_ARB_multisample */
      case GL_MULTISAMPLE_ARB:
         CHECK_EXTENSION(ARB_multisample);
         return ctx->Multisample.Enabled;
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         CHECK_EXTENSION(ARB_multisample);
         return ctx->Multisample.SampleAlphaToCoverage;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
         CHECK_EXTENSION(ARB_multisample);
         return ctx->Multisample.SampleAlphaToOne;
      case GL_SAMPLE_COVERAGE_ARB:
         CHECK_EXTENSION(ARB_multisample);
         return ctx->Multisample.SampleCoverage;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
         CHECK_EXTENSION(ARB_multisample);
         return ctx->Multisample.SampleCoverageInvert;

      /* GL_IBM_rasterpos_clip */
      case GL_RASTER_POSITION_UNCLIPPED_IBM:
         CHECK_EXTENSION(IBM_rasterpos_clip);
         return ctx->Transform.RasterPositionUnclipped;

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_NV:
         return ctx->Point.PointSprite;

      /* GL_NV_vertex_program */
      case GL_VERTEX_PROGRAM_NV:
         CHECK_EXTENSION(NV_vertex_program);
         return ctx->VertexProgram.Enabled;
      case GL_VERTEX_PROGRAM_POINT_SIZE_NV:
         CHECK_EXTENSION(NV_vertex_program);
         return ctx->VertexProgram.PointSizeEnabled;
      case GL_VERTEX_PROGRAM_TWO_SIDE_NV:
         CHECK_EXTENSION(NV_vertex_program);
         return ctx->VertexProgram.TwoSideEnabled;
      case GL_VERTEX_ATTRIB_ARRAY0_NV:
      case GL_VERTEX_ATTRIB_ARRAY1_NV:
      case GL_VERTEX_ATTRIB_ARRAY2_NV:
      case GL_VERTEX_ATTRIB_ARRAY3_NV:
      case GL_VERTEX_ATTRIB_ARRAY4_NV:
      case GL_VERTEX_ATTRIB_ARRAY5_NV:
      case GL_VERTEX_ATTRIB_ARRAY6_NV:
      case GL_VERTEX_ATTRIB_ARRAY7_NV:
      case GL_VERTEX_ATTRIB_ARRAY8_NV:
      case GL_VERTEX_ATTRIB_ARRAY9_NV:
      case GL_VERTEX_ATTRIB_ARRAY10_NV:
      case GL_VERTEX_ATTRIB_ARRAY11_NV:
      case GL_VERTEX_ATTRIB_ARRAY12_NV:
      case GL_VERTEX_ATTRIB_ARRAY13_NV:
      case GL_VERTEX_ATTRIB_ARRAY14_NV:
      case GL_VERTEX_ATTRIB_ARRAY15_NV:
         CHECK_EXTENSION(NV_vertex_program);
         {
            GLint n = (GLint) cap - GL_VERTEX_ATTRIB_ARRAY0_NV;
            return (ctx->Array.VertexAttrib[n].Enabled != 0);
         }
      case GL_MAP1_VERTEX_ATTRIB0_4_NV:
      case GL_MAP1_VERTEX_ATTRIB1_4_NV:
      case GL_MAP1_VERTEX_ATTRIB2_4_NV:
      case GL_MAP1_VERTEX_ATTRIB3_4_NV:
      case GL_MAP1_VERTEX_ATTRIB4_4_NV:
      case GL_MAP1_VERTEX_ATTRIB5_4_NV:
      case GL_MAP1_VERTEX_ATTRIB6_4_NV:
      case GL_MAP1_VERTEX_ATTRIB7_4_NV:
      case GL_MAP1_VERTEX_ATTRIB8_4_NV:
      case GL_MAP1_VERTEX_ATTRIB9_4_NV:
      case GL_MAP1_VERTEX_ATTRIB10_4_NV:
      case GL_MAP1_VERTEX_ATTRIB11_4_NV:
      case GL_MAP1_VERTEX_ATTRIB12_4_NV:
      case GL_MAP1_VERTEX_ATTRIB13_4_NV:
      case GL_MAP1_VERTEX_ATTRIB14_4_NV:
      case GL_MAP1_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program);
         {
            const GLuint map = (GLuint) (cap - GL_MAP1_VERTEX_ATTRIB0_4_NV);
            return ctx->Eval.Map1Attrib[map];
         }
      case GL_MAP2_VERTEX_ATTRIB0_4_NV:
      case GL_MAP2_VERTEX_ATTRIB1_4_NV:
      case GL_MAP2_VERTEX_ATTRIB2_4_NV:
      case GL_MAP2_VERTEX_ATTRIB3_4_NV:
      case GL_MAP2_VERTEX_ATTRIB4_4_NV:
      case GL_MAP2_VERTEX_ATTRIB5_4_NV:
      case GL_MAP2_VERTEX_ATTRIB6_4_NV:
      case GL_MAP2_VERTEX_ATTRIB7_4_NV:
      case GL_MAP2_VERTEX_ATTRIB8_4_NV:
      case GL_MAP2_VERTEX_ATTRIB9_4_NV:
      case GL_MAP2_VERTEX_ATTRIB10_4_NV:
      case GL_MAP2_VERTEX_ATTRIB11_4_NV:
      case GL_MAP2_VERTEX_ATTRIB12_4_NV:
      case GL_MAP2_VERTEX_ATTRIB13_4_NV:
      case GL_MAP2_VERTEX_ATTRIB14_4_NV:
      case GL_MAP2_VERTEX_ATTRIB15_4_NV:
         CHECK_EXTENSION(NV_vertex_program);
         {
            const GLuint map = (GLuint) (cap - GL_MAP2_VERTEX_ATTRIB0_4_NV);
            return ctx->Eval.Map2Attrib[map];
         }

      /* GL_NV_texture_rectangle */
      case GL_TEXTURE_RECTANGLE_NV:
         CHECK_EXTENSION(NV_texture_rectangle);
         {
            const struct gl_texture_unit *texUnit;
            texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE_RECT_BIT) ? GL_TRUE : GL_FALSE;
         }

      default:
         {
            char s[100];
            sprintf(s, "glIsEnabled(0x%x)", (int) cap);
            _mesa_error( ctx, GL_INVALID_ENUM, s );
         }
	 return GL_FALSE;
   }
}
