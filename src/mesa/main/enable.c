/* $Id: enable.c,v 1.9 1999/11/10 06:29:44 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


/* $XFree86: xc/lib/GL/mesa/src/enable.c,v 1.3 1999/04/04 00:20:23 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdio.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
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
void gl_set_enable( GLcontext *ctx, GLenum cap, GLboolean state )
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
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_AUTO_NORMAL:
	 ctx->Eval.AutoNormal = state;
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
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
	 if (cap >= GL_CLIP_PLANE0 && 
	     cap <= GL_CLIP_PLANE5 &&
	     ctx->Transform.ClipEnabled[cap-GL_CLIP_PLANE0] != state) 
	 {
	    GLuint p = cap-GL_CLIP_PLANE0;

	    ctx->Transform.ClipEnabled[p] = state;
	    ctx->NewState |= NEW_USER_CLIP;

	    if (state) {
	       ctx->Enabled |= ENABLE_USERCLIP;
	       ctx->Transform.AnyClip++;
	       
	       if (ctx->ProjectionMatrix.flags & MAT_DIRTY_ALL_OVER) {
		  gl_matrix_analyze( &ctx->ProjectionMatrix );
	       }
	       
	       gl_transform_vector( ctx->Transform.ClipUserPlane[p],
				    ctx->Transform.EyeUserPlane[p],
				    ctx->ProjectionMatrix.inv );
	    } else {
	       if (--ctx->Transform.AnyClip == 0)
		  ctx->Enabled &= ~ENABLE_USERCLIP;	       
	    }	    
	 }
	 break;
      case GL_COLOR_MATERIAL:
         if (ctx->Light.ColorMaterialEnabled!=state) {
            ctx->Light.ColorMaterialEnabled = state;
	    ctx->NewState |= NEW_LIGHTING;
	    if (state) 
	       gl_update_color_material( ctx, ctx->Current.ByteColor );
         }
	 break;
      case GL_CULL_FACE:
         if (ctx->Polygon.CullFlag!=state) {
            ctx->Polygon.CullFlag = state;
	    ctx->TriangleCaps ^= DD_TRI_CULL;
            ctx->NewState |= NEW_POLYGON;
         }
	 break;
      case GL_DEPTH_TEST:
         if (state && ctx->Visual->DepthBits==0) {
            gl_warning(ctx,"glEnable(GL_DEPTH_TEST) but no depth buffer");
            return;
         }
	 if (ctx->Depth.Test!=state) {
            ctx->Depth.Test = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
         break;
      case GL_DITHER:
         if (ctx->NoDither) {
            /* MESA_NO_DITHER env var */
            state = GL_FALSE;
         }
         if (ctx->Color.DitherFlag!=state) {
            ctx->Color.DitherFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_FOG:
	 if (ctx->Fog.Enabled!=state) {
            ctx->Fog.Enabled = state;
	    ctx->Enabled ^= ENABLE_FOG;
            ctx->NewState |= NEW_FOG|NEW_RASTER_OPS;
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
	 if (ctx->Light.Light[cap-GL_LIGHT0].Enabled != state) 
	 {
	    ctx->Light.Light[cap-GL_LIGHT0].Enabled = state;

	    if (state) {
	       insert_at_tail(&ctx->Light.EnabledList, 
			      &ctx->Light.Light[cap-GL_LIGHT0]);
	       if (ctx->Light.Enabled)
		  ctx->Enabled |= ENABLE_LIGHT;
	    } else {
	       remove_from_list(&ctx->Light.Light[cap-GL_LIGHT0]);
	       if (is_empty_list(&ctx->Light.EnabledList))
		  ctx->Enabled &= ~ENABLE_LIGHT;
	    }

	    ctx->NewState |= NEW_LIGHTING;
	 }
         break;
      case GL_LIGHTING:
         if (ctx->Light.Enabled!=state) {
            ctx->Light.Enabled = state;
	    ctx->Enabled &= ~ENABLE_LIGHT;
            if (state)
	       ctx->Enabled |= ENABLE_LIGHT;
            ctx->NewState |= NEW_LIGHTING;
         }
         break;
      case GL_LINE_SMOOTH:
	 if (ctx->Line.SmoothFlag!=state) {
            ctx->Line.SmoothFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_LINE_STIPPLE:
	 if (ctx->Line.StippleFlag!=state) {
            ctx->Line.StippleFlag = state;
	    ctx->TriangleCaps ^= DD_LINE_STIPPLE;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_INDEX_LOGIC_OP:
         if (ctx->Color.IndexLogicOpEnabled!=state) {
	    ctx->Color.IndexLogicOpEnabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_COLOR_LOGIC_OP:
         if (ctx->Color.ColorLogicOpEnabled!=state) {
	    ctx->Color.ColorLogicOpEnabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_MAP1_COLOR_4:
	 ctx->Eval.Map1Color4 = state;
	 break;
      case GL_MAP1_INDEX:
	 ctx->Eval.Map1Index = state;
	 break;
      case GL_MAP1_NORMAL:
	 ctx->Eval.Map1Normal = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
	 ctx->Eval.Map1TextureCoord1 = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
	 ctx->Eval.Map1TextureCoord2 = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
	 ctx->Eval.Map1TextureCoord3 = state;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
	 ctx->Eval.Map1TextureCoord4 = state;
	 break;
      case GL_MAP1_VERTEX_3:
	 ctx->Eval.Map1Vertex3 = state;
	 break;
      case GL_MAP1_VERTEX_4:
	 ctx->Eval.Map1Vertex4 = state;
	 break;
      case GL_MAP2_COLOR_4:
	 ctx->Eval.Map2Color4 = state;
	 break;
      case GL_MAP2_INDEX:
	 ctx->Eval.Map2Index = state;
	 break;
      case GL_MAP2_NORMAL:
	 ctx->Eval.Map2Normal = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_1: 
	 ctx->Eval.Map2TextureCoord1 = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
	 ctx->Eval.Map2TextureCoord2 = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
	 ctx->Eval.Map2TextureCoord3 = state;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
	 ctx->Eval.Map2TextureCoord4 = state;
	 break;
      case GL_MAP2_VERTEX_3:
	 ctx->Eval.Map2Vertex3 = state;
	 break;
      case GL_MAP2_VERTEX_4:
	 ctx->Eval.Map2Vertex4 = state;
	 break;
      case GL_NORMALIZE:
	 if (ctx->Transform.Normalize != state) {
	    ctx->Transform.Normalize = state;
	    ctx->NewState |= NEW_NORMAL_TRANSFORM|NEW_LIGHTING;
	    ctx->Enabled ^= ENABLE_NORMALIZE;
	 }
	 break;
      case GL_POINT_SMOOTH:
	 if (ctx->Point.SmoothFlag!=state) {
            ctx->Point.SmoothFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_POLYGON_SMOOTH:
	 if (ctx->Polygon.SmoothFlag!=state) {
            ctx->Polygon.SmoothFlag = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_POLYGON_STIPPLE:
	 if (ctx->Polygon.StippleFlag!=state) {
            ctx->Polygon.StippleFlag = state;
	    ctx->TriangleCaps ^= DD_TRI_STIPPLE;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_POLYGON_OFFSET_POINT:
         if (ctx->Polygon.OffsetPoint!=state) {
            ctx->Polygon.OffsetPoint = state;
            ctx->NewState |= NEW_POLYGON;
         }
         break;
      case GL_POLYGON_OFFSET_LINE:
         if (ctx->Polygon.OffsetLine!=state) {
            ctx->Polygon.OffsetLine = state;
            ctx->NewState |= NEW_POLYGON;
         }
         break;
      case GL_POLYGON_OFFSET_FILL:
      /*case GL_POLYGON_OFFSET_EXT:*/
         if (ctx->Polygon.OffsetFill!=state) {
            ctx->Polygon.OffsetFill = state;
            ctx->NewState |= NEW_POLYGON;
         }
         break;
      case GL_RESCALE_NORMAL_EXT:
	 if (ctx->Transform.RescaleNormals != state) {
	    ctx->Transform.RescaleNormals = state;
	    ctx->NewState |= NEW_NORMAL_TRANSFORM|NEW_LIGHTING;
	    ctx->Enabled ^= ENABLE_RESCALE;
	 }
         break;
      case GL_SCISSOR_TEST:
         if (ctx->Scissor.Enabled!=state) {
            ctx->Scissor.Enabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
         }
	 break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         ctx->Texture.SharedPalette = state;
         if (ctx->Driver.UseGlobalTexturePalette)
            (*ctx->Driver.UseGlobalTexturePalette)( ctx, state );
         break;
      case GL_STENCIL_TEST:
	 if (state && ctx->Visual->StencilBits==0) {
            gl_warning(ctx, "glEnable(GL_STENCIL_TEST) but no stencil buffer");
            return;
	 }
	 if (ctx->Stencil.Enabled!=state) {
            ctx->Stencil.Enabled = state;
            ctx->NewState |= NEW_RASTER_OPS;
	    ctx->TriangleCaps ^= DD_STENCIL;
         }
	 break;
      case GL_TEXTURE_1D:
         if (ctx->Visual->RGBAflag) {
	    const GLuint curr = ctx->Texture.CurrentUnit;
	    const GLuint flag = TEXTURE0_1D << (curr * 4);
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
	    ctx->NewState |= NEW_TEXTURE_ENABLE;
            if (state) {
	       texUnit->Enabled |= TEXTURE0_1D;
	       ctx->Enabled |= flag;
	    }
            else {
               texUnit->Enabled &= ~TEXTURE0_1D;
               ctx->Enabled &= ~flag;
            }
         }
         break;
      case GL_TEXTURE_2D:
         if (ctx->Visual->RGBAflag) {
	    const GLuint curr = ctx->Texture.CurrentUnit;
	    const GLuint flag = TEXTURE0_2D << (curr * 4);
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
	    ctx->NewState |= NEW_TEXTURE_ENABLE;
            if (state) {
	       texUnit->Enabled |= TEXTURE0_2D;
	       ctx->Enabled |= flag;
	    }
            else {
               texUnit->Enabled &= ~TEXTURE0_2D;
               ctx->Enabled &= ~flag;
            }
         }
	 break;
      case GL_TEXTURE_3D:
         if (ctx->Visual->RGBAflag) {
	    const GLuint curr = ctx->Texture.CurrentUnit;
	    const GLuint flag = TEXTURE0_3D << (curr * 4);
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[curr];
	    ctx->NewState |= NEW_TEXTURE_ENABLE;
            if (state) {
	       texUnit->Enabled |= TEXTURE0_3D;
	       ctx->Enabled |= flag;
	    }
            else {
               texUnit->Enabled &= ~TEXTURE0_3D;
               ctx->Enabled &= ~flag;
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
            ctx->NewState |= NEW_TEXTURING;
         }
	 break;
      case GL_TEXTURE_GEN_R:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= R_BIT;
            else
               texUnit->TexGenEnabled &= ~R_BIT;
            ctx->NewState |= NEW_TEXTURING;
         }
	 break;
      case GL_TEXTURE_GEN_S:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= S_BIT;
            else
               texUnit->TexGenEnabled &= ~S_BIT;
            ctx->NewState |= NEW_TEXTURING;
         }
	 break;
      case GL_TEXTURE_GEN_T:
         {
            struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
            if (state)
               texUnit->TexGenEnabled |= T_BIT;
            else
               texUnit->TexGenEnabled &= ~T_BIT;
            ctx->NewState |= NEW_TEXTURING;
         }
	 break;

      /*
       * CLIENT STATE!!!
       */
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

      default:
	 if (state) {
	    gl_error( ctx, GL_INVALID_ENUM, "glEnable" );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glDisable" );
	 }
         return;
   }

   if (ctx->Driver.Enable) {
      (*ctx->Driver.Enable)( ctx, cap, state );
   }
}




void gl_Enable( GLcontext* ctx, GLenum cap )
{
   gl_set_enable( ctx, cap, GL_TRUE );
}



void gl_Disable( GLcontext* ctx, GLenum cap )
{
   gl_set_enable( ctx, cap, GL_FALSE );
}



GLboolean gl_IsEnabled( GLcontext* ctx, GLenum cap )
{
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
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glIsEnabled" );
	 return GL_FALSE;
   }
}




static void gl_client_state( GLcontext *ctx, GLenum cap, GLboolean state )
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
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glEnable/DisableClientState" );
   }

   ctx->NewState |= NEW_CLIENT_STATE;
}



void gl_EnableClientState( GLcontext *ctx, GLenum cap )
{
   gl_client_state( ctx, cap, GL_TRUE );
}



void gl_DisableClientState( GLcontext *ctx, GLenum cap )
{
   gl_client_state( ctx, cap, GL_FALSE );
}

