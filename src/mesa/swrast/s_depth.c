/*
 * Mesa 3-D graphics library
 * Version:  7.2.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


#include "main/glheader.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/format_unpack.h"
#include "main/format_pack.h"
#include "main/macros.h"
#include "main/imports.h"

#include "s_depth.h"
#include "s_span.h"


/**
 * Do depth test for a horizontal span of fragments.
 * Input:  zbuffer - array of z values in the zbuffer
 *         z - array of fragment z values
 * Return:  number of fragments which pass the test.
 */
static GLuint
depth_test_span16( struct gl_context *ctx, GLuint n,
                   GLushort zbuffer[], const GLuint z[], GLubyte mask[] )
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
         memset(mask, 0, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in depth_test_span16");
   }

   return passed;
}


static GLuint
depth_test_span32( struct gl_context *ctx, GLuint n,
                   GLuint zbuffer[], const GLuint z[], GLubyte mask[] )
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
         memset(mask, 0, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in depth_test_span32");
   }

   return passed;
}



/**
 * Clamp fragment Z values to the depth near/far range (glDepthRange()).
 * This is used when GL_ARB_depth_clamp/GL_DEPTH_CLAMP is turned on.
 * In that case, vertexes are not clipped against the near/far planes
 * so rasterization will produce fragment Z values outside the usual
 * [0,1] range.
 */
void
_swrast_depth_clamp_span( struct gl_context *ctx, SWspan *span )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLuint count = span->end;
   GLint *zValues = (GLint *) span->array->z; /* sign change */
   GLint min, max;
   GLfloat min_f, max_f;
   GLuint i;

   if (ctx->Viewport.Near < ctx->Viewport.Far) {
      min_f = ctx->Viewport.Near;
      max_f = ctx->Viewport.Far;
   } else {
      min_f = ctx->Viewport.Far;
      max_f = ctx->Viewport.Near;
   }

   /* Convert floating point values in [0,1] to device Z coordinates in
    * [0, DepthMax].
    * ex: If the Z buffer has 24 bits, DepthMax = 0xffffff.
    * 
    * XXX this all falls apart if we have 31 or more bits of Z because
    * the triangle rasterization code produces unsigned Z values.  Negative
    * vertex Z values come out as large fragment Z uints.
    */
   min = (GLint) (min_f * fb->_DepthMaxF);
   max = (GLint) (max_f * fb->_DepthMaxF);
   if (max < 0)
      max = 0x7fffffff; /* catch over flow for 30-bit z */

   /* Note that we do the comparisons here using signed integers.
    */
   for (i = 0; i < count; i++) {
      if (zValues[i] < min)
	 zValues[i] = min;
      if (zValues[i] > max)
	 zValues[i] = max;
   }
}


/**
 * Get array of 16-bit z values from the depth buffer.  With clipping.
 */
static void
get_z16_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
               GLuint count, const GLint x[], const GLint y[],
               GLushort zbuffer[])
{
   const GLint w = rb->Width, h = rb->Height;
   const GLubyte *map = (const GLubyte *) rb->Data;
   GLuint i;

   if (rb->Format == MESA_FORMAT_Z16) {
      const GLuint rowStride = rb->RowStride * 2;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            zbuffer[i] = *((GLushort *) (map + y[i] * rowStride + x[i] * 2));
         }
      }
   }
   else {
      const GLuint bpp = _mesa_get_format_bytes(rb->Format);
      const GLuint rowStride = rb->RowStride * bpp;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            GLuint d32;
            const GLubyte *src = map + y[i] * rowStride + x[i] * bpp;
            _mesa_unpack_uint_z_row(rb->Format, 1, src, &d32);
            zbuffer[i] = d32 >> 16;
         }
      }
   }
}


/**
 * Get array of 32-bit z values from the depth buffer.  With clipping.
 */
static void
get_z32_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
               GLuint count, const GLint x[], const GLint y[],
               GLuint zbuffer[])
{
   const GLint w = rb->Width, h = rb->Height;
   const GLubyte *map = (const GLubyte *) rb->Data;
   GLuint i;

   if (rb->Format == MESA_FORMAT_Z32) {
      const GLuint rowStride = rb->RowStride * 4;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            zbuffer[i] = *((GLuint *) (map + y[i] * rowStride + x[i] * 4));
         }
      }
   }
   else {
      const GLuint bpp = _mesa_get_format_bytes(rb->Format);
      const GLuint rowStride = rb->RowStride * bpp;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            const GLubyte *src = map + y[i] * rowStride+ x[i] * bpp;
            _mesa_unpack_uint_z_row(rb->Format, 1, src, &zbuffer[i]);
         }
      }
   }
}



/*
 * Apply depth test to span of fragments.
 */
static GLuint
depth_test_span( struct gl_context *ctx, SWspan *span)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_DepthBuffer;
   const GLint x = span->x;
   const GLint y = span->y;
   const GLuint count = span->end;
   const GLuint *zValues = span->array->z;
   GLubyte *mask = span->array->mask;
   GLuint passed;

   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(span->arrayMask & SPAN_Z);
   
   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* Directly access buffer */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort *zbuffer = (GLushort *) rb->GetPointer(ctx, rb, x, y);
         passed = depth_test_span16(ctx, count, zbuffer, zValues, mask);
      }
      else {
         GLuint *zbuffer = (GLuint *) rb->GetPointer(ctx, rb, x, y);
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         passed = depth_test_span32(ctx, count, zbuffer, zValues, mask);
      }
   }
   else {
      /* read depth values from buffer, test, write back */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort zbuffer[MAX_WIDTH];
         rb->GetRow(ctx, rb, count, x, y, zbuffer);
         passed = depth_test_span16(ctx, count, zbuffer, zValues, mask);
         rb->PutRow(ctx, rb, count, x, y, zbuffer, mask);
      }
      else {
         GLuint zbuffer[MAX_WIDTH];
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         rb->GetRow(ctx, rb, count, x, y, zbuffer);
         passed = depth_test_span32(ctx, count, zbuffer, zValues, mask);
         rb->PutRow(ctx, rb, count, x, y, zbuffer, mask);
      }
   }

   if (passed < count) {
      span->writeAll = GL_FALSE;
   }
   return passed;
}



#define Z_ADDRESS(X, Y)   (zStart + (Y) * stride + (X))


/*
 * Do depth testing for an array of fragments at assorted locations.
 */
static void
direct_depth_test_pixels16(struct gl_context *ctx, GLushort *zStart, GLuint stride,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLuint z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLushort *zptr = Z_ADDRESS(x[i], y[i]);
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
         memset(mask, 0, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in direct_depth_test_pixels");
   }
}



/*
 * Do depth testing for an array of fragments with direct access to zbuffer.
 */
static void
direct_depth_test_pixels32(struct gl_context *ctx, GLuint *zStart, GLuint stride,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLuint z[], GLubyte mask[] )
{
   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
            GLuint i;
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
		  GLuint *zptr = Z_ADDRESS(x[i], y[i]);
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
         memset(mask, 0, n * sizeof(GLubyte));
	 break;
      default:
         _mesa_problem(ctx, "Bad depth func in direct_depth_test_pixels");
   }
}




static GLuint
depth_test_pixels( struct gl_context *ctx, SWspan *span )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_DepthBuffer;
   const GLuint count = span->end;
   const GLint *x = span->array->x;
   const GLint *y = span->array->y;
   const GLuint *z = span->array->z;
   GLubyte *mask = span->array->mask;

   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* Directly access values */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort *zStart = (GLushort *) rb->Data;
         GLuint stride = rb->Width;
         direct_depth_test_pixels16(ctx, zStart, stride, count, x, y, z, mask);
      }
      else {
         GLuint *zStart = (GLuint *) rb->Data;
         GLuint stride = rb->Width;
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         direct_depth_test_pixels32(ctx, zStart, stride, count, x, y, z, mask);
      }
   }
   else {
      /* read depth values from buffer, test, write back */
      if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort zbuffer[MAX_WIDTH];
         get_z16_values(ctx, rb, count, x, y, zbuffer);
         depth_test_span16(ctx, count, zbuffer, z, mask);
         rb->PutValues(ctx, rb, count, x, y, zbuffer, mask);
      }
      else {
         GLuint zbuffer[MAX_WIDTH];
         ASSERT(rb->DataType == GL_UNSIGNED_INT);
         get_z32_values(ctx, rb, count, x, y, zbuffer);
         depth_test_span32(ctx, count, zbuffer, z, mask);
         rb->PutValues(ctx, rb, count, x, y, zbuffer, mask);
      }
   }

   return count; /* not really correct, but OK */
}


/**
 * Apply depth (Z) buffer testing to the span.
 * \return approx number of pixels that passed (only zero is reliable)
 */
GLuint
_swrast_depth_test_span( struct gl_context *ctx, SWspan *span)
{
   if (span->arrayMask & SPAN_XY)
      return depth_test_pixels(ctx, span);
   else
      return depth_test_span(ctx, span);
}


/**
 * GL_EXT_depth_bounds_test extension.
 * Discard fragments depending on whether the corresponding Z-buffer
 * values are outside the depth bounds test range.
 * Note: we test the Z buffer values, not the fragment Z values!
 * \return GL_TRUE if any fragments pass, GL_FALSE if no fragments pass
 */
GLboolean
_swrast_depth_bounds_test( struct gl_context *ctx, SWspan *span )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->_DepthBuffer;
   GLuint zMin = (GLuint) (ctx->Depth.BoundsMin * fb->_DepthMaxF + 0.5F);
   GLuint zMax = (GLuint) (ctx->Depth.BoundsMax * fb->_DepthMaxF + 0.5F);
   GLubyte *mask = span->array->mask;
   const GLuint count = span->end;
   GLuint i;
   GLboolean anyPass = GL_FALSE;

   if (rb->DataType == GL_UNSIGNED_SHORT) {
      /* get 16-bit values */
      GLushort zbuffer16[MAX_WIDTH], *zbuffer;
      if (span->arrayMask & SPAN_XY) {
         get_z16_values(ctx, rb, count, span->array->x, span->array->y,
                        zbuffer16);
         zbuffer = zbuffer16;
      }
      else {
         zbuffer = (GLushort*) rb->GetPointer(ctx, rb, span->x, span->y);
         if (!zbuffer) {
            rb->GetRow(ctx, rb, count, span->x, span->y, zbuffer16);
            zbuffer = zbuffer16;
         }
      }
      assert(zbuffer);

      /* Now do the tests */
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            if (zbuffer[i] < zMin || zbuffer[i] > zMax)
               mask[i] = GL_FALSE;
            else
               anyPass = GL_TRUE;
         }
      }
   }
   else {
      /* get 32-bit values */
      GLuint zbuffer32[MAX_WIDTH], *zbuffer;
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      if (span->arrayMask & SPAN_XY) {
         get_z32_values(ctx, rb, count, span->array->x, span->array->y,
                        zbuffer32);
         zbuffer = zbuffer32;
      }
      else {
         zbuffer = (GLuint*) rb->GetPointer(ctx, rb, span->x, span->y);
         if (!zbuffer) {
            rb->GetRow(ctx, rb, count, span->x, span->y, zbuffer32);
            zbuffer = zbuffer32;
         }
      }
      assert(zbuffer);

      /* Now do the tests */
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            if (zbuffer[i] < zMin || zbuffer[i] > zMax)
               mask[i] = GL_FALSE;
            else
               anyPass = GL_TRUE;
         }
      }
   }

   return anyPass;
}



/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/**
 * Read a span of depth values from the given depth renderbuffer, returning
 * the values as GLfloats.
 * This function does clipping to prevent reading outside the depth buffer's
 * bounds.
 */
void
_swrast_read_depth_span_float( struct gl_context *ctx, struct gl_renderbuffer *rb,
                               GLint n, GLint x, GLint y, GLfloat depth[] )
{
   const GLfloat scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;

   if (!rb) {
      /* really only doing this to prevent FP exceptions later */
      memset(depth, 0, n * sizeof(GLfloat));
      return;
   }

   ASSERT(rb->_BaseFormat == GL_DEPTH_COMPONENT);

   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      memset(depth, 0, n * sizeof(GLfloat));
      return;
   }

   if (x < 0) {
      GLint dx = -x;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[i] = 0.0;
      x = 0;
      n -= dx;
      depth += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - (GLint) rb->Width;
      GLint i;
      for (i = 0; i < dx; i++)
         depth[n - i - 1] = 0.0;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   if (rb->DataType == GL_UNSIGNED_INT) {
      GLuint temp[MAX_WIDTH];
      GLint i;
      rb->GetRow(ctx, rb, n, x, y, temp);
      for (i = 0; i < n; i++) {
         depth[i] = temp[i] * scale;
      }
   }
   else if (rb->DataType == GL_UNSIGNED_SHORT) {
      GLushort temp[MAX_WIDTH];
      GLint i;
      rb->GetRow(ctx, rb, n, x, y, temp);
      for (i = 0; i < n; i++) {
         depth[i] = temp[i] * scale;
      }
   }
   else {
      _mesa_problem(ctx, "Invalid depth renderbuffer data type");
   }
}


/**
 * Clear the given z/depth renderbuffer.  If the buffer is a combined
 * depth+stencil buffer, only the Z bits will be touched.
 */
void
_swrast_clear_depth_buffer(struct gl_context *ctx)
{
   struct gl_renderbuffer *rb =
      ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLuint clearValue;
   GLint x, y, width, height;
   GLubyte *map;
   GLint rowStride, i, j;
   GLbitfield mapMode;

   if (!rb || !ctx->Depth.Mask) {
      /* no depth buffer, or writing to it is disabled */
      return;
   }

   /* compute integer clearing value */
   if (ctx->Depth.Clear == 1.0) {
      clearValue = ctx->DrawBuffer->_DepthMax;
   }
   else {
      clearValue = (GLuint) (ctx->Depth.Clear * ctx->DrawBuffer->_DepthMaxF);
   }

   /* compute region to clear */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   mapMode = GL_MAP_WRITE_BIT;
   if (rb->Format == MESA_FORMAT_S8_Z24 ||
       rb->Format == MESA_FORMAT_X8_Z24 ||
       rb->Format == MESA_FORMAT_Z24_S8 ||
       rb->Format == MESA_FORMAT_Z24_X8) {
      mapMode |= GL_MAP_READ_BIT;
   }

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               mapMode, &map, &rowStride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClear(depth)");
      return;
   }

   switch (rb->Format) {
   case MESA_FORMAT_Z16:
      {
         GLfloat clear = (GLfloat) ctx->Depth.Clear;
         GLushort clearVal = 0;
         _mesa_pack_float_z_row(rb->Format, 1, &clear, &clearVal);
         if (clearVal == 0xffff && width * 2 == rowStride) {
            /* common case */
            memset(map, 0xff, width * height * 2);
         }
         else {
            for (i = 0; i < height; i++) {
               GLushort *row = (GLushort *) map;
               for (j = 0; j < width; j++) {
                  row[j] = clearVal;
               }
               map += rowStride;
            }
         }
      }
      break;
   case MESA_FORMAT_Z32:
   case MESA_FORMAT_Z32_FLOAT:
      {
         GLfloat clear = (GLfloat) ctx->Depth.Clear;
         GLuint clearVal = 0;
         _mesa_pack_float_z_row(rb->Format, 1, &clear, &clearVal);
         for (i = 0; i < height; i++) {
            GLuint *row = (GLuint *) map;
            for (j = 0; j < width; j++) {
               row[j] = clearVal;
            }
            map += rowStride;
         }
      }
      break;
   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_Z24_S8:
   case MESA_FORMAT_Z24_X8:
      {
         GLfloat clear = (GLfloat) ctx->Depth.Clear;
         GLuint clearVal = 0;
         GLuint mask;

         if (rb->Format == MESA_FORMAT_S8_Z24 ||
             rb->Format == MESA_FORMAT_X8_Z24)
            mask = 0xff000000;
         else
            mask = 0xff;

         _mesa_pack_float_z_row(rb->Format, 1, &clear, &clearVal);
         for (i = 0; i < height; i++) {
            GLuint *row = (GLuint *) map;
            for (j = 0; j < width; j++) {
               row[j] = (row[j] & mask) | clearVal;
            }
            map += rowStride;
         }

      }
      break;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      /* XXX untested */
      {
         GLfloat clearVal = (GLfloat) ctx->Depth.Clear;
         for (i = 0; i < height; i++) {
            GLfloat *row = (GLfloat *) map;
            for (j = 0; j < width; j++) {
               row[j * 2] = clearVal;
            }
            map += rowStride;
         }
      }
      break;
   default:
      _mesa_problem(ctx, "Unexpected depth buffer format %s"
                    " in _swrast_clear_depth_buffer()",
                    _mesa_get_format_name(rb->Format));
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}




/**
 * Clear both depth and stencil values in a combined depth+stencil buffer.
 */
void
_swrast_clear_depth_stencil_buffer(struct gl_context *ctx)
{
   const GLubyte stencilBits = ctx->DrawBuffer->Visual.stencilBits;
   const GLuint writeMask = ctx->Stencil.WriteMask[0];
   const GLuint stencilMax = (1 << stencilBits) - 1;
   struct gl_renderbuffer *rb =
      ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLint x, y, width, height;
   GLbitfield mapMode;
   GLubyte *map;
   GLint rowStride, i, j;

   /* check that we really have a combined depth+stencil buffer */
   assert(rb == ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer);

   /* compute region to clear */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   mapMode = GL_MAP_WRITE_BIT;
   if ((writeMask & stencilMax) != stencilMax) {
      /* need to mask stencil values */
      mapMode |= GL_MAP_READ_BIT;
   }

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               mapMode, &map, &rowStride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClear(depth+stencil)");
      return;
   }

   switch (rb->Format) {
   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_Z24_S8:
      {
         GLfloat zClear = (GLfloat) ctx->Depth.Clear;
         GLuint clear = 0, mask;

         _mesa_pack_float_z_row(rb->Format, 1, &zClear, &clear);

         if (rb->Format == MESA_FORMAT_S8_Z24) {
            mask = ((~writeMask) & 0xff) << 24;
            clear |= (ctx->Stencil.Clear & writeMask & 0xff) << 24;
         }
         else {
            mask = ((~writeMask) & 0xff);
            clear |= (ctx->Stencil.Clear & writeMask & 0xff);
         }

         for (i = 0; i < height; i++) {
            GLuint *row = (GLuint *) map;
            if (mask != 0x0) {
               for (j = 0; j < width; j++) {
                  row[j] = (row[j] & mask) | clear;
               }
            }
            else {
               for (j = 0; j < width; j++) {
                  row[j] = clear;
               }
            }
            map += rowStride;
         }
      }
      break;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      /* XXX untested */
      {
         const GLfloat zClear = (GLfloat) ctx->Depth.Clear;
         const GLuint sClear = ctx->Stencil.Clear & writeMask;
         const GLuint sMask = (~writeMask) & 0xff;
         for (i = 0; i < height; i++) {
            GLfloat *zRow = (GLfloat *) map;
            GLuint *sRow = (GLuint *) map;
            for (j = 0; j < width; j++) {
               zRow[j * 2 + 0] = zClear;
            }
            if (sMask != 0) {
               for (j = 0; j < width; j++) {
                  sRow[j * 2 + 1] = (sRow[j * 2 + 1] & sMask) | sClear;
               }
            }
            else {
               for (j = 0; j < width; j++) {
                  sRow[j * 2 + 1] = sClear;
               }
            }
            map += rowStride;
         }
      }
      break;
   default:
      _mesa_problem(ctx, "Unexpected depth buffer format %s"
                    " in _swrast_clear_depth_buffer()",
                    _mesa_get_format_name(rb->Format));
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

}
