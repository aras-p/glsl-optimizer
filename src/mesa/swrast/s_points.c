/* $Id: s_points.c,v 1.2 2000/11/05 18:24:40 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.4
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


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mmath.h"
#include "texstate.h"
#include "vb.h"

#include "s_context.h"
#include "s_feedback.h"
#include "s_pb.h"
#include "s_points.h"
#include "s_span.h"


/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/


/*
 * There are 3 pairs (RGBA, CI) of point rendering functions:
 *   1. simple:  size=1 and no special rasterization functions (fastest)
 *   2. size1:  size=1 and any rasterization functions
 *   3. general:  any size and rasterization functions (slowest)
 *
 */





/*
 * CI points with size == 1.0
 */
static void
size1_ci_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint *pbx = PB->x, *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfixed *pbfog = PB->fog;
   GLuint *pbi = PB->index;
   GLuint pbcount = PB->count;

   pbx[pbcount] = (GLint) vert->win[0];
   pby[pbcount] = (GLint) vert->win[1];
   pbz[pbcount] = (GLint) vert->win[2];
   pbfog[pbcount] = FloatToFixed(vert->fog);
   pbi[pbcount] = vert->index;

   PB->count++;
   PB_CHECK_FLUSH(ctx, PB);
}



/*
 * RGBA points with size == 1.0
 */
static void
size1_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;

   GLint x = (GLint)  vert->win[0];
   GLint y = (GLint)  vert->win[1];
   GLint z = (GLint) (vert->win[2]);
   GLfixed fog = FloatToFixed( vert->fog );
   GLubyte red   = vert->color[0];
   GLubyte green = vert->color[1];
   GLubyte blue  = vert->color[2];
   GLubyte alpha = vert->color[3];

   PB_WRITE_RGBA_PIXEL( PB, x, y, z, fog, red, green, blue, alpha );
   PB_CHECK_FLUSH(ctx, PB);
}



/*
 * General CI points.
 */
static void
general_ci_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLint isize = (GLint) (ctx->Point.Size + 0.5F);
   GLint radius = isize >> 1;

   GLint x0, x1, y0, y1;
   GLint ix, iy;

   GLint x = (GLint)  vert->win[0];
   GLint y = (GLint)  vert->win[1];
   GLint z = (GLint) (vert->win[2]);

   GLfixed fog = FloatToFixed( vert->fog );

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

   PB_SET_INDEX( PB, vert->index );

   for (iy = y0; iy <= y1; iy++) {
      for (ix = x0; ix <= x1; ix++) {
	 PB_WRITE_PIXEL( PB, ix, iy, z, fog );
      }
   }
   PB_CHECK_FLUSH(ctx,PB);
}


/*
 * General RGBA points.
 */
static void
general_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint isize = (GLint) (ctx->Point.Size + 0.5F);
   GLint radius = isize >> 1;

   GLint x0, x1, y0, y1;
   GLint ix, iy;

   GLint x = (GLint)  vert->win[0];
   GLint y = (GLint)  vert->win[1];
   GLint z = (GLint) (vert->win[2]);

   GLfixed fog = FloatToFixed( vert->fog );

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
		 vert->color[0],
		 vert->color[1],
		 vert->color[2],
		 vert->color[3] );

   for (iy = y0; iy <= y1; iy++) {
      for (ix = x0; ix <= x1; ix++) {
	 PB_WRITE_PIXEL( PB, ix, iy, z, fog );
      }
   }
   PB_CHECK_FLUSH(ctx,PB);
}




/*
 * Textured RGBA points.
 */
static void
textured_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;

   GLint x0, x1, y0, y1;
   GLint ix, iy, radius;
   GLint red, green, blue, alpha;
   GLfloat s, t, u;

   GLint x = (GLint)  vert->win[0];
   GLint y = (GLint)  vert->win[1];
   GLint z = (GLint) (vert->win[2]);
   GLint isize = (GLint) (ctx->Point.Size + 0.5F);

   GLfixed fog = FloatToFixed( vert->fog );

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

   red   = vert->color[0];
   green = vert->color[1];
   blue  = vert->color[2];
   alpha = vert->color[3];

   if (vert->texcoord[0][3] != 1.0) {
      s = vert->texcoord[0][0]/vert->texcoord[0][3];
      t = vert->texcoord[0][1]/vert->texcoord[0][3];
      u = vert->texcoord[0][2]/vert->texcoord[0][3];
   } else {
      s = vert->texcoord[0][0];
      t = vert->texcoord[0][1];
      u = vert->texcoord[0][2];
   }

   for (iy = y0; iy <= y1; iy++) {
      for (ix = x0; ix <= x1; ix++) {
	 PB_WRITE_TEX_PIXEL( PB, ix, iy, z, fog, red, green, blue, alpha,
			     s, t, u );
      }
   }

   PB_CHECK_FLUSH(ctx, PB);
}



/*
 * Multitextured RGBA points.
 */
static void
multitextured_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;

   const GLint red   = vert->color[0];
   const GLint green = vert->color[1];
   const GLint blue  = vert->color[2];
   const GLint alpha = vert->color[3];
   const GLint sRed   = vert->specular[0];
   const GLint sGreen = vert->specular[1];
   const GLint sBlue  = vert->specular[2];
   const GLint x = (GLint)  vert->win[0];
   const GLint y = (GLint)  vert->win[1];
   const GLint z = (GLint) (vert->win[2]);
   GLint x0, x1, y0, y1;
   GLint ix, iy;
   GLfloat texcoord[MAX_TEXTURE_UNITS][4];
   GLint radius, u;
   GLint isize = (GLint) (ctx->Point.Size + 0.5F);

   GLfixed fog = FloatToFixed( vert->fog );

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

   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
      if (ctx->Texture.Unit[u]._ReallyEnabled) {
	 if (vert->texcoord[u][3] != 1.0) {
	    texcoord[u][0] = vert->texcoord[u][0] /
	       vert->texcoord[u][3];
	    texcoord[u][1] = vert->texcoord[u][1] /
	       vert->texcoord[u][3];
	    texcoord[u][2] = vert->texcoord[u][2] /
	       vert->texcoord[u][3];
	 }
	 else {
	    texcoord[u][0] = vert->texcoord[u][0];
	    texcoord[u][1] = vert->texcoord[u][1];
	    texcoord[u][2] = vert->texcoord[u][2];
	 }
      }
   }

   for (iy = y0; iy <= y1; iy++) {
      for (ix = x0; ix <= x1; ix++) {
	 PB_WRITE_MULTITEX_SPEC_PIXEL( PB, ix, iy, z, fog,
				       red, green, blue, alpha,
				       sRed, sGreen, sBlue,
				       texcoord );
      }
   }
   PB_CHECK_FLUSH(ctx, PB);
}


/*
 * NOTES on aa point rasterization:
 *
 * Let d = distance of fragment center from vertex.
 * if d < rmin2 then
 *    fragment has 100% coverage
 * else if d > rmax2 then
 *    fragment has 0% coverage
 * else
 *    fragement has % coverage = (d - rmin2) / (rmax2 - rmin2)
 */


/*
 * Antialiased points with or without texture mapping.
 */
static void
antialiased_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLfloat radius = ctx->Point.Size * 0.5F;
   const GLfloat rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
   const GLfloat rmax = radius + 0.7071F;
   const GLfloat rmin2 = MAX2(0.0, rmin * rmin);
   const GLfloat rmax2 = rmax * rmax;
   const GLfloat cscale = 256.0F / (rmax2 - rmin2);

   if (ctx->Texture._ReallyEnabled) {
      GLint x, y;
      GLfloat vx = vert->win[0];
      GLfloat vy = vert->win[1];
      const GLint xmin = (GLint) (vx - radius);
      const GLint xmax = (GLint) (vx + radius);
      const GLint ymin = (GLint) (vy - radius);
      const GLint ymax = (GLint) (vy + radius);
      const GLint z = (GLint) (vert->win[2]);
      const GLint red   = vert->color[0];
      const GLint green = vert->color[1];
      const GLint blue  = vert->color[2];
      GLfloat texcoord[MAX_TEXTURE_UNITS][4];
      GLint u, alpha;

      GLfixed fog = FloatToFixed( vert->fog );

      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
	 if (ctx->Texture.Unit[u]._ReallyEnabled) {
	    if (texcoord[u][3] != 1.0) {
	       texcoord[u][0] = (vert->texcoord[u][0] /
				 vert->texcoord[u][3]);
	       texcoord[u][1] = (vert->texcoord[u][1] /
				 vert->texcoord[u][3]);
	       texcoord[u][2] = (vert->texcoord[u][2] /
				 vert->texcoord[u][3]);
	    } 
	    else {
	       texcoord[u][0] = vert->texcoord[u][0];
	       texcoord[u][1] = vert->texcoord[u][1];
	       texcoord[u][2] = vert->texcoord[u][2];
	    }
	 }
      }

      /* translate by a half pixel to simplify math below */
      vx -= 0.5F;
      vx -= 0.5F;

      for (y = ymin; y <= ymax; y++) {
	 for (x = xmin; x <= xmax; x++) {
	    const GLfloat dx = x - vx;
	    const GLfloat dy = y - vy;
	    const GLfloat dist2 = dx*dx + dy*dy;
	    if (dist2 < rmax2) {
	       alpha = vert->color[3];
	       if (dist2 >= rmin2) {
		  GLint coverage = (GLint) (256.0F - (dist2 - rmin2) * cscale);
		  /* coverage is in [0,256] */
		  alpha = (alpha * coverage) >> 8;
	       }
	       if (ctx->Texture._MultiTextureEnabled) {
		  PB_WRITE_MULTITEX_PIXEL( PB, x,y,z, fog,
					   red, green, blue, 
					   alpha, texcoord );
	       }
	       else {
		  PB_WRITE_TEX_PIXEL( PB, x,y,z, fog,
				      red, green, blue, alpha,
				      texcoord[0][0],
				      texcoord[0][1],
				      texcoord[0][2] );
	       }
	    }
	 }
      }

      PB_CHECK_FLUSH(ctx,PB);
   }
   else {
      /* Not texture mapped */
      const GLint xmin = (GLint) (vert->win[0] - 0.0 - radius);
      const GLint xmax = (GLint) (vert->win[0] - 0.0 + radius);
      const GLint ymin = (GLint) (vert->win[1] - 0.0 - radius);
      const GLint ymax = (GLint) (vert->win[1] - 0.0 + radius);
      const GLint red   = vert->color[0];
      const GLint green = vert->color[1];
      const GLint blue  = vert->color[2];
      const GLint z = (GLint) (vert->win[2]);
      GLint x, y;

      GLfixed fog = FloatToFixed( vert->fog );

      /*
	printf("point %g, %g\n", vert->win[0], vert->win[1]);
	printf("%d..%d X %d..%d\n", xmin, xmax, ymin, ymax);
      */
      for (y = ymin; y <= ymax; y++) {
	 for (x = xmin; x <= xmax; x++) {
	    const GLfloat dx = x + 0.5F - vert->win[0];
	    const GLfloat dy = y + 0.5F - vert->win[1];
	    const GLfloat dist2 = dx*dx + dy*dy;
	    if (dist2 < rmax2) {
	       GLint alpha = vert->color[3];
	       if (dist2 >= rmin2) {
		  GLint coverage = (GLint) (256.0F - (dist2 - rmin2) * cscale);
		  /* coverage is in [0,256] */
		  alpha = (alpha * coverage) >> 8;
	       }
	       PB_WRITE_RGBA_PIXEL(PB, x, y, z, fog,
				   red, green, blue, alpha);
	    }
	 }
      }
      PB_CHECK_FLUSH(ctx,PB);
   }
}



/* Definition of the functions for GL_EXT_point_parameters */

/* Calculates the distance attenuation formula of a vector of points in
 * eye space coordinates 
 */
static GLfloat attenuation_distance(const GLcontext *ctx, const GLfloat *pos)
{
   GLfloat dist = GL_SQRT(pos[0]*pos[0]+pos[1]*pos[1]+pos[2]*pos[2]);
   return 1.0F / (ctx->Point.Params[0] +
		  dist * (ctx->Point.Params[1] +
			  dist * ctx->Point.Params[2]));
}





/*
 * Distance Attenuated General CI points.
 */
static void
dist_atten_general_ci_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLfloat psize = ctx->Point.Size;
   GLfloat dist = attenuation_distance( ctx, vert->eye );
   GLint x0, x1, y0, y1;
   GLint ix, iy;
   GLint isize, radius;
   GLint x = (GLint)  vert->win[0];
   GLint y = (GLint)  vert->win[1];
   GLint z = (GLint) (vert->win[2]);
   GLfloat dsize = psize * dist;

   GLfixed fog = FloatToFixed( vert->fog );

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

   PB_SET_INDEX( PB, vert->index );

   for (iy=y0;iy<=y1;iy++) {
      for (ix=x0;ix<=x1;ix++) {
	 PB_WRITE_PIXEL( PB, ix, iy, z, fog );
      }
   }
   PB_CHECK_FLUSH(ctx,PB);
}

/*
 * Distance Attenuated General RGBA points.
 */
static void
dist_atten_general_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLfloat psize = ctx->Point.Size;
   GLfloat dist = attenuation_distance( ctx, vert->eye );
   GLint x0, x1, y0, y1;
   GLint ix, iy;
   GLint isize, radius;
   GLint x = (GLint)  vert->win[0];
   GLint y = (GLint)  vert->win[1];
   GLint z = (GLint) (vert->win[2]);
   GLfloat dsize=psize*dist;
   GLchan alpha;
   GLfixed fog = FloatToFixed( vert->fog );

   if (dsize >= ctx->Point.Threshold) {
      isize = (GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
      alpha = vert->color[3];
   }
   else {
      isize = (GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
      dsize /= ctx->Point.Threshold;
      alpha = (GLint) (vert->color[3]* (dsize*dsize));
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
		 vert->color[0],
		 vert->color[1],
		 vert->color[2],
		 alpha );

   for (iy = y0; iy <= y1; iy++) {
      for (ix = x0; ix <= x1; ix++) {
	 PB_WRITE_PIXEL( PB, ix, iy, z, fog );
      }
   }
   PB_CHECK_FLUSH(ctx,PB);
}

/*
 *  Distance Attenuated Textured RGBA points.
 */
static void
dist_atten_textured_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLfloat psize = ctx->Point.Size;
   GLfloat dist = attenuation_distance( ctx, vert->eye );

   const GLint x = (GLint)  vert->win[0];
   const GLint y = (GLint)  vert->win[1];
   const GLint z = (GLint) (vert->win[2]);
   const GLint red   = vert->color[0];
   const GLint green = vert->color[1];
   const GLint blue  = vert->color[2];
   GLfloat texcoord[MAX_TEXTURE_UNITS][4];
   GLint x0, x1, y0, y1;
   GLint ix, iy, alpha, u;
   GLint isize, radius;
   GLfloat dsize = psize*dist;

   GLfixed fog = FloatToFixed( vert->fog );

   /* compute point size and alpha */
   if (dsize >= ctx->Point.Threshold) {
      isize = (GLint) (MIN2(dsize, ctx->Point.MaxSize) + 0.5F);
      alpha = vert->color[3];
   }
   else {
      isize = (GLint) (MAX2(ctx->Point.Threshold, ctx->Point.MinSize) + 0.5F);
      dsize /= ctx->Point.Threshold;
      alpha = (GLint) (vert->color[3] * (dsize * dsize));
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

   /* get texture coordinates */
   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
      if (ctx->Texture.Unit[u]._ReallyEnabled) {
	 if (texcoord[u][3] != 1.0) {
	    texcoord[u][0] = vert->texcoord[u][0] /
	       vert->texcoord[u][3];
	    texcoord[u][1] = vert->texcoord[u][1] /
	       vert->texcoord[u][3];
	    texcoord[u][2] = vert->texcoord[u][2] /
	       vert->texcoord[u][3];
	 } 
	 else {
	    texcoord[u][0] = vert->texcoord[u][0];
	    texcoord[u][1] = vert->texcoord[u][1];
	    texcoord[u][2] = vert->texcoord[u][2];
	 }
      }
   }

   for (iy = y0; iy <= y1; iy++) {
      for (ix = x0; ix <= x1; ix++) {
	 if (ctx->Texture._MultiTextureEnabled) {
	    PB_WRITE_MULTITEX_PIXEL( PB, ix, iy, z, fog,
				     red, green, blue, alpha,
				     texcoord );
	 }
	 else {
	    PB_WRITE_TEX_PIXEL( PB, ix, iy, z, fog,
				red, green, blue, alpha,
				texcoord[0][0],
				texcoord[0][1],
				texcoord[0][2] );
	 }
      }
   }
   PB_CHECK_FLUSH(ctx,PB);
}

/*
 * Distance Attenuated Antialiased points with or without texture mapping.
 */
static void
dist_atten_antialiased_rgba_point( GLcontext *ctx, SWvertex *vert )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLfloat psize = ctx->Point.Size;
   GLfloat dist = attenuation_distance( ctx, vert->eye );

   if (ctx->Texture._ReallyEnabled) {
      GLfloat radius, rmin, rmax, rmin2, rmax2, cscale, alphaf;
      GLint xmin, ymin, xmax, ymax;
      GLint x, y, z;
      GLint red, green, blue, alpha;
      GLfloat texcoord[MAX_TEXTURE_UNITS][4];
      GLfloat dsize = psize * dist;
      GLint u;

      GLfixed fog = FloatToFixed( vert->fog );

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
      rmin2 = MAX2(0.0, rmin * rmin);
      rmax2 = rmax * rmax;
      cscale = 256.0F / (rmax2 - rmin2);

      xmin = (GLint) (vert->win[0] - radius);
      xmax = (GLint) (vert->win[0] + radius);
      ymin = (GLint) (vert->win[1] - radius);
      ymax = (GLint) (vert->win[1] + radius);
      z = (GLint) (vert->win[2]);

      red   = vert->color[0];
      green = vert->color[1];
      blue  = vert->color[2];
	 
      /* get texture coordinates */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
	 if (ctx->Texture.Unit[u]._ReallyEnabled) {
	    if (vert->texcoord[u][3] != 1.0 && vert->texcoord[u][3] != 0.0) {
	       texcoord[u][0] = vert->texcoord[u][0] / vert->texcoord[u][3];
	       texcoord[u][1] = vert->texcoord[u][1] / vert->texcoord[u][3];
	       texcoord[u][2] = vert->texcoord[u][2] / vert->texcoord[u][3];
	    } 
	    else {
	       texcoord[u][0] = vert->texcoord[u][0];
	       texcoord[u][1] = vert->texcoord[u][1];
	       texcoord[u][2] = vert->texcoord[u][2];
	    }
	 }
      }

      for (y = ymin; y <= ymax; y++) {
	 for (x = xmin; x <= xmax; x++) {
	    const GLfloat dx = x + 0.5F - vert->win[0];
	    const GLfloat dy = y + 0.5F - vert->win[1];
	    const GLfloat dist2 = dx*dx + dy*dy;
	    if (dist2 < rmax2) {
	       alpha = vert->color[3];
	       if (dist2 >= rmin2) {
		  GLint coverage = (GLint) (256.0F - (dist2 - rmin2) * cscale);
		  /* coverage is in [0,256] */
		  alpha = (alpha * coverage) >> 8;
	       }
	       alpha = (GLint) (alpha * alphaf);
	       if (ctx->Texture._MultiTextureEnabled) {
		  PB_WRITE_MULTITEX_PIXEL( PB, x, y, z, fog,
					   red, green, blue, alpha,
					   texcoord );
	       }
	       else {
		  PB_WRITE_TEX_PIXEL( PB, x,y,z, fog,
				      red, green, blue, alpha,
				      texcoord[0][0],
				      texcoord[0][1],
				      texcoord[0][2] );
	       }
	    }
	 }
      }
      PB_CHECK_FLUSH(ctx,PB);
   }
   else {
      /* Not texture mapped */
      GLfloat radius, rmin, rmax, rmin2, rmax2, cscale, alphaf;
      GLint xmin, ymin, xmax, ymax;
      GLint x, y, z;
      GLfixed fog;
      GLint red, green, blue, alpha;
      GLfloat dsize = psize * dist;

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
      rmin2 = MAX2(0.0, rmin * rmin);
      rmax2 = rmax * rmax;
      cscale = 256.0F / (rmax2 - rmin2);

      xmin = (GLint) (vert->win[0] - radius);
      xmax = (GLint) (vert->win[0] + radius);
      ymin = (GLint) (vert->win[1] - radius);
      ymax = (GLint) (vert->win[1] + radius);
      z = (GLint) (vert->win[2]);

      fog = FloatToFixed( vert->fog );

      red   = vert->color[0];
      green = vert->color[1];
      blue  = vert->color[2];

      for (y = ymin; y <= ymax; y++) {
	 for (x = xmin; x <= xmax; x++) {
	    const GLfloat dx = x + 0.5F - vert->win[0];
	    const GLfloat dy = y + 0.5F - vert->win[1];
	    const GLfloat dist2 = dx * dx + dy * dy;
	    if (dist2 < rmax2) {
	       alpha = vert->color[3];
	       if (dist2 >= rmin2) {
		  GLint coverage = (GLint) (256.0F - (dist2 - rmin2) * cscale);
		  /* coverage is in [0,256] */
		  alpha = (alpha * coverage) >> 8;
	       }
	       alpha = (GLint) (alpha * alphaf);
	       PB_WRITE_RGBA_PIXEL(PB, x, y, z, fog, 
				   red, green, blue, alpha);
	    }
	 }
      }
      PB_CHECK_FLUSH(ctx,PB);
   }
}


#ifdef DEBUG
void
_mesa_print_point_function(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   printf("Point Func == ");
   if (swrast->Point == size1_ci_point)
      printf("size1_ci_point\n");
   else if (swrast->Point == size1_rgba_point)
      printf("size1_rgba_point\n");
   else if (swrast->Point == general_ci_point)
      printf("general_ci_point\n");
   else if (swrast->Point == general_rgba_point)
      printf("general_rgba_point\n");
   else if (swrast->Point == textured_rgba_point)
      printf("textured_rgba_point\n");
   else if (swrast->Point == multitextured_rgba_point)
      printf("multitextured_rgba_point\n");
   else if (swrast->Point == antialiased_rgba_point)
      printf("antialiased_rgba_point\n");
   else if (swrast->Point == dist_atten_general_ci_point)
      printf("dist_atten_general_ci_point\n");
   else if (swrast->Point == dist_atten_general_rgba_point)
      printf("dist_atten_general_rgba_point\n");
   else if (swrast->Point == dist_atten_textured_rgba_point)
      printf("dist_atten_textured_rgba_point\n");
   else if (swrast->Point == dist_atten_antialiased_rgba_point)
      printf("dist_atten_antialiased_rgba_point\n");
   else if (!swrast->Point)
      printf("NULL\n");
   else
      printf("Driver func %p\n", swrast->Point);
}
#endif


/*
 * Examine the current context to determine which point drawing function
 * should be used.
 */
void 
_swrast_choose_point( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLboolean rgbmode = ctx->Visual.RGBAflag;

   if (ctx->RenderMode==GL_RENDER) {
      if (!ctx->Point._Attenuated) {
         if (ctx->Point.SmoothFlag && rgbmode) {
            swrast->Point = antialiased_rgba_point;
         }
         else if (ctx->Texture._ReallyEnabled) {
            if (ctx->Texture._MultiTextureEnabled ||
                ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR ||
		ctx->Fog.ColorSumEnabled) {
	       swrast->Point = multitextured_rgba_point;
            }
            else {
               swrast->Point = textured_rgba_point;
            }
         }
         else if (ctx->Point.Size==1.0) {
            /* size=1, any raster ops */
            if (rgbmode)
               swrast->Point = size1_rgba_point;
            else
               swrast->Point = size1_ci_point;
         }
         else {
	    /* every other kind of point rendering */
            if (rgbmode)
               swrast->Point = general_rgba_point;
            else
               swrast->Point = general_ci_point;
         }
      } 
      else if(ctx->Point.SmoothFlag && rgbmode) {
         swrast->Point = dist_atten_antialiased_rgba_point;
      }
      else if (ctx->Texture._ReallyEnabled) {
         swrast->Point = dist_atten_textured_rgba_point;
      } 
      else {
         /* every other kind of point rendering */
         if (rgbmode)
            swrast->Point = dist_atten_general_rgba_point;
         else
            swrast->Point = dist_atten_general_ci_point;
     }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      swrast->Point = gl_feedback_point;
   }
   else {
      /* GL_SELECT mode */
      swrast->Point = gl_select_point;
   }

   /*_mesa_print_points_function(ctx);*/
}

