
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


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "image.h"
#include "enums.h"
#include "macros.h"
#include "polygon.h"
#include "mtypes.h"


void
_mesa_CullFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glCullFace %s\n", _mesa_lookup_enum_by_nr(mode));

   if (mode!=GL_FRONT && mode!=GL_BACK && mode!=GL_FRONT_AND_BACK) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glCullFace" );
      return;
   }

   if (ctx->Polygon.CullFaceMode == mode)
      return;

   FLUSH_VERTICES(ctx, _NEW_POLYGON);
   ctx->Polygon.CullFaceMode = mode;

   if (ctx->Driver.CullFace)
      ctx->Driver.CullFace( ctx, mode );
}



void
_mesa_FrontFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glFrontFace %s\n", _mesa_lookup_enum_by_nr(mode));

   if (mode!=GL_CW && mode!=GL_CCW) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glFrontFace" );
      return;
   }

   if (ctx->Polygon.FrontFace == mode)
      return;

   FLUSH_VERTICES(ctx, _NEW_POLYGON);
   ctx->Polygon.FrontFace = mode;

   ctx->Polygon._FrontBit = (GLboolean) (mode == GL_CW);

   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace( ctx, mode );
}



void
_mesa_PolygonMode( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPolygonMode %s %s\n",
                  _mesa_lookup_enum_by_nr(face),
                  _mesa_lookup_enum_by_nr(mode));

   if (mode!=GL_POINT && mode!=GL_LINE && mode!=GL_FILL) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glPolygonMode(mode)" );
      return;
   }

   switch (face) {
   case GL_FRONT:
      if (ctx->Polygon.FrontMode == mode)
	 return;
      FLUSH_VERTICES(ctx, _NEW_POLYGON);
      ctx->Polygon.FrontMode = mode;
      break;
   case GL_FRONT_AND_BACK:
      if (ctx->Polygon.FrontMode == mode &&
	  ctx->Polygon.BackMode == mode)
	 return;
      FLUSH_VERTICES(ctx, _NEW_POLYGON);
      ctx->Polygon.FrontMode = mode;
      ctx->Polygon.BackMode = mode;
      break;
   case GL_BACK:
      if (ctx->Polygon.BackMode == mode)
	 return;
      FLUSH_VERTICES(ctx, _NEW_POLYGON);
      ctx->Polygon.BackMode = mode;
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glPolygonMode(face)" );
      return;
   }

   ctx->_TriangleCaps &= ~DD_TRI_UNFILLED;
   if (ctx->Polygon.FrontMode!=GL_FILL || ctx->Polygon.BackMode!=GL_FILL)
      ctx->_TriangleCaps |= DD_TRI_UNFILLED;

   if (ctx->Driver.PolygonMode) {
      (*ctx->Driver.PolygonMode)( ctx, face, mode );
   }
}



void
_mesa_PolygonStipple( const GLubyte *pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPolygonStipple\n");

   FLUSH_VERTICES(ctx, _NEW_POLYGONSTIPPLE);
   _mesa_unpack_polygon_stipple(pattern, ctx->PolygonStipple, &ctx->Unpack);

   if (ctx->Driver.PolygonStipple)
      ctx->Driver.PolygonStipple( ctx, (const GLubyte *) ctx->PolygonStipple );
}



void
_mesa_GetPolygonStipple( GLubyte *dest )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glGetPolygonStipple\n");

   _mesa_pack_polygon_stipple(ctx->PolygonStipple, dest, &ctx->Pack);
}



void
_mesa_PolygonOffset( GLfloat factor, GLfloat units )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glPolygonOffset %f %f\n", factor, units);

   if (ctx->Polygon.OffsetFactor == factor &&
       ctx->Polygon.OffsetUnits == units)
      return;

   FLUSH_VERTICES(ctx, _NEW_POLYGON);
   ctx->Polygon.OffsetFactor = factor;
   ctx->Polygon.OffsetUnits = units;

   if (ctx->Driver.PolygonOffset)
      ctx->Driver.PolygonOffset( ctx, factor, units );
}



void
_mesa_PolygonOffsetEXT( GLfloat factor, GLfloat bias )
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_PolygonOffset(factor, bias * ctx->DepthMaxF );
}
