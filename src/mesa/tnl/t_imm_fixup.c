/* $Id: t_imm_fixup.c,v 1.32 2002/01/06 03:54:12 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
#include "light.h"
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
#include "t_imm_elt.h"
#include "t_imm_fixup.h"
#include "t_imm_exec.h"
#include "t_pipeline.h"


static const GLuint increment[GL_POLYGON+2] = { 1,2,1,1,3,1,1,4,2,1,1 };
static const GLuint intro[GL_POLYGON+2]     = { 0,0,2,2,0,2,2,0,2,2,0 };

void
_tnl_fixup_4f( GLfloat data[][4], GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 COPY_4FV(data[i], data[i-1]);
	 if (flag[i] & VERT_END_VB) break;
      }
   }
}

void
_tnl_fixup_3f( float data[][3], GLuint flag[], GLuint start, GLuint match )
{
   GLuint i = start;


   for (;;) {
      if ((flag[++i] & match) == 0) {
/*  	 fprintf(stderr, "_tnl_fixup_3f copy to %p values %f %f %f\n", */
/*  		 data[i],  */
/*  		 data[i-1][0], */
/*  		 data[i-1][1], */
/*  		 data[i-1][2]); */
	 COPY_3V(data[i], data[i-1]);
	 if (flag[i] & VERT_END_VB) break;
      }
   }
}


void
_tnl_fixup_1ui( GLuint *data, GLuint flag[], GLuint start, GLuint match )
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


void
_tnl_fixup_1f( GLfloat *data, GLuint flag[], GLuint start, GLuint match )
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

void
_tnl_fixup_1ub( GLubyte *data, GLuint flag[], GLuint start, GLuint match )
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
fixup_first_4f( GLfloat data[][4], GLuint flag[], GLuint match,
		GLuint start, GLfloat *dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      COPY_4FV(data[i], dflt);
}

#if 0
static void
fixup_first_3f( GLfloat data[][3], GLuint flag[], GLuint match,
		GLuint start, GLfloat *dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

/*     fprintf(stderr, "fixup_first_3f default: %f %f %f start: %d\n", */
/*  	   dflt[0], dflt[1], dflt[2], start);  */

   while ((flag[++i]&match) == 0)
      COPY_3FV(data[i], dflt);
}
#endif

static void
fixup_first_1ui( GLuint data[], GLuint flag[], GLuint match,
		 GLuint start, GLuint dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}

#if 00
static void
fixup_first_1f( GLfloat data[], GLuint flag[], GLuint match,
		GLuint start, GLfloat dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}
#endif

static void
fixup_first_1ub( GLubyte data[], GLuint flag[], GLuint match,
		 GLuint start, GLubyte dflt )
{
   GLuint i = start-1;
   match |= VERT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}

/*
 * Copy vertex attributes from the ctx->Current group into the immediate
 * struct at the given position according to copyMask.
 */
static void copy_from_current( GLcontext *ctx, struct immediate *IM, 
			      GLuint pos, GLuint copyMask )
{
   GLuint attrib, attribBit;

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      _tnl_print_vert_flags("copy from current", copyMask); 

#if 0
   if (copyMask & VERT_NORMAL_BIT) {
      COPY_4V(IM->Attrib[VERT_ATTRIB_NORMAL][pos],
              ctx->Current.Attrib[VERT_ATTRIB_NORMAL]);
   }

   if (copyMask & VERT_COLOR0_BIT) {
      COPY_4FV( IM->Attrib[VERT_ATTRIB_COLOR0][pos],
                ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
   }

   if (copyMask & VERT_COLOR1_BIT)
      COPY_4FV( IM->Attrib[VERT_ATTRIB_COLOR1][pos],
                ctx->Current.Attrib[VERT_ATTRIB_COLOR1]);

   if (copyMask & VERT_FOG_BIT)
      IM->Attrib[VERT_ATTRIB_FOG][pos][0] = ctx->Current.Attrib[VERT_ATTRIB_FOG][0];

   if (copyMask & VERT_TEX_ANY) {
      GLuint i;
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	 if (copyMask & VERT_TEX(i))
            COPY_4FV(IM->Attrib[VERT_ATTRIB_TEX0 + i][pos],
                     ctx->Current.Attrib[VERT_ATTRIB_TEX0 + i]);
      }
   }
#else
   for (attrib = 0, attribBit = 1; attrib < 16; attrib++, attribBit <<= 1) {
      if (copyMask & attribBit) {
         COPY_4FV( IM->Attrib[attrib][pos], ctx->Current.Attrib[attrib]);
      }
   }
#endif

   if (copyMask & VERT_INDEX_BIT)
      IM->Index[pos] = ctx->Current.Index;

   if (copyMask & VERT_EDGEFLAG_BIT)
      IM->EdgeFlag[pos] = ctx->Current.EdgeFlag;
}


void _tnl_fixup_input( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint start = IM->CopyStart;
   GLuint andflag = IM->CopyAndFlag;
   GLuint orflag = IM->CopyOrFlag | IM->Evaluated;
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

   if ((orflag & (VERT_OBJ_BIT|VERT_EVAL_ANY)) == 0)
      fixup = 0;

   if (fixup) {
      GLuint copy = fixup & ~IM->Flag[start];


      /* Equivalent to a lazy copy-from-current when setting up the
       * immediate.
       */
      if (ctx->ExecuteFlag && copy) 
	 copy_from_current( ctx, IM, start, copy );

      if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
  	 _tnl_print_vert_flags("fixup", fixup); 

      /* XXX replace these conditionals with a loop over the 16
       * vertex attributes.
       */

      if (fixup & VERT_TEX_ANY) {
	 GLuint i;
	 for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	    if (fixup & VERT_TEX(i)) {
	       if (orflag & VERT_TEX(i))
		  _tnl_fixup_4f( IM->Attrib[VERT_ATTRIB_TEX0 + i], IM->Flag,
                                 start, VERT_TEX(i) );
	       else
		  fixup_first_4f( IM->Attrib[VERT_ATTRIB_TEX0 + i], IM->Flag,
                                  VERT_END_VB, start,
				  IM->Attrib[VERT_ATTRIB_TEX0 + i][start]);
	    }
	 }
      }
   

      if (fixup & VERT_EDGEFLAG_BIT) {
	 if (orflag & VERT_EDGEFLAG_BIT)
	    _tnl_fixup_1ub( IM->EdgeFlag, IM->Flag, start, VERT_EDGEFLAG_BIT );
	 else
	    fixup_first_1ub( IM->EdgeFlag, IM->Flag, VERT_END_VB, start,
			     IM->EdgeFlag[start] );
      }

      if (fixup & VERT_INDEX_BIT) {
	 if (orflag & VERT_INDEX_BIT)
	    _tnl_fixup_1ui( IM->Index, IM->Flag, start, VERT_INDEX_BIT );
	 else
	    fixup_first_1ui( IM->Index, IM->Flag, VERT_END_VB, start, 
			     IM->Index[start] );
      }

      if (fixup & VERT_COLOR0_BIT) {
	 if (orflag & VERT_COLOR0_BIT)
	    _tnl_fixup_4f( IM->Attrib[VERT_ATTRIB_COLOR0], IM->Flag, start,
                           VERT_COLOR0_BIT );
	 /* No need for else case as the drivers understand stride
	  * zero here.  (TODO - propogate this)
	  */
      }
      
      if (fixup & VERT_COLOR1_BIT) {
	 if (orflag & VERT_COLOR1_BIT)
	    _tnl_fixup_4f( IM->Attrib[VERT_ATTRIB_COLOR1], IM->Flag, start, 
			   VERT_COLOR1_BIT );
	 else
	    fixup_first_4f( IM->Attrib[VERT_ATTRIB_COLOR1], IM->Flag, VERT_END_VB, start,
			    IM->Attrib[VERT_ATTRIB_COLOR1][start] );
      }
      
      if (fixup & VERT_FOG_BIT) {
	 if (orflag & VERT_FOG_BIT)
	    _tnl_fixup_4f( IM->Attrib[VERT_ATTRIB_FOG], IM->Flag,
                           start, VERT_FOG_BIT );
	 else
	    fixup_first_4f( IM->Attrib[VERT_ATTRIB_FOG], IM->Flag, VERT_END_VB,
                            start, IM->Attrib[VERT_ATTRIB_FOG][start] );
      }

      if (fixup & VERT_NORMAL_BIT) {
	 if (orflag & VERT_NORMAL_BIT)
	    _tnl_fixup_4f( IM->Attrib[VERT_ATTRIB_NORMAL], IM->Flag, start,
                           VERT_NORMAL_BIT );
	 else
	    fixup_first_4f( IM->Attrib[VERT_ATTRIB_NORMAL], IM->Flag,
                            VERT_END_VB, start,
			    IM->Attrib[VERT_ATTRIB_NORMAL][start] );
      }
   }
      
   /* Prune possible half-filled slot.
    */
   IM->Flag[IM->LastData+1] &= ~VERT_END_VB;
   IM->Flag[IM->Count] |= VERT_END_VB;


   /* Materials:
    */
   if (IM->MaterialOrMask & ~IM->MaterialAndMask) {
      GLuint vulnerable = IM->MaterialOrMask;
      GLuint i = IM->Start;

      do {
	 while (!(IM->Flag[i] & VERT_MATERIAL))
	    i++;

	 vulnerable &= ~IM->MaterialMask[i];
	 _mesa_copy_material_pairs( IM->Material[i],
				    ctx->Light.Material,
				    vulnerable );


      } while (vulnerable);
   }
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

   next->MaterialMask[dst] = prev->MaterialOrMask;
   MEMCPY(next->Material[dst], prev->Material[src], 2*sizeof(GLmaterial));
}

static GLboolean is_fan_like[GL_POLYGON+1] = {
   GL_FALSE,
   GL_FALSE,
   GL_TRUE,			/* line loop */
   GL_FALSE,
   GL_FALSE,
   GL_FALSE,
   GL_TRUE,			/* tri fan */
   GL_FALSE,
   GL_FALSE,
   GL_TRUE			/* polygon */
};


/* Copy the untransformed data from the shared vertices of a primitive
 * that wraps over two immediate structs.  This is done prior to
 * set_immediate so that prev and next may point to the same
 * structure.  In general it's difficult to avoid this copy on long
 * primitives.
 *
 * Have to be careful with the transitions between display list
 * replay, compile and normal execute modes.
 */
void _tnl_copy_immediate_vertices( GLcontext *ctx, struct immediate *next )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct immediate *prev = tnl->ExecCopySource;
   struct vertex_arrays *inputs = &tnl->imm_inputs;
   GLuint count = tnl->ExecCopyCount;
   GLuint *elts = tnl->ExecCopyElts;
   GLuint offset = IMM_MAX_COPIED_VERTS - count;
   GLuint i;

   if (!prev) {
      ASSERT(tnl->ExecCopyCount == 0);
      return;
   }

   next->CopyStart = next->Start - count;

   if ((prev->CopyOrFlag & VERT_DATA) == VERT_ELT &&
       ctx->Array.LockCount &&
       ctx->Array.Vertex.Enabled)
   {
      /* Copy Elt values only
       */
      for (i = 0 ; i < count ; i++)
      {
	 GLuint src = elts[i+offset];
	 GLuint dst = next->CopyStart+i;
	 next->Elt[dst] = prev->Elt[src];
	 next->Flag[dst] = VERT_ELT;
      }
/*        fprintf(stderr, "ADDING VERT_ELT!\n"); */
      next->CopyOrFlag |= VERT_ELT;
      next->CopyAndFlag &= VERT_ELT;
   }
   else {
      GLuint copy = tnl->pipeline.inputs & (prev->CopyOrFlag|prev->Evaluated);
      GLuint flag;

      if (is_fan_like[ctx->Driver.CurrentExecPrimitive]) {
	 flag = ((prev->CopyOrFlag|prev->Evaluated) & VERT_FIXUP);
	 next->CopyOrFlag |= flag;
      } 
      else {
	 /* Don't let an early 'glColor', etc. poison the elt path.
	  */
	 flag = ((prev->OrFlag|prev->Evaluated) & VERT_FIXUP);
      }

      next->TexSize |= tnl->ExecCopyTexSize;
      next->CopyAndFlag &= flag;
	 

/*        _tnl_print_vert_flags("copy vertex components", copy); */
/*        _tnl_print_vert_flags("prev copyorflag", prev->CopyOrFlag); */
/*        _tnl_print_vert_flags("flag", flag); */

      /* Copy whole vertices
       */
      for (i = 0 ; i < count ; i++)
      {
	 GLuint src = elts[i+offset];
	 GLuint isrc = src - prev->CopyStart;
	 GLuint dst = next->CopyStart+i;

	 /* Values subject to eval must be copied out of the 'inputs'
	  * struct.  (Copied rows should not be evaluated twice).
	  *
	  * Note these pointers are null when inactive.
	  */
	 COPY_4FV( next->Attrib[VERT_ATTRIB_POS][dst],
                   inputs->Obj.data[isrc] );

	 if (copy & VERT_NORMAL_BIT) {
/*  	    fprintf(stderr, "copy vert norm %d to %d (%p): %f %f %f\n", */
/*  		    isrc, dst,  */
/*  		    next->Normal[dst], */
/*  		    inputs->Normal.data[isrc][0], */
/*  		    inputs->Normal.data[isrc][1], */
/*  		    inputs->Normal.data[isrc][2]); */
	    COPY_3FV( next->Attrib[VERT_ATTRIB_NORMAL][dst], inputs->Normal.data[isrc] );
	 }

	 if (copy & VERT_COLOR0_BIT)
	    COPY_4FV( next->Attrib[VERT_ATTRIB_COLOR0][dst], 
		      ((GLfloat (*)[4])inputs->Color.Ptr)[isrc] );

	 if (copy & VERT_INDEX_BIT)
	    next->Index[dst] = inputs->Index.data[isrc];

	 if (copy & VERT_TEX_ANY) {
	    GLuint i;
	    for (i = 0 ; i < prev->MaxTextureUnits ; i++) {
	       if (copy & VERT_TEX(i))
		  COPY_4FV( next->Attrib[VERT_ATTRIB_TEX0 + i][dst], 
			    inputs->TexCoord[i].data[isrc] );
	    }
	 }

	 /* Remaining values should be the same in the 'input' struct and the
	  * original immediate.
	  */
	 if (copy & (VERT_ELT|VERT_EDGEFLAG_BIT|VERT_COLOR1_BIT|VERT_FOG_BIT|
		     VERT_MATERIAL)) {

	    if (prev->Flag[src] & VERT_MATERIAL)
	       copy_material(next, prev, dst, src);

	    next->Elt[dst] = prev->Elt[src];
	    next->EdgeFlag[dst] = prev->EdgeFlag[src];
	    COPY_4FV( next->Attrib[VERT_ATTRIB_COLOR1][dst],
                      prev->Attrib[VERT_ATTRIB_COLOR1][src] );
	    COPY_4FV( next->Attrib[VERT_ATTRIB_FOG][dst],
                      prev->Attrib[VERT_ATTRIB_FOG][src] );
	 }

	 next->Flag[dst] = flag;
	 next->CopyOrFlag |= prev->Flag[src] & (VERT_FIXUP|
						VERT_MATERIAL|
						VERT_OBJ_BIT);
      }
   }

   if (--tnl->ExecCopySource->ref_count == 0)
      _tnl_free_immediate( tnl->ExecCopySource );

   tnl->ExecCopySource = 0;
   tnl->ExecCopyCount = 0;
}



/* Revive a compiled immediate struct - propogate new 'Current'
 * values.  Often this is redundant because the current values were
 * known and fixed up at compile time (or in the first execution of
 * the cassette).
 */
void _tnl_fixup_compiled_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint fixup;
   GLuint start = IM->Start;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   IM->Evaluated = 0;
   IM->CopyOrFlag = IM->OrFlag;	  
   IM->CopyAndFlag = IM->AndFlag; 
   IM->CopyTexSize = IM->TexSize | tnl->ExecCopyTexSize;

   _tnl_copy_immediate_vertices( ctx, IM );

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      ASSERT(IM->CopyStart == IM->Start);
   }

   /* Naked array elements can be copied into the first cassette in a
    * display list.  Need to translate them away:
    */
   if (IM->CopyOrFlag & VERT_ELT) {
      GLuint copy = tnl->pipeline.inputs & ~ctx->Array._Enabled;
      GLuint i;

      ASSERT(IM->CopyStart < IM->Start);

      _tnl_translate_array_elts( ctx, IM, IM->CopyStart, IM->Start );

      for (i = IM->CopyStart ; i < IM->Start ; i++)
	 copy_from_current( ctx, IM, i, copy ); 

      _tnl_copy_to_current( ctx, IM, ctx->Array._Enabled, IM->Start );
   }

   fixup = tnl->pipeline.inputs & ~IM->Flag[start] & VERT_FIXUP;

/*     _tnl_print_vert_flags("fixup compiled", fixup); */

   if (fixup) {

      /* XXX try to replace this code with a loop over the 16 vertex
       * attributes.
       */

      if (fixup & VERT_NORMAL_BIT) {
	 fixup_first_4f(IM->Attrib[VERT_ATTRIB_NORMAL], IM->Flag,
                        VERT_NORMAL_BIT, start,
			ctx->Current.Attrib[VERT_ATTRIB_NORMAL] );
      }

      if (fixup & VERT_COLOR0_BIT) {
	 if (IM->CopyOrFlag & VERT_COLOR0_BIT)
	    fixup_first_4f(IM->Attrib[VERT_ATTRIB_COLOR0], IM->Flag,
                           VERT_COLOR0_BIT, start,
			   ctx->Current.Attrib[VERT_ATTRIB_COLOR0] );
	 else
	    fixup &= ~VERT_COLOR0_BIT;
      }

      if (fixup & VERT_COLOR1_BIT)
	 fixup_first_4f(IM->Attrib[VERT_ATTRIB_COLOR1], IM->Flag,
                        VERT_COLOR1_BIT, start,
			ctx->Current.Attrib[VERT_ATTRIB_COLOR1] );

      if (fixup & VERT_FOG_BIT)
	 fixup_first_4f( IM->Attrib[VERT_ATTRIB_FOG], IM->Flag,
                         VERT_FOG_BIT, start,
                         ctx->Current.Attrib[VERT_ATTRIB_FOG] );

      if (fixup & VERT_TEX_ANY) {
	 GLuint i;
	 for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	    if (fixup & VERT_TEX(i))
	       fixup_first_4f( IM->Attrib[VERT_ATTRIB_TEX0 + i], IM->Flag,
                               VERT_TEX(i), start,
			       ctx->Current.Attrib[VERT_ATTRIB_TEX0 + i] );
	 }
      }

      if (fixup & VERT_EDGEFLAG_BIT)
	 fixup_first_1ub(IM->EdgeFlag, IM->Flag, VERT_EDGEFLAG_BIT, start,
			 ctx->Current.EdgeFlag );

      if (fixup & VERT_INDEX_BIT)
	 fixup_first_1ui(IM->Index, IM->Flag, VERT_INDEX_BIT, start,
			 ctx->Current.Index );

      IM->CopyOrFlag |= fixup;
   }
   

   /* Materials:
    */
   if (IM->MaterialOrMask & ~IM->MaterialAndMask) {
      GLuint vulnerable = IM->MaterialOrMask;
      GLuint i = IM->Start;

      do {
	 while (!(IM->Flag[i] & VERT_MATERIAL))
	    i++;

	 vulnerable &= ~IM->MaterialMask[i];
	 _mesa_copy_material_pairs( IM->Material[i],
				    ctx->Light.Material,
				    vulnerable );


      } while (vulnerable);
   }
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

   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint last = IM->LastPrimitive;
   GLuint prim = ctx->Driver.CurrentExecPrimitive;
   GLuint pincr = increment[prim];
   GLuint pintro = intro[prim];
   GLuint ovf = 0;

/*     fprintf(stderr, "_tnl_get_exec_copy_verts %s\n",  */
/*  	   _mesa_lookup_enum_by_nr(prim)); */

   ASSERT(tnl->ExecCopySource == 0);

   if (prim == GL_POLYGON+1) {
      tnl->ExecCopyCount = 0;
      tnl->ExecCopyTexSize = 0;
      tnl->ExecParity = 0;
   } else {
      /* Remember this immediate as the one to copy from.
       */
      IM->ref_count++;
      tnl->ExecCopySource = IM;
      tnl->ExecCopyCount = 0;
      tnl->ExecCopyTexSize = IM->CopyTexSize;

      if (IM->LastPrimitive != IM->CopyStart)
	 tnl->ExecParity = 0;
	 
      tnl->ExecParity ^= IM->PrimitiveLength[IM->LastPrimitive] & 1;


      if (pincr != 1 && (IM->Count - last - pintro))
	 ovf = (IM->Count - last - pintro) % pincr;

      if (last < IM->Count)
	 copy_tab[prim]( tnl, last, IM->Count, ovf );
   }
}


/* Recalculate ExecCopyElts, ExecParity, etc.  
 */
void 
_tnl_get_purged_copy_verts( GLcontext *ctx, struct immediate *IM ) 
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
      GLuint last = IM->LastPrimitive;
      GLenum prim = IM->Primitive[last];
      GLuint pincr = increment[prim];
      GLuint pintro = intro[prim];
      GLuint ovf = 0, i;

      tnl->ExecCopyCount = 0;
      if (IM->LastPrimitive != IM->CopyStart)
	 tnl->ExecParity = 0;
	 
      tnl->ExecParity ^= IM->PrimitiveLength[IM->LastPrimitive] & 1;

      if (pincr != 1 && (IM->Count - last - pintro))
	 ovf = (IM->Count - last - pintro) % pincr;

      if (last < IM->Count)
	 copy_tab[prim]( tnl, last, IM->Count, ovf );

      for (i = 0 ; i < tnl->ExecCopyCount ; i++)
	 tnl->ExecCopyElts[i] = IM->Elt[tnl->ExecCopyElts[i]];
   }
}


void _tnl_upgrade_current_data( GLcontext *ctx,
				GLuint required,
				GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct immediate *IM = (struct immediate *)VB->import_source;

   ASSERT(IM);

/*     _tnl_print_vert_flags("_tnl_upgrade_client_data", required); */

   if ((required & VERT_COLOR0_BIT) && (VB->ColorPtr[0]->Flags & CA_CLIENT_DATA)) {
      struct gl_client_array *tmp = &tnl->imm_inputs.Color;
      GLuint start = IM->CopyStart;

      tmp->Ptr = IM->Attrib[VERT_ATTRIB_COLOR0] + start;
      tmp->StrideB = 4 * sizeof(GLfloat);
      tmp->Flags = 0;

      COPY_4FV( IM->Attrib[VERT_ATTRIB_COLOR0][start],
                ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);   

      /*
      ASSERT(IM->Flag[IM->LastData+1] & VERT_END_VB);
      */

      fixup_first_4f( IM->Attrib[VERT_ATTRIB_COLOR0], IM->Flag, VERT_END_VB,
                      start, IM->Attrib[VERT_ATTRIB_COLOR0][start] );

      VB->importable_data &= ~VERT_COLOR0_BIT;
   }
}
 
