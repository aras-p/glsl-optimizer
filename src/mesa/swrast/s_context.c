/* $Id: s_context.c,v 1.10 2001/01/13 07:13:28 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "mtypes.h"
#include "mem.h"

#include "s_pb.h"
#include "s_points.h"
#include "s_lines.h"
#include "s_triangle.h"
#include "s_quads.h"
#include "s_blend.h"
#include "s_context.h"
#include "s_texture.h"





/*
 * Recompute the value of swrast->_RasterMask, etc. according to
 * the current context.
 */
static void
_swrast_update_rasterflags( GLcontext *ctx )
{
   GLuint RasterMask = 0;

   if (ctx->Color.AlphaEnabled)           RasterMask |= ALPHATEST_BIT;
   if (ctx->Color.BlendEnabled)           RasterMask |= BLEND_BIT;
   if (ctx->Depth.Test)                   RasterMask |= DEPTH_BIT;
   if (ctx->Fog.Enabled)                  RasterMask |= FOG_BIT;
   if (ctx->Scissor.Enabled)              RasterMask |= SCISSOR_BIT;
   if (ctx->Stencil.Enabled)              RasterMask |= STENCIL_BIT;
   if (ctx->Visual.RGBAflag) {
      const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
      if (colorMask != 0xffffffff)        RasterMask |= MASKING_BIT;
      if (ctx->Color.ColorLogicOpEnabled) RasterMask |= LOGIC_OP_BIT;
      if (ctx->Texture._ReallyEnabled)     RasterMask |= TEXTURE_BIT;
   }
   else {
      if (ctx->Color.IndexMask != 0xffffffff) RasterMask |= MASKING_BIT;
      if (ctx->Color.IndexLogicOpEnabled)     RasterMask |= LOGIC_OP_BIT;
   }

   if (ctx->DrawBuffer->UseSoftwareAlphaBuffers
       && ctx->Color.ColorMask[ACOMP]
       && ctx->Color.DrawBuffer != GL_NONE)
      RasterMask |= ALPHABUF_BIT;

   if (   ctx->Viewport.X < 0
       || ctx->Viewport.X + ctx->Viewport.Width > ctx->DrawBuffer->Width
       || ctx->Viewport.Y < 0
       || ctx->Viewport.Y + ctx->Viewport.Height > ctx->DrawBuffer->Height) {
      RasterMask |= WINCLIP_BIT;
   }

   if (ctx->Depth.OcclusionTest)
      RasterMask |= OCCLUSION_BIT;


   /* If we're not drawing to exactly one color buffer set the
    * MULTI_DRAW_BIT flag.  Also set it if we're drawing to no
    * buffers or the RGBA or CI mask disables all writes.
    */
   if (ctx->Color.MultiDrawBuffer) {
      RasterMask |= MULTI_DRAW_BIT;
   }
   else if (ctx->Color.DrawBuffer==GL_NONE) {
      RasterMask |= MULTI_DRAW_BIT;
   }
   else if (ctx->Visual.RGBAflag && *((GLuint *) ctx->Color.ColorMask) == 0) {
      RasterMask |= MULTI_DRAW_BIT; /* all RGBA channels disabled */
   }
   else if (!ctx->Visual.RGBAflag && ctx->Color.IndexMask==0) {
      RasterMask |= MULTI_DRAW_BIT; /* all color index bits disabled */
   }

   if (   ctx->Viewport.X<0
       || ctx->Viewport.X + ctx->Viewport.Width > ctx->DrawBuffer->Width
       || ctx->Viewport.Y<0
       || ctx->Viewport.Y + ctx->Viewport.Height > ctx->DrawBuffer->Height) {
      RasterMask |= WINCLIP_BIT;
   }

   SWRAST_CONTEXT(ctx)->_RasterMask = RasterMask;
}


static void
_swrast_update_polygon( GLcontext *ctx )
{
   GLfloat backface_sign = 1;

   if (ctx->Polygon.CullFlag) {
      backface_sign = 1;
      switch(ctx->Polygon.CullFaceMode) {
      case GL_BACK:
	 if(ctx->Polygon.FrontFace==GL_CCW)
	    backface_sign = -1;
	 break;
      case GL_FRONT:
	 if(ctx->Polygon.FrontFace!=GL_CCW)
	    backface_sign = -1;
	 break;
      default:
      case GL_FRONT_AND_BACK:
	 backface_sign = 0;
	 break;
      }
   }
   else {
      backface_sign = 0;
   }

   SWRAST_CONTEXT(ctx)->_backface_sign = backface_sign;
}


static void
_swrast_update_hint( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   swrast->_PreferPixelFog = (!swrast->AllowVertexFog ||
			      (ctx->Hint.Fog == GL_NICEST &&
			       swrast->AllowPixelFog));
}

#define _SWRAST_NEW_TRIANGLE (_NEW_RENDERMODE|		\
                              _NEW_POLYGON|		\
                              _NEW_DEPTH|		\
                              _NEW_STENCIL|		\
                              _NEW_COLOR|		\
                              _NEW_TEXTURE|		\
                              _NEW_HINT|		\
                              _SWRAST_NEW_RASTERMASK|	\
                              _NEW_LIGHT|		\
                              _NEW_FOG)

#define _SWRAST_NEW_LINE (_NEW_RENDERMODE|	\
                          _NEW_LINE|		\
                          _NEW_TEXTURE|		\
                          _NEW_LIGHT|		\
                          _NEW_FOG|		\
                          _NEW_DEPTH)

#define _SWRAST_NEW_POINT (_NEW_RENDERMODE |	\
			   _NEW_POINT |		\
			   _NEW_TEXTURE |	\
			   _NEW_LIGHT |		\
			   _NEW_FOG)

#define _SWRAST_NEW_QUAD  0

#define _SWRAST_NEW_TEXTURE_SAMPLE_FUNC _NEW_TEXTURE

#define _SWRAST_NEW_BLEND_FUNC _NEW_COLOR



/* Stub for swrast->Triangle to select a true triangle function
 * after a state change.
 */
static void
_swrast_validate_quad( GLcontext *ctx,
		       const SWvertex *v0, const SWvertex *v1,
                       const SWvertex *v2, const SWvertex *v3 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_quad( ctx );

   swrast->Quad( ctx, v0, v1, v2, v3 );
}

static void
_swrast_validate_triangle( GLcontext *ctx,
			   const SWvertex *v0,
                           const SWvertex *v1,
                           const SWvertex *v2 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_triangle( ctx );

   swrast->Triangle( ctx, v0, v1, v2 );
}

static void
_swrast_validate_line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_line( ctx );

   swrast->Line( ctx, v0, v1 );
}

static void
_swrast_validate_point( GLcontext *ctx, const SWvertex *v0 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_point( ctx );

   swrast->Point( ctx, v0 );
}

static void
_swrast_validate_blend_func( GLcontext *ctx, GLuint n,
			     const GLubyte mask[],
			     GLchan src[][4],
			     CONST GLchan dst[][4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   _swrast_choose_blend_func( ctx );

   swrast->BlendFunc( ctx, n, mask, src, dst );
}


static void
_swrast_validate_texture_sample( GLcontext *ctx, GLuint texUnit,
				 const struct gl_texture_object *tObj,
				 GLuint n,
				 const GLfloat s[], const GLfloat t[],
				 const GLfloat u[], const GLfloat lambda[],
				 GLchan rgba[][4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   _swrast_choose_texture_sample_func( ctx, texUnit, tObj );

   swrast->TextureSample[texUnit]( ctx, texUnit, tObj, n, s, t, u,
				   lambda, rgba );
}


static void
_swrast_sleep( GLcontext *ctx, GLuint new_state )
{
}


static void
_swrast_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;

   swrast->NewState |= new_state;

   /* After 10 statechanges without any swrast functions being called,
    * put the module to sleep.
    */
   if (++swrast->StateChanges > 10) {
      swrast->InvalidateState = _swrast_sleep;
      swrast->NewState = ~0;
      new_state = ~0;
   }

   if (new_state & swrast->invalidate_triangle)
      swrast->Triangle = _swrast_validate_triangle;

   if (new_state & swrast->invalidate_line)
      swrast->Line = _swrast_validate_line;

   if (new_state & swrast->invalidate_point)
      swrast->Point = _swrast_validate_point;

   if (new_state & swrast->invalidate_quad)
      swrast->Quad = _swrast_validate_quad;

   if (new_state & _SWRAST_NEW_BLEND_FUNC)
      swrast->BlendFunc = _swrast_validate_blend_func;

   if (new_state & _SWRAST_NEW_TEXTURE_SAMPLE_FUNC)
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
	 swrast->TextureSample[i] = _swrast_validate_texture_sample;
}



void
_swrast_validate_derived( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (swrast->NewState)
   {
      if (swrast->NewState & _SWRAST_NEW_RASTERMASK)
 	 _swrast_update_rasterflags( ctx );

      if (swrast->NewState & _NEW_TEXTURE)
	 swrast->_MultiTextureEnabled = (ctx->Texture._ReallyEnabled &
					 ~TEXTURE0_ANY);

      if (swrast->NewState & _NEW_POLYGON)
	 _swrast_update_polygon( ctx );

      if (swrast->NewState & _NEW_HINT)
	 _swrast_update_hint( ctx );

      swrast->NewState = 0;
      swrast->StateChanges = 0;
      swrast->InvalidateState = _swrast_invalidate_state;
   }
}



/* Public entrypoints:  See also s_accum.c, s_bitmap.c, etc.
 */
void
_swrast_Quad( GLcontext *ctx,
	      const SWvertex *v0, const SWvertex *v1,
              const SWvertex *v2, const SWvertex *v3 )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */
/*     _swrast_print_vertex( ctx, v0 ); */
/*     _swrast_print_vertex( ctx, v1 ); */
/*     _swrast_print_vertex( ctx, v2 ); */
/*     _swrast_print_vertex( ctx, v3 ); */
   SWRAST_CONTEXT(ctx)->Quad( ctx, v0, v1, v2, v3 );
}

void
_swrast_Triangle( GLcontext *ctx, const SWvertex *v0,
                  const SWvertex *v1, const SWvertex *v2 )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */
/*     _swrast_print_vertex( ctx, v0 ); */
/*     _swrast_print_vertex( ctx, v1 ); */
/*     _swrast_print_vertex( ctx, v2 ); */
   SWRAST_CONTEXT(ctx)->Triangle( ctx, v0, v1, v2 );
}

void
_swrast_Line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */
/*     _swrast_print_vertex( ctx, v0 ); */
/*     _swrast_print_vertex( ctx, v1 ); */
   SWRAST_CONTEXT(ctx)->Line( ctx, v0, v1 );
}

void
_swrast_Point( GLcontext *ctx, const SWvertex *v0 )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */
/*     _swrast_print_vertex( ctx, v0 ); */
   SWRAST_CONTEXT(ctx)->Point( ctx, v0 );
}

void
_swrast_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   SWRAST_CONTEXT(ctx)->InvalidateState( ctx, new_state );
}

void
_swrast_ResetLineStipple( GLcontext *ctx )
{
   SWRAST_CONTEXT(ctx)->StippleCounter = 0;
}

void
_swrast_allow_vertex_fog( GLcontext *ctx, GLboolean value )
{
   SWRAST_CONTEXT(ctx)->InvalidateState( ctx, _NEW_HINT );
   SWRAST_CONTEXT(ctx)->AllowVertexFog = value;
}

void
_swrast_allow_pixel_fog( GLcontext *ctx, GLboolean value )
{
   SWRAST_CONTEXT(ctx)->InvalidateState( ctx, _NEW_HINT );
   SWRAST_CONTEXT(ctx)->AllowPixelFog = value;
}


GLboolean
_swrast_CreateContext( GLcontext *ctx )
{
   GLuint i;
   SWcontext *swrast = (SWcontext *)CALLOC(sizeof(SWcontext));
   if (!swrast)
      return GL_FALSE;

   swrast->PB = gl_alloc_pb();
   if (!swrast->PB) {
      FREE(swrast);
      return GL_FALSE;
   }

   swrast->NewState = ~0;

   swrast->choose_point = _swrast_choose_point;
   swrast->choose_line = _swrast_choose_line;
   swrast->choose_triangle = _swrast_choose_triangle;
   swrast->choose_quad = _swrast_choose_quad;

   swrast->invalidate_point = _SWRAST_NEW_POINT;
   swrast->invalidate_line = _SWRAST_NEW_LINE;
   swrast->invalidate_triangle = _SWRAST_NEW_TRIANGLE;
   swrast->invalidate_quad = _SWRAST_NEW_QUAD;

   swrast->Point = _swrast_validate_point;
   swrast->Line = _swrast_validate_line;
   swrast->Triangle = _swrast_validate_triangle;
   swrast->Quad = _swrast_validate_quad;
   swrast->InvalidateState = _swrast_sleep;
   swrast->BlendFunc = _swrast_validate_blend_func;

   swrast->AllowVertexFog = GL_TRUE;
   swrast->AllowPixelFog = GL_TRUE;

   /* Optimized Accum buffer */
   swrast->_IntegerAccumMode = GL_TRUE;
   swrast->_IntegerAccumScaler = 0.0;


   for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++)
      swrast->TextureSample[i] = _swrast_validate_texture_sample;

   ctx->swrast_context = swrast;
   return GL_TRUE;
}

void
_swrast_DestroyContext( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   FREE( swrast->PB );
   FREE( swrast );

   ctx->swrast_context = 0;
}


void
_swrast_print_vertex( GLcontext *ctx, const SWvertex *v )
{
   GLuint i;

   fprintf(stderr, "win %f %f %f %f\n", 
	   v->win[0], v->win[1], v->win[2], v->win[3]);

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      fprintf(stderr, "texcoord[%d] %f %f %f %f\n", i,
	      v->texcoord[i][0], v->texcoord[i][1], 
	      v->texcoord[i][2], v->texcoord[i][3]);

   fprintf(stderr, "color %d %d %d %d\n", 
	   v->color[0], v->color[1], v->color[2], v->color[3]);
   fprintf(stderr, "spec %d %d %d %d\n",
	   v->specular[0], v->specular[1], v->specular[2], v->specular[3]);
   fprintf(stderr, "fog %f\n", v->fog);
   fprintf(stderr, "index %d\n", v->index);
   fprintf(stderr, "pointsize %f\n", v->pointSize);
   fprintf(stderr, "\n");
}
