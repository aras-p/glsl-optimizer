
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-2000  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * This file is part of the polygon tesselation code contributed by
 * Bogdan Sikorski
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <math.h>
#include "tess.h"
#endif



static GLboolean edge_flag;

static void emit_triangle(GLUtriangulatorObj *, tess_vertex *,
			  tess_vertex *, tess_vertex *);

static void emit_triangle_with_edge_flag(GLUtriangulatorObj *,
					 tess_vertex *, GLboolean,
					 tess_vertex *, GLboolean,
					 tess_vertex *, GLboolean);

static GLdouble
twice_the_triangle_area(tess_vertex * va, tess_vertex * vb, tess_vertex * vc)
{
   return (vb->x - va->x) * (vc->y - va->y) - (vb->y - va->y) * (vc->x -
								 va->x);
}

static GLboolean
left(GLdouble A, GLdouble B, GLdouble C, GLdouble x, GLdouble y)
{
   if (A * x + B * y + C > -EPSILON)
      return GL_TRUE;
   else
      return GL_FALSE;
}

static GLboolean
right(GLdouble A, GLdouble B, GLdouble C, GLdouble x, GLdouble y)
{
   if (A * x + B * y + C < EPSILON)
      return GL_TRUE;
   else
      return GL_FALSE;
}

static GLint
convex_ccw(tess_vertex * va,
	   tess_vertex * vb, tess_vertex * vc, GLUtriangulatorObj * tobj)
{
   GLdouble d;

   d = twice_the_triangle_area(va, vb, vc);

   if (d > EPSILON) {
      return 1;
   }
   else if (d < -EPSILON) {
      return 0;
   }
   else {
      return -1;
   }
}

static GLint
convex_cw(tess_vertex * va,
	  tess_vertex * vb, tess_vertex * vc, GLUtriangulatorObj * tobj)
{
   GLdouble d;

   d = twice_the_triangle_area(va, vb, vc);

   if (d < -EPSILON) {
      return 1;
   }
   else if (d > EPSILON) {
      return 0;
   }
   else {
      return -1;
   }
}

static GLboolean
diagonal_ccw(tess_vertex * va,
	     tess_vertex * vb,
	     GLUtriangulatorObj * tobj, tess_contour * contour)
{
   tess_vertex *vc = va->next, *vertex, *shadow_vertex;
   struct
   {
      GLdouble A, B, C;
   }
   ac, cb, ba;
   GLdouble x, y;

   GLint res = convex_ccw(va, vc, vb, tobj);
   if (res == 0)
      return GL_FALSE;
   if (res == -1)
      return GL_TRUE;

   ba.A = vb->y - va->y;
   ba.B = va->x - vb->x;
   ba.C = -ba.A * va->x - ba.B * va->y;
   ac.A = va->y - vc->y;
   ac.B = vc->x - va->x;
   ac.C = -ac.A * vc->x - ac.B * vc->y;
   cb.A = vc->y - vb->y;
   cb.B = vb->x - vc->x;
   cb.C = -cb.A * vb->x - cb.B * vb->y;
   for (vertex = vb->next; vertex != va; vertex = vertex->next) {
      shadow_vertex = vertex->shadow_vertex;
      if (shadow_vertex != NULL &&
	  (shadow_vertex == va || shadow_vertex == vb || shadow_vertex == vc))
	 continue;
      x = vertex->x;
      y = vertex->y;
      if (left(ba.A, ba.B, ba.C, x, y) &&
	  left(ac.A, ac.B, ac.C, x, y) && left(cb.A, cb.B, cb.C, x, y))
	 return GL_FALSE;
   }
   return GL_TRUE;
}

static GLboolean
diagonal_cw(tess_vertex * va,
	    tess_vertex * vb,
	    GLUtriangulatorObj * tobj, tess_contour * contour)
{
   tess_vertex *vc = va->next, *vertex, *shadow_vertex;
   struct
   {
      GLdouble A, B, C;
   }
   ac, cb, ba;
   GLdouble x, y;

   GLint res = convex_cw(va, vc, vb, tobj);
   if (res == 0)
      return GL_FALSE;
   if (res == -1)
      return GL_TRUE;

   ba.A = vb->y - va->y;
   ba.B = va->x - vb->x;
   ba.C = -ba.A * va->x - ba.B * va->y;
   ac.A = va->y - vc->y;
   ac.B = vc->x - va->x;
   ac.C = -ac.A * vc->x - ac.B * vc->y;
   cb.A = vc->y - vb->y;
   cb.B = vb->x - vc->x;
   cb.C = -cb.A * vb->x - cb.B * vb->y;
   for (vertex = vb->next; vertex != va; vertex = vertex->next) {
      shadow_vertex = vertex->shadow_vertex;
      if (shadow_vertex != NULL &&
	  (shadow_vertex == va || shadow_vertex == vb || shadow_vertex == vc))
	 continue;
      x = vertex->x;
      y = vertex->y;
      if (right(ba.A, ba.B, ba.C, x, y) &&
	  right(ac.A, ac.B, ac.C, x, y) && right(cb.A, cb.B, cb.C, x, y))
	 return GL_FALSE;
   }
   return GL_TRUE;
}

static void
clip_ear(GLUtriangulatorObj * tobj, tess_vertex * v, tess_contour * contour)
{
   emit_triangle(tobj, v->previous, v, v->next);
   /* the first in the list */
   if (contour->vertices == v) {
      contour->vertices = v->next;
      contour->last_vertex->next = v->next;
      v->next->previous = contour->last_vertex;
   }
   else
      /* the last ? */
   if (contour->last_vertex == v) {
      contour->vertices->previous = v->previous;
      v->previous->next = v->next;
      contour->last_vertex = v->previous;
   }
   else {
      v->next->previous = v->previous;
      v->previous->next = v->next;
   }
   free(v);
   --(contour->vertex_cnt);
}

static void
clip_ear_with_edge_flag(GLUtriangulatorObj * tobj,
			tess_vertex * v, tess_contour * contour)
{
   emit_triangle_with_edge_flag(tobj, v->previous, v->previous->edge_flag,
				v, v->edge_flag, v->next, GL_FALSE);
   v->previous->edge_flag = GL_FALSE;
   /* the first in the list */
   if (contour->vertices == v) {
      contour->vertices = v->next;
      contour->last_vertex->next = v->next;
      v->next->previous = contour->last_vertex;
   }
   else
      /* the last ? */
   if (contour->last_vertex == v) {
      contour->vertices->previous = v->previous;
      v->previous->next = v->next;
      contour->last_vertex = v->previous;
   }
   else {
      v->next->previous = v->previous;
      v->previous->next = v->next;
   }
   free(v);
   --(contour->vertex_cnt);
}

static void
triangulate_ccw(GLUtriangulatorObj * tobj, tess_contour * contour)
{
   tess_vertex *vertex;
   GLuint vertex_cnt = contour->vertex_cnt;

   while (vertex_cnt > 3) {
      vertex = contour->vertices;
      while (diagonal_ccw(vertex, vertex->next->next, tobj, contour) ==
	     GL_FALSE && tobj->error == GLU_NO_ERROR)
	 vertex = vertex->next;
      if (tobj->error != GLU_NO_ERROR)
	 return;
      clip_ear(tobj, vertex->next, contour);
      --vertex_cnt;
   }
}

static void
triangulate_cw(GLUtriangulatorObj * tobj, tess_contour * contour)
{
   tess_vertex *vertex;
   GLuint vertex_cnt = contour->vertex_cnt;

   while (vertex_cnt > 3) {
      vertex = contour->vertices;
      while (diagonal_cw(vertex, vertex->next->next, tobj, contour) ==
	     GL_FALSE && tobj->error == GLU_NO_ERROR)
	 vertex = vertex->next;
      if (tobj->error != GLU_NO_ERROR)
	 return;
      clip_ear(tobj, vertex->next, contour);
      --vertex_cnt;
   }
}

static void
triangulate_ccw_with_edge_flag(GLUtriangulatorObj * tobj,
			       tess_contour * contour)
{
   tess_vertex *vertex;
   GLuint vertex_cnt = contour->vertex_cnt;

   while (vertex_cnt > 3) {
      vertex = contour->vertices;
      while (diagonal_ccw(vertex, vertex->next->next, tobj, contour) ==
	     GL_FALSE && tobj->error == GLU_NO_ERROR)
	 vertex = vertex->next;
      if (tobj->error != GLU_NO_ERROR)
	 return;
      clip_ear_with_edge_flag(tobj, vertex->next, contour);
      --vertex_cnt;
   }
}

static void
triangulate_cw_with_edge_flag(GLUtriangulatorObj * tobj,
			      tess_contour * contour)
{
   tess_vertex *vertex;
   GLuint vertex_cnt = contour->vertex_cnt;

   while (vertex_cnt > 3) {
      vertex = contour->vertices;
      while (diagonal_cw(vertex, vertex->next->next, tobj, contour) ==
	     GL_FALSE && tobj->error == GLU_NO_ERROR)
	 vertex = vertex->next;
      if (tobj->error != GLU_NO_ERROR)
	 return;
      clip_ear_with_edge_flag(tobj, vertex->next, contour);
      --vertex_cnt;
   }
}

void
tess_tesselate(GLUtriangulatorObj * tobj)
{
   tess_contour *contour;

   for (contour = tobj->contours; contour != NULL; contour = contour->next) {
      if (contour->orientation == GLU_CCW) {
	 triangulate_ccw(tobj, contour);
      }
      else {
	 triangulate_cw(tobj, contour);
      }
      if (tobj->error != GLU_NO_ERROR)
	 return;

      /* emit the last triangle */
      emit_triangle(tobj, contour->vertices, contour->vertices->next,
		    contour->vertices->next->next);
   }
}

void
tess_tesselate_with_edge_flag(GLUtriangulatorObj * tobj)
{
   tess_contour *contour;

   edge_flag = GL_TRUE;
   /* first callback with edgeFlag set to GL_TRUE */
   (tobj->callbacks.edgeFlag) (GL_TRUE);

   for (contour = tobj->contours; contour != NULL; contour = contour->next) {
      if (contour->orientation == GLU_CCW)
	 triangulate_ccw_with_edge_flag(tobj, contour);
      else
	 triangulate_cw_with_edge_flag(tobj, contour);
      if (tobj->error != GLU_NO_ERROR)
	 return;
      /* emit the last triangle */
      emit_triangle_with_edge_flag(tobj, contour->vertices,
				   contour->vertices->edge_flag,
				   contour->vertices->next,
				   contour->vertices->next->edge_flag,
				   contour->vertices->next->next,
				   contour->vertices->next->next->edge_flag);
   }
}

static void
emit_triangle(GLUtriangulatorObj * tobj,
	      tess_vertex * v1, tess_vertex * v2, tess_vertex * v3)
{
   (tobj->callbacks.begin) (GL_TRIANGLES);
   (tobj->callbacks.vertex) (v1->data);
   (tobj->callbacks.vertex) (v2->data);
   (tobj->callbacks.vertex) (v3->data);
   (tobj->callbacks.end) ();
}

static void
emit_triangle_with_edge_flag(GLUtriangulatorObj * tobj,
			     tess_vertex * v1,
			     GLboolean edge_flag1,
			     tess_vertex * v2,
			     GLboolean edge_flag2,
			     tess_vertex * v3, GLboolean edge_flag3)
{
   (tobj->callbacks.begin) (GL_TRIANGLES);
   if (edge_flag1 != edge_flag) {
      edge_flag = (edge_flag == GL_TRUE ? GL_FALSE : GL_TRUE);
      (tobj->callbacks.edgeFlag) (edge_flag);
   }
   (tobj->callbacks.vertex) (v1->data);
   if (edge_flag2 != edge_flag) {
      edge_flag = (edge_flag == GL_TRUE ? GL_FALSE : GL_TRUE);
      (tobj->callbacks.edgeFlag) (edge_flag);
   }
   (tobj->callbacks.vertex) (v2->data);
   if (edge_flag3 != edge_flag) {
      edge_flag = (edge_flag == GL_TRUE ? GL_FALSE : GL_TRUE);
      (tobj->callbacks.edgeFlag) (edge_flag);
   }
   (tobj->callbacks.vertex) (v3->data);
   (tobj->callbacks.end) ();
}
