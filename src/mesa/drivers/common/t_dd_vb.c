/* $Id: t_dd_vb.c,v 1.7 2001/03/17 17:31:42 keithw Exp $ */

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

#if (HAVE_HW_VIEWPORT)
#define UNVIEWPORT_VARS
#define UNVIEWPORT_X(x) x
#define UNVIEWPORT_Y(x) x
#define UNVIEWPORT_Z(x) x
#endif

#ifndef LOCALVARS
#define LOCALVARS
#endif

/* These don't need to be duplicated, but there's currently nowhere
 * really convenient to put them.  Need to build some actual .o files in
 * this directory?
 */
static void copy_pv_rgba4_spec5( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   LOCALVARS   
   GLubyte *verts = GET_VERTEX_STORE();
   GLuint shift = GET_VERTEX_STRIDE_SHIFT();
   GLuint *dst = (GLuint *)(verts + (edst << shift));
   GLuint *src = (GLuint *)(verts + (esrc << shift));
   dst[4] = src[4];
   dst[5] = src[5];
}

static void copy_pv_rgba4( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   LOCALVARS
   GLubyte *verts = GET_VERTEX_STORE();
   GLuint shift = GET_VERTEX_STRIDE_SHIFT();
   GLuint *dst = (GLuint *)(verts + (edst << shift));
   GLuint *src = (GLuint *)(verts + (esrc << shift));
   dst[4] = src[4];
}

static void copy_pv_rgba3( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   LOCALVARS
   GLubyte *verts = GET_VERTEX_STORE();
   GLuint shift = GET_VERTEX_STRIDE_SHIFT();
   GLuint *dst = (GLuint *)(verts + (edst << shift));
   GLuint *src = (GLuint *)(verts + (esrc << shift));
   dst[3] = src[3];
}


void TAG(translate_vertex)(GLcontext *ctx,
			   const VERTEX *src,
			   SWvertex *dst)
{
   LOCALVARS
   GLuint format = GET_VERTEX_FORMAT();
   GLfloat *s = ctx->Viewport._WindowMap.m;
   UNVIEWPORT_VARS;

   if (format == TINY_VERTEX_FORMAT) {
      if (HAVE_HW_VIEWPORT) {
	 dst->win[0] = s[0]  * src->v.x + s[12];
	 dst->win[1] = s[5]  * src->v.y + s[13];
	 dst->win[2] = s[10] * src->v.z + s[14];
	 dst->win[3] = 1.0;
      } else {
	 dst->win[0] = UNVIEWPORT_X( src->v.x );
	 dst->win[1] = UNVIEWPORT_Y( src->v.y );
	 dst->win[2] = UNVIEWPORT_Z( src->v.z );
	 dst->win[3] = 1.0;
      }

      dst->color[0] = src->tv.color.red;
      dst->color[1] = src->tv.color.green;
      dst->color[2] = src->tv.color.blue;
      dst->color[3] = src->tv.color.alpha;
   }
   else {
      GLfloat oow = (HAVE_HW_DIVIDE) ? 1.0 / src->v.w : src->v.w;

      if (HAVE_HW_VIEWPORT) {
	 if (HAVE_HW_DIVIDE) {
	    dst->win[0] = s[0]  * src->v.x * oow + s[12];
	    dst->win[1] = s[5]  * src->v.y * oow + s[13];
	    dst->win[2] = s[10] * src->v.z * oow + s[14];
	    dst->win[3] = oow;
	 } else {
	    dst->win[0] = s[0]  * src->v.x + s[12];
	    dst->win[1] = s[5]  * src->v.y + s[13];
	    dst->win[2] = s[10] * src->v.z + s[14];
	    dst->win[3] = oow;
	 }
      } else {
	 dst->win[0] = UNVIEWPORT_X( src->v.x );
	 dst->win[1] = UNVIEWPORT_Y( src->v.y );
	 dst->win[2] = UNVIEWPORT_Z( src->v.z );
	 dst->win[3] = oow;
      }

      dst->color[0] = src->v.color.red;
      dst->color[1] = src->v.color.green;
      dst->color[2] = src->v.color.blue;
      dst->color[3] = src->v.color.alpha;

      dst->specular[0] = src->v.specular.red;
      dst->specular[1] = src->v.specular.green;
      dst->specular[2] = src->v.specular.blue;

      dst->fog = src->v.color.alpha/255.0;

      if (HAVE_PTEX_VERTICES &&
	  ((HAVE_TEX2_VERTICES && format == PROJ_TEX3_VERTEX_FORMAT) ||
	   (format == PROJ_TEX1_VERTEX_FORMAT))) {

	 dst->texcoord[0][0] = src->pv.u0;
	 dst->texcoord[0][1] = src->pv.v0;
	 dst->texcoord[0][3] = src->pv.q0;

	 dst->texcoord[1][0] = src->pv.u1;
	 dst->texcoord[1][1] = src->pv.v1;
	 dst->texcoord[1][3] = src->pv.q1;

	 if (HAVE_TEX2_VERTICES) {
	    dst->texcoord[2][0] = src->pv.u2;
	    dst->texcoord[2][1] = src->pv.v2;
	    dst->texcoord[2][3] = src->pv.q2;
	 }

	 if (HAVE_TEX3_VERTICES) {
	    dst->texcoord[3][0] = src->pv.u3;
	    dst->texcoord[3][1] = src->pv.v3;
	    dst->texcoord[3][3] = src->pv.q3;
	 }
      }
      else {
	 dst->texcoord[0][0] = src->v.u0;
	 dst->texcoord[0][1] = src->v.v0;
	 dst->texcoord[0][3] = 1.0;

	 dst->texcoord[1][0] = src->v.u1;
	 dst->texcoord[1][1] = src->v.v1;
	 dst->texcoord[1][3] = 1.0;

	 if (HAVE_TEX2_VERTICES) {
	    dst->texcoord[2][0] = src->v.u2;
	    dst->texcoord[2][1] = src->v.v2;
	    dst->texcoord[2][3] = 1.0;
	 }

	 if (HAVE_TEX3_VERTICES) {
	    dst->texcoord[3][0] = src->v.u3;
	    dst->texcoord[3][1] = src->v.v3;
	    dst->texcoord[3][3] = 1.0;
	 }
      }
   }

   dst->pointSize = ctx->Point._Size;
}



void TAG(print_vertex)( GLcontext *ctx, const VERTEX *v )
{
   LOCALVARS
   GLuint format = GET_VERTEX_FORMAT();

   if (format == TINY_VERTEX_FORMAT) {
      fprintf(stderr, "x %f y %f z %f\n", v->v.x, v->v.y, v->v.z);
      fprintf(stderr, "r %d g %d b %d a %d\n",
	      v->tv.color.red,
	      v->tv.color.green,
	      v->tv.color.blue,
	      v->tv.color.alpha);
   }
   else {
      fprintf(stderr, "x %f y %f z %f oow %f\n",
	      v->v.x, v->v.y, v->v.z, v->v.w);
      fprintf(stderr, "r %d g %d b %d a %d\n",
	      v->v.color.red,
	      v->v.color.green,
	      v->v.color.blue,
	      v->v.color.alpha);
   }

   fprintf(stderr, "\n");
}


#undef TAG
