/* $Id: t_imm_fixup.c,v 1.9 2001/03/12 00:48:43 gareth Exp $ */

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
 */

/*
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */


#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "dlist.h"
#include "colormac.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "mtypes.h"

#include "math/m_matrix.h"
#include "math/m_xform.h"

#include "t_context.h"
#include "t_imm_alloc.h"
#include "t_imm_debug.h"
#include "t_imm_fixup.h"
#include "t_pipeline.h"



static void
fixup_4f( GLfloat data[][4], GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 COPY_4FV(data[i], data[i-1]);
	 if (flag[i] & VERT_END_VB) break;
      }
   }
}

static void
fixup_3f( float data[][3], GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 COPY_3V(data[i], data[i-1]);
	 if (flag[i] & VERT_END_VB) break;
      }
   }
}


static void
fixup_1ui( GLuint *data, GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 data[i] = data[i-1];
	 if (flag[i] & VERT_END_VB) break;
      }
   }
   flag[i] |= match;
}


static void
fixup_1f( GLfloat *data, GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 data[i] = data[i-1];
	 if (flag[i] & VERT_END_VB) break;
      }
   }
   flag[i] |= match;
}

static void
fixup_1ub( GLubyte *data, GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 data[i] = data[i-1];
	 if (flag[i] & VERT_END_VB) break;
      }
   }
   flag[i] |= match;
}


static void
fixup_4chan( GLchan data[][4], GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 COPY_CHAN4(data[i], data[i-1]);
	 if (flag[i] & VERT_END_VB) break;
      }
   }
   flag[i] |= match;
}


static void
fixup_first_4f( GLfloat data[][4], GLuint flag[], GLuint match,
		GLuint start, GLfloat *dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      COPY_4FV(data[i], dflt);
}

static void
fixup_first_3f( GLfloat data[][3], GLuint flag[], GLuint match,
		GLuint start, GLfloat *dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      COPY_3FV(data[i], dflt);
}


static void
fixup_first_1ui( GLuint data[], GLuint flag[], GLuint match,
		 GLuint start, GLuint dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}

static void
fixup_first_1f( GLfloat data[], GLuint flag[], GLuint match,
		GLuint start, GLfloat dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}


static void
fixup_first_1ub( GLubyte data[], GLuint flag[], GLuint match,
		 GLuint start, GLubyte dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}


static void
fixup_first_4chan( GLchan data[][4], GLuint flag[], GLuint match,
                   GLuint start, GLchan dflt[4] )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      COPY_CHAN4(data[i], dflt);
}


void _tnl_fixup_input( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint start = IM->CopyStart;
   GLuint andflag = IM->CopyAndFlag;
   GLuint orflag = IM->CopyOrFlag;
   GLuint fixup;

   IM->CopyTexSize = IM->TexSize;

/*     fprintf(stderr, "Fixup input, Start: %u Count: %u LastData: %u\n", */
/*  	   IM->Start, IM->Count, IM->LastData); */
/*     _tnl_print_vert_flags("Orflag", orflag); */
/*     _tnl_print_vert_flags("Andflag", andflag); */


   fixup = ~andflag & VERT_FIXUP;

   if (!ctx->CompileFlag)
      fixup &= tnl->pipeline.inputs;

   if (!ctx->ExecuteFlag)
      fixup &= orflag;

   if ((orflag & (VERT_OBJ|VERT_EVAL_ANY)) == 0)
      fixup = 0;

   if (fixup) {
      GLuint copy = fixup & ~IM->Flag[start];


      /* Equivalent to a lazy copy-from-current when setting up the
       * immediate.
       */
      if (ctx->ExecuteFlag && copy) {
/*  	 _tnl_print_vert_flags("copy from current", copy); */

	 if (copy & VERT_NORM) {
	    COPY_3V( IM->Normal[start], ctx->Current.Normal );
	 }

	 if (copy & VERT_RGBA) {
	    COPY_CHAN4( IM->Color[start], ctx->Current.Color);
	 }

	 if (copy & VERT_SPEC_RGB)
	    COPY_CHAN4( IM->SecondaryColor[start], ctx->Current.SecondaryColor);

	 if (copy & VERT_FOG_COORD)
	    IM->FogCoord[start] = ctx->Current.FogCoord;

	 if (copy & VERT_INDEX)
	    IM->Index[start] = ctx->Current.Index;

	 if (copy & VERT_EDGE)
	    IM->EdgeFlag[start] = ctx->Current.EdgeFlag;

	 if (copy & VERT_TEX_ANY) {
	    GLuint i;
	    for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	       if (copy & VERT_TEX(i))
		  COPY_4FV( IM->TexCoord[i][start], ctx->Current.Texcoord[i] );
	    }
	 }
      }

      if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
/*  	 _tnl_print_vert_flags("fixup", fixup); */

      if (fixup & VERT_TEX_ANY) {
	 GLuint i;
	 for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	    if (fixup & VERT_TEX(i)) {
	       if (orflag & VERT_TEX(i))
		  fixup_4f( IM->TexCoord[i], IM->Flag, start, VERT_TEX(i) );
	       else
		  fixup_first_4f( IM->TexCoord[i], IM->Flag, VERT_END_VB, start,
				  IM->TexCoord[i][start]);
	    }
	 }
      }
   }

   if (fixup & VERT_EDGE) {
      if (orflag & VERT_EDGE)
	 fixup_1ub( IM->EdgeFlag, IM->Flag, start, VERT_EDGE );
      else
	 fixup_first_1ub( IM->EdgeFlag, IM->Flag, VERT_END_VB, start,
			  IM->EdgeFlag[start] );
   }

   if (fixup & VERT_INDEX) {
      if (orflag & VERT_INDEX)
	 fixup_1ui( IM->Index, IM->Flag, start, VERT_INDEX );
      else
	 fixup_first_1ui( IM->Index, IM->Flag, VERT_END_VB, start, IM->Index[start] );
   }

   if (fixup & VERT_RGBA) {
      if (orflag & VERT_RGBA)
	 fixup_4chan( IM->Color, IM->Flag, start, VERT_RGBA );
      else
	 fixup_first_4chan( IM->Color, IM->Flag, VERT_END_VB, start, IM->Color[start] );
   }

   if (fixup & VERT_SPEC_RGB) {
      if (orflag & VERT_SPEC_RGB)
	 fixup_4chan( IM->SecondaryColor, IM->Flag, start, VERT_SPEC_RGB );
      else
	 fixup_first_4chan( IM->SecondaryColor, IM->Flag, VERT_END_VB, start,
                            IM->SecondaryColor[start] );
   }

   if (fixup & VERT_FOG_COORD) {
      if (orflag & VERT_FOG_COORD)
	 fixup_1f( IM->FogCoord, IM->Flag, start, VERT_FOG_COORD );
      else
	 fixup_first_1f( IM->FogCoord, IM->Flag, VERT_END_VB, start,
			 IM->FogCoord[start] );
   }

   if (fixup & VERT_NORM) {
      if (orflag & VERT_NORM)
	 fixup_3f( IM->Normal, IM->Flag, start, VERT_NORM );
      else
	 fixup_first_3f( IM->Normal, IM->Flag, VERT_END_VB, start,
			 IM->Normal[start] );
   }

   /* Prune possible half-filled slot.
    */
   IM->Flag[IM->LastData+1] &= ~VERT_END_VB;
   IM->Flag[IM->Count] |= VERT_END_VB;

}




static void copy_material( struct immediate *next,
			   struct immediate *prev,
			   GLuint dst, GLuint src )
{
   if (next->Material == 0) {
      next->Material = (GLmaterial (*)[2]) MALLOC( sizeof(GLmaterial) *
						   IMM_SIZE * 2 );
      next->MaterialMask = (GLuint *) MALLOC( sizeof(GLuint) * IMM_SIZE );
   }

   next->MaterialMask[dst] = prev->MaterialMask[src];
   MEMCPY(next->Material[dst], prev->Material[src], 2*sizeof(GLmaterial));
}



/* Copy the untransformed data from the shared vertices of a primitive
 * that wraps over two immediate structs.  This is done prior to
 * set_immediate so that prev and next may point to the same
 * structure.  In general it's difficult to avoid this copy on long
 * primitives.
 *
 * Have to be careful with the transitions between display list
 * replay, compile and normal execute modes.
 */
static void copy_vertices( GLcontext *ctx,
			   struct immediate *next,
			   struct immediate *prev,
			   GLuint count,
			   GLuint *elts )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint offset = IMM_MAX_COPIED_VERTS - count;
   GLuint i;

   next->CopyStart = next->Start - count;

   /* Copy the vertices
    */
   for (i = 0 ; i < count ; i++)
   {
      GLuint src = elts[i+offset];
      GLuint dst = next->CopyStart+i;

      COPY_4FV( next->Obj[dst], prev->Obj[src] );
      COPY_3FV( next->Normal[dst], prev->Normal[src] );
      COPY_CHAN4( next->Color[dst], prev->Color[src] );

      if (prev->OrFlag & VERT_TEX_ANY) {
	 GLuint i;
	 for (i = 0 ; i < prev->MaxTextureUnits ; i++) {
	    if (prev->OrFlag & VERT_TEX(i))
	       COPY_4FV( next->TexCoord[i][dst], prev->TexCoord[i][src] );
	 }
      }

      if (prev->Flag[src] & VERT_MATERIAL)
	 copy_material(next, prev, dst, src);

      next->Elt[dst] = prev->Elt[src];
      next->EdgeFlag[dst] = prev->EdgeFlag[src];
      next->Index[dst] = prev->Index[src];
      COPY_CHAN4( next->SecondaryColor[dst], prev->SecondaryColor[src] );
      next->FogCoord[dst] = prev->FogCoord[src];
      next->Flag[dst] = (prev->CopyOrFlag & VERT_FIXUP);
      next->CopyOrFlag |= prev->Flag[src];  /* redundant for current_im */
      next->CopyAndFlag &= prev->Flag[src]; /* redundant for current_im */
   }


   ASSERT(prev == tnl->ExecCopySource);

   if (--tnl->ExecCopySource->ref_count == 0)
      _tnl_free_immediate( tnl->ExecCopySource );

   next->ref_count++;
   tnl->ExecCopySource = next;

   tnl->ExecCopyElts[0] = next->Start-3;
   tnl->ExecCopyElts[1] = next->Start-2;
   tnl->ExecCopyElts[2] = next->Start-1;
}

/* Copy vertices to an empty immediate struct.
 */
void _tnl_copy_immediate_vertices( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   ASSERT(IM == TNL_CURRENT_IM(ctx));
   ASSERT(IM->Count == IM->Start);

   /* Need to push this in now as it won't be computed anywhere else/
    */
   IM->TexSize = tnl->ExecCopyTexSize;

   /* A wrapped primitive.  We may be copying into a revived
    * display list immediate, or onto the front of a new execute-mode
    * immediate.
    */
   copy_vertices( ctx, IM,
		  tnl->ExecCopySource,
		  tnl->ExecCopyCount,
		  tnl->ExecCopyElts );

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      /* Immediates are built by default to be correct in this state,
       * and copying to the first slots of an immediate doesn't remove
       * this property.
       */
      ASSERT(tnl->ExecCopyTexSize == 0);
      ASSERT(tnl->ExecCopyCount == 0);
      ASSERT(IM->CopyStart == IM->Start);
   }

   /* Copy the primitive information:
    */
   IM->Primitive[IM->CopyStart] = (ctx->Driver.CurrentExecPrimitive | PRIM_LAST);
   IM->LastPrimitive = IM->CopyStart;
   if (tnl->ExecParity)
      IM->Primitive[IM->CopyStart] |= PRIM_PARITY;
}


/* Revive a compiled immediate struct - propogate new 'Current'
 * values.  Often this is redundant because the current values were
 * known and fixed up at compile time.
 */
void _tnl_fixup_compiled_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint fixup;
   GLuint count = IM->Count;
   GLuint start = IM->Start;

   if (count == start)
      return;

   IM->CopyOrFlag = IM->OrFlag;	  /* redundant for current_im */
   IM->CopyAndFlag = IM->AndFlag; /* redundant for current_im */
   IM->CopyTexSize = IM->TexSize | tnl->ExecCopyTexSize;

   copy_vertices( ctx, IM,
		  tnl->ExecCopySource,
		  tnl->ExecCopyCount,
		  tnl->ExecCopyElts );

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      ASSERT(tnl->ExecCopyTexSize == 0);
      ASSERT(tnl->ExecCopyCount == 0);
      ASSERT(IM->CopyStart == IM->Start);
   }

   fixup = tnl->pipeline.inputs & ~IM->Flag[start] & VERT_FIXUP;

   if (fixup) {
      if (fixup & VERT_TEX_ANY) {
	 GLuint i;
	 for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	    if (fixup & VERT_TEX(i))
	       fixup_first_4f( IM->TexCoord[i], IM->Flag, VERT_TEX(i), start,
			       ctx->Current.Texcoord[i] );
	 }
      }

      if (fixup & VERT_EDGE)
	 fixup_first_1ub(IM->EdgeFlag, IM->Flag, VERT_EDGE, start,
			 ctx->Current.EdgeFlag );

      if (fixup & VERT_INDEX)
	 fixup_first_1ui(IM->Index, IM->Flag, VERT_INDEX, start,
			 ctx->Current.Index );

      if (fixup & VERT_RGBA)
	 fixup_first_4chan(IM->Color, IM->Flag, VERT_RGBA, start,
                           ctx->Current.Color );

      if (fixup & VERT_SPEC_RGB)
	 fixup_first_4chan(IM->SecondaryColor, IM->Flag, VERT_SPEC_RGB, start,
                           ctx->Current.SecondaryColor );

      if (fixup & VERT_FOG_COORD)
	 fixup_first_1f(IM->FogCoord, IM->Flag, VERT_FOG_COORD, start,
			 ctx->Current.FogCoord );

      if (fixup & VERT_NORM) {
	 fixup_first_3f(IM->Normal, IM->Flag, VERT_NORM, start,
			ctx->Current.Normal );
      }
   }


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
	 if (IM->Flag[i] & (VERT_BEGIN|VERT_END_VB))
	    break;

      /* Would like to just ignore vertices upto this point.  Can't
       * set copystart because it might skip materials?
       */
      ASSERT(IM->Start == IM->CopyStart);
      if (i > IM->CopyStart) {
	 IM->Primitive[IM->CopyStart] = GL_POLYGON+1;
	 IM->PrimitiveLength[IM->CopyStart] = i - IM->CopyStart;
	 if (IM->Flag[i] & VERT_END_VB) {
	    IM->Primitive[IM->CopyStart] |= PRIM_LAST;
	    IM->LastPrimitive = IM->CopyStart;
	 }
      }
      /* Shouldn't immediates be set up to have this structure *by default*?
       */
   } else {
      GLuint i;

      if (IM->BeginState & VERT_ERROR_0)
	 _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin/glEnd");

      if (IM->CopyStart == IM->Start &&
	  IM->Flag[IM->Start] & (VERT_END|VERT_END_VB))
      {
      }
      else
      {
	 IM->Primitive[IM->CopyStart] = ctx->Driver.CurrentExecPrimitive;
	 if (tnl->ExecParity)
	    IM->Primitive[IM->CopyStart] |= PRIM_PARITY;


	 for (i = IM->Start ; i <= IM->Count ; i += IM->PrimitiveLength[i])
	    if (IM->Flag[i] & (VERT_END|VERT_END_VB)) {
	       IM->PrimitiveLength[IM->CopyStart] = i - IM->CopyStart;
	       if (IM->Flag[i] & VERT_END_VB) {
		  IM->Primitive[IM->CopyStart] |= PRIM_LAST;
		  IM->LastPrimitive = IM->CopyStart;
	       }
	       if (IM->Flag[i] & VERT_END) {
		  IM->Primitive[IM->CopyStart] |= PRIM_END;
	       }
	       break;
	    }
      }
   }

   if (IM->Primitive[IM->LastPrimitive] & PRIM_END)
      ctx->Driver.CurrentExecPrimitive = GL_POLYGON+1;
   else
      ctx->Driver.CurrentExecPrimitive =
	 IM->Primitive[IM->LastPrimitive] & PRIM_MODE_MASK;
}


/* Undo any changes potentially made to the immediate in the range
 * IM->Start..IM->Count above.
 */
void _tnl_restore_compiled_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   IM->Primitive[IM->Start] = tnl->DlistPrimitive;
   IM->PrimitiveLength[IM->Start] = tnl->DlistPrimitiveLength;
}






static void copy_none( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf)
{
   (void) (start && ovf && tnl && count);
}

static void copy_last( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf)
{
   (void) start; (void) ovf;
   tnl->ExecCopyCount = 1;
   tnl->ExecCopyElts[2] = count-1;
}

static void copy_first_and_last( TNLcontext *tnl, GLuint start, GLuint count,
				 GLuint ovf)
{
   (void) ovf;
   tnl->ExecCopyCount = 2;
   tnl->ExecCopyElts[1] = start;
   tnl->ExecCopyElts[2] = count-1;
}

static void copy_last_two( TNLcontext *tnl, GLuint start, GLuint count,
			   GLuint ovf )
{
   (void) start;
   tnl->ExecCopyCount = 2+ovf;
   tnl->ExecCopyElts[0] = count-3;
   tnl->ExecCopyElts[1] = count-2;
   tnl->ExecCopyElts[2] = count-1;
}

static void copy_overflow( TNLcontext *tnl, GLuint start, GLuint count,
			   GLuint ovf )
{
   (void) start;
   tnl->ExecCopyCount = ovf;
   tnl->ExecCopyElts[0] = count-3;
   tnl->ExecCopyElts[1] = count-2;
   tnl->ExecCopyElts[2] = count-1;
}


typedef void (*copy_func)( TNLcontext *tnl, GLuint start, GLuint count,
			   GLuint ovf );

static copy_func copy_tab[GL_POLYGON+2] =
{
   copy_none,
   copy_overflow,
   copy_first_and_last,
   copy_last,
   copy_overflow,
   copy_last_two,
   copy_first_and_last,
   copy_overflow,
   copy_last_two,
   copy_first_and_last,
   copy_none
};





/* Figure out what vertices need to be copied next time.
 */
void
_tnl_get_exec_copy_verts( GLcontext *ctx, struct immediate *IM )
{
   static const GLuint increment[GL_POLYGON+2] = { 1,2,1,1,3,1,1,4,2,1,1 };
   static const GLuint intro[GL_POLYGON+2]     = { 0,0,2,2,0,2,2,0,2,2,0 };

   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint last = IM->LastPrimitive;
   GLuint prim = ctx->Driver.CurrentExecPrimitive;
   GLuint pincr = increment[prim];
   GLuint pintro = intro[prim];
   GLuint ovf = 0;


   if (tnl->ExecCopySource != IM) {
      if (--tnl->ExecCopySource->ref_count == 0)
	 _tnl_free_immediate( tnl->ExecCopySource );
      IM->ref_count++;
      tnl->ExecCopySource = IM;
   }

   if (prim == GL_POLYGON+1) {
      tnl->ExecCopyCount = 0;
      tnl->ExecCopyTexSize = 0;
      tnl->ExecParity = 0;
   } else {
      tnl->ExecCopyCount = 0;
      tnl->ExecCopyTexSize = IM->CopyTexSize;
      tnl->ExecParity = IM->PrimitiveLength[IM->LastPrimitive] & 1;

      if (pincr != 1 && (IM->Count - last - pintro))
	 ovf = (IM->Count - last - pintro) % pincr;

      if (last < IM->Count)
	 copy_tab[prim]( tnl, last, IM->Count, ovf );
   }
}


/* If we receive evalcoords in an immediate struct for maps which
 * don't have a vertex enabled, need to do an additional fixup, as
 * those rows containing evalcoords must now be ignored.  The
 * evalcoords may still generate colors, normals, etc, so have to
 * respect the relative order between calls to EvalCoord and Normal
 * etc.
 *
 * Generate the index list that will be used to render this immediate
 * struct.
 *
 * Finally, generate a new primitives list for rendering the indices.
 */
#if 0
void _tnl_fixup_purged_eval( GLcontext *ctx,
			     GLuint fixup, GLuint purge )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_eval_store *store = &tnl->eval;
   GLuint *flags = tnl->vb.Flag;
   GLuint i, j, nextprim;
   GLuint fixup_fence = purge|VERT_OBJ;
   GLuint good_index = (VERT_EVAL_ANY & ~purge)|VERT_OBJ;
   GLuint prim_length = 0, lastprim = 0, nextprim = 0;

   if (fixup & VERT_TEX0)
      fixup_4f( store->TexCoord, flags, 0, VERT_TEX0|fixup_fence );

   if (fixup & VERT_INDEX)
      fixup_1ui( store->Index, flags, 0, VERT_INDEX|fixup_fence );

   if (fixup & VERT_RGBA)
      fixup_4chan( store->Color, flags, 0, VERT_RGBA|fixup_fence );

   if (fixup & VERT_NORM)
      fixup_3f( store->Normal, flags, 0, VERT_NORM|fixup_fence );

   for (i = 0, j = 0 ; i < tnl->vb.Count ; i++) {
      if (flags[i] & good_index) {
	 store->Elts[j++] = i;
	 prim_length++;
      }
      if (i == nextprim) {
	 VB->PrimitiveLength[lastprim] = prim_length;
	 VB->Primitive[j] = VB->Primitive[i];
	 nextprim += lastprimlen;
	 lastprim = i;
	 lastprimlen = VB->PrimitiveLength[i];
      }
   }

   VB->Elts = store->Elts;

   /* What about copying???  No immediate exists with the right
    * vertices in place...
    */
   if (tnl->CurrentPrimitive != GL_POLYGON+1) {
   }
}
#endif
