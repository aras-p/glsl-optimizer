/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "glheader.h"
#include "context.h"
#include "state.h"
#include "api_validate.h"
#include "api_noop.h"
#include "dispatch.h"

#include "brw_attrib.h"
#include "brw_draw.h"
#include "brw_exec.h"
#include "brw_fallback.h"

static GLuint get_max_index( GLuint count, GLuint type, 
			     const GLvoid *indices )
{
   GLint i;

   /* Compute max element.  This is only needed for upload of non-VBO,
    * non-constant data elements.
    *
    * XXX: Postpone this calculation until it is known that it is
    * needed.  Otherwise could scan this pointlessly in the all-vbo
    * case.
    */
   switch(type) {
   case GL_UNSIGNED_INT: {
      const GLuint *ui_indices = (const GLuint *)indices;
      GLuint max_ui = 0;
      for (i = 0; i < count; i++)
	 if (ui_indices[i] > max_ui)
	    max_ui = ui_indices[i];
      return max_ui;
   }
   case GL_UNSIGNED_SHORT: {
      const GLushort *us_indices = (const GLushort *)indices;
      GLuint max_us = 0;
      for (i = 0; i < count; i++)
	 if (us_indices[i] > max_us)
	    max_us = us_indices[i];
      return max_us;
   }
   case GL_UNSIGNED_BYTE: {
      const GLubyte *ub_indices = (const GLubyte *)indices;
      GLuint max_ub = 0;
      for (i = 0; i < count; i++)
	 if (ub_indices[i] > max_ub)
	    max_ub = ub_indices[i];
      return max_ub;
   }
   default:
      return 0;
   }
}




/***********************************************************************
 * API functions.
 */

static void GLAPIENTRY
brw_exec_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
   struct brw_draw_prim prim[1];
   GLboolean ok;

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (ctx->NewState)
      _mesa_update_state( ctx );
      
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;

   if (exec->array.inputs[0]->BufferObj->Name) {
      /* Use vertex attribute as a hint to tell us if we expect all
       * arrays to be in VBO's and if so, don't worry about avoiding
       * the upload of elements < start.
       */
      prim[0].mode = mode;
      prim[0].start = start;
      prim[0].count = count;
      prim[0].indexed = 0;

      ok = brw_draw_prims( ctx, exec->array.inputs, prim, 1, NULL, 0, start + count, 0 );
   }
   else {
      /* If not using VBO's, we don't want to upload any more elements
       * than necessary from the arrays as they will not be valid next
       * time the application tries to draw with them.
       */
      prim[0].mode = mode;
      prim[0].start = 0;
      prim[0].count = count;
      prim[0].indexed = 0;

      ok = brw_draw_prims( ctx, exec->array.inputs, prim, 1, NULL, start, start + count, 0 );
   }
   
   if (!ok) {
      brw_fallback(ctx);
      CALL_DrawArrays(ctx->Exec, ( mode, start, count ));
      brw_unfallback(ctx);
   }
}



static void GLAPIENTRY
brw_exec_DrawRangeElements(GLenum mode,
			   GLuint start, GLuint end,
			   GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
   struct brw_draw_index_buffer ib;
   struct brw_draw_prim prim[1];

   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count, type, indices ))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (ctx->NewState)
      _mesa_update_state( ctx );
      
   ib.count = count;
   ib.type = type; 
   ib.obj = ctx->Array.ElementArrayBufferObj;
   ib.ptr = indices;

   if (ctx->Array.ElementArrayBufferObj->Name) {
      /* Use the fact that indices are in a VBO as a hint that the
       * program has put all the arrays in VBO's and we don't have to
       * worry about performance implications of start > 0.
       *
       * XXX: consider passing start as min_index to draw_prims instead.
       */
      ib.rebase = 0;
   }
   else {
      ib.rebase = start;
   }

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = 0;
   prim[0].count = count;
   prim[0].indexed = 1;

   if (!brw_draw_prims( ctx, exec->array.inputs, prim, 1, &ib, ib.rebase, end+1, 0 )) {
      brw_fallback(ctx);
      CALL_DrawRangeElements(ctx->Exec, (mode, start, end, count, type, indices));
      brw_unfallback(ctx);
   }
}


static void GLAPIENTRY
brw_exec_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint max_index;

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
      return;

   if (ctx->Array.ElementArrayBufferObj->Name) {
      const GLvoid *map = ctx->Driver.MapBuffer(ctx,
						 GL_ELEMENT_ARRAY_BUFFER_ARB,
						 GL_DYNAMIC_READ_ARB,
						 ctx->Array.ElementArrayBufferObj);

      max_index = get_max_index(count, type, ADD_POINTERS(map, indices));

      ctx->Driver.UnmapBuffer(ctx,
			      GL_ELEMENT_ARRAY_BUFFER_ARB,
			      ctx->Array.ElementArrayBufferObj);
   }
   else {
      max_index = get_max_index(count, type, indices);
   }

   brw_exec_DrawRangeElements(mode, 0, max_index, count, type, indices);
}


/***********************************************************************
 * Initialization
 */


static void init_arrays( GLcontext *ctx, 
			 const struct gl_client_array *arrays[] )
{
   struct gl_array_object *obj = ctx->Array.ArrayObj;
   GLuint i;

   memset(arrays, 0, sizeof(*arrays) * BRW_ATTRIB_MAX);

   arrays[BRW_ATTRIB_POS] = &obj->Vertex;
   arrays[BRW_ATTRIB_NORMAL] = &obj->Normal;
   arrays[BRW_ATTRIB_COLOR0] = &obj->Color;
   arrays[BRW_ATTRIB_COLOR1] = &obj->SecondaryColor;
   arrays[BRW_ATTRIB_FOG] = &obj->FogCoord;

   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++)
      arrays[BRW_ATTRIB_TEX0 + i] = &obj->TexCoord[i];

   arrays[BRW_ATTRIB_INDEX] = &obj->Index;
   arrays[BRW_ATTRIB_EDGEFLAG] = &obj->EdgeFlag;

   for (i = BRW_ATTRIB_GENERIC0; i <= BRW_ATTRIB_GENERIC15; i++)
      arrays[i] = &obj->VertexAttrib[i - BRW_ATTRIB_GENERIC0];
}




void brw_exec_array_init( struct brw_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;

   init_arrays(ctx, exec->array.inputs);

#if 1
   exec->vtxfmt.DrawArrays = brw_exec_DrawArrays;
   exec->vtxfmt.DrawElements = brw_exec_DrawElements;
   exec->vtxfmt.DrawRangeElements = brw_exec_DrawRangeElements;
#else
   exec->vtxfmt.DrawArrays = _mesa_noop_DrawArrays;
   exec->vtxfmt.DrawElements = _mesa_noop_DrawElements;
   exec->vtxfmt.DrawRangeElements = _mesa_noop_DrawRangeElements;
#endif

   exec->array.index_obj = ctx->Driver.NewBufferObject(ctx, 1, GL_ARRAY_BUFFER_ARB);
}


void brw_exec_array_destroy( struct brw_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;

   ctx->Driver.DeleteBuffer(ctx, exec->array.index_obj);
}
