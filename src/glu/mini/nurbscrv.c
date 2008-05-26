
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
