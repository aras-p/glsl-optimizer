
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


static int
get_curve_dim(GLenum type)
{
   switch (type) {
   case GL_MAP1_VERTEX_3:
      return 3;
   case GL_MAP1_VERTEX_4:
      return 4;
   case GL_MAP1_INDEX:
      return 1;
   case GL_MAP1_COLOR_4:
      return 4;
   case GL_MAP1_NORMAL:
      return 3;
   case GL_MAP1_TEXTURE_COORD_1:
      return 1;
   case GL_MAP1_TEXTURE_COORD_2:
      return 2;
   case GL_MAP1_TEXTURE_COORD_3:
      return 3;
   case GL_MAP1_TEXTURE_COORD_4:
      return 4;
   default:
      abort();			/* TODO: is this OK? */
   }
   return 0;			/*never get here */
}

static GLenum
test_nurbs_curve(GLUnurbsObj * nobj, curve_attribs * attribs)
{
   GLenum err;
   GLint tmp_int;

   if (attribs->order < 0) {
      call_user_error(nobj, GLU_INVALID_VALUE);
      return GLU_ERROR;
   }
   glGetIntegerv(GL_MAX_EVAL_ORDER, &tmp_int);
   if (attribs->order > tmp_int || attribs->order < 2) {
      call_user_error(nobj, GLU_NURBS_ERROR1);
      return GLU_ERROR;
   }
   if (attribs->knot_count < attribs->order + 2) {
      call_user_error(nobj, GLU_NURBS_ERROR2);
      return GLU_ERROR;
   }
   if (attribs->stride < 0) {
      call_user_error(nobj, GLU_NURBS_ERROR34);
      return GLU_ERROR;
   }
   if (attribs->knot == NULL || attribs->ctrlarray == NULL) {
      call_user_error(nobj, GLU_NURBS_ERROR36);
      return GLU_ERROR;
   }
   if ((err = test_knot(attribs->knot_count, attribs->knot, attribs->order))
       != GLU_NO_ERROR) {
      call_user_error(nobj, err);
      return GLU_ERROR;
   }
   return GLU_NO_ERROR;
}

static GLenum
test_nurbs_curves(GLUnurbsObj * nobj)
{
   /* test the geometric data */
   if (test_nurbs_curve(nobj, &(nobj->curve.geom)) != GLU_NO_ERROR)
      return GLU_ERROR;
   /* now test the attributive data */
   /* color */
   if (nobj->curve.color.type != GLU_INVALID_ENUM)
      if (test_nurbs_curve(nobj, &(nobj->curve.color)) != GLU_NO_ERROR)
	 return GLU_ERROR;
   /* normal */
   if (nobj->curve.normal.type != GLU_INVALID_ENUM)
      if (test_nurbs_curve(nobj, &(nobj->curve.normal)) != GLU_NO_ERROR)
	 return GLU_ERROR;
   /* texture */
   if (nobj->curve.texture.type != GLU_INVALID_ENUM)
      if (test_nurbs_curve(nobj, &(nobj->curve.texture)) != GLU_NO_ERROR)
	 return GLU_ERROR;
   return GLU_NO_ERROR;
}

/* prepare the knot information structures */
static GLenum
fill_knot_structures(GLUnurbsObj * nobj, knot_str_type * geom_knot,
		     knot_str_type * color_knot, knot_str_type * normal_knot,
		     knot_str_type * texture_knot)
{
   GLint order;
   GLfloat *knot;
   GLint nknots;
   GLint t_min, t_max;

   geom_knot->unified_knot = NULL;
   knot = geom_knot->knot = nobj->curve.geom.knot;
   nknots = geom_knot->nknots = nobj->curve.geom.knot_count;
   order = geom_knot->order = nobj->curve.geom.order;
   geom_knot->delta_nknots = 0;
   t_min = geom_knot->t_min = order - 1;
   t_max = geom_knot->t_max = nknots - order;
   if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
      call_user_error(nobj, GLU_NURBS_ERROR3);
      return GLU_ERROR;
   }
   if (fabs(knot[0] - knot[t_min]) < EPSILON) {
      /* knot open at beggining */
      geom_knot->open_at_begin = GL_TRUE;
   }
   else
      geom_knot->open_at_begin = GL_FALSE;
   if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
      /* knot open at end */
      geom_knot->open_at_end = GL_TRUE;
   }
   else
      geom_knot->open_at_end = GL_FALSE;
   if (nobj->curve.color.type != GLU_INVALID_ENUM) {
      color_knot->unified_knot = (GLfloat *) 1;
      knot = color_knot->knot = nobj->curve.color.knot;
      nknots = color_knot->nknots = nobj->curve.color.knot_count;
      order = color_knot->order = nobj->curve.color.order;
      color_knot->delta_nknots = 0;
      t_min = color_knot->t_min = order - 1;
      t_max = color_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 color_knot->open_at_begin = GL_TRUE;
      }
      else
	 color_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 color_knot->open_at_end = GL_TRUE;
      }
      else
	 color_knot->open_at_end = GL_FALSE;
   }
   else
      color_knot->unified_knot = NULL;
   if (nobj->curve.normal.type != GLU_INVALID_ENUM) {
      normal_knot->unified_knot = (GLfloat *) 1;
      knot = normal_knot->knot = nobj->curve.normal.knot;
      nknots = normal_knot->nknots = nobj->curve.normal.knot_count;
      order = normal_knot->order = nobj->curve.normal.order;
      normal_knot->delta_nknots = 0;
      t_min = normal_knot->t_min = order - 1;
      t_max = normal_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 normal_knot->open_at_begin = GL_TRUE;
      }
      else
	 normal_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 normal_knot->open_at_end = GL_TRUE;
      }
      else
	 normal_knot->open_at_end = GL_FALSE;
   }
   else
      normal_knot->unified_knot = NULL;
   if (nobj->curve.texture.type != GLU_INVALID_ENUM) {
      texture_knot->unified_knot = (GLfloat *) 1;
      knot = texture_knot->knot = nobj->curve.texture.knot;
      nknots = texture_knot->nknots = nobj->curve.texture.knot_count;
      order = texture_knot->order = nobj->curve.texture.order;
      texture_knot->delta_nknots = 0;
      t_min = texture_knot->t_min = order - 1;
      t_max = texture_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 texture_knot->open_at_begin = GL_TRUE;
      }
      else
	 texture_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 texture_knot->open_at_end = GL_TRUE;
      }
      else
	 texture_knot->open_at_end = GL_FALSE;
   }
   else
      texture_knot->unified_knot = NULL;
   return GLU_NO_ERROR;
}

/* covert the NURBS curve into a series of adjacent Bezier curves */
static GLenum
convert_curve(knot_str_type * the_knot, curve_attribs * attrib,
	      GLfloat ** new_ctrl, GLint * ncontrol)
{
   GLenum err;

   if ((err = explode_knot(the_knot)) != GLU_NO_ERROR) {
      if (the_knot->unified_knot) {
	 free(the_knot->unified_knot);
	 the_knot->unified_knot = NULL;
      }
      return err;
   }
   if (the_knot->unified_knot) {
      free(the_knot->unified_knot);
      the_knot->unified_knot = NULL;
   }
   if ((err = calc_alphas(the_knot)) != GLU_NO_ERROR) {
      free(the_knot->new_knot);
      return err;
   }
   free(the_knot->new_knot);
   if ((err = calc_new_ctrl_pts(attrib->ctrlarray, attrib->stride, the_knot,
				attrib->dim, new_ctrl, ncontrol))
       != GLU_NO_ERROR) {
      free(the_knot->alpha);
      return err;
   }
   free(the_knot->alpha);
   return GLU_NO_ERROR;
}

/* covert curves - geometry and possible attribute ones into equivalent */
/* sequence of adjacent Bezier curves */
static GLenum
convert_curves(GLUnurbsObj * nobj, GLfloat ** new_geom_ctrl,
	       GLint * ncontrol, GLfloat ** new_color_ctrl,
	       GLfloat ** new_normal_ctrl, GLfloat ** new_texture_ctrl)
{
   knot_str_type geom_knot, color_knot, normal_knot, texture_knot;
   GLint junk;
   GLenum err;

   *new_color_ctrl = *new_normal_ctrl = *new_texture_ctrl = NULL;

   if (fill_knot_structures(nobj, &geom_knot, &color_knot, &normal_knot,
			    &texture_knot) != GLU_NO_ERROR)
      return GLU_ERROR;

   /* unify knots - all knots should have the same number of working */
   /* ranges */
   if (
       (err =
	select_knot_working_range(nobj, &geom_knot, &color_knot, &normal_knot,
				  &texture_knot)) != GLU_NO_ERROR) {
      return err;
   }
   /* convert the geometry curve */
   nobj->curve.geom.dim = get_curve_dim(nobj->curve.geom.type);
   if ((err = convert_curve(&geom_knot, &(nobj->curve.geom), new_geom_ctrl,
			    ncontrol)) != GLU_NO_ERROR) {
      free_unified_knots(&geom_knot, &color_knot, &normal_knot,
			 &texture_knot);
      call_user_error(nobj, err);
      return err;
   }
   /* if additional attributive curves are given convert them as well */
   if (color_knot.unified_knot) {
      nobj->curve.color.dim = get_curve_dim(nobj->curve.color.type);
      if ((err = convert_curve(&color_knot, &(nobj->curve.color),
			       new_color_ctrl, &junk)) != GLU_NO_ERROR) {
	 free_unified_knots(&geom_knot, &color_knot, &normal_knot,
			    &texture_knot);
	 free(*new_geom_ctrl);
	 call_user_error(nobj, err);
	 return err;
      }
   }
   if (normal_knot.unified_knot) {
      nobj->curve.normal.dim = get_curve_dim(nobj->curve.normal.type);
      if ((err = convert_curve(&normal_knot, &(nobj->curve.normal),
			       new_normal_ctrl, &junk)) != GLU_NO_ERROR) {
	 free_unified_knots(&geom_knot, &color_knot, &normal_knot,
			    &texture_knot);
	 free(*new_geom_ctrl);
	 if (*new_color_ctrl)
	    free(*new_color_ctrl);
	 call_user_error(nobj, err);
	 return err;
      }
   }
   if (texture_knot.unified_knot) {
      nobj->curve.texture.dim = get_curve_dim(nobj->curve.texture.type);
      if ((err = convert_curve(&texture_knot, &(nobj->curve.texture),
			       new_texture_ctrl, &junk)) != GLU_NO_ERROR) {
	 free_unified_knots(&geom_knot, &color_knot, &normal_knot,
			    &texture_knot);
	 free(*new_geom_ctrl);
	 if (*new_color_ctrl)
	    free(*new_color_ctrl);
	 if (*new_normal_ctrl)
	    free(*new_normal_ctrl);
	 call_user_error(nobj, err);
	 return err;
      }
   }
   return GLU_NO_ERROR;
}

/* main NURBS curve procedure */
void
do_nurbs_curve(GLUnurbsObj * nobj)
{
   GLint geom_order, color_order = 0, normal_order = 0, texture_order = 0;
   GLenum geom_type;
   GLint n_ctrl;
   GLfloat *new_geom_ctrl, *new_color_ctrl, *new_normal_ctrl,
      *new_texture_ctrl;
   GLfloat *geom_ctrl = 0, *color_ctrl = 0, *normal_ctrl = 0, *texture_ctrl = 0;
   GLint *factors;
   GLint i, j;
   GLint geom_dim, color_dim = 0, normal_dim = 0, texture_dim = 0;

   /* test the user supplied data */
   if (test_nurbs_curves(nobj) != GLU_NO_ERROR)
      return;

   if (convert_curves(nobj, &new_geom_ctrl, &n_ctrl, &new_color_ctrl,
		      &new_normal_ctrl, &new_texture_ctrl) != GLU_NO_ERROR)
      return;

   geom_order = nobj->curve.geom.order;
   geom_type = nobj->curve.geom.type;
   geom_dim = nobj->curve.geom.dim;

   if (glu_do_sampling_crv(nobj, new_geom_ctrl, n_ctrl, geom_order, geom_dim,
			   &factors) != GLU_NO_ERROR) {
      free(new_geom_ctrl);
      if (new_color_ctrl)
	 free(new_color_ctrl);
      if (new_normal_ctrl)
	 free(new_normal_ctrl);
      if (new_texture_ctrl)
	 free(new_texture_ctrl);
      return;
   }
   glEnable(geom_type);
   if (new_color_ctrl) {
      glEnable(nobj->curve.color.type);
      color_dim = nobj->curve.color.dim;
      color_ctrl = new_color_ctrl;
      color_order = nobj->curve.color.order;
   }
   if (new_normal_ctrl) {
      glEnable(nobj->curve.normal.type);
      normal_dim = nobj->curve.normal.dim;
      normal_ctrl = new_normal_ctrl;
      normal_order = nobj->curve.normal.order;
   }
   if (new_texture_ctrl) {
      glEnable(nobj->curve.texture.type);
      texture_dim = nobj->curve.texture.dim;
      texture_ctrl = new_texture_ctrl;
      texture_order = nobj->curve.texture.order;
   }
   for (i = 0, j = 0, geom_ctrl = new_geom_ctrl;
	i < n_ctrl; i += geom_order, j++, geom_ctrl += geom_order * geom_dim) {
      if (fine_culling_test_2D
	  (nobj, geom_ctrl, geom_order, geom_dim, geom_dim)) {
	 color_ctrl += color_order * color_dim;
	 normal_ctrl += normal_order * normal_dim;
	 texture_ctrl += texture_order * texture_dim;
	 continue;
      }
      glMap1f(geom_type, 0.0, 1.0, geom_dim, geom_order, geom_ctrl);
      if (new_color_ctrl) {
	 glMap1f(nobj->curve.color.type, 0.0, 1.0, color_dim,
		 color_order, color_ctrl);
	 color_ctrl += color_order * color_dim;
      }
      if (new_normal_ctrl) {
	 glMap1f(nobj->curve.normal.type, 0.0, 1.0, normal_dim,
		 normal_order, normal_ctrl);
	 normal_ctrl += normal_order * normal_dim;
      }
      if (new_texture_ctrl) {
	 glMap1f(nobj->curve.texture.type, 0.0, 1.0, texture_dim,
		 texture_order, texture_ctrl);
	 texture_ctrl += texture_order * texture_dim;
      }
      glMapGrid1f(factors[j], 0.0, 1.0);
      glEvalMesh1(GL_LINE, 0, factors[j]);
   }
   free(new_geom_ctrl);
   free(factors);
   if (new_color_ctrl)
      free(new_color_ctrl);
   if (new_normal_ctrl)
      free(new_normal_ctrl);
   if (new_texture_ctrl)
      free(new_texture_ctrl);
}
