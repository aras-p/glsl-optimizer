/* $Id: api_arrayelt.c,v 1.2 2001/03/12 00:48:37 gareth Exp $ */

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

#include "glheader.h"
#include "api_noop.h"
#include "context.h"
#include "colormac.h"
#include "light.h"
#include "macros.h"
#include "mmath.h"
#include "mtypes.h"


typedef struct {
   GLint unit;
   struct gl_client_array *array;
   void *func;
} AAtexarray;


typedef struct {
   struct gl_client_array *array;
   void *func;
} AAarray;

typedef struct {
   AAtexarray texarrays[MAX_TEXTURE_UNITS+1];
   AAarray arrays[10];
   GLuint NewState;
} AAcontext;


static void *colorfuncs[2][7] = {
   { glColor3bv,
     glColor3ub,
     glColor3sv,
     glColor3usv,
     glColor3iv,
     glColor3fv,
     glColor3dv },

   { glColor4bv,
     glColor4ub,
     glColor4sv,
     glColor4usv,
     glColor4iv,
     glColor4fv,
     glColor4dv }
};

static void *vertexfuncs[3][7] = {
   { glVertex3bv,
     glVertex3ub,
     glVertex3sv,
     glVertex3usv,
     glVertex3iv,
     glVertex3fv,
     glVertex3dv },

   { glVertex3bv,
     glVertex3ub,
     glVertex3sv,
     glVertex3usv,
     glVertex3iv,
     glVertex3fv,
     glVertex3dv },

   { glVertex4bv,
     glVertex4ub,
     glVertex4sv,
     glVertex4usv,
     glVertex4iv,
     glVertex4fv,
     glVertex4dv }
};


static void *multitexfuncs[4][7] = {
   { glMultiTexCoord1bv,
     glMultiTexCoord1ub,
     glMultiTexCoord1sv,
     glMultiTexCoord1usv,
     glMultiTexCoord1iv,
     glMultiTexCoord1fv,
     glMultiTexCoord1dv },

   { glMultiTexCoord2bv,
     glMultiTexCoord2ub,
     glMultiTexCoord2sv,
     glMultiTexCoord2usv,
     glMultiTexCoord2iv,
     glMultiTexCoord2fv,
     glMultiTexCoord2dv },

   { glMultiTexCoord3bv,
     glMultiTexCoord3ub,
     glMultiTexCoord3sv,
     glMultiTexCoord3usv,
     glMultiTexCoord3iv,
     glMultiTexCoord3fv,
     glMultiTexCoord3dv },

   { glMultiTexCoord4bv,
     glMultiTexCoord4ub,
     glMultiTexCoord4sv,
     glMultiTexCoord4usv,
     glMultiTexCoord4iv,
     glMultiTexCoord4fv,
     glMultiTexCoord4dv }
};

static void *indexfuncs[7] = {
   { glIndexbv,
     glIndexub,
     glIndexsv,
     glIndexusv,
     glIndexiv,
     glIndexfv,
     glIndexdv },
};

static void *edgeflagfuncs[7] = {
   { glEdgeFlagbv,
     glEdgeFlagub,
     glEdgeFlagsv,
     glEdgeFlagusv,
     glEdgeFlagiv,
     glEdgeFlagfv,
     glEdgeFlagdv },
};

static void *normalfuncs[7] = {
   { glNormal3bv,
     glNormal3ub,
     glNormal3sv,
     glNormal3usv,
     glNormal3iv,
     glNormal3fv,
     glNormal3dv },
};

static void *fogcoordfuncs[7] = {
   { glFogCoordbv,
     glFogCoordub,
     glFogCoordsv,
     glFogCoordusv,
     glFogCoordiv,
     glFogCoordfv,
     glFogCoorddv },
};

static void *secondarycolorfuncs[7] = {
   { glSecondaryColor3bv,
     glSecondaryColor3ub,
     glSecondaryColor3sv,
     glSecondaryColor3usv,
     glSecondaryColor3iv,
     glSecondaryColor3fv,
     glSecondaryColor3dv },
};


void _aa_create_context( GLcontext *ctx )
{
   ctx->aa_context = MALLOC( sizeof(AAcontext) );
   AA_CONTEXT(ctx)->NewState = ~0;
}

static void _aa_update_state( GLcontext *ctx )
{
   AAcontext *actx = AA_CONTEXT(ctx);
   AAtexarray *ta = actx->texarrays;
   AAarray *aa = actx->arrays;
   int i;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      if (ctx->Array.TexCoord[i].Enabled) {
	 ta->unit = i;
	 ta->array = &ctx->Array.TexCoord[i];
	 ta->func = multitexfuncs[ta->array->Size-1][TYPE_IDX(ta->array->Type)];
	 ta++;
      }

   ta->func = 0;

   if (ctx->Array.Color.Enabled) {
      aa->array = &ctx->Array.Color;
      aa->func = colorfuncs[aa->array->Size-3][TYPE_IDX(aa->array->Type)];
      aa++;
   }

   if (ctx->Array.Normal.Enabled) {
      aa->array = &ctx->Array.Normal;
      aa->func = normalfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }

   if (ctx->Array.Index.Enabled) {
      aa->array = &ctx->Array.Index;
      aa->func = indexfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }

   if (ctx->Array.EdgeFlag.Enabled) {
      aa->array = &ctx->Array.Edgeflag;
      aa->func = edgeflagfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }

   if (ctx->Array.FogCoord.Enabled) {
      aa->array = &ctx->Array.Fogcoord;
      aa->func = fogcoordfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }

   if (ctx->Array.SecondaryColor.Enabled) {
      aa->array = &ctx->Array.SecondaryColor;
      aa->func = secondarycolorfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }

   /* Must be last
    */
   if (ctx->Array.Vertex.Enabled) {
      aa->array = &ctx->Array.Vertex;
      aa->func = vertexfuncs[aa->array->Size-2][TYPE_IDX(aa->array->Type)];
      aa++;
   }

   aa->func = 0;
   actx->NewState = 0;
}


static void _aa_loopback_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   AAcontext *actx = AA_CONTEXT(ctx);
   AAtexarray *ta;
   AAarray *aa;

   for (ta = actx->texarrays ; ta->func ; ta++) {
      void (*func)( GLint, const void * ) =
	 (void (*)( GLint, const void * )) ta->func;
      func( ta->unit, (char *)ta->array->Ptr + elt * ta->array->SizeB );
   }

   for (aa = actx->arrays ; aa->func ; aa++) {
      void (*func)( GLint, const void * ) =
	 (void (*)( GLint, const void * )) aa->func;
      func( (char *)aa->array->Ptr + elt * aa->array->SizeB );
   }
}


void _aa_exec_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   AAcontext *actx = AA_CONTEXT(ctx);

   if (actx->NewState)
      _aa_update_state( ctx );

   ctx->Exec->ArrayElement = _aa_loopback_array_elt;
   _aa_loopback_array_elt( elt );
}

/* This works for save as well:
 */
void _aa_save_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   AAcontext *actx = AA_CONTEXT(ctx);

   if (actx->NewState)
      _aa_update_state( ctx );

   ctx->Save->ArrayElement = _aa_loopback_array_elt;
   _aa_loopback_array_elt( elt );
}


void aa_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   if (AA_CONTEXT(ctx))
      AA_CONTEXT(ctx)->NewState |= new_state;
}
