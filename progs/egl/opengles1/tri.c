/*
 * Copyright (C) 2008  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Draw a triangle with X/EGL and OpenGL ES 1.x
 * Brian Paul
 * 5 June 2008
 */

#define USE_FIXED_POINT 0


#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <GLES/gl.h>  /* use OpenGL ES 1.x */
#include <GLES/glext.h>
#include <EGL/egl.h>

#include "eglut.h"


#define FLOAT_TO_FIXED(X)   ((X) * 65535.0)



static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;


static void
draw(void)
{
#if USE_FIXED_POINT
   static const GLfixed verts[3][2] = {
      { -65536, -65536 },
      {  65536, -65536 },
      {      0,  65536 }
   };
   static const GLfixed colors[3][4] = {
      { 65536,     0,     0,    65536 },
      {     0, 65536,     0 ,   65536},
      {     0,     0, 65536 ,   65536}
   };
#else
   static const GLfloat verts[3][2] = {
      { -1, -1 },
      {  1, -1 },
      {  0,  1 }
   };
   static const GLfloat colors[3][4] = {
      { 1, 0, 0, 1 },
      { 0, 1, 0, 1 },
      { 0, 0, 1, 1 }
   };
#endif

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1, 0, 0);
   glRotatef(view_roty, 0, 1, 0);
   glRotatef(view_rotz, 0, 0, 1);

   {
#if USE_FIXED_POINT
      glVertexPointer(2, GL_FIXED, 0, verts);
      glColorPointer(4, GL_FIXED, 0, colors);
#else
      glVertexPointer(2, GL_FLOAT, 0, verts);
      glColorPointer(4, GL_FLOAT, 0, colors);
#endif
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_COLOR_ARRAY);

      /* draw triangle */
      glDrawArrays(GL_TRIANGLES, 0, 3);

      /* draw some points */
      glPointSizex(FLOAT_TO_FIXED(15.5));
      glDrawArrays(GL_POINTS, 0, 3);

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_COLOR_ARRAY);
   }

   if (0) {
      /* test code */
      GLfixed size;
      glGetFixedv(GL_POINT_SIZE, &size);
      printf("GL_POINT_SIZE = 0x%x %f\n", size, size / 65536.0);
   }

   glPopMatrix();
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
#ifdef GL_VERSION_ES_CM_1_0
   glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
#else
   glFrustum(-ar, ar, -1, 1, 5.0, 60.0);
#endif
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
}


static void
test_query_matrix(void)
{
   PFNGLQUERYMATRIXXOESPROC procQueryMatrixx;
   typedef void (*voidproc)();
   GLfixed mantissa[16];
   GLint exponent[16];
   GLbitfield rv;
   int i;

   procQueryMatrixx = (PFNGLQUERYMATRIXXOESPROC) eglGetProcAddress("glQueryMatrixxOES");
   assert(procQueryMatrixx);
   /* Actually try out this one */
   rv = (*procQueryMatrixx)(mantissa, exponent);
   for (i = 0; i < 16; i++) {
      if (rv & (1<<i)) {
        printf("matrix[%d] invalid\n", i);
      }
      else {
         printf("matrix[%d] = %f * 2^(%d)\n", i, mantissa[i]/65536.0, exponent[i]);
      }
   }
   assert(!eglGetProcAddress("glFoo"));
}


static void
init(void)
{
   glClearColor(0.4, 0.4, 0.4, 0.0);

   if (0)
      test_query_matrix();
}

static void
special_key(int special)
{
   switch (special) {
   case EGLUT_KEY_LEFT:
      view_roty += 5.0;
      break;
   case EGLUT_KEY_RIGHT:
      view_roty -= 5.0;
      break;
   case EGLUT_KEY_UP:
      view_rotx += 5.0;
      break;
   case EGLUT_KEY_DOWN:
      view_rotx -= 5.0;
      break;
   default:
      break;
   }
}

int
main(int argc, char *argv[])
{
   eglutInitWindowSize(300, 300);
   eglutInitAPIMask(EGLUT_OPENGL_ES1_BIT);
   eglutInit(argc, argv);

   eglutCreateWindow("tri");

   eglutReshapeFunc(reshape);
   eglutDisplayFunc(draw);
   eglutSpecialFunc(special_key);

   init();

   eglutMainLoop();

   return 0;
}
