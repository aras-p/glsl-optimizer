/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Tungsten Graphics   All Rights Reserved.
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
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file t_vp_build.c
 * Create a vertex program to execute the current fixed function T&L pipeline.
 * \author Keith Whitwell
 */


#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "brw_vs.h"
#include "brw_state.h"


static void prepare_active_vertprog( struct brw_context *brw )
{
   const struct gl_vertex_program *prev = brw->vertex_program;

   brw->vertex_program = brw->attribs.VertexProgram->_Current;

   if (brw->vertex_program != prev) 
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
}



const struct brw_tracked_state brw_active_vertprog = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = 0,
      .cache = 0
   },
   .prepare = prepare_active_vertprog
};
