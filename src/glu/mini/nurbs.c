
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
}


void GLAPIENTRY
gluNurbsProperty(GLUnurbsObj * nobj, GLenum property, GLfloat value)
{
}


void GLAPIENTRY
gluGetNurbsProperty(GLUnurbsObj * nobj, GLenum property, GLfloat * value)
{
}



void GLAPIENTRY
gluBeginCurve(GLUnurbsObj * nobj)
{
}


void GLAPIENTRY
gluEndCurve(GLUnurbsObj * nobj)
{
}


void GLAPIENTRY
gluNurbsCurve(GLUnurbsObj * nobj, GLint nknots, GLfloat * knot,
	      GLint stride, GLfloat * ctlarray, GLint order, GLenum type)
{
}


void GLAPIENTRY
gluBeginSurface(GLUnurbsObj * nobj)
{
}


void GLAPIENTRY
gluEndSurface(GLUnurbsObj * nobj)
{
}


void GLAPIENTRY
gluNurbsSurface(GLUnurbsObj * nobj,
		GLint sknot_count, GLfloat * sknot,
		GLint tknot_count, GLfloat * tknot,
		GLint s_stride, GLint t_stride,
		GLfloat * ctrlarray, GLint sorder, GLint torder, GLenum type)
{
}


void GLAPIENTRY
gluNurbsCallback(GLUnurbsObj * nobj, GLenum which, void (GLCALLBACK * fn) ())
{
}

void GLAPIENTRY
gluBeginTrim(GLUnurbsObj * nobj)
{
}

void GLAPIENTRY
gluPwlCurve(GLUnurbsObj * nobj, GLint count, GLfloat * array, GLint stride,
	    GLenum type)
{
}

void GLAPIENTRY
gluEndTrim(GLUnurbsObj * nobj)
{
}
