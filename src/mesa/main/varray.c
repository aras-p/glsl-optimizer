/* $Id: varray.c,v 1.36 2001/01/05 05:31:42 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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

#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "dlist.h"
#include "light.h"
#include "macros.h"
#include "mmath.h"
#include "state.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#include "math/m_translate.h"
#endif



void
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (size<2 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glVertexPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glVertexPointer(stride)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_VARRAY|VERBOSE_API))
      fprintf(stderr, "glVertexPointer( sz %d type %s stride %d )\n", size,
	      gl_lookup_enum_by_nr( type ),
	      stride);

   ctx->Array.Vertex.StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_SHORT:
         ctx->Array.Vertex.StrideB =  size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.Vertex.StrideB =  size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.Vertex.StrideB =  size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.Vertex.StrideB =  size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glVertexPointer(type)" );
         return;
      }
   }
   ctx->Array.Vertex.Size = size;
   ctx->Array.Vertex.Type = type;
   ctx->Array.Vertex.Stride = stride;
   ctx->Array.Vertex.Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.VertexPointer)
      ctx->Driver.VertexPointer( ctx, size, type, stride, ptr );
}




void
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glNormalPointer(stride)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_VARRAY|VERBOSE_API))
      fprintf(stderr, "glNormalPointer( type %s stride %d )\n",
	      gl_lookup_enum_by_nr( type ),
	      stride);

   ctx->Array.Normal.StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_BYTE:
         ctx->Array.Normal.StrideB =  3*sizeof(GLbyte);
         break;
      case GL_SHORT:
         ctx->Array.Normal.StrideB =  3*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.Normal.StrideB =  3*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.Normal.StrideB =  3*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.Normal.StrideB =  3*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glNormalPointer(type)" );
         return;
      }
   }
   ctx->Array.Normal.Type = type;
   ctx->Array.Normal.Stride = stride;
   ctx->Array.Normal.Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.NormalPointer)
      ctx->Driver.NormalPointer( ctx, type, stride, ptr );
}



void
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (size<3 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(stride)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_VARRAY|VERBOSE_API))
      fprintf(stderr, "glColorPointer( sz %d type %s stride %d )\n", size,
	  gl_lookup_enum_by_nr( type ),
	  stride);

   ctx->Array.Color.StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_BYTE:
         ctx->Array.Color.StrideB =  size*sizeof(GLbyte);
         break;
      case GL_UNSIGNED_BYTE:
         ctx->Array.Color.StrideB =  size*sizeof(GLubyte);
         break;
      case GL_SHORT:
         ctx->Array.Color.StrideB =  size*sizeof(GLshort);
         break;
      case GL_UNSIGNED_SHORT:
         ctx->Array.Color.StrideB =  size*sizeof(GLushort);
         break;
      case GL_INT:
         ctx->Array.Color.StrideB =  size*sizeof(GLint);
         break;
      case GL_UNSIGNED_INT:
         ctx->Array.Color.StrideB =  size*sizeof(GLuint);
         break;
      case GL_FLOAT:
         ctx->Array.Color.StrideB =  size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.Color.StrideB =  size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorPointer(type)" );
         return;
      }
   }
   ctx->Array.Color.Size = size;
   ctx->Array.Color.Type = type;
   ctx->Array.Color.Stride = stride;
   ctx->Array.Color.Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.ColorPointer)
      ctx->Driver.ColorPointer( ctx, size, type, stride, ptr );
}



void
_mesa_FogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glFogCoordPointer(stride)" );
      return;
   }

   ctx->Array.FogCoord.StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_FLOAT:
         ctx->Array.FogCoord.StrideB =  sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.FogCoord.StrideB =  sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glFogCoordPointer(type)" );
         return;
      }
   }
   ctx->Array.FogCoord.Type = type;
   ctx->Array.FogCoord.Stride = stride;
   ctx->Array.FogCoord.Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.FogCoordPointer)
      ctx->Driver.FogCoordPointer( ctx, type, stride, ptr );
}


void
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glIndexPointer(stride)" );
      return;
   }

   ctx->Array.Index.StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_UNSIGNED_BYTE:
         ctx->Array.Index.StrideB =  sizeof(GLubyte);
         break;
      case GL_SHORT:
         ctx->Array.Index.StrideB =  sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.Index.StrideB =  sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.Index.StrideB =  sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.Index.StrideB =  sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glIndexPointer(type)" );
         return;
      }
   }
   ctx->Array.Index.Type = type;
   ctx->Array.Index.Stride = stride;
   ctx->Array.Index.Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.IndexPointer)
      ctx->Driver.IndexPointer( ctx, type, stride, ptr );
}


void
_mesa_SecondaryColorPointerEXT(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (size != 3 && size != 4) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(stride)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_VARRAY|VERBOSE_API))
      fprintf(stderr, "glColorPointer( sz %d type %s stride %d )\n", size,
	  gl_lookup_enum_by_nr( type ),
	  stride);

   ctx->Array.SecondaryColor.StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_BYTE:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLbyte);
         break;
      case GL_UNSIGNED_BYTE:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLubyte);
         break;
      case GL_SHORT:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLshort);
         break;
      case GL_UNSIGNED_SHORT:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLushort);
         break;
      case GL_INT:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLint);
         break;
      case GL_UNSIGNED_INT:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLuint);
         break;
      case GL_FLOAT:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.SecondaryColor.StrideB =  size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glSecondaryColorPointer(type)" );
         return;
      }
   }
   ctx->Array.SecondaryColor.Size = 3; /* hardwire */
   ctx->Array.SecondaryColor.Type = type;
   ctx->Array.SecondaryColor.Stride = stride;
   ctx->Array.SecondaryColor.Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.SecondaryColorPointer)
      ctx->Driver.SecondaryColorPointer( ctx, size, type, stride, ptr );
}



void
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint texUnit = ctx->Array.ActiveTexture;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (size<1 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexCoordPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexCoordPointer(stride)" );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_VARRAY|VERBOSE_API))
      fprintf(stderr, "glTexCoordPointer( unit %u sz %d type %s stride %d )\n",
	  texUnit,
	  size,
	  gl_lookup_enum_by_nr( type ),
	  stride);

   ctx->Array.TexCoord[texUnit].StrideB = stride;
   if (!stride) {
      switch (type) {
      case GL_SHORT:
         ctx->Array.TexCoord[texUnit].StrideB =  size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.TexCoord[texUnit].StrideB =  size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.TexCoord[texUnit].StrideB =  size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.TexCoord[texUnit].StrideB =  size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexCoordPointer(type)" );
         return;
      }
   }
   ctx->Array.TexCoord[texUnit].Size = size;
   ctx->Array.TexCoord[texUnit].Type = type;
   ctx->Array.TexCoord[texUnit].Stride = stride;
   ctx->Array.TexCoord[texUnit].Ptr = (void *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

/*     fprintf(stderr, "%s ptr %p\n", __FUNCTION__, ptr); */

   if (ctx->Driver.TexCoordPointer)
      ctx->Driver.TexCoordPointer( ctx, size, type, stride, ptr );
}




void
_mesa_EdgeFlagPointer(GLsizei stride, const void *vptr)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLboolean *ptr = (GLboolean *)vptr;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glEdgeFlagPointer(stride)" );
      return;
   }
   ctx->Array.EdgeFlag.Stride = stride;
   ctx->Array.EdgeFlag.StrideB = stride ? stride : sizeof(GLboolean);
   ctx->Array.EdgeFlag.Ptr = (GLboolean *) ptr;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_VERTEX;

   if (ctx->Driver.EdgeFlagPointer)
      ctx->Driver.EdgeFlagPointer( ctx, stride, ptr );
}





void
_mesa_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr)
{
   (void) count;
   _mesa_VertexPointer(size, type, stride, ptr);
}


void
_mesa_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr)
{
   (void) count;
   _mesa_NormalPointer(type, stride, ptr);
}


void
_mesa_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   (void) count;
   _mesa_ColorPointer(size, type, stride, ptr);
}


void
_mesa_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   (void) count;
   _mesa_IndexPointer(type, stride, ptr);
}


void
_mesa_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                         GLsizei count, const GLvoid *ptr)
{
   (void) count;
   _mesa_TexCoordPointer(size, type, stride, ptr);
}


void
_mesa_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   (void) count;
   _mesa_EdgeFlagPointer(stride, ptr);
}






void
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean tflag, cflag, nflag;  /* enable/disable flags */
   GLint tcomps, ccomps, vcomps;   /* components per texcoord, color, vertex */

   GLenum ctype = 0;               /* color type */
   GLint coffset = 0, noffset = 0, voffset;/* color, normal, vertex offsets */
   GLint defstride;                /* default stride */
   GLint c, f;
   GLint coordUnitSave;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   f = sizeof(GLfloat);
   c = f * ((4*sizeof(GLubyte) + (f-1)) / f);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glInterleavedArrays(stride)" );
      return;
   }

   switch (format) {
      case GL_V2F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 2;
         voffset = 0;
         defstride = 2*f;
         break;
      case GL_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         voffset = 0;
         defstride = 3*f;
         break;
      case GL_C4UB_V2F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 2;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 2*f;
         break;
      case GL_C4UB_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 3*f;
         break;
      case GL_C3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         noffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_C4F_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         noffset = 4*f;
         voffset = 7*f;
         defstride = 10*f;
         break;
      case GL_T2F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         voffset = 2*f;
         defstride = 5*f;
         break;
      case GL_T4F_V4F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 4;  ccomps = 0;  vcomps = 4;
         voffset = 4*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4UB_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 2*f;
         voffset = c+2*f;
         defstride = c+5*f;
         break;
      case GL_T2F_C3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         noffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         noffset = 6*f;
         voffset = 9*f;
         defstride = 12*f;
         break;
      case GL_T4F_C4F_N3F_V4F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 4;  ccomps = 4;  vcomps = 4;
         ctype = GL_FLOAT;
         coffset = 4*f;
         noffset = 8*f;
         voffset = 11*f;
         defstride = 15*f;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glInterleavedArrays(format)" );
         return;
   }

   if (stride==0) {
      stride = defstride;
   }

   _mesa_DisableClientState( GL_EDGE_FLAG_ARRAY );
   _mesa_DisableClientState( GL_INDEX_ARRAY );

   /* Texcoords */
   coordUnitSave = ctx->Array.ActiveTexture;
   if (tflag) {
      GLint i;
      GLint factor = ctx->Array.TexCoordInterleaveFactor;
      for (i = 0; i < factor; i++) {
         _mesa_ClientActiveTextureARB( (GLenum) (GL_TEXTURE0_ARB + i) );
         _mesa_EnableClientState( GL_TEXTURE_COORD_ARRAY );
         _mesa_TexCoordPointer( tcomps, GL_FLOAT, stride,
				(GLubyte *) pointer + i * coffset );
      }
      for (i = factor; i < ctx->Const.MaxTextureUnits; i++) {
         _mesa_ClientActiveTextureARB( (GLenum) (GL_TEXTURE0_ARB + i) );
         _mesa_DisableClientState( GL_TEXTURE_COORD_ARRAY );
      }
   }
   else {
      GLint i;
      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
         _mesa_ClientActiveTextureARB( (GLenum) (GL_TEXTURE0_ARB + i) );
         _mesa_DisableClientState( GL_TEXTURE_COORD_ARRAY );
      }
   }
   /* Restore texture coordinate unit index */
   _mesa_ClientActiveTextureARB( (GLenum) (GL_TEXTURE0_ARB + coordUnitSave) );


   /* Color */
   if (cflag) {
      _mesa_EnableClientState( GL_COLOR_ARRAY );
      _mesa_ColorPointer( ccomps, ctype, stride,
			  (GLubyte*) pointer + coffset );
   }
   else {
      _mesa_DisableClientState( GL_COLOR_ARRAY );
   }


   /* Normals */
   if (nflag) {
      _mesa_EnableClientState( GL_NORMAL_ARRAY );
      _mesa_NormalPointer( GL_FLOAT, stride,
			   (GLubyte*) pointer + noffset );
   }
   else {
      _mesa_DisableClientState( GL_NORMAL_ARRAY );
   }

   _mesa_EnableClientState( GL_VERTEX_ARRAY );
   _mesa_VertexPointer( vcomps, GL_FLOAT, stride,
			(GLubyte *) pointer + voffset );
}



void
_mesa_LockArraysEXT(GLint first, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glLockArrays %d %d\n", first, count);

   if (first == 0 && count > 0 && count <= ctx->Const.MaxArrayLockSize) {
      ctx->Array.LockFirst = first;
      ctx->Array.LockCount = count;
   } 
   else {
      ctx->Array.LockFirst = 0;
      ctx->Array.LockCount = 0;
   }

   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;

   if (ctx->Driver.LockArraysEXT)
      ctx->Driver.LockArraysEXT( ctx, first, count );
}


void
_mesa_UnlockArraysEXT( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glUnlockArrays\n");

   ctx->Array.LockFirst = 0;
   ctx->Array.LockCount = 0;
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;

   if (ctx->Driver.UnlockArraysEXT)
      ctx->Driver.UnlockArraysEXT( ctx );
}
