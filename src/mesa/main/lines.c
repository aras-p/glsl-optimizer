/* $Id: lines.c,v 1.29 2001/03/12 00:48:38 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
#include "depth.h"
#include "lines.h"
#include "macros.h"
#include "mmath.h"
#include "texstate.h"
#include "mtypes.h"
#endif



void
_mesa_LineWidth( GLfloat width )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (width<=0.0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glLineWidth" );
      return;
   }

   if (ctx->Line.Width == width)
      return;

   FLUSH_VERTICES(ctx, _NEW_LINE);
   ctx->Line.Width = width;
   ctx->Line._Width = CLAMP(width,
			    ctx->Const.MinLineWidth,
			    ctx->Const.MaxLineWidth);


   if (width != 1.0)
      ctx->_TriangleCaps |= DD_LINE_WIDTH;
   else
      ctx->_TriangleCaps &= ~DD_LINE_WIDTH;

   if (ctx->Driver.LineWidth)
      (*ctx->Driver.LineWidth)(ctx, width);
}



void
_mesa_LineStipple( GLint factor, GLushort pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   factor = CLAMP( factor, 1, 256 );

   if (ctx->Line.StippleFactor == factor &&
       ctx->Line.StipplePattern == pattern)
      return;

   FLUSH_VERTICES(ctx, _NEW_LINE);
   ctx->Line.StippleFactor = factor;
   ctx->Line.StipplePattern = pattern;

   if (ctx->Driver.LineStipple)
      ctx->Driver.LineStipple( ctx, factor, pattern );
}
