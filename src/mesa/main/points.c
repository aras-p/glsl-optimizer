/* $Id: points.c,v 1.4 1999/10/21 12:45:53 brianp Exp $ */

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
/* $XFree86: xc/lib/GL/mesa/src/points.c,v 1.4 1999/04/04 00:20:29 dawes Exp $ */



#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <math.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "feedback.h"
#include "macros.h"
#include "mmath.h"
#include "pb.h"
#include "points.h"
#include "span.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#include "mmath.h"
#endif



void gl_PointSize( GLcontext *ctx, GLfloat size )
{
   if (size<=0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPointSize");

   if (ctx->Point.Size != size) {
      ctx->Point.Size = size;
      ctx->TriangleCaps &= ~DD_POINT_SIZE;
      if (size != 1.0) ctx->TriangleCaps |= DD_POINT_SIZE;
      ctx->NewState |= NEW_RASTER_OPS;
   }
}



void gl_PointParameterfvEXT( GLcontext *ctx, GLenum pname,
                                    const GLfloat *params)
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPointParameterfvEXT");
   if(pname==GL_DISTANCE_ATTENUATION_EXT) {
      GLboolean tmp = ctx->Point.Attenuated;
      COPY_3V(ctx->Point.Params,params);
      ctx->Point.Attenuated = (params[0] != 1.0 ||
			       params[1] != 0.0 ||
			       params[2] != 0.0);

      if (tmp != ctx->Point.Attenuated) {
	 ctx->Enabled ^= ENABLE_POINT_ATTEN;
	 ctx->TriangleCaps ^= DD_POINT_ATTEN;
	 ctx->NewState |= NEW_RASTER_OPS;
      }
   } else {
        if (*params<0.0 ) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
        }
        switch (pname) {
            case GL_POINT_SIZE_MIN_EXT:
                ctx->Point.MinSize=*params;
                break;
            case GL_POINT_SIZE_MAX_EXT:
                ctx->Point.MaxSize=*params;
                break;
            case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
                ctx->Point.Threshold=*params;
                break;
            default:
                gl_error( ctx, GL_INVALID_ENUM, "glPointParameterfvEXT" );
                return;
        }
   }
   ctx->NewState |= NEW_RASTER_OPS;
}


/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/


/*
 * There are 3 pairs (RGBA, CI) of point rendering functions:
 *   1. simple:  size=1 and no special rasterization functions (fastest)
 *   2. size1:  size=1 and any rasterization functions
 *   3. general:  any size and rasterization functions (slowest)
 *
 * All point rendering functions take the same two arguments: first and
 * last which specify that the points specified by VB[first] through
 * VB[last] are to be rendered.
 */





/*
 * CI points with size == 1.0
 */
static void size1_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat *win;
   GLint *pbx = PB->x, *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLuint *pbi = PB->i;
   GLuint pbcount = PB->count;
   GLuint i;

   win = &VB->Win.data[first][0];
   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         pbx[pbcount] = (GLint)  win[0];
         pby[pbcount] = (GLint)  win[1];
         pbz[pbcount] = (GLint) (win[2] + ctx->PointZoffset);
         pbi[pbcount] = VB->IndexPtr->data[i];
         pbcount++;
      }
      win += 3;
   }
   PB->count = pbcount;
   PB_CHECK_FLUSH(ctx, PB);
}



/*
 * RGBA points with size == 1.0
 */
static void size1_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint red, green, blue, alpha;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         red   = VB->ColorPtr->data[i][0];
         green = VB->ColorPtr->data[i][1];
         blue  = VB->ColorPtr->data[i][2];
         alpha = VB->ColorPtr->data[i][3];

         PB_WRITE_RGBA_PIXEL( PB, x, y, z, red, green, blue, alpha );
      }
   }
   PB_CHECK_FLUSH(ctx,PB);
}



/*
 * General CI points.
 */
static void general_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLint isize = (GLint) (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);
   GLint radius = isize >> 1;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

         PB_SET_INDEX( ctx, PB, VB->IndexPtr->data[i] );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}


/*
 * General RGBA points.
 */
static void general_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLint isize = (GLint) (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);
   GLint radius = isize >> 1;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

         PB_SET_COLOR( ctx, PB,
                       VB->ColorPtr->data[i][0],
                       VB->ColorPtr->data[i][1],
                       VB->ColorPtr->data[i][2],
                       VB->ColorPtr->data[i][3] );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}




/*
 * Textured RGBA points.
 */
static void textured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         isize = (GLint)
                   (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);
         if (isize<1) {
            isize = 1;
         }
         radius = isize >> 1;

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

         red   = VB->ColorPtr->data[i][0];
         green = VB->ColorPtr->data[i][1];
         blue  = VB->ColorPtr->data[i][2];
         alpha = VB->ColorPtr->data[i][3];
	 
	 switch (VB->TexCoordPtr[0]->size) {
	 case 4:
	    s = VB->TexCoordPtr[0]->data[i][0]/VB->TexCoordPtr[0]->data[i][3];
	    t = VB->TexCoordPtr[0]->data[i][1]/VB->TexCoordPtr[0]->data[i][3];
	    u = VB->TexCoordPtr[0]->data[i][2]/VB->TexCoordPtr[0]->data[i][3];
	    break;
	 case 3:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = VB->TexCoordPtr[0]->data[i][1];
	    u = VB->TexCoordPtr[0]->data[i][2];
	    break;
	 case 2:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = VB->TexCoordPtr[0]->data[i][1];
	    u = 0.0;
	    break;
	 case 1:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = 0.0;
	    u = 0.0;
	    break;
         default:
            /* should never get here */
            s = t = u = 0.0;
            gl_problem(ctx, "unexpected texcoord size in textured_rgba_points()");
	 }

/*    don't think this is needed
         PB_SET_COLOR( red, green, blue, alpha );
*/

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_TEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u );
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}


/*
 * Multitextured RGBA points.
 */
static void multitextured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;
         GLfloat s1, t1, u1;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         isize = (GLint)
                   (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);
         if (isize<1) {
            isize = 1;
         }
         radius = isize >> 1;

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

         red   = VB->ColorPtr->data[i][0];
         green = VB->ColorPtr->data[i][1];
         blue  = VB->ColorPtr->data[i][2];
         alpha = VB->ColorPtr->data[i][3];
	 
	 switch (VB->TexCoordPtr[0]->size) {
	 case 4:
	    s = VB->TexCoordPtr[0]->data[i][0]/VB->TexCoordPtr[0]->data[i][3];
	    t = VB->TexCoordPtr[0]->data[i][1]/VB->TexCoordPtr[0]->data[i][3];
	    u = VB->TexCoordPtr[0]->data[i][2]/VB->TexCoordPtr[0]->data[i][3];
	    break;
	 case 3:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = VB->TexCoordPtr[0]->data[i][1];
	    u = VB->TexCoordPtr[0]->data[i][2];
	    break;
	 case 2:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = VB->TexCoordPtr[0]->data[i][1];
	    u = 0.0;
	    break;
	 case 1:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = 0.0;
	    u = 0.0;
	    break;
         default:
            /* should never get here */
            s = t = u = 0.0;
            gl_problem(ctx, "unexpected texcoord size in multitextured_rgba_points()");
	 }

	 switch (VB->TexCoordPtr[1]->size) {
	 case 4:
	    s1 = VB->TexCoordPtr[1]->data[i][0]/VB->TexCoordPtr[1]->data[i][3];
	    t1 = VB->TexCoordPtr[1]->data[i][1]/VB->TexCoordPtr[1]->data[i][3];
	    u1 = VB->TexCoordPtr[1]->data[i][2]/VB->TexCoordPtr[1]->data[i][3];
	    break;
	 case 3:
	    s1 = VB->TexCoordPtr[1]->data[i][0];
	    t1 = VB->TexCoordPtr[1]->data[i][1];
	    u1 = VB->TexCoordPtr[1]->data[i][2];
	    break;
	 case 2:
	    s1 = VB->TexCoordPtr[1]->data[i][0];
	    t1 = VB->TexCoordPtr[1]->data[i][1];
	    u1 = 0.0;
	    break;
	 case 1:
	    s1 = VB->TexCoordPtr[1]->data[i][0];
	    t1 = 0.0;
	    u1 = 0.0;
	    break;
         default:
            /* should never get here */
            s1 = t1 = u1 = 0.0;
            gl_problem(ctx, "unexpected texcoord size in multitextured_rgba_points()");
	 }

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_MULTITEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}




/*
 * Antialiased points with or without texture mapping.
 */
static void antialiased_rgba_points( GLcontext *ctx,
                                     GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat radius, rmin, rmax, rmin2, rmax2, cscale;

   radius = CLAMP( ctx->Point.Size, MIN_POINT_SIZE, MAX_POINT_SIZE ) * 0.5F;
   rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
   rmax = radius + 0.7071F;
   rmin2 = rmin*rmin;
   rmax2 = rmax*rmax;
   cscale = 256.0F / (rmax2-rmin2);

   if (ctx->Texture.ReallyEnabled) {
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;
            GLfloat s, t, u;
            GLfloat s1, t1, u1;

            xmin = (GLint) (VB->Win.data[i][0] - radius);
            xmax = (GLint) (VB->Win.data[i][0] + radius);
            ymin = (GLint) (VB->Win.data[i][1] - radius);
            ymax = (GLint) (VB->Win.data[i][1] + radius);
            z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

            red   = VB->ColorPtr->data[i][0];
            green = VB->ColorPtr->data[i][1];
            blue  = VB->ColorPtr->data[i][2];

	    switch (VB->TexCoordPtr[0]->size) {
	    case 4:
	       s = (VB->TexCoordPtr[0]->data[i][0]/
		    VB->TexCoordPtr[0]->data[i][3]);
	       t = (VB->TexCoordPtr[0]->data[i][1]/
		    VB->TexCoordPtr[0]->data[i][3]);
	       u = (VB->TexCoordPtr[0]->data[i][2]/
		    VB->TexCoordPtr[0]->data[i][3]);
	       break;
	    case 3:
	       s = VB->TexCoordPtr[0]->data[i][0];
	       t = VB->TexCoordPtr[0]->data[i][1];
	       u = VB->TexCoordPtr[0]->data[i][2];
	       break;
	    case 2:
	       s = VB->TexCoordPtr[0]->data[i][0];
	       t = VB->TexCoordPtr[0]->data[i][1];
	       u = 0.0;
	       break;
	    case 1:
	       s = VB->TexCoordPtr[0]->data[i][0];
	       t = 0.0;
	       u = 0.0;
	       break;
            default:
               /* should never get here */
               s = t = u = 0.0;
               gl_problem(ctx, "unexpected texcoord size in antialiased_rgba_points()");
	    }

	    if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
	       /* Multitextured!  This is probably a slow enough path that
		  there's no reason to specialize the multitexture case. */
	       switch (VB->TexCoordPtr[1]->size) {
	       case 4:
		  s1 = ( VB->TexCoordPtr[1]->data[i][0] /  
			 VB->TexCoordPtr[1]->data[i][3]);
		  t1 = ( VB->TexCoordPtr[1]->data[i][1] / 
			 VB->TexCoordPtr[1]->data[i][3]);
		  u1 = ( VB->TexCoordPtr[1]->data[i][2] / 
			 VB->TexCoordPtr[1]->data[i][3]);
		  break;
	       case 3:
		  s1 = VB->TexCoordPtr[1]->data[i][0];
		  t1 = VB->TexCoordPtr[1]->data[i][1];
		  u1 = VB->TexCoordPtr[1]->data[i][2];
		  break;
	       case 2:
		  s1 = VB->TexCoordPtr[1]->data[i][0];
		  t1 = VB->TexCoordPtr[1]->data[i][1];
		  u1 = 0.0;
		  break;
	       case 1:
		  s1 = VB->TexCoordPtr[1]->data[i][0];
		  t1 = 0.0;
		  u1 = 0.0;
		  break;
               default:
                  /* should never get here */
                  s1 = t1 = u1 = 0.0;
                  gl_problem(ctx, "unexpected texcoord size in antialiased_rgba_points()");
	       }
	    }

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win.data[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win.data[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                     alpha = VB->ColorPtr->data[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                        PB_WRITE_MULTITEX_PIXEL( PB, x,y,z, red, green, blue, 
						 alpha, s, t, u, s1, t1, u1 );
                     } else {
                        PB_WRITE_TEX_PIXEL( PB, x,y,z, red, green, blue, 
					    alpha, s, t, u );
                     }
                  }
               }
            }

            PB_CHECK_FLUSH(ctx,PB);
         }
      }
   }
   else {
      /* Not texture mapped */
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;

            xmin = (GLint) (VB->Win.data[i][0] - radius);
            xmax = (GLint) (VB->Win.data[i][0] + radius);
            ymin = (GLint) (VB->Win.data[i][1] - radius);
            ymax = (GLint) (VB->Win.data[i][1] + radius);
            z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

            red   = VB->ColorPtr->data[i][0];
            green = VB->ColorPtr->data[i][1];
            blue  = VB->ColorPtr->data[i][2];

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win.data[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win.data[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                     alpha = VB->ColorPtr->data[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     PB_WRITE_RGBA_PIXEL( PB, x, y, z, red, green, blue, 
					  alpha );
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB);
	 }
      }
   }
}



/*
 * Null rasterizer for measuring transformation speed.
 */
static void null_points( GLcontext *ctx, GLuint first, GLuint last )
{
   (void) ctx;
   (void) first;
   (void) last;
}



/* Definition of the functions for GL_EXT_point_parameters */

/* Calculates the distance attenuation formula of a vector of points in
 * eye space coordinates 
 */
static void dist3(GLfloat *out, GLuint first, GLuint last,
		  const GLcontext *ctx, const GLvector4f *v)
{
   GLuint stride = v->stride;
   GLfloat *p = VEC_ELT(v, GLfloat, first);
   GLuint i;

   for (i = first ; i <= last ; i++, STRIDE_F(p, stride) )
   {
      GLfloat dist = GL_SQRT(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
      out[i] = 1/(ctx->Point.Params[0]+ 
		  dist * (ctx->Point.Params[1] +
			  dist * ctx->Point.Params[2]));
   }
}

static void dist2(GLfloat *out, GLuint first, GLuint last,
		  const GLcontext *ctx, const GLvector4f *v)
{
   GLuint stride = v->stride;
   GLfloat *p = VEC_ELT(v, GLfloat, first);
   GLuint i;

   for (i = first ; i <= last ; i++, STRIDE_F(p, stride) )
   {
      GLfloat dist = GL_SQRT(p[0]*p[0]+p[1]*p[1]);
      out[i] = 1/(ctx->Point.Params[0]+ 
		  dist * (ctx->Point.Params[1] +
			  dist * ctx->Point.Params[2]));
   }
}


typedef void (*dist_func)(GLfloat *out, GLuint first, GLuint last,
			     const GLcontext *ctx, const GLvector4f *v);


static dist_func eye_dist_tab[5] = {
   0,
   0,
   dist2,
   dist3,
   dist3
};


static void clip_dist(GLfloat *out, GLuint first, GLuint last,
		      const GLcontext *ctx, GLvector4f *clip)
{
   /* this is never called */
   gl_problem(NULL, "clip_dist() called - dead code!\n");

   (void) out;
   (void) first;
   (void) last;
   (void) ctx;
   (void) clip;

#if 0
   GLuint i;
   const GLfloat *from = (GLfloat *)clip_vec->start;
   const GLuint stride = clip_vec->stride;

   for (i = first ; i <= last ; i++ )
   {
      GLfloat dist = win[i][2];
      out[i] = 1/(ctx->Point.Params[0]+ 
		  dist * (ctx->Point.Params[1] +
			  dist * ctx->Point.Params[2]));
   }
#endif
}



/*
 * Distance Attenuated General CI points.
 */
static void dist_atten_general_ci_points( GLcontext *ctx, GLuint first, 
					GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat psize,dsize;
   GLfloat dist[VB_SIZE];
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         dsize=psize*dist[i];
         if(dsize>=ctx->Point.Threshold) {
            isize=(GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
         } else {
            isize=(GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
         }
         radius = isize >> 1;

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

         PB_SET_INDEX( ctx, PB, VB->IndexPtr->data[i] );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}

/*
 * Distance Attenuated General RGBA points.
 */
static void dist_atten_general_rgba_points( GLcontext *ctx, GLuint first, 
				GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLubyte alpha;
   GLfloat psize,dsize;
   GLfloat dist[VB_SIZE];
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);
         dsize=psize*dist[i];
         if (dsize >= ctx->Point.Threshold) {
            isize = (GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
            alpha = VB->ColorPtr->data[i][3];
         }
         else {
            isize = (GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
            dsize /= ctx->Point.Threshold;
            alpha = (GLint) (VB->ColorPtr->data[i][3]* (dsize*dsize));
         }
         radius = isize >> 1;

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

         PB_SET_COLOR( ctx, PB,
                       VB->ColorPtr->data[i][0],
                       VB->ColorPtr->data[i][1],
                       VB->ColorPtr->data[i][2],
                       alpha );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}

/*
 *  Distance Attenuated Textured RGBA points.
 */
static void dist_atten_textured_rgba_points( GLcontext *ctx, GLuint first, 
					GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat psize,dsize;
   GLfloat dist[VB_SIZE];
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;
         GLfloat s1, t1, u1;

         x = (GLint)  VB->Win.data[i][0];
         y = (GLint)  VB->Win.data[i][1];
         z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         dsize=psize*dist[i];
         if(dsize>=ctx->Point.Threshold) {
            isize=(GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
            alpha=VB->ColorPtr->data[i][3];
         } else {
            isize=(GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
            dsize/=ctx->Point.Threshold;
            alpha = (GLint) (VB->ColorPtr->data[i][3]* (dsize*dsize));
         }

         if (isize<1) {
            isize = 1;
         }
         radius = isize >> 1;

         if (isize & 1) {
            /* odd size */
            x0 = x - radius;
            x1 = x + radius;
            y0 = y - radius;
            y1 = y + radius;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 1.5F) - radius;
            x1 = x0 + isize - 1;
            y0 = (GLint) (y + 1.5F) - radius;
            y1 = y0 + isize - 1;
         }

	 red   = VB->ColorPtr->data[i][0];
	 green = VB->ColorPtr->data[i][1];
	 blue  = VB->ColorPtr->data[i][2];
	 
	 switch (VB->TexCoordPtr[0]->size) {
	 case 4:
	    s = (VB->TexCoordPtr[0]->data[i][0]/
		 VB->TexCoordPtr[0]->data[i][3]);
	    t = (VB->TexCoordPtr[0]->data[i][1]/
		 VB->TexCoordPtr[0]->data[i][3]);
	    u = (VB->TexCoordPtr[0]->data[i][2]/
		 VB->TexCoordPtr[0]->data[i][3]);
	    break;
	 case 3:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = VB->TexCoordPtr[0]->data[i][1];
	    u = VB->TexCoordPtr[0]->data[i][2];
	    break;
	 case 2:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = VB->TexCoordPtr[0]->data[i][1];
	    u = 0.0;
	    break;
	 case 1:
	    s = VB->TexCoordPtr[0]->data[i][0];
	    t = 0.0;
	    u = 0.0;
	    break;
         default:
            /* should never get here */
            s = t = u = 0.0;
            gl_problem(ctx, "unexpected texcoord size in dist_atten_textured_rgba_points()");
	 }

	 if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
	    /* Multitextured!  This is probably a slow enough path that
	       there's no reason to specialize the multitexture case. */
	    switch (VB->TexCoordPtr[1]->size) {
	    case 4:
	       s1 = ( VB->TexCoordPtr[1]->data[i][0] /  
		      VB->TexCoordPtr[1]->data[i][3] );
	       t1 = ( VB->TexCoordPtr[1]->data[i][1] / 
		      VB->TexCoordPtr[1]->data[i][3] );
	       u1 = ( VB->TexCoordPtr[1]->data[i][2] / 
		      VB->TexCoordPtr[1]->data[i][3] );
	       break;
	    case 3:
	       s1 = VB->TexCoordPtr[1]->data[i][0];
	       t1 = VB->TexCoordPtr[1]->data[i][1];
	       u1 = VB->TexCoordPtr[1]->data[i][2];
	       break;
	    case 2:
	       s1 = VB->TexCoordPtr[1]->data[i][0];
	       t1 = VB->TexCoordPtr[1]->data[i][1];
	       u1 = 0.0;
	       break;
	    case 1:
	       s1 = VB->TexCoordPtr[1]->data[i][0];
	       t1 = 0.0;
	       u1 = 0.0;
	       break;
            default:
               /* should never get here */
               s1 = t1 = u1 = 0.0;
               gl_problem(ctx, "unexpected texcoord size in dist_atten_textured_rgba_points()");
	    }
	 }

/*    don't think this is needed
      PB_SET_COLOR( red, green, blue, alpha );
*/
  
         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                  PB_WRITE_MULTITEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
               } else {
                  PB_WRITE_TEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u );
               }
            }
         }
         PB_CHECK_FLUSH(ctx,PB);
      }
   }
}

/*
 * Distance Attenuated Antialiased points with or without texture mapping.
 */
static void dist_atten_antialiased_rgba_points( GLcontext *ctx,
                                     GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat radius, rmin, rmax, rmin2, rmax2, cscale;
   GLfloat psize,dsize,alphaf;
   GLfloat dist[VB_SIZE];
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   if (ctx->Texture.ReallyEnabled) {
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;
            GLfloat s, t, u;
            GLfloat s1, t1, u1;

            dsize=psize*dist[i];
            if(dsize>=ctx->Point.Threshold) {
               radius=(MIN2(dsize,ctx->Point.MaxSize)*0.5F);
               alphaf=1.0;
            } else {
               radius=(MAX2(ctx->Point.Threshold,ctx->Point.MinSize)*0.5F);
               dsize/=ctx->Point.Threshold;
               alphaf=(dsize*dsize);
            }
            rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
            rmax = radius + 0.7071F;
            rmin2 = rmin*rmin;
            rmax2 = rmax*rmax;
            cscale = 256.0F / (rmax2-rmin2);

            xmin = (GLint) (VB->Win.data[i][0] - radius);
            xmax = (GLint) (VB->Win.data[i][0] + radius);
            ymin = (GLint) (VB->Win.data[i][1] - radius);
            ymax = (GLint) (VB->Win.data[i][1] + radius);
            z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

	    red   = VB->ColorPtr->data[i][0];
	    green = VB->ColorPtr->data[i][1];
	    blue  = VB->ColorPtr->data[i][2];
	 
	    switch (VB->TexCoordPtr[0]->size) {
	    case 4:
	       s = (VB->TexCoordPtr[0]->data[i][0]/
		    VB->TexCoordPtr[0]->data[i][3]);
	       t = (VB->TexCoordPtr[0]->data[i][1]/
		    VB->TexCoordPtr[0]->data[i][3]);
	       u = (VB->TexCoordPtr[0]->data[i][2]/
		    VB->TexCoordPtr[0]->data[i][3]);
	       break;
	    case 3:
	       s = VB->TexCoordPtr[0]->data[i][0];
	       t = VB->TexCoordPtr[0]->data[i][1];
	       u = VB->TexCoordPtr[0]->data[i][2];
	       break;
	    case 2:
	       s = VB->TexCoordPtr[0]->data[i][0];
	       t = VB->TexCoordPtr[0]->data[i][1];
	       u = 0.0;
	       break;
	    case 1:
	       s = VB->TexCoordPtr[0]->data[i][0];
	       t = 0.0;
	       u = 0.0;
	       break;
            default:
               /* should never get here */
               s = t = u = 0.0;
               gl_problem(ctx, "unexpected texcoord size in dist_atten_antialiased_rgba_points()");
	    }

	    if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
	       /* Multitextured!  This is probably a slow enough path that
		  there's no reason to specialize the multitexture case. */
	       switch (VB->TexCoordPtr[1]->size) {
	       case 4:
		  s1 = ( VB->TexCoordPtr[1]->data[i][0] /  
			 VB->TexCoordPtr[1]->data[i][3] );
		  t1 = ( VB->TexCoordPtr[1]->data[i][1] / 
			 VB->TexCoordPtr[1]->data[i][3] );
		  u1 = ( VB->TexCoordPtr[1]->data[i][2] / 
			 VB->TexCoordPtr[1]->data[i][3] );
		  break;
	       case 3:
		  s1 = VB->TexCoordPtr[1]->data[i][0];
		  t1 = VB->TexCoordPtr[1]->data[i][1];
		  u1 = VB->TexCoordPtr[1]->data[i][2];
		  break;
	       case 2:
		  s1 = VB->TexCoordPtr[1]->data[i][0];
		  t1 = VB->TexCoordPtr[1]->data[i][1];
		  u1 = 0.0;
		  break;
	       case 1:
		  s1 = VB->TexCoordPtr[1]->data[i][0];
		  t1 = 0.0;
		  u1 = 0.0;
		  break;
               default:
                  /* should never get here */
                  s = t = u = 0.0;
                  gl_problem(ctx, "unexpected texcoord size in dist_atten_antialiased_rgba_points()");
	       }
	    }

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win.data[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win.data[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                     alpha = VB->ColorPtr->data[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     alpha = (GLint) (alpha * alphaf);
                     if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                        PB_WRITE_MULTITEX_PIXEL( PB, x,y,z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
                     } else {
                        PB_WRITE_TEX_PIXEL( PB, x,y,z, red, green, blue, alpha, s, t, u );
                     }
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB);
         }
      }
   }
   else {
      /* Not texture mapped */
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;

            dsize=psize*dist[i];
            if(dsize>=ctx->Point.Threshold) {
               radius=(MIN2(dsize,ctx->Point.MaxSize)*0.5F);
               alphaf=1.0;
            } else {
               radius=(MAX2(ctx->Point.Threshold,ctx->Point.MinSize)*0.5F);
               dsize/=ctx->Point.Threshold;
               alphaf=(dsize*dsize);
            }
            rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
            rmax = radius + 0.7071F;
            rmin2 = rmin*rmin;
            rmax2 = rmax*rmax;
            cscale = 256.0F / (rmax2-rmin2);

            xmin = (GLint) (VB->Win.data[i][0] - radius);
            xmax = (GLint) (VB->Win.data[i][0] + radius);
            ymin = (GLint) (VB->Win.data[i][1] - radius);
            ymax = (GLint) (VB->Win.data[i][1] + radius);
            z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

            red   = VB->ColorPtr->data[i][0];
            green = VB->ColorPtr->data[i][1];
            blue  = VB->ColorPtr->data[i][2];

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win.data[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win.data[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
		     alpha = VB->ColorPtr->data[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     alpha = (GLint) (alpha * alphaf);
                     PB_WRITE_RGBA_PIXEL( PB, x, y, z, red, green, blue, alpha )
			;
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB);
	 }
      }
   }
}


/*
 * Examine the current context to determine which point drawing function
 * should be used.
 */
void gl_set_point_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual->RGBAflag;

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.PointsFunc = null_points;
         return;
      }
      if (ctx->Driver.PointsFunc) {
         /* Device driver will draw points. */
	 ctx->IndirectTriangles &= ~DD_POINT_SW_RASTERIZE;
	 return;
      }

      if (!ctx->Point.Attenuated) {
         if (ctx->Point.SmoothFlag && rgbmode) {
            ctx->Driver.PointsFunc = antialiased_rgba_points;
         }
         else if (ctx->Texture.ReallyEnabled) {
            if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
	       ctx->Driver.PointsFunc = multitextured_rgba_points;
            }
            else {
               ctx->Driver.PointsFunc = textured_rgba_points;
            }
         }
         else if (ctx->Point.Size==1.0) {
            /* size=1, any raster ops */
            if (rgbmode)
               ctx->Driver.PointsFunc = size1_rgba_points;
            else
               ctx->Driver.PointsFunc = size1_ci_points;
         }
         else {
	    /* every other kind of point rendering */
            if (rgbmode)
               ctx->Driver.PointsFunc = general_rgba_points;
            else
               ctx->Driver.PointsFunc = general_ci_points;
         }
      } 
      else if(ctx->Point.SmoothFlag && rgbmode) {
         ctx->Driver.PointsFunc = dist_atten_antialiased_rgba_points;
      }
      else if (ctx->Texture.ReallyEnabled) {
         ctx->Driver.PointsFunc = dist_atten_textured_rgba_points;
      } 
      else {
         /* every other kind of point rendering */
         if (rgbmode)
            ctx->Driver.PointsFunc = dist_atten_general_rgba_points;
         else
            ctx->Driver.PointsFunc = dist_atten_general_ci_points;
     }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      ctx->Driver.PointsFunc = gl_feedback_points;
   }
   else {
      /* GL_SELECT mode */
      ctx->Driver.PointsFunc = gl_select_points;
   }

}

