/* $Id: debug.c,v 1.12 2001/03/29 21:16:25 keithw Exp $ */

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

#include "mtypes.h"
#include "debug.h"

void _mesa_print_state( const char *msg, GLuint state )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & _NEW_MODELVIEW)       ? "ctx->ModelView, " : "",
	   (state & _NEW_PROJECTION)      ? "ctx->Projection, " : "",
	   (state & _NEW_TEXTURE_MATRIX)  ? "ctx->TextureMatrix, " : "",
	   (state & _NEW_COLOR_MATRIX)    ? "ctx->ColorMatrix, " : "",
	   (state & _NEW_ACCUM)           ? "ctx->Accum, " : "",
	   (state & _NEW_COLOR)           ? "ctx->Color, " : "",
	   (state & _NEW_DEPTH)           ? "ctx->Depth, " : "",
	   (state & _NEW_EVAL)            ? "ctx->Eval/EvalMap, " : "",
	   (state & _NEW_FOG)             ? "ctx->Fog, " : "",
	   (state & _NEW_HINT)            ? "ctx->Hint, " : "",
	   (state & _NEW_LIGHT)           ? "ctx->Light, " : "",
	   (state & _NEW_LINE)            ? "ctx->Line, " : "",
	   (state & _NEW_PIXEL)           ? "ctx->Pixel, " : "",
	   (state & _NEW_POINT)           ? "ctx->Point, " : "",
	   (state & _NEW_POLYGON)         ? "ctx->Polygon, " : "",
	   (state & _NEW_POLYGONSTIPPLE)  ? "ctx->PolygonStipple, " : "",
	   (state & _NEW_SCISSOR)         ? "ctx->Scissor, " : "",
	   (state & _NEW_TEXTURE)         ? "ctx->Texture, " : "",
	   (state & _NEW_TRANSFORM)       ? "ctx->Transform, " : "",
	   (state & _NEW_VIEWPORT)        ? "ctx->Viewport, " : "",
	   (state & _NEW_PACKUNPACK)      ? "ctx->Pack/Unpack, " : "",
	   (state & _NEW_ARRAY)           ? "ctx->Array, " : "",
	   (state & _NEW_RENDERMODE)      ? "ctx->RenderMode, " : "",
	   (state & _NEW_BUFFERS)         ? "ctx->Visual, ctx->DrawBuffer,, " : "");
}



void _mesa_print_tri_caps( const char *name, GLuint flags )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   name,
	   flags,
	   (flags & DD_FLATSHADE)           ? "flat-shade, " : "",
	   (flags & DD_SEPARATE_SPECULAR)   ? "separate-specular, " : "",
	   (flags & DD_TRI_LIGHT_TWOSIDE)   ? "tri-light-twoside, " : "",
	   (flags & DD_TRI_UNFILLED)        ? "tri-unfilled, " : "",
	   (flags & DD_TRI_STIPPLE)         ? "tri-stipple, " : "",
	   (flags & DD_TRI_OFFSET)          ? "tri-offset, " : "",
	   (flags & DD_TRI_SMOOTH)          ? "tri-smooth, " : "",
	   (flags & DD_LINE_SMOOTH)         ? "line-smooth, " : "",
	   (flags & DD_LINE_STIPPLE)        ? "line-stipple, " : "",
	   (flags & DD_LINE_WIDTH)          ? "line-wide, " : "",
	   (flags & DD_POINT_SMOOTH)        ? "point-smooth, " : "",
	   (flags & DD_POINT_SIZE)          ? "point-size, " : "",
	   (flags & DD_POINT_ATTEN)         ? "point-atten, " : "",
	   (flags & DD_TRI_CULL_FRONT_BACK) ? "cull-all, " : ""
      );
}
