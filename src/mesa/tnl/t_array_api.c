/* $Id: t_array_api.c,v 1.2 2001/01/08 21:56:00 keithw Exp $ */

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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "api_validate.h"
#include "context.h"
#include "macros.h"
#include "mmath.h"
#include "mem.h"
#include "state.h"
#include "mtypes.h"

#include "array_cache/acache.h"

#include "t_array_api.h"
#include "t_array_import.h"
#include "t_imm_api.h"
#include "t_imm_exec.h"
#include "t_context.h"
#include "t_pipeline.h"





void
_tnl_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   /* Check arguments, etc.
    */
   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   if (tnl->pipeline.build_state_changes)
      _tnl_validate_pipeline( ctx );

   if (!ctx->CompileFlag && count - start < ctx->Const.MaxArrayLockSize) {
      FLUSH_CURRENT( ctx, 0 );

      if (ctx->Array.LockCount)
      {
	 if (start < ctx->Array.LockFirst) start = ctx->Array.LockFirst;
	 if (count > ctx->Array.LockCount) count = ctx->Array.LockCount;
	 if (start >= count) return;

	 /* Locked drawarrays.  Reuse any previously transformed data.
	  */
	 _tnl_vb_bind_arrays( ctx, ctx->Array.LockFirst, ctx->Array.LockCount );
	 VB->FirstPrimitive = start;
	 VB->Primitive[start] = mode | PRIM_BEGIN | PRIM_END | PRIM_LAST;
	 VB->PrimitiveLength[start] = count - start;
	 _tnl_run_pipeline( ctx );
      } else {
	 /* The arrays are small enough to fit in a single VB; just bind
	  * them and go.  Any untransformed data will be copied on
	  * clipping.
	  * 
	  * Invalidate any locked data dependent on these arrays.
	  */
	 _tnl_vb_bind_arrays( ctx, start, count );
	 VB->FirstPrimitive = 0;
	 VB->Primitive[0] = mode | PRIM_BEGIN | PRIM_END | PRIM_LAST;
	 VB->PrimitiveLength[0] = count - start;	 
	 tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
	 _tnl_run_pipeline( ctx );
	 tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
      }
   } 
   else {
      /* Need to produce immediate structs, either for compiling or
       * because the array range is too large to process in a single
       * VB.  In GL_EXECUTE mode, this introduces two redundant
       * operations: producing the flag array and computing the orflag
       * of the flag array.
       */
#if 1
      if (_tnl_hard_begin( ctx, mode )) {
	 GLuint j;
	 for (j = 0 ; j < count ; ) {
	    struct immediate *IM = TNL_CURRENT_IM(ctx);
	    GLuint nr = MIN2( IMM_MAXDATA - IM->Start, count - j );
	    GLuint sf = IM->Flag[IM->Start];

	    _tnl_fill_immediate_drawarrays( ctx, IM, j, j+nr );

	    if (j == 0) IM->Flag[IM->Start] |= sf;

	    IM->Count = IM->Start + nr;
	    j += nr;

	    if (j == count)
	       _tnl_end( ctx );

	    _tnl_flush_immediate( IM );
	 }
      }
#else
      /* Simple alternative to above code.
       */
      if (_tnl_hard_begin( ctx, mode )) 
      {
	 GLuint i;
	 for (i=start;i<count;i++) {
	    _tnl_array_element( ctx, i );
	 }
	 _tnl_end( ctx );
      }
#endif
   }
}



static void _tnl_draw_range_elements( GLcontext *ctx, GLenum mode, 
				      GLuint start, GLuint end, 
				      GLsizei count, const GLuint *indices )
   
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   FLUSH_CURRENT( ctx, 0 );
   
   _tnl_vb_bind_arrays( ctx, start, end );

   tnl->vb.FirstPrimitive = 0;
   tnl->vb.Primitive[0] = mode | PRIM_BEGIN | PRIM_END | PRIM_LAST;
   tnl->vb.PrimitiveLength[0] = count;
   tnl->vb.Elts = (GLuint *)indices;

   if (ctx->Array.LockCount) 
      _tnl_run_pipeline( ctx );
   else {
      /* Note that arrays may have changed before/after execution.
       */
      tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
      _tnl_run_pipeline( ctx );
      tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
   }
}




static void _tnl_draw_elements( GLcontext *ctx, GLenum mode, GLsizei count, 
				const GLuint *indices)
{
#if 1
   /* Optimized code that fakes the effect of calling
    * _tnl_array_element for each index in the list.
    */
   if (_tnl_hard_begin( ctx, mode )) {
      GLuint i,j;
      for (j = 0 ; j < count ; ) {
	 struct immediate *IM = TNL_CURRENT_IM(ctx);
	 GLuint start = IM->Start;
	 GLuint nr = MIN2( IMM_MAXDATA - start, count - j ) + start;
	 GLuint sf = IM->Flag[start];
	 IM->FlushElt |= 1;

	 for (i = start ; i < nr ; i++) {
	    IM->Elt[i] = (GLuint) *indices++;
	    IM->Flag[i] = VERT_ELT;
	 }

	 if (j == 0) IM->Flag[start] |= sf;

	 IM->Count = nr;
	 j += nr - start;

	 if (j == count)
	    _tnl_end( ctx );

	 _tnl_flush_immediate( IM );
      }
   }
#else
   /* Simple version of the above code.
    */
   if (_tnl_hard_begin(ctx, mode)) {
      GLuint i;
      for (i = 0 ; i < count ; i++)
	 _tnl_array_element( ctx, indices[i] );
      _tnl_end( ctx );
   }
#endif
}


void
_tnl_DrawRangeElements(GLenum mode,
		       GLuint start, GLuint end,
		       GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint *ui_indices;

   /* Check arguments, etc.
    */
   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count, 
                                          type, indices ))
      return;

   if (tnl->pipeline.build_state_changes)
      _tnl_validate_pipeline( ctx );

   ui_indices = (GLuint *)_ac_import_elements( ctx, GL_UNSIGNED_INT, 
					       count, type, indices );
            

   if (ctx->Array.LockCount) {
      /* Are the arrays already locked?  If so we currently have to look
       * at the whole locked range.
       */
      if (start >= ctx->Array.LockFirst && end <= ctx->Array.LockCount)
	 _tnl_draw_range_elements( ctx, mode, 
				   ctx->Array.LockFirst, 
				   ctx->Array.LockCount,
				   count, ui_indices );
      else {
	 /* The spec says referencing elements outside the locked
	  * range is undefined.  I'm going to make it a noop this time
	  * round, maybe come up with something beter before 3.6.  
	  *
	  * May be able to get away with just setting LockCount==0,
	  * though this raises the problems of dependent state.  May
	  * have to call glUnlockArrays() directly?
	  */
	 gl_problem( ctx, 
		     "DrawRangeElements references "
		     "elements outside locked range.");
      }
   }
   else if (end - start < ctx->Const.MaxArrayLockSize) {
      /* The arrays aren't locked but we can still fit them inside a single
       * vertexbuffer.
       */
      _tnl_draw_range_elements( ctx, mode, start, end, count, ui_indices );
   } else {
      /* Range is too big to optimize:
       */
      _tnl_draw_elements( ctx, mode, count, ui_indices );
   }
}



void
_tnl_DrawElements(GLenum mode, GLsizei count, GLenum type, 
		  const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint *ui_indices;

   /* Check arguments, etc.
    */
   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
      return;

   if (tnl->pipeline.build_state_changes)
      _tnl_validate_pipeline( ctx );

   ui_indices = (GLuint *)_ac_import_elements( ctx, GL_UNSIGNED_INT, 
					       count, type, indices );
            
   if (ctx->Array.LockCount) {
      _tnl_draw_range_elements( ctx, mode, 
				ctx->Array.LockFirst,
				ctx->Array.LockCount,
				count, ui_indices );
   } 
   else {
      /* Scan the index list and see if we can use the locked path anyway.
       */
      GLuint max_elt = 0;
      GLuint i;

      for (i = 0 ; i < count ; i++) 
	 if (ui_indices[i] > max_elt) max_elt = ui_indices[i];

      if (max_elt < ctx->Const.MaxArrayLockSize && /* can we use it? */
	  max_elt < count)	                   /* do we want to use it? */
	 _tnl_draw_range_elements( ctx, mode, 0, max_elt, count, ui_indices );
      else
	 _tnl_draw_elements( ctx, mode, count, ui_indices );
   }
}


void _tnl_array_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_arrays *tmp = &tnl->array_inputs;
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->vtxfmt);
   GLuint i;

   vfmt->DrawArrays = _tnl_DrawArrays;
   vfmt->DrawElements = _tnl_DrawElements;
   vfmt->DrawRangeElements = _tnl_DrawRangeElements;

   /* Setup vector pointers that will be used to bind arrays to VB's.
    */
   gl_vector4f_init( &tmp->Obj, 0, 0 );
   gl_vector3f_init( &tmp->Normal, 0, 0 );
   gl_vector4ub_init( &tmp->Color, 0, 0 );
   gl_vector4ub_init( &tmp->SecondaryColor, 0, 0 );
   gl_vector1f_init( &tmp->FogCoord, 0, 0 );
   gl_vector1ui_init( &tmp->Index, 0, 0 );
   gl_vector1ub_init( &tmp->EdgeFlag, 0, 0 );

   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) 
      gl_vector4f_init( &tmp->TexCoord[i], 0, 0);

   tnl->tmp_primitive = (GLuint *)MALLOC(sizeof(GLuint)*tnl->vb.Size);
   tnl->tmp_primitive_length = (GLuint *)MALLOC(sizeof(GLuint)*tnl->vb.Size);
}


void _tnl_array_destroy( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   if (tnl->tmp_primitive_length) FREE(tnl->tmp_primitive_length);
   if (tnl->tmp_primitive) FREE(tnl->tmp_primitive);
}
