
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
#include <math.h>
#include <stdlib.h>
#include "gluP.h"
#include "tess.h"
#endif



static GLenum store_polygon_as_contour(GLUtriangulatorObj *);
static void free_current_polygon(tess_polygon *);
static void prepare_projection_info(GLUtriangulatorObj *);
static GLdouble twice_the_polygon_area(tess_vertex *, tess_vertex *);
static GLenum verify_edge_vertex_intersections(GLUtriangulatorObj *);
void tess_find_contour_hierarchies(GLUtriangulatorObj *);
static GLenum test_for_overlapping_contours(GLUtriangulatorObj *);
static GLenum contours_overlap(tess_contour *, tess_polygon *);
static GLenum is_contour_contained_in(tess_contour *, tess_contour *);
static void add_new_exterior(GLUtriangulatorObj *, tess_contour *);
static void add_new_interior(GLUtriangulatorObj *, tess_contour *,
			     tess_contour *);
static void add_interior_with_hierarchy_check(GLUtriangulatorObj *,
					      tess_contour *, tess_contour *);
static void reverse_hierarchy_and_add_exterior(GLUtriangulatorObj *,
					       tess_contour *,
					       tess_contour *);
static GLboolean point_in_polygon(tess_contour *, GLdouble, GLdouble);
static void shift_interior_to_exterior(GLUtriangulatorObj *, tess_contour *);
static void add_exterior_with_check(GLUtriangulatorObj *, tess_contour *,
				    tess_contour *);
static GLenum cut_out_hole(GLUtriangulatorObj *, tess_contour *,
			   tess_contour *);
static GLenum merge_hole_with_contour(GLUtriangulatorObj *,
				      tess_contour *, tess_contour *,
				      tess_vertex *, tess_vertex *);

static GLenum
find_normal(GLUtriangulatorObj * tobj)
{
   tess_polygon *polygon = tobj->current_polygon;
   tess_vertex *va, *vb, *vc;
   GLdouble A, B, C;
   GLdouble A0, A1, A2, B0, B1, B2;

   va = polygon->vertices;
   vb = va->next;
   A0 = vb->location[0] - va->location[0];
   A1 = vb->location[1] - va->location[1];
   A2 = vb->location[2] - va->location[2];
   for (vc = vb->next; vc != va; vc = vc->next) {
      B0 = vc->location[0] - va->location[0];
      B1 = vc->location[1] - va->location[1];
      B2 = vc->location[2] - va->location[2];
      A = A1 * B2 - A2 * B1;
      B = A2 * B0 - A0 * B2;
      C = A0 * B1 - A1 * B0;
      if (fabs(A) > EPSILON || fabs(B) > EPSILON || fabs(C) > EPSILON) {
	 polygon->A = A;
	 polygon->B = B;
	 polygon->C = C;
	 polygon->D =
	    -A * va->location[0] - B * va->location[1] - C * va->location[2];
	 return GLU_NO_ERROR;
      }
   }
   tess_call_user_error(tobj, GLU_TESS_ERROR7);
   return GLU_ERROR;
}

void
tess_test_polygon(GLUtriangulatorObj * tobj)
{
   tess_polygon *polygon = tobj->current_polygon;

   /* any vertices defined? */
   if (polygon->vertex_cnt < 3) {
      free_current_polygon(polygon);
      return;
   }
   /* wrap pointers */
   polygon->last_vertex->next = polygon->vertices;
   polygon->vertices->previous = polygon->last_vertex;
   /* determine the normal */
   if (find_normal(tobj) == GLU_ERROR)
      return;
   /* compare the normals of previously defined contours and this one */
   /* first contour define ? */
   if (tobj->contours == NULL) {
      tobj->A = polygon->A;
      tobj->B = polygon->B;
      tobj->C = polygon->C;
      tobj->D = polygon->D;
      /* determine the best projection to use */
      if (fabs(polygon->A) > fabs(polygon->B))
	 if (fabs(polygon->A) > fabs(polygon->C))
	    tobj->projection = OYZ;
	 else
	    tobj->projection = OXY;
      else if (fabs(polygon->B) > fabs(polygon->C))
	 tobj->projection = OXZ;
      else
	 tobj->projection = OXY;
   }
   else {
      GLdouble a[3], b[3];
      tess_vertex *vertex = polygon->vertices;

      a[0] = tobj->A;
      a[1] = tobj->B;
      a[2] = tobj->C;
      b[0] = polygon->A;
      b[1] = polygon->B;
      b[2] = polygon->C;

      /* compare the normals */
      if (fabs(a[1] * b[2] - a[2] * b[1]) > EPSILON ||
	  fabs(a[2] * b[0] - a[0] * b[2]) > EPSILON ||
	  fabs(a[0] * b[1] - a[1] * b[0]) > EPSILON) {
	 /* not coplanar */
	 tess_call_user_error(tobj, GLU_TESS_ERROR9);
	 return;
      }
      /* the normals are parallel - test for plane equation */
      if (fabs(a[0] * vertex->location[0] + a[1] * vertex->location[1] +
	       a[2] * vertex->location[2] + tobj->D) > EPSILON) {
	 /* not the same plane */
	 tess_call_user_error(tobj, GLU_TESS_ERROR9);
	 return;
      }
   }
   prepare_projection_info(tobj);
   if (verify_edge_vertex_intersections(tobj) == GLU_ERROR)
      return;
   if (test_for_overlapping_contours(tobj) == GLU_ERROR)
      return;
   if (store_polygon_as_contour(tobj) == GLU_ERROR)
      return;
}

static GLenum
test_for_overlapping_contours(GLUtriangulatorObj * tobj)
{
   tess_contour *contour;
   tess_polygon *polygon;

   polygon = tobj->current_polygon;
   for (contour = tobj->contours; contour != NULL; contour = contour->next)
      if (contours_overlap(contour, polygon) != GLU_NO_ERROR) {
	 tess_call_user_error(tobj, GLU_TESS_ERROR5);
	 return GLU_ERROR;
      }
   return GLU_NO_ERROR;
}

static GLenum
store_polygon_as_contour(GLUtriangulatorObj * tobj)
{
   tess_polygon *polygon = tobj->current_polygon;
   tess_contour *contour = tobj->contours;

   /* the first contour defined */
   if (contour == NULL) {
      if ((contour = (tess_contour *) malloc(sizeof(tess_contour))) == NULL) {
	 tess_call_user_error(tobj, GLU_OUT_OF_MEMORY);
	 free_current_polygon(polygon);
	 return GLU_ERROR;
      }
      tobj->contours = tobj->last_contour = contour;
      contour->next = contour->previous = NULL;
   }
   else {
      if ((contour = (tess_contour *) malloc(sizeof(tess_contour))) == NULL) {
	 tess_call_user_error(tobj, GLU_OUT_OF_MEMORY);
	 free_current_polygon(polygon);
	 return GLU_ERROR;
      }
      contour->previous = tobj->last_contour;
      tobj->last_contour->next = contour;
      tobj->last_contour = contour;
      contour->next = NULL;
   }
   /* mark all vertices in new contour as not special */
   /* and all are boundary edges */
   {
      tess_vertex *vertex;
      GLuint vertex_cnt, i;

      for (vertex = polygon->vertices, i = 0, vertex_cnt =
	   polygon->vertex_cnt; i < vertex_cnt; vertex = vertex->next, i++) {
	 vertex->shadow_vertex = NULL;
	 vertex->edge_flag = GL_TRUE;
      }
   }
   contour->vertex_cnt = polygon->vertex_cnt;
   contour->area = polygon->area;
   contour->orientation = polygon->orientation;
   contour->type = GLU_UNKNOWN;
   contour->vertices = polygon->vertices;
   contour->last_vertex = polygon->last_vertex;
   polygon->vertices = polygon->last_vertex = NULL;
   polygon->vertex_cnt = 0;
   ++(tobj->contour_cnt);
   return GLU_NO_ERROR;
}

static void
free_current_polygon(tess_polygon * polygon)
{
   tess_vertex *vertex, *vertex_tmp;
   GLuint i;

   /* free current_polygon structures */
   for (vertex = polygon->vertices, i = 0; i < polygon->vertex_cnt; i++) {
      vertex_tmp = vertex->next;
      free(vertex);
      vertex = vertex_tmp;
   }
   polygon->vertices = polygon->last_vertex = NULL;
   polygon->vertex_cnt = 0;
}

static void
prepare_projection_info(GLUtriangulatorObj * tobj)
{
   tess_polygon *polygon = tobj->current_polygon;
   tess_vertex *vertex, *last_vertex_ptr;
   GLdouble area;

   last_vertex_ptr = polygon->last_vertex;
   switch (tobj->projection) {
   case OXY:
      for (vertex = polygon->vertices; vertex != last_vertex_ptr;
	   vertex = vertex->next) {
	 vertex->x = vertex->location[0];
	 vertex->y = vertex->location[1];
      }
      last_vertex_ptr->x = last_vertex_ptr->location[0];
      last_vertex_ptr->y = last_vertex_ptr->location[1];
      break;
   case OXZ:
      for (vertex = polygon->vertices; vertex != last_vertex_ptr;
	   vertex = vertex->next) {
	 vertex->x = vertex->location[0];
	 vertex->y = vertex->location[2];
      }
      last_vertex_ptr->x = last_vertex_ptr->location[0];
      last_vertex_ptr->y = last_vertex_ptr->location[2];
      break;
   case OYZ:
      for (vertex = polygon->vertices; vertex != last_vertex_ptr;
	   vertex = vertex->next) {
	 vertex->x = vertex->location[1];
	 vertex->y = vertex->location[2];
      }
      last_vertex_ptr->x = last_vertex_ptr->location[1];
      last_vertex_ptr->y = last_vertex_ptr->location[2];
      break;
   }
   area = twice_the_polygon_area(polygon->vertices, polygon->last_vertex);
   if (area >= 0.0) {
      polygon->orientation = GLU_CCW;
      polygon->area = area;
   }
   else {
      polygon->orientation = GLU_CW;
      polygon->area = -area;
   }
}

static GLdouble
twice_the_polygon_area(tess_vertex * vertex, tess_vertex * last_vertex)
{
   tess_vertex *next;
   GLdouble area, x, y;

   area = 0.0;
   x = vertex->x;
   y = vertex->y;
   vertex = vertex->next;
   for (; vertex != last_vertex; vertex = vertex->next) {
      next = vertex->next;
      area +=
	 (vertex->x - x) * (next->y - y) - (vertex->y - y) * (next->x - x);
   }
   return area;
}

/* test if edges ab and cd intersect */
/* if not return GLU_NO_ERROR, else if cross return GLU_TESS_ERROR8, */
/* else if adjacent return GLU_TESS_ERROR4 */
static GLenum
edge_edge_intersect(tess_vertex * a,
		    tess_vertex * b, tess_vertex * c, tess_vertex * d)
{
   GLdouble denom, r, s;
   GLdouble xba, ydc, yba, xdc, yac, xac;

   xba = b->x - a->x;
   yba = b->y - a->y;
   xdc = d->x - c->x;
   ydc = d->y - c->y;
   xac = a->x - c->x;
   yac = a->y - c->y;
   denom = xba * ydc - yba * xdc;
   r = yac * xdc - xac * ydc;
   /* parallel? */
   if (fabs(denom) < EPSILON) {
      if (fabs(r) < EPSILON) {
	 /* colinear */
	 if (fabs(xba) < EPSILON) {
	    /* compare the Y coordinate */
	    if (yba > 0.0) {
	       if (
		   (fabs(a->y - c->y) < EPSILON
		    && fabs(c->y - b->y) < EPSILON)
		   || (fabs(a->y - d->y) < EPSILON
		       && fabs(d->y - b->y) <
		       EPSILON)) return GLU_TESS_ERROR4;

	    }
	    else {
	       if (
		   (fabs(b->y - c->y) < EPSILON
		    && fabs(c->y - a->y) < EPSILON)
		   || (fabs(b->y - d->y) < EPSILON
		       && fabs(d->y - a->y) <
		       EPSILON)) return GLU_TESS_ERROR4;
	    }
	 }
	 else {
	    /* compare the X coordinate */
	    if (xba > 0.0) {
	       if (
		   (fabs(a->x - c->x) < EPSILON
		    && fabs(c->x - b->x) < EPSILON)
		   || (fabs(a->x - d->x) < EPSILON
		       && fabs(d->x - b->x) <
		       EPSILON)) return GLU_TESS_ERROR4;
	    }
	    else {
	       if (
		   (fabs(b->x - c->x) < EPSILON
		    && fabs(c->x - a->x) < EPSILON)
		   || (fabs(b->x - d->x) < EPSILON
		       && fabs(d->x - a->x) <
		       EPSILON)) return GLU_TESS_ERROR4;
	    }
	 }
      }
      return GLU_NO_ERROR;
   }
   r /= denom;
   s = (yac * xba - xac * yba) / denom;
   /* test if one vertex lies on other edge */
   if (((fabs(r) < EPSILON || (r < 1.0 + EPSILON && r > 1.0 - EPSILON)) &&
	s > -EPSILON && s < 1.0 + EPSILON) ||
       ((fabs(s) < EPSILON || (s < 1.0 + EPSILON && s > 1.0 - EPSILON)) &&
	r > -EPSILON && r < 1.0 + EPSILON)) {
      return GLU_TESS_ERROR4;
   }
   /* test for crossing */
   if (r > -EPSILON && r < 1.0 + EPSILON && s > -EPSILON && s < 1.0 + EPSILON) {
      return GLU_TESS_ERROR8;
   }
   return GLU_NO_ERROR;
}

static GLenum
verify_edge_vertex_intersections(GLUtriangulatorObj * tobj)
{
   tess_polygon *polygon = tobj->current_polygon;
   tess_vertex *vertex1, *last_vertex, *vertex2;
   GLenum test;

   last_vertex = polygon->last_vertex;
   vertex1 = last_vertex;
   for (vertex2 = vertex1->next->next;
	vertex2->next != last_vertex; vertex2 = vertex2->next) {
      test = edge_edge_intersect(vertex1, vertex1->next, vertex2,
				 vertex2->next);
      if (test != GLU_NO_ERROR) {
	 tess_call_user_error(tobj, test);
	 return GLU_ERROR;
      }
   }
   for (vertex1 = polygon->vertices;
	vertex1->next->next != last_vertex; vertex1 = vertex1->next) {
      for (vertex2 = vertex1->next->next;
	   vertex2 != last_vertex; vertex2 = vertex2->next) {
	 test = edge_edge_intersect(vertex1, vertex1->next, vertex2,
				    vertex2->next);
	 if (test != GLU_NO_ERROR) {
	    tess_call_user_error(tobj, test);
	    return GLU_ERROR;
	 }
      }
   }
   return GLU_NO_ERROR;
}

static int
#ifdef WIN32
  __cdecl
#endif
area_compare(const void *a, const void *b)
{
   GLdouble area1, area2;

   area1 = (*((tess_contour **) a))->area;
   area2 = (*((tess_contour **) b))->area;
   if (area1 < area2)
      return 1;
   if (area1 > area2)
      return -1;
   return 0;
}

void
tess_find_contour_hierarchies(GLUtriangulatorObj * tobj)
{
   tess_contour **contours;	/* dinamic array of pointers */
   tess_contour *tmp_contour_ptr = tobj->contours;
   GLuint cnt, i;
   GLenum result;
   GLboolean hierarchy_changed;

   /* any contours? */
   if (tobj->contour_cnt < 2) {
      tobj->contours->type = GLU_EXTERIOR;
      return;
   }
   if ((contours = (tess_contour **)
	malloc(sizeof(tess_contour *) * (tobj->contour_cnt))) == NULL) {
      tess_call_user_error(tobj, GLU_OUT_OF_MEMORY);
      return;
   }
   for (tmp_contour_ptr = tobj->contours, cnt = 0;
	tmp_contour_ptr != NULL; tmp_contour_ptr = tmp_contour_ptr->next)
      contours[cnt++] = tmp_contour_ptr;
   /* now sort the contours in decreasing area size order */
   qsort((void *) contours, (size_t) cnt, (size_t) sizeof(tess_contour *),
	 area_compare);
   /* we leave just the first contour - remove others from list */
   tobj->contours = contours[0];
   tobj->contours->next = tobj->contours->previous = NULL;
   tobj->last_contour = tobj->contours;
   tobj->contour_cnt = 1;
   /* first contour is the one with greatest area */
   /* must be EXTERIOR */
   tobj->contours->type = GLU_EXTERIOR;
   tmp_contour_ptr = tobj->contours;
   /* now we play! */
   for (i = 1; i < cnt; i++) {
      hierarchy_changed = GL_FALSE;
      for (tmp_contour_ptr = tobj->contours;
	   tmp_contour_ptr != NULL; tmp_contour_ptr = tmp_contour_ptr->next) {
	 if (tmp_contour_ptr->type == GLU_EXTERIOR) {
	    /* check if contour completely contained in EXTERIOR */
	    result = is_contour_contained_in(tmp_contour_ptr, contours[i]);
	    switch (result) {
	    case GLU_INTERIOR:
	       /* now we have to check if contour is inside interiors */
	       /* or not */
	       /* any interiors? */
	       if (tmp_contour_ptr->next != NULL &&
		   tmp_contour_ptr->next->type == GLU_INTERIOR) {
		  /* for all interior, check if inside any of them */
		  /* if not inside any of interiors, its another */
		  /* interior */
		  /* or it may contain some interiors, then change */
		  /* the contained interiors to exterior ones */
		  add_interior_with_hierarchy_check(tobj,
						    tmp_contour_ptr,
						    contours[i]);
	       }
	       else {
		  /* not in interior, add as new interior contour */
		  add_new_interior(tobj, tmp_contour_ptr, contours[i]);
	       }
	       hierarchy_changed = GL_TRUE;
	       break;
	    case GLU_EXTERIOR:
	       /* ooops, the marked as EXTERIOR (contours[i]) is */
	       /* actually an interior of tmp_contour_ptr */
	       /*  reverse the local hierarchy */
	       reverse_hierarchy_and_add_exterior(tobj, tmp_contour_ptr,
						  contours[i]);
	       hierarchy_changed = GL_TRUE;
	       break;
	    case GLU_NO_ERROR:
	       break;
	    default:
	       abort();
	    }
	 }
	 if (hierarchy_changed)
	    break;		/* break from for loop */
      }
      if (hierarchy_changed == GL_FALSE) {
	 /* disjoint with all contours, add to contour list */
	 add_new_exterior(tobj, contours[i]);
      }
   }
   free(contours);
}

/* returns GLU_INTERIOR if inner is completey enclosed within outer */
/* returns GLU_EXTERIOR if outer is completely enclosed within inner */
/* returns GLU_NO_ERROR if contours are disjoint */
static GLenum
is_contour_contained_in(tess_contour * outer, tess_contour * inner)
{
   GLenum relation_flag;

   /* set relation_flag to relation of containment of first inner vertex */
   /* regarding outer contour */
   if (point_in_polygon(outer, inner->vertices->x, inner->vertices->y))
      relation_flag = GLU_INTERIOR;
   else
      relation_flag = GLU_EXTERIOR;
   if (relation_flag == GLU_INTERIOR)
      return GLU_INTERIOR;
   if (point_in_polygon(inner, outer->vertices->x, outer->vertices->y))
      return GLU_EXTERIOR;
   return GLU_NO_ERROR;
}

static GLboolean
point_in_polygon(tess_contour * contour, GLdouble x, GLdouble y)
{
   tess_vertex *v1, *v2;
   GLuint i, vertex_cnt;
   GLdouble xp1, yp1, xp2, yp2;
   GLboolean tst;

   tst = GL_FALSE;
   v1 = contour->vertices;
   v2 = contour->vertices->previous;
   for (i = 0, vertex_cnt = contour->vertex_cnt; i < vertex_cnt; i++) {
      xp1 = v1->x;
      yp1 = v1->y;
      xp2 = v2->x;
      yp2 = v2->y;
      if ((((yp1 <= y) && (y < yp2)) || ((yp2 <= y) && (y < yp1))) &&
	  (x < (xp2 - xp1) * (y - yp1) / (yp2 - yp1) + xp1))
	 tst = (tst == GL_FALSE ? GL_TRUE : GL_FALSE);
      v2 = v1;
      v1 = v1->next;
   }
   return tst;
}

static GLenum
contours_overlap(tess_contour * contour, tess_polygon * polygon)
{
   tess_vertex *vertex1, *vertex2;
   GLuint vertex1_cnt, vertex2_cnt, i, j;
   GLenum test;

   vertex1 = contour->vertices;
   vertex2 = polygon->vertices;
   vertex1_cnt = contour->vertex_cnt;
   vertex2_cnt = polygon->vertex_cnt;
   for (i = 0; i < vertex1_cnt; vertex1 = vertex1->next, i++) {
      for (j = 0; j < vertex2_cnt; vertex2 = vertex2->next, j++)
	 if ((test = edge_edge_intersect(vertex1, vertex1->next, vertex2,
					 vertex2->next)) != GLU_NO_ERROR)
	    return test;
   }
   return GLU_NO_ERROR;
}

static void
add_new_exterior(GLUtriangulatorObj * tobj, tess_contour * contour)
{
   contour->type = GLU_EXTERIOR;
   contour->next = NULL;
   contour->previous = tobj->last_contour;
   tobj->last_contour->next = contour;
   tobj->last_contour = contour;
}

static void
add_new_interior(GLUtriangulatorObj * tobj,
		 tess_contour * outer, tess_contour * contour)
{
   contour->type = GLU_INTERIOR;
   contour->next = outer->next;
   contour->previous = outer;
   if (outer->next != NULL)
      outer->next->previous = contour;
   outer->next = contour;
   if (tobj->last_contour == outer)
      tobj->last_contour = contour;
}

static void
add_interior_with_hierarchy_check(GLUtriangulatorObj * tobj,
				  tess_contour * outer,
				  tess_contour * contour)
{
   tess_contour *ptr;

   /* for all interiors of outer check if they are interior of contour */
   /* if so, change that interior to exterior and move it of of the */
   /* interior sequence */
   if (outer->next != NULL && outer->next->type == GLU_INTERIOR) {
      GLenum test;

      for (ptr = outer->next; ptr != NULL && ptr->type == GLU_INTERIOR;
	   ptr = ptr->next) {
	 test = is_contour_contained_in(ptr, contour);
	 switch (test) {
	 case GLU_INTERIOR:
	    /* contour is contained in one of the interiors */
	    /* check if possibly contained in other exteriors */
	    /* move ptr to first EXTERIOR */
	    for (; ptr != NULL && ptr->type == GLU_INTERIOR; ptr = ptr->next);
	    if (ptr == NULL)
	       /* another exterior */
	       add_new_exterior(tobj, contour);
	    else
	       add_exterior_with_check(tobj, ptr, contour);
	    return;
	 case GLU_EXTERIOR:
	    /* one of the interiors is contained in the contour */
	    /* change it to EXTERIOR, and shift it away from the */
	    /* interior sequence */
	    shift_interior_to_exterior(tobj, ptr);
	    break;
	 case GLU_NO_ERROR:
	    /* disjoint */
	    break;
	 default:
	    abort();
	 }
      }
   }
   /* add contour to the interior sequence */
   add_new_interior(tobj, outer, contour);
}

static void
reverse_hierarchy_and_add_exterior(GLUtriangulatorObj * tobj,
				   tess_contour * outer,
				   tess_contour * contour)
{
   tess_contour *ptr;

   /* reverse INTERIORS to EXTERIORS */
   /* any INTERIORS? */
   if (outer->next != NULL && outer->next->type == GLU_INTERIOR)
      for (ptr = outer->next; ptr != NULL && ptr->type == GLU_INTERIOR;
	   ptr = ptr->next) ptr->type = GLU_EXTERIOR;
   /* the outer now becomes inner */
   outer->type = GLU_INTERIOR;
   /* contour is the EXTERIOR */
   contour->next = outer;
   if (tobj->contours == outer) {
      /* first contour beeing reversed */
      contour->previous = NULL;
      tobj->contours = contour;
   }
   else {
      outer->previous->next = contour;
      contour->previous = outer->previous;
   }
   outer->previous = contour;
}

static void
shift_interior_to_exterior(GLUtriangulatorObj * tobj, tess_contour * contour)
{
   contour->previous->next = contour->next;
   if (contour->next != NULL)
      contour->next->previous = contour->previous;
   else
      tobj->last_contour = contour->previous;
}

static void
add_exterior_with_check(GLUtriangulatorObj * tobj,
			tess_contour * outer, tess_contour * contour)
{
   GLenum test;

   /* this contour might be interior to further exteriors - check */
   /* if not, just add as a new exterior */
   for (; outer != NULL && outer->type == GLU_EXTERIOR; outer = outer->next) {
      test = is_contour_contained_in(outer, contour);
      switch (test) {
      case GLU_INTERIOR:
	 /* now we have to check if contour is inside interiors */
	 /* or not */
	 /* any interiors? */
	 if (outer->next != NULL && outer->next->type == GLU_INTERIOR) {
	    /* for all interior, check if inside any of them */
	    /* if not inside any of interiors, its another */
	    /* interior */
	    /* or it may contain some interiors, then change */
	    /* the contained interiors to exterior ones */
	    add_interior_with_hierarchy_check(tobj, outer, contour);
	 }
	 else {
	    /* not in interior, add as new interior contour */
	    add_new_interior(tobj, outer, contour);
	 }
	 return;
      case GLU_NO_ERROR:
	 /* disjoint */
	 break;
      default:
	 abort();
      }
   }
   /* add contour to the exterior sequence */
   add_new_exterior(tobj, contour);
}

void
tess_handle_holes(GLUtriangulatorObj * tobj)
{
   tess_contour *contour, *hole;
   GLenum exterior_orientation;

   /* verify hole orientation */
   for (contour = tobj->contours; contour != NULL;) {
      exterior_orientation = contour->orientation;
      for (contour = contour->next;
	   contour != NULL && contour->type == GLU_INTERIOR;
	   contour = contour->next) {
	 if (contour->orientation == exterior_orientation) {
	    tess_call_user_error(tobj, GLU_TESS_ERROR5);
	    return;
	 }
      }
   }
   /* now cut-out holes */
   for (contour = tobj->contours; contour != NULL;) {
      hole = contour->next;
      while (hole != NULL && hole->type == GLU_INTERIOR) {
	 if (cut_out_hole(tobj, contour, hole) == GLU_ERROR)
	    return;
	 hole = contour->next;
      }
      contour = contour->next;
   }
}

static GLenum
cut_out_hole(GLUtriangulatorObj * tobj,
	     tess_contour * contour, tess_contour * hole)
{
   tess_contour *tmp_hole;
   tess_vertex *v1, *v2, *tmp_vertex;
   GLuint vertex1_cnt, vertex2_cnt, tmp_vertex_cnt;
   GLuint i, j, k;
   GLenum test = 0;

   /* find an edge connecting contour and hole not intersecting any other */
   /* edge belonging to either the contour or any of the other holes */
   for (v1 = contour->vertices, vertex1_cnt = contour->vertex_cnt, i = 0;
	i < vertex1_cnt; i++, v1 = v1->next) {
      for (v2 = hole->vertices, vertex2_cnt = hole->vertex_cnt, j = 0;
	   j < vertex2_cnt; j++, v2 = v2->next) {
	 /* does edge (v1,v2) intersect any edge of contour */
	 for (tmp_vertex = contour->vertices, tmp_vertex_cnt =
	      contour->vertex_cnt, k = 0; k < tmp_vertex_cnt;
	      tmp_vertex = tmp_vertex->next, k++) {
	    /* skip edge tests for edges directly connected */
	    if (v1 == tmp_vertex || v1 == tmp_vertex->next)
	       continue;
	    test = edge_edge_intersect(v1, v2, tmp_vertex, tmp_vertex->next);
	    if (test != GLU_NO_ERROR)
	       break;
	 }
	 if (test == GLU_NO_ERROR) {
	    /* does edge (v1,v2) intersect any edge of hole */
	    for (tmp_vertex = hole->vertices,
		 tmp_vertex_cnt = hole->vertex_cnt, k = 0;
		 k < tmp_vertex_cnt; tmp_vertex = tmp_vertex->next, k++) {
	       /* skip edge tests for edges directly connected */
	       if (v2 == tmp_vertex || v2 == tmp_vertex->next)
		  continue;
	       test =
		  edge_edge_intersect(v1, v2, tmp_vertex, tmp_vertex->next);
	       if (test != GLU_NO_ERROR)
		  break;
	    }
	    if (test == GLU_NO_ERROR) {
	       /* does edge (v1,v2) intersect any other hole? */
	       for (tmp_hole = hole->next;
		    tmp_hole != NULL && tmp_hole->type == GLU_INTERIOR;
		    tmp_hole = tmp_hole->next) {
		  /* does edge (v1,v2) intersect any edge of hole */
		  for (tmp_vertex = tmp_hole->vertices,
		       tmp_vertex_cnt = tmp_hole->vertex_cnt, k = 0;
		       k < tmp_vertex_cnt; tmp_vertex = tmp_vertex->next, k++) {
		     test = edge_edge_intersect(v1, v2, tmp_vertex,
						tmp_vertex->next);
		     if (test != GLU_NO_ERROR)
			break;
		  }
		  if (test != GLU_NO_ERROR)
		     break;
	       }
	    }
	 }
	 if (test == GLU_NO_ERROR) {
	    /* edge (v1,v2) is good for eliminating the hole */
	    if (merge_hole_with_contour(tobj, contour, hole, v1, v2)
		== GLU_NO_ERROR)
	       return GLU_NO_ERROR;
	    else
	       return GLU_ERROR;
	 }
      }
   }
   /* other holes are blocking all possible connections of hole */
   /* with contour, we shift this hole as the last hole and retry */
   for (tmp_hole = hole;
	tmp_hole != NULL && tmp_hole->type == GLU_INTERIOR;
	tmp_hole = tmp_hole->next);
   contour->next = hole->next;
   hole->next->previous = contour;
   if (tmp_hole == NULL) {
      /* last EXTERIOR contour, shift hole as last contour */
      hole->next = NULL;
      hole->previous = tobj->last_contour;
      tobj->last_contour->next = hole;
      tobj->last_contour = hole;
   }
   else {
      tmp_hole->previous->next = hole;
      hole->previous = tmp_hole->previous;
      tmp_hole->previous = hole;
      hole->next = tmp_hole;
   }
   hole = contour->next;
   /* try once again - recurse */
   return cut_out_hole(tobj, contour, hole);
}

static GLenum
merge_hole_with_contour(GLUtriangulatorObj * tobj,
			tess_contour * contour,
			tess_contour * hole,
			tess_vertex * v1, tess_vertex * v2)
{
   tess_vertex *v1_new, *v2_new;

   /* make copies of v1 and v2, place them respectively after their originals */
   if ((v1_new = (tess_vertex *) malloc(sizeof(tess_vertex))) == NULL) {
      tess_call_user_error(tobj, GLU_OUT_OF_MEMORY);
      return GLU_ERROR;
   }
   if ((v2_new = (tess_vertex *) malloc(sizeof(tess_vertex))) == NULL) {
      tess_call_user_error(tobj, GLU_OUT_OF_MEMORY);
      return GLU_ERROR;
   }
   v1_new->edge_flag = GL_TRUE;
   v1_new->data = v1->data;
   v1_new->location[0] = v1->location[0];
   v1_new->location[1] = v1->location[1];
   v1_new->location[2] = v1->location[2];
   v1_new->x = v1->x;
   v1_new->y = v1->y;
   v1_new->shadow_vertex = v1;
   v1->shadow_vertex = v1_new;
   v1_new->next = v1->next;
   v1_new->previous = v1;
   v1->next->previous = v1_new;
   v1->next = v1_new;
   v2_new->edge_flag = GL_TRUE;
   v2_new->data = v2->data;
   v2_new->location[0] = v2->location[0];
   v2_new->location[1] = v2->location[1];
   v2_new->location[2] = v2->location[2];
   v2_new->x = v2->x;
   v2_new->y = v2->y;
   v2_new->shadow_vertex = v2;
   v2->shadow_vertex = v2_new;
   v2_new->next = v2->next;
   v2_new->previous = v2;
   v2->next->previous = v2_new;
   v2->next = v2_new;
   /* link together the two lists */
   v1->next = v2_new;
   v2_new->previous = v1;
   v2->next = v1_new;
   v1_new->previous = v2;
   /* update the vertex count of the contour */
   contour->vertex_cnt += hole->vertex_cnt + 2;
   /* remove the INTERIOR contour */
   contour->next = hole->next;
   if (hole->next != NULL)
      hole->next->previous = contour;
   free(hole);
   /* update tobj structure */
   --(tobj->contour_cnt);
   if (contour->last_vertex == v1)
      contour->last_vertex = v1_new;
   /* mark two vertices with edge_flag */
   v2->edge_flag = GL_FALSE;
   v1->edge_flag = GL_FALSE;
   return GLU_NO_ERROR;
}
