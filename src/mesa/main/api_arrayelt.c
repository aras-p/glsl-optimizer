/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"

typedef void (GLAPIENTRY *array_func)( const void * );

typedef struct {
   const struct gl_client_array *array;
   array_func func;
} AEarray;

typedef void (GLAPIENTRY *attrib_func)( GLuint indx, const void *data, GLboolean normalized, GLuint size );

typedef struct {
   const struct gl_client_array *array;
   attrib_func func;
   GLuint index;
} AEattrib;

typedef struct {
   AEarray arrays[3]; /* color index, edge flag, null terminator */
   AEattrib attribs[VERT_ATTRIB_MAX + 1];
   GLuint NewState;
} AEcontext;

#define AE_CONTEXT(ctx) ((AEcontext *)(ctx)->aelt_context)

/*
 * Convert GL_BYTE, GL_UNSIGNED_BYTE, .. GL_DOUBLE into an integer
 * in the range [0, 7].  Luckily these type tokens are sequentially
 * numbered in gl.h
 */
#define TYPE_IDX(t) ((t) & 0xf)

static void (GLAPIENTRY *indexfuncs[8])( const void * ) = {
   0,
   (array_func)glIndexubv,
   (array_func)glIndexsv,
   0,
   (array_func)glIndexiv,
   0,
   (array_func)glIndexfv,
   (array_func)glIndexdv
};


/**********************************************************************/

/* 1, 2, 3 or 4 GL_BYTE attribute */
static void VertexAttribbv(GLuint index, const GLbyte *v,
                           GLboolean normalized, GLuint size)
{
   switch (size) {
   case 1:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, BYTE_TO_FLOAT(v[0]),
                                           0, 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], 0, 0, 1);
      return;
   case 2:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, BYTE_TO_FLOAT(v[0]),
                                           BYTE_TO_FLOAT(v[1]), 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], 0, 1);
      return;
   case 3:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, BYTE_TO_FLOAT(v[0]),
                                           BYTE_TO_FLOAT(v[1]),
                                           BYTE_TO_FLOAT(v[2]), 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], 1);
      return;
   case 4:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, BYTE_TO_FLOAT(v[0]),
                                           BYTE_TO_FLOAT(v[1]),
                                           BYTE_TO_FLOAT(v[2]),
                                           BYTE_TO_FLOAT(v[3]));
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribbv");
   }
}

/* 1, 2, 3 or 4 GL_UNSIGNED_BYTE attribute */
static void VertexAttribubv(GLuint index, const GLubyte *v,
                            GLboolean normalized, GLuint size)
{
   switch (size) {
   case 1:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UBYTE_TO_FLOAT(v[0]),
                                           0, 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], 0, 0, 1);
      return;
   case 2:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UBYTE_TO_FLOAT(v[0]),
                                           UBYTE_TO_FLOAT(v[1]), 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], 0, 1);
      return;
   case 3:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UBYTE_TO_FLOAT(v[0]),
                                           UBYTE_TO_FLOAT(v[1]),
                                           UBYTE_TO_FLOAT(v[2]), 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], 1);
      return;
   case 4:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UBYTE_TO_FLOAT(v[0]),
                                           UBYTE_TO_FLOAT(v[1]),
                                           UBYTE_TO_FLOAT(v[2]),
                                           UBYTE_TO_FLOAT(v[3]));
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribubv");
   }
}

/* 1, 2, 3 or 4 GL_SHORT attribute */
static void VertexAttribsv(GLuint index, const GLshort *v,
                           GLboolean normalized, GLuint size)
{
   switch (size) {
   case 1:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, SHORT_TO_FLOAT(v[0]),
                                           0, 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], 0, 0, 1);
      return;
   case 2:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, SHORT_TO_FLOAT(v[0]),
                                           SHORT_TO_FLOAT(v[1]), 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], 0, 1);
      return;
   case 3:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, SHORT_TO_FLOAT(v[0]),
                                           SHORT_TO_FLOAT(v[1]),
                                           SHORT_TO_FLOAT(v[2]), 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], 1);
      return;
   case 4:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, SHORT_TO_FLOAT(v[0]),
                                           SHORT_TO_FLOAT(v[1]),
                                           SHORT_TO_FLOAT(v[2]),
                                           SHORT_TO_FLOAT(v[3]));
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribsv");
   }
}


/* 1, 2, 3 or 4 GL_UNSIGNED_SHORT attribute */
static void VertexAttribusv(GLuint index, const GLushort *v,
                            GLboolean normalized, GLuint size)
{
   switch (size) {
   case 1:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, USHORT_TO_FLOAT(v[0]),
                                           0, 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], 0, 0, 1);
      return;
   case 2:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, USHORT_TO_FLOAT(v[0]),
                                           USHORT_TO_FLOAT(v[1]), 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], 0, 1);
      return;
   case 3:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, USHORT_TO_FLOAT(v[0]),
                                           USHORT_TO_FLOAT(v[1]),
                                           USHORT_TO_FLOAT(v[2]), 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], 1);
      return;
   case 4:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, USHORT_TO_FLOAT(v[0]),
                                           USHORT_TO_FLOAT(v[1]),
                                           USHORT_TO_FLOAT(v[2]),
                                           USHORT_TO_FLOAT(v[3]));
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribusv");
   }
}

/* 1, 2, 3 or 4 GL_INT attribute */
static void VertexAttribiv(GLuint index, const GLint *v,
                           GLboolean normalized, GLuint size)
{
   switch (size) {
   case 1:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, INT_TO_FLOAT(v[0]),
                                           0, 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], 0, 0, 1);
      return;
   case 2:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, INT_TO_FLOAT(v[0]),
                                           INT_TO_FLOAT(v[1]), 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], 0, 1);
      return;
   case 3:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, INT_TO_FLOAT(v[0]),
                                           INT_TO_FLOAT(v[1]),
                                           INT_TO_FLOAT(v[2]), 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], 1);
      return;
   case 4:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, INT_TO_FLOAT(v[0]),
                                           INT_TO_FLOAT(v[1]),
                                           INT_TO_FLOAT(v[2]),
                                           INT_TO_FLOAT(v[3]));
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribiv");
   }
}

/* 1, 2, 3 or 4 GL_UNSIGNED_INT attribute */
static void VertexAttribuiv(GLuint index, const GLuint *v,
                            GLboolean normalized, GLuint size)
{
   switch (size) {
   case 1:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UINT_TO_FLOAT(v[0]),
                                           0, 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], 0, 0, 1);
      return;
   case 2:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UINT_TO_FLOAT(v[0]),
                                           UINT_TO_FLOAT(v[1]), 0, 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], 0, 1);
      return;
   case 3:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UINT_TO_FLOAT(v[0]),
                                           UINT_TO_FLOAT(v[1]),
                                           UINT_TO_FLOAT(v[2]), 1);
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], 1);
      return;
   case 4:
      if (normalized)
         _glapi_Dispatch->VertexAttrib4fNV(index, UINT_TO_FLOAT(v[0]),
                                           UINT_TO_FLOAT(v[1]),
                                           UINT_TO_FLOAT(v[2]),
                                           UINT_TO_FLOAT(v[3]));
      else
         _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribuiv");
   }
}

/* 1, 2, 3 or 4 GL_FLOAT attribute */
static void VertexAttribfv(GLuint index, const GLfloat *v,
                           GLboolean normalized, GLuint size)
{
   (void) normalized;
   switch (size) {
   case 1:
      _glapi_Dispatch->VertexAttrib1fvNV(index, v);
      return;
   case 2:
      _glapi_Dispatch->VertexAttrib2fvNV(index, v);
      return;
   case 3:
      _glapi_Dispatch->VertexAttrib3fvNV(index, v);
      return;
   case 4:
      _glapi_Dispatch->VertexAttrib4fvNV(index, v);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribfv");
   }
}

/* 1, 2, 3 or 4 GL_DOUBLE attribute */
static void VertexAttribdv(GLuint index, const GLdouble *v,
                           GLboolean normalized, GLuint size)
{
   (void) normalized;
   switch (size) {
   case 1:
      _glapi_Dispatch->VertexAttrib1dvNV(index, v);
      return;
   case 2:
      _glapi_Dispatch->VertexAttrib2dvNV(index, v);
      return;
   case 3:
      _glapi_Dispatch->VertexAttrib3dvNV(index, v);
      return;
   case 4:
      _glapi_Dispatch->VertexAttrib4dvNV(index, v);
      return;
   default:
      _mesa_problem(NULL, "Bad size in VertexAttribdv");
   }
}

/*
 * Array [size][type] of VertexAttrib functions
 */
static void (GLAPIENTRY *attribfuncs[8])( GLuint, const void *, GLboolean, GLuint ) = {
   (attrib_func) VertexAttribbv,
   (attrib_func) VertexAttribubv,
   (attrib_func) VertexAttribsv,
   (attrib_func) VertexAttribusv,
   (attrib_func) VertexAttribiv,
   (attrib_func) VertexAttribuiv,
   (attrib_func) VertexAttribfv,
   (attrib_func) VertexAttribdv
};

/**********************************************************************/


GLboolean _ae_create_context( GLcontext *ctx )
{
   if (ctx->aelt_context)
      return GL_TRUE;

   ctx->aelt_context = MALLOC( sizeof(AEcontext) );
   if (!ctx->aelt_context)
      return GL_FALSE;

   AE_CONTEXT(ctx)->NewState = ~0;
   return GL_TRUE;
}


void _ae_destroy_context( GLcontext *ctx )
{
   if ( AE_CONTEXT( ctx ) ) {
      FREE( ctx->aelt_context );
      ctx->aelt_context = 0;
   }
}


/*
 * Return pointer to the conventional vertex array which corresponds
 * to the given vertex attribute index.
 */
static struct gl_client_array *
conventional_array(GLcontext *ctx, GLuint index)
{
   ASSERT(index < VERT_ATTRIB_MAX);
   switch (index) {
   case VERT_ATTRIB_POS:
      return &ctx->Array.Vertex;
   case VERT_ATTRIB_NORMAL:
      return &ctx->Array.Normal;
   case VERT_ATTRIB_COLOR0:
      return &ctx->Array.Color;
   case VERT_ATTRIB_COLOR1:
      return &ctx->Array.SecondaryColor;
   case VERT_ATTRIB_FOG:
      return &ctx->Array.FogCoord;
   case VERT_ATTRIB_TEX0:
   case VERT_ATTRIB_TEX1:
   case VERT_ATTRIB_TEX2:
   case VERT_ATTRIB_TEX3:
   case VERT_ATTRIB_TEX4:
   case VERT_ATTRIB_TEX5:
   case VERT_ATTRIB_TEX6:
   case VERT_ATTRIB_TEX7:
      return &ctx->Array.TexCoord[index - VERT_ATTRIB_TEX0];
   default:
      return NULL;
   }
}


/**
 * Make a list of functions to call per glArrayElement call which will
 * access the vertex array data.
 * Most vertex attributes are handled via glVertexAttrib4fvNV.
 */
static void _ae_update_state( GLcontext *ctx )
{
   AEcontext *actx = AE_CONTEXT(ctx);
   AEarray *aa = actx->arrays;
   AEattrib *at = actx->attribs;
   GLuint i;

   /* yuck, no generic array to correspond to color index or edge flag */
   if (ctx->Array.Index.Enabled) {
      aa->array = &ctx->Array.Index;
      aa->func = indexfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }
   if (ctx->Array.EdgeFlag.Enabled) {
      aa->array = &ctx->Array.EdgeFlag;
      aa->func = (array_func) glEdgeFlagv;
      aa++;
   }
   aa->func = NULL; /* terminate the list */

   /* all other arrays handled here */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      /* Note: we count down to zero so glVertex (attrib 0) is last!!! */
      const GLuint index = VERT_ATTRIB_MAX - i - 1;
      struct gl_client_array *array = conventional_array(ctx, index);

      /* check for overriding generic vertex attribute */
      if (ctx->VertexProgram.Enabled
          && ctx->Array.VertexAttrib[index].Enabled) {
         array = &ctx->Array.VertexAttrib[index];
      }

      /* if array's enabled, add it to the list */
      if (array && array->Enabled) {
         at->array = array;
         at->func = attribfuncs[TYPE_IDX(array->Type)];
         at->index = index;
         at++;
      }
   }
   ASSERT(at - actx->attribs <= VERT_ATTRIB_MAX);
   at->func = NULL;  /* terminate the list */

   actx->NewState = 0;
}


void GLAPIENTRY _ae_loopback_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   const AEcontext *actx = AE_CONTEXT(ctx);
   const AEarray *aa;
   const AEattrib *at;

   if (actx->NewState)
      _ae_update_state( ctx );

   /* color index and edge flags */
   for (aa = actx->arrays; aa->func ; aa++) {
      const GLubyte *src = aa->array->BufferObj->Data
                         + (GLuint) aa->array->Ptr
                         + elt * aa->array->StrideB;
      aa->func( src );
   }

   /* all other attributes */
   for (at = actx->attribs; at->func; at++) {
      const GLubyte *src = at->array->BufferObj->Data
                         + (GLuint) at->array->Ptr
                         + elt * at->array->StrideB;
      at->func( at->index, src, at->array->Normalized, at->array->Size );
   }
}



void _ae_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   AE_CONTEXT(ctx)->NewState |= new_state;
}
