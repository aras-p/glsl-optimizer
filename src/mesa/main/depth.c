/* $Id: depth.c,v 1.19 2000/09/26 20:53:53 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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
#include "enums.h"
#include "depth.h"
#include "mem.h"
#include "pb.h"
#include "types.h"
#endif



/**********************************************************************/
/*****                          API Functions                     *****/
/**********************************************************************/



void
_mesa_ClearDepth( GLclampd depth )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClearDepth");
   ctx->Depth.Clear = (GLfloat) CLAMP( depth, 0.0, 1.0 );
   if (ctx->Driver.ClearDepth)
      (*ctx->Driver.ClearDepth)( ctx, ctx->Depth.Clear );
}



void
_mesa_DepthFunc( GLenum func )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDepthFunc");

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glDepthFunc %s\n", gl_lookup_enum_by_nr(func));

   switch (func) {
      case GL_LESS:    /* (default) pass if incoming z < stored z */
      case GL_GEQUAL:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_NOTEQUAL:
      case GL_EQUAL:
      case GL_ALWAYS:
	 if (ctx->Depth.Func != func) {
	    ctx->Depth.Func = func;
	    ctx->NewState |= NEW_RASTER_OPS;
	    ctx->TriangleCaps &= ~DD_Z_NEVER;
	    if (ctx->Driver.DepthFunc) {
	       (*ctx->Driver.DepthFunc)( ctx, func );
	    }
	 }
         break;
      case GL_NEVER:
	 if (ctx->Depth.Func != func) {
	    ctx->Depth.Func = func;
	    ctx->NewState |= NEW_RASTER_OPS;
	    ctx->TriangleCaps |= DD_Z_NEVER;
	    if (ctx->Driver.DepthFunc) {
	       (*ctx->Driver.DepthFunc)( ctx, func );
	    }
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDepth.Func" );
   }
}



void
_mesa_DepthMask( GLboolean flag )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDepthMask");

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glDepthMask %d\n", flag);

   /*
    * GL_TRUE indicates depth buffer writing is enabled (default)
    * GL_FALSE indicates depth buffer writing is disabled
    */
   if (ctx->Depth.Mask != flag) {
      ctx->Depth.Mask = flag;
      ctx->NewState |= NEW_RASTER_OPS;
      if (ctx->Driver.DepthMask) {
	 (*ctx->Driver.DepthMask)( ctx, flag );
      }
   }
}



/**********************************************************************/
/*****                           Misc                             *****/
/**********************************************************************/

/*
 * Return address of depth buffer value for given window coord.
 */
GLvoid *
_mesa_zbuffer_address(GLcontext *ctx, GLint x, GLint y)
{
   if (ctx->Visual.DepthBits <= 16)
      return (GLushort *) ctx->DrawBuffer->DepthBuffer + ctx->DrawBuffer->Width * y + x;
   else
      return (GLuint *) ctx->DrawBuffer->DepthBuffer + ctx->DrawBuffer->Width * y + x;
}


#define Z_ADDRESS16( CTX, X, Y )				\
            ( ((GLushort *) (CTX)->DrawBuffer->DepthBuffer)	\
              + (CTX)->DrawBuffer->Width * (Y) + (X) )

#define Z_ADDRESS32( CTX, X, Y )				\
            ( ((GLuint *) (CTX)->DrawBuffer->DepthBuffer)	\
              + (CTX)->DrawBuffer->Width * (Y) + (X) )



/**********************************************************************/
/*****                   Depth Testing Functions                  *****/
/**********************************************************************/


/*
 * Do depth test for an array of fragments.  This is used both for
 * software and hardware Z buffers.
 * Input:  zbuffer - array of z values in the zbuffer
 *         z - array of fragment z values
 * Return:  number of fragments which pass the test.
 */
static GLuint
depth_test_span16( GLcontext *ctx, GLuint n, GLint x, GLint y,
                   GLushort zbuffer[], const GLdepth z[], GLubyte mask[] )
{
   GLuint passed = 0;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  zbuffer[i] = z[i];
		  passed++;
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	    passed = n;
	 }
	 break;
      case GL_NEVER:
         BZERO(mask, n * sizeof(GLubyte));
	 break;
      default:
         gl_problem(ctx, "Bad depth func in depth_test_span16");
   }

   return passed;
}


static GLuint
depth_test_span32( GLcontext *ctx, GLuint n, GLint x, GLint y,
                   GLuint zbuffer[], const GLdepth z[], GLubyte mask[] )
{
   GLuint passed = 0;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     zbuffer[i] = z[i];
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		     passed++;
		  }
		  else {
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
		  zbuffer[i] = z[i];
		  passed++;
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	    passed = n;
	 }
	 break;
      case GL_NEVER:
         BZERO(mask, n * sizeof(GLubyte));
	 break;
      default:
         gl_problem(ctx, "Bad depth func in depth_test_span32");
   }

   return passed;
}



/*
 * Apply depth test to span of fragments.  Hardware or software z buffer.
 */
GLuint
_mesa_depth_test_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLdepth z[], GLubyte mask[] )
{
   if (ctx->Driver.ReadDepthSpan) {
      /* hardware-based depth buffer */
      GLdepth zbuffer[MAX_WIDTH];
      GLuint passed;
      (*ctx->Driver.ReadDepthSpan)(ctx, n, x, y, zbuffer);
      passed = depth_test_span32(ctx, n, x, y, zbuffer, z, mask);
      assert(ctx->Driver.WriteDepthSpan);
      (*ctx->Driver.WriteDepthSpan)(ctx, n, x, y, zbuffer, mask);
      return passed;
   }
   else {
      /* software depth buffer */
      if (ctx->Visual.DepthBits <= 16) {
         GLushort *zptr = (GLushort *) Z_ADDRESS16(ctx, x, y);
         GLuint passed = depth_test_span16(ctx, n, x, y, zptr, z, mask);
         return passed;
      }
      else {
         GLuint *zptr = (GLuint *) Z_ADDRESS32(ctx, x, y);
         GLuint passed = depth_test_span32(ctx, n, x, y, zptr, z, mask);
         return passed;
      }
   }
}




/*
 * Do depth testing for an array of fragments using software Z buffer.
 */
static void
software_depth_test_pixels16( GLcontext *ctx, GLuint n,
                              const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS16(ctx,x[i],y[i]);
		  *zptr = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
         BZERO(mask, n * sizeof(GLubyte));
	 break;
      default:
         gl_problem(ctx, "Bad depth func in software_depth_test_pixels");
   }
}



/*
 * Do depth testing for an array of fragments using software Z buffer.
 */
static void
software_depth_test_pixels32( GLcontext *ctx, GLuint n,
                              const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS32(ctx,x[i],y[i]);
		  *zptr = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
         BZERO(mask, n * sizeof(GLubyte));
	 break;
      default:
         gl_problem(ctx, "Bad depth func in software_depth_test_pixels");
   }
}



/*
 * Do depth testing for an array of pixels using hardware Z buffer.
 * Input/output:  zbuffer - array of depth values from Z buffer
 * Input:  z - array of fragment z values.
 */
static void
hardware_depth_test_pixels( GLcontext *ctx, GLuint n, GLdepth zbuffer[],
                            const GLdepth z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] < zbuffer[i]) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] <= zbuffer[i]) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] >= zbuffer[i]) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] > zbuffer[i]) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] != zbuffer[i]) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		     zbuffer[i] = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  if (z[i] == zbuffer[i]) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zbuffer[i] = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
         BZERO(mask, n * sizeof(GLubyte));
	 break;
      default:
         gl_problem(ctx, "Bad depth func in hardware_depth_test_pixels");
   }
}



void
_mesa_depth_test_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         const GLdepth z[], GLubyte mask[] )
{
   if (ctx->Driver.ReadDepthPixels) {
      /* read depth values from hardware Z buffer */
      GLdepth zbuffer[PB_SIZE];
      (*ctx->Driver.ReadDepthPixels)(ctx, n, x, y, zbuffer);

      hardware_depth_test_pixels( ctx, n, zbuffer, z, mask );

      /* update hardware Z buffer with new values */
      assert(ctx->Driver.WriteDepthPixels);
      (*ctx->Driver.WriteDepthPixels)(ctx, n, x, y, zbuffer, mask );
   }
   else {
      /* software depth testing */
      if (ctx->Visual.DepthBits <= 16)
         software_depth_test_pixels16(ctx, n, x, y, z, mask);
      else
         software_depth_test_pixels32(ctx, n, x, y, z, mask);
   }
}





/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/*
 * Read a span of depth values from the depth buffer.
 * This function does clipping before calling the device driver function.
 */
void
_mesa_read_depth_span( GLcontext *ctx,
                       GLint n, GLint x, GLint y, GLdepth depth[] )
{
   if (y < 0 || y >= ctx->DrawBuffer->Height ||
       x + (GLint) n <= 0 || x >= ctx->DrawBuffer->Width) {
      /* span is completely outside framebuffer */
      GLint i;
      for (i = 0; i < n; i++)
         depth[i] = 0;
      return;
   }

   if (x < 0) {
      GLint dx = -x;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[i] = 0;
      x = 0;
      n -= dx;
      depth += dx;
   }
   if (x + n > ctx->DrawBuffer->Width) {
      GLint dx = x + n - ctx->DrawBuffer->Width;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[n - i - 1] = 0;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if (ctx->DrawBuffer->DepthBuffer) {
      /* read from software depth buffer */
      if (ctx->Visual.DepthBits <= 16) {
         const GLushort *zptr = Z_ADDRESS16( ctx, x, y );
         GLuint i;
         for (i = 0; i < n; i++) {
            depth[i] = zptr[i];
         }
      }
      else {
         const GLuint *zptr = Z_ADDRESS32( ctx, x, y );
         GLuint i;
         for (i = 0; i < n; i++) {
            depth[i] = zptr[i];
         }
      }
   }
   else if (ctx->Driver.ReadDepthSpan) {
      /* read from hardware depth buffer */
      (*ctx->Driver.ReadDepthSpan)( ctx, n, x, y, depth );
   }
   else {
      /* no depth buffer */
      BZERO(depth, n * sizeof(GLfloat));
   }

}




/*
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * This is used for both hardware and software depth buffers.
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void
_mesa_read_depth_span_float( GLcontext *ctx,
                             GLint n, GLint x, GLint y, GLfloat depth[] )
{
   const GLfloat scale = 1.0F / ctx->Visual.DepthMaxF;

   if (y < 0 || y >= ctx->DrawBuffer->Height ||
       x + (GLint) n <= 0 || x >= ctx->DrawBuffer->Width) {
      /* span is completely outside framebuffer */
      GLint i;
      for (i = 0; i < n; i++)
         depth[i] = 0.0F;
      return;
   }

   if (x < 0) {
      GLint dx = -x;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[i] = 0.0F;
      n -= dx;
      x = 0;
   }
   if (x + n > ctx->DrawBuffer->Width) {
      GLint dx = x + n - ctx->DrawBuffer->Width;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[n - i - 1] = 0.0F;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if (ctx->DrawBuffer->DepthBuffer) {
      /* read from software depth buffer */
      if (ctx->Visual.DepthBits <= 16) {
         const GLushort *zptr = Z_ADDRESS16( ctx, x, y );
         GLuint i;
         for (i = 0; i < n; i++) {
            depth[i] = (GLfloat) zptr[i] * scale;
         }
      }
      else {
         const GLuint *zptr = Z_ADDRESS32( ctx, x, y );
         GLuint i;
         for (i = 0; i < n; i++) {
            depth[i] = (GLfloat) zptr[i] * scale;
         }
      }
   }
   else if (ctx->Driver.ReadDepthSpan) {
      /* read from hardware depth buffer */
      GLdepth d[MAX_WIDTH];
      GLuint i;
      assert(n <= MAX_WIDTH);
      (*ctx->Driver.ReadDepthSpan)( ctx, n, x, y, d );
      for (i = 0; i < n; i++) {
	 depth[i] = d[i] * scale;
      }
   }
   else {
      /* no depth buffer */
      BZERO(depth, n * sizeof(GLfloat));
   }
}



/**********************************************************************/
/*****                Allocate and Clear Depth Buffer             *****/
/**********************************************************************/



/*
 * Allocate a new depth buffer.  If there's already a depth buffer allocated
 * it will be free()'d.  The new depth buffer will be uniniitalized.
 * This function is only called through Driver.alloc_depth_buffer.
 */
void
_mesa_alloc_depth_buffer( GLcontext *ctx )
{
   /* deallocate current depth buffer if present */
   if (ctx->DrawBuffer->UseSoftwareDepthBuffer) {
      GLint bytesPerValue;

      if (ctx->DrawBuffer->DepthBuffer) {
         FREE(ctx->DrawBuffer->DepthBuffer);
         ctx->DrawBuffer->DepthBuffer = NULL;
      }

      /* allocate new depth buffer, but don't initialize it */
      if (ctx->Visual.DepthBits <= 16)
         bytesPerValue = sizeof(GLushort);
      else
         bytesPerValue = sizeof(GLuint);

      ctx->DrawBuffer->DepthBuffer = MALLOC( ctx->DrawBuffer->Width
                                             * ctx->DrawBuffer->Height
                                             * bytesPerValue );

      if (!ctx->DrawBuffer->DepthBuffer) {
         /* out of memory */
         ctx->Depth.Test = GL_FALSE;
         ctx->NewState |= NEW_RASTER_OPS;
         gl_error( ctx, GL_OUT_OF_MEMORY, "Couldn't allocate depth buffer" );
      }
   }
}




/*
 * Clear the depth buffer.  If the depth buffer doesn't exist yet we'll
 * allocate it now.
 * This function is only called through Driver.clear_depth_buffer.
 */
void
_mesa_clear_depth_buffer( GLcontext *ctx )
{
   if (ctx->Visual.DepthBits == 0
       || !ctx->DrawBuffer->DepthBuffer
       || !ctx->Depth.Mask) {
      /* no depth buffer, or writing to it is disabled */
      return;
   }

   /* The loops in this function have been written so the IRIX 5.3
    * C compiler can unroll them.  Hopefully other compilers can too!
    */

   if (ctx->Scissor.Enabled) {
      /* only clear scissor region */
      if (ctx->Visual.DepthBits <= 16) {
         const GLushort clearValue = (GLushort) (ctx->Depth.Clear * ctx->Visual.DepthMax);
         const GLint rows = ctx->DrawBuffer->Ymax - ctx->DrawBuffer->Ymin;
         const GLint width = ctx->DrawBuffer->Width;
         GLushort *dRow = (GLushort *) ctx->DrawBuffer->DepthBuffer
            + ctx->DrawBuffer->Ymin * width + ctx->DrawBuffer->Xmin;
         GLint i, j;
         for (i = 0; i < rows; i++) {
            for (j = 0; j < width; j++) {
               dRow[j] = clearValue;
            }
            dRow += width;
         }
      }
      else {
         const GLuint clearValue = (GLuint) (ctx->Depth.Clear * ctx->Visual.DepthMax);
         const GLint rows = ctx->DrawBuffer->Ymax - ctx->DrawBuffer->Ymin;
         const GLint width = ctx->DrawBuffer->Width;
         GLuint *dRow = (GLuint *) ctx->DrawBuffer->DepthBuffer
            + ctx->DrawBuffer->Ymin * width + ctx->DrawBuffer->Xmin;
         GLint i, j;
         for (i = 0; i < rows; i++) {
            for (j = 0; j < width; j++) {
               dRow[j] = clearValue;
            }
            dRow += width;
         }
      }
   }
   else {
      /* clear whole buffer */
      if (ctx->Visual.DepthBits <= 16) {
         const GLushort clearValue = (GLushort) (ctx->Depth.Clear * ctx->Visual.DepthMax);
         if ((clearValue & 0xff) == (clearValue >> 8)) {
            if (clearValue == 0) {
               BZERO(ctx->DrawBuffer->DepthBuffer,
                     2*ctx->DrawBuffer->Width*ctx->DrawBuffer->Height);
            }
            else {
               /* lower and upper bytes of clear_value are same, use MEMSET */
               MEMSET( ctx->DrawBuffer->DepthBuffer, clearValue & 0xff,
                       2 * ctx->DrawBuffer->Width * ctx->DrawBuffer->Height);
            }
         }
         else {
            GLushort *d = (GLushort *) ctx->DrawBuffer->DepthBuffer;
            GLint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
            while (n >= 16) {
               d[0] = clearValue;    d[1] = clearValue;
               d[2] = clearValue;    d[3] = clearValue;
               d[4] = clearValue;    d[5] = clearValue;
               d[6] = clearValue;    d[7] = clearValue;
               d[8] = clearValue;    d[9] = clearValue;
               d[10] = clearValue;   d[11] = clearValue;
               d[12] = clearValue;   d[13] = clearValue;
               d[14] = clearValue;   d[15] = clearValue;
               d += 16;
               n -= 16;
            }
            while (n > 0) {
               *d++ = clearValue;
               n--;
            }
         }
      }
      else {
         /* >16 bit depth buffer */
         const GLuint clearValue = (GLuint) (ctx->Depth.Clear * ctx->Visual.DepthMax);
         if (clearValue == 0) {
            BZERO(ctx->DrawBuffer->DepthBuffer,
                ctx->DrawBuffer->Width*ctx->DrawBuffer->Height*sizeof(GLuint));
         }
         else {
            GLint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
            GLuint *d = (GLuint *) ctx->DrawBuffer->DepthBuffer;
            while (n >= 16) {
               d[0] = clearValue;    d[1] = clearValue;
               d[2] = clearValue;    d[3] = clearValue;
               d[4] = clearValue;    d[5] = clearValue;
               d[6] = clearValue;    d[7] = clearValue;
               d[8] = clearValue;    d[9] = clearValue;
               d[10] = clearValue;   d[11] = clearValue;
               d[12] = clearValue;   d[13] = clearValue;
               d[14] = clearValue;   d[15] = clearValue;
               d += 16;
               n -= 16;
            }
            while (n > 0) {
               *d++ = clearValue;
               n--;
            }
         }
      }
   }
}
