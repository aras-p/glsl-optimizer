/* $Id: api_arrayelt.c,v 1.3 2001/06/01 22:22:10 keithw Exp $ */

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
#include "api_arrayelt.h"
#include "context.h"
#include "mem.h"
#include "macros.h"
#include "mtypes.h"


typedef struct {
   GLint unit;
   struct gl_client_array *array;
   void (*func)( GLenum, const void * );
} AEtexarray;


typedef struct {
   struct gl_client_array *array;
   void (*func)( const void * );
} AEarray;

typedef struct {
   AEtexarray texarrays[MAX_TEXTURE_UNITS+1];
   AEarray arrays[32];
   GLuint NewState;
} AEcontext;

#define AE_CONTEXT(ctx) ((AEcontext *)(ctx)->aelt_context)
#define TYPE_IDX(t) ((t) & 0xf)

static void (*colorfuncs[2][8])( const void * ) = {
   { (void (*)( const void * ))glColor3bv,
     (void (*)( const void * ))glColor3ub,
     (void (*)( const void * ))glColor3sv,
     (void (*)( const void * ))glColor3usv,
     (void (*)( const void * ))glColor3iv,
     (void (*)( const void * ))glColor3uiv,
     (void (*)( const void * ))glColor3fv,
     (void (*)( const void * ))glColor3dv },

   { (void (*)( const void * ))glColor4bv,
     (void (*)( const void * ))glColor4ub,
     (void (*)( const void * ))glColor4sv,
     (void (*)( const void * ))glColor4usv,
     (void (*)( const void * ))glColor4iv,
     (void (*)( const void * ))glColor4uiv,
     (void (*)( const void * ))glColor4fv,
     (void (*)( const void * ))glColor4dv }
};

static void (*vertexfuncs[3][8])( const void * ) = {
   { 0,
     0,
     (void (*)( const void * ))glVertex2sv,
     0,
     (void (*)( const void * ))glVertex2iv,
     0,
     (void (*)( const void * ))glVertex2fv,
     (void (*)( const void * ))glVertex2dv },

   { 0,
     0,
     (void (*)( const void * ))glVertex3sv,
     0,
     (void (*)( const void * ))glVertex3iv,
     0,
     (void (*)( const void * ))glVertex3fv,
     (void (*)( const void * ))glVertex3dv },

   { 0,
     0,
     (void (*)( const void * ))glVertex4sv,
     0,
     (void (*)( const void * ))glVertex4iv,
     0,
     (void (*)( const void * ))glVertex4fv,
     (void (*)( const void * ))glVertex4dv }
};


static void (*multitexfuncs[4][8])( GLenum, const void * ) = {
   { 0,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord1svARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord1ivARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord1fvARB,
     (void (*)( GLenum, const void * ))glMultiTexCoord1dvARB },

   { 0,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord2svARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord2ivARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord2fvARB,
     (void (*)( GLenum, const void * ))glMultiTexCoord2dvARB },

   { 0,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord3svARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord3ivARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord3fvARB,
     (void (*)( GLenum, const void * ))glMultiTexCoord3dvARB },

   { 0,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord4svARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord4ivARB,
     0,
     (void (*)( GLenum, const void * ))glMultiTexCoord4fvARB,
     (void (*)( GLenum, const void * ))glMultiTexCoord4dvARB }
};

static void (*indexfuncs[8])( const void * ) = {
   0,
   (void (*)( const void * ))glIndexubv,
   (void (*)( const void * ))glIndexsv,
   0,
   (void (*)( const void * ))glIndexiv,
   0,
   (void (*)( const void * ))glIndexfv,
   (void (*)( const void * ))glIndexdv
};


static void (*normalfuncs[8])( const void * ) = {
   (void (*)( const void * ))glNormal3bv,
   0,
   (void (*)( const void * ))glNormal3sv,
   0,
   (void (*)( const void * ))glNormal3iv,
   0,
   (void (*)( const void * ))glNormal3fv,
   (void (*)( const void * ))glNormal3dv,
};

static void (*fogcoordfuncs[8])( const void * ) = {
   0,
   0,
   0,
   0,
   0,
   0,
   (void (*)( const void * ))glFogCoordfvEXT,
   (void (*)( const void * ))glFogCoorddvEXT,
};

static void (*secondarycolorfuncs[8])( const void * ) = {
   (void (*)( const void * ))glSecondaryColor3bvEXT,
   (void (*)( const void * ))glSecondaryColor3ubvEXT,
   (void (*)( const void * ))glSecondaryColor3svEXT,
   (void (*)( const void * ))glSecondaryColor3usvEXT,
   (void (*)( const void * ))glSecondaryColor3ivEXT,
   (void (*)( const void * ))glSecondaryColor3uivEXT,
   (void (*)( const void * ))glSecondaryColor3fvEXT,
   (void (*)( const void * ))glSecondaryColor3dvEXT,
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
      aa->array = &ctx->Array.EdgeFlag;
      aa->func = (void (*)( const void * ))glEdgeFlagv;
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
      ta->func( ta->unit, (char *)ta->array->Ptr + elt * ta->array->StrideB );
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
