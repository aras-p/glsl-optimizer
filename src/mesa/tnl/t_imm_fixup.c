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
 */

/*
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "dlist.h"
#include "colormac.h"
#include "light.h"
#include "macros.h"
#include "imports.h"
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


/*
 * Indexed by primitive type (GL_POINTS=0, GL_LINES=1, GL_LINE_LOOP=2, etc)
 */
static const GLuint increment[GL_POLYGON+2] = { 1,2,1,1,3,1,1,4,2,1,1 };
static const GLuint intro[GL_POLYGON+2]     = { 0,0,2,2,0,2,2,0,2,2,0 };


void
_tnl_fixup_4f( GLfloat data[][4], const GLuint flag[],
               GLuint start, GLuint match )
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 COPY_4FV(data[i], data[i-1]);
	 if (flag[i] & VERT_BIT_END_VB) break;
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
	 if (flag[i] & VERT_BIT_END_VB) break;
      }
   }
   flag[i] |= match;
}


void
_tnl_fixup_1ub( GLubyte *data, GLuint flag[], GLuint start, GLuint match)
{
   GLuint i = start;

   for (;;) {
      if ((flag[++i] & match) == 0) {
	 data[i] = data[i-1];
	 if (flag[i] & VERT_BIT_END_VB) break;
      }
   }
   flag[i] |= match;
}


static void
fixup_first_4f( GLfloat data[][4], const GLuint flag[], GLuint match,
		GLuint start, const GLfloat *dflt )
{
   GLuint i = start-1;
   match |= VERT_BIT_END_VB;

   while ((flag[++i]&match) == 0)
      COPY_4FV(data[i], dflt);
}


static void
fixup_first_1ui( GLuint data[], const GLuint flag[], GLuint match,
		 GLuint start, const GLuint dflt )
{
   GLuint i = start-1;
   match |= VERT_BIT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}


static void
fixup_first_1ub( GLubyte data[], const GLuint flag[], GLuint match,
		 GLuint start, GLubyte dflt )
{
   GLuint i = start-1;
   match |= VERT_BIT_END_VB;

   while ((flag[++i]&match) == 0)
      data[i] = dflt;
}


/**
 * Copy vertex attributes from the ctx->Current group into the immediate
 * struct at the given position according to copyMask.
 */
static void
copy_from_current( GLcontext *ctx, struct immediate *IM, 
                   GLuint pos, GLuint copyMask )
{
   GLuint attrib, attribBit;

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      _tnl_print_vert_flags("copy from current", copyMask); 

   for (attrib = 0, attribBit = 1; attrib < 16; attrib++, attribBit <<= 1) {
      if (copyMask & attribBit) {
         if (!IM->Attrib[attrib]) {
            IM->Attrib[attrib] = _mesa_malloc(IMM_SIZE * 4 * sizeof(GLfloat));
            if (!IM->Attrib[attrib]) {
               _mesa_error(ctx, GL_OUT_OF_MEMORY, "vertex processing3");
               return;
            }
         }
         COPY_4FV( IM->Attrib[attrib][pos], ctx->Current.Attrib[attrib]);
      }
   }

   if (copyMask & VERT_BIT_INDEX)
      IM->Index[pos] = ctx->Current.Index;

   if (copyMask & VERT_BIT_EDGEFLAG)
      IM->EdgeFlag[pos] = ctx->Current.EdgeFlag;
}


/**
 * Fill in missing vertex attributes in the incoming immediate structure.
 * For example, suppose the following calls are made:
 * glBegin()
 * glColor(c1)
 * glVertex(v1)
 * glVertex(v2)
 * glEnd()
 * The v2 vertex should get color c1 since glColor wasn't called for v2.
 * This function will make c2 be c1.  Same story for all vertex attribs.
 */
void
_tnl_fixup_input( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint start = IM->CopyStart;
   GLuint andflag = IM->CopyAndFlag;
   GLuint orflag = IM->CopyOrFlag | IM->Evaluated;
   GLuint fixup;

   IM->CopyTexSize = IM->TexSize;

/*     _mesa_debug(ctx, "Fixup input, Start: %u Count: %u LastData: %u\n", */
/*  	   IM->Start, IM->Count, IM->LastData); */
/*     _tnl_print_vert_flags("Orflag", orflag); */
/*     _tnl_print_vert_flags("Andflag", andflag); */


   fixup = ~andflag & VERT_BITS_FIXUP;

   if (!ctx->CompileFlag)
      fixup &= tnl->pipeline.inputs;

   if (!ctx->ExecuteFlag)
      fixup &= orflag;

   if ((orflag & (VERT_BIT_POS|VERT_BITS_EVAL_ANY)) == 0)
      fixup = 0;

   if (fixup) {
      const GLuint copy = fixup & ~IM->Flag[start];
      GLuint attr;

      /* Equivalent to a lazy copy-from-current when setting up the
       * immediate.
       */
      if (ctx->ExecuteFlag && copy) 
	 copy_from_current( ctx, IM, start, copy );

      if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
  	 _tnl_print_vert_flags("fixup", fixup); 

      for (attr = 1; attr < VERT_ATTRIB_MAX; attr++) { /* skip 0 (POS) */
         const GLuint attrBit = 1 << attr;
         if (fixup & attrBit) {
            if (!IM->Attrib[attr]) {
               IM->Attrib[attr] = _mesa_malloc(IMM_SIZE * 4 * sizeof(GLfloat));
               if (!IM->Attrib[attr]) {
                  _mesa_error(ctx, GL_OUT_OF_MEMORY, "vertex processing");
                  return;
               }
            }

            if (attr == VERT_BIT_COLOR0) {
               /* special case, darn - try to remove someday */
               if (orflag & VERT_BIT_COLOR0) {
                  _tnl_fixup_4f( IM->Attrib[VERT_ATTRIB_COLOR0], IM->Flag,
                                 start, VERT_BIT_COLOR0 );
               }
               /* No need for else case as the drivers understand stride
                * zero here.  (TODO - propogate this)
                */
            }
            else {
               /* general case */
               if (orflag & attrBit)
                  _tnl_fixup_4f( IM->Attrib[attr], IM->Flag, start, attrBit );
               else
                  fixup_first_4f( IM->Attrib[attr], IM->Flag, VERT_BIT_END_VB,
                                  start, IM->Attrib[attr][start] );
            }
         }
      }

      if (fixup & VERT_BIT_EDGEFLAG) {
	 if (orflag & VERT_BIT_EDGEFLAG)
	    _tnl_fixup_1ub( IM->EdgeFlag, IM->Flag, start, VERT_BIT_EDGEFLAG );
	 else
	    fixup_first_1ub( IM->EdgeFlag, IM->Flag, VERT_BIT_END_VB, start,
			     IM->EdgeFlag[start] );
      }

      if (fixup & VERT_BIT_INDEX) {
	 if (orflag & VERT_BIT_INDEX)
	    _tnl_fixup_1ui( IM->Index, IM->Flag, start, VERT_BIT_INDEX );
	 else
	    fixup_first_1ui( IM->Index, IM->Flag, VERT_BIT_END_VB, start, 
			     IM->Index[start] );
      }

   }
      
   /* Prune possible half-filled slot.
    */
   IM->Flag[IM->LastData+1] &= ~VERT_BIT_END_VB;
   IM->Flag[IM->Count] |= VERT_BIT_END_VB;


   /* Materials:
    */
   if (IM->MaterialOrMask & ~IM->MaterialAndMask) {
      GLuint vulnerable = IM->MaterialOrMask;
      GLuint i = IM->Start;

      do {
	 while (!(IM->Flag[i] & VERT_BIT_MATERIAL))
	    i++;

	 vulnerable &= ~IM->MaterialMask[i];
	 _mesa_copy_material_pairs( IM->Material[i],
				    ctx->Light.Material,
				    vulnerable );


	++i;
      } while (vulnerable);
   }
}


static void
copy_material( struct immediate *next,
               struct immediate *prev,
               GLuint dst, GLuint src )
{
/*     _mesa_debug(NULL, "%s\n", __FUNCTION__); */

   if (next->Material == 0) {
      next->Material = (struct gl_material (*)[2])
         MALLOC( sizeof(struct gl_material) * IMM_SIZE * 2 );
      next->MaterialMask = (GLuint *) MALLOC( sizeof(GLuint) * IMM_SIZE );
   }

   next->MaterialMask[dst] = prev->MaterialOrMask;
   MEMCPY(next->Material[dst], prev->Material[src],
          2 * sizeof(struct gl_material));
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


/**
 * Copy the untransformed data from the shared vertices of a primitive
 * that wraps over two immediate structs.  This is done prior to
 * set_immediate so that prev and next may point to the same
 * structure.  In general it's difficult to avoid this copy on long
 * primitives.
 *
 * Have to be careful with the transitions between display list
 * replay, compile and normal execute modes.
 */
void
_tnl_copy_immediate_vertices( GLcontext *ctx, struct immediate *next )
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

   if ((prev->CopyOrFlag & VERT_BITS_DATA) == VERT_BIT_ELT &&
       ctx->Array.LockCount &&
       (ctx->Array.Vertex.Enabled ||
        (ctx->VertexProgram.Enabled && ctx->Array.VertexAttrib[0].Enabled)))
   {
      /* Copy Elt values only
       */
      for (i = 0 ; i < count ; i++)
      {
	 GLuint src = elts[i+offset];
	 GLuint dst = next->CopyStart+i;
	 next->Elt[dst] = prev->Elt[src];
	 next->Flag[dst] = VERT_BIT_ELT;
	 elts[i+offset] = dst;
      }
/*        _mesa_debug(ctx, "ADDING VERT_BIT_ELT!\n"); */
      next->CopyOrFlag |= VERT_BIT_ELT;
      next->CopyAndFlag &= VERT_BIT_ELT;
   }
   else {
      GLuint copy = tnl->pipeline.inputs & (prev->CopyOrFlag|prev->Evaluated);
      GLuint flag, attr;

      if (is_fan_like[ctx->Driver.CurrentExecPrimitive]) {
	 flag = ((prev->CopyOrFlag|prev->Evaluated) & VERT_BITS_FIXUP);
	 next->CopyOrFlag |= flag;
      } 
      else {
	 /* Don't let an early 'glColor', etc. poison the elt path.
	  */
	 flag = ((prev->OrFlag|prev->Evaluated) & VERT_BITS_FIXUP);
      }

      next->TexSize |= tnl->ExecCopyTexSize;
      next->CopyAndFlag &= flag;
	 

/*        _tnl_print_vert_flags("copy vertex components", copy); */
/*        _tnl_print_vert_flags("prev copyorflag", prev->CopyOrFlag); */
/*        _tnl_print_vert_flags("flag", flag); */

      /* Allocate attribute arrays in the destination immediate struct */
      for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
         if ((copy & (1 << attr)) && !next->Attrib[attr]) {
            next->Attrib[attr] = _mesa_malloc(IMM_SIZE * 4 * sizeof(GLfloat));
            if (!next->Attrib[attr]) {
               _mesa_error(ctx, GL_OUT_OF_MEMORY, "vertex processing");
               return;
            }
         }
      }

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
          *
          * XXX try to use a loop over vertex attribs here.
	  */
	 COPY_4FV( next->Attrib[VERT_ATTRIB_POS][dst],
                   inputs->Obj.data[isrc] );

	 if (copy & VERT_BIT_NORMAL) {
	    COPY_3FV( next->Attrib[VERT_ATTRIB_NORMAL][dst],
                      inputs->Normal.data[isrc] );
	 }

	 if (copy & VERT_BIT_COLOR0)
	    COPY_4FV( next->Attrib[VERT_ATTRIB_COLOR0][dst], 
		      ((GLfloat (*)[4])inputs->Color.Ptr)[isrc] );

	 if (copy & VERT_BITS_TEX_ANY) {
	    GLuint i;
	    for (i = 0 ; i < prev->MaxTextureUnits ; i++) {
	       if (copy & VERT_BIT_TEX(i))
		  COPY_4FV( next->Attrib[VERT_ATTRIB_TEX0 + i][dst], 
			    inputs->TexCoord[i].data[isrc] );
	    }
	 }

         /* the rest aren't used for evaluators */
         if (copy & VERT_BIT_COLOR1)
	    COPY_4FV( next->Attrib[VERT_ATTRIB_COLOR1][dst],
                      prev->Attrib[VERT_ATTRIB_COLOR1][src] );

         if (copy & VERT_BIT_FOG)
	    COPY_4FV( next->Attrib[VERT_ATTRIB_FOG][dst],
                      prev->Attrib[VERT_ATTRIB_FOG][src] );


	 if (copy & VERT_BIT_INDEX)
	    next->Index[dst] = inputs->Index.data[isrc];

         if (copy & VERT_BIT_EDGEFLAG)
	    next->EdgeFlag[dst] = prev->EdgeFlag[src];

	 if (copy & VERT_BIT_ELT)
	    next->Elt[dst] = prev->Elt[src];

         if (copy & VERT_BIT_MATERIAL)
	    if (prev->Flag[src] & VERT_BIT_MATERIAL)
	       copy_material(next, prev, dst, src);

	 next->Flag[dst] = flag;
	 next->CopyOrFlag |= prev->Flag[src] & (VERT_BITS_FIXUP|
						VERT_BIT_MATERIAL|
						VERT_BIT_POS);
 	 elts[i+offset] = dst;
      }
   }

   if (--tnl->ExecCopySource->ref_count == 0) 
      _tnl_free_immediate( ctx, tnl->ExecCopySource );
  
   tnl->ExecCopySource = next; next->ref_count++;
}



/**
 * Revive a compiled immediate struct - propogate new 'Current'
 * values.  Often this is redundant because the current values were
 * known and fixed up at compile time (or in the first execution of
 * the cassette).
 */
void
_tnl_fixup_compiled_cassette( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint fixup;
   GLuint start = IM->Start;

/*     _mesa_debug(ctx, "%s\n", __FUNCTION__); */

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
   if (IM->CopyOrFlag & VERT_BIT_ELT) {
      GLuint copy = tnl->pipeline.inputs & ~ctx->Array._Enabled;
      GLuint i;

      ASSERT(IM->CopyStart < IM->Start);

      _tnl_translate_array_elts( ctx, IM, IM->CopyStart, IM->Start );

      for (i = IM->CopyStart ; i < IM->Start ; i++)
	 copy_from_current( ctx, IM, i, copy ); 

      _tnl_copy_to_current( ctx, IM, ctx->Array._Enabled, IM->Start );
   }

   fixup = tnl->pipeline.inputs & ~IM->Flag[start] & VERT_BITS_FIXUP;

/*     _tnl_print_vert_flags("fixup compiled", fixup); */

   if (fixup) {
      GLuint attr;

      for (attr = 1; attr < VERT_ATTRIB_MAX; attr++) { /* skip 0 (POS) */
         const GLuint attrBit = 1 << attr;
         if (fixup & attrBit) {
            if (!IM->Attrib[attr]) {
               IM->Attrib[attr] = _mesa_malloc(IMM_SIZE * 4 * sizeof(GLfloat));
               if (!IM->Attrib[attr]) {
                  _mesa_error(ctx, GL_OUT_OF_MEMORY, "vertex processing");
               }
            }
            if (attr == VERT_ATTRIB_COLOR0) {
               /* special case, darn */
               if (IM->CopyOrFlag & VERT_BIT_COLOR0)
                  fixup_first_4f(IM->Attrib[VERT_ATTRIB_COLOR0], IM->Flag,
                                 VERT_BIT_COLOR0, start,
                                 ctx->Current.Attrib[VERT_ATTRIB_COLOR0] );
               else
                  fixup &= ~VERT_BIT_COLOR0;
            }
            else {
               fixup_first_4f(IM->Attrib[attr], IM->Flag,
                              attrBit, start, ctx->Current.Attrib[attr] );
            }
         }
      }

      if (fixup & VERT_BIT_EDGEFLAG)
	 fixup_first_1ub(IM->EdgeFlag, IM->Flag, VERT_BIT_EDGEFLAG, start,
			 ctx->Current.EdgeFlag );

      if (fixup & VERT_BIT_INDEX)
	 fixup_first_1ui(IM->Index, IM->Flag, VERT_BIT_INDEX, start,
			 ctx->Current.Index );

      IM->CopyOrFlag |= fixup;
   }
   

   /* Materials:
    */
   if (IM->MaterialOrMask & ~IM->MaterialAndMask) {
      GLuint vulnerable = IM->MaterialOrMask;
      GLuint i = IM->Start;

      do {
	 while (!(IM->Flag[i] & VERT_BIT_MATERIAL))
	    i++;

	 vulnerable &= ~IM->MaterialMask[i];
	 _mesa_copy_material_pairs( IM->Material[i],
				    ctx->Light.Material,
				    vulnerable );


	 ++i;
      } while (vulnerable);
   }
}



static void
copy_none( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf)
{
   (void) (start && ovf && tnl && count);
}

static void
copy_last( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf)
{
   (void) start; (void) ovf;
   tnl->ExecCopyCount = 1;
   tnl->ExecCopyElts[2] = count-1;
}

static void
copy_first_and_last( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf)
{
   (void) ovf;
   tnl->ExecCopyCount = 2;
   tnl->ExecCopyElts[1] = start;
   tnl->ExecCopyElts[2] = count-1;
}

static void
copy_last_three( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf )
{
   (void) start;
   tnl->ExecCopyCount = 2+ovf;
   tnl->ExecCopyElts[0] = count-3;
   tnl->ExecCopyElts[1] = count-2;
   tnl->ExecCopyElts[2] = count-1;
}

static void
copy_overflow( TNLcontext *tnl, GLuint start, GLuint count, GLuint ovf )
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
   copy_none,            /* GL_POINTS */
   copy_overflow,        /* GL_LINES */
   copy_first_and_last,  /* GL_LINE_LOOP */
   copy_last,            /* GL_LINE_STRIP */
   copy_overflow,        /* GL_TRIANGLES */
   copy_last_three,      /* GL_TRIANGLE_STRIP */
   copy_first_and_last,  /* GL_TRIANGLE_FAN */
   copy_overflow,        /* GL_QUADS */
   copy_last_three,      /* GL_QUAD_STRIP */
   copy_first_and_last,  /* GL_POLYGON */
   copy_none
};



/**
 * Figure out what vertices need to be copied next time.
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

/*     _mesa_debug(ctx, "_tnl_get_exec_copy_verts %s\n",  */
/*  	   _mesa_lookup_enum_by_nr(prim)); */

   if (tnl->ExecCopySource)
      if (--tnl->ExecCopySource->ref_count == 0) 
	 _tnl_free_immediate( ctx, tnl->ExecCopySource );

   if (prim == GL_POLYGON+1) {
      tnl->ExecCopySource = 0;
      tnl->ExecCopyCount = 0;
      tnl->ExecCopyTexSize = 0;
      tnl->ExecParity = 0;
   }
   else {
      /* Remember this immediate as the one to copy from.
       */
      tnl->ExecCopySource = IM; IM->ref_count++;
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


/**
 * Recalculate ExecCopyElts, ExecParity, etc.  
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


/**
 * Called via the VB->import_data function pointer.
 */
void
_tnl_upgrade_current_data( GLcontext *ctx, GLuint required, GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct immediate *IM = (struct immediate *)VB->import_source;

   ASSERT(IM);

/*     _tnl_print_vert_flags("_tnl_upgrade_client_data", required); */

   if ((required & VERT_BIT_COLOR0) && (VB->ColorPtr[0]->Flags & CA_CLIENT_DATA)) {
      struct gl_client_array *tmp = &tnl->imm_inputs.Color;
      GLuint start = IM->CopyStart;

      tmp->Ptr = IM->Attrib[VERT_ATTRIB_COLOR0] + start;
      tmp->StrideB = 4 * sizeof(GLfloat);
      tmp->Flags = 0;

      COPY_4FV( IM->Attrib[VERT_ATTRIB_COLOR0][start],
                ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);   

      /*
      ASSERT(IM->Flag[IM->LastData+1] & VERT_BIT_END_VB);
      */

      fixup_first_4f( IM->Attrib[VERT_ATTRIB_COLOR0], IM->Flag,
                      VERT_BIT_END_VB,
                      start, IM->Attrib[VERT_ATTRIB_COLOR0][start] );

      VB->importable_data &= ~VERT_BIT_COLOR0;
   }
}
