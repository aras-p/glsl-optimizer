/* $Id: depth.c,v 1.12 2000/02/02 22:16:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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
depth_test_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                 GLdepth zbuffer[], const GLdepth z[], GLubyte mask[] )
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
         MEMSET(mask, 0, n * sizeof(GLubyte));
	 break;
      default:
         gl_problem(ctx, "Bad depth func in depth_test_span");
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
   GLdepth zbuffer[MAX_WIDTH];
   GLdepth *zptr;
   GLuint passed;

   if (ctx->Driver.ReadDepthSpan) {
      /* read depth values out of hardware Z buffer */
      (*ctx->Driver.ReadDepthSpan)(ctx, n, x, y, zbuffer);
      zptr = zbuffer;
   }
   else {
      /* test against software depth buffer values */
      zptr = Z_ADDRESS( ctx, x, y );
   }

   passed = depth_test_span( ctx, n, x, y, zptr, z, mask );

   if (ctx->Driver.WriteDepthSpan) {
      /* write updated depth values into hardware Z buffer */
      assert(zptr == zbuffer);
      (*ctx->Driver.WriteDepthSpan)(ctx, n, x, y, zbuffer, mask);
   }

   return passed;
}




/*
 * Do depth testing for an array of fragments using software Z buffer.
 */
static void
software_depth_test_pixels( GLcontext *ctx, GLuint n,
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
		  GLdepth *zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
         MEMSET(mask, 0, n * sizeof(GLubyte));
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
         MEMSET(mask, 0, n * sizeof(GLubyte));
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
      (*ctx->Driver.WriteDepthPixels)(ctx, n, x, y, z, mask );
   }
   else {
      /* software depth testing */
      software_depth_test_pixels(ctx, n, x, y, z, mask);
   }
}





/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/*
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * This is used for both hardware and software depth buffers.
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void
_mesa_read_depth_span_float( GLcontext* ctx,
                             GLuint n, GLint x, GLint y, GLfloat depth[] )
{
   const GLfloat scale = 1.0F / DEPTH_SCALE;

   if (ctx->DrawBuffer->Depth) {
      /* read from software depth buffer */
      const GLdepth *zptr = Z_ADDRESS( ctx, x, y );
      GLuint i;
      for (i = 0; i < n; i++) {
	 depth[i] = (GLfloat) zptr[i] * scale;
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
      MEMSET(depth, 0, n * sizeof(GLfloat));
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
_mesa_alloc_depth_buffer( GLcontext* ctx )
{
   /* deallocate current depth buffer if present */
   if (ctx->DrawBuffer->UseSoftwareDepthBuffer) {
      if (ctx->DrawBuffer->Depth) {
         FREE(ctx->DrawBuffer->Depth);
         ctx->DrawBuffer->Depth = NULL;
      }

      /* allocate new depth buffer, but don't initialize it */
      ctx->DrawBuffer->Depth = (GLdepth *) MALLOC( ctx->DrawBuffer->Width
                                                   * ctx->DrawBuffer->Height
                                                   * sizeof(GLdepth) );
      if (!ctx->DrawBuffer->Depth) {
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
_mesa_clear_depth_buffer( GLcontext* ctx )
{
   GLdepth clear_value = (GLdepth) (ctx->Depth.Clear * DEPTH_SCALE);
   
   if (ctx->Visual->DepthBits==0 || !ctx->DrawBuffer->Depth || !ctx->Depth.Mask) {
      /* no depth buffer, or writing to it is disabled */
      return;
   }

   /* The loops in this function have been written so the IRIX 5.3
    * C compiler can unroll them.  Hopefully other compilers can too!
    */

   if (ctx->Scissor.Enabled) {
      /* only clear scissor region */
      GLint y;
      for (y=ctx->DrawBuffer->Ymin; y<=ctx->DrawBuffer->Ymax; y++) {
         GLdepth *d = Z_ADDRESS( ctx, ctx->DrawBuffer->Xmin, y );
         GLint n = ctx->DrawBuffer->Xmax - ctx->DrawBuffer->Xmin + 1;
         do {
            *d++ = clear_value;
            n--;
         } while (n);
      }
   }
   else {
      /* clear whole buffer */
      if (sizeof(GLdepth)==2 && (clear_value&0xff)==(clear_value>>8)) {
         /* lower and upper bytes of clear_value are same, use MEMSET */
         MEMSET( ctx->DrawBuffer->Depth, clear_value & 0xff,
                 2*ctx->DrawBuffer->Width * ctx->DrawBuffer->Height);
      }
      else {
         GLdepth *d = ctx->DrawBuffer->Depth;
         GLint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
         while (n>=16) {
            d[0] = clear_value;    d[1] = clear_value;
            d[2] = clear_value;    d[3] = clear_value;
            d[4] = clear_value;    d[5] = clear_value;
            d[6] = clear_value;    d[7] = clear_value;
            d[8] = clear_value;    d[9] = clear_value;
            d[10] = clear_value;   d[11] = clear_value;
            d[12] = clear_value;   d[13] = clear_value;
            d[14] = clear_value;   d[15] = clear_value;
            d += 16;
            n -= 16;
         }
         while (n>0) {
            *d++ = clear_value;
            n--;
         }
      }
   }
}
