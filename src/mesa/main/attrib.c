/* $Id: attrib.c,v 1.24 2000/07/05 22:26:43 brianp Exp $ */

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
#include "attrib.h"
#include "buffers.h"
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "mem.h"
#include "simple_list.h"
#include "texstate.h"
#include "types.h"
#endif




/*
 * Allocate a new attribute state node.  These nodes have a
 * "kind" value and a pointer to a struct of state data.
 */
static struct gl_attrib_node *
new_attrib_node( GLbitfield kind )
{
   struct gl_attrib_node *an = MALLOC_STRUCT(gl_attrib_node);
   if (an) {
      an->kind = kind;
   }
   return an;
}



/*
 * Copy texture object state from one texture object to another.
 */
static void
copy_texobj_state( struct gl_texture_object *dest,
                   const struct gl_texture_object *src )
{
   /*
   dest->Name = src->Name;
   dest->Dimensions = src->Dimensions;
   */
   dest->Priority = src->Priority;
   dest->BorderColor[0] = src->BorderColor[0];
   dest->BorderColor[1] = src->BorderColor[1];
   dest->BorderColor[2] = src->BorderColor[2];
   dest->BorderColor[3] = src->BorderColor[3];
   dest->WrapS = src->WrapS;
   dest->WrapT = src->WrapT;
   dest->WrapR = src->WrapR;
   dest->MinFilter = src->MinFilter;
   dest->MagFilter = src->MagFilter;
   dest->MinLod = src->MinLod;
   dest->MaxLod = src->MaxLod;
   dest->BaseLevel = src->BaseLevel;
   dest->MaxLevel = src->MaxLevel;
   dest->P = src->P;
   dest->M = src->M;
   dest->MinMagThresh = src->MinMagThresh;
   dest->Palette = src->Palette;
   dest->Complete = src->Complete;
   dest->SampleFunc = src->SampleFunc;
}



void
_mesa_PushAttrib(GLbitfield mask)
{
   struct gl_attrib_node *newnode;
   struct gl_attrib_node *head;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPushAttrib");

   if (MESA_VERBOSE&VERBOSE_API)
      fprintf(stderr, "glPushAttrib %x\n", (int)mask);

   if (ctx->AttribStackDepth>=MAX_ATTRIB_STACK_DEPTH) {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_ACCUM_BUFFER_BIT) {
      struct gl_accum_attrib *attr;
      attr = MALLOC_STRUCT( gl_accum_attrib );
      MEMCPY( attr, &ctx->Accum, sizeof(struct gl_accum_attrib) );
      newnode = new_attrib_node( GL_ACCUM_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_colorbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_colorbuffer_attrib );
      MEMCPY( attr, &ctx->Color, sizeof(struct gl_colorbuffer_attrib) );
      newnode = new_attrib_node( GL_COLOR_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_CURRENT_BIT) {
      struct gl_current_attrib *attr;
      attr = MALLOC_STRUCT( gl_current_attrib );
      MEMCPY( attr, &ctx->Current, sizeof(struct gl_current_attrib) );
      newnode = new_attrib_node( GL_CURRENT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_depthbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_depthbuffer_attrib );
      MEMCPY( attr, &ctx->Depth, sizeof(struct gl_depthbuffer_attrib) );
      newnode = new_attrib_node( GL_DEPTH_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_ENABLE_BIT) {
      struct gl_enable_attrib *attr;
      GLuint i;
      attr = MALLOC_STRUCT( gl_enable_attrib );
      /* Copy enable flags from all other attributes into the enable struct. */
      attr->AlphaTest = ctx->Color.AlphaEnabled;
      attr->AutoNormal = ctx->Eval.AutoNormal;
      attr->Blend = ctx->Color.BlendEnabled;
      for (i=0;i<MAX_CLIP_PLANES;i++) {
         attr->ClipPlane[i] = ctx->Transform.ClipEnabled[i];
      }
      attr->ColorMaterial = ctx->Light.ColorMaterialEnabled;
      attr->Convolution1D = ctx->Pixel.Convolution1DEnabled;
      attr->Convolution2D = ctx->Pixel.Convolution2DEnabled;
      attr->Separable2D = ctx->Pixel.Separable2DEnabled;
      attr->CullFace = ctx->Polygon.CullFlag;
      attr->DepthTest = ctx->Depth.Test;
      attr->Dither = ctx->Color.DitherFlag;
      attr->Fog = ctx->Fog.Enabled;
      for (i=0;i<MAX_LIGHTS;i++) {
         attr->Light[i] = ctx->Light.Light[i].Enabled;
      }
      attr->Lighting = ctx->Light.Enabled;
      attr->LineSmooth = ctx->Line.SmoothFlag;
      attr->LineStipple = ctx->Line.StippleFlag;
      attr->Histogram = ctx->Pixel.HistogramEnabled;
      attr->MinMax = ctx->Pixel.MinMaxEnabled;
      attr->IndexLogicOp = ctx->Color.IndexLogicOpEnabled;
      attr->ColorLogicOp = ctx->Color.ColorLogicOpEnabled;
      attr->Map1Color4 = ctx->Eval.Map1Color4;
      attr->Map1Index = ctx->Eval.Map1Index;
      attr->Map1Normal = ctx->Eval.Map1Normal;
      attr->Map1TextureCoord1 = ctx->Eval.Map1TextureCoord1;
      attr->Map1TextureCoord2 = ctx->Eval.Map1TextureCoord2;
      attr->Map1TextureCoord3 = ctx->Eval.Map1TextureCoord3;
      attr->Map1TextureCoord4 = ctx->Eval.Map1TextureCoord4;
      attr->Map1Vertex3 = ctx->Eval.Map1Vertex3;
      attr->Map1Vertex4 = ctx->Eval.Map1Vertex4;
      attr->Map2Color4 = ctx->Eval.Map2Color4;
      attr->Map2Index = ctx->Eval.Map2Index;
      attr->Map2Normal = ctx->Eval.Map2Normal;
      attr->Map2TextureCoord1 = ctx->Eval.Map2TextureCoord1;
      attr->Map2TextureCoord2 = ctx->Eval.Map2TextureCoord2;
      attr->Map2TextureCoord3 = ctx->Eval.Map2TextureCoord3;
      attr->Map2TextureCoord4 = ctx->Eval.Map2TextureCoord4;
      attr->Map2Vertex3 = ctx->Eval.Map2Vertex3;
      attr->Map2Vertex4 = ctx->Eval.Map2Vertex4;
      attr->Normalize = ctx->Transform.Normalize;
      attr->PixelTexture = ctx->Pixel.PixelTextureEnabled;
      attr->PointSmooth = ctx->Point.SmoothFlag;
      attr->PolygonOffsetPoint = ctx->Polygon.OffsetPoint;
      attr->PolygonOffsetLine = ctx->Polygon.OffsetLine;
      attr->PolygonOffsetFill = ctx->Polygon.OffsetFill;
      attr->PolygonSmooth = ctx->Polygon.SmoothFlag;
      attr->PolygonStipple = ctx->Polygon.StippleFlag;
      attr->RescaleNormals = ctx->Transform.RescaleNormals;
      attr->Scissor = ctx->Scissor.Enabled;
      attr->Stencil = ctx->Stencil.Enabled;
      attr->Texture = ctx->Texture.Enabled;
      for (i=0; i<MAX_TEXTURE_UNITS; i++) {
         attr->TexGen[i] = ctx->Texture.Unit[i].TexGenEnabled;
      }
      newnode = new_attrib_node( GL_ENABLE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_EVAL_BIT) {
      struct gl_eval_attrib *attr;
      attr = MALLOC_STRUCT( gl_eval_attrib );
      MEMCPY( attr, &ctx->Eval, sizeof(struct gl_eval_attrib) );
      newnode = new_attrib_node( GL_EVAL_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_FOG_BIT) {
      struct gl_fog_attrib *attr;
      attr = MALLOC_STRUCT( gl_fog_attrib );
      MEMCPY( attr, &ctx->Fog, sizeof(struct gl_fog_attrib) );
      newnode = new_attrib_node( GL_FOG_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_HINT_BIT) {
      struct gl_hint_attrib *attr;
      attr = MALLOC_STRUCT( gl_hint_attrib );
      MEMCPY( attr, &ctx->Hint, sizeof(struct gl_hint_attrib) );
      newnode = new_attrib_node( GL_HINT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LIGHTING_BIT) {
      struct gl_light_attrib *attr;
      attr = MALLOC_STRUCT( gl_light_attrib );
      MEMCPY( attr, &ctx->Light, sizeof(struct gl_light_attrib) );
      newnode = new_attrib_node( GL_LIGHTING_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LINE_BIT) {
      struct gl_line_attrib *attr;
      attr = MALLOC_STRUCT( gl_line_attrib );
      MEMCPY( attr, &ctx->Line, sizeof(struct gl_line_attrib) );
      newnode = new_attrib_node( GL_LINE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LIST_BIT) {
      struct gl_list_attrib *attr;
      attr = MALLOC_STRUCT( gl_list_attrib );
      MEMCPY( attr, &ctx->List, sizeof(struct gl_list_attrib) );
      newnode = new_attrib_node( GL_LIST_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_PIXEL_MODE_BIT) {
      struct gl_pixel_attrib *attr;
      attr = MALLOC_STRUCT( gl_pixel_attrib );
      MEMCPY( attr, &ctx->Pixel, sizeof(struct gl_pixel_attrib) );
      newnode = new_attrib_node( GL_PIXEL_MODE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POINT_BIT) {
      struct gl_point_attrib *attr;
      attr = MALLOC_STRUCT( gl_point_attrib );
      MEMCPY( attr, &ctx->Point, sizeof(struct gl_point_attrib) );
      newnode = new_attrib_node( GL_POINT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POLYGON_BIT) {
      struct gl_polygon_attrib *attr;
      attr = MALLOC_STRUCT( gl_polygon_attrib );
      MEMCPY( attr, &ctx->Polygon, sizeof(struct gl_polygon_attrib) );
      newnode = new_attrib_node( GL_POLYGON_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POLYGON_STIPPLE_BIT) {
      GLuint *stipple;
      stipple = (GLuint *) MALLOC( 32*sizeof(GLuint) );
      MEMCPY( stipple, ctx->PolygonStipple, 32*sizeof(GLuint) );
      newnode = new_attrib_node( GL_POLYGON_STIPPLE_BIT );
      newnode->data = stipple;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_SCISSOR_BIT) {
      struct gl_scissor_attrib *attr;
      attr = MALLOC_STRUCT( gl_scissor_attrib );
      MEMCPY( attr, &ctx->Scissor, sizeof(struct gl_scissor_attrib) );
      newnode = new_attrib_node( GL_SCISSOR_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_stencil_attrib *attr;
      attr = MALLOC_STRUCT( gl_stencil_attrib );
      MEMCPY( attr, &ctx->Stencil, sizeof(struct gl_stencil_attrib) );
      newnode = new_attrib_node( GL_STENCIL_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_TEXTURE_BIT) {
      struct gl_texture_attrib *attr;
      GLuint u;
      /* Take care of texture object reference counters */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
	 ctx->Texture.Unit[u].CurrentD[1]->RefCount++;
	 ctx->Texture.Unit[u].CurrentD[2]->RefCount++;
	 ctx->Texture.Unit[u].CurrentD[3]->RefCount++;
      }
      attr = MALLOC_STRUCT( gl_texture_attrib );
      MEMCPY( attr, &ctx->Texture, sizeof(struct gl_texture_attrib) );
      /* copy state of the currently bound texture objects */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         copy_texobj_state(&attr->Unit[u].Saved1D, attr->Unit[u].CurrentD[1]);
         copy_texobj_state(&attr->Unit[u].Saved2D, attr->Unit[u].CurrentD[2]);
         copy_texobj_state(&attr->Unit[u].Saved3D, attr->Unit[u].CurrentD[3]);
         copy_texobj_state(&attr->Unit[u].SavedCubeMap, attr->Unit[u].CurrentCubeMap);
      }
      newnode = new_attrib_node( GL_TEXTURE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_TRANSFORM_BIT) {
      struct gl_transform_attrib *attr;
      attr = MALLOC_STRUCT( gl_transform_attrib );
      MEMCPY( attr, &ctx->Transform, sizeof(struct gl_transform_attrib) );
      newnode = new_attrib_node( GL_TRANSFORM_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_VIEWPORT_BIT) {
      struct gl_viewport_attrib *attr;
      attr = MALLOC_STRUCT( gl_viewport_attrib );
      MEMCPY( attr, &ctx->Viewport, sizeof(struct gl_viewport_attrib) );
      newnode = new_attrib_node( GL_VIEWPORT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   ctx->AttribStack[ctx->AttribStackDepth] = head;
   ctx->AttribStackDepth++;
}



/*
 * This function is kind of long just because we have to call a lot
 * of device driver functions to update device driver state.
 */
void
_mesa_PopAttrib(void)
{
   struct gl_attrib_node *attr, *next;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPopAttrib");


   if (ctx->AttribStackDepth==0) {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopAttrib" );
      return;
   }

   ctx->AttribStackDepth--;
   attr = ctx->AttribStack[ctx->AttribStackDepth];

   while (attr) {

      if (MESA_VERBOSE&VERBOSE_API)
	 fprintf(stderr, "glPopAttrib %s\n", gl_lookup_enum_by_nr(attr->kind));

      switch (attr->kind) {
         case GL_ACCUM_BUFFER_BIT:
            MEMCPY( &ctx->Accum, attr->data, sizeof(struct gl_accum_attrib) );
            break;
         case GL_COLOR_BUFFER_BIT:
            {
               GLenum oldDrawBuffer = ctx->Color.DrawBuffer;
               GLenum oldAlphaFunc = ctx->Color.AlphaFunc;
               GLubyte oldAlphaRef = ctx->Color.AlphaRef;
               GLenum oldBlendSrc = ctx->Color.BlendSrcRGB;
               GLenum oldBlendDst = ctx->Color.BlendDstRGB;
	       GLenum oldLogicOp = ctx->Color.LogicOp;
               MEMCPY( &ctx->Color, attr->data,
                       sizeof(struct gl_colorbuffer_attrib) );
               if (ctx->Color.DrawBuffer != oldDrawBuffer) {
                  _mesa_DrawBuffer( ctx->Color.DrawBuffer);
               }
               if ((ctx->Color.BlendSrcRGB != oldBlendSrc ||
                    ctx->Color.BlendDstRGB != oldBlendDst) &&
                   ctx->Driver.BlendFunc)
                  (*ctx->Driver.BlendFunc)( ctx, ctx->Color.BlendSrcRGB,
                                            ctx->Color.BlendDstRGB);
	       if (ctx->Color.LogicOp != oldLogicOp &&
		   ctx->Driver.LogicOpcode) {
		  ctx->Driver.LogicOpcode( ctx, ctx->Color.LogicOp );
               }
               if (ctx->Visual->RGBAflag) {
                  GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
                  GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
                  GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);
                  GLubyte a = (GLint) (ctx->Color.ClearColor[3] * 255.0F);
                  (*ctx->Driver.ClearColor)( ctx, r, g, b, a );
                  if ((ctx->Color.AlphaFunc != oldAlphaFunc ||
                       ctx->Color.AlphaRef != oldAlphaRef) &&
                      ctx->Driver.AlphaFunc)
                     (*ctx->Driver.AlphaFunc)( ctx, ctx->Color.AlphaFunc,
                                               ctx->Color.AlphaRef / 255.0F);
                  if (ctx->Driver.ColorMask) {
                     (*ctx->Driver.ColorMask)(ctx,
                                              ctx->Color.ColorMask[0],
                                              ctx->Color.ColorMask[1],
                                              ctx->Color.ColorMask[2],
                                              ctx->Color.ColorMask[3]);
                  }
               }
               else {
                  (*ctx->Driver.ClearIndex)( ctx, ctx->Color.ClearIndex);
               }
            }
            break;
         case GL_CURRENT_BIT:
            MEMCPY( &ctx->Current, attr->data,
		    sizeof(struct gl_current_attrib) );
            break;
         case GL_DEPTH_BUFFER_BIT:
            {
               GLenum oldDepthFunc = ctx->Depth.Func;
               GLboolean oldDepthMask = ctx->Depth.Mask;
               GLfloat oldDepthClear = ctx->Depth.Clear;
               MEMCPY( &ctx->Depth, attr->data,
                       sizeof(struct gl_depthbuffer_attrib) );
               if (ctx->Depth.Func != oldDepthFunc && ctx->Driver.DepthFunc)
                  (*ctx->Driver.DepthFunc)( ctx, ctx->Depth.Func );
               if (ctx->Depth.Mask != oldDepthMask && ctx->Driver.DepthMask)
                  (*ctx->Driver.DepthMask)( ctx, ctx->Depth.Mask );
               if (ctx->Depth.Clear != oldDepthClear && ctx->Driver.ClearDepth)
                  (*ctx->Driver.ClearDepth)( ctx, ctx->Depth.Clear );
            }
            break;
         case GL_ENABLE_BIT:
            {
               const struct gl_enable_attrib *enable;
               enable = (const struct gl_enable_attrib *) attr->data;

#define TEST_AND_UPDATE(VALUE, NEWVALUE, ENUM)		\
	if ((VALUE) != (NEWVALUE)) {			\
	   _mesa_set_enable( ctx, ENUM, (NEWVALUE) );	\
	}

               TEST_AND_UPDATE(ctx->Color.AlphaEnabled, enable->AlphaTest, GL_ALPHA_TEST);
               TEST_AND_UPDATE(ctx->Transform.Normalize, enable->AutoNormal, GL_NORMALIZE);
               TEST_AND_UPDATE(ctx->Color.BlendEnabled, enable->Blend, GL_BLEND);
               {
                  GLuint i;
                  for (i=0;i<MAX_CLIP_PLANES;i++) {
                     if (ctx->Transform.ClipEnabled[i] != enable->ClipPlane[i])
                        _mesa_set_enable( ctx, (GLenum) (GL_CLIP_PLANE0 + i), enable->ClipPlane[i] );
                  }
               }
               TEST_AND_UPDATE(ctx->Light.ColorMaterialEnabled, enable->ColorMaterial, GL_COLOR_MATERIAL);
               TEST_AND_UPDATE(ctx->Polygon.CullFlag, enable->CullFace, GL_CULL_FACE);
               TEST_AND_UPDATE(ctx->Depth.Test, enable->DepthTest, GL_DEPTH_TEST);
               TEST_AND_UPDATE(ctx->Color.DitherFlag, enable->Dither, GL_DITHER);
               TEST_AND_UPDATE(ctx->Pixel.Convolution1DEnabled, enable->Convolution1D, GL_CONVOLUTION_1D);
               TEST_AND_UPDATE(ctx->Pixel.Convolution2DEnabled, enable->Convolution2D, GL_CONVOLUTION_2D);
               TEST_AND_UPDATE(ctx->Pixel.Separable2DEnabled, enable->Separable2D, GL_SEPARABLE_2D);
               TEST_AND_UPDATE(ctx->Fog.Enabled, enable->Fog, GL_FOG);
               TEST_AND_UPDATE(ctx->Light.Enabled, enable->Lighting, GL_LIGHTING);
               TEST_AND_UPDATE(ctx->Line.SmoothFlag, enable->LineSmooth, GL_LINE_SMOOTH);
               TEST_AND_UPDATE(ctx->Line.StippleFlag, enable->LineStipple, GL_LINE_STIPPLE);
               TEST_AND_UPDATE(ctx->Color.IndexLogicOpEnabled, enable->IndexLogicOp, GL_INDEX_LOGIC_OP);
               TEST_AND_UPDATE(ctx->Color.ColorLogicOpEnabled, enable->ColorLogicOp, GL_COLOR_LOGIC_OP);
               TEST_AND_UPDATE(ctx->Eval.Map1Color4, enable->Map1Color4, GL_MAP1_COLOR_4);
               TEST_AND_UPDATE(ctx->Eval.Map1Index, enable->Map1Index, GL_MAP1_INDEX);
               TEST_AND_UPDATE(ctx->Eval.Map1Normal, enable->Map1Normal, GL_MAP1_NORMAL);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord1, enable->Map1TextureCoord1, GL_MAP1_TEXTURE_COORD_1);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord2, enable->Map1TextureCoord2, GL_MAP1_TEXTURE_COORD_2);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord3, enable->Map1TextureCoord3, GL_MAP1_TEXTURE_COORD_3);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord4, enable->Map1TextureCoord4, GL_MAP1_TEXTURE_COORD_4);
               TEST_AND_UPDATE(ctx->Eval.Map1Vertex3, enable->Map1Vertex3, GL_MAP1_VERTEX_3);
               TEST_AND_UPDATE(ctx->Eval.Map1Vertex4, enable->Map1Vertex4, GL_MAP1_VERTEX_4);
               TEST_AND_UPDATE(ctx->Eval.Map2Color4, enable->Map2Color4, GL_MAP2_COLOR_4);
               TEST_AND_UPDATE(ctx->Eval.Map2Index, enable->Map2Index, GL_MAP2_INDEX);
               TEST_AND_UPDATE(ctx->Eval.Map2Normal, enable->Map2Normal, GL_MAP2_NORMAL);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord1, enable->Map2TextureCoord1, GL_MAP2_TEXTURE_COORD_1);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord2, enable->Map2TextureCoord2, GL_MAP2_TEXTURE_COORD_2);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord3, enable->Map2TextureCoord3, GL_MAP2_TEXTURE_COORD_3);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord4, enable->Map2TextureCoord4, GL_MAP2_TEXTURE_COORD_4);
               TEST_AND_UPDATE(ctx->Eval.Map2Vertex3, enable->Map2Vertex3, GL_MAP2_VERTEX_3);
               TEST_AND_UPDATE(ctx->Eval.Map2Vertex4, enable->Map2Vertex4, GL_MAP2_VERTEX_4);
               TEST_AND_UPDATE(ctx->Transform.Normalize, enable->Normalize, GL_NORMALIZE);
               TEST_AND_UPDATE(ctx->Transform.RescaleNormals, enable->RescaleNormals, GL_RESCALE_NORMAL_EXT);
               TEST_AND_UPDATE(ctx->Pixel.PixelTextureEnabled, enable->PixelTexture, GL_POINT_SMOOTH);
               TEST_AND_UPDATE(ctx->Point.SmoothFlag, enable->PointSmooth, GL_POINT_SMOOTH);
               TEST_AND_UPDATE(ctx->Polygon.OffsetPoint, enable->PolygonOffsetPoint, GL_POLYGON_OFFSET_POINT);
               TEST_AND_UPDATE(ctx->Polygon.OffsetLine, enable->PolygonOffsetLine, GL_POLYGON_OFFSET_LINE);
               TEST_AND_UPDATE(ctx->Polygon.OffsetFill, enable->PolygonOffsetFill, GL_POLYGON_OFFSET_FILL);
               TEST_AND_UPDATE(ctx->Polygon.SmoothFlag, enable->PolygonSmooth, GL_POLYGON_SMOOTH);
               TEST_AND_UPDATE(ctx->Polygon.StippleFlag, enable->PolygonStipple, GL_POLYGON_STIPPLE);
               TEST_AND_UPDATE(ctx->Scissor.Enabled, enable->Scissor, GL_SCISSOR_TEST);
               TEST_AND_UPDATE(ctx->Stencil.Enabled, enable->Stencil, GL_STENCIL_TEST);
               if (ctx->Texture.Enabled != enable->Texture) {
                  ctx->Texture.Enabled = enable->Texture;
                  if (ctx->Driver.Enable) {
                     if (ctx->Driver.ActiveTexture)
                        (*ctx->Driver.ActiveTexture)( ctx, 0 );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_1D, (GLboolean) (enable->Texture & TEXTURE0_1D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_2D, (GLboolean) (enable->Texture & TEXTURE0_2D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_3D, (GLboolean) (enable->Texture & TEXTURE0_3D) );
                     if (ctx->Driver.ActiveTexture)
                        (*ctx->Driver.ActiveTexture)( ctx, 1 );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_1D, (GLboolean) (enable->Texture & TEXTURE1_1D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_2D, (GLboolean) (enable->Texture & TEXTURE1_2D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_3D, (GLboolean) (enable->Texture & TEXTURE1_3D) );
                     if (ctx->Driver.ActiveTexture)
                        (*ctx->Driver.ActiveTexture)( ctx, ctx->Texture.CurrentUnit );
                  }
               }
#undef TEST_AND_UPDATE
               {
                  GLuint i;
                  for (i=0; i<MAX_TEXTURE_UNITS; i++) {
                     if (ctx->Texture.Unit[i].TexGenEnabled != enable->TexGen[i]) {
                        ctx->Texture.Unit[i].TexGenEnabled = enable->TexGen[i];

			/* ctx->Enabled recalculated in state change
                           processing */
			
                        if (ctx->Driver.Enable) {
                           if (ctx->Driver.ActiveTexture)
                              (*ctx->Driver.ActiveTexture)( ctx, i );
                           if (enable->TexGen[i] & S_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_S, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_S, GL_FALSE);
                           if (enable->TexGen[i] & T_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_T, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_T, GL_FALSE);
                           if (enable->TexGen[i] & R_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_R, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_R, GL_FALSE);
                           if (enable->TexGen[i] & Q_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_Q, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_Q, GL_FALSE);
                        }
                     }
                  }
                  if (ctx->Driver.ActiveTexture)
                     (*ctx->Driver.ActiveTexture)( ctx, ctx->Texture.CurrentUnit );
               }
            }
            break;
         case GL_EVAL_BIT:
            MEMCPY( &ctx->Eval, attr->data, sizeof(struct gl_eval_attrib) );
            break;
         case GL_FOG_BIT:
            {
               GLboolean anyChange = (GLboolean) (memcmp( &ctx->Fog, attr->data, sizeof(struct gl_fog_attrib) ) != 0);
               MEMCPY( &ctx->Fog, attr->data, sizeof(struct gl_fog_attrib) );
               if (anyChange && ctx->Driver.Fogfv) {
                  const GLfloat mode = (GLfloat) ctx->Fog.Mode;
                  const GLfloat density = ctx->Fog.Density;
                  const GLfloat start = ctx->Fog.Start;
                  const GLfloat end = ctx->Fog.End;
                  const GLfloat index = ctx->Fog.Index;
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_MODE, &mode);
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_DENSITY, &density );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_START, &start );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_END, &end );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_INDEX, &index );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_COLOR, ctx->Fog.Color );
               }
	       ctx->Enabled &= ~ENABLE_FOG;
	       if (ctx->Fog.Enabled) ctx->Enabled |= ENABLE_FOG;
            }
            break;
         case GL_HINT_BIT:
            MEMCPY( &ctx->Hint, attr->data, sizeof(struct gl_hint_attrib) );
            if (ctx->Driver.Hint) {
               (*ctx->Driver.Hint)( ctx, GL_PERSPECTIVE_CORRECTION_HINT,
                                    ctx->Hint.PerspectiveCorrection );
               (*ctx->Driver.Hint)( ctx, GL_POINT_SMOOTH_HINT,
                                    ctx->Hint.PointSmooth);
               (*ctx->Driver.Hint)( ctx, GL_LINE_SMOOTH_HINT,
                                    ctx->Hint.LineSmooth );
               (*ctx->Driver.Hint)( ctx, GL_POLYGON_SMOOTH_HINT,
                                    ctx->Hint.PolygonSmooth );
               (*ctx->Driver.Hint)( ctx, GL_FOG_HINT, ctx->Hint.Fog );
            }
            break;
         case GL_LIGHTING_BIT:
            MEMCPY( &ctx->Light, attr->data, sizeof(struct gl_light_attrib) );
            if (ctx->Driver.Enable) {
               GLuint i;
               for (i = 0; i < MAX_LIGHTS; i++) {
                  GLenum light = (GLenum) (GL_LIGHT0 + i);
                  (*ctx->Driver.Enable)( ctx, light, ctx->Light.Light[i].Enabled );
               }
               (*ctx->Driver.Enable)( ctx, GL_LIGHTING, ctx->Light.Enabled );
            }
            if (ctx->Light.ShadeModel == GL_FLAT)
               ctx->TriangleCaps |= DD_FLATSHADE;
            else
               ctx->TriangleCaps &= ~DD_FLATSHADE;
            if (ctx->Driver.ShadeModel)
               (*ctx->Driver.ShadeModel)(ctx, ctx->Light.ShadeModel);
	    ctx->Enabled &= ~ENABLE_LIGHT;
	    if (ctx->Light.Enabled && !is_empty_list(&ctx->Light.EnabledList))
	       ctx->Enabled |= ENABLE_LIGHT;
            break;
         case GL_LINE_BIT:
            MEMCPY( &ctx->Line, attr->data, sizeof(struct gl_line_attrib) );
            if (ctx->Driver.Enable) {
               (*ctx->Driver.Enable)( ctx, GL_LINE_SMOOTH, ctx->Line.SmoothFlag );
               (*ctx->Driver.Enable)( ctx, GL_LINE_STIPPLE, ctx->Line.StippleFlag );
            }
            if (ctx->Driver.LineStipple)
               (*ctx->Driver.LineStipple)(ctx, ctx->Line.StippleFactor,
                                          ctx->Line.StipplePattern);
            if (ctx->Driver.LineWidth)
               (*ctx->Driver.LineWidth)(ctx, ctx->Line.Width);
            break;
         case GL_LIST_BIT:
            MEMCPY( &ctx->List, attr->data, sizeof(struct gl_list_attrib) );
            break;
         case GL_PIXEL_MODE_BIT:
            MEMCPY( &ctx->Pixel, attr->data, sizeof(struct gl_pixel_attrib) );
            break;
         case GL_POINT_BIT:
            MEMCPY( &ctx->Point, attr->data, sizeof(struct gl_point_attrib) );
            if (ctx->Driver.Enable)
               (*ctx->Driver.Enable)( ctx, GL_POINT_SMOOTH, ctx->Point.SmoothFlag );
            break;
         case GL_POLYGON_BIT:
            {
               GLenum oldFrontMode = ctx->Polygon.FrontMode;
               GLenum oldBackMode = ctx->Polygon.BackMode;
               MEMCPY( &ctx->Polygon, attr->data,
                       sizeof(struct gl_polygon_attrib) );
               if ((ctx->Polygon.FrontMode != oldFrontMode ||
                    ctx->Polygon.BackMode != oldBackMode) &&
                   ctx->Driver.PolygonMode) {
                  (*ctx->Driver.PolygonMode)( ctx, GL_FRONT, ctx->Polygon.FrontMode);
                  (*ctx->Driver.PolygonMode)( ctx, GL_BACK, ctx->Polygon.BackMode);
               }
	       if (ctx->Driver.CullFace)
		  ctx->Driver.CullFace( ctx, ctx->Polygon.CullFaceMode );

	       if (ctx->Driver.FrontFace)
		  ctx->Driver.FrontFace( ctx, ctx->Polygon.FrontFace );

               if (ctx->Driver.Enable)
                  (*ctx->Driver.Enable)( ctx, GL_POLYGON_SMOOTH, ctx->Polygon.SmoothFlag );
            }
            break;
	 case GL_POLYGON_STIPPLE_BIT:
	    MEMCPY( ctx->PolygonStipple, attr->data, 32*sizeof(GLuint) );
	    if (ctx->Driver.PolygonStipple) 
	       ctx->Driver.PolygonStipple( ctx, (const GLubyte *) attr->data );
	    break;
         case GL_SCISSOR_BIT:
            MEMCPY( &ctx->Scissor, attr->data,
		    sizeof(struct gl_scissor_attrib) );
            if (ctx->Driver.Enable)
               (*ctx->Driver.Enable)( ctx, GL_SCISSOR_TEST, ctx->Scissor.Enabled );
	    if (ctx->Driver.Scissor)
	       ctx->Driver.Scissor( ctx, ctx->Scissor.X, ctx->Scissor.Y,
				    ctx->Scissor.Width, ctx->Scissor.Height );
            break;
         case GL_STENCIL_BUFFER_BIT:
            MEMCPY( &ctx->Stencil, attr->data,
		    sizeof(struct gl_stencil_attrib) );
            if (ctx->Driver.StencilFunc)
               (*ctx->Driver.StencilFunc)( ctx, ctx->Stencil.Function,
                                   ctx->Stencil.Ref, ctx->Stencil.ValueMask);
            if (ctx->Driver.StencilMask)
               (*ctx->Driver.StencilMask)( ctx, ctx->Stencil.WriteMask );
            if (ctx->Driver.StencilOp)
               (*ctx->Driver.StencilOp)( ctx, ctx->Stencil.FailFunc,
                              ctx->Stencil.ZFailFunc, ctx->Stencil.ZPassFunc);
            if (ctx->Driver.ClearStencil)
               (*ctx->Driver.ClearStencil)( ctx, ctx->Stencil.Clear );
            if (ctx->Driver.Enable)
               (*ctx->Driver.Enable)( ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled );
	    ctx->TriangleCaps &= ~DD_STENCIL;
	    if (ctx->Stencil.Enabled)
	       ctx->TriangleCaps |= DD_STENCIL;

            break;
         case GL_TRANSFORM_BIT:
            MEMCPY( &ctx->Transform, attr->data,
		    sizeof(struct gl_transform_attrib) );
            if (ctx->Driver.Enable) {
               (*ctx->Driver.Enable)( ctx, GL_NORMALIZE, ctx->Transform.Normalize );
               (*ctx->Driver.Enable)( ctx, GL_RESCALE_NORMAL_EXT, ctx->Transform.RescaleNormals );
            }
	    ctx->Enabled &= ~(ENABLE_NORMALIZE|ENABLE_RESCALE);
	    if (ctx->Transform.Normalize) ctx->Enabled |= ENABLE_NORMALIZE;
	    if (ctx->Transform.RescaleNormals) ctx->Enabled |= ENABLE_RESCALE;
            break;
         case GL_TEXTURE_BIT:
            /* Take care of texture object reference counters */
            {
               GLuint u;
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
		  ctx->Texture.Unit[u].CurrentD[1]->RefCount--;
		  ctx->Texture.Unit[u].CurrentD[2]->RefCount--;
		  ctx->Texture.Unit[u].CurrentD[3]->RefCount--;
               }
               MEMCPY( &ctx->Texture, attr->data,
                       sizeof(struct gl_texture_attrib) );
               /* restore state of the currently bound texture objects */
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                  copy_texobj_state( ctx->Texture.Unit[u].CurrentD[1],
                                     &(ctx->Texture.Unit[u].Saved1D) );
                  copy_texobj_state( ctx->Texture.Unit[u].CurrentD[2],
                                     &(ctx->Texture.Unit[u].Saved2D) );
                  copy_texobj_state( ctx->Texture.Unit[u].CurrentD[3],
                                     &(ctx->Texture.Unit[u].Saved3D) );
                  copy_texobj_state( ctx->Texture.Unit[u].CurrentCubeMap,
                                     &(ctx->Texture.Unit[u].SavedCubeMap) );

                  gl_put_texobj_on_dirty_list( ctx, ctx->Texture.Unit[u].CurrentD[1] );
                  gl_put_texobj_on_dirty_list( ctx, ctx->Texture.Unit[u].CurrentD[2] );
                  gl_put_texobj_on_dirty_list( ctx, ctx->Texture.Unit[u].CurrentD[3] );
                  gl_put_texobj_on_dirty_list( ctx, ctx->Texture.Unit[u].CurrentCubeMap );

               }
            }
            break;
         case GL_VIEWPORT_BIT: 
	 {
	    struct gl_viewport_attrib *v = 
	       (struct gl_viewport_attrib *)attr->data;
	    
	    _mesa_Viewport( v->X, v->Y, v->Width, v->Height );
	    _mesa_DepthRange( v->Near, v->Far );
	    break;
	 }
         default:
            gl_problem( ctx, "Bad attrib flag in PopAttrib");
            break;
      }

      next = attr->next;
      FREE( attr->data );
      FREE( attr );
      attr = next;
   }

   ctx->NewState = NEW_ALL;
}


#define GL_CLIENT_PACK_BIT (1<<20)
#define GL_CLIENT_UNPACK_BIT (1<<21)


void
_mesa_PushClientAttrib(GLbitfield mask)
{
   struct gl_attrib_node *newnode;
   struct gl_attrib_node *head;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPushClientAttrib");

   if (ctx->ClientAttribStackDepth>=MAX_CLIENT_ATTRIB_STACK_DEPTH) {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushClientAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      struct gl_pixelstore_attrib *attr;
      /* packing attribs */
      attr = MALLOC_STRUCT( gl_pixelstore_attrib );
      MEMCPY( attr, &ctx->Pack, sizeof(struct gl_pixelstore_attrib) );
      newnode = new_attrib_node( GL_CLIENT_PACK_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
      /* unpacking attribs */
      attr = MALLOC_STRUCT( gl_pixelstore_attrib );
      MEMCPY( attr, &ctx->Unpack, sizeof(struct gl_pixelstore_attrib) );
      newnode = new_attrib_node( GL_CLIENT_UNPACK_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }
   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      struct gl_array_attrib *attr;
      attr = MALLOC_STRUCT( gl_array_attrib );
      MEMCPY( attr, &ctx->Array, sizeof(struct gl_array_attrib) );
      newnode = new_attrib_node( GL_CLIENT_VERTEX_ARRAY_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   ctx->ClientAttribStack[ctx->ClientAttribStackDepth] = head;
   ctx->ClientAttribStackDepth++;
}




void
_mesa_PopClientAttrib(void)
{
   struct gl_attrib_node *attr, *next;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPopClientAttrib");

   if (ctx->ClientAttribStackDepth==0) {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopClientAttrib" );
      return;
   }

   ctx->ClientAttribStackDepth--;
   attr = ctx->ClientAttribStack[ctx->ClientAttribStackDepth];

   while (attr) {
      switch (attr->kind) {
         case GL_CLIENT_PACK_BIT:
            MEMCPY( &ctx->Pack, attr->data,
                    sizeof(struct gl_pixelstore_attrib) );
            break;
         case GL_CLIENT_UNPACK_BIT:
            MEMCPY( &ctx->Unpack, attr->data,
                    sizeof(struct gl_pixelstore_attrib) );
            break;
         case GL_CLIENT_VERTEX_ARRAY_BIT:
            MEMCPY( &ctx->Array, attr->data,
		    sizeof(struct gl_array_attrib) );
            break;
         default:
            gl_problem( ctx, "Bad attrib flag in PopClientAttrib");
            break;
      }

      next = attr->next;
      FREE( attr->data );
      FREE( attr );
      attr = next;
   }

   ctx->NewState = NEW_ALL;
}



