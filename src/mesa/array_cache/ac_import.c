/* $Id: ac_import.c,v 1.1 2000/12/26 15:14:04 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * Author:
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"

#include "array_cache/ac_context.h"



/* Set the array pointer back to its source when the cached data is
 * invalidated:
 */

static void reset_texcoord( GLcontext *ctx, GLuint unit )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array._Enabled & _NEW_ARRAY_TEXCOORD(unit))
      ac->Current.TexCoord[unit] = &ctx->Array.TexCoord[unit];
   else {
      ac->Current.TexCoord[unit] = &ac->Fallback.TexCoord[unit];

      if (ctx->Current.Texcoord[unit][4] != 1.0)
	 ac->Current.TexCoord[unit]->Size = 4;
      else if (ctx->Current.Texcoord[unit][3] != 0.0)
	 ac->Current.TexCoord[unit]->Size = 3;
      else
	 ac->Current.TexCoord[unit]->Size = 2;
   }

   ac->Writeable.TexCoord[unit] = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_TEXCOORD(unit);
}

static void reset_vertex( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   ASSERT(ctx->Array.Vertex.Enabled);
   ac->Current.Vertex = &ctx->Array.Vertex;
   ac->Writeable.Vertex = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_VERTEX;
}


static void reset_normal( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array._Enabled & _NEW_ARRAY_NORMAL) {
/*        fprintf(stderr, "reset normal: using client array enab %d\n", */
/*  	      ctx->Array.Normal.Enabled); */
      ac->Current.Normal = &ctx->Array.Normal;
   }
   else {
/*        fprintf(stderr, "reset normal: using fallback enab %d\n", */
/*  	      ctx->Array.Normal.Enabled); */
      ac->Current.Normal = &ac->Fallback.Normal;
   }

   ac->Writeable.Normal = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_NORMAL;
}


static void reset_color( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);


   if (ctx->Array._Enabled & _NEW_ARRAY_COLOR)
      ac->Current.Color = &ctx->Array.Color;
   else 
      ac->Current.Color = &ac->Fallback.Color;

/*     fprintf(stderr, "reset_color, stride now %d\n", ac->Current.Color->StrideB); */

   ac->Writeable.Color = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_COLOR;
}


static void reset_secondarycolor( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array._Enabled & _NEW_ARRAY_SECONDARYCOLOR)
      ac->Current.SecondaryColor = &ctx->Array.SecondaryColor;
   else 
      ac->Current.SecondaryColor = &ac->Fallback.SecondaryColor;

   ac->Writeable.SecondaryColor = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_SECONDARYCOLOR;
}


static void reset_index( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array._Enabled & _NEW_ARRAY_INDEX)
      ac->Current.Index = &ctx->Array.Index;
   else 
      ac->Current.Index = &ac->Fallback.Index;

   ac->Writeable.Index = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_INDEX;
}

static void reset_fogcoord( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array._Enabled & _NEW_ARRAY_FOGCOORD)
      ac->Current.FogCoord = &ctx->Array.FogCoord;
   else 
      ac->Current.FogCoord = &ac->Fallback.FogCoord;

   ac->Writeable.FogCoord = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_FOGCOORD;
}

static void reset_edgeflag( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array._Enabled & _NEW_ARRAY_EDGEFLAG)
      ac->Current.EdgeFlag = &ctx->Array.EdgeFlag;
   else 
      ac->Current.EdgeFlag = &ac->Fallback.EdgeFlag;

   ac->Writeable.EdgeFlag = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_EDGEFLAG;
}


/* Functions to import array ranges with specified types and strides.
 */
static void import_texcoord( GLcontext *ctx, GLuint unit,
			     GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.TexCoord[unit];
   struct gl_client_array *to = &ac->Cache.TexCoord[unit];

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 4*sizeof(GLfloat) || stride == 0);
   ASSERT(ac->count - ac->start < ctx->Const.MaxArrayLockSize);

   _math_trans_4f( to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   from->Size,
		   ac->start, 
		   ac->count);

   to->Size = from->Size;
   to->StrideB = 4 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->Current.TexCoord[unit] = to;
   ac->Writeable.TexCoord[unit] = GL_TRUE;
}

static void import_vertex( GLcontext *ctx,
			   GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.Vertex;
   struct gl_client_array *to = &ac->Cache.Vertex;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 4*sizeof(GLfloat) || stride == 0);

   _math_trans_4f( to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   from->Size,
		   ac->start, 
		   ac->count);

   to->Size = from->Size;
   to->StrideB = 4 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->Current.Vertex = to;
   ac->Writeable.Vertex = GL_TRUE;
}

static void import_normal( GLcontext *ctx,
			   GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.Normal;
   struct gl_client_array *to = &ac->Cache.Normal;


/*     fprintf(stderr, "ac: import_normal\n"); */

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 3*sizeof(GLfloat) || stride == 0);

   _math_trans_3f( to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   ac->start, 
		   ac->count);

   to->StrideB = 3 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->Current.Normal = to;
   ac->Writeable.Normal = GL_TRUE;
}

static void import_color( GLcontext *ctx,
			  GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.Color;
   struct gl_client_array *to = &ac->Cache.Color;

/*     fprintf(stderr, "(ac) %s\n", __FUNCTION__); */

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_UNSIGNED_BYTE);
   ASSERT(stride == 4*sizeof(GLubyte) || stride == 0);

   _math_trans_4ub( to->Ptr,
		    from->Ptr,
		    from->StrideB,
		    from->Type,
		    from->Size,
		    ac->start, 
		    ac->count);

   to->Size = from->Size;
   to->StrideB = 4 * sizeof(GLubyte);
   to->Type = GL_FLOAT;
   ac->Current.Color = to;
   ac->Writeable.Color = GL_TRUE;
}

static void import_index( GLcontext *ctx,
			  GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.Index;
   struct gl_client_array *to = &ac->Cache.Index;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_UNSIGNED_INT);
   ASSERT(stride == sizeof(GLuint) || stride == 0);

   _math_trans_1ui( to->Ptr,
		    from->Ptr,
		    from->StrideB,
		    from->Type,
		    ac->start, 
		    ac->count);

   to->StrideB = sizeof(GLuint);
   to->Type = GL_UNSIGNED_INT;
   ac->Current.Index = to;
   ac->Writeable.Index = GL_TRUE;
}

static void import_secondarycolor( GLcontext *ctx,
				   GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.SecondaryColor;
   struct gl_client_array *to = &ac->Cache.SecondaryColor;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_UNSIGNED_BYTE);
   ASSERT(stride == 4*sizeof(GLubyte) || stride == 0);

   _math_trans_4ub( to->Ptr,
		    from->Ptr,
		    from->StrideB,
		    from->Type,
		    from->Size,
		    ac->start, 
		    ac->count);

   to->StrideB = 4 * sizeof(GLubyte);
   to->Type = GL_UNSIGNED_BYTE;
   ac->Current.SecondaryColor = to;
   ac->Writeable.SecondaryColor = GL_TRUE;
}

static void import_fogcoord( GLcontext *ctx,
			     GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.FogCoord;
   struct gl_client_array *to = &ac->Cache.FogCoord;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == sizeof(GLfloat) || stride == 0);

   _math_trans_1f( to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   ac->start, 
		   ac->count);
   
   to->StrideB = sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->Current.FogCoord = to;
   ac->Writeable.FogCoord = GL_TRUE;
}

static void import_edgeflag( GLcontext *ctx,
			     GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   struct gl_client_array *from = ac->Current.EdgeFlag;
   struct gl_client_array *to = &ac->Cache.EdgeFlag;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == sizeof(GLfloat) || stride == 0);

   _math_trans_1f( to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   ac->start, 
		   ac->count);
   
   to->StrideB = sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->Current.EdgeFlag = to;
   ac->Writeable.EdgeFlag = GL_TRUE;
}



/* Externals to request arrays with specific properties:
 */
struct gl_client_array *_ac_import_texcoord( GLcontext *ctx, 
					     GLuint unit,
					     GLenum type,
					     GLuint reqstride, 
					     GLuint reqsize,
					     GLboolean reqwriteable,
					     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_TEXCOORD(unit))
      reset_texcoord( ctx, unit );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Current.TexCoord[unit]->Size > reqsize)
      return 0;

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.TexCoord[unit]->Type != type ||
       (reqstride != 0 && ac->Current.TexCoord[unit]->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.TexCoord[unit]))
      import_texcoord(ctx, unit, type, reqstride );
	 
   *writeable = ac->Writeable.TexCoord[unit];
   return ac->Current.TexCoord[unit];
}

struct gl_client_array *_ac_import_vertex( GLcontext *ctx, 
					   GLenum type,
					   GLuint reqstride, 
					   GLuint reqsize,
					   GLboolean reqwriteable,
					   GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_VERTEX)
      reset_vertex( ctx );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Current.Vertex->Size > reqsize)
      return 0;

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.Vertex->Type != type ||
       (reqstride != 0 && ac->Current.Vertex->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.Vertex))
      import_vertex(ctx, type, reqstride );
	 
   *writeable = ac->Writeable.Vertex;
   return ac->Current.Vertex;
}

struct gl_client_array *_ac_import_normal( GLcontext *ctx, 
					     GLenum type,
					     GLuint reqstride, 
					     GLboolean reqwriteable,
					     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

/*     fprintf(stderr, "%s %d\n", __FUNCTION__, ac->NewArrayState & _NEW_ARRAY_NORMAL); */

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_NORMAL) 
      reset_normal( ctx );

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.Normal->Type != type ||
       (reqstride != 0 && ac->Current.Normal->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.Normal))
      import_normal(ctx, type, reqstride );
	 
   *writeable = ac->Writeable.Normal;
   return ac->Current.Normal;
}

struct gl_client_array *_ac_import_color( GLcontext *ctx, 
					  GLenum type,
					  GLuint reqstride, 
					  GLuint reqsize,
					  GLboolean reqwriteable,
					  GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

/*     fprintf(stderr, "%s stride %d sz %d wr %d\n", __FUNCTION__, */
/*  	   reqstride, reqsize, reqwriteable); */

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_COLOR) 
      reset_color( ctx );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Current.Color->Size > reqsize) {
/*        fprintf(stderr, "%s -- failed\n", __FUNCTION__); */
      return 0;
   }

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.Color->Type != type ||
       (reqstride != 0 && ac->Current.Color->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.Color))
      import_color(ctx, type, reqstride );
	 
   *writeable = ac->Writeable.Color;
   return ac->Current.Color;
}

struct gl_client_array *_ac_import_index( GLcontext *ctx, 
					  GLenum type,
					  GLuint reqstride, 
					  GLboolean reqwriteable,
					  GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_INDEX) 
      reset_index( ctx );


   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.Index->Type != type ||
       (reqstride != 0 && ac->Current.Index->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.Index))
      import_index(ctx, type, reqstride );
	 
   *writeable = ac->Writeable.Index;
   return ac->Current.Index;
}

struct gl_client_array *_ac_import_secondarycolor( GLcontext *ctx, 
						   GLenum type,
						   GLuint reqstride, 
						   GLuint reqsize,
						   GLboolean reqwriteable,
						   GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_SECONDARYCOLOR) 
      reset_secondarycolor( ctx );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Current.SecondaryColor->Size > reqsize)
      return 0;

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.SecondaryColor->Type != type ||
       (reqstride != 0 && ac->Current.SecondaryColor->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.SecondaryColor))
      import_secondarycolor( ctx, type, reqstride );
	 
   *writeable = ac->Writeable.SecondaryColor;
   return ac->Current.SecondaryColor;
}

struct gl_client_array *_ac_import_fogcoord( GLcontext *ctx, 
					     GLenum type,
					     GLuint reqstride, 
					     GLboolean reqwriteable,
					     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_FOGCOORD) 
      reset_fogcoord( ctx );

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.FogCoord->Type != type ||
       (reqstride != 0 && ac->Current.FogCoord->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.FogCoord))
      import_fogcoord(ctx, type, reqstride );
	 
   *writeable = ac->Writeable.FogCoord;
   return ac->Current.FogCoord;
}




struct gl_client_array *_ac_import_edgeflag( GLcontext *ctx, 
					     GLenum type,
					     GLuint reqstride, 
					     GLboolean reqwriteable,
					     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_EDGEFLAG) 
      reset_edgeflag( ctx );

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Current.EdgeFlag->Type != type ||
       (reqstride != 0 && ac->Current.EdgeFlag->StrideB != reqstride) ||
       (reqwriteable && !ac->Writeable.EdgeFlag))
      import_edgeflag(ctx, type, reqstride );
	 
   *writeable = ac->Writeable.EdgeFlag;
   return ac->Current.EdgeFlag;
}





/* Clients must call this function to validate state and set bounds
 * before importing any data: 
 */
void _ac_import_range( GLcontext *ctx, GLuint start, GLuint count )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Discard cached data which has been invalidated by state changes
    * since last time.  **ALREADY DONE**
   if (ac->NewState)
      _ac_update_state( ctx );
    */

   if (!ctx->Array.LockCount) {
      /* Not locked, discard cached data.  Changes to lock
       * status are caught via. _ac_invalidate_state().
       */
      ac->NewArrayState = _NEW_ARRAY_ALL;
      ac->start = start;
      ac->count = count;
   }
   else {
      /* Locked, discard data for any disabled arrays.  Require that
       * the whole locked range always be dealt with, otherwise hard to
       * maintain cached data in the face of clipping.
       */
      ac->NewArrayState |= ~ctx->Array._Enabled; 
      ac->start = ctx->Array.LockFirst;
      ac->count = ctx->Array.LockCount;
      ASSERT(ac->start == start); /* hmm? */
      ASSERT(ac->count == count);
   }
}



/* Additional convienence function for importing a the element list
 * for drawelements, drawrangeelements:
 */
CONST void *
_ac_import_elements( GLcontext *ctx,
		     GLenum new_type,
		     GLuint count,
		     GLenum old_type,
		     CONST void *indices )
{
   ACcontext *ac = AC_CONTEXT(ctx);
      
   if (old_type == new_type)
      return indices;

   if (ac->elt_size < count * sizeof(GLuint)) {
      if (ac->Elts) FREE(ac->Elts);
      while (ac->elt_size < count * sizeof(GLuint)) 
	 ac->elt_size *= 2;
      ac->Elts = MALLOC(ac->elt_size);
   }

   switch (new_type) {
   case GL_UNSIGNED_BYTE:
      ASSERT(0);
      return 0;
   case GL_UNSIGNED_SHORT:
      ASSERT(0);
      return 0;
   case GL_UNSIGNED_INT: {
      GLuint *out = (GLuint *)ac->Elts;
      GLuint i;
      
      switch (old_type) {
      case GL_UNSIGNED_BYTE: {
	 CONST GLubyte *in = (CONST GLubyte *)indices;
	 for (i = 0 ; i < count ; i++)
	    out[i] = in[i];
	 break;
      }
      case GL_UNSIGNED_SHORT: {
	 CONST GLushort *in = (CONST GLushort *)indices;
	 for (i = 0 ; i < count ; i++)
	       out[i] = in[i];
	 break;
      }
      default:
	 ASSERT(0);
      }

      return (CONST void *)out;
   }
   default:
      ASSERT(0);
      break;
   }

   return 0;
}

