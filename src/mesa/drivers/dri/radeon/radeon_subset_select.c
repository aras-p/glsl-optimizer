/**
 * \file radeon_subset_select.c
 * \brief Selection.
 */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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

/* $Id: radeon_subset_select.c,v 1.2 2003/08/22 20:11:45 brianp Exp $ */


#include "glheader.h"
#include "imports.h"
#include "context.h"
/*#include "mmath.h"*/
#include "mtypes.h"
#include "enums.h"
#include "glapi.h"
#include "feedback.h"

#include "radeon_context.h"
#include "radeon_subset.h"

/**
 * \brief Vertex.
 */
typedef struct {
   struct { GLfloat x, y, z, w; } pos;      /**< \brief position */
   struct { GLfloat x, y, z, w; } eyePos;   /**< \brief position, eye coordinates */
   struct { GLfloat x, y, z, w; } clipPos;  /**< \brief clipped coordinates */
   struct { GLfloat x, y, z, w; } winPos;   /**< \brief position, windows coordinates */
   struct { GLfloat s, t; } texCoord;       /**< \brief texture coordinates */
   struct { GLfloat r, g, b, a; } color;    /**< \brief color */
} vertex;


/**
 * \brief Vertex buffer.
 */
static struct select_vb_t {
   GLuint    vCount;		/**< \brief vertex count */
   vertex  vBuffer[4];		/**< \brief vertex buffer */
   GLboolean lineReset;
   GLboolean partialLineLoop;	/**< \brief whether we are in a middle of a line loop */
} vb;




/**********************************************************************/
/** \name        Vertex Transformation and Clipping                   */
/**********************************************************************/
/*@{*/

/**
 * \brief Transform a point (column vector) by a matrix:  Q = M * P.
 *
 * \param Q destination point.
 * \param P source point.
 * \param M transformation matrix.
 */
#define TRANSFORM_POINT( Q, M, P )				\
   Q.x = M[0] * P.x + M[4] * P.y + M[8] *  P.z + M[12] * P.w;	\
   Q.y = M[1] * P.x + M[5] * P.y + M[9] *  P.z + M[13] * P.w;	\
   Q.z = M[2] * P.x + M[6] * P.y + M[10] * P.z + M[14] * P.w;	\
   Q.w = M[3] * P.x + M[7] * P.y + M[11] * P.z + M[15] * P.w;

/**
 * \brief Clip coord to window coord mapping.
 *
 * \param Q destination point.
 * \param P source point.
 * \param VP view port.
 */
#define MAP_POINT( Q, P, VP )                                      \
   Q.x = (GLfloat) (((P.x / P.w) + 1.0) * VP.Width / 2.0 + VP.X);  \
   Q.y = (GLfloat) (((P.y / P.w) + 1.0) * VP.Height / 2.0 + VP.Y);  \
   Q.z = (GLfloat) (((P.z / P.w) + 1.0) * (VP.Far - VP.Near) / 2.0 + VP.Near);\
   Q.w = (GLfloat) P.w;


/**
 * \brief Linear interpolation:  (1 - T) * A + T * B.
 *
 * \param T interpolation factor.
 * \param A first value.
 * \param B second value.
 * \result interpolated value.
 */
#define INTERPOLATE(T, A, B)   ((A) + ((B) - (A)) * (T))



/**
 * \brief Interpolate vertex position, color, texcoords, etc.
 *
 * \param t interpolation factor.
 * \param v0 first vertex.
 * \param v1 second vertex.
 * \param vOut output vertex.
 * 
 * Uses the #INTERPOLATE macro for all the interpolation of all elements.
 */
static void
interpolate_vertex(GLfloat t, const vertex *v0, const vertex *v1, 
		   vertex *vOut)
{
   vOut->eyePos.x = INTERPOLATE(t, v0->eyePos.x, v1->eyePos.x);
   vOut->eyePos.y = INTERPOLATE(t, v0->eyePos.y, v1->eyePos.y);
   vOut->eyePos.z = INTERPOLATE(t, v0->eyePos.z, v1->eyePos.z);
   vOut->eyePos.w = INTERPOLATE(t, v0->eyePos.w, v1->eyePos.w);

   vOut->clipPos.x = INTERPOLATE(t, v0->clipPos.x, v1->clipPos.x);
   vOut->clipPos.y = INTERPOLATE(t, v0->clipPos.y, v1->clipPos.y);
   vOut->clipPos.z = INTERPOLATE(t, v0->clipPos.z, v1->clipPos.z);
   vOut->clipPos.w = INTERPOLATE(t, v0->clipPos.w, v1->clipPos.w);

   vOut->color.r = INTERPOLATE(t, v0->color.r, v1->color.r);
   vOut->color.g = INTERPOLATE(t, v0->color.g, v1->color.g);
   vOut->color.b = INTERPOLATE(t, v0->color.b, v1->color.b);
   vOut->color.a = INTERPOLATE(t, v0->color.a, v1->color.a);

   vOut->texCoord.s = INTERPOLATE(t, v0->texCoord.s, v1->texCoord.s);
   vOut->texCoord.t = INTERPOLATE(t, v0->texCoord.t, v1->texCoord.t);
}




/*
 * Clip bit codes 
 */
#define CLIP_LEFT    1
#define CLIP_RIGHT   2
#define CLIP_BOTTOM  4
#define CLIP_TOP     8
#define CLIP_NEAR   16
#define CLIP_FAR    32


/**
 * \brief Apply view volume clip testing to a point.
 *
 * \param v point to test.
 * \return zero if visible, or the clip code mask, i.e., binary OR of a
 * combination of the #CLIP_LEFT, #CLIP_RIGHT, #CLIP_BOTTOM, #CLIP_TOP, #CLIP_NEAR,
 * #CLIP_FAR clip bit codes.
 */
static GLuint
clip_point(const vertex *v)
{
   GLuint mask = 0;
   if (v->clipPos.x > v->clipPos.w) mask |= CLIP_RIGHT;
   if (v->clipPos.x < -v->clipPos.w) mask |= CLIP_LEFT;
   if (v->clipPos.y > v->clipPos.w) mask |= CLIP_TOP;
   if (v->clipPos.y < -v->clipPos.w) mask |= CLIP_BOTTOM;
   if (v->clipPos.z > v->clipPos.w) mask |= CLIP_FAR;
   if (v->clipPos.z < -v->clipPos.w) mask |= CLIP_NEAR;
   return mask;
}


/**
 * \def GENERAL_CLIP
 * \brief Clipping utility macro.
 * 
 * We use 6 instances of this code in each of the clip_line() and
 * clip_polygon() to clip against the 6 planes.  For each plane, we define the
 * #OUTSIDE and #COMPUTE_INTERSECTION macros appropriately.
 */


/**
 * \brief Apply clipping to a line segment.
 *
 * \param v0in input start vertex
 * \param v1in input end vertesx
 * \param v0new output start vertex
 * \param v1new output end vertex
 *
 * \return GL_TRUE if the line segment is visible, or GL_FALSE if it is totally
 * clipped.
 *
 * \sa #GENERAL_CLIP.
 */
static GLboolean
clip_line(const vertex *v0in, const vertex *v1in,
	  vertex *v0new, vertex *v1new)
{
   vertex v0, v1, vNew;
   GLfloat dx, dy, dz, dw, t;
   GLuint code0, code1;

   code0 = clip_point(v0in);
   code1 = clip_point(v1in);
   if (code0 & code1)
      return GL_FALSE;  /* totally clipped */
   
   *v0new = *v0in;
   *v1new = *v1in;
   if (code0 == 0 && code1 == 0)
      return GL_TRUE;   /* no clipping needed */
   
   v0 = *v0in;
   v1 = *v1in;


#define GENERAL_CLIP                                                    \
   if (OUTSIDE(v0)) {                                                   \
      if (OUTSIDE(v1)) {                                                \
         /* both verts are outside ==> return 0 */                      \
         return 0;                                                      \
      }                                                                 \
      else {                                                            \
         /* v0 is outside, v1 is inside ==> clip */                     \
         COMPUTE_INTERSECTION( v1, v0, vNew )                           \
         interpolate_vertex(t, &v1, &v0, &vNew);                        \
         v0 = vNew;                                                     \
      }                                                                 \
   }                                                                    \
   else {                                                               \
      if (OUTSIDE(v1)) {                                                \
         /* v0 is inside, v1 is outside ==> clip */                     \
         COMPUTE_INTERSECTION( v0, v1, vNew )                           \
         interpolate_vertex(t, &v0, &v1, &vNew);                        \
         v1 = vNew;                                                     \
      }                                                                 \
      /* else both verts are inside ==> do nothing */                   \
   }

   /* Clip against +X side */
#define OUTSIDE(V)      (V.clipPos.x > V.clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dx = OUT.clipPos.x - IN.clipPos.x;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = (IN.clipPos.x - IN.clipPos.w) / (dw-dx);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /* Clip against -X side */
#define OUTSIDE(V)      (V.clipPos.x < -(V.clipPos.w))
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dx = OUT.clipPos.x - IN.clipPos.x;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = -(IN.clipPos.x + IN.clipPos.w) / (dw+dx);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /* Clip against +Y side */
#define OUTSIDE(V)      (V.clipPos.y > V.clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dy = OUT.clipPos.y - IN.clipPos.y;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = (IN.clipPos.y - IN.clipPos.w) / (dw-dy);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /* Clip against -Y side */
#define OUTSIDE(V)      (V.clipPos.y < -(V.clipPos.w))
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dy = OUT.clipPos.y - IN.clipPos.y;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = -(IN.clipPos.y + IN.clipPos.w) / (dw+dy);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /* Clip against +Z side */
#define OUTSIDE(V)      (V.clipPos.z > V.clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dz = OUT.clipPos.z - IN.clipPos.z;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = (IN.clipPos.z - IN.clipPos.w) / (dw-dz);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /* Clip against -Z side */
#define OUTSIDE(V)      (V.clipPos.z < -(V.clipPos.w))
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dz = OUT.clipPos.z - IN.clipPos.z;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = -(IN.clipPos.z + IN.clipPos.w) / (dw+dz);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

#undef GENERAL_CLIP

   *v0new = v0;
   *v1new = v1;
   return GL_TRUE;
}



/**
 * \brief Apply clipping to a polygon.
 *
 * \param vIn array of input vertices.
 * \param inCount number of input vertices
 * \param vOut array of output vertices.
 *
 * \return number of vertices in \p vOut.
 *
 * \sa #GENERAL_CLIP.
 */
static GLuint
clip_polygon(const vertex *vIn, unsigned int inCount, vertex *vOut)
{
   vertex inlist[20], outlist[20];
   GLfloat dx, dy, dz, dw, t;
   GLuint incount, outcount, previ, curri, result;
   const vertex *currVert, *prevVert;
   vertex *newVert;


#define GENERAL_CLIP(INCOUNT, INLIST, OUTCOUNT, OUTLIST)                \
   if (INCOUNT < 3)                                                     \
      return GL_FALSE;                                                  \
   previ = INCOUNT - 1;         /* let previous = last vertex */        \
   prevVert = INLIST + previ;                                           \
   OUTCOUNT = 0;                                                        \
   for (curri = 0; curri < INCOUNT; curri++) {                          \
      currVert = INLIST + curri;                                        \
      if (INSIDE(currVert)) {                                           \
         if (INSIDE(prevVert)) {                                        \
            /* both verts are inside ==> copy current to outlist */     \
            OUTLIST[OUTCOUNT] = *currVert;                              \
            OUTCOUNT++;                                                 \
         }                                                              \
         else {                                                         \
            newVert = OUTLIST + OUTCOUNT;                               \
            /* current is inside and previous is outside ==> clip */    \
            COMPUTE_INTERSECTION( currVert, prevVert, newVert )         \
            OUTCOUNT++;                                                 \
            /* Output current */                                        \
            OUTLIST[OUTCOUNT] = *currVert;                              \
            OUTCOUNT++;                                                 \
         }                                                              \
      }                                                                 \
      else {                                                            \
         if (INSIDE(prevVert)) {                                        \
            newVert = OUTLIST + OUTCOUNT;                               \
            /* current is outside and previous is inside ==> clip */    \
            COMPUTE_INTERSECTION( prevVert, currVert, newVert );        \
            OUTLIST[OUTCOUNT] = *newVert;                               \
            OUTCOUNT++;                                                 \
         }                                                              \
         /* else both verts are outside ==> do nothing */               \
      }                                                                 \
      /* let previous = current */                                      \
      previ = curri;                                                    \
      prevVert = currVert;                                              \
   }

/*
 * Clip against +X
 */
#define INSIDE(V)       (V->clipPos.x <= V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )              \
        dx = OUT->clipPos.x - IN->clipPos.x;              \
        dw = OUT->clipPos.w - IN->clipPos.w;              \
        t = (IN->clipPos.x - IN->clipPos.w) / (dw - dx);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(inCount, vIn, outcount, outlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against -X
 */
#define INSIDE(V)       (V->clipPos.x >= -V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )               \
        dx = OUT->clipPos.x - IN->clipPos.x;               \
        dw = OUT->clipPos.w - IN->clipPos.w;               \
        t = -(IN->clipPos.x + IN->clipPos.w) / (dw + dx);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(outcount, outlist, incount, inlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against +Y
 */
#define INSIDE(V)       (V->clipPos.y <= V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )              \
        dy = OUT->clipPos.y - IN->clipPos.y;              \
        dw = OUT->clipPos.w - IN->clipPos.w;              \
        t = (IN->clipPos.y - IN->clipPos.w) / (dw - dy);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(incount, inlist, outcount, outlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against -Y
 */
#define INSIDE(V)       (V->clipPos.y >= -V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )               \
        dy = OUT->clipPos.y - IN->clipPos.y;               \
        dw = OUT->clipPos.w - IN->clipPos.w;               \
        t = -(IN->clipPos.y + IN->clipPos.w) / (dw + dy);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(outcount, outlist, incount, inlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against +Z
 */
#define INSIDE(V)       (V->clipPos.z <= V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )              \
        dz = OUT->clipPos.z - IN->clipPos.z;              \
        dw = OUT->clipPos.w - IN->clipPos.w;              \
        t = (IN->clipPos.z - IN->clipPos.w) / (dw - dz);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(incount, inlist, outcount, outlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against -Z
 */
#define INSIDE(V)       (V->clipPos.z >= -V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )               \
        dz = OUT->clipPos.z - IN->clipPos.z;               \
        dw = OUT->clipPos.w - IN->clipPos.w;               \
        t = -(IN->clipPos.z + IN->clipPos.w) / (dw + dz);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(outcount, outlist, result, vOut)

#undef INSIDE
#undef COMPUTE_INTERSECTION

#undef GENERAL_CLIP

	return result;
}

/*@}*/



/**********************************************************************/
/** \name                    Selection                                */
/**********************************************************************/
/*@{*/

/**
 * \brief Select point.
 * 
 * \param v vertex.
 *
 * If the clipped point is visible then maps the vertex into window coordinates
 * and calls _mesa_update_hitflag().
 */
static void
select_point(const vertex *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (clip_point(v) == 0)
   {
      vertex c = *v;
      MAP_POINT(c.winPos, c.clipPos, ctx->Viewport);
      _mesa_update_hitflag(ctx, c.winPos.z);
   }
}

/**
 * \brief Select line.
 * 
 * \param v0 first vertex.
 * \param v1 second vertex.
 *
 * If the clipped line is visible then maps the vertices into window coordinates
 * and calls _mesa_update_hitflag().
 */
static void
select_line(const vertex *v0, const vertex *v1)
{
   GET_CURRENT_CONTEXT(ctx);
   vertex c0, c1;
   if (clip_line(v0, v1, &c0, &c1))
   {
      MAP_POINT(c0.winPos, c0.clipPos, ctx->Viewport);
      MAP_POINT(c1.winPos, c1.clipPos, ctx->Viewport);
      _mesa_update_hitflag(ctx, c0.winPos.z);
      _mesa_update_hitflag(ctx, c1.winPos.z);
   }
}

/**
 * \brief Select line.
 * 
 * \param v0 first vertex.
 * \param v1 second vertex.
 * \param v2 third vertex.
 *
 * If the clipped polygon is visible then maps the vertices into window
 * coordinates and calls _mesa_update_hitflag().
 */
static void
select_triangle(const vertex *v0,
		const vertex *v1,
		const vertex *v2)
{
   GET_CURRENT_CONTEXT(ctx);
   vertex vlist[3], vclipped[8];
   GLuint i, n;

   vlist[0] = *v0;
   vlist[1] = *v1;
   vlist[2] = *v2;
   n = clip_polygon(vlist, 3, vclipped);
   for (i = 0; i < n; i++) {
      MAP_POINT(vclipped[i].winPos, vclipped[i].clipPos, ctx->Viewport);
      _mesa_update_hitflag(ctx, vclipped[i].winPos.z);
   }
}

/**
 * \brief Set current vertex coordinates.
 *
 * \param x x vertex coordinate.
 * \param y y vertex coordinate.
 * \param z z vertex coordinate.
 * \param w homogeneous coordinate.
 * 
 * Stores the vertex and current attributes in ::vb, transforms it into eye space and then clip space.
 * 
 * If a sufficient number of vertices is stored calls one of select_point(),
 * select_line() or select_triangle(), according to the current primitive.
 */
static void
radeon_select_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_polygon_attrib *p = &(ctx->Polygon);
   vertex *v = vb.vBuffer + vb.vCount;

   /* store the vertex */
   v->pos.x = x;
   v->pos.y = y;
   v->pos.z = z;
   v->pos.w = w;
   v->color.r = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0];  
   v->color.g = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1];  
   v->color.b = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2];  
   v->color.a = ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3];  
   v->texCoord.s = ctx->Current.Attrib[VERT_ATTRIB_TEX0][0]; 
   v->texCoord.t = ctx->Current.Attrib[VERT_ATTRIB_TEX0][1]; 

   /* transform to eye space, then clip space */
   TRANSFORM_POINT(v->eyePos, ctx->ModelviewMatrixStack.Top->m, v->pos);
   TRANSFORM_POINT(v->clipPos, ctx->ProjectionMatrixStack.Top->m, v->eyePos);

   switch (ctx->Driver.CurrentExecPrimitive) {
   case GL_POINTS:
      assert(vb.vCount == 0);
      select_point(v);
      break;
   case GL_LINES:
      if (vb.vCount == 0)
      {
	 vb.vCount = 1;
      }
      else
      {
	 assert(vb.vCount == 1);
	 select_line(vb.vBuffer + 0, vb.vBuffer + 1);
	 vb.vCount = 0;
      }
      break;
   case GL_LINE_STRIP:
      if (vb.vCount == 0)
      {
	 vb.vCount = 1;
      }
      else
      {
	 assert(vb.vCount == 1);
	 select_line(vb.vBuffer + 0, vb.vBuffer + 1);
	 vb.vBuffer[0] = vb.vBuffer[1];
	 /* leave vb.vCount at 1 */
      }
      break;
   case GL_LINE_LOOP:
      if (vb.vCount == 0)
      {
	 vb.vCount = 1;
	 vb.partialLineLoop = GL_FALSE;
      }
      else if (vb.vCount == 1)
      {
	 select_line(vb.vBuffer + 0, vb.vBuffer + 1);
	 vb.partialLineLoop = GL_TRUE;
	 vb.vCount = 2;
      }
      else
      {
	 assert(vb.vCount == 2);
	 vb.partialLineLoop = GL_FALSE;
	 select_line(vb.vBuffer + 1, vb.vBuffer + 2);
	 vb.vBuffer[1] = vb.vBuffer[2];
	 /* leave vb.vCount at 2 */
      }
      break;
   case GL_TRIANGLES:
      if (vb.vCount == 0 || vb.vCount == 1)
      {
	 vb.vCount++;
      }
      else
      {
	 assert(vb.vCount == 2);
	 select_triangle(vb.vBuffer + 0, vb.vBuffer + 1, vb.vBuffer + 2);
	 vb.vCount = 0;
      }
      break;
   case GL_TRIANGLE_STRIP:
      if (vb.vCount == 0 || vb.vCount == 1)
      {
	 vb.vCount++;
      }
      else if (vb.vCount == 2)
      {
	 select_triangle(vb.vBuffer + 0, vb.vBuffer + 1, vb.vBuffer + 2);
	 vb.vCount = 3;
      }
      else
      {
	 assert(vb.vCount == 3);
	 select_triangle(vb.vBuffer + 1, vb.vBuffer + 3, vb.vBuffer + 2);
	 vb.vBuffer[0] = vb.vBuffer[2];
	 vb.vBuffer[1] = vb.vBuffer[3];
	 vb.vCount = 2;
      }
      break;		
   case GL_TRIANGLE_FAN:
      if (vb.vCount == 0 || vb.vCount == 1)
      {
	 vb.vCount++;
      }
      else
      {
	 assert(vb.vCount == 2);
	 select_triangle(vb.vBuffer + 0, vb.vBuffer + 1, vb.vBuffer + 2);
	 vb.vBuffer[1] = vb.vBuffer[2];
	 /* leave vb.vCount = 2 */
      }
      break;
   case GL_QUADS:
      if (vb.vCount < 3)
      {
	 vb.vCount++;
      }
      else
      {
	 assert(vb.vCount == 3);
	 select_triangle(vb.vBuffer + 0, vb.vBuffer + 1, vb.vBuffer + 2);
	 select_triangle(vb.vBuffer + 0, vb.vBuffer + 2, vb.vBuffer + 3);
	 vb.vCount = 0;
      }
      break;		
   case GL_QUAD_STRIP:
      if (vb.vCount < 3)
      {
	 vb.vCount++;
      }
      else
      {
	 assert(vb.vCount == 3);
	 select_triangle(vb.vBuffer + 0, vb.vBuffer + 1, vb.vBuffer + 2);
	 select_triangle(vb.vBuffer + 1, vb.vBuffer + 3, vb.vBuffer + 2);
	 vb.vBuffer[0] = vb.vBuffer[2];
	 vb.vBuffer[1] = vb.vBuffer[3];
	 vb.vCount = 2;
      }
      break;		
   case GL_POLYGON:
      switch (p->FrontMode) {
      case GL_POINT:
	 assert(vb.vCount == 0);
	 select_point(v);
	 break;
      case GL_LINE:
	 if (vb.vCount == 0)
	 {
	    vb.vCount = 1;
	    vb.partialLineLoop = GL_FALSE;
	 }
	 else if (vb.vCount == 1)
	 {
	    select_line(vb.vBuffer + 0, vb.vBuffer + 1);
	    vb.partialLineLoop = GL_TRUE;
	    vb.vCount = 2;
	 }
	 else
	 {
	    assert(vb.vCount == 2);
	    vb.partialLineLoop = GL_FALSE;
	    select_line(vb.vBuffer + 1, vb.vBuffer + 2);
	    vb.vBuffer[1] = vb.vBuffer[2];
	    /* leave vb.vCount at 2 */
	 }
	 break;
      case GL_FILL:
	 /* draw as a tri-fan */
	 if (vb.vCount == 0 || vb.vCount == 1)
	 {
	    vb.vCount++;
	 }
	 else
	 {
	    assert(vb.vCount == 2);
	    select_triangle(vb.vBuffer + 0, vb.vBuffer + 1, vb.vBuffer + 2);
	    vb.vBuffer[1] = vb.vBuffer[2];
	    /* leave vb.vCount = 2 */
	 }
	 break;
      default:
	 ; /* impossible */
      }
      break;
   default:
      ; /* outside begin/end -- no action required */
   }
}

/**
 * \brief Calls radeon_select_Vertex4f().
 */
static void radeon_select_Vertex2f(GLfloat x, GLfloat y)
{
   radeon_select_Vertex4f(x, y, 0.0, 1.0);
}

/**
 * \brief Calls radeon_select_Vertex4f().
 */
static void radeon_select_Vertex2fv(const GLfloat * v)
{
   radeon_select_Vertex4f(v[0], v[1], 0.0, 1.0);
}

/**
 * \brief Calls radeon_select_Vertex4f().
 */
static void radeon_select_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
   radeon_select_Vertex4f(x, y, z, 1.0);
}

/**
 * \brief Calls radeon_select_Vertex4f().
 */
static void radeon_select_Vertex3fv(const GLfloat * v)
{
   radeon_select_Vertex4f(v[0], v[1], v[2], 1.0);
}


/**
 * \brief Set current vertex color.
 *
 * \param r red color component.
 * \param g gree color component.
 * \param b blue color component.
 * \param a alpha color component.
 *
 * Updates the GL context's current vertex color.
 */
static void radeon_select_Color4f( GLfloat r, GLfloat g,
				   GLfloat b, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   dest[0] = r;
   dest[1] = g;
   dest[2] = b;
   dest[3] = a;
}

/**
 * \brief Calls radeon_select_Color4f().
 */
static void radeon_select_Color4fv( const GLfloat *v )
{
   radeon_select_Color4f( v[0], v[1], v[2], v[3] );
}

/**
 * \brief Calls radeon_select_Color4f().
 */
static void radeon_select_Color3f( GLfloat r, GLfloat g, GLfloat b )
{
   radeon_select_Color4f( r, g, b, 1.0 );
}

/**
 * \brief Calls radeon_select_Color4f().
 */
static void radeon_select_Color3fv( const GLfloat *v )
{
   radeon_select_Color4f( v[0], v[1], v[2], 1.0 );
}

/**
 * \brief Set current vertex texture coordinates.
 *
 * \param s texture coordinate.
 * \param t texture coordinate.
 *
 * Updates the GL context's current vertex texture coordinates.
 */
static __inline__ void radeon_select_TexCoord2f( GLfloat s, GLfloat t )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   dest[0] = s;
   dest[1] = t;
}

/**
 * \brief Calls radeon_select_TexCoord2f().
 */
static void radeon_select_TexCoord2fv( const GLfloat *v )
{
   radeon_select_TexCoord2f( v[0], v[1] );
}


/**
 * \brief Process glBegin().
 *
 * \param mode primitive.
 */ 
static void radeon_select_Begin(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);

   if (mode > GL_POLYGON) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glBegin" );
      return;
   }

   if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }

   ctx->Driver.CurrentExecPrimitive = mode;

   vb.vCount = 0;
   vb.lineReset = GL_TRUE;
   vb.partialLineLoop = GL_FALSE;
}

/**
 * \brief Process glEnd().
 */
static void radeon_select_End(void)
{
   GET_CURRENT_CONTEXT(ctx);

   if ( (ctx->Driver.CurrentExecPrimitive == GL_LINE_LOOP ||
	 (ctx->Driver.CurrentExecPrimitive == GL_POLYGON && 
	  ctx->Polygon.FrontMode == GL_LINE))
	&& vb.vCount == 2 )
   {
      /* draw the last line segment */
      if (vb.partialLineLoop)
	 select_line(vb.vBuffer + 1, vb.vBuffer + 0);
      else
	 select_line(vb.vBuffer + 2, vb.vBuffer + 0);
   }

   ctx->Driver.CurrentExecPrimitive = GL_POLYGON+1;
}


/**
 * \brief Flush vertices.
 * 
 * \param ctx GL context.
 * \param flags not used.
 *
 * Nothing much to do here, besides marking the vertices as flushed, as we
 * don't buffer anything.
 */
static void radeonSelectFlushVertices( GLcontext *ctx, GLuint flags )
{
   ctx->Driver.NeedFlush = 0;
}

/**
 * \brief Install the select callbacks.
 *
 * \param ctx GL context.
 *
 * Installs the glBegin()/glEnd() associated select callbacks into the glapi
 * table.
 */
void radeon_select_Install( GLcontext *ctx )
{
   struct _glapi_table *exec = ctx->Exec;

   exec->Color3f = radeon_select_Color3f;
   exec->Color3fv = radeon_select_Color3fv;
   exec->Color4f = radeon_select_Color4f;
   exec->Color4fv = radeon_select_Color4fv;
   exec->TexCoord2f = radeon_select_TexCoord2f;
   exec->TexCoord2fv = radeon_select_TexCoord2fv;
   exec->Vertex2f = radeon_select_Vertex2f;
   exec->Vertex2fv = radeon_select_Vertex2fv;
   exec->Vertex3f = radeon_select_Vertex3f;
   exec->Vertex3fv = radeon_select_Vertex3fv;
   exec->Begin = radeon_select_Begin;
   exec->End = radeon_select_End;

   ctx->Driver.FlushVertices = radeonSelectFlushVertices;
}
/*@}*/



/**
 * \brief Set rasterization mode.
 *
 * \param ctx GL context.
 * \param mode rasterization mode. Supports GL_RENDER or
 *
 * If mode is GL_RENDER, calls either radeonVtxfmtInit() or
 * radeon_noop_Install depending on whether the application has focus
 * (i.e., a fullscreen-cliprect) or not.  If mode is GL_SELECT, calls
 * radeon_select_Install().
 */
static void radeonRenderMode( GLcontext *ctx, GLenum mode )
{
   switch (mode) {
   case GL_RENDER:
      radeonVtxfmtInit( ctx );
      break;
   case GL_SELECT:
      radeon_select_Install( ctx );
      break;
   default:
      break;
   }
}

/**
 * \brief Setup the GL context driver callbacks.
 *
 * \param ctx GL context.
 *
 * \sa Called by radeonCreateContext().
 */
void radeonInitSelect( GLcontext *ctx )
{
   ctx->Driver.RenderMode = radeonRenderMode;
}
