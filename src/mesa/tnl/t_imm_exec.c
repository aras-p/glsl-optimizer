/* $Id: t_imm_exec.c,v 1.13 2001/02/20 18:28:52 keithw Exp $ */

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
 *   Keith Whitwell <keithw@valinux.com>
 */


#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "dlist.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "light.h"
#include "state.h"
#include "mtypes.h"

#include "math/m_matrix.h"
#include "math/m_xform.h"

#include "t_context.h"
#include "t_array_import.h"
#include "t_imm_alloc.h"
#include "t_imm_api.h"
#include "t_imm_debug.h"
#include "t_imm_dlist.h"
#include "t_imm_eval.h"
#include "t_imm_elt.h"
#include "t_imm_exec.h"
#include "t_imm_fixup.h"
#include "t_pipeline.h"



/* Called to initialize new buffers, and to recycle old ones.
 */
void _tnl_reset_input( GLcontext *ctx, 
		       GLuint start,
		       GLuint beginstate, 
		       GLuint savedbeginstate )
{
   struct immediate *IM = TNL_CURRENT_IM(ctx);

   /* Clear the dirty part of the flag array.
    */
   if (start < IM->Count+2)
      MEMSET(IM->Flag + start, 0, sizeof(GLuint) * (IM->Count+2-start));

   IM->CopyStart = IM->Start = IM->Count = start;
   IM->Primitive[IM->Start] = (ctx->Driver.CurrentExecPrimitive | PRIM_LAST);
   IM->LastPrimitive = IM->Start;
   IM->BeginState = beginstate;         
   IM->SavedBeginState = savedbeginstate;
   IM->TexSize = 0;
   IM->LastMaterial = 0;
   IM->MaterialOrMask = 0;

   IM->ArrayEltFlags = ~ctx->Array._Enabled;
   IM->ArrayEltIncr = ctx->Array.Vertex.Enabled ? 1 : 0;
   IM->ArrayEltFlush = !ctx->Array.LockCount;
}



void _tnl_copy_to_current( GLcontext *ctx, struct immediate *IM,
			   GLuint flag )
{
   GLuint count = IM->LastData;

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      _tnl_print_vert_flags("copy to current", flag);

   if (flag & VERT_NORM) 
      COPY_3FV( ctx->Current.Normal, IM->Normal[count]);   
   
   if (flag & VERT_INDEX)
      ctx->Current.Index = IM->Index[count];

   if (flag & VERT_EDGE)
      ctx->Current.EdgeFlag = IM->EdgeFlag[count];

   if (flag & VERT_RGBA) {
      COPY_4UBV(ctx->Current.Color, IM->Color[count]);
      if (ctx->Light.ColorMaterialEnabled) {
	 gl_update_color_material( ctx, ctx->Current.Color );
	 gl_validate_all_lighting_tables( ctx );
      }
   }

   if (flag & VERT_SPEC_RGB)
      COPY_4UBV(ctx->Current.SecondaryColor, IM->SecondaryColor[count]);

   if (flag & VERT_FOG_COORD)
      ctx->Current.FogCoord = IM->FogCoord[count];

   if (flag & VERT_TEX_ANY) {
      GLuint i;
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	 if (flag & VERT_TEX(i)) {
	    COPY_4FV( ctx->Current.Texcoord[0], IM->TexCoord[0][count]);
	 }
      }
   }

   if (flag & VERT_MATERIAL) {
      gl_update_material( ctx, 
			  IM->Material[IM->LastMaterial], 
			  IM->MaterialOrMask );
      
      gl_validate_all_lighting_tables( ctx );
   }
}



void _tnl_compute_orflag( struct immediate *IM )
{
   GLuint count = IM->Count;
   GLuint orflag = 0;
   GLuint andflag = ~0U;
   GLuint i;

   IM->LastData = count-1;


   /* Compute the flags for the whole buffer.
    */
   for (i = IM->CopyStart ; i < count ; i++) {
      andflag &= IM->Flag[i];
      orflag |= IM->Flag[i];
   }

   /* It is possible there will be data in the buffer arising from
    * calls like 'glNormal', 'glMaterial' that occur after the final
    * glVertex, glEval, etc.  Additionally, a buffer can consist of
    * eg. a single glMaterial call, in which case IM->Start ==
    * IM->Count, but the buffer is definitely not empty.  
    */
   if (IM->Flag[i] & VERT_DATA) {
      IM->LastData++;
      orflag |= IM->Flag[i];
   }

   IM->Flag[IM->LastData+1] |= VERT_END_VB;
   IM->CopyAndFlag = IM->AndFlag = andflag;
   IM->CopyOrFlag = IM->OrFlag = orflag;
}





/* Note: The 'start' member of the GLvector structs is now redundant
 * because we always re-transform copied vertices, and the vectors
 * below are set up so that the first copied vertex (if any) appears
 * at position zero.  
 */
static void _tnl_vb_bind_immediate( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct vertex_arrays *tmp = &tnl->imm_inputs;
   GLuint inputs = tnl->pipeline.inputs; /* for copy-to-current */
   GLuint start = IM->CopyStart;
   GLuint count = IM->Count - start;
   
   /* TODO: optimize the case where nothing has changed.  (Just bind
    * tmp to vb).
    */

   /* Setup constant data in the VB.
    */
   VB->Count = count;
   VB->FirstClipped = IMM_MAXDATA - IM->CopyStart;
   VB->import_data = 0;
   VB->importable_data = 0;

   /* Need an IM->FirstPrimitive?
    */
   VB->Primitive = IM->Primitive + IM->CopyStart;
   VB->PrimitiveLength = IM->PrimitiveLength + IM->CopyStart;
   VB->FirstPrimitive = 0;

   VB->Flag = IM->Flag + start;

   /* TexCoordPtr's are zeroed in loop below.
    */
   VB->NormalPtr = 0;
   VB->FogCoordPtr = 0;
   VB->EdgeFlag = 0;
   VB->IndexPtr[0] = 0;
   VB->IndexPtr[1] = 0;
   VB->ColorPtr[0] = 0;
   VB->ColorPtr[1] = 0;
   VB->SecondaryColorPtr[0] = 0;
   VB->SecondaryColorPtr[1] = 0;
   VB->Elts = 0;
   VB->MaterialMask = 0;
   VB->Material = 0;

/*     _tnl_print_vert_flags("copy-orflag", IM->CopyOrFlag); */
/*     _tnl_print_vert_flags("orflag", IM->OrFlag); */
/*     _tnl_print_vert_flags("inputs", inputs); */

   /* Setup the initial values of array pointers in the vb.
    */
   if (inputs & VERT_OBJ) {
      tmp->Obj.data = IM->Obj + start;
      tmp->Obj.start = (GLfloat *)(IM->Obj + start);
      tmp->Obj.count = count;
      VB->ObjPtr = &tmp->Obj; 
      if ((IM->CopyOrFlag & VERT_OBJ_234) == VERT_OBJ_234) 
	 tmp->Obj.size = 4;
      else if ((IM->CopyOrFlag & VERT_OBJ_234) == VERT_OBJ_23) 
	 tmp->Obj.size = 3;
      else
	 tmp->Obj.size = 2;
   }

   if (inputs & VERT_NORM) {
      tmp->Normal.data = IM->Normal + start;
      tmp->Normal.start = (GLfloat *)(IM->Normal + start);
      tmp->Normal.count = count;
      VB->NormalPtr = &tmp->Normal;
   }

   if (inputs & VERT_INDEX) {
      tmp->Index.count = count;
      tmp->Index.data = IM->Index + start;
      tmp->Index.start = IM->Index + start;
      VB->IndexPtr[0] = &tmp->Index;
   }

   if (inputs & VERT_FOG_COORD) {
      tmp->FogCoord.data = IM->FogCoord + start;
      tmp->FogCoord.start = IM->FogCoord + start;
      tmp->FogCoord.count = count;
      VB->FogCoordPtr = &tmp->FogCoord;
   }

   if (inputs & VERT_SPEC_RGB) {
      tmp->SecondaryColor.data = IM->SecondaryColor + start;
      tmp->SecondaryColor.start = (GLchan *)(IM->SecondaryColor + start);
      tmp->SecondaryColor.count = count;
      VB->SecondaryColorPtr[0] = &tmp->SecondaryColor;
   }

   if (inputs & VERT_EDGE) {
      VB->EdgeFlag = IM->EdgeFlag + start;
   }

   if (inputs & VERT_RGBA) {
      tmp->Color.data = IM->Color + start;
      tmp->Color.start = (GLchan *)(IM->Color + start);
      tmp->Color.count = count;
      VB->ColorPtr[0] = &tmp->Color;
   }

   if (inputs & VERT_TEX_ANY) {
      GLuint i;
      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
	 VB->TexCoordPtr[i] = 0;
	 if (inputs & VERT_TEX(i)) {
	    tmp->TexCoord[i].count = count;
	    tmp->TexCoord[i].data = IM->TexCoord[i] + start;
	    tmp->TexCoord[i].start = (GLfloat *)(IM->TexCoord[i] + start);
	    tmp->TexCoord[i].size = 2;
	    if (IM->TexSize & TEX_SIZE_3(i)) {
	       tmp->TexCoord[i].size = 3;
	       if (IM->TexSize & TEX_SIZE_4(i)) 
		  tmp->TexCoord[i].size = 4;
	    }
	    VB->TexCoordPtr[i] = &tmp->TexCoord[i];
	 }
      }
   }

   if ((inputs & VERT_MATERIAL) && IM->Material) {
      VB->MaterialMask = IM->MaterialMask + start;
      VB->Material = IM->Material + start;
   } 
}




/* Called by exec_cassette and execute_compiled_cassette.
 */
void _tnl_run_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_vb_bind_immediate( ctx, IM );

   if (IM->CopyOrFlag & VERT_EVAL_ANY) 
      _tnl_eval_vb( ctx, 
		    IM->Obj + IM->CopyStart, 
		    IM->CopyOrFlag, 
		    IM->CopyAndFlag );

   
   /* Invalidate all stored data before and after run:
    */
   tnl->pipeline.run_input_changes |= tnl->pipeline.inputs;
   _tnl_run_pipeline( ctx );      
   tnl->pipeline.run_input_changes |= tnl->pipeline.inputs;

   _tnl_copy_to_current( ctx, IM, IM->OrFlag ); 
}




/* Called for pure, locked VERT_ELT cassettes instead of
 * _tnl_run_cassette.  
 */
static void exec_elt_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   _tnl_vb_bind_arrays( ctx, ctx->Array.LockFirst, ctx->Array.LockCount );

   VB->Elts = IM->Elt + IM->CopyStart;
   VB->Primitive = IM->Primitive + IM->CopyStart;
   VB->PrimitiveLength = IM->PrimitiveLength + IM->CopyStart;
   VB->FirstPrimitive = 0;

   /* Run the pipeline.  No input changes as a result of this action.
    */
   _tnl_run_pipeline( ctx );

   /* Still need to update current values:  (TODO - copy from VB)
    * TODO: delay this until FlushVertices
    */
   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      _tnl_translate_array_elts( ctx, IM, IM->LastData, IM->LastData ); 
      _tnl_copy_to_current( ctx, IM, ctx->Array._Enabled );
   }
}



/* Called for regular vertex cassettes. 
 */
static void exec_vert_cassette( GLcontext *ctx, struct immediate *IM )
{
   if (IM->OrFlag & VERT_ELT) {
      GLuint andflag = ~0;
      GLuint i;
      GLuint start = IM->FlushElt ? IM->LastPrimitive : IM->CopyStart;
      _tnl_translate_array_elts( ctx, IM, start, IM->Count ); 

      /* Need to recompute andflag.
       */
      if (IM->CopyAndFlag & VERT_ELT)
	 IM->CopyAndFlag |= ctx->Array._Enabled;
      else {
	 for (i = IM->CopyStart ; i < IM->Count ; i++)
	    andflag &= IM->Flag[i];
	 IM->CopyAndFlag = andflag;
      }
   }

   _tnl_fixup_input( ctx, IM );
/*     _tnl_print_cassette( IM ); */
   _tnl_run_cassette( ctx, IM );      
}



/* Called for all cassettes when not compiling or playing a display
 * list.
 */
void _tnl_execute_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   ASSERT(tnl->ExecCopySource == IM);

   _tnl_compute_orflag( IM );

   /* Mark the last primitive:
    */
   IM->PrimitiveLength[IM->LastPrimitive] = IM->Count - IM->LastPrimitive;
   ASSERT(IM->Primitive[IM->LastPrimitive] & PRIM_LAST);

   if (tnl->pipeline.build_state_changes)
      _tnl_validate_pipeline( ctx );

   _tnl_get_exec_copy_verts( ctx, IM );
   
   if (IM->CopyStart == IM->Count) {
      if (IM->OrFlag & VERT_ELT) 
	 _tnl_translate_array_elts( ctx, IM, IM->CopyStart, IM->CopyStart ); 

      _tnl_copy_to_current( ctx, IM, IM->OrFlag );
   }
   else if ((IM->OrFlag & VERT_DATA) == VERT_ELT && 
	    ctx->Array.LockCount &&
	    ctx->Array.Vertex.Enabled) {
      exec_elt_cassette( ctx, IM );
   }
   else {
      exec_vert_cassette( ctx, IM );
   }

   _tnl_reset_input( ctx, 
		     IMM_MAX_COPIED_VERTS,
		     IM->BeginState & (VERT_BEGIN_0|VERT_BEGIN_1), 
		     IM->SavedBeginState );	

   /* Copy vertices and primitive information to immediate before it
    * can be overwritten.  
    */
   _tnl_copy_immediate_vertices( ctx, IM );

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1)
      ctx->Driver.NeedFlush &= ~FLUSH_STORED_VERTICES;
}




/* Setup vector pointers that will be used to bind immediates to VB's.
 */
void _tnl_imm_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_arrays *tmp = &tnl->imm_inputs;
   GLuint i;
   static int firsttime = 1;

   if (firsttime) {
      firsttime = 0;
      _tnl_imm_elt_init();
   }

   ctx->swtnl_im = _tnl_alloc_immediate( ctx );
   TNL_CURRENT_IM(ctx)->ref_count++;

   tnl->ExecCopyTexSize = 0;
   tnl->ExecCopyCount = 0;
   tnl->ExecCopySource = TNL_CURRENT_IM(ctx);
   TNL_CURRENT_IM(ctx)->ref_count++;

   TNL_CURRENT_IM(ctx)->CopyStart = IMM_MAX_COPIED_VERTS;

   gl_vector4f_init( &tmp->Obj, 0, 0 );
   gl_vector3f_init( &tmp->Normal, 0, 0 );
   gl_vector4chan_init( &tmp->Color, 0, 0 );
   gl_vector4chan_init( &tmp->SecondaryColor, 0, 0 );
   gl_vector1f_init( &tmp->FogCoord, 0, 0 );
   gl_vector1ui_init( &tmp->Index, 0, 0 );
   gl_vector1ub_init( &tmp->EdgeFlag, 0, 0 );

   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) 
      gl_vector4f_init( &tmp->TexCoord[i], 0, 0);

   /* Install the first immediate.  Intially outside begin/end.
    */
   _tnl_reset_input( ctx, IMM_MAX_COPIED_VERTS, 0, 0 );
   tnl->ReplayHardBeginEnd = 0;

   _tnl_imm_vtxfmt_init( ctx );
}


void _tnl_imm_destroy( GLcontext *ctx )
{
   if (TNL_CURRENT_IM(ctx)) 
      _tnl_free_immediate( TNL_CURRENT_IM(ctx) );

}
