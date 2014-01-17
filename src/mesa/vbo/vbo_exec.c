/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@vmware.com>
 */


#include "main/api_arrayelt.h"
#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/vtxfmt.h"
#include "vbo_context.h"



void vbo_exec_init( struct gl_context *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   exec->ctx = ctx;

   /* Initialize the arrayelt helper
    */
   if (!ctx->aelt_context &&
       !_ae_create_context( ctx )) 
      return;

   vbo_exec_vtx_init( exec );

   ctx->Driver.NeedFlush = 0;
   ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
   ctx->Driver.BeginVertices = vbo_exec_BeginVertices;
   ctx->Driver.FlushVertices = vbo_exec_FlushVertices;

   vbo_exec_invalidate_state( ctx, ~0 );
}


void vbo_exec_destroy( struct gl_context *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   vbo_exec_vtx_destroy( exec );
}


/**
 * Really want to install these callbacks to a central facility to be
 * invoked according to the state flags.  That will have to wait for a
 * mesa rework:
 */ 
void vbo_exec_invalidate_state( struct gl_context *ctx, GLuint new_state )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;

   if (!exec->validating && new_state & (_NEW_PROGRAM|_NEW_ARRAY)) {
      exec->array.recalculate_inputs = GL_TRUE;

      /* If we ended up here because a VAO was deleted, the _DrawArrays
       * pointer which pointed to the VAO might be invalid now, so set it
       * to NULL.  This prevents crashes in driver functions like Clear
       * where driver state validation might occur, but the vbo module is
       * still in an invalid state.
       *
       * Drivers should skip vertex array state validation if _DrawArrays
       * is NULL.  It also has no effect on performance, because attrib
       * bindings will be recalculated anyway.
       */
      if (vbo->last_draw_method == DRAW_ARRAYS) {
         ctx->Array._DrawArrays = NULL;
         vbo->last_draw_method = DRAW_NONE;
      }
   }

   if (new_state & _NEW_EVAL)
      exec->eval.recalculate_maps = GL_TRUE;

   _ae_invalidate_state(ctx, new_state);
}


/**
 * Figure out the number of transform feedback primitives that will be output
 * considering the drawing mode, number of vertices, and instance count,
 * assuming that no geometry shading is done and primitive restart is not
 * used.
 *
 * This is used by driver back-ends in implementing the PRIMITIVES_GENERATED
 * and TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN queries.  It is also used to
 * pre-validate draw calls in GLES3 (where draw calls only succeed if there is
 * enough room in the transform feedback buffer for the result).
 */
size_t
vbo_count_tessellated_primitives(GLenum mode, GLuint count,
                                 GLuint num_instances)
{
   size_t num_primitives;
   switch (mode) {
   case GL_POINTS:
      num_primitives = count;
      break;
   case GL_LINE_STRIP:
      num_primitives = count >= 2 ? count - 1 : 0;
      break;
   case GL_LINE_LOOP:
      num_primitives = count >= 2 ? count : 0;
      break;
   case GL_LINES:
      num_primitives = count / 2;
      break;
   case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      num_primitives = count >= 3 ? count - 2 : 0;
      break;
   case GL_TRIANGLES:
      num_primitives = count / 3;
      break;
   case GL_QUAD_STRIP:
      num_primitives = count >= 4 ? ((count / 2) - 1) * 2 : 0;
      break;
   case GL_QUADS:
      num_primitives = (count / 4) * 2;
      break;
   case GL_LINES_ADJACENCY:
      num_primitives = count / 4;
      break;
   case GL_LINE_STRIP_ADJACENCY:
      num_primitives = count >= 4 ? count - 3 : 0;
      break;
   case GL_TRIANGLES_ADJACENCY:
      num_primitives = count / 6;
      break;
   case GL_TRIANGLE_STRIP_ADJACENCY:
      num_primitives = count >= 6 ? (count - 4) / 2 : 0;
      break;
   default:
      assert(!"Unexpected primitive type in count_tessellated_primitives");
      num_primitives = 0;
      break;
   }
   return num_primitives * num_instances;
}



/**
 * In some degenarate cases we can improve our ability to merge
 * consecutive primitives.  For example:
 * glBegin(GL_LINE_STRIP);
 * glVertex(1);
 * glVertex(1);
 * glEnd();
 * glBegin(GL_LINE_STRIP);
 * glVertex(1);
 * glVertex(1);
 * glEnd();
 * Can be merged as a GL_LINES prim with four vertices.
 *
 * This function converts 2-vertex line strips/loops into GL_LINES, etc.
 */
void
vbo_try_prim_conversion(struct _mesa_prim *p)
{
   if (p->mode == GL_LINE_STRIP && p->count == 2) {
      /* convert 2-vertex line strip to a separate line */
      p->mode = GL_LINES;
   }
   else if ((p->mode == GL_TRIANGLE_STRIP || p->mode == GL_TRIANGLE_FAN)
       && p->count == 3) {
      /* convert 3-vertex tri strip or fan to a separate triangle */
      p->mode = GL_TRIANGLES;
   }

   /* Note: we can't convert a 4-vertex quad strip to a separate quad
    * because the vertex ordering is different.  We'd have to muck
    * around in the vertex data to make it work.
    */
}


/**
 * Helper function for determining if two subsequent glBegin/glEnd
 * primitives can be combined.  This is only possible for GL_POINTS,
 * GL_LINES, GL_TRIANGLES and GL_QUADS.
 * If we return true, it means that we can concatenate p1 onto p0 (and
 * discard p1).
 */
bool
vbo_can_merge_prims(const struct _mesa_prim *p0, const struct _mesa_prim *p1)
{
   if (!p0->begin ||
       !p1->begin ||
       !p0->end ||
       !p1->end)
      return false;

   /* The prim mode must match (ex: both GL_TRIANGLES) */
   if (p0->mode != p1->mode)
      return false;

   /* p1's vertices must come right after p0 */
   if (p0->start + p0->count != p1->start)
      return false;

   if (p0->basevertex != p1->basevertex ||
       p0->num_instances != p1->num_instances ||
       p0->base_instance != p1->base_instance)
      return false;

   /* can always merge subsequent GL_POINTS primitives */
   if (p0->mode == GL_POINTS)
      return true;

   /* independent lines with no extra vertices */
   if (p0->mode == GL_LINES && p0->count % 2 == 0 && p1->count % 2 == 0)
      return true;

   /* independent tris */
   if (p0->mode == GL_TRIANGLES && p0->count % 3 == 0 && p1->count % 3 == 0)
      return true;

   /* independent quads */
   if (p0->mode == GL_QUADS && p0->count % 4 == 0 && p1->count % 4 == 0)
      return true;

   return false;
}


/**
 * If we've determined that p0 and p1 can be merged, this function
 * concatenates p1 onto p0.
 */
void
vbo_merge_prims(struct _mesa_prim *p0, const struct _mesa_prim *p1)
{
   assert(vbo_can_merge_prims(p0, p1));

   p0->count += p1->count;
   p0->end = p1->end;
}
