/* $Id: t_imm_dlist.c,v 1.13 2001/04/26 14:53:48 keithw Exp $ */

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
#include "context.h"
#include "dlist.h"
#include "debug.h"
#include "mmath.h"
#include "mem.h"
#include "state.h"

#include "t_context.h"
#include "t_imm_api.h"
#include "t_imm_elt.h"
#include "t_imm_alloc.h"
#include "t_imm_dlist.h"
#include "t_imm_debug.h"
#include "t_imm_exec.h"
#include "t_imm_fixup.h"
#include "t_pipeline.h"

typedef struct {
   struct immediate *IM;
   GLuint Start;
   GLuint Count;
   GLuint BeginState;
   GLuint SavedBeginState;
   GLuint OrFlag;
   GLuint AndFlag;
   GLuint TexSize;
   GLuint LastData;
   GLuint LastPrimitive;
   GLuint LastMaterial;
   GLuint MaterialOrMask;
   GLuint MaterialAndMask;
} TNLvertexcassette;

static void execute_compiled_cassette( GLcontext *ctx, void *data );


/* Insert the active immediate struct onto the display list currently
 * being built.
 */
void
_tnl_compile_cassette( GLcontext *ctx, struct immediate *IM )
{
   struct immediate *im = TNL_CURRENT_IM(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   TNLvertexcassette *node;
   GLuint new_beginstate;

   _tnl_compute_orflag( IM );

   IM->CopyStart = IM->Start;

   if (IM->OrFlag & VERT_ELT) {
      GLuint andflag = ~0;
      GLuint i;
      GLuint start = IM->FlushElt ? IM->LastPrimitive : IM->CopyStart;
      _tnl_translate_array_elts( ctx, IM, start, IM->Count );

      /* Need to recompute andflag.
       */
      if (IM->AndFlag & VERT_ELT)
	 IM->CopyAndFlag = IM->AndFlag |= ctx->Array._Enabled;
      else {
	 for (i = IM->CopyStart ; i < IM->Count ; i++)
	    andflag &= IM->Flag[i];
	 IM->CopyAndFlag = IM->AndFlag = andflag;
      }
   }

   _tnl_fixup_input( ctx, IM );

   /* Mark the last primitive:
    */
   IM->PrimitiveLength[IM->LastPrimitive] = IM->Count - IM->LastPrimitive;
   ASSERT(IM->Primitive[IM->LastPrimitive] & PRIM_LAST);

   
/*     _tnl_print_cassette( IM ); */


   node = (TNLvertexcassette *)
      _mesa_alloc_instruction(ctx,
			      tnl->opcode_vertex_cassette,
			      sizeof(TNLvertexcassette));
   if (!node)
      return;

   node->IM = im; im->ref_count++;
   node->Start = im->Start;
   node->Count = im->Count;
   node->BeginState = im->BeginState;
   node->SavedBeginState = im->SavedBeginState;
   node->OrFlag = im->OrFlag;
   node->TexSize = im->TexSize;
   node->AndFlag = im->AndFlag;
   node->LastData = im->LastData;
   node->LastPrimitive = im->LastPrimitive;
   node->LastMaterial = im->LastMaterial;
   node->MaterialOrMask = im->MaterialOrMask;
   node->MaterialAndMask = im->MaterialAndMask;

   if (ctx->ExecuteFlag) {
      execute_compiled_cassette( ctx, (void *)node );
   }


   /* Discard any errors raised in the last cassette.
    */
   new_beginstate = node->BeginState & (VERT_BEGIN_0|VERT_BEGIN_1);

   /* Decide whether this immediate struct is full, or can be used for
    * the next batch of vertices as well.
    */
   if (im->Count > IMM_MAXDATA - 16) {
      /* Call it full...
       */
      struct immediate *new_im = _tnl_alloc_immediate(ctx);
      if (!new_im) return;
      new_im->ref_count++;
      im->ref_count--;		/* remove CURRENT_IM reference */
      ASSERT(im->ref_count > 0);
      SET_IMMEDIATE( ctx, new_im );
      _tnl_reset_input( ctx, IMM_MAX_COPIED_VERTS,
			new_beginstate, node->SavedBeginState );
   } else {
      /* Still some room in the current immediate.
       */
      _tnl_reset_input( ctx, im->Count+1+IMM_MAX_COPIED_VERTS,
			new_beginstate, node->SavedBeginState);
   }
}


static void
execute_compiled_cassette( GLcontext *ctx, void *data )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   TNLvertexcassette *node = (TNLvertexcassette *)data;
   struct immediate *IM = node->IM;

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (tnl->pipeline.build_state_changes)
      _tnl_validate_pipeline( ctx );

   IM->Start = node->Start;
   IM->CopyStart = node->Start;
   IM->Count = node->Count;
   IM->BeginState = node->BeginState;
   IM->SavedBeginState = node->SavedBeginState;
   IM->OrFlag = node->OrFlag;
   IM->TexSize = node->TexSize;
   IM->AndFlag = node->AndFlag;
   IM->LastData = node->LastData;
   IM->LastPrimitive = node->LastPrimitive;
   IM->LastMaterial = node->LastMaterial;
   IM->MaterialOrMask = node->MaterialOrMask;
   IM->MaterialAndMask = node->MaterialAndMask;

   if ((MESA_VERBOSE & VERBOSE_DISPLAY_LIST) &&
       (MESA_VERBOSE & VERBOSE_IMMEDIATE))
      _tnl_print_cassette( IM );

   if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST) {
      fprintf(stderr, "Run cassette %d, rows %d..%d, beginstate %x ",
	      IM->id,
	      IM->Start, IM->Count, IM->BeginState);
      _tnl_print_vert_flags("orflag", IM->OrFlag);
   }

   if (IM->Count == IM->Start) {
      _tnl_copy_to_current( ctx, IM, IM->OrFlag );
      return;
   }

   if (IM->SavedBeginState) {
      if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1)
	 tnl->ReplayHardBeginEnd = 1;
      if (!tnl->ReplayHardBeginEnd) {
	 /* This is a user error.  Whatever operation (like glRectf)
	  * decomposed to this hard begin/end pair is now being run
	  * inside a begin/end object -- illegally.  Reject it and
	  * raise an error.
	  */
	 _mesa_error(ctx, GL_INVALID_OPERATION, "hard replay");
	 return;
      }
   }

   _tnl_fixup_compiled_cassette( ctx, IM );
   _tnl_get_exec_copy_verts( ctx, IM );
   _tnl_run_cassette( ctx, IM );
   _tnl_restore_compiled_cassette( ctx, IM );

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1)
      tnl->ReplayHardBeginEnd = 0;
}

static void
destroy_compiled_cassette( GLcontext *ctx, void *data )
{
   TNLvertexcassette *node = (TNLvertexcassette *)data;

   if ( --node->IM->ref_count == 0 )
      _tnl_free_immediate( node->IM );
}


static void
print_compiled_cassette( GLcontext *ctx, void *data )
{
   TNLvertexcassette *node = (TNLvertexcassette *)data;
   struct immediate *IM = node->IM;

   fprintf(stderr, "TNL-VERTEX-CASSETTE, id %u, rows %u..%u\n",
	   node->IM->id, node->Start, node->Count);

   IM->Start = node->Start;
   IM->Count = node->Count;
   IM->BeginState = node->BeginState;
   IM->OrFlag = node->OrFlag;
   IM->TexSize = node->TexSize;
   IM->AndFlag = node->AndFlag;
   IM->LastData = node->LastData;
   IM->LastPrimitive = node->LastPrimitive;
   IM->LastMaterial = node->LastMaterial;
   IM->MaterialOrMask = node->MaterialOrMask;
   IM->MaterialAndMask = node->MaterialAndMask;

   _tnl_print_cassette( node->IM );
}

void
_tnl_BeginCallList( GLcontext *ctx, GLuint list )
{
   (void) ctx;
   (void) list;
   FLUSH_CURRENT(ctx, 0);
}


/* Called at the tail of a CallList.  Copy vertices out of the display
 * list if necessary.
 */
void
_tnl_EndCallList( GLcontext *ctx )
{
   /* May have to copy vertices from a dangling begin/end inside the
    * list to the current immediate.
    */
   if (ctx->CallDepth == 0) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      struct immediate *IM = TNL_CURRENT_IM(ctx);

      if (tnl->ExecCopySource != IM)
	 _tnl_copy_immediate_vertices( ctx, IM );
   }
}


void
_tnl_EndList( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct immediate *IM = TNL_CURRENT_IM(ctx);

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   ctx->swtnl_im = 0;
   IM->ref_count--;
   if (IM == tnl->ExecCopySource) {
      IM->ref_count--;
   } else {
      if ( --tnl->ExecCopySource->ref_count == 0 )
	 _tnl_free_immediate( tnl->ExecCopySource );
   }

   /* If this one isn't free, get a clean one.  (Otherwise we'll be
    * using one that's already half full).
    */
   if (IM->ref_count != 0)
      IM = _tnl_alloc_immediate( ctx );

   ASSERT(IM->ref_count == 0);

   tnl->ExecCopySource = IM;
   IM->ref_count++;

   SET_IMMEDIATE( ctx, IM );
   IM->ref_count++;

   _tnl_reset_input( ctx, IMM_MAX_COPIED_VERTS, 0, 0 );

   /* outside begin/end, even in COMPILE_AND_EXEC,
    * so no vertices to copy, right?
    */
   ASSERT(TNL_CONTEXT(ctx)->ExecCopyCount == 0);
}


void
_tnl_NewList( GLcontext *ctx, GLuint list, GLenum mode )
{
   struct immediate *IM = TNL_CURRENT_IM(ctx);

   /* Use the installed immediate struct.  No vertices in the current
    * immediate, no copied vertices in the system.
    */
   ASSERT(TNL_CURRENT_IM(ctx));
   ASSERT(TNL_CURRENT_IM(ctx)->Start == IMM_MAX_COPIED_VERTS);
   ASSERT(TNL_CURRENT_IM(ctx)->Start == TNL_CURRENT_IM(ctx)->Count);
   ASSERT(TNL_CONTEXT(ctx)->ExecCopyCount == 0);

   /* Set current Begin/End state to unknown:
    */
   IM->BeginState = VERT_BEGIN_0;
   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;
}


void
_tnl_dlist_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   tnl->opcode_vertex_cassette =
      _mesa_alloc_opcode( ctx,
			  sizeof(TNLvertexcassette),
			  execute_compiled_cassette,
			  destroy_compiled_cassette,
			  print_compiled_cassette );
}
