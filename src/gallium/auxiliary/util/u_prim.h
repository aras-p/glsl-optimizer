/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_BLIT_H
#define U_BLIT_H


#include "pipe/p_defines.h"
#include "util/u_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

static INLINE boolean u_validate_pipe_prim( unsigned pipe_prim, unsigned nr )
{
   boolean ok = TRUE;

   switch (pipe_prim) {
   case PIPE_PRIM_POINTS:
      ok = (nr >= 1);
      break;
   case PIPE_PRIM_LINES:
      ok = (nr >= 2);
      break;
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_LINE_LOOP:
      ok = (nr >= 2);
      break;
   case PIPE_PRIM_TRIANGLES:
      ok = (nr >= 3);
      break;
   case PIPE_PRIM_TRIANGLE_STRIP:
   case PIPE_PRIM_TRIANGLE_FAN:
   case PIPE_PRIM_POLYGON:
      ok = (nr >= 3);
      break;
   case PIPE_PRIM_QUADS:
      ok = (nr >= 4);
      break;
   case PIPE_PRIM_QUAD_STRIP:
      ok = (nr >= 4);
      break;
   default:
      ok = 0;
      break;
   }

   return ok;
}


static INLINE boolean u_trim_pipe_prim( unsigned pipe_prim, unsigned *nr )
{
   boolean ok = TRUE;

   switch (pipe_prim) {
   case PIPE_PRIM_POINTS:
      ok = (*nr >= 1);
      break;
   case PIPE_PRIM_LINES:
      ok = (*nr >= 2);
      *nr -= (*nr % 2);
      break;
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_LINE_LOOP:
      ok = (*nr >= 2);
      break;
   case PIPE_PRIM_TRIANGLES:
      ok = (*nr >= 3);
      *nr -= (*nr % 3);
      break;
   case PIPE_PRIM_TRIANGLE_STRIP:
   case PIPE_PRIM_TRIANGLE_FAN:
   case PIPE_PRIM_POLYGON:
      ok = (*nr >= 3);
      break;
   case PIPE_PRIM_QUADS:
      ok = (*nr >= 4);
      *nr -= (*nr % 4);
      break;
   case PIPE_PRIM_QUAD_STRIP:
      ok = (*nr >= 4);
      *nr -= (*nr % 2);
      break;
   default:
      ok = 0;
      break;
   }

   if (!ok)
      *nr = 0;

   return ok;
}


static INLINE unsigned u_reduced_prim( unsigned pipe_prim )
{
   switch (pipe_prim) {
   case PIPE_PRIM_POINTS:
      return PIPE_PRIM_POINTS;

   case PIPE_PRIM_LINES:
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_LINE_LOOP:
      return PIPE_PRIM_LINES;

   default:
      return PIPE_PRIM_TRIANGLES;
   }
}

static INLINE unsigned
u_vertices_per_prim(int primitive)
{
   switch(primitive) {
   case PIPE_PRIM_POINTS:
      return 1;
   case PIPE_PRIM_LINES:
   case PIPE_PRIM_LINE_LOOP:
   case PIPE_PRIM_LINE_STRIP:
      return 2;
   case PIPE_PRIM_TRIANGLES:
   case PIPE_PRIM_TRIANGLE_STRIP:
   case PIPE_PRIM_TRIANGLE_FAN:
      return 3;
   case PIPE_PRIM_LINES_ADJACENCY:
   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
      return 4;
   case PIPE_PRIM_TRIANGLES_ADJACENCY:
   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      return 6;

   /* following primitives should never be used
    * with geometry shaders abd their size is
    * undefined */
   case PIPE_PRIM_POLYGON:
   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_QUAD_STRIP:
   default:
      debug_printf("Unrecognized geometry shader primitive");
      return 3;
   }
}

const char *u_prim_name( unsigned pipe_prim );

#endif
