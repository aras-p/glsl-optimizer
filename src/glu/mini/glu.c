
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * Copyright (C) 1995-2001  Brian Paul
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


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gluP.h"
#endif


/*
 * Miscellaneous utility functions
 */


#ifndef M_PI
#define M_PI 3.1415926536
#endif
#define EPS 0.00001

#ifndef GLU_INCOMPATIBLE_GL_VERSION
#define GLU_INCOMPATIBLE_GL_VERSION     100903
#endif


void GLAPIENTRY
gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez,
	  GLdouble centerx, GLdouble centery, GLdouble centerz,
	  GLdouble upx, GLdouble upy, GLdouble upz)
{
   GLfloat m[16];
   GLfloat x[3], y[3], z[3];
   GLfloat mag;

   /* Make rotation matrix */

   /* Z vector */
   z[0] = eyex - centerx;
   z[1] = eyey - centery;
   z[2] = eyez - centerz;
   mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
   if (mag) {			/* mpichler, 19950515 */
      z[0] /= mag;
      z[1] /= mag;
      z[2] /= mag;
   }

   /* Y vector */
   y[0] = upx;
   y[1] = upy;
   y[2] = upz;

   /* X vector = Y cross Z */
   x[0] = y[1] * z[2] - y[2] * z[1];
   x[1] = -y[0] * z[2] + y[2] * z[0];
   x[2] = y[0] * z[1] - y[1] * z[0];

   /* Recompute Y = Z cross X */
   y[0] = z[1] * x[2] - z[2] * x[1];
   y[1] = -z[0] * x[2] + z[2] * x[0];
   y[2] = z[0] * x[1] - z[1] * x[0];

   /* mpichler, 19950515 */
   /* cross product gives area of parallelogram, which is < 1.0 for
    * non-perpendicular unit-length vectors; so normalize x, y here
    */

   mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
   if (mag) {
      x[0] /= mag;
      x[1] /= mag;
      x[2] /= mag;
   }

   mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
   if (mag) {
      y[0] /= mag;
      y[1] /= mag;
      y[2] /= mag;
   }

#define M(row,col)  m[col*4+row]
   M(0, 0) = x[0];
   M(0, 1) = x[1];
   M(0, 2) = x[2];
   M(0, 3) = 0.0;
   M(1, 0) = y[0];
   M(1, 1) = y[1];
   M(1, 2) = y[2];
   M(1, 3) = 0.0;
   M(2, 0) = z[0];
   M(2, 1) = z[1];
   M(2, 2) = z[2];
   M(2, 3) = 0.0;
   M(3, 0) = 0.0;
   M(3, 1) = 0.0;
   M(3, 2) = 0.0;
   M(3, 3) = 1.0;
#undef M
   glMultMatrixf(m);

   /* Translate Eye to Origin */
   glTranslatef(-eyex, -eyey, -eyez);

}



void GLAPIENTRY
gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
   glOrtho(left, right, bottom, top, -1.0, 1.0);
}



static void
frustum(GLfloat left, GLfloat right,
        GLfloat bottom, GLfloat top, 
        GLfloat nearval, GLfloat farval)
{
   GLfloat x, y, a, b, c, d;
   GLfloat m[16];

   x = (2.0 * nearval) / (right - left);
   y = (2.0 * nearval) / (top - bottom);
   a = (right + left) / (right - left);
   b = (top + bottom) / (top - bottom);
   c = -(farval + nearval) / ( farval - nearval);
   d = -(2.0 * farval * nearval) / (farval - nearval);

#define M(row,col)  m[col*4+row]
   M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
   M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
   M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
   M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M

   glMultMatrixf(m);
}


void GLAPIENTRY
gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
   GLfloat xmin, xmax, ymin, ymax;

   ymax = zNear * tan(fovy * M_PI / 360.0);
   ymin = -ymax;
   xmin = ymin * aspect;
   xmax = ymax * aspect;

   /* don't call glFrustum() because of error semantics (covglu) */
   frustum(xmin, xmax, ymin, ymax, zNear, zFar);
}



void GLAPIENTRY
gluPickMatrix(GLdouble x, GLdouble y,
	      GLdouble width, GLdouble height, GLint viewport[4])
{
   GLfloat m[16];
   GLfloat sx, sy;
   GLfloat tx, ty;

   sx = viewport[2] / width;
   sy = viewport[3] / height;
   tx = (viewport[2] + 2.0 * (viewport[0] - x)) / width;
   ty = (viewport[3] + 2.0 * (viewport[1] - y)) / height;

#define M(row,col)  m[col*4+row]
   M(0, 0) = sx;
   M(0, 1) = 0.0;
   M(0, 2) = 0.0;
   M(0, 3) = tx;
   M(1, 0) = 0.0;
   M(1, 1) = sy;
   M(1, 2) = 0.0;
   M(1, 3) = ty;
   M(2, 0) = 0.0;
   M(2, 1) = 0.0;
   M(2, 2) = 1.0;
   M(2, 3) = 0.0;
   M(3, 0) = 0.0;
   M(3, 1) = 0.0;
   M(3, 2) = 0.0;
   M(3, 3) = 1.0;
#undef M

   glMultMatrixf(m);
}



const GLubyte *GLAPIENTRY
gluErrorString(GLenum errorCode)
{
   static char *tess_error[] = {
      "missing gluBeginPolygon",
      "missing gluBeginContour",
      "missing gluEndPolygon",
      "missing gluEndContour",
      "misoriented or self-intersecting loops",
      "coincident vertices",
      "colinear vertices",
      "FIST recovery process fatal error"
   };
   static char *nurbs_error[] = {
      "spline order un-supported",
      "too few knots",
      "valid knot range is empty",
      "decreasing knot sequence knot",
      "knot multiplicity greater than order of spline",
      "endcurve() must follow bgncurve()",
      "bgncurve() must precede endcurve()",
      "missing or extra geometric data",
      "can't draw pwlcurves",
      "missing bgncurve()",
      "missing bgnsurface()",
      "endtrim() must precede endsurface()",
      "bgnsurface() must precede endsurface()",
      "curve of improper type passed as trim curve",
      "bgnsurface() must precede bgntrim()",
      "endtrim() must follow bgntrim()",
      "bgntrim() must precede endtrim()",
      "invalid or missing trim curve",
      "bgntrim() must precede pwlcurve()",
      "pwlcurve referenced twice",
      "pwlcurve and nurbscurve mixed",
      "improper usage of trim data type",
      "nurbscurve referenced twice",
      "nurbscurve and pwlcurve mixed",
      "nurbssurface referenced twice",
      "invalid property",
      "endsurface() must follow bgnsurface()",
      "misoriented trim curves",
      "intersecting trim curves",
      "UNUSED",
      "unconnected trim curves",
      "unknown knot error",
      "negative vertex count encountered",
      "negative byte-stride encountered",
      "unknown type descriptor",
      "null control array or knot vector",
      "duplicate point on pwlcurve"
   };

   /* GL Errors */
   if (errorCode == GL_NO_ERROR) {
      return (GLubyte *) "no error";
   }
   else if (errorCode == GL_INVALID_VALUE) {
      return (GLubyte *) "invalid value";
   }
   else if (errorCode == GL_INVALID_ENUM) {
      return (GLubyte *) "invalid enum";
   }
   else if (errorCode == GL_INVALID_OPERATION) {
      return (GLubyte *) "invalid operation";
   }
   else if (errorCode == GL_STACK_OVERFLOW) {
      return (GLubyte *) "stack overflow";
   }
   else if (errorCode == GL_STACK_UNDERFLOW) {
      return (GLubyte *) "stack underflow";
   }
   else if (errorCode == GL_OUT_OF_MEMORY) {
      return (GLubyte *) "out of memory";
   }
   /* GLU Errors */
   else if (errorCode == GLU_NO_ERROR) {
      return (GLubyte *) "no error";
   }
   else if (errorCode == GLU_INVALID_ENUM) {
      return (GLubyte *) "invalid enum";
   }
   else if (errorCode == GLU_INVALID_VALUE) {
      return (GLubyte *) "invalid value";
   }
   else if (errorCode == GLU_OUT_OF_MEMORY) {
      return (GLubyte *) "out of memory";
   }
   else if (errorCode == GLU_INCOMPATIBLE_GL_VERSION) {
      return (GLubyte *) "incompatible GL version";
   }
   else if (errorCode >= GLU_TESS_ERROR1 && errorCode <= GLU_TESS_ERROR8) {
      return (GLubyte *) tess_error[errorCode - GLU_TESS_ERROR1];
   }
   else if (errorCode >= GLU_NURBS_ERROR1 && errorCode <= GLU_NURBS_ERROR37) {
      return (GLubyte *) nurbs_error[errorCode - GLU_NURBS_ERROR1];
   }
   else {
      return NULL;
   }
}



/*
 * New in GLU 1.1
 */

const GLubyte *GLAPIENTRY
gluGetString(GLenum name)
{
   static char *extensions = "GL_EXT_abgr";
   static char *version = "1.1 Mesa 3.5";

   switch (name) {
   case GLU_EXTENSIONS:
      return (GLubyte *) extensions;
   case GLU_VERSION:
      return (GLubyte *) version;
   default:
      return NULL;
   }
}



#if 0				/* gluGetProcAddressEXT not finalized yet! */

#ifdef __cplusplus
   /* for BeOS R4.5 */
void GLAPIENTRY(*gluGetProcAddressEXT(const GLubyte * procName)) (...)
#else
void (GLAPIENTRY * gluGetProcAddressEXT(const GLubyte * procName)) ()
#endif
{
   struct proc
   {
      const char *name;
      void *address;
   };
   static struct proc procTable[] = {
      {"gluGetProcAddressEXT", (void *) gluGetProcAddressEXT},	/* me! */

      /* new 1.1 functions */
      {"gluGetString", (void *) gluGetString},

      /* new 1.2 functions */
      {"gluTessBeginPolygon", (void *) gluTessBeginPolygon},
      {"gluTessBeginContour", (void *) gluTessBeginContour},
      {"gluTessEndContour", (void *) gluTessEndContour},
      {"gluTessEndPolygon", (void *) gluTessEndPolygon},
      {"gluGetTessProperty", (void *) gluGetTessProperty},

      /* new 1.3 functions */

      {NULL, NULL}
   };
   GLuint i;

   for (i = 0; procTable[i].address; i++) {
      if (strcmp((const char *) procName, procTable[i].name) == 0)
	 return (void (GLAPIENTRY *) ()) procTable[i].address;
   }

   return NULL;
}

#endif



/*
 * New in GLU 1.3
 */
#ifdef GLU_VERSION_1_3
GLboolean GLAPIENTRY
gluCheckExtension(const GLubyte *extName, const GLubyte * extString)
{
   assert(extName);
   assert(extString);
   {
      const int len = strlen((const char *) extName);
      const char *start = (const char *) extString;

      while (1) {
	 const char *c = strstr(start, (const char *) extName);
	 if (!c)
	    return GL_FALSE;

	 if ((c == start || c[-1] == ' ') && (c[len] == ' ' || c[len] == 0))
	    return GL_TRUE;

	 start = c + len;
      }
   }
}
#endif
