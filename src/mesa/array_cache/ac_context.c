/* $Id: ac_context.c,v 1.4 2001/04/28 08:39:18 keithw Exp $ */

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
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"

#include "array_cache/ac_context.h"

static void _ac_fallbacks_init( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *cl;
   GLuint i;

   cl = &ac->Fallback.Normal;
   cl->Size = 3;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) ctx->Current.Normal;
   cl->Enabled = 1;
   cl->Flags = CA_CLIENT_DATA;	/* hack */

   cl = &ac->Fallback.Color;
   cl->Size = 4;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) ctx->Current.Color;
   cl->Enabled = 1;
   cl->Flags = CA_CLIENT_DATA;	/* hack */

   cl = &ac->Fallback.SecondaryColor;
   cl->Size = 3;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) ctx->Current.SecondaryColor;
   cl->Enabled = 1;
   cl->Flags = CA_CLIENT_DATA;	/* hack */

   cl = &ac->Fallback.FogCoord;
   cl->Size = 1;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) &ctx->Current.FogCoord;
   cl->Enabled = 1;
   cl->Flags = CA_CLIENT_DATA;	/* hack */

   cl = &ac->Fallback.Index;
   cl->Size = 1;
   cl->Type = GL_UNSIGNED_INT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) &ctx->Current.Index;
   cl->Enabled = 1;
   cl->Flags = CA_CLIENT_DATA;	/* hack */

   for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++) {
      cl = &ac->Fallback.TexCoord[i];
      cl->Size = 4;
      cl->Type = GL_FLOAT;
      cl->Stride = 0;
      cl->StrideB = 0;
      cl->Ptr = (void *) ctx->Current.Texcoord[i];
      cl->Enabled = 1;
      cl->Flags = CA_CLIENT_DATA;	/* hack */
   }

   cl = &ac->Fallback.EdgeFlag;
   cl->Size = 1;
   cl->Type = GL_UNSIGNED_BYTE;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) &ctx->Current.EdgeFlag;
   cl->Enabled = 1;
   cl->Flags = CA_CLIENT_DATA;	/* hack */
}


static void _ac_cache_init( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *cl;
   GLuint size = ctx->Const.MaxArrayLockSize + MAX_CLIPPED_VERTICES;
   GLuint i;

   cl = &ac->Cache.Vertex;
   cl->Size = 4;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 4 * sizeof(GLfloat);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;

   cl = &ac->Cache.Normal;
   cl->Size = 3;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 3 * sizeof(GLfloat);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;

   cl = &ac->Cache.Color;
   cl->Size = 4;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 4 * sizeof(GLfloat);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;

   cl = &ac->Cache.SecondaryColor;
   cl->Size = 3;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 4 * sizeof(GLfloat);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;

   cl = &ac->Cache.FogCoord;
   cl->Size = 1;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = sizeof(GLfloat);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;

   cl = &ac->Cache.Index;
   cl->Size = 1;
   cl->Type = GL_UNSIGNED_INT;
   cl->Stride = 0;
   cl->StrideB = sizeof(GLuint);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;

   for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++) {
      cl = &ac->Cache.TexCoord[i];
      cl->Size = 4;
      cl->Type = GL_FLOAT;
      cl->Stride = 0;
      cl->StrideB = 4 * sizeof(GLfloat);
      cl->Ptr = MALLOC( cl->StrideB * size );
      cl->Enabled = 1;
      cl->Flags = 0;
   }

   cl = &ac->Cache.EdgeFlag;
   cl->Size = 1;
   cl->Type = GL_UNSIGNED_BYTE;
   cl->Stride = 0;
   cl->StrideB = sizeof(GLubyte);
   cl->Ptr = MALLOC( cl->StrideB * size );
   cl->Enabled = 1;
   cl->Flags = 0;
}


/* This storage used to hold translated client data if type or stride
 * need to be fixed.
 */
static void _ac_elts_init( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   GLuint size = 1000;

   ac->Elts = (GLuint *)MALLOC( sizeof(GLuint) * size );
   ac->elt_size = size;
}

static void _ac_raw_init( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   GLuint i;

   ac->Raw.Color = ac->Fallback.Color;
   ac->Raw.EdgeFlag = ac->Fallback.EdgeFlag;
   ac->Raw.FogCoord = ac->Fallback.FogCoord;
   ac->Raw.Index = ac->Fallback.Index;
   ac->Raw.Normal = ac->Fallback.Normal;
   ac->Raw.SecondaryColor = ac->Fallback.SecondaryColor;
   ac->Raw.Vertex = ctx->Array.Vertex;

   ac->IsCached.Color = GL_FALSE;
   ac->IsCached.EdgeFlag = GL_FALSE;
   ac->IsCached.FogCoord = GL_FALSE;
   ac->IsCached.Index = GL_FALSE;
   ac->IsCached.Normal = GL_FALSE;
   ac->IsCached.SecondaryColor = GL_FALSE;
   ac->IsCached.Vertex = GL_FALSE;

   for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++) {
      ac->Raw.TexCoord[i] = ac->Fallback.TexCoord[i];
      ac->IsCached.TexCoord[i] = GL_FALSE;
   }

}

GLboolean _ac_CreateContext( GLcontext *ctx )
{
   ctx->acache_context = CALLOC(sizeof(ACcontext));
   if (ctx->acache_context) {
      _ac_cache_init( ctx );
      _ac_fallbacks_init( ctx );
      _ac_raw_init( ctx );
      _ac_elts_init( ctx );
      return GL_TRUE;
   }
   return GL_FALSE;
}

void _ac_DestroyContext( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   GLint i;

   if (ac->Cache.Vertex.Ptr) FREE( ac->Cache.Vertex.Ptr );
   if (ac->Cache.Normal.Ptr) FREE( ac->Cache.Normal.Ptr );
   if (ac->Cache.Color.Ptr) FREE( ac->Cache.Color.Ptr );
   if (ac->Cache.SecondaryColor.Ptr) FREE( ac->Cache.SecondaryColor.Ptr );
   if (ac->Cache.EdgeFlag.Ptr) FREE( ac->Cache.EdgeFlag.Ptr );
   if (ac->Cache.Index.Ptr) FREE( ac->Cache.Index.Ptr );
   if (ac->Cache.FogCoord.Ptr) FREE( ac->Cache.FogCoord.Ptr );

   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      if (ac->Cache.TexCoord[i].Ptr)
	 FREE( ac->Cache.TexCoord[i].Ptr );
   }

   if (ac->Elts) FREE( ac->Elts );
}

void _ac_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   AC_CONTEXT(ctx)->NewState |= new_state;
   AC_CONTEXT(ctx)->NewArrayState |= ctx->Array.NewState;
}
