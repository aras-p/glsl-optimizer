
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1999-2000  Brian Paul
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


/* TODO:
 *   texture coordinate support
 *   flip normals according to orientation
 *   there's still some inside/outside orientation bugs in possibly all
 *     but the sphere function
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "gluP.h"
#endif



#ifndef M_PI
#  define M_PI (3.1415926)
#endif


/*
 * Convert degrees to radians:
 */
#define DEG_TO_RAD(A)   ((A)*(M_PI/180.0))


/*
 * Sin and Cos for degree angles:
 */
#define SIND( A )   sin( (A)*(M_PI/180.0) )
#define COSD( A)    cos( (A)*(M_PI/180.0) )


/*
 * Texture coordinates if texture flag is set
 */
#define TXTR_COORD(x,y)    if (qobj->TextureFlag) glTexCoord2f(x,y);



struct GLUquadric
{
   GLenum DrawStyle;		/* GLU_FILL, LINE, SILHOUETTE, or POINT */
   GLenum Orientation;		/* GLU_INSIDE or GLU_OUTSIDE */
   GLboolean TextureFlag;	/* Generate texture coords? */
   GLenum Normals;		/* GLU_NONE, GLU_FLAT, or GLU_SMOOTH */
   void (GLCALLBACK * ErrorFunc) (GLenum err);	/* Error handler callback function */
};



/*
 * Process a GLU error.
 */
static void
quadric_error(GLUquadricObj * qobj, GLenum error, const char *msg)
{
   /* Call the error call back function if any */
   if (qobj->ErrorFunc) {
      (*qobj->ErrorFunc) (error);
   }
   /* Print a message to stdout if MESA_DEBUG variable is defined */
   if (getenv("MESA_DEBUG")) {
      fprintf(stderr, "GLUError: %s: %s\n", (char *) gluErrorString(error),
	      msg);
   }
}




GLUquadricObj *GLAPIENTRY
gluNewQuadric(void)
{
   GLUquadricObj *q;

   q = (GLUquadricObj *) malloc(sizeof(struct GLUquadric));
   if (q) {
      q->DrawStyle = GLU_FILL;
      q->Orientation = GLU_OUTSIDE;
      q->TextureFlag = GL_FALSE;
      q->Normals = GLU_SMOOTH;
      q->ErrorFunc = NULL;
   }
   return q;
}



void GLAPIENTRY
gluDeleteQuadric(GLUquadricObj * state)
{
   if (state) {
      free((void *) state);
   }
}



/*
 * Set the drawing style to be GLU_FILL, GLU_LINE, GLU_SILHOUETTE,
 * or GLU_POINT.
 */
void GLAPIENTRY
gluQuadricDrawStyle(GLUquadricObj * quadObject, GLenum drawStyle)
{
   if (quadObject && (drawStyle == GLU_FILL || drawStyle == GLU_LINE
		      || drawStyle == GLU_SILHOUETTE
		      || drawStyle == GLU_POINT)) {
      quadObject->DrawStyle = drawStyle;
   }
   else {
      quadric_error(quadObject, GLU_INVALID_ENUM, "qluQuadricDrawStyle");
   }
}



/*
 * Set the orientation to GLU_INSIDE or GLU_OUTSIDE.
 */
void GLAPIENTRY
gluQuadricOrientation(GLUquadricObj * quadObject, GLenum orientation)
{
   if (quadObject
       && (orientation == GLU_INSIDE || orientation == GLU_OUTSIDE)) {
      quadObject->Orientation = orientation;
   }
   else {
      quadric_error(quadObject, GLU_INVALID_ENUM, "qluQuadricOrientation");
   }
}



/*
 * Set the error handler callback function.
 */
void GLAPIENTRY
gluQuadricCallback(GLUquadricObj * qobj,
		   GLenum which, void (GLCALLBACK * fn) ())
{
   /*
    * UGH, this is a mess!  I thought ANSI was a standard.
    */
   if (qobj && which == GLU_ERROR) {
#ifdef __CYGWIN32__
      qobj->ErrorFunc = (void (GLCALLBACKPCAST) (GLenum)) fn;
#elif defined(OPENSTEP)
      qobj->ErrorFunc = (void (*)(GLenum)) fn;
#elif defined(_WIN32)
      qobj->ErrorFunc = (void (GLCALLBACK *) (int)) fn;
#elif defined(__STORM__)
      qobj->ErrorFunc = (void (GLCALLBACK *) (GLenum)) fn;
#elif defined(__BEOS__)
      qobj->ErrorFunc = (void (*)(GLenum)) fn;
#else
      qobj->ErrorFunc = (void (GLCALLBACK *) ()) fn;
#endif
   }
}


void GLAPIENTRY
gluQuadricNormals(GLUquadricObj * quadObject, GLenum normals)
{
   if (quadObject
       && (normals == GLU_NONE || normals == GLU_FLAT
	   || normals == GLU_SMOOTH)) {
      quadObject->Normals = normals;
   }
}


void GLAPIENTRY
gluQuadricTexture(GLUquadricObj * quadObject, GLboolean textureCoords)
{
   if (quadObject) {
      quadObject->TextureFlag = textureCoords;
   }
}




/*
 * Call glNormal3f after scaling normal to unit length.
 */
static void
normal3f(GLfloat x, GLfloat y, GLfloat z)
{
}



void GLAPIENTRY
gluCylinder(GLUquadricObj * qobj,
	    GLdouble baseRadius, GLdouble topRadius,
	    GLdouble height, GLint slices, GLint stacks)
{
   GLdouble da, r, dr, dz;
   GLfloat x, y, z, nz, nsign;
   GLint i, j;

   if (qobj->Orientation == GLU_INSIDE) {
      nsign = -1.0;
   }
   else {
      nsign = 1.0;
   }

   da = 2.0 * M_PI / slices;
   dr = (topRadius - baseRadius) / stacks;
   dz = height / stacks;
   nz = (baseRadius - topRadius) / height;	/* Z component of normal vectors */

   if (qobj->DrawStyle == GLU_POINT) {
      glBegin(GL_POINTS);
      for (i = 0; i < slices; i++) {
	 x = cos(i * da);
	 y = sin(i * da);
	 normal3f(x * nsign, y * nsign, nz * nsign);

	 z = 0.0;
	 r = baseRadius;
	 for (j = 0; j <= stacks; j++) {
	    glVertex3f(x * r, y * r, z);
	    z += dz;
	    r += dr;
	 }
      }
      glEnd();
   }
   else if (qobj->DrawStyle == GLU_LINE || qobj->DrawStyle == GLU_SILHOUETTE) {
      /* Draw rings */
      if (qobj->DrawStyle == GLU_LINE) {
	 z = 0.0;
	 r = baseRadius;
	 for (j = 0; j <= stacks; j++) {
	    glBegin(GL_LINE_LOOP);
	    for (i = 0; i < slices; i++) {
	       x = cos(i * da);
	       y = sin(i * da);
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       glVertex3f(x * r, y * r, z);
	    }
	    glEnd();
	    z += dz;
	    r += dr;
	 }
      }
      else {
	 /* draw one ring at each end */
	 if (baseRadius != 0.0) {
	    glBegin(GL_LINE_LOOP);
	    for (i = 0; i < slices; i++) {
	       x = cos(i * da);
	       y = sin(i * da);
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       glVertex3f(x * baseRadius, y * baseRadius, 0.0);
	    }
	    glEnd();
	    glBegin(GL_LINE_LOOP);
	    for (i = 0; i < slices; i++) {
	       x = cos(i * da);
	       y = sin(i * da);
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       glVertex3f(x * topRadius, y * topRadius, height);
	    }
	    glEnd();
	 }
      }
      /* draw length lines */
      glBegin(GL_LINES);
      for (i = 0; i < slices; i++) {
	 x = cos(i * da);
	 y = sin(i * da);
	 normal3f(x * nsign, y * nsign, nz * nsign);
	 glVertex3f(x * baseRadius, y * baseRadius, 0.0);
	 glVertex3f(x * topRadius, y * topRadius, height);
      }
      glEnd();
   }
   else if (qobj->DrawStyle == GLU_FILL) {
      GLfloat ds = 1.0 / slices;
      GLfloat dt = 1.0 / stacks;
      GLfloat t = 0.0;
      z = 0.0;
      r = baseRadius;
      for (j = 0; j < stacks; j++) {
	 GLfloat s = 0.0;
	 glBegin(GL_QUAD_STRIP);
	 for (i = 0; i <= slices; i++) {
	    GLfloat x, y;
	    if (i == slices) {
	       x = sin(0.0);
	       y = cos(0.0);
	    }
	    else {
	       x = sin(i * da);
	       y = cos(i * da);
	    }
	    if (nsign == 1.0) {
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       TXTR_COORD(s, t);
	       glVertex3f(x * r, y * r, z);
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       TXTR_COORD(s, t + dt);
	       glVertex3f(x * (r + dr), y * (r + dr), z + dz);
	    }
	    else {
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       TXTR_COORD(s, t);
	       glVertex3f(x * r, y * r, z);
	       normal3f(x * nsign, y * nsign, nz * nsign);
	       TXTR_COORD(s, t + dt);
	       glVertex3f(x * (r + dr), y * (r + dr), z + dz);
	    }
	    s += ds;
	 }			/* for slices */
	 glEnd();
	 r += dr;
	 t += dt;
	 z += dz;
      }				/* for stacks */
   }
}





void GLAPIENTRY
gluSphere(GLUquadricObj * qobj, GLdouble radius, GLint slices, GLint stacks)
{
   GLfloat rho, drho, theta, dtheta;
   GLfloat x, y, z;
   GLfloat s, t, ds, dt;
   GLint i, j, imin, imax;
   GLboolean normals;
   GLfloat nsign;

   if (qobj->Normals == GLU_NONE) {
      normals = GL_FALSE;
   }
   else {
      normals = GL_TRUE;
   }
   if (qobj->Orientation == GLU_INSIDE) {
      nsign = -1.0;
   }
   else {
      nsign = 1.0;
   }

   drho = M_PI / (GLfloat) stacks;
   dtheta = 2.0 * M_PI / (GLfloat) slices;

   /* texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis */
   /* t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes) */
   /* cannot use triangle fan on texturing (s coord. at top/bottom tip varies) */

   if (qobj->DrawStyle == GLU_FILL) {
      if (!qobj->TextureFlag) {
	 /* draw +Z end as a triangle fan */
	 glBegin(GL_TRIANGLE_FAN);
/* 	 glNormal3f(0.0, 0.0, 1.0); */
	 glVertex3f(0.0, 0.0, nsign * radius);
	 for (j = 0; j <= slices; j++) {
	    theta = (j == slices) ? 0.0 : j * dtheta;
	    x = -sin(theta) * sin(drho);
	    y = cos(theta) * sin(drho);
	    z = nsign * cos(drho);
	    glVertex3f(x * radius, y * radius, z * radius);
	 }
	 glEnd();
      }

      ds = 1.0 / slices;
      dt = 1.0 / stacks;
      t = 1.0;			/* because loop now runs from 0 */
      if (qobj->TextureFlag) {
	 imin = 0;
	 imax = stacks;
      }
      else {
	 imin = 1;
	 imax = stacks - 1;
      }

      /* draw intermediate stacks as quad strips */
      for (i = imin; i < imax; i++) {
	 rho = i * drho;
	 glBegin(GL_QUAD_STRIP);
	 s = 0.0;
	 for (j = 0; j <= slices; j++) {
	    theta = (j == slices) ? 0.0 : j * dtheta;
	    x = -sin(theta) * sin(rho);
	    y = cos(theta) * sin(rho);
	    z = nsign * cos(rho);
	    TXTR_COORD(s, t);
	    glVertex3f(x * radius, y * radius, z * radius);
	    x = -sin(theta) * sin(rho + drho);
	    y = cos(theta) * sin(rho + drho);
	    z = nsign * cos(rho + drho);
	    TXTR_COORD(s, t - dt);
	    s += ds;
	    glVertex3f(x * radius, y * radius, z * radius);
	 }
	 glEnd();
	 t -= dt;
      }

      if (!qobj->TextureFlag) {
	 /* draw -Z end as a triangle fan */
	 glBegin(GL_TRIANGLE_FAN);
	 glVertex3f(0.0, 0.0, -radius * nsign);
	 rho = M_PI - drho;
	 s = 1.0;
	 t = dt;
	 for (j = slices; j >= 0; j--) {
	    theta = (j == slices) ? 0.0 : j * dtheta;
	    x = -sin(theta) * sin(rho);
	    y = cos(theta) * sin(rho);
	    z = nsign * cos(rho);
	    s -= ds;
	    glVertex3f(x * radius, y * radius, z * radius);
	 }
	 glEnd();
      }
   }
   else if (qobj->DrawStyle == GLU_LINE || qobj->DrawStyle == GLU_SILHOUETTE) {
      /* draw stack lines */
      for (i = 1; i < stacks; i++) {	/* stack line at i==stacks-1 was missing here */
	 rho = i * drho;
	 glBegin(GL_LINE_LOOP);
	 for (j = 0; j < slices; j++) {
	    theta = j * dtheta;
	    x = cos(theta) * sin(rho);
	    y = sin(theta) * sin(rho);
	    z = cos(rho);
	    glVertex3f(x * radius, y * radius, z * radius);
	 }
	 glEnd();
      }
      /* draw slice lines */
      for (j = 0; j < slices; j++) {
	 theta = j * dtheta;
	 glBegin(GL_LINE_STRIP);
	 for (i = 0; i <= stacks; i++) {
	    rho = i * drho;
	    x = cos(theta) * sin(rho);
	    y = sin(theta) * sin(rho);
	    z = cos(rho);
	    glVertex3f(x * radius, y * radius, z * radius);
	 }
	 glEnd();
      }
   }
   else if (qobj->DrawStyle == GLU_POINT) {
      /* top and bottom-most points */
      glBegin(GL_POINTS);
      glVertex3f(0.0, 0.0, radius);
      glVertex3f(0.0, 0.0, -radius);

      /* loop over stacks */
      for (i = 1; i < stacks - 1; i++) {
	 rho = i * drho;
	 for (j = 0; j < slices; j++) {
	    theta = j * dtheta;
	    x = cos(theta) * sin(rho);
	    y = sin(theta) * sin(rho);
	    z = cos(rho);
	    glVertex3f(x * radius, y * radius, z * radius);
	 }
      }
      glEnd();
   }

}



void GLAPIENTRY
gluDisk(GLUquadricObj * qobj,
	GLdouble innerRadius, GLdouble outerRadius, GLint slices, GLint loops)
{
   GLfloat da, dr;
#if 0
   GLdouble a, da;
   GLfloat r, dr;
   GLfloat x, y;
   GLfloat r1, r2, dtc;
   GLint s, l;
#endif


   da = 2.0 * M_PI / slices;
   dr = (outerRadius - innerRadius) / (GLfloat) loops;

   switch (qobj->DrawStyle) {
   case GLU_FILL:
      {
	 /* texture of a gluDisk is a cut out of the texture unit square
	  * x, y in [-outerRadius, +outerRadius]; s, t in [0, 1]
	  * (linear mapping)
	  */
	 GLfloat dtc = 2.0f * outerRadius;
	 GLfloat sa, ca;
	 GLfloat r1 = innerRadius;
	 GLint l;
	 for (l = 0; l < loops; l++) {
	    GLfloat r2 = r1 + dr;
	    if (qobj->Orientation == GLU_OUTSIDE) {
	       GLint s;
	       glBegin(GL_QUAD_STRIP);
	       for (s = 0; s <= slices; s++) {
		  GLfloat a;
		  if (s == slices)
		     a = 0.0;
		  else
		     a = s * da;
		  sa = sin(a);
		  ca = cos(a);
		  TXTR_COORD(0.5 + sa * r2 / dtc, 0.5 + ca * r2 / dtc);
		  glVertex2f(r2 * sa, r2 * ca);
		  TXTR_COORD(0.5 + sa * r1 / dtc, 0.5 + ca * r1 / dtc);
		  glVertex2f(r1 * sa, r1 * ca);
	       }
	       glEnd();
	    }
	    else {
	       GLint s;
	       glBegin(GL_QUAD_STRIP);
	       for (s = slices; s >= 0; s--) {
		  GLfloat a;
		  if (s == slices)
		     a = 0.0;
		  else
		     a = s * da;
		  sa = sin(a);
		  ca = cos(a);
		  TXTR_COORD(0.5 - sa * r2 / dtc, 0.5 + ca * r2 / dtc);
		  glVertex2f(r2 * sa, r2 * ca);
		  TXTR_COORD(0.5 - sa * r1 / dtc, 0.5 + ca * r1 / dtc);
		  glVertex2f(r1 * sa, r1 * ca);
	       }
	       glEnd();
	    }
	    r1 = r2;
	 }
	 break;
      }
   case GLU_LINE:
      {
	 GLint l, s;
	 /* draw loops */
	 for (l = 0; l <= loops; l++) {
	    GLfloat r = innerRadius + l * dr;
	    glBegin(GL_LINE_LOOP);
	    for (s = 0; s < slices; s++) {
	       GLfloat a = s * da;
	       glVertex2f(r * sin(a), r * cos(a));
	    }
	    glEnd();
	 }
	 /* draw spokes */
	 for (s = 0; s < slices; s++) {
	    GLfloat a = s * da;
	    GLfloat x = sin(a);
	    GLfloat y = cos(a);
	    glBegin(GL_LINE_STRIP);
	    for (l = 0; l <= loops; l++) {
	       GLfloat r = innerRadius + l * dr;
	       glVertex2f(r * x, r * y);
	    }
	    glEnd();
	 }
	 break;
      }
   case GLU_POINT:
      {
	 GLint s;
	 glBegin(GL_POINTS);
	 for (s = 0; s < slices; s++) {
	    GLfloat a = s * da;
	    GLfloat x = sin(a);
	    GLfloat y = cos(a);
	    GLint l;
	    for (l = 0; l <= loops; l++) {
	       GLfloat r = innerRadius * l * dr;
	       glVertex2f(r * x, r * y);
	    }
	 }
	 glEnd();
	 break;
      }
   case GLU_SILHOUETTE:
      {
	 if (innerRadius != 0.0) {
	    GLfloat a;
	    glBegin(GL_LINE_LOOP);
	    for (a = 0.0; a < 2.0 * M_PI; a += da) {
	       GLfloat x = innerRadius * sin(a);
	       GLfloat y = innerRadius * cos(a);
	       glVertex2f(x, y);
	    }
	    glEnd();
	 }
	 {
	    GLfloat a;
	    glBegin(GL_LINE_LOOP);
	    for (a = 0; a < 2.0 * M_PI; a += da) {
	       GLfloat x = outerRadius * sin(a);
	       GLfloat y = outerRadius * cos(a);
	       glVertex2f(x, y);
	    }
	    glEnd();
	 }
	 break;
      }
   default:
      abort();
   }
}



void GLAPIENTRY
gluPartialDisk(GLUquadricObj * qobj, GLdouble innerRadius,
	       GLdouble outerRadius, GLint slices, GLint loops,
	       GLdouble startAngle, GLdouble sweepAngle)
{
   if (qobj->DrawStyle == GLU_POINT) {
      GLint loop, slice;
      GLdouble radius, delta_radius;
      GLdouble angle, delta_angle;
      delta_radius = (outerRadius - innerRadius) / (loops - 1);
      delta_angle = DEG_TO_RAD((sweepAngle) / (slices - 1));
      glBegin(GL_POINTS);
      radius = innerRadius;
      for (loop = 0; loop < loops; loop++) {
	 angle = DEG_TO_RAD(startAngle);
	 for (slice = 0; slice < slices; slice++) {
	    glVertex2f(radius * sin(angle), radius * cos(angle));
	    angle += delta_angle;
	 }
	 radius += delta_radius;
      }
      glEnd();
   }
   else if (qobj->DrawStyle == GLU_LINE) {
      GLint loop, slice;
      GLdouble radius, delta_radius;
      GLdouble angle, delta_angle;
      delta_radius = (outerRadius - innerRadius) / loops;
      delta_angle = DEG_TO_RAD(sweepAngle / slices);
      /* draw rings */
      radius = innerRadius;
      for (loop = 0; loop < loops; loop++) {
	 angle = DEG_TO_RAD(startAngle);
	 glBegin(GL_LINE_STRIP);
	 for (slice = 0; slice <= slices; slice++) {
	    glVertex2f(radius * sin(angle), radius * cos(angle));
	    angle += delta_angle;
	 }
	 glEnd();
	 radius += delta_radius;
      }
      /* draw spokes */
      angle = DEG_TO_RAD(startAngle);
      for (slice = 0; slice <= slices; slice++) {
	 radius = innerRadius;
	 glBegin(GL_LINE_STRIP);
	 for (loop = 0; loop < loops; loop++) {
	    glVertex2f(radius * sin(angle), radius * cos(angle));
	    radius += delta_radius;
	 }
	 glEnd();
	 angle += delta_angle;
      }
   }
   else if (qobj->DrawStyle == GLU_SILHOUETTE) {
      GLint slice;
      GLdouble angle, delta_angle;
      delta_angle = DEG_TO_RAD(sweepAngle / slices);
      /* draw outer ring */
      glBegin(GL_LINE_STRIP);
      angle = DEG_TO_RAD(startAngle);
      for (slice = 0; slice <= slices; slice++) {
	 glVertex2f(outerRadius * sin(angle), outerRadius * cos(angle));
	 angle += delta_angle;
      }
      glEnd();
      /* draw inner ring */
      if (innerRadius > 0.0) {
	 glBegin(GL_LINE_STRIP);
	 angle = DEG_TO_RAD(startAngle);
	 for (slice = 0; slice < slices; slice++) {
	    glVertex2f(innerRadius * sin(angle), innerRadius * cos(angle));
	    angle += delta_angle;
	 }
	 glEnd();
      }
      /* draw spokes */
      if (sweepAngle < 360.0) {
	 GLdouble stopAngle = startAngle + sweepAngle;
	 glBegin(GL_LINES);
	 glVertex2f(innerRadius * SIND(startAngle),
		    innerRadius * COSD(startAngle));
	 glVertex2f(outerRadius * SIND(startAngle),
		    outerRadius * COSD(startAngle));
	 glVertex2f(innerRadius * SIND(stopAngle),
		    innerRadius * COSD(stopAngle));
	 glVertex2f(outerRadius * SIND(stopAngle),
		    outerRadius * COSD(stopAngle));
	 glEnd();
      }
   }
   else if (qobj->DrawStyle == GLU_FILL) {
      GLint loop, slice;
      GLdouble radius, delta_radius;
      GLdouble angle, delta_angle;
      delta_radius = (outerRadius - innerRadius) / loops;
      delta_angle = DEG_TO_RAD(sweepAngle / slices);
      radius = innerRadius;
      for (loop = 0; loop < loops; loop++) {
	 glBegin(GL_QUAD_STRIP);
	 angle = DEG_TO_RAD(startAngle);
	 for (slice = 0; slice <= slices; slice++) {
	    if (qobj->Orientation == GLU_OUTSIDE) {
	       glVertex2f((radius + delta_radius) * sin(angle),
			  (radius + delta_radius) * cos(angle));
	       glVertex2f(radius * sin(angle), radius * cos(angle));
	    }
	    else {
	       glVertex2f(radius * sin(angle), radius * cos(angle));
	       glVertex2f((radius + delta_radius) * sin(angle),
			  (radius + delta_radius) * cos(angle));
	    }
	    angle += delta_angle;
	 }
	 glEnd();
	 radius += delta_radius;
      }
   }
}
