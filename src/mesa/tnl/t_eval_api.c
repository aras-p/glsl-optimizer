
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell - original code
 *    Brian Paul - vertex program updates
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"
#include "math/m_eval.h"

#include "t_eval_api.h"
#include "t_imm_api.h"
#include "t_imm_alloc.h"
#include "t_imm_exec.h"


/* KW: If are compiling, we don't know whether eval will produce a
 *     vertex when it is run in the future.  If this is pure immediate
 *     mode, eval is a noop if neither vertex map is enabled.
 *
 *     Thus we need to have a check in the display list code or
 *     elsewhere for eval(1,2) vertices in the case where
 *     map(1,2)_vertex is disabled, and to purge those vertices from
 *     the vb.
 */
void GLAPIENTRY
_tnl_exec_EvalMesh1( GLenum mode, GLint i1, GLint i2 )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   GLfloat u, du;
   GLenum prim;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glEvalMesh1()");

   switch (mode) {
      case GL_POINT:
         prim = GL_POINTS;
         break;
      case GL_LINE:
         prim = GL_LINE_STRIP;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glEvalMesh1(mode)" );
         return;
   }

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map1Vertex4 && !ctx->Eval.Map1Vertex3 &&
       (!ctx->VertexProgram.Enabled || !ctx->Eval.Map1Attrib[VERT_ATTRIB_POS]))
      return;

   du = ctx->Eval.MapGrid1du;
   u = ctx->Eval.MapGrid1u1 + i1 * du;

   /* Need to turn off compilation -- this is already saved, and the
    * coordinates generated and the test above depend on state that
    * may change before the list is executed.
    *
    * TODO: Anaylse display lists to determine if this state is
    * constant.
    *
    * State to watch:
    *       - enabled maps
    *       - map state for each enabled map, including control points
    *       - grid state
    *
    * Could alternatively cache individual maps in arrays, rather than
    * building immediates.  
    */
   {
      GLboolean compiling = ctx->CompileFlag;
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      struct immediate *im = TNL_CURRENT_IM(ctx);
      GLboolean (*NotifyBegin)(GLcontext *ctx, GLenum p);

      NotifyBegin = tnl->Driver.NotifyBegin;
      tnl->Driver.NotifyBegin = 0;

      if (compiling) {
	 struct immediate *tmp = _tnl_alloc_immediate( ctx );
	 FLUSH_VERTICES( ctx, 0 );
	 SET_IMMEDIATE( ctx, tmp );
	 TNL_CURRENT_IM(ctx)->ref_count++;	 
	 ctx->CompileFlag = GL_FALSE;
      }

      _tnl_Begin( prim );
      for (i=i1;i<=i2;i++,u+=du) {
	 _tnl_eval_coord1f( ctx, u );
      }
      _tnl_end_ctx(ctx);

      /* Need this for replay *and* compile:
       */
      FLUSH_VERTICES( ctx, 0 );
      tnl->Driver.NotifyBegin = NotifyBegin;

      if (compiling) {
	 TNL_CURRENT_IM(ctx)->ref_count--;
	 ASSERT( TNL_CURRENT_IM(ctx)->ref_count == 0 );
	 _tnl_free_immediate( ctx, TNL_CURRENT_IM(ctx) );
	 SET_IMMEDIATE( ctx, im );
	 ctx->CompileFlag = GL_TRUE;
      }
   }
}



void GLAPIENTRY
_tnl_exec_EvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i, j;
   GLfloat u, du, v, dv, v1, u1;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glEvalMesh2()");

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map2Vertex4 && !ctx->Eval.Map2Vertex3 &&
       (!ctx->VertexProgram.Enabled || !ctx->Eval.Map2Attrib[VERT_ATTRIB_POS]))
      return;

   du = ctx->Eval.MapGrid2du;
   dv = ctx->Eval.MapGrid2dv;
   v1 = ctx->Eval.MapGrid2v1 + j1 * dv;
   u1 = ctx->Eval.MapGrid2u1 + i1 * du;

   /* Need to turn off compilation -- this is already saved, and the
    * coordinates generated and the test above depend on state that
    * may change before the list is executed.
    */
   {
      GLboolean compiling = ctx->CompileFlag;
      struct immediate *im = TNL_CURRENT_IM(ctx);
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      GLboolean (*NotifyBegin)(GLcontext *ctx, GLenum p);

      NotifyBegin = tnl->Driver.NotifyBegin;
      tnl->Driver.NotifyBegin = 0;

      if (compiling) {
	 struct immediate *tmp = _tnl_alloc_immediate( ctx );
	 FLUSH_VERTICES( ctx, 0 );
	 SET_IMMEDIATE( ctx, tmp );
	 TNL_CURRENT_IM(ctx)->ref_count++;	 
	 ctx->CompileFlag = GL_FALSE;
      }

      switch (mode) {
      case GL_POINT:
	 _tnl_Begin( GL_POINTS );
	 for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	    for (u=u1,i=i1;i<=i2;i++,u+=du) {
	       _tnl_eval_coord2f( ctx, u, v );
	    }
	 }
	 _tnl_end_ctx(ctx);
	 break;
      case GL_LINE:
	 for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	    _tnl_Begin( GL_LINE_STRIP );
	    for (u=u1,i=i1;i<=i2;i++,u+=du) {
	       _tnl_eval_coord2f( ctx, u, v );
	    }
	    _tnl_end_ctx(ctx);
	 }
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    _tnl_Begin( GL_LINE_STRIP );
	    for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	       _tnl_eval_coord2f( ctx, u, v );
	    }
	    _tnl_end_ctx(ctx);
	 }
	 break;
      case GL_FILL:
	 for (v=v1,j=j1;j<j2;j++,v+=dv) {
	    _tnl_Begin( GL_TRIANGLE_STRIP );
	    for (u=u1,i=i1;i<=i2;i++,u+=du) {
	       _tnl_eval_coord2f( ctx, u, v );
	       _tnl_eval_coord2f( ctx, u, v+dv );
	    }
	    _tnl_end_ctx(ctx);
	 }
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glEvalMesh2(mode)" );
	 return;
      }

      /* Need this for replay *and* compile:
       */
      FLUSH_VERTICES( ctx, 0 );
      tnl->Driver.NotifyBegin = NotifyBegin;
	 
      if (compiling) {
	 TNL_CURRENT_IM(ctx)->ref_count--;
	 _tnl_free_immediate( ctx, TNL_CURRENT_IM( ctx ) );
	 SET_IMMEDIATE( ctx, im );
	 ctx->CompileFlag = GL_TRUE;
      }
   }
}


void _tnl_eval_init( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->vtxfmt);
   vfmt->EvalMesh1 = _tnl_exec_EvalMesh1;
   vfmt->EvalMesh2 = _tnl_exec_EvalMesh2;
}
