/* $Id: t_array_api.c,v 1.15 2001/05/11 15:53:06 keithw Exp $ */

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

static void fallback_drawarrays( GLcontext *ctx, GLenum mode, GLint start,
				 GLsizei count )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   /* Need to produce immediate structs, either for compiling or
    * because the array range is too large to process in a single
    * VB.  In GL_EXECUTE mode, this introduces two redundant
    * operations: producing the flag array and computing the orflag
    * of the flag array.
    */
#if 1
   if (_tnl_hard_begin( ctx, mode )) {
      GLint i;
      for (i = 0 ; i < count ; ) {
	 struct immediate *IM = TNL_CURRENT_IM(ctx);
	 GLuint start = IM->Start;
	 GLuint nr = MIN2( IMM_MAXDATA - start, (GLuint) (count - i) );

	 _tnl_fill_immediate_drawarrays( ctx, IM, i, i+nr );

	 IM->Count = start + nr;
	 i += nr;

	 if (i == count)
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


static void fallback_drawelements( GLcontext *ctx, GLenum mode, GLsizei count,
				   const GLuint *indices)
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

#if 1
   /* Optimized code that fakes the effect of calling
    * _tnl_array_element for each index in the list.
    */
   if (_tnl_hard_begin( ctx, mode )) {
      GLint i, j;
      for (i = 0 ; i < count ; ) {
	 struct immediate *IM = TNL_CURRENT_IM(ctx);
	 GLuint start = IM->Start;
	 GLint end = MIN2( IMM_MAXDATA, (count - i) + start);
	 GLuint sf = IM->Flag[start];
	 IM->FlushElt = IM->ArrayEltFlush;

	 for (j = start ; j < end ; j++) {
	    IM->Elt[j] = (GLuint) *indices++;
	    IM->Flag[j] = VERT_ELT;
	 }

	 IM->Flag[start] |= (sf & IM->ArrayEltFlags);
	 IM->Count = end;
	 i += end - start;

	 if (i == count)
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
      tnl->Driver.RunPipeline( ctx );
   else {
      /* Note that arrays may have changed before/after execution.
       */
      tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
      tnl->Driver.RunPipeline( ctx );
      tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
   }
}




void
_tnl_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */
   
   /* Check arguments, etc.
    */
   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   if (tnl->pipeline.build_state_changes)
      _tnl_validate_pipeline( ctx );

   if (ctx->CompileFlag) {
      fallback_drawarrays( ctx, mode, start, count );
   }    
   else if (count - start < (GLint) ctx->Const.MaxArrayLockSize) {

      /* Small primitives which can fit in a single vertex buffer:
       */
      FLUSH_CURRENT( ctx, 0 );

      if (ctx->Array.LockCount)
      {
	 if (start < (GLint) ctx->Array.LockFirst)
            start = ctx->Array.LockFirst;
	 if (count > (GLint) ctx->Array.LockCount)
            count = ctx->Array.LockCount;
	 if (start >= count)
            return;

	 /* Locked drawarrays.  Reuse any previously transformed data.
	  */
	 _tnl_vb_bind_arrays( ctx, ctx->Array.LockFirst, ctx->Array.LockCount );
	 VB->FirstPrimitive = start;
	 VB->Primitive[start] = mode | PRIM_BEGIN | PRIM_END | PRIM_LAST;
	 VB->PrimitiveLength[start] = count - start;
	 tnl->Driver.RunPipeline( ctx );
      } else {
	 /* The arrays are small enough to fit in a single VB; just bind
	  * them and go.  Any untransformed data will be copied on
	  * clipping.
	  *
	  * Invalidate any cached data dependent on these arrays.
	  */
	 _tnl_vb_bind_arrays( ctx, start, count );
	 VB->FirstPrimitive = 0;
	 VB->Primitive[0] = mode | PRIM_BEGIN | PRIM_END | PRIM_LAST;
	 VB->PrimitiveLength[0] = count - start;
	 tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
	 tnl->Driver.RunPipeline( ctx );
	 tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
      }
   }
   else {
      int bufsz = (ctx->Const.MaxArrayLockSize - 4) & ~3;
      int j, nr;
      int minimum, modulo, skip;

      /* Large primitives requiring decomposition to multiple vertex
       * buffers:
       */
      switch (mode) {
      case GL_POINTS:
	 minimum = 0;
	 modulo = 1;
	 skip = 0;
      case GL_LINES:
	 minimum = 1;
	 modulo = 2;
	 skip = 1;
      case GL_LINE_STRIP:
	 minimum = 1;
	 modulo = 1;
	 skip = 0;
	 break;
      case GL_TRIANGLES:
	 minimum = 2;
	 modulo = 3;
	 skip = 2;
	 break;
      case GL_TRIANGLE_STRIP:
	 minimum = 2;
	 modulo = 1;
	 skip = 0;
	 break;
      case GL_QUADS:
	 minimum = 3;
	 modulo = 4;
	 skip = 3;
	 break;
      case GL_QUAD_STRIP:
	 minimum = 3;
	 modulo = 2;
	 skip = 0;
	 break;
      case GL_LINE_LOOP:
      case GL_TRIANGLE_FAN:
      case GL_POLYGON:
      default:
	 /* Primitives requiring a copied vertex (fan-like primitives)
	  * must use the slow path:
	  */
	 fallback_drawarrays( ctx, mode, start, count );
	 return;
      }

      FLUSH_CURRENT( ctx, 0 );

/*        fprintf(stderr, "start %d count %d min %d modulo %d skip %d\n", */
/*  	      start, count, minimum, modulo, skip); */

      
      bufsz -= bufsz % modulo;
      bufsz -= minimum;

      for (j = start + minimum ; j < count ; j += nr + skip ) {

	 nr = MIN2( bufsz, count - j );

/*  	 fprintf(stderr, "%d..%d\n", j - minimum, j+nr); */

	 _tnl_vb_bind_arrays( ctx, j - minimum, j + nr );

	 VB->FirstPrimitive = 0;
	 VB->Primitive[0] = mode | PRIM_BEGIN | PRIM_END | PRIM_LAST;
	 VB->PrimitiveLength[0] = nr + minimum;
	 tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
	 tnl->Driver.RunPipeline( ctx );
	 tnl->pipeline.run_input_changes |= ctx->Array._Enabled;
      }
   }
}


void
_tnl_DrawRangeElements(GLenum mode,
		       GLuint start, GLuint end,
		       GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint *ui_indices;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

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
	  *
	  * Or scan the list and replace bad indices?
	  */
	 _mesa_problem( ctx,
		     "DrawRangeElements references "
		     "elements outside locked range.");
      }
   }
   else if (end + 1 - start < ctx->Const.MaxArrayLockSize) {
      /* The arrays aren't locked but we can still fit them inside a
       * single vertexbuffer.
       */
      _tnl_draw_range_elements( ctx, mode, start, end + 1, count, ui_indices );
   } else {
      /* Range is too big to optimize:
       */
      fallback_drawelements( ctx, mode, count, ui_indices );
   }
}



void
_tnl_DrawElements(GLenum mode, GLsizei count, GLenum type,
		  const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint *ui_indices;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

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
      GLint i;

      for (i = 0 ; i < count ; i++)
	 if (ui_indices[i] > max_elt)
            max_elt = ui_indices[i];

      if (max_elt < ctx->Const.MaxArrayLockSize && /* can we use it? */
	  max_elt < (GLuint) count) 	           /* do we want to use it? */
	 _tnl_draw_range_elements( ctx, mode, 0, max_elt+1, count, ui_indices );
      else
	 fallback_drawelements( ctx, mode, count, ui_indices );
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
   _mesa_vector4f_init( &tmp->Obj, 0, 0 );
   _mesa_vector3f_init( &tmp->Normal, 0, 0 );   
   _mesa_vector1f_init( &tmp->FogCoord, 0, 0 );
   _mesa_vector1ui_init( &tmp->Index, 0, 0 );
   _mesa_vector1ub_init( &tmp->EdgeFlag, 0, 0 );

   for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
      _mesa_vector4f_init( &tmp->TexCoord[i], 0, 0);

   tnl->tmp_primitive = (GLuint *)MALLOC(sizeof(GLuint)*tnl->vb.Size);
   tnl->tmp_primitive_length = (GLuint *)MALLOC(sizeof(GLuint)*tnl->vb.Size);
}


void _tnl_array_destroy( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   if (tnl->tmp_primitive_length) FREE(tnl->tmp_primitive_length);
   if (tnl->tmp_primitive) FREE(tnl->tmp_primitive);
}
