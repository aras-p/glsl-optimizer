
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
 * NURBS implementation written by Bogdan Sikorski (bogdan@cira.it)
 * See README2 for more info.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <math.h>
#include <stdlib.h>
#include "gluP.h"
#include "nurbs.h"
#endif


GLenum test_knot(GLint nknots, GLfloat * knot, GLint order)
{
   GLsizei i;
   GLint knot_mult;
   GLfloat tmp_knot;

   tmp_knot = knot[0];
   knot_mult = 1;
   for (i = 1; i < nknots; i++) {
      if (knot[i] < tmp_knot)
	 return GLU_NURBS_ERROR4;
      if (fabs(tmp_knot - knot[i]) > EPSILON) {
	 if (knot_mult > order)
	    return GLU_NURBS_ERROR5;
	 knot_mult = 1;
	 tmp_knot = knot[i];
      }
      else
	 ++knot_mult;
   }
   return GLU_NO_ERROR;
}

static int
/* qsort function */
#if defined(WIN32) && !defined(OPENSTEP)
  __cdecl
#endif
knot_sort(const void *a, const void *b)
{
   GLfloat x, y;

   x = *((GLfloat *) a);
   y = *((GLfloat *) b);
   if (fabs(x - y) < EPSILON)
      return 0;
   if (x > y)
      return 1;
   return -1;
}

/* insert into dest knot all values within the valid range from src knot */
/* that do not appear in dest */
void
collect_unified_knot(knot_str_type * dest, knot_str_type * src,
		     GLfloat maximal_min_knot, GLfloat minimal_max_knot)
{
   GLfloat *src_knot, *dest_knot;
   GLint src_t_min, src_t_max, dest_t_min, dest_t_max;
   GLint src_nknots, dest_nknots;
   GLint i, j, k, new_cnt;
   GLboolean not_found_flag;

   src_knot = src->unified_knot;
   dest_knot = dest->unified_knot;
   src_t_min = src->t_min;
   src_t_max = src->t_max;
   dest_t_min = dest->t_min;
   dest_t_max = dest->t_max;
   src_nknots = src->unified_nknots;
   dest_nknots = dest->unified_nknots;

   k = new_cnt = dest_nknots;
   for (i = src_t_min; i <= src_t_max; i++)
      if (src_knot[i] - maximal_min_knot > -EPSILON &&
	  src_knot[i] - minimal_max_knot < EPSILON) {
	 not_found_flag = GL_TRUE;
	 for (j = dest_t_min; j <= dest_t_max; j++)
	    if (fabs(dest_knot[j] - src_knot[i]) < EPSILON) {
	       not_found_flag = GL_FALSE;
	       break;
	    }
	 if (not_found_flag) {
	    /* knot from src is not in dest - add this knot to dest */
	    dest_knot[k++] = src_knot[i];
	    ++new_cnt;
	    ++(dest->t_max);	/* the valid range widens */
	    ++(dest->delta_nknots);	/* increment the extra knot value counter */
	 }
      }
   dest->unified_nknots = new_cnt;
   qsort((void *) dest_knot, (size_t) new_cnt, (size_t) sizeof(GLfloat),
	 &knot_sort);
}

/* basing on the new common knot range for all attributes set */
/* t_min and t_max values for each knot - they will be used later on */
/* by explode_knot() and calc_new_ctrl_pts */
static void
set_new_t_min_t_max(knot_str_type * geom_knot, knot_str_type * color_knot,
		    knot_str_type * normal_knot, knot_str_type * texture_knot,
		    GLfloat maximal_min_knot, GLfloat minimal_max_knot)
{
   GLuint t_min = 0, t_max = 0, cnt = 0;

   if (minimal_max_knot - maximal_min_knot < EPSILON) {
      /* knot common range empty */
      geom_knot->t_min = geom_knot->t_max = 0;
      color_knot->t_min = color_knot->t_max = 0;
      normal_knot->t_min = normal_knot->t_max = 0;
      texture_knot->t_min = texture_knot->t_max = 0;
   }
   else {
      if (geom_knot->unified_knot != NULL) {
	 cnt = geom_knot->unified_nknots;
	 for (t_min = 0; t_min < cnt; t_min++)
	    if (fabs((geom_knot->unified_knot)[t_min] - maximal_min_knot) <
		EPSILON) break;
	 for (t_max = cnt - 1; t_max; t_max--)
	    if (fabs((geom_knot->unified_knot)[t_max] - minimal_max_knot) <
		EPSILON) break;
      }
      else if (geom_knot->nknots) {
	 cnt = geom_knot->nknots;
	 for (t_min = 0; t_min < cnt; t_min++)
	    if (fabs((geom_knot->knot)[t_min] - maximal_min_knot) < EPSILON)
	       break;
	 for (t_max = cnt - 1; t_max; t_max--)
	    if (fabs((geom_knot->knot)[t_max] - minimal_max_knot) < EPSILON)
	       break;
      }
      geom_knot->t_min = t_min;
      geom_knot->t_max = t_max;
      if (color_knot->unified_knot != NULL) {
	 cnt = color_knot->unified_nknots;
	 for (t_min = 0; t_min < cnt; t_min++)
	    if (fabs((color_knot->unified_knot)[t_min] - maximal_min_knot) <
		EPSILON) break;
	 for (t_max = cnt - 1; t_max; t_max--)
	    if (fabs((color_knot->unified_knot)[t_max] - minimal_max_knot) <
		EPSILON) break;
	 color_knot->t_min = t_min;
	 color_knot->t_max = t_max;
      }
      if (normal_knot->unified_knot != NULL) {
	 cnt = normal_knot->unified_nknots;
	 for (t_min = 0; t_min < cnt; t_min++)
	    if (fabs((normal_knot->unified_knot)[t_min] - maximal_min_knot) <
		EPSILON) break;
	 for (t_max = cnt - 1; t_max; t_max--)
	    if (fabs((normal_knot->unified_knot)[t_max] - minimal_max_knot) <
		EPSILON) break;
	 normal_knot->t_min = t_min;
	 normal_knot->t_max = t_max;
      }
      if (texture_knot->unified_knot != NULL) {
	 cnt = texture_knot->unified_nknots;
	 for (t_min = 0; t_min < cnt; t_min++)
	    if (fabs((texture_knot->unified_knot)[t_min] - maximal_min_knot)
		< EPSILON)
	       break;
	 for (t_max = cnt - 1; t_max; t_max--)
	    if (fabs((texture_knot->unified_knot)[t_max] - minimal_max_knot)
		< EPSILON)
	       break;
	 texture_knot->t_min = t_min;
	 texture_knot->t_max = t_max;
      }
   }
}

/* modify all knot valid ranges in such a way that all have the same */
/* range, common to all knots */
/* do this by knot insertion */
GLenum
select_knot_working_range(GLUnurbsObj * nobj, knot_str_type * geom_knot,
			  knot_str_type * color_knot,
			  knot_str_type * normal_knot,
			  knot_str_type * texture_knot)
{
   GLint max_nknots;
   GLfloat maximal_min_knot, minimal_max_knot;
   GLint i;

   /* find the maximum modified knot length */
   max_nknots = geom_knot->nknots;
   if (color_knot->unified_knot)
      max_nknots += color_knot->nknots;
   if (normal_knot->unified_knot)
      max_nknots += normal_knot->nknots;
   if (texture_knot->unified_knot)
      max_nknots += texture_knot->nknots;
   maximal_min_knot = (geom_knot->knot)[geom_knot->t_min];
   minimal_max_knot = (geom_knot->knot)[geom_knot->t_max];
   /* any attirb data ? */
   if (max_nknots != geom_knot->nknots) {
      /* allocate space for the unified knots */
      if ((geom_knot->unified_knot =
	   (GLfloat *) malloc(sizeof(GLfloat) * max_nknots)) == NULL) {
	 call_user_error(nobj, GLU_OUT_OF_MEMORY);
	 return GLU_ERROR;
      }
      /* copy the original knot to the unified one */
      geom_knot->unified_nknots = geom_knot->nknots;
      for (i = 0; i < geom_knot->nknots; i++)
	 (geom_knot->unified_knot)[i] = (geom_knot->knot)[i];
      if (color_knot->unified_knot) {
	 if ((color_knot->knot)[color_knot->t_min] - maximal_min_knot >
	     EPSILON)
	       maximal_min_knot = (color_knot->knot)[color_knot->t_min];
	 if (minimal_max_knot - (color_knot->knot)[color_knot->t_max] >
	     EPSILON)
	       minimal_max_knot = (color_knot->knot)[color_knot->t_max];
	 if ((color_knot->unified_knot =
	      (GLfloat *) malloc(sizeof(GLfloat) * max_nknots)) == NULL) {
	    free(geom_knot->unified_knot);
	    call_user_error(nobj, GLU_OUT_OF_MEMORY);
	    return GLU_ERROR;
	 }
	 /* copy the original knot to the unified one */
	 color_knot->unified_nknots = color_knot->nknots;
	 for (i = 0; i < color_knot->nknots; i++)
	    (color_knot->unified_knot)[i] = (color_knot->knot)[i];
      }
      if (normal_knot->unified_knot) {
	 if ((normal_knot->knot)[normal_knot->t_min] - maximal_min_knot >
	     EPSILON)
	       maximal_min_knot = (normal_knot->knot)[normal_knot->t_min];
	 if (minimal_max_knot - (normal_knot->knot)[normal_knot->t_max] >
	     EPSILON)
	       minimal_max_knot = (normal_knot->knot)[normal_knot->t_max];
	 if ((normal_knot->unified_knot =
	      (GLfloat *) malloc(sizeof(GLfloat) * max_nknots)) == NULL) {
	    free(geom_knot->unified_knot);
	    free(color_knot->unified_knot);
	    call_user_error(nobj, GLU_OUT_OF_MEMORY);
	    return GLU_ERROR;
	 }
	 /* copy the original knot to the unified one */
	 normal_knot->unified_nknots = normal_knot->nknots;
	 for (i = 0; i < normal_knot->nknots; i++)
	    (normal_knot->unified_knot)[i] = (normal_knot->knot)[i];
      }
      if (texture_knot->unified_knot) {
	 if ((texture_knot->knot)[texture_knot->t_min] - maximal_min_knot >
	     EPSILON)
	       maximal_min_knot = (texture_knot->knot)[texture_knot->t_min];
	 if (minimal_max_knot - (texture_knot->knot)[texture_knot->t_max] >
	     EPSILON)
	       minimal_max_knot = (texture_knot->knot)[texture_knot->t_max];
	 if ((texture_knot->unified_knot =
	      (GLfloat *) malloc(sizeof(GLfloat) * max_nknots)) == NULL) {
	    free(geom_knot->unified_knot);
	    free(color_knot->unified_knot);
	    free(normal_knot->unified_knot);
	    call_user_error(nobj, GLU_OUT_OF_MEMORY);
	    return GLU_ERROR;
	 }
	 /* copy the original knot to the unified one */
	 texture_knot->unified_nknots = texture_knot->nknots;
	 for (i = 0; i < texture_knot->nknots; i++)
	    (texture_knot->unified_knot)[i] = (texture_knot->knot)[i];
      }
      /* work on the geometry knot with all additional knot values */
      /* appearing in attirbutive knots */
      if (minimal_max_knot - maximal_min_knot < EPSILON) {
	 /* empty working range */
	 geom_knot->unified_nknots = 0;
	 color_knot->unified_nknots = 0;
	 normal_knot->unified_nknots = 0;
	 texture_knot->unified_nknots = 0;
      }
      else {
	 if (color_knot->unified_knot)
	    collect_unified_knot(geom_knot, color_knot, maximal_min_knot,
				 minimal_max_knot);
	 if (normal_knot->unified_knot)
	    collect_unified_knot(geom_knot, normal_knot, maximal_min_knot,
				 minimal_max_knot);
	 if (texture_knot->unified_knot)
	    collect_unified_knot(geom_knot, texture_knot, maximal_min_knot,
				 minimal_max_knot);
	 /* since we have now built the "unified" geometry knot */
	 /* add same knot values to all attributive knots */
	 if (color_knot->unified_knot)
	    collect_unified_knot(color_knot, geom_knot, maximal_min_knot,
				 minimal_max_knot);
	 if (normal_knot->unified_knot)
	    collect_unified_knot(normal_knot, geom_knot, maximal_min_knot,
				 minimal_max_knot);
	 if (texture_knot->unified_knot)
	    collect_unified_knot(texture_knot, geom_knot, maximal_min_knot,
				 minimal_max_knot);
      }
   }
   set_new_t_min_t_max(geom_knot, color_knot, normal_knot, texture_knot,
		       maximal_min_knot, minimal_max_knot);
   return GLU_NO_ERROR;
}

void
free_unified_knots(knot_str_type * geom_knot, knot_str_type * color_knot,
		   knot_str_type * normal_knot, knot_str_type * texture_knot)
{
   if (geom_knot->unified_knot)
      free(geom_knot->unified_knot);
   if (color_knot->unified_knot)
      free(color_knot->unified_knot);
   if (normal_knot->unified_knot)
      free(normal_knot->unified_knot);
   if (texture_knot->unified_knot)
      free(texture_knot->unified_knot);
}

GLenum explode_knot(knot_str_type * the_knot)
{
   GLfloat *knot, *new_knot;
   GLint nknots, n_new_knots = 0;
   GLint t_min, t_max;
   GLint ord;
   GLsizei i, j, k;
   GLfloat tmp_float;

   if (the_knot->unified_knot) {
      knot = the_knot->unified_knot;
      nknots = the_knot->unified_nknots;
   }
   else {
      knot = the_knot->knot;
      nknots = the_knot->nknots;
   }
   ord = the_knot->order;
   t_min = the_knot->t_min;
   t_max = the_knot->t_max;

   for (i = t_min; i <= t_max;) {
      tmp_float = knot[i];
      for (j = 0; j < ord && (i + j) <= t_max; j++)
	 if (fabs(tmp_float - knot[i + j]) > EPSILON)
	    break;
      n_new_knots += ord - j;
      i += j;
   }
   /* alloc space for new_knot */
   if (
       (new_knot =
	(GLfloat *) malloc(sizeof(GLfloat) * (nknots + n_new_knots + 1))) == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   /* fill in new knot */
   for (j = 0; j < t_min; j++)
      new_knot[j] = knot[j];
   for (i = j; i <= t_max; i++) {
      tmp_float = knot[i];
      for (k = 0; k < ord; k++) {
	 new_knot[j++] = knot[i];
	 if (tmp_float == knot[i + 1])
	    i++;
      }
   }
   for (i = t_max + 1; i < (int) nknots; i++)
      new_knot[j++] = knot[i];
   /* fill in the knot structure */
   the_knot->new_knot = new_knot;
   the_knot->delta_nknots += n_new_knots;
   the_knot->t_max += n_new_knots;
   return GLU_NO_ERROR;
}

GLenum calc_alphas(knot_str_type * the_knot)
{
   GLfloat tmp_float;
   int i, j, k, m, n;
   int order;
   GLfloat *alpha, *alpha_new, *tmp_alpha;
   GLfloat denom;
   GLfloat *knot, *new_knot;


   knot = the_knot->knot;
   order = the_knot->order;
   new_knot = the_knot->new_knot;
   n = the_knot->nknots - the_knot->order;
   m = n + the_knot->delta_nknots;
   if ((alpha = (GLfloat *) malloc(sizeof(GLfloat) * n * m)) == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   if ((alpha_new = (GLfloat *) malloc(sizeof(GLfloat) * n * m)) == NULL) {
      free(alpha);
      return GLU_OUT_OF_MEMORY;
   }
   for (j = 0; j < m; j++) {
      for (i = 0; i < n; i++) {
	 if ((knot[i] <= new_knot[j]) && (new_knot[j] < knot[i + 1]))
	    tmp_float = 1.0;
	 else
	    tmp_float = 0.0;
	 alpha[i + j * n] = tmp_float;
      }
   }
   for (k = 1; k < order; k++) {
      for (j = 0; j < m; j++)
	 for (i = 0; i < n; i++) {
	    denom = knot[i + k] - knot[i];
	    if (fabs(denom) < EPSILON)
	       tmp_float = 0.0;
	    else
	       tmp_float = (new_knot[j + k] - knot[i]) / denom *
		  alpha[i + j * n];
	    denom = knot[i + k + 1] - knot[i + 1];
	    if (fabs(denom) > EPSILON)
	       tmp_float += (knot[i + k + 1] - new_knot[j + k]) / denom *
		  alpha[(i + 1) + j * n];
	    alpha_new[i + j * n] = tmp_float;
	 }
      tmp_alpha = alpha_new;
      alpha_new = alpha;
      alpha = tmp_alpha;
   }
   the_knot->alpha = alpha;
   free(alpha_new);
   return GLU_NO_ERROR;
}

GLenum
calc_new_ctrl_pts(GLfloat * ctrl, GLint stride, knot_str_type * the_knot,
		  GLint dim, GLfloat ** new_ctrl, GLint * ncontrol)
{
   GLsizei i, j, k, l, m, n;
   GLsizei index1, index2;
   GLfloat *alpha;
   GLfloat *new_knot;

   new_knot = the_knot->new_knot;
   n = the_knot->nknots - the_knot->order;
   alpha = the_knot->alpha;

   m = the_knot->t_max + 1 - the_knot->t_min - the_knot->order;
   k = the_knot->t_min;
   /* allocate space for new control points */
   if ((*new_ctrl = (GLfloat *) malloc(sizeof(GLfloat) * dim * m)) == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   for (j = 0; j < m; j++) {
      for (l = 0; l < dim; l++)
	 (*new_ctrl)[j * dim + l] = 0.0;
      for (i = 0; i < n; i++) {
	 index1 = i + (j + k) * n;
	 index2 = i * stride;
	 for (l = 0; l < dim; l++)
	    (*new_ctrl)[j * dim + l] += alpha[index1] * ctrl[index2 + l];
      }
   }
   *ncontrol = (GLint) m;
   return GLU_NO_ERROR;
}

static GLint
calc_factor(GLfloat * pts, GLint order, GLint indx, GLint stride,
	    GLfloat tolerance, GLint dim)
{
   GLdouble model[16], proj[16];
   GLint viewport[4];
   GLdouble x, y, z, w, winx1, winy1, winz, winx2, winy2;
   GLint i;
   GLdouble len, dx, dy;

   glGetDoublev(GL_MODELVIEW_MATRIX, model);
   glGetDoublev(GL_PROJECTION_MATRIX, proj);
   glGetIntegerv(GL_VIEWPORT, viewport);
   if (dim == 4) {
      w = (GLdouble) pts[indx + 3];
      x = (GLdouble) pts[indx] / w;
      y = (GLdouble) pts[indx + 1] / w;
      z = (GLdouble) pts[indx + 2] / w;
      gluProject(x, y, z, model, proj, viewport, &winx1, &winy1, &winz);
      len = 0.0;
      for (i = 1; i < order; i++) {
	 w = (GLdouble) pts[indx + i * stride + 3];
	 x = (GLdouble) pts[indx + i * stride] / w;
	 y = (GLdouble) pts[indx + i * stride + 1] / w;
	 z = (GLdouble) pts[indx + i * stride + 2] / w;
	 if (gluProject
	     (x, y, z, model, proj, viewport, &winx2, &winy2, &winz)) {
	    dx = winx2 - winx1;
	    dy = winy2 - winy1;
	    len += sqrt(dx * dx + dy * dy);
	 }
	 winx1 = winx2;
	 winy1 = winy2;
      }
   }
   else {
      x = (GLdouble) pts[indx];
      y = (GLdouble) pts[indx + 1];
      if (dim == 2)
	 z = 0.0;
      else
	 z = (GLdouble) pts[indx + 2];
      gluProject(x, y, z, model, proj, viewport, &winx1, &winy1, &winz);
      len = 0.0;
      for (i = 1; i < order; i++) {
	 x = (GLdouble) pts[indx + i * stride];
	 y = (GLdouble) pts[indx + i * stride + 1];
	 if (dim == 2)
	    z = 0.0;
	 else
	    z = (GLdouble) pts[indx + i * stride + 2];
	 if (gluProject
	     (x, y, z, model, proj, viewport, &winx2, &winy2, &winz)) {
	    dx = winx2 - winx1;
	    dy = winy2 - winy1;
	    len += sqrt(dx * dx + dy * dy);
	 }
	 winx1 = winx2;
	 winy1 = winy2;
      }
   }
   len /= tolerance;
   return ((GLint) len + 1);
}

/* we can't use the Mesa evaluators - no way to get the point coords */
/* so we use our own Bezier point calculus routines */
/* because I'm lazy, I reuse the ones from eval.c */

static void
bezier_curve(GLfloat * cp, GLfloat * out, GLfloat t,
	     GLuint dim, GLuint order, GLint offset)
{
   GLfloat s, powert;
   GLuint i, k, bincoeff;

   if (order >= 2) {
      bincoeff = order - 1;
      s = 1.0 - t;

      for (k = 0; k < dim; k++)
	 out[k] = s * cp[k] + bincoeff * t * cp[offset + k];

      for (i = 2, cp += 2 * offset, powert = t * t; i < order;
	   i++, powert *= t, cp += offset) {
	 bincoeff *= order - i;
	 bincoeff /= i;

	 for (k = 0; k < dim; k++)
	    out[k] = s * out[k] + bincoeff * powert * cp[k];
      }
   }
   else {			/* order=1 -> constant curve */

      for (k = 0; k < dim; k++)
	 out[k] = cp[k];
   }
}

static GLint
calc_parametric_factor(GLfloat * pts, GLint order, GLint indx, GLint stride,
		       GLfloat tolerance, GLint dim)
{
   GLdouble model[16], proj[16];
   GLint viewport[4];
   GLdouble x, y, z, w, x1, y1, z1, x2, y2, z2, x3, y3, z3;
   GLint i;
   GLint P;
   GLfloat bez_pt[4];
   GLdouble len = 0.0, tmp, z_med;

   P = 2 * (order + 2);
   glGetDoublev(GL_MODELVIEW_MATRIX, model);
   glGetDoublev(GL_PROJECTION_MATRIX, proj);
   glGetIntegerv(GL_VIEWPORT, viewport);
   z_med = (viewport[2] + viewport[3]) * 0.5;
   switch (dim) {
   case 4:
      for (i = 1; i < P; i++) {
	 bezier_curve(pts + indx, bez_pt, (GLfloat) i / (GLfloat) P, 4,
		      order, stride);
	 w = (GLdouble) bez_pt[3];
	 x = (GLdouble) bez_pt[0] / w;
	 y = (GLdouble) bez_pt[1] / w;
	 z = (GLdouble) bez_pt[2] / w;
	 gluProject(x, y, z, model, proj, viewport, &x3, &y3, &z3);
	 z3 *= z_med;
	 bezier_curve(pts + indx, bez_pt, (GLfloat) (i - 1) / (GLfloat) P, 4,
		      order, stride);
	 w = (GLdouble) bez_pt[3];
	 x = (GLdouble) bez_pt[0] / w;
	 y = (GLdouble) bez_pt[1] / w;
	 z = (GLdouble) bez_pt[2] / w;
	 gluProject(x, y, z, model, proj, viewport, &x1, &y1, &z1);
	 z1 *= z_med;
	 bezier_curve(pts + indx, bez_pt, (GLfloat) (i + 1) / (GLfloat) P, 4,
		      order, stride);
	 w = (GLdouble) bez_pt[3];
	 x = (GLdouble) bez_pt[0] / w;
	 y = (GLdouble) bez_pt[1] / w;
	 z = (GLdouble) bez_pt[2] / w;
	 gluProject(x, y, z, model, proj, viewport, &x2, &y2, &z2);
	 z2 *= z_med;
	 /* calc distance between point (x3,y3,z3) and line segment */
	 /* <x1,y1,z1><x2,y2,z2> */
	 x = x2 - x1;
	 y = y2 - y1;
	 z = z2 - z1;
	 tmp = sqrt(x * x + y * y + z * z);
	 x /= tmp;
	 y /= tmp;
	 z /= tmp;
	 tmp = x3 * x + y3 * y + z3 * z - x1 * x - y1 * y - z1 * z;
	 x = x1 + x * tmp - x3;
	 y = y1 + y * tmp - y3;
	 z = z1 + z * tmp - z3;
	 tmp = sqrt(x * x + y * y + z * z);
	 if (tmp > len)
	    len = tmp;
      }
      break;
   case 3:
      for (i = 1; i < P; i++) {
	 bezier_curve(pts + indx, bez_pt, (GLfloat) i / (GLfloat) P, 3,
		      order, stride);
	 x = (GLdouble) bez_pt[0];
	 y = (GLdouble) bez_pt[1];
	 z = (GLdouble) bez_pt[2];
	 gluProject(x, y, z, model, proj, viewport, &x3, &y3, &z3);
	 z3 *= z_med;
	 bezier_curve(pts + indx, bez_pt, (GLfloat) (i - 1) / (GLfloat) P, 3,
		      order, stride);
	 x = (GLdouble) bez_pt[0];
	 y = (GLdouble) bez_pt[1];
	 z = (GLdouble) bez_pt[2];
	 gluProject(x, y, z, model, proj, viewport, &x1, &y1, &z1);
	 z1 *= z_med;
	 bezier_curve(pts + indx, bez_pt, (GLfloat) (i + 1) / (GLfloat) P, 3,
		      order, stride);
	 x = (GLdouble) bez_pt[0];
	 y = (GLdouble) bez_pt[1];
	 z = (GLdouble) bez_pt[2];
	 gluProject(x, y, z, model, proj, viewport, &x2, &y2, &z2);
	 z2 *= z_med;
	 /* calc distance between point (x3,y3,z3) and line segment */
	 /* <x1,y1,z1><x2,y2,z2> */
	 x = x2 - x1;
	 y = y2 - y1;
	 z = z2 - z1;
	 tmp = sqrt(x * x + y * y + z * z);
	 x /= tmp;
	 y /= tmp;
	 z /= tmp;
	 tmp = x3 * x + y3 * y + z3 * z - x1 * x - y1 * y - z1 * z;
	 x = x1 + x * tmp - x3;
	 y = y1 + y * tmp - y3;
	 z = z1 + z * tmp - z3;
	 tmp = sqrt(x * x + y * y + z * z);
	 if (tmp > len)
	    len = tmp;
      }
      break;
   case 2:
      for (i = 1; i < P; i++) {
	 bezier_curve(pts + indx, bez_pt, (GLfloat) i / (GLfloat) P, 2,
		      order, stride);
	 x = (GLdouble) bez_pt[0];
	 y = (GLdouble) bez_pt[1];
	 z = 0.0;
	 gluProject(x, y, z, model, proj, viewport, &x3, &y3, &z3);
	 z3 *= z_med;
	 bezier_curve(pts + indx, bez_pt, (GLfloat) (i - 1) / (GLfloat) P, 2,
		      order, stride);
	 x = (GLdouble) bez_pt[0];
	 y = (GLdouble) bez_pt[1];
	 z = 0.0;
	 gluProject(x, y, z, model, proj, viewport, &x1, &y1, &z1);
	 z1 *= z_med;
	 bezier_curve(pts + indx, bez_pt, (GLfloat) (i + 1) / (GLfloat) P, 2,
		      order, stride);
	 x = (GLdouble) bez_pt[0];
	 y = (GLdouble) bez_pt[1];
	 z = 0.0;
	 gluProject(x, y, z, model, proj, viewport, &x2, &y2, &z2);
	 z2 *= z_med;
	 /* calc distance between point (x3,y3,z3) and line segment */
	 /* <x1,y1,z1><x2,y2,z2> */
	 x = x2 - x1;
	 y = y2 - y1;
	 z = z2 - z1;
	 tmp = sqrt(x * x + y * y + z * z);
	 x /= tmp;
	 y /= tmp;
	 z /= tmp;
	 tmp = x3 * x + y3 * y + z3 * z - x1 * x - y1 * y - z1 * z;
	 x = x1 + x * tmp - x3;
	 y = y1 + y * tmp - y3;
	 z = z1 + z * tmp - z3;
	 tmp = sqrt(x * x + y * y + z * z);
	 if (tmp > len)
	    len = tmp;
      }
      break;

   }
   if (len < tolerance)
      return (order);
   else
      return (GLint) (sqrt(len / tolerance) * (order + 2) + 1);
}

static GLenum
calc_sampling_3D(new_ctrl_type * new_ctrl, GLfloat tolerance, GLint dim,
		 GLint uorder, GLint vorder, GLint ** ufactors,
		 GLint ** vfactors)
{
   GLfloat *ctrl;
   GLint tmp_factor1, tmp_factor2;
   GLint ufactor_cnt, vfactor_cnt;
   GLint offset1, offset2, offset3;
   GLint i, j;

   ufactor_cnt = new_ctrl->s_bezier_cnt;
   vfactor_cnt = new_ctrl->t_bezier_cnt;
   if ((*ufactors = (GLint *) malloc(sizeof(GLint) * ufactor_cnt * 3))
       == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   if ((*vfactors = (GLint *) malloc(sizeof(GLint) * vfactor_cnt * 3))
       == NULL) {
      free(*ufactors);
      return GLU_OUT_OF_MEMORY;
   }
   ctrl = new_ctrl->geom_ctrl;
   offset1 = new_ctrl->geom_t_stride * vorder;
   offset2 = new_ctrl->geom_s_stride * uorder;
   for (j = 0; j < vfactor_cnt; j++) {
      *(*vfactors + j * 3 + 1) = tmp_factor1 = calc_factor(ctrl, vorder,
							   j * offset1, dim,
							   tolerance, dim);
      /* loop ufactor_cnt-1 times */
      for (i = 1; i < ufactor_cnt; i++) {
	 tmp_factor2 = calc_factor(ctrl, vorder,
				   j * offset1 + i * offset2, dim, tolerance,
				   dim);
	 if (tmp_factor2 > tmp_factor1)
	    tmp_factor1 = tmp_factor2;
      }
      /* last time for the opposite edge */
      *(*vfactors + j * 3 + 2) = tmp_factor2 = calc_factor(ctrl, vorder,
							   j * offset1 +
							   i * offset2 -
							   new_ctrl->
							   geom_s_stride, dim,
							   tolerance, dim);
      if (tmp_factor2 > tmp_factor1)
	 *(*vfactors + j * 3) = tmp_factor2;
      else
	 *(*vfactors + j * 3) = tmp_factor1;
   }
   offset3 = new_ctrl->geom_s_stride;
   offset2 = new_ctrl->geom_s_stride * uorder;
   for (j = 0; j < ufactor_cnt; j++) {
      *(*ufactors + j * 3 + 1) = tmp_factor1 = calc_factor(ctrl, uorder,
							   j * offset2,
							   offset3, tolerance,
							   dim);
      /* loop vfactor_cnt-1 times */
      for (i = 1; i < vfactor_cnt; i++) {
	 tmp_factor2 = calc_factor(ctrl, uorder,
				   j * offset2 + i * offset1, offset3,
				   tolerance, dim);
	 if (tmp_factor2 > tmp_factor1)
	    tmp_factor1 = tmp_factor2;
      }
      /* last time for the opposite edge */
      *(*ufactors + j * 3 + 2) = tmp_factor2 = calc_factor(ctrl, uorder,
							   j * offset2 +
							   i * offset1 -
							   new_ctrl->
							   geom_t_stride,
							   offset3, tolerance,
							   dim);
      if (tmp_factor2 > tmp_factor1)
	 *(*ufactors + j * 3) = tmp_factor2;
      else
	 *(*ufactors + j * 3) = tmp_factor1;
   }
   return GL_NO_ERROR;
}

static GLenum
calc_sampling_param_3D(new_ctrl_type * new_ctrl, GLfloat tolerance, GLint dim,
		       GLint uorder, GLint vorder, GLint ** ufactors,
		       GLint ** vfactors)
{
   GLfloat *ctrl;
   GLint tmp_factor1, tmp_factor2;
   GLint ufactor_cnt, vfactor_cnt;
   GLint offset1, offset2, offset3;
   GLint i, j;

   ufactor_cnt = new_ctrl->s_bezier_cnt;
   vfactor_cnt = new_ctrl->t_bezier_cnt;
   if ((*ufactors = (GLint *) malloc(sizeof(GLint) * ufactor_cnt * 3))
       == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   if ((*vfactors = (GLint *) malloc(sizeof(GLint) * vfactor_cnt * 3))
       == NULL) {
      free(*ufactors);
      return GLU_OUT_OF_MEMORY;
   }
   ctrl = new_ctrl->geom_ctrl;
   offset1 = new_ctrl->geom_t_stride * vorder;
   offset2 = new_ctrl->geom_s_stride * uorder;
   for (j = 0; j < vfactor_cnt; j++) {
      *(*vfactors + j * 3 + 1) = tmp_factor1 =
	 calc_parametric_factor(ctrl, vorder, j * offset1, dim, tolerance,
				dim);
      /* loop ufactor_cnt-1 times */
      for (i = 1; i < ufactor_cnt; i++) {
	 tmp_factor2 = calc_parametric_factor(ctrl, vorder,
					      j * offset1 + i * offset2, dim,
					      tolerance, dim);
	 if (tmp_factor2 > tmp_factor1)
	    tmp_factor1 = tmp_factor2;
      }
      /* last time for the opposite edge */
      *(*vfactors + j * 3 + 2) = tmp_factor2 =
	 calc_parametric_factor(ctrl, vorder,
				j * offset1 + i * offset2 -
				new_ctrl->geom_s_stride, dim, tolerance, dim);
      if (tmp_factor2 > tmp_factor1)
	 *(*vfactors + j * 3) = tmp_factor2;
      else
	 *(*vfactors + j * 3) = tmp_factor1;
   }
   offset3 = new_ctrl->geom_s_stride;
   offset2 = new_ctrl->geom_s_stride * uorder;
   for (j = 0; j < ufactor_cnt; j++) {
      *(*ufactors + j * 3 + 1) = tmp_factor1 =
	 calc_parametric_factor(ctrl, uorder, j * offset2, offset3, tolerance,
				dim);
      /* loop vfactor_cnt-1 times */
      for (i = 1; i < vfactor_cnt; i++) {
	 tmp_factor2 = calc_parametric_factor(ctrl, uorder,
					      j * offset2 + i * offset1,
					      offset3, tolerance, dim);
	 if (tmp_factor2 > tmp_factor1)
	    tmp_factor1 = tmp_factor2;
      }
      /* last time for the opposite edge */
      *(*ufactors + j * 3 + 2) = tmp_factor2 =
	 calc_parametric_factor(ctrl, uorder,
				j * offset2 + i * offset1 -
				new_ctrl->geom_t_stride, offset3, tolerance,
				dim);
      if (tmp_factor2 > tmp_factor1)
	 *(*ufactors + j * 3) = tmp_factor2;
      else
	 *(*ufactors + j * 3) = tmp_factor1;
   }
   return GL_NO_ERROR;
}

static GLenum
calc_sampling_2D(GLfloat * ctrl, GLint cnt, GLint order,
		 GLfloat tolerance, GLint dim, GLint ** factors)
{
   GLint factor_cnt;
   GLint tmp_factor;
   GLint offset;
   GLint i;

   factor_cnt = cnt / order;
   if ((*factors = (GLint *) malloc(sizeof(GLint) * factor_cnt)) == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   offset = order * dim;
   for (i = 0; i < factor_cnt; i++) {
      tmp_factor = calc_factor(ctrl, order, i * offset, dim, tolerance, dim);
      if (tmp_factor == 0)
	 (*factors)[i] = 1;
      else
	 (*factors)[i] = tmp_factor;
   }
   return GL_NO_ERROR;
}

static void
set_sampling_and_culling(GLUnurbsObj * nobj)
{
   if (nobj->auto_load_matrix == GL_FALSE) {
      GLint i;
      GLfloat m[4];

      glPushAttrib((GLbitfield) (GL_VIEWPORT_BIT | GL_TRANSFORM_BIT));
      for (i = 0; i < 4; i++)
	 m[i] = nobj->sampling_matrices.viewport[i];
      glViewport(m[0], m[1], m[2], m[3]);
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadMatrixf(nobj->sampling_matrices.proj);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadMatrixf(nobj->sampling_matrices.model);
   }
}

static void
revert_sampling_and_culling(GLUnurbsObj * nobj)
{
   if (nobj->auto_load_matrix == GL_FALSE) {
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glPopAttrib();
   }
}

GLenum
glu_do_sampling_3D(GLUnurbsObj * nobj, new_ctrl_type * new_ctrl,
		   GLint ** sfactors, GLint ** tfactors)
{
   GLint dim;
   GLenum err;

   *sfactors = NULL;
   *tfactors = NULL;
   dim = nobj->surface.geom.dim;
   set_sampling_and_culling(nobj);
   if ((err = calc_sampling_3D(new_ctrl, nobj->sampling_tolerance, dim,
			       nobj->surface.geom.sorder,
			       nobj->surface.geom.torder, sfactors,
			       tfactors)) == GLU_ERROR) {
      revert_sampling_and_culling(nobj);
      call_user_error(nobj, err);
      return GLU_ERROR;
   }
   revert_sampling_and_culling(nobj);
   return GLU_NO_ERROR;
}

GLenum
glu_do_sampling_uv(GLUnurbsObj * nobj, new_ctrl_type * new_ctrl,
		   GLint ** sfactors, GLint ** tfactors)
{
   GLint s_cnt, t_cnt, i;
   GLint u_steps, v_steps;

   s_cnt = new_ctrl->s_bezier_cnt;
   t_cnt = new_ctrl->t_bezier_cnt;
   *sfactors = NULL;
   *tfactors = NULL;
   if ((*sfactors = (GLint *) malloc(sizeof(GLint) * s_cnt * 3))
       == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   if ((*tfactors = (GLint *) malloc(sizeof(GLint) * t_cnt * 3))
       == NULL) {
      free(*sfactors);
      return GLU_OUT_OF_MEMORY;
   }
   u_steps = nobj->u_step;
   v_steps = nobj->v_step;
   for (i = 0; i < s_cnt; i++) {
      *(*sfactors + i * 3) = u_steps;
      *(*sfactors + i * 3 + 1) = u_steps;
      *(*sfactors + i * 3 + 2) = u_steps;
   }
   for (i = 0; i < t_cnt; i++) {
      *(*tfactors + i * 3) = v_steps;
      *(*tfactors + i * 3 + 1) = v_steps;
      *(*tfactors + i * 3 + 2) = v_steps;
   }
   return GLU_NO_ERROR;
}


GLenum
glu_do_sampling_param_3D(GLUnurbsObj * nobj, new_ctrl_type * new_ctrl,
			 GLint ** sfactors, GLint ** tfactors)
{
   GLint dim;
   GLenum err;

   *sfactors = NULL;
   *tfactors = NULL;
   dim = nobj->surface.geom.dim;
   set_sampling_and_culling(nobj);
   if (
       (err =
	calc_sampling_param_3D(new_ctrl, nobj->parametric_tolerance, dim,
			       nobj->surface.geom.sorder,
			       nobj->surface.geom.torder, sfactors,
			       tfactors)) == GLU_ERROR) {
      revert_sampling_and_culling(nobj);
      call_user_error(nobj, err);
      return GLU_ERROR;
   }
   revert_sampling_and_culling(nobj);
   return GLU_NO_ERROR;
}


static GLenum
glu_do_sampling_2D(GLUnurbsObj * nobj, GLfloat * ctrl, GLint cnt, GLint order,
		   GLint dim, GLint ** factors)
{
   GLenum err;

   set_sampling_and_culling(nobj);
   err = calc_sampling_2D(ctrl, cnt, order, nobj->sampling_tolerance, dim,
			  factors);
   revert_sampling_and_culling(nobj);
   return err;
}


static GLenum
glu_do_sampling_u(GLUnurbsObj * nobj, GLfloat * ctrl, GLint cnt, GLint order,
		  GLint dim, GLint ** factors)
{
   GLint i;
   GLint u_steps;

   cnt /= order;
   if ((*factors = (GLint *) malloc(sizeof(GLint) * cnt))
       == NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   u_steps = nobj->u_step;
   for (i = 0; i < cnt; i++)
      (*factors)[i] = u_steps;
   return GLU_NO_ERROR;
}


static GLenum
glu_do_sampling_param_2D(GLUnurbsObj * nobj, GLfloat * ctrl, GLint cnt,
			 GLint order, GLint dim, GLint ** factors)
{
   GLint i;
   GLint u_steps;
   GLfloat tolerance;

   set_sampling_and_culling(nobj);
   tolerance = nobj->parametric_tolerance;
   cnt /= order;
   if ((*factors = (GLint *) malloc(sizeof(GLint) * cnt))
       == NULL) {
      revert_sampling_and_culling(nobj);
      return GLU_OUT_OF_MEMORY;
   }
   u_steps = nobj->u_step;
   for (i = 0; i < cnt; i++) {
      (*factors)[i] = calc_parametric_factor(ctrl, order, 0,
					     dim, tolerance, dim);

   }
   revert_sampling_and_culling(nobj);
   return GLU_NO_ERROR;
}

GLenum
glu_do_sampling_crv(GLUnurbsObj * nobj, GLfloat * ctrl, GLint cnt,
		    GLint order, GLint dim, GLint ** factors)
{
   GLenum err;

   *factors = NULL;
   switch (nobj->sampling_method) {
   case GLU_PATH_LENGTH:
      if ((err = glu_do_sampling_2D(nobj, ctrl, cnt, order, dim, factors)) !=
	  GLU_NO_ERROR) {
	 call_user_error(nobj, err);
	 return GLU_ERROR;
      }
      break;
   case GLU_DOMAIN_DISTANCE:
      if ((err = glu_do_sampling_u(nobj, ctrl, cnt, order, dim, factors)) !=
	  GLU_NO_ERROR) {
	 call_user_error(nobj, err);
	 return GLU_ERROR;
      }
      break;
   case GLU_PARAMETRIC_ERROR:
      if (
	  (err =
	   glu_do_sampling_param_2D(nobj, ctrl, cnt, order, dim,
				    factors)) != GLU_NO_ERROR) {
	 call_user_error(nobj, err);
	 return GLU_ERROR;
      }
      break;
   default:
      abort();
   }

   return GLU_NO_ERROR;
}

/* TODO - i don't like this culling - this one just tests if at least one */
/* ctrl point lies within the viewport . Also the point_in_viewport() */
/* should be included in the fnctions for efficiency reasons */

static GLboolean
point_in_viewport(GLfloat * pt, GLint dim)
{
   GLdouble model[16], proj[16];
   GLint viewport[4];
   GLdouble x, y, z, w, winx, winy, winz;

   glGetDoublev(GL_MODELVIEW_MATRIX, model);
   glGetDoublev(GL_PROJECTION_MATRIX, proj);
   glGetIntegerv(GL_VIEWPORT, viewport);
   if (dim == 3) {
      x = (GLdouble) pt[0];
      y = (GLdouble) pt[1];
      z = (GLdouble) pt[2];
      gluProject(x, y, z, model, proj, viewport, &winx, &winy, &winz);
   }
   else {
      w = (GLdouble) pt[3];
      x = (GLdouble) pt[0] / w;
      y = (GLdouble) pt[1] / w;
      z = (GLdouble) pt[2] / w;
      gluProject(x, y, z, model, proj, viewport, &winx, &winy, &winz);
   }
   if ((GLint) winx >= viewport[0] && (GLint) winx < viewport[2] &&
       (GLint) winy >= viewport[1] && (GLint) winy < viewport[3])
      return GL_TRUE;
   return GL_FALSE;
}

GLboolean
fine_culling_test_3D(GLUnurbsObj * nobj, GLfloat * pts, GLint s_cnt,
		     GLint t_cnt, GLint s_stride, GLint t_stride, GLint dim)
{
   GLint i, j;

   if (nobj->culling == GL_FALSE)
      return GL_FALSE;
   set_sampling_and_culling(nobj);

   if (dim == 3) {
      for (i = 0; i < s_cnt; i++)
	 for (j = 0; j < t_cnt; j++)
	    if (point_in_viewport(pts + i * s_stride + j * t_stride, dim)) {
	       revert_sampling_and_culling(nobj);
	       return GL_FALSE;
	    }
   }
   else {
      for (i = 0; i < s_cnt; i++)
	 for (j = 0; j < t_cnt; j++)
	    if (point_in_viewport(pts + i * s_stride + j * t_stride, dim)) {
	       revert_sampling_and_culling(nobj);
	       return GL_FALSE;
	    }
   }
   revert_sampling_and_culling(nobj);
   return GL_TRUE;
}

/*GLboolean
fine_culling_test_3D(GLUnurbsObj *nobj,GLfloat *pts,GLint s_cnt,GLint t_cnt,
	GLint s_stride,GLint t_stride, GLint dim)
{
	GLint		visible_cnt;
	GLfloat		feedback_buffer[5];
	GLsizei		buffer_size;
	GLint 		i,j;

	if(nobj->culling==GL_FALSE)
		return GL_FALSE;
	buffer_size=5;
	set_sampling_and_culling(nobj);
	
	glFeedbackBuffer(buffer_size,GL_2D,feedback_buffer);
	glRenderMode(GL_FEEDBACK);
	if(dim==3)
	{
		for(i=0;i<s_cnt;i++)
		{
			glBegin(GL_LINE_LOOP);
			for(j=0;j<t_cnt;j++)
				glVertex3fv(pts+i*s_stride+j*t_stride);
			glEnd();
		}
		for(j=0;j<t_cnt;j++)
		{
			glBegin(GL_LINE_LOOP);
			for(i=0;i<s_cnt;i++)
				glVertex3fv(pts+i*s_stride+j*t_stride);
			glEnd();
		}
	}
	else
	{
		for(i=0;i<s_cnt;i++)
		{
			glBegin(GL_LINE_LOOP);
			for(j=0;j<t_cnt;j++)
				glVertex4fv(pts+i*s_stride+j*t_stride);
			glEnd();
		}
		for(j=0;j<t_cnt;j++)
		{
			glBegin(GL_LINE_LOOP);
			for(i=0;i<s_cnt;i++)
				glVertex4fv(pts+i*s_stride+j*t_stride);
			glEnd();
		}
	}
	visible_cnt=glRenderMode(GL_RENDER);

	revert_sampling_and_culling(nobj);
	return (GLboolean)(visible_cnt==0);
}*/

GLboolean
fine_culling_test_2D(GLUnurbsObj * nobj, GLfloat * pts, GLint cnt,
		     GLint stride, GLint dim)
{
   GLint i;

   if (nobj->culling == GL_FALSE)
      return GL_FALSE;
   set_sampling_and_culling(nobj);

   if (dim == 3) {
      for (i = 0; i < cnt; i++)
	 if (point_in_viewport(pts + i * stride, dim)) {
	    revert_sampling_and_culling(nobj);
	    return GL_FALSE;
	 }
   }
   else {
      for (i = 0; i < cnt; i++)
	 if (point_in_viewport(pts + i * stride, dim)) {
	    revert_sampling_and_culling(nobj);
	    return GL_FALSE;
	 }
   }
   revert_sampling_and_culling(nobj);
   return GL_TRUE;
}

/*GLboolean
fine_culling_test_2D(GLUnurbsObj *nobj,GLfloat *pts,GLint cnt,
	GLint stride, GLint dim)
{
	GLint		visible_cnt;
	GLfloat		feedback_buffer[5];
	GLsizei		buffer_size;
	GLint 		i;

	if(nobj->culling==GL_FALSE)
		return GL_FALSE;
	buffer_size=5;
	set_sampling_and_culling(nobj);
	
	glFeedbackBuffer(buffer_size,GL_2D,feedback_buffer);
	glRenderMode(GL_FEEDBACK);
	glBegin(GL_LINE_LOOP);
	if(dim==3)
	{
		for(i=0;i<cnt;i++)
			glVertex3fv(pts+i*stride);
	}
	else
	{
		for(i=0;i<cnt;i++)
			glVertex4fv(pts+i*stride);
	}
	glEnd();
	visible_cnt=glRenderMode(GL_RENDER);

	revert_sampling_and_culling(nobj);
	return (GLboolean)(visible_cnt==0);
}*/
