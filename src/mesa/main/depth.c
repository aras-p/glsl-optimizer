/* $Id: depth.c,v 1.8 1999/11/08 07:36:43 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
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


/* $XFree86: xc/lib/GL/mesa/src/depth.c,v 1.3 1999/04/04 00:20:22 dawes Exp $ */

/*
 * Depth buffer functions
 */

#include <stdlib.h>

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdio.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "enums.h"
#include "depth.h"
#include "macros.h"
#include "types.h"
#endif



/**********************************************************************/
/*****                          API Functions                     *****/
/**********************************************************************/



void gl_ClearDepth( GLcontext* ctx, GLclampd depth )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glClearDepth");
   ctx->Depth.Clear = (GLfloat) CLAMP( depth, 0.0, 1.0 );
   if (ctx->Driver.ClearDepth)
      (*ctx->Driver.ClearDepth)( ctx, ctx->Depth.Clear );
}



void gl_DepthFunc( GLcontext* ctx, GLenum func )
{
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



void gl_DepthMask( GLcontext* ctx, GLboolean flag )
{
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
 * Depth test horizontal spans of fragments.  These functions are called
 * via ctx->Driver.depth_test_span only.
 *
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in span in window coords
 *         z - array [n] of integer depth values
 * In/Out:  mask - array [n] of flags (1=draw pixel, 0=don't draw) 
 * Return:  number of pixels which passed depth test
 */


/*
 * glDepthFunc( any ) and glDepthMask( GL_TRUE or GL_FALSE ).
 */
GLuint gl_depth_test_span_generic( GLcontext* ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLdepth z[],
                                   GLubyte mask[] )
{
   GLdepth *zptr = Z_ADDRESS( ctx, x, y );
   GLubyte *m = mask;
   GLuint i;
   GLuint passed = 0;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] < *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] <= *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] <= *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] >= *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] >= *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] > *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] > *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] != *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] != *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] == *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m =0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] == *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m =0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  *zptr = z[i];
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
	 for (i=0;i<n;i++) {
	    mask[i] = 0;
	 }
	 break;
      default:
         gl_problem(ctx, "Bad depth func in gl_depth_test_span_generic");
   } /*switch*/

   return passed;
}



/*
 * glDepthFunc(GL_LESS) and glDepthMask(GL_TRUE).
 */
GLuint gl_depth_test_span_less( GLcontext* ctx,
                                GLuint n, GLint x, GLint y, const GLdepth z[],
                                GLubyte mask[] )
{
   GLdepth *zptr = Z_ADDRESS( ctx, x, y );
   GLuint i;
   GLuint passed = 0;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         if (z[i] < zptr[i]) {
            /* pass */
            zptr[i] = z[i];
            passed++;
         }
         else {
            /* fail */
            mask[i] = 0;
         }
      }
   }
   return passed;
}


/*
 * glDepthFunc(GL_GREATER) and glDepthMask(GL_TRUE).
 */
GLuint gl_depth_test_span_greater( GLcontext* ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLdepth z[],
                                   GLubyte mask[] )
{
   GLdepth *zptr = Z_ADDRESS( ctx, x, y );
   GLuint i;
   GLuint passed = 0;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         if (z[i] > zptr[i]) {
            /* pass */
            zptr[i] = z[i];
            passed++;
         }
         else {
            /* fail */
            mask[i] = 0;
         }
      }
   }
   return passed;
}



/*
 * Depth test an array of randomly positioned fragments.
 */


#define ZADDR_SETUP   GLdepth *depthbuffer = ctx->Buffer->Depth; \
                      GLint width = ctx->Buffer->Width;

#define ZADDR( X, Y )   (depthbuffer + (Y) * width + (X) )



/*
 * glDepthFunc( any ) and glDepthMask( GL_TRUE or GL_FALSE ).
 */
void gl_depth_test_pixels_generic( GLcontext* ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLdepth z[], GLubyte mask[] )
{
   register GLdepth *zptr;
   register GLuint i;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(ctx,x[i],y[i]);
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
	 for (i=0;i<n;i++) {
	    mask[i] = 0;
	 }
	 break;
      default:
         gl_problem(ctx, "Bad depth func in gl_depth_test_pixels_generic");
   } /*switch*/
}



/*
 * glDepthFunc( GL_LESS ) and glDepthMask( GL_TRUE ).
 */
void gl_depth_test_pixels_less( GLcontext* ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLdepth z[], GLubyte mask[] )
{
   GLdepth *zptr;
   GLuint i;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         zptr = Z_ADDRESS(ctx,x[i],y[i]);
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


/*
 * glDepthFunc( GL_GREATER ) and glDepthMask( GL_TRUE ).
 */
void gl_depth_test_pixels_greater( GLcontext* ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLdepth z[], GLubyte mask[] )
{
   GLdepth *zptr;
   GLuint i;

   for (i=0; i<n; i++) {
      if (mask[i]) {
         zptr = Z_ADDRESS(ctx,x[i],y[i]);
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




/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/*
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * This function is only called through Driver.read_depth_span_float()
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void gl_read_depth_span_float( GLcontext* ctx,
                               GLuint n, GLint x, GLint y, GLfloat depth[] )
{
   GLdepth *zptr;
   GLfloat scale;
   GLuint i;

   scale = 1.0F / DEPTH_SCALE;

   if (ctx->Buffer->Depth) {
      zptr = Z_ADDRESS( ctx, x, y );
      for (i=0;i<n;i++) {
	 depth[i] = (GLfloat) zptr[i] * scale;
      }
   }
   else {
      for (i=0;i<n;i++) {
	 depth[i] = 0.0F;
      }
   }
}


/*
 * Return a span of depth values from the depth buffer as integers in
 * [0,MAX_DEPTH].
 * This function is only called through Driver.read_depth_span_int()
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
void gl_read_depth_span_int( GLcontext* ctx,
                             GLuint n, GLint x, GLint y, GLdepth depth[] )
{
   if (ctx->Buffer->Depth) {
      GLdepth *zptr = Z_ADDRESS( ctx, x, y );
      MEMCPY( depth, zptr, n * sizeof(GLdepth) );
   }
   else {
      GLuint i;
      for (i=0;i<n;i++) {
	 depth[i] = 0;
      }
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
void gl_alloc_depth_buffer( GLcontext* ctx )
{
   /* deallocate current depth buffer if present */
   if (ctx->Buffer->Depth) {
      FREE(ctx->Buffer->Depth);
      ctx->Buffer->Depth = NULL;
   }

   /* allocate new depth buffer, but don't initialize it */
   ctx->Buffer->Depth = (GLdepth *) MALLOC( ctx->Buffer->Width
                                            * ctx->Buffer->Height
                                            * sizeof(GLdepth) );
   if (!ctx->Buffer->Depth) {
      /* out of memory */
      ctx->Depth.Test = GL_FALSE;
      ctx->NewState |= NEW_RASTER_OPS;
      gl_error( ctx, GL_OUT_OF_MEMORY, "Couldn't allocate depth buffer" );
   }
}




/*
 * Clear the depth buffer.  If the depth buffer doesn't exist yet we'll
 * allocate it now.
 * This function is only called through Driver.clear_depth_buffer.
 */
void gl_clear_depth_buffer( GLcontext* ctx )
{
   GLdepth clear_value = (GLdepth) (ctx->Depth.Clear * DEPTH_SCALE);
   
   if (ctx->Visual->DepthBits==0 || !ctx->Buffer->Depth || !ctx->Depth.Mask) {
      /* no depth buffer, or writing to it is disabled */
      return;
   }

   /* The loops in this function have been written so the IRIX 5.3
    * C compiler can unroll them.  Hopefully other compilers can too!
    */

   if (ctx->Scissor.Enabled) {
      /* only clear scissor region */
      GLint y;
      for (y=ctx->Buffer->Ymin; y<=ctx->Buffer->Ymax; y++) {
         GLdepth *d = Z_ADDRESS( ctx, ctx->Buffer->Xmin, y );
         GLint n = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
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
         MEMSET( ctx->Buffer->Depth, clear_value&0xff,
                 2*ctx->Buffer->Width*ctx->Buffer->Height);
      }
      else {
         GLdepth *d = ctx->Buffer->Depth;
         GLint n = ctx->Buffer->Width * ctx->Buffer->Height;
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



