/* $Id: enable.c,v 1.31 2000/11/05 18:40:57 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
#include "enable.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "simple_list.h"
#include "types.h"
#include "vbfill.h"
#include "xform.h"
#include "enums.h"
#endif



/*
 * Perform glEnable and glDisable calls.
 */
void _mesa_set_enable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx, "gl_enable/disable" );

   if (MESA_VERBOSE & VERBOSE_API) 
      fprintf(stderr, "%s %s (newstate is %x)\n", 
	      state ? "glEnable" : "glDisable",
	      gl_lookup_enum_by_nr(cap),
	      ctx->NewState);

   switch (cap) {
      case GL_ALPHA_TEST:
         if (ctx->Color.AlphaEnabled!=state) {
            ctx->Color.AlphaEnabled = state;
            ctx->NewState |= _NEW_COLOR;
         }
	 break;
      case GL_AUTO_NORMAL:
	 ctx->Eval.AutoNormal = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_BLEND:
         if (ctx->Color.BlendEnabled!=state) {
            ctx->Color.BlendEnabled = state;
            /* The following needed to accomodate 1.0 RGB logic op blending */
            if (ctx->Color.BlendEquation==GL_LOGIC_OP && state) {
               ctx->Color.ColorLogicOpEnabled = GL_TRUE;
            }
            else {
               ctx->Color.ColorLogicOpEnabled = GL_FALSE;
            }
            ctx->NewState |= _NEW_COLOR;
         }
	 break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
	 if (ctx->Transform.ClipEnabled[cap-GL_CLIP_PLANE0] != state) 
	 {
	    GLuint p = cap-GL_CLIP_PLANE0;

	    ctx->Transform.ClipEnabled[p] = state;
	    ctx->NewState |= _NEW_TRANSFORM;

	    if (state) {
	       ctx->_Enabled |= ENABLE_USERCLIP;
	       ctx->Transform._AnyClip++;
	       
	       if (ctx->ProjectionMatrix.flags & MAT_DIRTY_ALL_OVER) {
		  gl_matrix_analyze( &ctx->ProjectionMatrix );
	       }
	       
	       gl_transform_vector( ctx->Transform._ClipUserPlane[p],
				    ctx->Transform.EyeUserPlane[p],
				    ctx->ProjectionMatrix.inv );
	    } else {
	       if (--ctx->Transform._AnyClip == 0)
		  ctx->_Enabled &= ~ENABLE_USERCLIP;	       
	    }	    
	 }
	 break;
      case GL_COLOR_MATERIAL:
         if (ctx->Light.ColorMaterialEnabled!=state) {
            ctx->Light.ColorMaterialEnabled = state;
	    ctx->NewState |= _NEW_LIGHT;
            if (state)
               gl_update_color_material( ctx, ctx->Current.Color );
         }
	 break;
      case GL_CULL_FACE:
         if (ctx->Polygon.CullFlag!=state) {
            ctx->Polygon.CullFlag = state;
/*  	    ctx->_TriangleCaps ^= DD_TRI_CULL; */
            ctx->NewState |= _NEW_POLYGON;
         }
	 break;
      case GL_DEPTH_TEST:
         if (state && ctx->Visual.DepthBits==0) {
            _mesa_warning(ctx,"glEnable(GL_DEPTH_TEST) but no depth buffer");
            return;
         }
	 if (ctx->Depth.Test!=state) {
            ctx->Depth.Test = state;
            ctx->NewState |= _NEW_DEPTH;
         }
         break;
      case GL_DITHER:
         if (ctx->NoDither) {
            /* MESA_NO_DITHER env var */
            state = GL_FALSE;
         }
         if (ctx->Color.DitherFlag!=state) {
            ctx->Color.DitherFlag = state;
            ctx->NewState |= _NEW_COLOR;
         }
	 break;
      case GL_FOG:
	 if (ctx->Fog.Enabled!=state) {
            ctx->Fog.Enabled = state;
	    ctx->_Enabled ^= ENABLE_FOG;
            ctx->NewState |= _NEW_FOG;
         }
	 break;
      case GL_HISTOGRAM:
         if (ctx->Extensions.EXT_histogram) {
            ctx->Pixel.HistogramEnabled = state;
            ctx->NewState |= _NEW_PIXEL;
         }
         else {
            gl_error( ctx, GL_INVALID_ENUM, state ? "glEnable": "glDisable" );
            return;
         }
         break;
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
	 if (ctx->Light.Light[cap-GL_LIGHT0].Enabled != state) {
	    ctx->Light.Light[cap-GL_LIGHT0].Enabled = state;

	    if (state) {
	       insert_at_tail(&ctx->Light.EnabledList, 
			      &ctx->Light.Light[cap-GL_LIGHT0]);
	       if (ctx->Light.Enabled)
		  ctx->_Enabled |= ENABLE_LIGHT;
	    } else {
	       remove_from_list(&ctx->Light.Light[cap-GL_LIGHT0]);
	       if (is_empty_list(&ctx->Light.EnabledList))
		  ctx->_Enabled &= ~ENABLE_LIGHT;
	    }

	    ctx->NewState |= _NEW_LIGHT;
	 }
         break;
      case GL_LIGHTING:
         if (ctx->Light.Enabled!=state) {
            ctx->Light.Enabled = state;
	    ctx->_Enabled &= ~ENABLE_LIGHT;
            if (state)
	       ctx->_Enabled |= ENABLE_LIGHT;
            ctx->NewState |= _NEW_LIGHT;
         }
         break;
      case GL_LINE_SMOOTH:
	 if (ctx->Line.SmoothFlag!=state) {
            ctx->Line.SmoothFlag = state;
	    ctx->_TriangleCaps ^= DD_LINE_SMOOTH;
            ctx->NewState |= _NEW_LINE;
         }
	 break;
      case GL_LINE_STIPPLE:
	 if (ctx->Line.StippleFlag!=state) {
            ctx->Line.StippleFlag = state;
	    ctx->_TriangleCaps ^= DD_LINE_STIPPLE;
            ctx->NewState |= _NEW_LINE;
         }
	 break;
      case GL_INDEX_LOGIC_OP:
         if (ctx->Color.IndexLogicOpEnabled!=state) {
	    ctx->Color.IndexLogicOpEnabled = state;
            ctx->NewState |= _NEW_COLOR;
         }
	 break;
      case GL_COLOR_LOGIC_OP:
         if (ctx->Color.ColorLogicOpEnabled!=state) {
	    ctx->Color.ColorLogicOpEnabled = state;
            ctx->NewState |= _NEW_COLOR;
         }
	 break;
      case GL_MAP1_COLOR_4:
	 ctx->Eval.Map1Color4 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_INDEX:
	 ctx->Eval.Map1Index = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_NORMAL:
	 ctx->Eval.Map1Normal = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 ctx->Eval.Map1TextureCoord1 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 ctx->Eval.Map1TextureCoord2 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 ctx->Eval.Map1TextureCoord3 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 ctx->Eval.Map1TextureCoord4 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_VERTEX_3:
	 ctx->Eval.Map1Vertex3 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP1_VERTEX_4:
	 ctx->Eval.Map1Vertex4 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_COLOR_4:
	 ctx->Eval.Map2Color4 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_INDEX:
	 ctx->Eval.Map2Index = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_NORMAL:
	 ctx->Eval.Map2Normal = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_TEXTURE_COORD_1: 
	 ctx->Eval.Map2TextureCoord1 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 ctx->Eval.Map2TextureCoord2 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 ctx->Eval.Map2TextureCoord3 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 ctx->Eval.Map2TextureCoord4 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_VERTEX_3:
	 ctx->Eval.Map2Vertex3 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MAP2_VERTEX_4:
	 ctx->Eval.Map2Vertex4 = state;
	 ctx->NewState |= _NEW_EVAL;
	 break;
      case GL_MINMAX:
         ctx->Pixel.MinMaxEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;
      case GL_NORMALIZE:
	 if (ctx->Transform.Normalize != state) {
	    ctx->Transform.Normalize = state;
	    ctx->NewState |= _NEW_TRANSFORM;
	    ctx->_Enabled ^= ENABLE_NORMALIZE;
	 }
	 break;
      case GL_POINT_SMOOTH:
	 if (ctx->Point.SmoothFlag!=state) {
            ctx->Point.SmoothFlag = state;
	    ctx->_TriangleCaps ^= DD_POINT_SMOOTH;
            ctx->NewState |= _NEW_POINT;
         }
	 break;
      case GL_POLYGON_SMOOTH:
	 if (ctx->Polygon.SmoothFlag!=state) {
            ctx->Polygon.SmoothFlag = state;
	    ctx->_TriangleCaps ^= DD_TRI_SMOOTH;
            ctx->NewState |= _NEW_POLYGON;
         }
	 break;
      case GL_POLYGON_STIPPLE:
	 if (ctx->Polygon.StippleFlag!=state) {
            ctx->Polygon.StippleFlag = state;
	    ctx->_TriangleCaps ^= DD_TRI_STIPPLE;
            ctx->NewState |= _NEW_POLYGON;
         }
	 break;
      case GL_POLYGON_OFFSET_POINT:
         if (ctx->Polygon.OffsetPoint!=state) {
            ctx->Polygon.OffsetPoint = state;
            ctx->NewState |= _NEW_POLYGON;
         }
         break;
      case GL_POLYGON_OFFSET_LINE:
         if (ctx->Polygon.OffsetLine!=state) {
            ctx->Polygon.OffsetLine = state;
            ctx->NewState |= _NEW_POLYGON;
         }
         break;
      case GL_POLYGON_OFFSET_FILL:
      /*case GL_POLYGON_OFFSET_EXT:*/
         if (ctx->Polygon.OffsetFill!=state) {
            ctx->Polygon.OffsetFill = state;
            ctx->NewState |= _NEW_POLYGON;
         }
         break;
      case GL_RESCALE_NORMAL_EXT:
	 if (ctx->Transform.RescaleNormals != state) {
	    ctx->Transform.RescaleNormals = state;
	    ctx->NewState |= _NEW_TRANSFORM;
	    ctx->_Enabled ^= ENABLE_RESCALE;
	 }
         break;
      case GL_SCISSOR_TEST:
         if (ctx->Scissor.Enabled!=state) {
            ctx->Scissor.Enabled = state;
            ctx->NewState |= _NEW_SCISSOR;
         }
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         ctx->Texture.SharedPalette = state;
	 ctx->NewState |= _NEW_TEXTURE;
         break;
      case GL_STENCIL_TEST:
	 if (state && ctx->Visual.StencilBits==0) {
            _mesa_warning(ctx, "glEnable(GL_STENCIL_TEST) but no stencil buffer");
            return;
	 }
	 if (ctx->Stencil.Enabled!=state) {
            ctx->Stencil.Enabled = state;
            ctx->NewState |= _NEW_STENCIL;
	    ctx->_TriangleCaps ^= DD_STENCIL;
         }
	 break;
      case GL_TEXTURE_1D:
         if (ctx->Visual.RGBAflag) {
	    const GLuint curr = ctx->Texture.CurrentUnit;
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
	    ctx->NewState |= _NEW_TEXTURE;
            if (state) {
	       texUnit->Enabled |= TEXTURE0_1D;
	    }
            else {
               texUnit->Enabled &= ~TEXTURE0_1D;
            }
         }
         break;
      case GL_TEXTURE_2D:
         if (ctx->Visual.RGBAflag) {
	    const GLuint curr = ctx->Texture.CurrentUnit;
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
	    ctx->NewState |= _NEW_TEXTURE;
            if (state) {
	       texUnit->Enabled |= TEXTURE0_2D;
	    }
            else {
               texUnit->Enabled &= ~TEXTURE0_2D;
            }
         }
	 break;
      case GL_TEXTURE_3D:
         if (ctx->Visual.RGBAflag) {
	    const GLuint curr = ctx->Texture.CurrentUnit;
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
	    ctx->NewState |= _NEW_TEXTURE;
            if (state) {
	       texUnit->Enabled |= TEXTURE0_3D;
	    }
            else {
               texUnit->Enabled &= ~TEXTURE0_3D;
            }
         }
         break;
      case GL_TEXTURE_GEN_Q:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= Q_BIT;
            else
               texUnit->TexGenEnabled &= ~Q_BIT;
            ctx->NewState |= _NEW_TEXTURE;
         }
	 break;
      case GL_TEXTURE_GEN_R:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= R_BIT;
            else
               texUnit->TexGenEnabled &= ~R_BIT;
            ctx->NewState |= _NEW_TEXTURE;
         }
	 break;
      case GL_TEXTURE_GEN_S:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= S_BIT;
            else
               texUnit->TexGenEnabled &= ~S_BIT;
            ctx->NewState |= _NEW_TEXTURE;
         }
	 break;
      case GL_TEXTURE_GEN_T:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= T_BIT;
            else
               texUnit->TexGenEnabled &= ~T_BIT;
            ctx->NewState |= _NEW_TEXTURE;
         }
	 break;

      /*
       * CLIENT STATE!!!
       */
      case GL_VERTEX_ARRAY:
         ctx->Array.Vertex.Enabled = state;
	 ctx->NewState |= _NEW_ARRAY;
	 break;
      case GL_NORMAL_ARRAY:
         ctx->Array.Normal.Enabled = state;
	 ctx->NewState |= _NEW_ARRAY;
         break;
      case GL_COLOR_ARRAY:
         ctx->Array.Color.Enabled = state;
	 ctx->NewState |= _NEW_ARRAY;
         break;
      case GL_INDEX_ARRAY:
         ctx->Array.Index.Enabled = state;
	 ctx->NewState |= _NEW_ARRAY;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         ctx->Array.TexCoord[ctx->Array.ActiveTexture].Enabled = state;
	 ctx->NewState |= _NEW_ARRAY;
         break;
      case GL_EDGE_FLAG_ARRAY:
         ctx->Array.EdgeFlag.Enabled = state;
	 ctx->NewState |= _NEW_ARRAY;
         break;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         if (ctx->Extensions.HP_occlusion_test) {
            ctx->Depth.OcclusionTest = state;
	    ctx->NewState |= _NEW_DEPTH;
            if (state)
               ctx->OcclusionResult = ctx->OcclusionResultSaved;
            else
               ctx->OcclusionResultSaved = ctx->OcclusionResult;
         }
         else {
            gl_error( ctx, GL_INVALID_ENUM, state ? "glEnable": "glDisable" );
            return;
         }
         break;

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         /* XXX check for extension */
         ctx->Pixel.PixelTextureEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         /* XXX check for extension */
         ctx->Pixel.PixelTextureEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;

      /* GL_SGI_color_table */
      case GL_COLOR_TABLE_SGI:
         /* XXX check for extension */
         ctx->Pixel.ColorTableEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         ctx->Pixel.PostConvolutionColorTableEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         ctx->Pixel.PostColorMatrixColorTableEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;

      /* GL_EXT_convolution */
      case GL_CONVOLUTION_1D:
         /* XXX check for extension */
         ctx->Pixel.Convolution1DEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;
      case GL_CONVOLUTION_2D:
         ctx->Pixel.Convolution2DEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;
      case GL_SEPARABLE_2D:
         ctx->Pixel.Separable2DEnabled = state;
	 ctx->NewState |= _NEW_PIXEL;
         break;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (ctx->Extensions.ARB_texture_cube_map) {
            if (ctx->Visual.RGBAflag) {
               const GLuint curr = ctx->Texture.CurrentUnit;
               struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
               ctx->NewState |= _NEW_TEXTURE;
               if (state) {
                  texUnit->Enabled |= TEXTURE0_CUBE;
               }
               else {
                  texUnit->Enabled &= ~TEXTURE0_CUBE;
               }
            }
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, state ? "glEnable" : "glDisable");
            return;
         }
	 break;

      /* GL_EXT_secondary_color */
      case GL_COLOR_SUM_EXT:
         ctx->Fog.ColorSumEnabled = state;
	 if (state)
	    SET_BITS(ctx->_TriangleCaps, DD_SEPERATE_SPECULAR);
	 else if (ctx->Light.Model.ColorControl == GL_SINGLE_COLOR)
	    CLEAR_BITS(ctx->_TriangleCaps, DD_SEPERATE_SPECULAR);
	 ctx->NewState |= _NEW_FOG;
         break;

      default:
         gl_error(ctx, GL_INVALID_ENUM, state ? "glEnable" : "glDisable");
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
   _mesa_set_enable( ctx, cap, GL_TRUE );
}



void
_mesa_Disable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_set_enable( ctx, cap, GL_FALSE );
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
	 return ctx->Transform.ClipEnabled[cap-GL_CLIP_PLANE0];
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
      case GL_HISTOGRAM:
         if (ctx->Extensions.EXT_histogram) {
            return ctx->Pixel.HistogramEnabled;
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glIsEnabled");
            return GL_FALSE;
         }
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
      case GL_MINMAX:
         return ctx->Pixel.MinMaxEnabled;
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
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE0_1D) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_2D:
         {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE0_2D) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_3D:
         {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE0_3D) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_Q:
         {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & Q_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_R:
         {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & R_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_S:
         {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & S_BIT) ? GL_TRUE : GL_FALSE;
         }
      case GL_TEXTURE_GEN_T:
         {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->TexGenEnabled & T_BIT) ? GL_TRUE : GL_FALSE;
         }

      /*
       * CLIENT STATE!!!
       */
      case GL_VERTEX_ARRAY:
         return ctx->Array.Vertex.Enabled;
      case GL_NORMAL_ARRAY:
         return ctx->Array.Normal.Enabled;
      case GL_COLOR_ARRAY:
         return ctx->Array.Color.Enabled;
      case GL_INDEX_ARRAY:
         return ctx->Array.Index.Enabled;
      case GL_TEXTURE_COORD_ARRAY:
         return ctx->Array.TexCoord[ctx->Array.ActiveTexture].Enabled;
      case GL_EDGE_FLAG_ARRAY:
         return ctx->Array.EdgeFlag.Enabled;

      /* GL_HP_occlusion_test */
      case GL_OCCLUSION_TEST_HP:
         if (ctx->Extensions.HP_occlusion_test) {
            return ctx->Depth.OcclusionTest;
         }
         else {
            gl_error( ctx, GL_INVALID_ENUM, "glIsEnabled" );
            return GL_FALSE;
         }

      /* GL_SGIS_pixel_texture */
      case GL_PIXEL_TEXTURE_SGIS:
         return ctx->Pixel.PixelTextureEnabled;

      /* GL_SGIX_pixel_texture */
      case GL_PIXEL_TEX_GEN_SGIX:
         return ctx->Pixel.PixelTextureEnabled;

      /* GL_SGI_color_table */
      case GL_COLOR_TABLE_SGI:
         return ctx->Pixel.ColorTableEnabled;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         return ctx->Pixel.PostConvolutionColorTableEnabled;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         return ctx->Pixel.PostColorMatrixColorTableEnabled;

      /* GL_EXT_convolution */
      case GL_CONVOLUTION_1D:
         return ctx->Pixel.Convolution1DEnabled;
      case GL_CONVOLUTION_2D:
         return ctx->Pixel.Convolution2DEnabled;
      case GL_SEPARABLE_2D:
         return ctx->Pixel.Separable2DEnabled;

      /* GL_ARB_texture_cube_map */
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (ctx->Extensions.ARB_texture_cube_map) {
            const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            return (texUnit->Enabled & TEXTURE0_CUBE) ? GL_TRUE : GL_FALSE;
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glIsEnabled");
            return GL_FALSE;
         }

      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glIsEnabled" );
	 return GL_FALSE;
   }
}




static void
client_state( GLcontext *ctx, GLenum cap, GLboolean state )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx, 
				       (state 
					? "glEnableClientState" 
					: "glDisableClientState") );

   switch (cap) {
      case GL_VERTEX_ARRAY:
         ctx->Array.Vertex.Enabled = state;
         break;
      case GL_NORMAL_ARRAY:
         ctx->Array.Normal.Enabled = state;
         break;
      case GL_COLOR_ARRAY:
         ctx->Array.Color.Enabled = state;
         break;
      case GL_INDEX_ARRAY:
         ctx->Array.Index.Enabled = state;
         break;
      case GL_TEXTURE_COORD_ARRAY:
         ctx->Array.TexCoord[ctx->Array.ActiveTexture].Enabled = state;
         break;
      case GL_EDGE_FLAG_ARRAY:
         ctx->Array.EdgeFlag.Enabled = state;
         break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
         ctx->Array.FogCoord.Enabled = state;
         break;
      case GL_SECONDARY_COLOR_ARRAY_EXT:
         ctx->Array.SecondaryColor.Enabled = state;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glEnable/DisableClientState" );
   }

   ctx->NewState |= _NEW_ARRAY;
}



void
_mesa_EnableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   client_state( ctx, cap, GL_TRUE );
}



void
_mesa_DisableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   client_state( ctx, cap, GL_FALSE );
}

