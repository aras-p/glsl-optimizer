/* $Id: t_imm_dlist.c,v 1.38 2002/02/13 00:53:20 keithw Exp $ */

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
static void loopback_compiled_cassette( GLcontext *ctx, struct immediate *IM );


static void build_normal_lengths( struct immediate *IM )
{
   GLuint i;
   GLfloat len;
   GLfloat (*data)[4] = IM->Attrib[VERT_ATTRIB_NORMAL] + IM->Start;
   GLfloat *dest = IM->NormalLengthPtr;
   GLuint *flags = IM->Flag + IM->Start;
   GLuint count = IM->Count - IM->Start;

   if (!dest) {
      dest = IM->NormalLengthPtr = ALIGN_MALLOC( IMM_SIZE*sizeof(GLfloat), 32 );
      if (!dest) return;
   }
   dest += IM->Start;

   len = (GLfloat) LEN_3FV( data[0] );
   if (len > 0.0F) len = 1.0F / len;
   
   for (i = 0 ; i < count ; ) {
      dest[i] = len;
      if (flags[++i] & VERT_BIT_NORMAL) {
	 len = (GLfloat) LEN_3FV( data[i] );
	 if (len > 0.0F) len = 1.0F / len;
      }
   } 
}

static void fixup_normal_lengths( struct immediate *IM ) 
{
   GLuint i;
   GLfloat len = 1.0F;  /* just to silence warnings */
   GLfloat (*data)[4] = IM->Attrib[VERT_ATTRIB_NORMAL];
   GLfloat *dest = IM->NormalLengthPtr;
   GLuint *flags = IM->Flag;

   for (i = IM->CopyStart ; i <= IM->Start ; i++) {
      len = (GLfloat) LEN_3FV( data[i] );
      if (len > 0.0F) len = 1.0F / len;
      dest[i] = len;
   } 

   if (i < IM->Count) {
      while (!(flags[i] & (VERT_BIT_NORMAL|VERT_BIT_END_VB))) {
	 dest[i] = len;
	 i++;
      }
   }
}



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

   if (IM->FlushElt) {
      ASSERT (IM->FlushElt == FLUSH_ELT_LAZY); 
      _tnl_translate_array_elts( ctx, IM, IM->Start, IM->Count );
   }

   _tnl_compute_orflag( IM, IM->Start );

   /* Need to clear this flag, or fixup gets confused.  (The
    * array-elements have been translated away by now, so it's ok to
    * remove it.)
    */
   IM->OrFlag &= ~VERT_BIT_ELT;	
   IM->AndFlag &= ~VERT_BIT_ELT;	

   _tnl_fixup_input( ctx, IM );

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
   
   if (tnl->CalcDListNormalLengths) {
      build_normal_lengths( im );
   }

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
      new_im->ref_count++;
      im->ref_count--;		 /* remove CURRENT_IM reference */
      ASSERT(im->ref_count > 0); /* it is compiled into a display list */
      SET_IMMEDIATE( ctx, new_im );
      _tnl_reset_compile_input( ctx, IMM_MAX_COPIED_VERTS,
				new_beginstate, node->SavedBeginState );
   } else {
      /* Still some room in the current immediate.
       */
      _tnl_reset_compile_input( ctx, im->Count+1+IMM_MAX_COPIED_VERTS,
			new_beginstate, node->SavedBeginState);
   }
}


static void fixup_compiled_primitives( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /* Can potentially overwrite primitive details - need to save the
    * first slot:
    */
   tnl->DlistPrimitive = IM->Primitive[IM->Start];
   tnl->DlistPrimitiveLength = IM->PrimitiveLength[IM->Start];
   tnl->DlistLastPrimitive = IM->LastPrimitive;

   /* The first primitive may be different from what was recorded in
    * the immediate struct.  Consider an immediate that starts with a
    * glBegin, compiled in a display list, which is called from within
    * an existing Begin/End object.
    */
   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      GLuint i;

      if (IM->BeginState & VERT_ERROR_1)
	 _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin/glEnd");

      for (i = IM->Start ; i <= IM->Count ; i += IM->PrimitiveLength[i])
	 if (IM->Flag[i] & (VERT_BIT_BEGIN|VERT_BIT_END_VB))
	    break;

      /* Would like to just ignore vertices upto this point.  Can't
       * set copystart because it might skip materials?
       */
      ASSERT(IM->Start == IM->CopyStart);
      if (i > IM->CopyStart || !(IM->Flag[IM->Start] & VERT_BIT_BEGIN)) {
	 IM->Primitive[IM->CopyStart] = GL_POLYGON+1;
	 IM->PrimitiveLength[IM->CopyStart] = i - IM->CopyStart;
	 if (IM->Flag[i] & VERT_BIT_END_VB) {
	    IM->Primitive[IM->CopyStart] |= PRIM_LAST;
	    IM->LastPrimitive = IM->CopyStart;
	 }
      }
   } else {
      GLuint i;

      if (IM->BeginState & VERT_ERROR_0)
	 _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin/glEnd");

      if (IM->CopyStart == IM->Start &&
	  IM->Flag[IM->Start] & (VERT_BIT_END | VERT_BIT_END_VB))
      {
      }
      else
      {
	 IM->Primitive[IM->CopyStart] = ctx->Driver.CurrentExecPrimitive;
	 if (tnl->ExecParity)
	    IM->Primitive[IM->CopyStart] |= PRIM_PARITY;

         /* one of these should be true, else we'll be in an infinite loop 
	  */
         ASSERT(IM->PrimitiveLength[IM->Start] > 0 ||
                IM->Flag[IM->Start] & (VERT_BIT_END | VERT_BIT_END_VB));

	 for (i = IM->Start ; i <= IM->Count ; i += IM->PrimitiveLength[i])
	    if (IM->Flag[i] & (VERT_BIT_END | VERT_BIT_END_VB)) {
	       IM->PrimitiveLength[IM->CopyStart] = i - IM->CopyStart;
	       if (IM->Flag[i] & VERT_BIT_END_VB) {
		  IM->Primitive[IM->CopyStart] |= PRIM_LAST;
		  IM->LastPrimitive = IM->CopyStart;
	       }
	       if (IM->Flag[i] & VERT_BIT_END) {
		  IM->Primitive[IM->CopyStart] |= PRIM_END;
	       }
	       break;
	    }
      }
   }
}

/* Undo any changes potentially made to the immediate in the range
 * IM->Start..IM->Count above.
 */
static void restore_compiled_primitives( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   IM->Primitive[IM->Start] = tnl->DlistPrimitive;
   IM->PrimitiveLength[IM->Start] = tnl->DlistPrimitiveLength;
}



static void
execute_compiled_cassette( GLcontext *ctx, void *data )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   TNLvertexcassette *node = (TNLvertexcassette *)data;
   struct immediate *IM = node->IM;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

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


   /* Need to respect 'HardBeginEnd' even if the commands are looped
    * back to a driver tnl module.
    */
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

   if (tnl->LoopbackDListCassettes) {
/*        (tnl->IsolateMaterials && (IM->OrFlag & VERT_MATERIAL)) ) { */
      fixup_compiled_primitives( ctx, IM );
      loopback_compiled_cassette( ctx, IM );
      restore_compiled_primitives( ctx, IM );
   }
   else {
      if (ctx->NewState)
	 _mesa_update_state(ctx);

      if (tnl->pipeline.build_state_changes)
	 _tnl_validate_pipeline( ctx );

      _tnl_fixup_compiled_cassette( ctx, IM );
      fixup_compiled_primitives( ctx, IM );

      if (IM->Primitive[IM->LastPrimitive] & PRIM_END)
	 ctx->Driver.CurrentExecPrimitive = GL_POLYGON+1;
      else if ((IM->Primitive[IM->LastPrimitive] & PRIM_BEGIN) ||
	       (IM->Primitive[IM->LastPrimitive] & PRIM_MODE_MASK) == 
	       PRIM_OUTSIDE_BEGIN_END) {
	 ctx->Driver.CurrentExecPrimitive =
	    IM->Primitive[IM->LastPrimitive] & PRIM_MODE_MASK;
      }

      _tnl_get_exec_copy_verts( ctx, IM );

      if (IM->NormalLengthPtr) 
	 fixup_normal_lengths( IM );
      
      if (IM->Count == IM->Start) 
	 _tnl_copy_to_current( ctx, IM, IM->OrFlag, IM->LastData );
      else {
/*  	 _tnl_print_cassette( IM ); */
	 _tnl_run_cassette( ctx, IM );
      }

      restore_compiled_primitives( ctx, IM );
   }

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
   IM->CopyStart = node->Start;
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


/* Called at the tail of a CallList.  Make current immediate aware of
 * any new to-be-copied vertices.
 */
void
_tnl_EndCallList( GLcontext *ctx )
{
   GLuint beginstate = 0;

   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END)
      beginstate = VERT_BEGIN_0|VERT_BEGIN_1;

   _tnl_reset_exec_input( ctx, TNL_CURRENT_IM(ctx)->Start, beginstate, 0 );
}


void
_tnl_EndList( GLcontext *ctx )
{
   struct immediate *IM = TNL_CURRENT_IM(ctx);

   ctx->swtnl_im = 0;
   IM->ref_count--;

   /* outside begin/end, even in COMPILE_AND_EXEC,
    * so no vertices to copy, right?
    */
   ASSERT(TNL_CONTEXT(ctx)->ExecCopyCount == 0);

   /* If this one isn't free, get a clean one.  (Otherwise we'll be
    * using one that's already half full).
    */
   if (IM->ref_count != 0)
      IM = _tnl_alloc_immediate( ctx );

   ASSERT(IM->ref_count == 0);

   SET_IMMEDIATE( ctx, IM );
   IM->ref_count++;

   _tnl_reset_exec_input( ctx, IMM_MAX_COPIED_VERTS, 0, 0 );
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


static void emit_material( struct gl_material *src, GLuint bitmask )
{
   if (bitmask & FRONT_EMISSION_BIT) 
      glMaterialfv( GL_FRONT, GL_EMISSION, src[0].Emission );

   if (bitmask & BACK_EMISSION_BIT) 
      glMaterialfv( GL_BACK, GL_EMISSION, src[1].Emission );

   if (bitmask & FRONT_AMBIENT_BIT) 
      glMaterialfv( GL_FRONT, GL_AMBIENT, src[0].Ambient );

   if (bitmask & BACK_AMBIENT_BIT) 
      glMaterialfv( GL_BACK, GL_AMBIENT, src[1].Ambient );

   if (bitmask & FRONT_DIFFUSE_BIT) 
      glMaterialfv( GL_FRONT, GL_DIFFUSE, src[0].Diffuse );

   if (bitmask & BACK_DIFFUSE_BIT) 
      glMaterialfv( GL_BACK, GL_DIFFUSE, src[1].Diffuse );

   if (bitmask & FRONT_SPECULAR_BIT) 
      glMaterialfv( GL_FRONT, GL_SPECULAR, src[0].Specular );

   if (bitmask & BACK_SPECULAR_BIT) 
      glMaterialfv( GL_BACK, GL_SPECULAR, src[1].Specular );

   if (bitmask & FRONT_SHININESS_BIT) 
      glMaterialfv( GL_FRONT, GL_SHININESS, &src[0].Shininess );

   if (bitmask & BACK_SHININESS_BIT) 
      glMaterialfv( GL_BACK, GL_SHININESS, &src[1].Shininess );

   if (bitmask & FRONT_INDEXES_BIT) {
      GLfloat ind[3];
      ind[0] = src[0].AmbientIndex;
      ind[1] = src[0].DiffuseIndex;
      ind[2] = src[0].SpecularIndex;
      glMaterialfv( GL_FRONT, GL_COLOR_INDEXES, ind );
   }

   if (bitmask & BACK_INDEXES_BIT) {
      GLfloat ind[3];
      ind[0] = src[1].AmbientIndex;
      ind[1] = src[1].DiffuseIndex;
      ind[2] = src[1].SpecularIndex;
      glMaterialfv( GL_BACK, GL_COLOR_INDEXES, ind );
   }
}


/* Low-performance helper function to allow driver-supplied tnl
 * modules to process tnl display lists.  This is primarily supplied
 * to avoid fallbacks if CallList is invoked inside a Begin/End pair.
 * For higher performance, drivers should fallback to tnl (if outside
 * begin/end), or (for tnl hardware) implement their own display list
 * mechanism. 
 */
static void loopback_compiled_cassette( GLcontext *ctx, struct immediate *IM )
{
   GLuint i;
   GLuint *flags = IM->Flag;
   GLuint orflag = IM->OrFlag;
   GLuint j;
   void (GLAPIENTRY *vertex)( const GLfloat * );
   void (GLAPIENTRY *texcoordfv[MAX_TEXTURE_UNITS])( GLenum, const GLfloat * );
   GLuint maxtex = 0;
   GLuint p, length, prim = 0;
   
   if (orflag & VERT_BITS_OBJ_234)
      vertex = (void (GLAPIENTRY *)(const GLfloat *)) glVertex4fv;
   else
      vertex = (void (GLAPIENTRY *)(const GLfloat *)) glVertex3fv;
   
   if (orflag & VERT_BITS_TEX_ANY) {
      for (j = 0 ; j < ctx->Const.MaxTextureUnits ; j++) {
	 if (orflag & VERT_BIT_TEX(j)) {
	    maxtex = j+1;
	    if ((IM->TexSize & TEX_SIZE_4(j)) == TEX_SIZE_4(j))
	       texcoordfv[j] = glMultiTexCoord4fvARB;
	    else if (IM->TexSize & TEX_SIZE_3(j))
	       texcoordfv[j] = glMultiTexCoord3fvARB;
	    else
	       texcoordfv[j] = glMultiTexCoord2fvARB;
	 }
      }      
   }

   for (p = IM->Start ; !(prim & PRIM_LAST) ; p += length)
   {
      prim = IM->Primitive[p];
      length= IM->PrimitiveLength[p];
      ASSERT(length || (prim & PRIM_LAST));
      ASSERT((prim & PRIM_MODE_MASK) <= GL_POLYGON+1);

      if (prim & PRIM_BEGIN) {
	 glBegin(prim & PRIM_MODE_MASK);
      }

      for ( i = p ; i <= p+length ; i++) {
	 if (flags[i] & VERT_BITS_TEX_ANY) {
	    GLuint k;
	    for (k = 0 ; k < maxtex ; k++) {
	       if (flags[i] & VERT_BIT_TEX(k)) {
		  texcoordfv[k]( GL_TEXTURE0_ARB + k,
                                 IM->Attrib[VERT_ATTRIB_TEX0 + k][i] );
	       }
	    }
	 }

	 if (flags[i] & VERT_BIT_NORMAL) 
	    glNormal3fv(IM->Attrib[VERT_ATTRIB_NORMAL][i]);

	 if (flags[i] & VERT_BIT_COLOR0) 
	    glColor4fv( IM->Attrib[VERT_ATTRIB_COLOR0][i] );

	 if (flags[i] & VERT_BIT_COLOR1)
	    _glapi_Dispatch->SecondaryColor3fvEXT( IM->Attrib[VERT_ATTRIB_COLOR1][i] );

	 if (flags[i] & VERT_BIT_FOG)
	    _glapi_Dispatch->FogCoordfEXT( IM->Attrib[VERT_ATTRIB_FOG][i][0] );

	 if (flags[i] & VERT_BIT_INDEX)
	    glIndexi( IM->Index[i] );

	 if (flags[i] & VERT_BIT_EDGEFLAG)
	    glEdgeFlag( IM->EdgeFlag[i] );

	 if (flags[i] & VERT_BIT_MATERIAL) 
	    emit_material( IM->Material[i], IM->MaterialMask[i] );

	 if (flags[i]&VERT_BITS_OBJ_234) 
	    vertex( IM->Attrib[VERT_ATTRIB_POS][i] );
	 else if (flags[i] & VERT_BIT_EVAL_C1)
	    glEvalCoord1f( IM->Attrib[VERT_ATTRIB_POS][i][0] );
	 else if (flags[i] & VERT_BIT_EVAL_P1)
	    glEvalPoint1( (GLint) IM->Attrib[VERT_ATTRIB_POS][i][0] );
	 else if (flags[i] & VERT_BIT_EVAL_C2)
	    glEvalCoord2f( IM->Attrib[VERT_ATTRIB_POS][i][0],
                           IM->Attrib[VERT_ATTRIB_POS][i][1] );
	 else if (flags[i] & VERT_BIT_EVAL_P2)
	    glEvalPoint2( (GLint) IM->Attrib[VERT_ATTRIB_POS][i][0],
                          (GLint) IM->Attrib[VERT_ATTRIB_POS][i][1] );
      }

      if (prim & PRIM_END) {
	 glEnd();
      }
   }
}
