/* $Id: glu.c,v 1.18 1999/11/22 22:15:50 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-1999  Brian Paul
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
 * $Log: glu.c,v $
 * Revision 1.18  1999/11/22 22:15:50  brianp
 * removed GLU_EXT_get_proc_address from ext strings
 *
 * Revision 1.17  1999/11/19 21:23:37  brianp
 * replace encounteed with encountered
 *
 * Revision 1.16  1999/10/27 09:47:41  brianp
 * disabled gluGetProcAddressEXT
 *
 * Revision 1.15  1999/09/19 02:03:19  tjump
 * More Win32 build compliance fixups
 *
 * Revision 1.14  1999/09/17 12:21:53  brianp
 * glGetProcAddressEXT changes to accomodate Win32 and non-Win32
 *
 * Revision 1.13  1999/09/17 03:17:18  tjump
 * Patch error fixup
 *
 * Revision 1.12  1999/09/17 03:07:28  tjump
 * Win32 build req't updates
 *
 * Revision 1.11  1999/09/17 01:00:38  brianp
 * fixed typo
 *
 * Revision 1.10  1999/09/17 00:06:14  brianp
 * gluGetProcAddressEXT change for C++ / BeOS
 *
 * Revision 1.9  1999/09/16 22:37:56  brianp
 * added some casts in gluGetProcAddressEXT()
 *
 * Revision 1.8  1999/09/16 16:53:28  brianp
 * clean-up of GLU_EXT_get_proc_address
 *
 * Revision 1.7  1999/09/14 00:11:40  brianp
 * added gluCheckExtension()
 *
 * Revision 1.6  1999/09/13 14:31:32  joukj
 *
 * strcmp needs the string.h
 *
 * Revision 1.5  1999/09/11 12:04:54  brianp
 * added 1.2 function to gluGetProcAddressEXT()
 *
 * Revision 1.4  1999/09/11 11:36:26  brianp
 * added GLU_EXT_get_proc_address
 *
 * Revision 1.3  1999/09/10 04:32:10  gareth
 * Fixed triangle output, recovery process termination.
 *
 * Revision 1.2  1999/09/10 02:03:31  gareth
 * Added GLU 1.3 tessellation (except winding rule code).
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.13  1999/03/31 19:07:28  brianp
 * added GL_EXT_abgr to extensions
 *
 * Revision 1.12  1999/02/06 06:12:41  brianp
 * updated version string to 3.1
 *
 * Revision 1.11  1999/01/03 03:23:15  brianp
 * now using GLAPIENTRY and GLCALLBACK keywords (Ted Jump)
 *
 * Revision 1.10  1998/04/22 00:35:50  brianp
 * changed version to 3.0
 *
 * Revision 1.9  1997/12/09 03:03:32  brianp
 * changed version to 2.6
 *
 * Revision 1.8  1997/10/04 01:30:20  brianp
 * changed version to 2.5
 *
 * Revision 1.7  1997/08/13 01:25:21  brianp
 * changed version string to 2.4
 *
 * Revision 1.6  1997/07/24 01:28:44  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.5  1997/07/13 22:59:11  brianp
 * added const to viewport parameter of gluPickMatrix()
 *
 * Revision 1.4  1997/05/28 02:29:38  brianp
 * added support for precompiled headers (PCH), inserted APIENTRY keyword
 *
 * Revision 1.3  1997/04/12 16:19:02  brianp
 * changed version to 2.3
 *
 * Revision 1.2  1997/03/11 00:58:34  brianp
 * changed version to 2.2
 *
 * Revision 1.1  1996/09/27 01:19:39  brianp
 * Initial revision
 *
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




void GLAPIENTRY gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
                         GLdouble centerx, GLdouble centery, GLdouble centerz,
                         GLdouble upx, GLdouble upy, GLdouble upz )
{
   GLdouble m[16];
   GLdouble x[3], y[3], z[3];
   GLdouble mag;

   /* Make rotation matrix */

   /* Z vector */
   z[0] = eyex - centerx;
   z[1] = eyey - centery;
   z[2] = eyez - centerz;
   mag = sqrt( z[0]*z[0] + z[1]*z[1] + z[2]*z[2] );
   if (mag) {  /* mpichler, 19950515 */
      z[0] /= mag;
      z[1] /= mag;
      z[2] /= mag;
   }

   /* Y vector */
   y[0] = upx;
   y[1] = upy;
   y[2] = upz;

   /* X vector = Y cross Z */
   x[0] =  y[1]*z[2] - y[2]*z[1];
   x[1] = -y[0]*z[2] + y[2]*z[0];
   x[2] =  y[0]*z[1] - y[1]*z[0];

   /* Recompute Y = Z cross X */
   y[0] =  z[1]*x[2] - z[2]*x[1];
   y[1] = -z[0]*x[2] + z[2]*x[0];
   y[2] =  z[0]*x[1] - z[1]*x[0];

   /* mpichler, 19950515 */
   /* cross product gives area of parallelogram, which is < 1.0 for
    * non-perpendicular unit-length vectors; so normalize x, y here
    */

   mag = sqrt( x[0]*x[0] + x[1]*x[1] + x[2]*x[2] );
   if (mag) {
      x[0] /= mag;
      x[1] /= mag;
      x[2] /= mag;
   }

   mag = sqrt( y[0]*y[0] + y[1]*y[1] + y[2]*y[2] );
   if (mag) {
      y[0] /= mag;
      y[1] /= mag;
      y[2] /= mag;
   }

#define M(row,col)  m[col*4+row]
   M(0,0) = x[0];  M(0,1) = x[1];  M(0,2) = x[2];  M(0,3) = 0.0;
   M(1,0) = y[0];  M(1,1) = y[1];  M(1,2) = y[2];  M(1,3) = 0.0;
   M(2,0) = z[0];  M(2,1) = z[1];  M(2,2) = z[2];  M(2,3) = 0.0;
   M(3,0) = 0.0;   M(3,1) = 0.0;   M(3,2) = 0.0;   M(3,3) = 1.0;
#undef M
   glMultMatrixd( m );

   /* Translate Eye to Origin */
   glTranslated( -eyex, -eyey, -eyez );

}



void GLAPIENTRY gluOrtho2D( GLdouble left, GLdouble right,
                          GLdouble bottom, GLdouble top )
{
   glOrtho( left, right, bottom, top, -1.0, 1.0 );
}



void GLAPIENTRY gluPerspective( GLdouble fovy, GLdouble aspect,
                              GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}



void GLAPIENTRY gluPickMatrix( GLdouble x, GLdouble y,
                             GLdouble width, GLdouble height,
                             const GLint viewport[4] )
{
   GLfloat m[16];
   GLfloat sx, sy;
   GLfloat tx, ty;

   sx = viewport[2] / width;
   sy = viewport[3] / height;
   tx = (viewport[2] + 2.0 * (viewport[0] - x)) / width;
   ty = (viewport[3] + 2.0 * (viewport[1] - y)) / height;

#define M(row,col)  m[col*4+row]
   M(0,0) = sx;   M(0,1) = 0.0;  M(0,2) = 0.0;  M(0,3) = tx;
   M(1,0) = 0.0;  M(1,1) = sy;   M(1,2) = 0.0;  M(1,3) = ty;
   M(2,0) = 0.0;  M(2,1) = 0.0;  M(2,2) = 1.0;  M(2,3) = 0.0;
   M(3,0) = 0.0;  M(3,1) = 0.0;  M(3,2) = 0.0;  M(3,3) = 1.0;
#undef M

   glMultMatrixf( m );
}



const GLubyte* GLAPIENTRY gluErrorString( GLenum errorCode )
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
   if (errorCode==GL_NO_ERROR) {
      return (GLubyte *) "no error";
   }
   else if (errorCode==GL_INVALID_VALUE) {
      return (GLubyte *) "invalid value";
   }
   else if (errorCode==GL_INVALID_ENUM) {
      return (GLubyte *) "invalid enum";
   }
   else if (errorCode==GL_INVALID_OPERATION) {
      return (GLubyte *) "invalid operation";
   }
   else if (errorCode==GL_STACK_OVERFLOW) {
      return (GLubyte *) "stack overflow";
   }
   else if (errorCode==GL_STACK_UNDERFLOW) {
      return (GLubyte *) "stack underflow";
   }
   else if (errorCode==GL_OUT_OF_MEMORY) {
      return (GLubyte *) "out of memory";
   }
   /* GLU Errors */
   else if (errorCode==GLU_NO_ERROR) {
      return (GLubyte *) "no error";
   }
   else if (errorCode==GLU_INVALID_ENUM) {
      return (GLubyte *) "invalid enum";
   }
   else if (errorCode==GLU_INVALID_VALUE) {
      return (GLubyte *) "invalid value";
   }
   else if (errorCode==GLU_OUT_OF_MEMORY) {
      return (GLubyte *) "out of memory";
   }
   else if (errorCode==GLU_INCOMPATIBLE_GL_VERSION) {
      return (GLubyte *) "incompatible GL version";
   }
   else if (errorCode>=GLU_TESS_ERROR1 && errorCode<=GLU_TESS_ERROR8) {
      return (GLubyte *) tess_error[errorCode-GLU_TESS_ERROR1];
   }
   else if (errorCode>=GLU_NURBS_ERROR1 && errorCode<=GLU_NURBS_ERROR37) {
      return (GLubyte *) nurbs_error[errorCode-GLU_NURBS_ERROR1];
   }
   else {
      return NULL;
   }
}



/*
 * New in GLU 1.1
 */

const GLubyte* GLAPIENTRY gluGetString( GLenum name )
{
   static char *extensions = "GL_EXT_abgr";
   static char *version = "1.2 Mesa 3.1";

   switch (name) {
      case GLU_EXTENSIONS:
         return (GLubyte *) extensions;
      case GLU_VERSION:
	 return (GLubyte *) version;
      default:
	 return NULL;
   }
}



#if 0  /* gluGetProcAddressEXT not finalized yet! */

#ifdef __cplusplus
   /* for BeOS R4.5 */
   void GLAPIENTRY (*gluGetProcAddressEXT(const GLubyte *procName))(...)
#else
   void (GLAPIENTRY *gluGetProcAddressEXT(const GLubyte *procName))()
#endif
{
   struct proc {
      const char *name;
      void *address;
   };
   static struct proc procTable[] = {
      { "gluGetProcAddressEXT", (void *) gluGetProcAddressEXT },  /* me! */

      /* new 1.1 functions */
      { "gluGetString", (void *) gluGetString },

      /* new 1.2 functions */
      { "gluTessBeginPolygon", (void *) gluTessBeginPolygon },
      { "gluTessBeginContour", (void *) gluTessBeginContour },
      { "gluTessEndContour", (void *) gluTessEndContour },
      { "gluTessEndPolygon", (void *) gluTessEndPolygon },
      { "gluGetTessProperty", (void *) gluGetTessProperty },

      /* new 1.3 functions */

      { NULL, NULL }
   };
   GLuint i;

   for (i = 0; procTable[i].address; i++) {
      if (strcmp((const char *) procName, procTable[i].name) == 0)
         return (void (GLAPIENTRY *)()) procTable[i].address;
   }

   return NULL;
}

#endif



/*
 * New in GLU 1.3
 */
GLboolean GLAPIENTRY
gluCheckExtension( const char *extName, const GLubyte *extString )
{
   assert(extName);
   assert(extString);
   {
      const int len = strlen(extName);
      const char *start = (const char *) extString;

      while (1) {
         const char *c = strstr( start, extName );
         if (!c)
            return GL_FALSE;

         if ((c == start || c[-1] == ' ') && (c[len] == ' ' || c[len] == 0))
            return GL_TRUE;

         start = c + len;
      }
   }
}
