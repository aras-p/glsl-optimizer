/* $Id: api_arrayelt.c,v 1.10 2002/10/24 23:57:19 brianp Exp $ */

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

/* Author:
 *    Keith Whitwell <keith_whitwell@yahoo.com>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"


typedef void (*texarray_func)( GLenum, const void * );

typedef struct {
   GLint unit;
   struct gl_client_array *array;
   texarray_func func;
} AEtexarray;

typedef void (*array_func)( const void * );

typedef struct {
   struct gl_client_array *array;
   array_func func;
} AEarray;

typedef struct {
   AEtexarray texarrays[MAX_TEXTURE_UNITS+1];
   AEarray arrays[32];
   GLuint NewState;
} AEcontext;

#define AE_CONTEXT(ctx) ((AEcontext *)(ctx)->aelt_context)
#define TYPE_IDX(t) ((t) & 0xf)

static void (*colorfuncs[2][8])( const void * ) = {
   { (array_func)glColor3bv,
     (array_func)glColor3ubv,
     (array_func)glColor3sv,
     (array_func)glColor3usv,
     (array_func)glColor3iv,
     (array_func)glColor3uiv,
     (array_func)glColor3fv,
     (array_func)glColor3dv },

   { (array_func)glColor4bv,
     (array_func)glColor4ubv,
     (array_func)glColor4sv,
     (array_func)glColor4usv,
     (array_func)glColor4iv,
     (array_func)glColor4uiv,
     (array_func)glColor4fv,
     (array_func)glColor4dv }
};

static void (*vertexfuncs[3][8])( const void * ) = {
   { 0,
     0,
     (array_func)glVertex2sv,
     0,
     (array_func)glVertex2iv,
     0,
     (array_func)glVertex2fv,
     (array_func)glVertex2dv },

   { 0,
     0,
     (array_func)glVertex3sv,
     0,
     (array_func)glVertex3iv,
     0,
     (array_func)glVertex3fv,
     (array_func)glVertex3dv },

   { 0,
     0,
     (array_func)glVertex4sv,
     0,
     (array_func)glVertex4iv,
     0,
     (array_func)glVertex4fv,
     (array_func)glVertex4dv }
};


static void (*multitexfuncs[4][8])( GLenum, const void * ) = {
   { 0,
     0,
     (texarray_func)glMultiTexCoord1svARB,
     0,
     (texarray_func)glMultiTexCoord1ivARB,
     0,
     (texarray_func)glMultiTexCoord1fvARB,
     (texarray_func)glMultiTexCoord1dvARB },

   { 0,
     0,
     (texarray_func)glMultiTexCoord2svARB,
     0,
     (texarray_func)glMultiTexCoord2ivARB,
     0,
     (texarray_func)glMultiTexCoord2fvARB,
     (texarray_func)glMultiTexCoord2dvARB },

   { 0,
     0,
     (texarray_func)glMultiTexCoord3svARB,
     0,
     (texarray_func)glMultiTexCoord3ivARB,
     0,
     (texarray_func)glMultiTexCoord3fvARB,
     (texarray_func)glMultiTexCoord3dvARB },

   { 0,
     0,
     (texarray_func)glMultiTexCoord4svARB,
     0,
     (texarray_func)glMultiTexCoord4ivARB,
     0,
     (texarray_func)glMultiTexCoord4fvARB,
     (texarray_func)glMultiTexCoord4dvARB }
};

static void (*indexfuncs[8])( const void * ) = {
   0,
   (array_func)glIndexubv,
   (array_func)glIndexsv,
   0,
   (array_func)glIndexiv,
   0,
   (array_func)glIndexfv,
   (array_func)glIndexdv
};


static void (*normalfuncs[8])( const void * ) = {
   (array_func)glNormal3bv,
   0,
   (array_func)glNormal3sv,
   0,
   (array_func)glNormal3iv,
   0,
   (array_func)glNormal3fv,
   (array_func)glNormal3dv,
};


/* Wrapper functions in case glSecondaryColor*EXT doesn't exist */
static void SecondaryColor3bvEXT(const GLbyte *c)
{
   _glapi_Dispatch->SecondaryColor3bvEXT(c);
}

static void SecondaryColor3ubvEXT(const GLubyte *c)
{
   _glapi_Dispatch->SecondaryColor3ubvEXT(c);
}

static void SecondaryColor3svEXT(const GLshort *c)
{
   _glapi_Dispatch->SecondaryColor3svEXT(c);
}

static void SecondaryColor3usvEXT(const GLushort *c)
{
   _glapi_Dispatch->SecondaryColor3usvEXT(c);
}

static void SecondaryColor3ivEXT(const GLint *c)
{
   _glapi_Dispatch->SecondaryColor3ivEXT(c);
}

static void SecondaryColor3uivEXT(const GLuint *c)
{
   _glapi_Dispatch->SecondaryColor3uivEXT(c);
}

static void SecondaryColor3fvEXT(const GLfloat *c)
{
   _glapi_Dispatch->SecondaryColor3fvEXT(c);
}

static void SecondaryColor3dvEXT(const GLdouble *c)
{
   _glapi_Dispatch->SecondaryColor3dvEXT(c);
}

static void (*secondarycolorfuncs[8])( const void * ) = {
   (array_func) SecondaryColor3bvEXT,
   (array_func) SecondaryColor3ubvEXT,
   (array_func) SecondaryColor3svEXT,
   (array_func) SecondaryColor3usvEXT,
   (array_func) SecondaryColor3ivEXT,
   (array_func) SecondaryColor3uivEXT,
   (array_func) SecondaryColor3fvEXT,
   (array_func) SecondaryColor3dvEXT,
};


/* Again, wrapper functions in case glSecondaryColor*EXT doesn't exist */
static void FogCoordfvEXT(const GLfloat *f)
{
   _glapi_Dispatch->FogCoordfvEXT(f);
}

static void FogCoorddvEXT(const GLdouble *f)
{
   _glapi_Dispatch->FogCoorddvEXT(f);
}

static void (*fogcoordfuncs[8])( const void * ) = {
   0,
   0,
   0,
   0,
   0,
   0,
   (array_func) FogCoordfvEXT,
   (array_func) FogCoorddvEXT
};



GLboolean _ae_create_context( GLcontext *ctx )
{
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


static void _ae_update_state( GLcontext *ctx )
{
   AEcontext *actx = AE_CONTEXT(ctx);
   AEtexarray *ta = actx->texarrays;
   AEarray *aa = actx->arrays;
   GLuint i;

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
      aa->array = &ctx->Array.EdgeFlag;
      aa->func = (array_func)glEdgeFlagv;
      aa++;
   }

   if (ctx->Array.FogCoord.Enabled) {
      aa->array = &ctx->Array.FogCoord;
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


void _ae_loopback_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   AEcontext *actx = AE_CONTEXT(ctx);
   AEtexarray *ta;
   AEarray *aa;

   if (actx->NewState)
      _ae_update_state( ctx );

   for (ta = actx->texarrays ; ta->func ; ta++) {
      ta->func( ta->unit + GL_TEXTURE0_ARB, (char *)ta->array->Ptr + elt * ta->array->StrideB );
   }

   /* Must be last
    */
   for (aa = actx->arrays ; aa->func ; aa++) {
      aa->func( (char *)aa->array->Ptr + elt * aa->array->StrideB );
   }
}



void _ae_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   AE_CONTEXT(ctx)->NewState |= new_state;
}
