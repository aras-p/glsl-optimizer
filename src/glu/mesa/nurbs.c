
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
#include <stdio.h>
#include <stdlib.h>
#include "gluP.h"
#include "nurbs.h"
#endif


void
call_user_error(GLUnurbsObj * nobj, GLenum error)
{
   nobj->error = error;
   if (nobj->error_callback != NULL) {
      (*(nobj->error_callback)) (error);
   }
   else {
      printf("NURBS error %d %s\n", error, (char *) gluErrorString(error));
   }
}



GLUnurbsObj *GLAPIENTRY
gluNewNurbsRenderer(void)
{
   GLUnurbsObj *n;
   GLfloat tmp_viewport[4];
   GLint i, j;

   n = (GLUnurbsObj *) malloc(sizeof(GLUnurbsObj));
   if (n) {
      /* init */
      n->culling = GL_FALSE;
      n->nurbs_type = GLU_NURBS_NONE;
      n->error = GLU_NO_ERROR;
      n->error_callback = NULL;
      n->auto_load_matrix = GL_TRUE;
      n->sampling_tolerance = 50.0;
      n->parametric_tolerance = 0.5;
      n->u_step = n->v_step = 100;
      n->sampling_method = GLU_PATH_LENGTH;
      n->display_mode = GLU_FILL;
      /* in case the user doesn't supply the sampling matrices */
      /* set projection and modelview to identity */
      for (i = 0; i < 4; i++)
	 for (j = 0; j < 4; j++)
	    if (i == j) {
	       n->sampling_matrices.model[i * 4 + j] = 1.0;
	       n->sampling_matrices.proj[i * 4 + j] = 1.0;
	    }
	    else {
	       n->sampling_matrices.model[i * 4 + j] = 0.0;
	       n->sampling_matrices.proj[i * 4 + j] = 0.0;
	    }
      /* and set the viewport sampling matrix to current ciewport */
      glGetFloatv(GL_VIEWPORT, tmp_viewport);
      for (i = 0; i < 4; i++)
	 n->sampling_matrices.viewport[i] = tmp_viewport[i];
      n->trim = NULL;
   }
   return n;
}



void GLAPIENTRY
gluDeleteNurbsRenderer(GLUnurbsObj * nobj)
{
   if (nobj) {
      free(nobj);
   }
}



void GLAPIENTRY
gluLoadSamplingMatrices(GLUnurbsObj * nobj,
			const GLfloat modelMatrix[16],
			const GLfloat projMatrix[16], const GLint viewport[4])
{
   GLint i;

   for (i = 0; i < 16; i++) {
      nobj->sampling_matrices.model[i] = modelMatrix[i];
      nobj->sampling_matrices.proj[i] = projMatrix[i];
   }
   for (i = 0; i < 4; i++)
      nobj->sampling_matrices.viewport[i] = viewport[i];
}


void GLAPIENTRY
gluNurbsProperty(GLUnurbsObj * nobj, GLenum property, GLfloat value)
{
   GLenum val;

   switch (property) {
   case GLU_SAMPLING_TOLERANCE:
      if (value <= 0.0) {
	 call_user_error(nobj, GLU_INVALID_VALUE);
	 return;
      }
      nobj->sampling_tolerance = value;
      break;
   case GLU_PARAMETRIC_TOLERANCE:
      if (value <= 0.0) {
	 call_user_error(nobj, GLU_INVALID_VALUE);
	 return;
      }
      nobj->parametric_tolerance = value;
      break;
   case GLU_U_STEP:
      if (value <= 0.0) {
	 call_user_error(nobj, GLU_INVALID_VALUE);
	 return;
      }
      nobj->u_step = (GLint) value;
      break;
   case GLU_V_STEP:
      if (value <= 0.0) {
	 call_user_error(nobj, GLU_INVALID_VALUE);
	 return;
      }
      nobj->v_step = (GLint) value;
      break;
   case GLU_SAMPLING_METHOD:
      val = (GLenum) value;
      if (val != GLU_PATH_LENGTH && val != GLU_PARAMETRIC_ERROR
	  && val != GLU_DOMAIN_DISTANCE) {
	 call_user_error(nobj, GLU_INVALID_ENUM);
	 return;
      }
      nobj->sampling_method = val;
      break;
   case GLU_DISPLAY_MODE:
      val = (GLenum) value;
      if (val != GLU_FILL && val != GLU_OUTLINE_POLYGON
	  && val != GLU_OUTLINE_PATCH) {
	 call_user_error(nobj, GLU_INVALID_ENUM);
	 return;
      }
      if (nobj->nurbs_type == GLU_NURBS_CURVE) {
	 call_user_error(nobj, GLU_NURBS_ERROR26);
	 return;
      }
      nobj->display_mode = val;
      if (val == GLU_OUTLINE_PATCH)
	 fprintf(stderr,
		 "NURBS, for the moment, can display only in POLYGON mode\n");
      break;
   case GLU_CULLING:
      val = (GLenum) value;
      if (val != GL_TRUE && val != GL_FALSE) {
	 call_user_error(nobj, GLU_INVALID_ENUM);
	 return;
      }
      nobj->culling = (GLboolean) value;
      break;
   case GLU_AUTO_LOAD_MATRIX:
      val = (GLenum) value;
      if (val != GL_TRUE && val != GL_FALSE) {
	 call_user_error(nobj, GLU_INVALID_ENUM);
	 return;
      }
      nobj->auto_load_matrix = (GLboolean) value;
      break;
   default:
      call_user_error(nobj, GLU_NURBS_ERROR26);
   }
}


void GLAPIENTRY
gluGetNurbsProperty(GLUnurbsObj * nobj, GLenum property, GLfloat * value)
{
   switch (property) {
   case GLU_SAMPLING_TOLERANCE:
      *value = nobj->sampling_tolerance;
      break;
   case GLU_DISPLAY_MODE:
      *value = (GLfloat) (GLint) nobj->display_mode;
      break;
   case GLU_CULLING:
      *value = nobj->culling ? 1.0 : 0.0;
      break;
   case GLU_AUTO_LOAD_MATRIX:
      *value = nobj->auto_load_matrix ? 1.0 : 0.0;
      break;
   default:
      call_user_error(nobj, GLU_INVALID_ENUM);
   }
}



void GLAPIENTRY
gluBeginCurve(GLUnurbsObj * nobj)
{
   if (nobj->nurbs_type == GLU_NURBS_CURVE) {
      call_user_error(nobj, GLU_NURBS_ERROR6);
      return;
   }
   nobj->nurbs_type = GLU_NURBS_CURVE;
   nobj->curve.geom.type = GLU_INVALID_ENUM;
   nobj->curve.color.type = GLU_INVALID_ENUM;
   nobj->curve.texture.type = GLU_INVALID_ENUM;
   nobj->curve.normal.type = GLU_INVALID_ENUM;
}


void GLAPIENTRY
gluEndCurve(GLUnurbsObj * nobj)
{
   if (nobj->nurbs_type == GLU_NURBS_NONE) {
      call_user_error(nobj, GLU_NURBS_ERROR7);
      return;
   }
   if (nobj->curve.geom.type == GLU_INVALID_ENUM) {
      call_user_error(nobj, GLU_NURBS_ERROR8);
      nobj->nurbs_type = GLU_NURBS_NONE;
      return;
   }
   glPushAttrib((GLbitfield) (GL_EVAL_BIT | GL_ENABLE_BIT));
   glDisable(GL_MAP1_VERTEX_3);
   glDisable(GL_MAP1_VERTEX_4);
   glDisable(GL_MAP1_INDEX);
   glDisable(GL_MAP1_COLOR_4);
   glDisable(GL_MAP1_NORMAL);
   glDisable(GL_MAP1_TEXTURE_COORD_1);
   glDisable(GL_MAP1_TEXTURE_COORD_2);
   glDisable(GL_MAP1_TEXTURE_COORD_3);
   glDisable(GL_MAP1_TEXTURE_COORD_4);
   glDisable(GL_MAP2_VERTEX_3);
   glDisable(GL_MAP2_VERTEX_4);
   glDisable(GL_MAP2_INDEX);
   glDisable(GL_MAP2_COLOR_4);
   glDisable(GL_MAP2_NORMAL);
   glDisable(GL_MAP2_TEXTURE_COORD_1);
   glDisable(GL_MAP2_TEXTURE_COORD_2);
   glDisable(GL_MAP2_TEXTURE_COORD_3);
   glDisable(GL_MAP2_TEXTURE_COORD_4);
   do_nurbs_curve(nobj);
   glPopAttrib();
   nobj->nurbs_type = GLU_NURBS_NONE;
}


void GLAPIENTRY
gluNurbsCurve(GLUnurbsObj * nobj, GLint nknots, GLfloat * knot,
	      GLint stride, GLfloat * ctlarray, GLint order, GLenum type)
{
   if (nobj->nurbs_type == GLU_NURBS_TRIM) {
#if 0
/* TODO: NOT IMPLEMENTED YET */
      nurbs_trim *ptr1;
      trim_list *ptr2;

      if (type != GLU_MAP1_TRIM_2 && type != GLU_MAP1_TRIM_3) {
	 call_user_error(nobj, GLU_NURBS_ERROR14);
	 return;
      }
      for (ptr1 = nobj->trim; ptr1->next; ptr1 = ptr1->next);
      if (ptr1->trim_loop) {
	 for (ptr2 = ptr1->trim_loop; ptr2->next; ptr2 = ptr2->next);
	 if ((ptr2->next = (trim_list *) malloc(sizeof(trim_list))) == NULL) {
	    call_user_error(nobj, GLU_OUT_OF_MEMORY);
	    return;
	 }
	 ptr2 = ptr2->next;
      }
      else {
	 if ((ptr2 = (trim_list *) malloc(sizeof(trim_list))) == NULL) {
	    call_user_error(nobj, GLU_OUT_OF_MEMORY);
	    return;
	 }
	 ptr1->trim_loop = ptr2;
      }
      ptr2->trim_type = GLU_TRIM_NURBS;
      ptr2->curve.nurbs_curve.knot_count = nknots;
      ptr2->curve.nurbs_curve.knot = knot;
      ptr2->curve.nurbs_curve.stride = stride;
      ptr2->curve.nurbs_curve.ctrlarray = ctlarray;
      ptr2->curve.nurbs_curve.order = order;
      ptr2->curve.nurbs_curve.dim = (type == GLU_MAP1_TRIM_2 ? 2 : 3);
      ptr2->curve.nurbs_curve.type = type;
      ptr2->next = NULL;
#endif
   }
   else {
      if (type == GLU_MAP1_TRIM_2 || type == GLU_MAP1_TRIM_3) {
	 call_user_error(nobj, GLU_NURBS_ERROR22);
	 return;
      }
      if (nobj->nurbs_type != GLU_NURBS_CURVE) {
	 call_user_error(nobj, GLU_NURBS_ERROR10);
	 return;
      }
      switch (type) {
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	 if (nobj->curve.geom.type != GLU_INVALID_ENUM) {
	    call_user_error(nobj, GLU_NURBS_ERROR8);
	    return;
	 }
	 nobj->curve.geom.type = type;
	 nobj->curve.geom.knot_count = nknots;
	 nobj->curve.geom.knot = knot;
	 nobj->curve.geom.stride = stride;
	 nobj->curve.geom.ctrlarray = ctlarray;
	 nobj->curve.geom.order = order;
	 break;
      case GL_MAP1_INDEX:
      case GL_MAP1_COLOR_4:
	 nobj->curve.color.type = type;
	 nobj->curve.color.knot_count = nknots;
	 nobj->curve.color.knot = knot;
	 nobj->curve.color.stride = stride;
	 nobj->curve.color.ctrlarray = ctlarray;
	 nobj->curve.color.order = order;
	 break;
      case GL_MAP1_NORMAL:
	 nobj->curve.normal.type = type;
	 nobj->curve.normal.knot_count = nknots;
	 nobj->curve.normal.knot = knot;
	 nobj->curve.normal.stride = stride;
	 nobj->curve.normal.ctrlarray = ctlarray;
	 nobj->curve.normal.order = order;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
	 nobj->curve.texture.type = type;
	 nobj->curve.texture.knot_count = nknots;
	 nobj->curve.texture.knot = knot;
	 nobj->curve.texture.stride = stride;
	 nobj->curve.texture.ctrlarray = ctlarray;
	 nobj->curve.texture.order = order;
	 break;
      default:
	 call_user_error(nobj, GLU_INVALID_ENUM);
      }
   }
}


void GLAPIENTRY
gluBeginSurface(GLUnurbsObj * nobj)
{
   switch (nobj->nurbs_type) {
   case GLU_NURBS_NONE:
      nobj->nurbs_type = GLU_NURBS_SURFACE;
      nobj->surface.geom.type = GLU_INVALID_ENUM;
      nobj->surface.color.type = GLU_INVALID_ENUM;
      nobj->surface.texture.type = GLU_INVALID_ENUM;
      nobj->surface.normal.type = GLU_INVALID_ENUM;
      break;
   case GLU_NURBS_TRIM:
      call_user_error(nobj, GLU_NURBS_ERROR16);
      break;
   case GLU_NURBS_SURFACE:
   case GLU_NURBS_NO_TRIM:
   case GLU_NURBS_TRIM_DONE:
      call_user_error(nobj, GLU_NURBS_ERROR27);
      break;
   case GLU_NURBS_CURVE:
      call_user_error(nobj, GLU_NURBS_ERROR6);
      break;
   }
}


void GLAPIENTRY
gluEndSurface(GLUnurbsObj * nobj)
{
   switch (nobj->nurbs_type) {
   case GLU_NURBS_NONE:
      call_user_error(nobj, GLU_NURBS_ERROR13);
      break;
   case GLU_NURBS_TRIM:
      call_user_error(nobj, GLU_NURBS_ERROR12);
      break;
   case GLU_NURBS_TRIM_DONE:
/*            if(nobj->trim->trim_loop==NULL)
            {
                call_user_error(nobj,GLU_NURBS_ERROR18);
                return;
            }*/
      /* no break - fallthrough */
   case GLU_NURBS_NO_TRIM:
      glPushAttrib((GLbitfield)
		   (GL_EVAL_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT));
      glDisable(GL_MAP2_VERTEX_3);
      glDisable(GL_MAP2_VERTEX_4);
      glDisable(GL_MAP2_INDEX);
      glDisable(GL_MAP2_COLOR_4);
      glDisable(GL_MAP2_NORMAL);
      glDisable(GL_MAP2_TEXTURE_COORD_1);
      glDisable(GL_MAP2_TEXTURE_COORD_2);
      glDisable(GL_MAP2_TEXTURE_COORD_3);
      glDisable(GL_MAP2_TEXTURE_COORD_4);
/*            glDisable(GL_MAP1_VERTEX_3);
            glDisable(GL_MAP1_VERTEX_4);
            glDisable(GL_MAP1_INDEX);
            glDisable(GL_MAP1_COLOR_4);
            glDisable(GL_MAP1_NORMAL);
            glDisable(GL_MAP1_TEXTURE_COORD_1);
            glDisable(GL_MAP1_TEXTURE_COORD_2);
            glDisable(GL_MAP1_TEXTURE_COORD_3);
            glDisable(GL_MAP1_TEXTURE_COORD_4);*/
      do_nurbs_surface(nobj);
      glPopAttrib();
      break;
   default:
      call_user_error(nobj, GLU_NURBS_ERROR8);
   }
   nobj->nurbs_type = GLU_NURBS_NONE;
}


void GLAPIENTRY
gluNurbsSurface(GLUnurbsObj * nobj,
		GLint sknot_count, GLfloat * sknot,
		GLint tknot_count, GLfloat * tknot,
		GLint s_stride, GLint t_stride,
		GLfloat * ctrlarray, GLint sorder, GLint torder, GLenum type)
{
   if (nobj->nurbs_type == GLU_NURBS_NO_TRIM
       || nobj->nurbs_type == GLU_NURBS_TRIM
       || nobj->nurbs_type == GLU_NURBS_TRIM_DONE) {
      if (type == GL_MAP2_VERTEX_3 || type == GL_MAP2_VERTEX_4) {
	 call_user_error(nobj, GLU_NURBS_ERROR8);
	 return;
      }
   }
   else if (nobj->nurbs_type != GLU_NURBS_SURFACE) {
      call_user_error(nobj, GLU_NURBS_ERROR11);
      return;
   }
   switch (type) {
   case GL_MAP2_VERTEX_3:
   case GL_MAP2_VERTEX_4:
      nobj->surface.geom.sknot_count = sknot_count;
      nobj->surface.geom.sknot = sknot;
      nobj->surface.geom.tknot_count = tknot_count;
      nobj->surface.geom.tknot = tknot;
      nobj->surface.geom.s_stride = s_stride;
      nobj->surface.geom.t_stride = t_stride;
      nobj->surface.geom.ctrlarray = ctrlarray;
      nobj->surface.geom.sorder = sorder;
      nobj->surface.geom.torder = torder;
      nobj->surface.geom.type = type;
      nobj->nurbs_type = GLU_NURBS_NO_TRIM;
      break;
   case GL_MAP2_INDEX:
   case GL_MAP2_COLOR_4:
      nobj->surface.color.sknot_count = sknot_count;
      nobj->surface.color.sknot = sknot;
      nobj->surface.color.tknot_count = tknot_count;
      nobj->surface.color.tknot = tknot;
      nobj->surface.color.s_stride = s_stride;
      nobj->surface.color.t_stride = t_stride;
      nobj->surface.color.ctrlarray = ctrlarray;
      nobj->surface.color.sorder = sorder;
      nobj->surface.color.torder = torder;
      nobj->surface.color.type = type;
      break;
   case GL_MAP2_NORMAL:
      nobj->surface.normal.sknot_count = sknot_count;
      nobj->surface.normal.sknot = sknot;
      nobj->surface.normal.tknot_count = tknot_count;
      nobj->surface.normal.tknot = tknot;
      nobj->surface.normal.s_stride = s_stride;
      nobj->surface.normal.t_stride = t_stride;
      nobj->surface.normal.ctrlarray = ctrlarray;
      nobj->surface.normal.sorder = sorder;
      nobj->surface.normal.torder = torder;
      nobj->surface.normal.type = type;
      break;
   case GL_MAP2_TEXTURE_COORD_1:
   case GL_MAP2_TEXTURE_COORD_2:
   case GL_MAP2_TEXTURE_COORD_3:
   case GL_MAP2_TEXTURE_COORD_4:
      nobj->surface.texture.sknot_count = sknot_count;
      nobj->surface.texture.sknot = sknot;
      nobj->surface.texture.tknot_count = tknot_count;
      nobj->surface.texture.tknot = tknot;
      nobj->surface.texture.s_stride = s_stride;
      nobj->surface.texture.t_stride = t_stride;
      nobj->surface.texture.ctrlarray = ctrlarray;
      nobj->surface.texture.sorder = sorder;
      nobj->surface.texture.torder = torder;
      nobj->surface.texture.type = type;
      break;
   default:
      call_user_error(nobj, GLU_INVALID_ENUM);
   }
}


void GLAPIENTRY
gluNurbsCallback(GLUnurbsObj * nobj, GLenum which, void (GLCALLBACK * fn) ())
{
   nobj->error_callback = (void (GLCALLBACKPCAST) (GLenum)) fn;

   if (which != GLU_ERROR)
      call_user_error(nobj, GLU_INVALID_ENUM);
}

void GLAPIENTRY
gluBeginTrim(GLUnurbsObj * nobj)
{
#if 0
   nurbs_trim *ptr;
#endif

   if (nobj->nurbs_type != GLU_NURBS_TRIM_DONE)
      if (nobj->nurbs_type != GLU_NURBS_NO_TRIM) {
	 call_user_error(nobj, GLU_NURBS_ERROR15);
	 return;
      }
   nobj->nurbs_type = GLU_NURBS_TRIM;
   fprintf(stderr, "NURBS - trimming not supported yet\n");
#if 0
   if ((ptr = (nurbs_trim *) malloc(sizeof(nurbs_trim))) == NULL) {
      call_user_error(nobj, GLU_OUT_OF_MEMORY);
      return;
   }
   if (nobj->trim) {
      nurbs_trim *tmp_ptr;

      for (tmp_ptr = nobj->trim; tmp_ptr->next; tmp_ptr = tmp_ptr->next);
      tmp_ptr->next = ptr;
   }
   else
      nobj->trim = ptr;
   ptr->trim_loop = NULL;
   ptr->segments = NULL;
   ptr->next = NULL;
#endif
}

void GLAPIENTRY
gluPwlCurve(GLUnurbsObj * nobj, GLint count, GLfloat * array, GLint stride,
	    GLenum type)
{
#if 0
   nurbs_trim *ptr1;
   trim_list *ptr2;
#endif
   if (nobj->nurbs_type == GLU_NURBS_CURVE) {
      call_user_error(nobj, GLU_NURBS_ERROR9);
      return;
   }
   if (nobj->nurbs_type == GLU_NURBS_NONE) {
      call_user_error(nobj, GLU_NURBS_ERROR19);
      return;
   }
   if (type != GLU_MAP1_TRIM_2 && type != GLU_MAP1_TRIM_3) {
      call_user_error(nobj, GLU_NURBS_ERROR14);
      return;
   }
#if 0
   for (ptr1 = nobj->trim; ptr1->next; ptr1 = ptr1->next);
   if (ptr1->trim_loop) {
      for (ptr2 = ptr1->trim_loop; ptr2->next; ptr2 = ptr2->next);
      if ((ptr2->next = (trim_list *) malloc(sizeof(trim_list))) == NULL) {
	 call_user_error(nobj, GLU_OUT_OF_MEMORY);
	 return;
      }
      ptr2 = ptr2->next;
   }
   else {
      if ((ptr2 = (trim_list *) malloc(sizeof(trim_list))) == NULL) {
	 call_user_error(nobj, GLU_OUT_OF_MEMORY);
	 return;
      }
      ptr1->trim_loop = ptr2;
   }
   ptr2->trim_type = GLU_TRIM_PWL;
   ptr2->curve.pwl_curve.pt_count = count;
   ptr2->curve.pwl_curve.ctrlarray = array;
   ptr2->curve.pwl_curve.stride = stride;
   ptr2->curve.pwl_curve.dim = (type == GLU_MAP1_TRIM_2 ? 2 : 3);
   ptr2->curve.pwl_curve.type = type;
   ptr2->next = NULL;
#endif
}

void GLAPIENTRY
gluEndTrim(GLUnurbsObj * nobj)
{
   if (nobj->nurbs_type != GLU_NURBS_TRIM) {
      call_user_error(nobj, GLU_NURBS_ERROR17);
      return;
   }
   nobj->nurbs_type = GLU_NURBS_TRIM_DONE;
}
