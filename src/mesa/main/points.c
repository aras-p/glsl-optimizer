/* $Id: points.c,v 1.10 2000/06/28 23:11:10 brianp Exp $ */

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
#include "feedback.h"
#include "macros.h"
#include "mmath.h"
#include "pb.h"
#include "points.h"
#include "span.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#endif



void
_mesa_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPointSize");

   if (size <= 0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }

   if (ctx->Point.UserSize != size) {
      ctx->Point.UserSize = size;
      ctx->Point.Size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);
      ctx->TriangleCaps &= ~DD_POINT_SIZE;
      if (size != 1.0)
         ctx->TriangleCaps |= DD_POINT_SIZE;
      ctx->NewState |= NEW_RASTER_OPS;
   }
}



void
_mesa_PointParameterfEXT( GLenum pname, GLfloat param)
{
   _mesa_PointParameterfvEXT(pname, &param);
}


void
_mesa_PointParameterfvEXT( GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPointParameterfvEXT");

   switch (pname) {
      case GL_DISTANCE_ATTENUATION_EXT:
         {
            const GLboolean tmp = ctx->Point.Attenuated;
            COPY_3V(ctx->Point.Params, params);
            ctx->Point.Attenuated = (params[0] != 1.0 ||
                                     params[1] != 0.0 ||
                                     params[2] != 0.0);

            if (tmp != ctx->Point.Attenuated) {
               ctx->Enabled ^= ENABLE_POINT_ATTEN;
               ctx->TriangleCaps ^= DD_POINT_ATTEN;
               ctx->NewState |= NEW_RASTER_OPS;
            }
         }
         break;
      case GL_POINT_SIZE_MIN_EXT:
         if (*params < 0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         ctx->Point.MinSize = *params;
         break;
      case GL_POINT_SIZE_MAX_EXT:
         if (*params < 0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         ctx->Point.MaxSize = *params;
         break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
         if (*params < 0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         ctx->Point.Threshold = *params;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPointParameterfvEXT" );
         return;
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
static void
size1_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat *win;
   GLint *pbx = PB->x, *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLuint *pbi = PB->index;
   GLuint pbcount = PB->count;
   GLuint i;

   win = &VB->Win.data[first][0];
   for (i = first; i <= last; i++) {
      if (VB->ClipMask[i] == 0) {
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
static void
size1_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i = first; i <= last; i++) {
      if (VB->ClipMask[i] == 0) {
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
   PB_CHECK_FLUSH(ctx, PB);
}



/*
 * General CI points.
 */
static void
general_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   const GLint isize = (GLint) (ctx->Point.Size + 0.5F);
   GLint radius = isize >> 1;
   GLuint i;

   for (i = first; i <= last; i++) {
      if (VB->ClipMask[i] == 0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

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

         PB_SET_INDEX( PB, VB->IndexPtr->data[i] );

         for (iy = y0; iy <= y1; iy++) {
            for (ix = x0; ix <= x1; ix++) {
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
static void
general_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLint isize = (GLint) (ctx->Point.Size + 0.5F);
   GLint radius = isize >> 1;
   GLuint i;

   for (i = first; i <= last; i++) {
      if (VB->ClipMask[i] == 0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

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

         PB_SET_COLOR( PB,
                       VB->ColorPtr->data[i][0],
                       VB->ColorPtr->data[i][1],
                       VB->ColorPtr->data[i][2],
                       VB->ColorPtr->data[i][3] );

         for (iy = y0; iy <= y1; iy++) {
            for (ix = x0; ix <= x1; ix++) {
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
static void
textured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i = first; i <= last; i++) {
      if (VB->ClipMask[i] == 0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy, radius;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;

         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);
         GLint isize = (GLint) (ctx->Point.Size + 0.5F);

         if (isize < 1) {
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

         for (iy = y0; iy <= y1; iy++) {
            for (ix = x0; ix <= x1; ix++) {
               PB_WRITE_TEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha,
                                   s, t, u );
            }
         }

         PB_CHECK_FLUSH(ctx, PB);
      }
   }
}


/*
 * Multitextured RGBA points.
 */
static void
multitextured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i = first; i <= last; i++) {
      if (VB->ClipMask[i] == 0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint radius;
         GLint red, green, blue, alpha;
         GLint sRed, sGreen, sBlue;
         GLfloat s, t, u;
         GLfloat s1, t1, u1;

         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);
         GLint isize = (GLint) (ctx->Point.Size + 0.5F);

         if (isize < 1) {
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
	 sRed   = VB->Specular ? VB->Specular[i][0] : 0;
	 sGreen = VB->Specular ? VB->Specular[i][1] : 0;
	 sBlue  = VB->Specular ? VB->Specular[i][2] : 0;
	 
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
               PB_WRITE_MULTITEX_SPEC_PIXEL( PB, ix, iy, z,
                                             red, green, blue, alpha,
                                             sRed, sGreen, sBlue,
                                             s, t, u, s1, t1, u1 );
            }
         }
         PB_CHECK_FLUSH(ctx, PB);
      }
   }
}




/*
 * Antialiased points with or without texture mapping.
 */
static void
antialiased_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   const GLfloat radius = ctx->Point.Size * 0.5F;
   const GLfloat rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
   const GLfloat rmax = radius + 0.7071F;
   const GLfloat rmin2 = rmin * rmin;
   const GLfloat rmax2 = rmax * rmax;
   const GLfloat cscale = 256.0F / (rmax2 - rmin2);
   GLuint i;

   if (ctx->Texture.ReallyEnabled) {
      for (i = first; i <= last; i++) {
         if (VB->ClipMask[i] == 0) {
            GLint x, y;
            GLint red, green, blue, alpha;
            GLfloat s, t, u;
            GLfloat s1, t1, u1;

            GLint xmin = (GLint) (VB->Win.data[i][0] - radius);
            GLint xmax = (GLint) (VB->Win.data[i][0] + radius);
            GLint ymin = (GLint) (VB->Win.data[i][1] - radius);
            GLint ymax = (GLint) (VB->Win.data[i][1] + radius);
            GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

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
static void
null_points( GLcontext *ctx, GLuint first, GLuint last )
{
   (void) ctx;
   (void) first;
   (void) last;
}



/* Definition of the functions for GL_EXT_point_parameters */

/* Calculates the distance attenuation formula of a vector of points in
 * eye space coordinates 
 */
static void
dist3(GLfloat *out, GLuint first, GLuint last,
      const GLcontext *ctx, const GLvector4f *v)
{
   GLuint stride = v->stride;
   const GLfloat *p = VEC_ELT(v, GLfloat, first);
   GLuint i;

   for (i = first ; i <= last ; i++, STRIDE_F(p, stride) ) {
      GLfloat dist = GL_SQRT(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
      out[i] = 1.0F / (ctx->Point.Params[0] +
                       dist * (ctx->Point.Params[1] +
                               dist * ctx->Point.Params[2]));
   }
}


static void
dist2(GLfloat *out, GLuint first, GLuint last,
      const GLcontext *ctx, const GLvector4f *v)
{
   GLuint stride = v->stride;
   const GLfloat *p = VEC_ELT(v, GLfloat, first);
   GLuint i;

   for (i = first ; i <= last ; i++, STRIDE_F(p, stride) ) {
      GLfloat dist = GL_SQRT(p[0]*p[0]+p[1]*p[1]);
      out[i] = 1.0F / (ctx->Point.Params[0] +
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


static void
clip_dist(GLfloat *out, GLuint first, GLuint last,
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
static void
dist_atten_general_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat dist[VB_SIZE];
   const GLfloat psize = ctx->Point.Size;
   GLuint i;

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;
         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);
         GLfloat dsize = psize * dist[i];

         if (dsize >= ctx->Point.Threshold) {
            isize = (GLint) (MIN2(dsize, ctx->Point.MaxSize) + 0.5F);
         }
         else {
            isize = (GLint) (MAX2(ctx->Point.Threshold, ctx->Point.MinSize) + 0.5F);
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

         PB_SET_INDEX( PB, VB->IndexPtr->data[i] );

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
static void
dist_atten_general_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat dist[VB_SIZE];
   const GLfloat psize = ctx->Point.Size;
   GLuint i;

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;
         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);
         GLfloat dsize=psize*dist[i];
         GLubyte alpha;

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

         PB_SET_COLOR( PB,
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
static void
dist_atten_textured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat dist[VB_SIZE];
   const GLfloat psize = ctx->Point.Size;
   GLuint i;

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize, radius;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;
         GLfloat s1, t1, u1;

         GLint x = (GLint)  VB->Win.data[i][0];
         GLint y = (GLint)  VB->Win.data[i][1];
         GLint z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

         GLfloat dsize = psize*dist[i];
         if(dsize >= ctx->Point.Threshold) {
            isize = (GLint) (MIN2(dsize, ctx->Point.MaxSize) + 0.5F);
            alpha = VB->ColorPtr->data[i][3];
         }
         else {
            isize = (GLint) (MAX2(ctx->Point.Threshold, ctx->Point.MinSize) + 0.5F);
            dsize /= ctx->Point.Threshold;
            alpha = (GLint) (VB->ColorPtr->data[i][3] * (dsize * dsize));
         }

         if (isize < 1) {
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
static void
dist_atten_antialiased_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat dist[VB_SIZE];
   const GLfloat psize = ctx->Point.Size;
   GLuint i;

   if (ctx->NeedEyeCoords)
      (eye_dist_tab[VB->EyePtr->size])( dist, first, last, ctx, VB->EyePtr );
   else 
      clip_dist( dist, first, last, ctx, VB->ClipPtr );

   if (ctx->Texture.ReallyEnabled) {
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLfloat radius, rmin, rmax, rmin2, rmax2, cscale, alphaf;
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;
            GLfloat s, t, u;
            GLfloat s1, t1, u1;
            GLfloat dsize = psize * dist[i];

            if (dsize >= ctx->Point.Threshold) {
               radius = MIN2(dsize, ctx->Point.MaxSize) * 0.5F;
               alphaf = 1.0F;
            }
            else {
               radius = (MAX2(ctx->Point.Threshold, ctx->Point.MinSize) * 0.5F);
               dsize /= ctx->Point.Threshold;
               alphaf = (dsize*dsize);
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

            for (y = ymin; y <= ymax; y++) {
               for (x = xmin; x <= xmax; x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win.data[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win.data[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2 < rmax2) {
                     alpha = VB->ColorPtr->data[i][3];
                     if (dist2 >= rmin2) {
                        GLint coverage = (GLint) (256.0F - (dist2 - rmin2) * cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     alpha = (GLint) (alpha * alphaf);
                     if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                        PB_WRITE_MULTITEX_PIXEL( PB, x,y,z, red, green, blue,
                                                 alpha, s, t, u, s1, t1, u1 );
                     } else {
                        PB_WRITE_TEX_PIXEL( PB, x,y,z, red, green, blue, alpha,
                                            s, t, u );
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
      for (i = first; i <= last; i++) {
         if (VB->ClipMask[i] == 0) {
            GLfloat radius, rmin, rmax, rmin2, rmax2, cscale, alphaf;
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;
            GLfloat dsize = psize * dist[i];

            if (dsize >= ctx->Point.Threshold) {
               radius = MIN2(dsize, ctx->Point.MaxSize) * 0.5F;
               alphaf = 1.0F;
            }
            else {
               radius = (MAX2(ctx->Point.Threshold, ctx->Point.MinSize) * 0.5F);
               dsize /= ctx->Point.Threshold;
               alphaf = dsize * dsize;
            }
            rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
            rmax = radius + 0.7071F;
            rmin2 = rmin * rmin;
            rmax2 = rmax * rmax;
            cscale = 256.0F / (rmax2 - rmin2);

            xmin = (GLint) (VB->Win.data[i][0] - radius);
            xmax = (GLint) (VB->Win.data[i][0] + radius);
            ymin = (GLint) (VB->Win.data[i][1] - radius);
            ymax = (GLint) (VB->Win.data[i][1] + radius);
            z = (GLint) (VB->Win.data[i][2] + ctx->PointZoffset);

            red   = VB->ColorPtr->data[i][0];
            green = VB->ColorPtr->data[i][1];
            blue  = VB->ColorPtr->data[i][2];

            for (y = ymin; y <= ymax; y++) {
               for (x = xmin; x <= xmax; x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win.data[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win.data[i][1];
                  GLfloat dist2 = dx * dx + dy * dy;
                  if (dist2 < rmax2) {
		     alpha = VB->ColorPtr->data[i][3];
                     if (dist2 >= rmin2) {
                        GLint coverage = (GLint) (256.0F - (dist2 - rmin2) * cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     alpha = (GLint) (alpha * alphaf);
                     PB_WRITE_RGBA_PIXEL(PB, x, y, z, red, green, blue, alpha);
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB);
	 }
      }
   }
}


#ifdef DEBUG
void
_mesa_print_points_function(GLcontext *ctx)
{
   printf("Point Func == ");
   if (ctx->Driver.PointsFunc == size1_ci_points)
      printf("size1_ci_points\n");
   else if (ctx->Driver.PointsFunc == size1_rgba_points)
      printf("size1_rgba_points\n");
   else if (ctx->Driver.PointsFunc == general_ci_points)
      printf("general_ci_points\n");
   else if (ctx->Driver.PointsFunc == general_rgba_points)
      printf("general_rgba_points\n");
   else if (ctx->Driver.PointsFunc == textured_rgba_points)
      printf("textured_rgba_points\n");
   else if (ctx->Driver.PointsFunc == multitextured_rgba_points)
      printf("multitextured_rgba_points\n");
   else if (ctx->Driver.PointsFunc == antialiased_rgba_points)
      printf("antialiased_rgba_points\n");
   else if (ctx->Driver.PointsFunc == null_points)
      printf("null_points\n");
   else if (ctx->Driver.PointsFunc == dist_atten_general_ci_points)
      printf("dist_atten_general_ci_points\n");
   else if (ctx->Driver.PointsFunc == dist_atten_general_rgba_points)
      printf("dist_atten_general_rgba_points\n");
   else if (ctx->Driver.PointsFunc == dist_atten_textured_rgba_points)
      printf("dist_atten_textured_rgba_points\n");
   else if (ctx->Driver.PointsFunc == dist_atten_antialiased_rgba_points)
      printf("dist_atten_antialiased_rgba_points\n");
   else if (!ctx->Driver.PointsFunc)
      printf("NULL\n");
   else
      printf("Driver func %p\n", ctx->Driver.PointsFunc);
}
#endif


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
            if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D ||
                ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) {
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

   /*_mesa_print_points_function(ctx);*/
}

