/* $Id: lines.c,v 1.20 2000/10/31 18:09:44 keithw Exp $ */

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
#include "depth.h"
#include "feedback.h"
#include "lines.h"
#include "macros.h"
#include "mmath.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#endif



void
_mesa_LineWidth( GLfloat width )
{
   GET_CURRENT_CONTEXT(ctx);
   if (width<=0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glLineWidth" );
      return;
   }
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLineWidth");
   
   if (ctx->Line.Width != width) {
      ctx->Line.Width = width;
      ctx->TriangleCaps &= ~DD_LINE_WIDTH;
      if (width != 1.0) ctx->TriangleCaps |= DD_LINE_WIDTH;

      ctx->NewState |= _NEW_LINE;

      if (ctx->Driver.LineWidth)
         (*ctx->Driver.LineWidth)(ctx, width);
   }
}



void
_mesa_LineStipple( GLint factor, GLushort pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLineStipple");
   ctx->Line.StippleFactor = CLAMP( factor, 1, 256 );
   ctx->Line.StipplePattern = pattern;

   ctx->NewState |= _NEW_LINE;

   if (ctx->Driver.LineStipple)
      ctx->Driver.LineStipple( ctx, factor, pattern );
}


