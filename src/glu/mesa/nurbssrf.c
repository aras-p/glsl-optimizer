
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
#include <string.h>
#include "gluP.h"
#include "nurbs.h"
#endif


static int
get_surface_dim(GLenum type)
{
   switch (type) {
   case GL_MAP2_VERTEX_3:
      return 3;
   case GL_MAP2_VERTEX_4:
      return 4;
   case GL_MAP2_INDEX:
      return 1;
   case GL_MAP2_COLOR_4:
      return 4;
   case GL_MAP2_NORMAL:
      return 3;
   case GL_MAP2_TEXTURE_COORD_1:
      return 1;
   case GL_MAP2_TEXTURE_COORD_2:
      return 2;
   case GL_MAP2_TEXTURE_COORD_3:
      return 3;
   case GL_MAP2_TEXTURE_COORD_4:
      return 4;
   default:
      abort();			/* TODO: is this OK? */
   }
   return 0;			/*never get here */
}

static GLenum
test_nurbs_surface(GLUnurbsObj * nobj, surface_attribs * attrib)
{
   GLenum err;
   GLint tmp_int;

   if (attrib->sorder < 0 || attrib->torder < 0) {
      call_user_error(nobj, GLU_INVALID_VALUE);
      return GLU_ERROR;
   }
   glGetIntegerv(GL_MAX_EVAL_ORDER, &tmp_int);
   if (attrib->sorder > tmp_int || attrib->sorder < 2) {
      call_user_error(nobj, GLU_NURBS_ERROR1);
      return GLU_ERROR;
   }
   if (attrib->torder > tmp_int || attrib->torder < 2) {
      call_user_error(nobj, GLU_NURBS_ERROR1);
      return GLU_ERROR;
   }
   if (attrib->sknot_count < attrib->sorder + 2) {
      call_user_error(nobj, GLU_NURBS_ERROR2);
      return GLU_ERROR;
   }
   if (attrib->tknot_count < attrib->torder + 2) {
      call_user_error(nobj, GLU_NURBS_ERROR2);
      return GLU_ERROR;
   }
   if (attrib->s_stride < 0 || attrib->t_stride < 0) {
      call_user_error(nobj, GLU_NURBS_ERROR34);
      return GLU_ERROR;
   }
   if (attrib->sknot == NULL || attrib->tknot == NULL
       || attrib->ctrlarray == NULL) {
      call_user_error(nobj, GLU_NURBS_ERROR36);
      return GLU_ERROR;
   }
   if ((err = test_knot(attrib->tknot_count, attrib->tknot, attrib->torder))
       != GLU_NO_ERROR) {
      call_user_error(nobj, err);
      return GLU_ERROR;
   }
   if ((err = test_knot(attrib->sknot_count, attrib->sknot, attrib->sorder))
       != GLU_NO_ERROR) {
      call_user_error(nobj, err);
      return GLU_ERROR;
   }
   return GLU_NO_ERROR;
}

static GLenum
test_nurbs_surfaces(GLUnurbsObj * nobj)
{
   /* test the geometric data */
   if (test_nurbs_surface(nobj, &(nobj->surface.geom)) != GLU_NO_ERROR)
      return GLU_ERROR;
   /* now test the attributive data */
   /* color */
   if (nobj->surface.color.type != GLU_INVALID_ENUM)
      if (test_nurbs_surface(nobj, &(nobj->surface.color)) != GLU_NO_ERROR)
	 return GLU_ERROR;
   /* normal */
   if (nobj->surface.normal.type != GLU_INVALID_ENUM)
      if (test_nurbs_surface(nobj, &(nobj->surface.normal)) != GLU_NO_ERROR)
	 return GLU_ERROR;
   /* texture */
   if (nobj->surface.texture.type != GLU_INVALID_ENUM)
      if (test_nurbs_surface(nobj, &(nobj->surface.texture)) != GLU_NO_ERROR)
	 return GLU_ERROR;
   return GLU_NO_ERROR;
}

static GLenum
convert_surf(knot_str_type * s_knot, knot_str_type * t_knot,
	     surface_attribs * attrib, GLfloat ** new_ctrl,
	     GLint * s_n_ctrl, GLint * t_n_ctrl)
{
   GLfloat **tmp_ctrl;
   GLfloat *ctrl_offset;
   GLint tmp_n_control;
   GLint i, j, t_cnt, s_cnt;
   GLint tmp_stride;
   GLint dim;
   GLenum err;

   /* valid range is empty? */
   if ((s_knot->unified_knot != NULL && s_knot->unified_nknots == 0) ||
       (t_knot->unified_knot != NULL && t_knot->unified_nknots == 0)) {
      if (s_knot->unified_knot) {
	 free(s_knot->unified_knot);
	 s_knot->unified_knot = NULL;
      }
      if (t_knot->unified_knot) {
	 free(t_knot->unified_knot);
	 t_knot->unified_knot = NULL;
      }
      *s_n_ctrl = 0;
      *t_n_ctrl = 0;
      return GLU_NO_ERROR;
   }
   t_cnt = attrib->tknot_count - attrib->torder;
   s_cnt = attrib->sknot_count - attrib->sorder;
   if ((tmp_ctrl = (GLfloat **) malloc(sizeof(GLfloat *) * t_cnt)) == NULL)
      return GLU_OUT_OF_MEMORY;
   if ((err = explode_knot(s_knot)) != GLU_NO_ERROR) {
      free(tmp_ctrl);
      if (s_knot->unified_knot) {
	 free(s_knot->unified_knot);
	 s_knot->unified_knot = NULL;
      }
      return err;
   }
   if (s_knot->unified_knot) {
      free(s_knot->unified_knot);
      s_knot->unified_knot = NULL;
   }
   if ((err = calc_alphas(s_knot)) != GLU_NO_ERROR) {
      free(tmp_ctrl);
      free(s_knot->new_knot);
      return err;
   }
   free(s_knot->new_knot);
   ctrl_offset = attrib->ctrlarray;
   dim = attrib->dim;
   for (i = 0; i < t_cnt; i++) {
      if ((err = calc_new_ctrl_pts(ctrl_offset, attrib->s_stride, s_knot,
				   dim, &(tmp_ctrl[i]),
				   &tmp_n_control)) != GLU_NO_ERROR) {
	 for (--i; i <= 0; i--)
	    free(tmp_ctrl[i]);
	 free(tmp_ctrl);
	 free(s_knot->alpha);
	 return err;
      }
      ctrl_offset += attrib->t_stride;
   }
   free(s_knot->alpha);
   tmp_stride = dim * tmp_n_control;
   if ((*new_ctrl = (GLfloat *) malloc(sizeof(GLfloat) * tmp_stride * t_cnt))
       == NULL) {
      for (i = 0; i < t_cnt; i++)
	 free(tmp_ctrl[i]);
      free(tmp_ctrl);
      return GLU_OUT_OF_MEMORY;
   }
   for (i = 0; i < tmp_n_control; i++)
      for (j = 0; j < t_cnt; j++)
	 MEMCPY(*new_ctrl + j * dim + i * dim * t_cnt, tmp_ctrl[j] + dim * i,
		sizeof(GLfloat) * dim);
   for (i = 0; i < t_cnt; i++)
      free(tmp_ctrl[i]);
   free(tmp_ctrl);
   *s_n_ctrl = tmp_n_control;

   if ((tmp_ctrl = (GLfloat **) malloc(sizeof(GLfloat *) * (*s_n_ctrl))) ==
       NULL) {
      return GLU_OUT_OF_MEMORY;
   }
   if ((err = explode_knot(t_knot)) != GLU_NO_ERROR) {
      free(tmp_ctrl);
      if (t_knot->unified_knot) {
	 free(t_knot->unified_knot);
	 t_knot->unified_knot = NULL;
      }
      return err;
   }
   if (t_knot->unified_knot) {
      free(t_knot->unified_knot);
      t_knot->unified_knot = NULL;
   }
   if ((err = calc_alphas(t_knot)) != GLU_NO_ERROR) {
      free(tmp_ctrl);
      free(t_knot->new_knot);
      return err;
   }
   free(t_knot->new_knot);
   ctrl_offset = *new_ctrl;
   for (i = 0; i < (*s_n_ctrl); i++) {
      if ((err = calc_new_ctrl_pts(ctrl_offset, dim, t_knot,
				   dim, &(tmp_ctrl[i]),
				   &tmp_n_control)) != GLU_NO_ERROR) {
	 for (--i; i <= 0; i--)
	    free(tmp_ctrl[i]);
	 free(tmp_ctrl);
	 free(t_knot->alpha);
	 return err;
      }
      ctrl_offset += dim * t_cnt;
   }
   free(t_knot->alpha);
   free(*new_ctrl);
   tmp_stride = dim * tmp_n_control;
   if (
       (*new_ctrl =
	(GLfloat *) malloc(sizeof(GLfloat) * tmp_stride * (*s_n_ctrl))) ==
       NULL) {
      for (i = 0; i < (*s_n_ctrl); i++)
	 free(tmp_ctrl[i]);
      free(tmp_ctrl);
      return GLU_OUT_OF_MEMORY;
   }
   for (i = 0; i < (*s_n_ctrl); i++) {
      MEMCPY(*new_ctrl + i * tmp_stride, tmp_ctrl[i],
	     sizeof(GLfloat) * tmp_stride);
      free(tmp_ctrl[i]);
   }
   free(tmp_ctrl);
   *t_n_ctrl = tmp_n_control;
   return GLU_NO_ERROR;
}

/* prepare the knot information structures */
static GLenum
fill_knot_structures(GLUnurbsObj * nobj,
		     knot_str_type * geom_s_knot, knot_str_type * geom_t_knot,
		     knot_str_type * color_s_knot,
		     knot_str_type * color_t_knot,
		     knot_str_type * normal_s_knot,
		     knot_str_type * normal_t_knot,
		     knot_str_type * texture_s_knot,
		     knot_str_type * texture_t_knot)
{
   GLint order;
   GLfloat *knot;
   GLint nknots;
   GLint t_min, t_max;

   geom_s_knot->unified_knot = NULL;
   knot = geom_s_knot->knot = nobj->surface.geom.sknot;
   nknots = geom_s_knot->nknots = nobj->surface.geom.sknot_count;
   order = geom_s_knot->order = nobj->surface.geom.sorder;
   geom_s_knot->delta_nknots = 0;
   t_min = geom_s_knot->t_min = order - 1;
   t_max = geom_s_knot->t_max = nknots - order;
   if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
      call_user_error(nobj, GLU_NURBS_ERROR3);
      return GLU_ERROR;
   }
   if (fabs(knot[0] - knot[t_min]) < EPSILON) {
      /* knot open at beggining */
      geom_s_knot->open_at_begin = GL_TRUE;
   }
   else
      geom_s_knot->open_at_begin = GL_FALSE;
   if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
      /* knot open at end */
      geom_s_knot->open_at_end = GL_TRUE;
   }
   else
      geom_s_knot->open_at_end = GL_FALSE;
   geom_t_knot->unified_knot = NULL;
   knot = geom_t_knot->knot = nobj->surface.geom.tknot;
   nknots = geom_t_knot->nknots = nobj->surface.geom.tknot_count;
   order = geom_t_knot->order = nobj->surface.geom.torder;
   geom_t_knot->delta_nknots = 0;
   t_min = geom_t_knot->t_min = order - 1;
   t_max = geom_t_knot->t_max = nknots - order;
   if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
      call_user_error(nobj, GLU_NURBS_ERROR3);
      return GLU_ERROR;
   }
   if (fabs(knot[0] - knot[t_min]) < EPSILON) {
      /* knot open at beggining */
      geom_t_knot->open_at_begin = GL_TRUE;
   }
   else
      geom_t_knot->open_at_begin = GL_FALSE;
   if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
      /* knot open at end */
      geom_t_knot->open_at_end = GL_TRUE;
   }
   else
      geom_t_knot->open_at_end = GL_FALSE;

   if (nobj->surface.color.type != GLU_INVALID_ENUM) {
      color_s_knot->unified_knot = (GLfloat *) 1;
      knot = color_s_knot->knot = nobj->surface.color.sknot;
      nknots = color_s_knot->nknots = nobj->surface.color.sknot_count;
      order = color_s_knot->order = nobj->surface.color.sorder;
      color_s_knot->delta_nknots = 0;
      t_min = color_s_knot->t_min = order - 1;
      t_max = color_s_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 color_s_knot->open_at_begin = GL_TRUE;
      }
      else
	 color_s_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 color_s_knot->open_at_end = GL_TRUE;
      }
      else
	 color_s_knot->open_at_end = GL_FALSE;
      color_t_knot->unified_knot = (GLfloat *) 1;
      knot = color_t_knot->knot = nobj->surface.color.tknot;
      nknots = color_t_knot->nknots = nobj->surface.color.tknot_count;
      order = color_t_knot->order = nobj->surface.color.torder;
      color_t_knot->delta_nknots = 0;
      t_min = color_t_knot->t_min = order - 1;
      t_max = color_t_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 color_t_knot->open_at_begin = GL_TRUE;
      }
      else
	 color_t_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 color_t_knot->open_at_end = GL_TRUE;
      }
      else
	 color_t_knot->open_at_end = GL_FALSE;
   }
   else {
      color_s_knot->unified_knot = NULL;
      color_t_knot->unified_knot = NULL;
   }

   if (nobj->surface.normal.type != GLU_INVALID_ENUM) {
      normal_s_knot->unified_knot = (GLfloat *) 1;
      knot = normal_s_knot->knot = nobj->surface.normal.sknot;
      nknots = normal_s_knot->nknots = nobj->surface.normal.sknot_count;
      order = normal_s_knot->order = nobj->surface.normal.sorder;
      normal_s_knot->delta_nknots = 0;
      t_min = normal_s_knot->t_min = order - 1;
      t_max = normal_s_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 normal_s_knot->open_at_begin = GL_TRUE;
      }
      else
	 normal_s_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 normal_s_knot->open_at_end = GL_TRUE;
      }
      else
	 normal_s_knot->open_at_end = GL_FALSE;
      normal_t_knot->unified_knot = (GLfloat *) 1;
      knot = normal_t_knot->knot = nobj->surface.normal.tknot;
      nknots = normal_t_knot->nknots = nobj->surface.normal.tknot_count;
      order = normal_t_knot->order = nobj->surface.normal.torder;
      normal_t_knot->delta_nknots = 0;
      t_min = normal_t_knot->t_min = order - 1;
      t_max = normal_t_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 normal_t_knot->open_at_begin = GL_TRUE;
      }
      else
	 normal_t_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 normal_t_knot->open_at_end = GL_TRUE;
      }
      else
	 normal_t_knot->open_at_end = GL_FALSE;
   }
   else {
      normal_s_knot->unified_knot = NULL;
      normal_t_knot->unified_knot = NULL;
   }

   if (nobj->surface.texture.type != GLU_INVALID_ENUM) {
      texture_s_knot->unified_knot = (GLfloat *) 1;
      knot = texture_s_knot->knot = nobj->surface.texture.sknot;
      nknots = texture_s_knot->nknots = nobj->surface.texture.sknot_count;
      order = texture_s_knot->order = nobj->surface.texture.sorder;
      texture_s_knot->delta_nknots = 0;
      t_min = texture_s_knot->t_min = order - 1;
      t_max = texture_s_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 texture_s_knot->open_at_begin = GL_TRUE;
      }
      else
	 texture_s_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 texture_s_knot->open_at_end = GL_TRUE;
      }
      else
	 texture_s_knot->open_at_end = GL_FALSE;
      texture_t_knot->unified_knot = (GLfloat *) 1;
      knot = texture_t_knot->knot = nobj->surface.texture.tknot;
      nknots = texture_t_knot->nknots = nobj->surface.texture.tknot_count;
      order = texture_t_knot->order = nobj->surface.texture.torder;
      texture_t_knot->delta_nknots = 0;
      t_min = texture_t_knot->t_min = order - 1;
      t_max = texture_t_knot->t_max = nknots - order;
      if (fabs(knot[t_min] - knot[t_max]) < EPSILON) {
	 call_user_error(nobj, GLU_NURBS_ERROR3);
	 return GLU_ERROR;
      }
      if (fabs(knot[0] - knot[t_min]) < EPSILON) {
	 /* knot open at beggining */
	 texture_t_knot->open_at_begin = GL_TRUE;
      }
      else
	 texture_t_knot->open_at_begin = GL_FALSE;
      if (fabs(knot[t_max] - knot[nknots - 1]) < EPSILON) {
	 /* knot open at end */
	 texture_t_knot->open_at_end = GL_TRUE;
      }
      else
	 texture_t_knot->open_at_end = GL_FALSE;
   }
   else {
      texture_s_knot->unified_knot = NULL;
      texture_t_knot->unified_knot = NULL;
   }
   return GLU_NO_ERROR;
}


static void
free_new_ctrl(new_ctrl_type * p)
{
   if (p->geom_ctrl)
      free(p->geom_ctrl);
   if (p->geom_offsets)
      free(p->geom_offsets);
   if (p->color_ctrl) {
      free(p->color_ctrl);
      if (p->color_offsets)
	 free(p->color_offsets);
   }
   if (p->normal_ctrl) {
      free(p->normal_ctrl);
      if (p->normal_offsets)
	 free(p->normal_offsets);
   }
   if (p->texture_ctrl) {
      free(p->texture_ctrl);
      if (p->texture_offsets)
	 free(p->texture_offsets);
   }
}

/* convert surfaces - geometry and possible attribute ones into equivalent */
/* sequence of adjacent Bezier patches */
static GLenum
convert_surfs(GLUnurbsObj * nobj, new_ctrl_type * new_ctrl)
{
   knot_str_type geom_s_knot, color_s_knot, normal_s_knot, texture_s_knot;
   knot_str_type geom_t_knot, color_t_knot, normal_t_knot, texture_t_knot;
   GLenum err;

   if ((err = fill_knot_structures(nobj, &geom_s_knot, &geom_t_knot,
				   &color_s_knot, &color_t_knot,
				   &normal_s_knot, &normal_t_knot,
				   &texture_s_knot,
				   &texture_t_knot)) != GLU_NO_ERROR) {
      return err;
   }
   /* unify knots - all knots should have the same working range */
   if ((err = select_knot_working_range(nobj, &geom_s_knot, &color_s_knot,
					&normal_s_knot,
					&texture_s_knot)) != GLU_NO_ERROR) {
      call_user_error(nobj, err);
      return err;
   }
   if ((err = select_knot_working_range(nobj, &geom_t_knot, &color_t_knot,
					&normal_t_knot,
					&texture_t_knot)) != GLU_NO_ERROR) {
      free_unified_knots(&geom_s_knot, &color_s_knot, &normal_s_knot,
			 &texture_s_knot);
      call_user_error(nobj, err);
      return err;
   }

   /* convert the geometry surface */
   nobj->surface.geom.dim = get_surface_dim(nobj->surface.geom.type);
   if ((err = convert_surf(&geom_s_knot, &geom_t_knot, &(nobj->surface.geom),
			   &(new_ctrl->geom_ctrl), &(new_ctrl->geom_s_pt_cnt),
			   &(new_ctrl->geom_t_pt_cnt))) != GLU_NO_ERROR) {
      free_unified_knots(&geom_s_knot, &color_s_knot, &normal_s_knot,
			 &texture_s_knot);
      free_unified_knots(&geom_t_knot, &color_t_knot, &normal_t_knot,
			 &texture_t_knot);
      call_user_error(nobj, err);
      return err;
   }
   /* if additional attributive surfaces are given convert them as well */
   if (color_s_knot.unified_knot) {
      nobj->surface.color.dim = get_surface_dim(nobj->surface.color.type);
      if (
	  (err =
	   convert_surf(&color_s_knot, &color_t_knot, &(nobj->surface.color),
			&(new_ctrl->color_ctrl), &(new_ctrl->color_s_pt_cnt),
			&(new_ctrl->color_t_pt_cnt))) != GLU_NO_ERROR) {
	 free_unified_knots(&color_s_knot, &color_s_knot, &normal_s_knot,
			    &texture_s_knot);
	 free_unified_knots(&color_t_knot, &color_t_knot, &normal_t_knot,
			    &texture_t_knot);
	 free_new_ctrl(new_ctrl);
	 call_user_error(nobj, err);
	 return err;
      }
   }
   if (normal_s_knot.unified_knot) {
      nobj->surface.normal.dim = get_surface_dim(nobj->surface.normal.type);
      if ((err = convert_surf(&normal_s_knot, &normal_t_knot,
			      &(nobj->surface.normal),
			      &(new_ctrl->normal_ctrl),
			      &(new_ctrl->normal_s_pt_cnt),
			      &(new_ctrl->normal_t_pt_cnt))) !=
	  GLU_NO_ERROR) {
	 free_unified_knots(&normal_s_knot, &normal_s_knot, &normal_s_knot,
			    &texture_s_knot);
	 free_unified_knots(&normal_t_knot, &normal_t_knot, &normal_t_knot,
			    &texture_t_knot);
	 free_new_ctrl(new_ctrl);
	 call_user_error(nobj, err);
	 return err;
      }
   }
   if (texture_s_knot.unified_knot) {
      nobj->surface.texture.dim = get_surface_dim(nobj->surface.texture.type);
      if ((err = convert_surf(&texture_s_knot, &texture_t_knot,
			      &(nobj->surface.texture),
			      &(new_ctrl->texture_ctrl),
			      &(new_ctrl->texture_s_pt_cnt),
			      &(new_ctrl->texture_t_pt_cnt))) !=
	  GLU_NO_ERROR) {
	 free_unified_knots(&texture_s_knot, &texture_s_knot, &texture_s_knot,
			    &texture_s_knot);
	 free_unified_knots(&texture_t_knot, &texture_t_knot, &texture_t_knot,
			    &texture_t_knot);
	 free_new_ctrl(new_ctrl);
	 call_user_error(nobj, err);
	 return err;
      }
   }
   return GLU_NO_ERROR;
}

/* tesselate the "boundary" Bezier edge strips */
static void
tesselate_strip_t_line(GLint top_start, GLint top_end, GLint top_z,
		       GLint bottom_start, GLint bottom_end, GLint bottom_z,
		       GLint bottom_domain)
{
   GLint top_cnt, bottom_cnt, tri_cnt, k;
   GLint direction;

   top_cnt = top_end - top_start;
   direction = (top_cnt >= 0 ? 1 : -1);
   bottom_cnt = bottom_end - bottom_start;
   glBegin(GL_LINES);
   while (top_cnt) {
      if (bottom_cnt)
	 tri_cnt = top_cnt / bottom_cnt;
      else
	 tri_cnt = abs(top_cnt);
      for (k = 0; k <= tri_cnt; k++, top_start += direction) {
	 glEvalCoord2f((GLfloat) bottom_z / bottom_domain,
		       (GLfloat) bottom_start / bottom_domain);
	 glEvalPoint2(top_z, top_start);
      }
      if (bottom_cnt) {
	 glEvalCoord2f((GLfloat) bottom_z / bottom_domain,
		       (GLfloat) bottom_start / bottom_domain);
	 bottom_start += direction;
	 top_start -= direction;
	 glEvalCoord2f((GLfloat) bottom_z / bottom_domain,
		       (GLfloat) bottom_start / bottom_domain);
	 glEvalCoord2f((GLfloat) bottom_z / bottom_domain,
		       (GLfloat) bottom_start / bottom_domain);
	 glEvalPoint2(top_z, top_start);
      }
      top_cnt -= direction * tri_cnt;
      bottom_cnt -= direction;
   }
   glEnd();
}


static void
tesselate_strip_t_fill(GLint top_start, GLint top_end, GLint top_z,
		       GLint bottom_start, GLint bottom_end, GLint bottom_z,
		       GLint bottom_domain)
{
   GLint top_cnt, bottom_cnt, tri_cnt, k;
   GLint direction;

   top_cnt = top_end - top_start;
   direction = (top_cnt >= 0 ? 1 : -1);
   bottom_cnt = bottom_end - bottom_start;
   while (top_cnt) {
      if (bottom_cnt)
	 tri_cnt = top_cnt / bottom_cnt;
      else
	 tri_cnt = abs(top_cnt);
      glBegin(GL_TRIANGLE_FAN);
      glEvalCoord2f((GLfloat) bottom_z / bottom_domain,
		    (GLfloat) bottom_start / bottom_domain);
      for (k = 0; k <= tri_cnt; k++, top_start += direction)
	 glEvalPoint2(top_z, top_start);
      if (bottom_cnt) {
	 bottom_start += direction;
	 top_start -= direction;
	 glEvalCoord2f((GLfloat) bottom_z / bottom_domain,
		       (GLfloat) bottom_start / bottom_domain);
      }
      glEnd();
      top_cnt -= direction * tri_cnt;
      bottom_cnt -= direction;
   }
}


static void
tesselate_strip_t(GLenum display_mode, GLint top_start, GLint top_end,
		  GLint top_z, GLint bottom_start, GLint bottom_end,
		  GLint bottom_z, GLint bottom_domain)
{
   if (display_mode == GL_FILL)
      tesselate_strip_t_fill(top_start, top_end, top_z, bottom_start,
			     bottom_end, bottom_z, bottom_domain);
   else
      tesselate_strip_t_line(top_start, top_end, top_z, bottom_start,
			     bottom_end, bottom_z, bottom_domain);
}


static void
tesselate_strip_s_fill(GLint top_start, GLint top_end, GLint top_z,
		       GLint bottom_start, GLint bottom_end, GLint bottom_z,
		       GLfloat bottom_domain)
{
   GLint top_cnt, bottom_cnt, tri_cnt, k;
   GLint direction;

   top_cnt = top_end - top_start;
   direction = (top_cnt >= 0 ? 1 : -1);
   bottom_cnt = bottom_end - bottom_start;
   while (top_cnt) {
      if (bottom_cnt)
	 tri_cnt = top_cnt / bottom_cnt;
      else
	 tri_cnt = abs(top_cnt);
      glBegin(GL_TRIANGLE_FAN);
      glEvalCoord2f((GLfloat) bottom_start / bottom_domain,
		    (GLfloat) bottom_z / bottom_domain);
      for (k = 0; k <= tri_cnt; k++, top_start += direction)
	 glEvalPoint2(top_start, top_z);
      if (bottom_cnt) {
	 bottom_start += direction;
	 top_start -= direction;
	 glEvalCoord2f((GLfloat) bottom_start / bottom_domain,
		       (GLfloat) bottom_z / bottom_domain);
      }
      glEnd();
      top_cnt -= direction * tri_cnt;
      bottom_cnt -= direction;
   }
}


static void
tesselate_strip_s_line(GLint top_start, GLint top_end, GLint top_z,
		       GLint bottom_start, GLint bottom_end, GLint bottom_z,
		       GLfloat bottom_domain)
{
   GLint top_cnt, bottom_cnt, tri_cnt, k;
   GLint direction;

   top_cnt = top_end - top_start;
   direction = (top_cnt >= 0 ? 1 : -1);
   bottom_cnt = bottom_end - bottom_start;
   glBegin(GL_LINES);
   while (top_cnt) {
      if (bottom_cnt)
	 tri_cnt = top_cnt / bottom_cnt;
      else
	 tri_cnt = abs(top_cnt);
      for (k = 0; k <= tri_cnt; k++, top_start += direction) {
	 glEvalCoord2f((GLfloat) bottom_start / bottom_domain,
		       (GLfloat) bottom_z / bottom_domain);
	 glEvalPoint2(top_start, top_z);
      }
      if (bottom_cnt) {
	 glEvalCoord2f((GLfloat) bottom_start / bottom_domain,
		       (GLfloat) bottom_z / bottom_domain);
	 bottom_start += direction;
	 top_start -= direction;
	 glEvalCoord2f((GLfloat) bottom_start / bottom_domain,
		       (GLfloat) bottom_z / bottom_domain);
	 glEvalPoint2(top_start, top_z);
	 glEvalCoord2f((GLfloat) bottom_start / bottom_domain,
		       (GLfloat) bottom_z / bottom_domain);
      }
      top_cnt -= direction * tri_cnt;
      bottom_cnt -= direction;
   }
   glEnd();
}


static void
tesselate_strip_s(GLenum display_mode, GLint top_start, GLint top_end,
		  GLint top_z, GLint bottom_start, GLint bottom_end,
		  GLint bottom_z, GLfloat bottom_domain)
{
   if (display_mode == GL_FILL)
      tesselate_strip_s_fill(top_start, top_end, top_z, bottom_start,
			     bottom_end, bottom_z, bottom_domain);
   else
      tesselate_strip_s_line(top_start, top_end, top_z, bottom_start,
			     bottom_end, bottom_z, bottom_domain);
}

static void
tesselate_bottom_left_corner(GLenum display_mode, GLfloat s_1, GLfloat t_1)
{
   if (display_mode == GL_FILL) {
      glBegin(GL_TRIANGLE_FAN);
      glEvalPoint2(1, 1);
      glEvalCoord2f(s_1, 0.0);
      glEvalCoord2f(0.0, 0.0);
      glEvalCoord2f(0.0, t_1);
   }
   else {
      glBegin(GL_LINES);
      glEvalCoord2f(0.0, 0.0);
      glEvalCoord2f(0.0, t_1);
      glEvalCoord2f(0.0, 0.0);
      glEvalPoint2(1, 1);
      glEvalCoord2f(0.0, 0.0);
      glEvalCoord2f(s_1, 0.0);
   }
   glEnd();
}

static void
tesselate_bottom_right_corner(GLenum display_mode, GLint v_top,
			      GLint v_bottom, GLfloat s_1, GLfloat t_1)
{
   if (display_mode == GL_FILL) {
      glBegin(GL_TRIANGLE_FAN);
      glEvalPoint2(1, v_top);
      glEvalCoord2f(0.0, v_bottom * t_1);
      glEvalCoord2f(0.0, (v_bottom + 1) * t_1);
      glEvalCoord2f(s_1, (v_bottom + 1) * t_1);
   }
   else {
      glBegin(GL_LINES);
      glEvalCoord2f(0.0, (v_bottom + 1) * t_1);
      glEvalPoint2(1, v_top);
      glEvalCoord2f(0.0, (v_bottom + 1) * t_1);
      glEvalCoord2f(0.0, v_bottom * t_1);
      glEvalCoord2f(0.0, (v_bottom + 1) * t_1);
      glEvalCoord2f(s_1, (v_bottom + 1) * t_1);
   }
   glEnd();
}

static void
tesselate_top_left_corner(GLenum display_mode, GLint u_right, GLint u_left,
			  GLfloat s_1, GLfloat t_1)
{
   if (display_mode == GL_FILL) {
      glBegin(GL_TRIANGLE_FAN);
      glEvalPoint2(u_right, 1);
      glEvalCoord2f((u_left + 1) * s_1, t_1);
      glEvalCoord2f((u_left + 1) * s_1, 0.0);
      glEvalCoord2f(u_left * s_1, 0.0);
   }
   else {
      glBegin(GL_LINES);
      glEvalCoord2f((u_left + 1) * s_1, 0.0);
      glEvalPoint2(u_right, 1);
      glEvalCoord2f((u_left + 1) * s_1, 0.0);
      glEvalCoord2f(u_left * s_1, 0.0);
      glEvalCoord2f((u_left + 1) * s_1, 0.0);
      glEvalCoord2f((u_left + 1) * s_1, t_1);
   }
   glEnd();
}

static void
tesselate_top_right_corner(GLenum display_mode, GLint u_left, GLint v_bottom,
			   GLint u_right, GLint v_top, GLfloat s_1,
			   GLfloat t_1)
{
   if (display_mode == GL_FILL) {
      glBegin(GL_TRIANGLE_FAN);
      glEvalPoint2(u_left, v_bottom);
      glEvalCoord2f((u_right - 1) * s_1, v_top * t_1);
      glEvalCoord2f(u_right * s_1, v_top * t_1);
      glEvalCoord2f(u_right * s_1, (v_top - 1) * t_1);
   }
   else {
      glBegin(GL_LINES);
      glEvalCoord2f(u_right * s_1, v_top * t_1);
      glEvalPoint2(u_left, v_bottom);
      glEvalCoord2f(u_right * s_1, v_top * t_1);
      glEvalCoord2f(u_right * s_1, (v_top - 1) * t_1);
      glEvalCoord2f(u_right * s_1, v_top * t_1);
      glEvalCoord2f((u_right - 1) * s_1, v_top * t_1);
   }
   glEnd();
}

/* do mesh mapping of Bezier */
static void
nurbs_map_bezier(GLenum display_mode, GLint * sfactors, GLint * tfactors,
		 GLint s_bezier_cnt, GLint t_bezier_cnt, GLint s, GLint t)
{
   GLint top, bottom, right, left;


   if (s == 0) {
      top = *(tfactors + t * 3);
      bottom = *(tfactors + t * 3 + 1);
   }
   else if (s == s_bezier_cnt - 1) {
      top = *(tfactors + t * 3 + 2);
      bottom = *(tfactors + t * 3);
   }
   else {
      top = bottom = *(tfactors + t * 3);
   }
   if (t == 0) {
      left = *(sfactors + s * 3 + 1);
      right = *(sfactors + s * 3);
   }
   else if (t == t_bezier_cnt - 1) {
      left = *(sfactors + s * 3);
      right = *(sfactors + s * 3 + 2);
   }
   else {
      left = right = *(sfactors + s * 3);
   }

   if (top > bottom) {
      if (left < right) {
	 glMapGrid2f(right, 0.0, 1.0, top, 0.0, 1.0);
	 glEvalMesh2(display_mode, 1, right, 1, top);
	 tesselate_strip_s(display_mode, 1, right, 1, 1, left, 0,
			   (GLfloat) left);
	 tesselate_bottom_left_corner(display_mode, (GLfloat) (1.0 / left),
				      (GLfloat) (1.0 / bottom));
/*			tesselate_strip_t(display_mode,1,top,1,1,bottom,0,(GLfloat)bottom);*/
	 tesselate_strip_t(display_mode, top, 1, 1, bottom, 1, 0,
			   (GLfloat) bottom);
      }
      else if (left == right) {
	 glMapGrid2f(right, 0.0, 1.0, top, 0.0, 1.0);
	 glEvalMesh2(display_mode, 1, right, 0, top);
/*			tesselate_strip_t(display_mode,0,top,1,0,bottom,0,(GLfloat)bottom);*/
	 tesselate_strip_t(display_mode, top, 0, 1, bottom, 0, 0,
			   (GLfloat) bottom);
      }
      else {
	 glMapGrid2f(left, 0.0, 1.0, top, 0.0, 1.0);
	 glEvalMesh2(display_mode, 1, left, 0, top - 1);
/*			tesselate_strip_t(display_mode,0,top-1,1,0,bottom-1,0,
				(GLfloat)bottom);*/
	 tesselate_strip_t(display_mode, top - 1, 0, 1, bottom - 1, 0, 0,
			   (GLfloat) bottom);
	 tesselate_bottom_right_corner(display_mode, top - 1, bottom - 1,
				       (GLfloat) (1.0 / right),
				       (GLfloat) (1.0 / bottom));
/*			tesselate_strip_s(display_mode,1,left,top-1,1,right,right,
				(GLfloat)right);*/
	 tesselate_strip_s(display_mode, left, 1, top - 1, right, 1, right,
			   (GLfloat) right);
      }
   }
   else if (top == bottom) {
      if (left < right) {
	 glMapGrid2f(right, 0.0, 1.0, top, 0.0, 1.0);
	 glEvalMesh2(display_mode, 0, right, 1, top);
	 tesselate_strip_s(display_mode, 0, right, 1, 0, left, 0,
			   (GLfloat) left);
      }
      else if (left == right) {
	 glMapGrid2f(right, 0.0, 1.0, top, 0.0, 1.0);
	 glEvalMesh2(display_mode, 0, right, 0, top);
      }
      else {
	 glMapGrid2f(left, 0.0, 1.0, top, 0.0, 1.0);
	 glEvalMesh2(display_mode, 0, left, 0, top - 1);
/*			tesselate_strip_s(display_mode,0,left,top-1,0,right,right,
				(GLfloat)right);*/
	 tesselate_strip_s(display_mode, left, 0, top - 1, right, 0, right,
			   (GLfloat) right);
      }
   }
   else {
      if (left < right) {
	 glMapGrid2f(right, 0.0, 1.0, bottom, 0.0, 1.0);
	 glEvalMesh2(display_mode, 0, right - 1, 1, bottom);
	 tesselate_strip_s(display_mode, 0, right - 1, 1, 0, left - 1, 0,
			   (GLfloat) left);
	 tesselate_top_left_corner(display_mode, right - 1, left - 1,
				   (GLfloat) (1.0 / left),
				   (GLfloat) (1.0 / top));
	 tesselate_strip_t(display_mode, 1, bottom, right - 1, 1, top, top,
			   (GLfloat) top);
      }
      else if (left == right) {
	 glMapGrid2f(right, 0.0, 1.0, bottom, 0.0, 1.0);
	 glEvalMesh2(display_mode, 0, right - 1, 0, bottom);
	 tesselate_strip_t(display_mode, 0, bottom, right - 1, 0, top, top,
			   (GLfloat) top);
      }
      else {
	 glMapGrid2f(left, 0.0, 1.0, bottom, 0.0, 1.0);
	 glEvalMesh2(display_mode, 0, left - 1, 0, bottom - 1);
	 tesselate_strip_t(display_mode, 0, bottom - 1, left - 1, 0, top - 1,
			   top, (GLfloat) top);
	 tesselate_top_right_corner(display_mode, left - 1, bottom - 1, right,
				    top, (GLfloat) (1.0 / right),
				    (GLfloat) (1.0 / top));
/*			tesselate_strip_s(display_mode,0,left-1,bottom-1,0,right-1,right,
				(GLfloat)right);*/
	 tesselate_strip_s(display_mode, left - 1, 0, bottom - 1, right - 1,
			   0, right, (GLfloat) right);
      }
   }
}

/* draw NURBS surface in OUTLINE POLYGON mode */
static void
draw_polygon_mode(GLenum display_mode, GLUnurbsObj * nobj,
		  new_ctrl_type * new_ctrl, GLint * sfactors,
		  GLint * tfactors)
{
   GLsizei offset;
   GLint t_bezier_cnt, s_bezier_cnt;
   GLboolean do_color, do_normal, do_texture;
   GLint i, j;

   t_bezier_cnt = new_ctrl->t_bezier_cnt;
   s_bezier_cnt = new_ctrl->s_bezier_cnt;
   glEnable(nobj->surface.geom.type);
   if (new_ctrl->color_ctrl) {
      glEnable(nobj->surface.color.type);
      do_color = GL_TRUE;
   }
   else
      do_color = GL_FALSE;
   if (new_ctrl->normal_ctrl) {
      glEnable(nobj->surface.normal.type);
      do_normal = GL_TRUE;
   }
   else
      do_normal = GL_FALSE;
   if (new_ctrl->texture_ctrl) {
      glEnable(nobj->surface.texture.type);
      do_texture = GL_TRUE;
   }
   else
      do_texture = GL_FALSE;
   for (j = 0; j < s_bezier_cnt; j++) {
      for (i = 0; i < t_bezier_cnt; i++) {
	 offset = j * t_bezier_cnt + i;
	 if (fine_culling_test_3D(nobj, *(new_ctrl->geom_offsets + offset),
				  nobj->surface.geom.sorder,
				  nobj->surface.geom.torder,
				  new_ctrl->geom_s_stride,
				  new_ctrl->geom_t_stride,
				  nobj->surface.geom.dim)) continue;
	 glMap2f(nobj->surface.geom.type, 0.0, 1.0, new_ctrl->geom_s_stride,
		 nobj->surface.geom.sorder, 0.0, 1.0, new_ctrl->geom_t_stride,
		 nobj->surface.geom.torder,
		 *(new_ctrl->geom_offsets + offset));
	 if (do_color) {
	    glMap2f(nobj->surface.color.type, 0.0, 1.0,
		    new_ctrl->color_s_stride, nobj->surface.color.sorder,
		    0.0, 1.0, new_ctrl->color_t_stride,
		    nobj->surface.color.torder,
		    *(new_ctrl->color_offsets + offset));
	 }
	 if (do_normal) {
	    glMap2f(nobj->surface.normal.type, 0.0, 1.0,
		    new_ctrl->normal_s_stride, nobj->surface.normal.sorder,
		    0.0, 1.0, new_ctrl->normal_t_stride,
		    nobj->surface.normal.torder,
		    *(new_ctrl->normal_offsets + offset));
	 }
	 if (do_texture) {
	    glMap2f(nobj->surface.texture.type, 0.0, 1.0,
		    new_ctrl->texture_s_stride, nobj->surface.texture.sorder,
		    0.0, 1.0, new_ctrl->texture_t_stride,
		    nobj->surface.texture.torder,
		    *(new_ctrl->texture_offsets + offset));
	 }
/*			glMapGrid2f(sfactors[j*3+0],0.0,1.0,tfactors[i*3+0],0.0,1.0);
			glEvalMesh2(display_mode,0,sfactors[j*3+0],0,tfactors[i*3+0]);*/
	 nurbs_map_bezier(display_mode, sfactors, tfactors, s_bezier_cnt,
			  t_bezier_cnt, j, i);
      }
   }
}



/* draw NURBS surface in OUTLINE POLYGON mode */
#if 0
static void
draw_patch_mode(GLenum display_mode, GLUnurbsObj * nobj,
		new_ctrl_type * new_ctrl, GLint * sfactors, GLint * tfactors)
{
   GLsizei offset;
   GLint t_bezier_cnt, s_bezier_cnt;
   GLboolean do_color, do_normal, do_texture;
   GLint i, j;

   t_bezier_cnt = new_ctrl->t_bezier_cnt;
   s_bezier_cnt = new_ctrl->s_bezier_cnt;
   glEnable(nobj->surface.geom.type);
   if (new_ctrl->color_ctrl) {
      glEnable(nobj->surface.color.type);
      do_color = GL_TRUE;
   }
   else
      do_color = GL_FALSE;
   if (new_ctrl->normal_ctrl) {
      glEnable(nobj->surface.normal.type);
      do_normal = GL_TRUE;
   }
   else
      do_normal = GL_FALSE;
   if (new_ctrl->texture_ctrl) {
      glEnable(nobj->surface.texture.type);
      do_texture = GL_TRUE;
   }
   else
      do_texture = GL_FALSE;
   for (j = 0; j < s_bezier_cnt; j++) {
      for (i = 0; i < t_bezier_cnt; i++) {
	 offset = j * t_bezier_cnt + i;
	 if (fine_culling_test_3D(nobj, *(new_ctrl->geom_offsets + offset),
				  nobj->surface.geom.sorder,
				  nobj->surface.geom.torder,
				  new_ctrl->geom_s_stride,
				  new_ctrl->geom_t_stride,
				  nobj->surface.geom.dim)) continue;
	 glMap2f(nobj->surface.geom.type, 0.0, 1.0, new_ctrl->geom_s_stride,
		 nobj->surface.geom.sorder, 0.0, 1.0, new_ctrl->geom_t_stride,
		 nobj->surface.geom.torder,
		 *(new_ctrl->geom_offsets + offset));
	 if (do_color) {
	    glMap2f(nobj->surface.color.type, 0.0, 1.0,
		    new_ctrl->color_s_stride, nobj->surface.color.sorder,
		    0.0, 1.0, new_ctrl->color_t_stride,
		    nobj->surface.color.torder,
		    *(new_ctrl->color_offsets + offset));
	 }
	 if (do_normal) {
	    glMap2f(nobj->surface.normal.type, 0.0, 1.0,
		    new_ctrl->normal_s_stride, nobj->surface.normal.sorder,
		    0.0, 1.0, new_ctrl->normal_t_stride,
		    nobj->surface.normal.torder,
		    *(new_ctrl->normal_offsets + offset));
	 }
	 if (do_texture) {
	    glMap2f(nobj->surface.texture.type, 0.0, 1.0,
		    new_ctrl->texture_s_stride, nobj->surface.texture.sorder,
		    0.0, 1.0, new_ctrl->texture_t_stride,
		    nobj->surface.texture.torder,
		    *(new_ctrl->texture_offsets + offset));
	 }
	 nurbs_map_bezier(display_mode, sfactors, tfactors, s_bezier_cnt,
			  t_bezier_cnt, i, j);
/*			glMapGrid2f(sfactors[j],0.0,1.0,tfactors[i],0.0,1.0);
			glEvalMesh2(display_mode,0,sfactors[j],0,tfactors[i]);*/
      }
   }
}
#endif



static void
init_new_ctrl(new_ctrl_type * p)
{
   p->geom_ctrl = p->color_ctrl = p->normal_ctrl = p->texture_ctrl = NULL;
   p->geom_offsets = p->color_offsets = p->normal_offsets =
      p->texture_offsets = NULL;
   p->s_bezier_cnt = p->t_bezier_cnt = 0;
}


static GLenum
augment_new_ctrl(GLUnurbsObj * nobj, new_ctrl_type * p)
{
   GLsizei offset_size;
   GLint i, j;

   p->s_bezier_cnt = (p->geom_s_pt_cnt) / (nobj->surface.geom.sorder);
   p->t_bezier_cnt = (p->geom_t_pt_cnt) / (nobj->surface.geom.torder);
   offset_size = (p->s_bezier_cnt) * (p->t_bezier_cnt);
   p->geom_t_stride = nobj->surface.geom.dim;
   p->geom_s_stride = (p->geom_t_pt_cnt) * (nobj->surface.geom.dim);
   p->color_t_stride = nobj->surface.color.dim;
   p->color_s_stride = (p->color_t_pt_cnt) * (nobj->surface.color.dim);
   p->normal_t_stride = nobj->surface.normal.dim;
   p->normal_s_stride = (p->normal_t_pt_cnt) * (nobj->surface.normal.dim);
   p->texture_t_stride = nobj->surface.texture.dim;
   p->texture_s_stride = (p->texture_t_pt_cnt) * (nobj->surface.texture.dim);
   if (
       (p->geom_offsets =
	(GLfloat **) malloc(sizeof(GLfloat *) * offset_size)) == NULL) {
      call_user_error(nobj, GLU_OUT_OF_MEMORY);
      return GLU_ERROR;
   }
   if (p->color_ctrl)
      if (
	  (p->color_offsets =
	   (GLfloat **) malloc(sizeof(GLfloat *) * offset_size)) == NULL) {
	 free_new_ctrl(p);
	 call_user_error(nobj, GLU_OUT_OF_MEMORY);
	 return GLU_ERROR;
      }
   if (p->normal_ctrl)
      if (
	  (p->normal_offsets =
	   (GLfloat **) malloc(sizeof(GLfloat *) * offset_size)) == NULL) {
	 free_new_ctrl(p);
	 call_user_error(nobj, GLU_OUT_OF_MEMORY);
	 return GLU_ERROR;
      }
   if (p->texture_ctrl)
      if (
	  (p->texture_offsets =
	   (GLfloat **) malloc(sizeof(GLfloat *) * offset_size)) == NULL) {
	 free_new_ctrl(p);
	 call_user_error(nobj, GLU_OUT_OF_MEMORY);
	 return GLU_ERROR;
      }
   for (i = 0; i < p->s_bezier_cnt; i++)
      for (j = 0; j < p->t_bezier_cnt; j++)
	 *(p->geom_offsets + i * (p->t_bezier_cnt) + j) =
	    p->geom_ctrl + i * (nobj->surface.geom.sorder) *
	    (nobj->surface.geom.dim) * (p->geom_t_pt_cnt) +
	    j * (nobj->surface.geom.dim) * (nobj->surface.geom.torder);
   if (p->color_ctrl)
      for (i = 0; i < p->s_bezier_cnt; i++)
	 for (j = 0; j < p->t_bezier_cnt; j++)
	    *(p->color_offsets + i * (p->t_bezier_cnt) + j) =
	       p->color_ctrl + i * (nobj->surface.color.sorder) *
	       (nobj->surface.color.dim) * (p->color_t_pt_cnt) +
	       j * (nobj->surface.color.dim) * (nobj->surface.color.torder);
   if (p->normal_ctrl)
      for (i = 0; i < p->s_bezier_cnt; i++)
	 for (j = 0; j < p->t_bezier_cnt; j++)
	    *(p->normal_offsets + i * (p->t_bezier_cnt) + j) =
	       p->normal_ctrl + i * (nobj->surface.normal.sorder) *
	       (nobj->surface.normal.dim) * (p->normal_t_pt_cnt) +
	       j * (nobj->surface.normal.dim) * (nobj->surface.normal.torder);
   if (p->texture_ctrl)
      for (i = 0; i < p->s_bezier_cnt; i++)
	 for (j = 0; j < p->t_bezier_cnt; j++)
	    *(p->texture_offsets + i * (p->t_bezier_cnt) + j) =
	       p->texture_ctrl + i * (nobj->surface.texture.sorder) *
	       (nobj->surface.texture.dim) * (p->texture_t_pt_cnt) +
	       j * (nobj->surface.texture.dim) *
	       (nobj->surface.texture.torder);
   return GLU_NO_ERROR;
}

/* main NURBS surface procedure */
void
do_nurbs_surface(GLUnurbsObj * nobj)
{
   GLint *sfactors, *tfactors;
   new_ctrl_type new_ctrl;

   /* test user supplied data */
   if (test_nurbs_surfaces(nobj) != GLU_NO_ERROR)
      return;

   init_new_ctrl(&new_ctrl);

   if (convert_surfs(nobj, &new_ctrl) != GLU_NO_ERROR)
      return;
   if (augment_new_ctrl(nobj, &new_ctrl) != GLU_NO_ERROR)
      return;
   switch (nobj->sampling_method) {
   case GLU_PATH_LENGTH:
      if (glu_do_sampling_3D(nobj, &new_ctrl, &sfactors, &tfactors) !=
	  GLU_NO_ERROR) {
	 free_new_ctrl(&new_ctrl);
	 return;
      }
      break;
   case GLU_DOMAIN_DISTANCE:
      if (glu_do_sampling_uv(nobj, &new_ctrl, &sfactors, &tfactors) !=
	  GLU_NO_ERROR) {
	 free_new_ctrl(&new_ctrl);
	 return;
      }
      break;
   case GLU_PARAMETRIC_ERROR:
      if (glu_do_sampling_param_3D(nobj, &new_ctrl, &sfactors, &tfactors) !=
	  GLU_NO_ERROR) {
	 free_new_ctrl(&new_ctrl);
	 return;
      }
      break;
   default:
      abort();
   }
   glFrontFace(GL_CW);
   switch (nobj->display_mode) {
   case GLU_FILL:
/*			if(polygon_trimming(nobj,&new_ctrl,sfactors,tfactors)==GLU_NO_ERROR)*/
      draw_polygon_mode(GL_FILL, nobj, &new_ctrl, sfactors, tfactors);
      break;
   case GLU_OUTLINE_POLYGON:
      /* TODO - missing trimming handeling */
/* just for now - no OUTLINE_PATCH mode 
			draw_patch_mode(GL_LINE,nobj,&new_ctrl,sfactors,tfactors);
			break; */
   case GLU_OUTLINE_PATCH:
/*			if(polygon_trimming(nobj,&new_ctrl,sfactors,tfactors)==GLU_NO_ERROR)*/
      draw_polygon_mode(GL_LINE, nobj, &new_ctrl, sfactors, tfactors);
      break;
   default:
      abort();			/* TODO: is this OK? */
   }
   free(sfactors);
   free(tfactors);
   free_new_ctrl(&new_ctrl);
}
