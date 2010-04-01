/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
 *
 * Based on eglgears by
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <GLES/gl.h>
#include "eglut.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif


struct gear {
   GLuint vbo;
   GLfloat *vertices;
   GLsizei stride;

   GLint num_teeth;
};

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static struct gear gears[3];
static GLfloat angle = 0.0;

/*
 *  Initialize a gear wheel.
 *
 *  Input:  gear - gear to initialize
 *          inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static void
init_gear(struct gear *gear, GLfloat inner_radius, GLfloat outer_radius,
          GLfloat width, GLint teeth, GLfloat tooth_depth)
{
   GLfloat r0, r1, r2;
   GLfloat a0, da;
   GLint verts_per_tooth, total_verts, total_size;
   GLint count, i;
   GLfloat *verts;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   a0 = 2.0 * M_PI / teeth;
   da = a0 / 4.0;

   gear->vbo = 0;
   gear->vertices = NULL;
   gear->stride = sizeof(GLfloat) * 6; /* XYZ + normal */
   gear->num_teeth = teeth;

   verts_per_tooth = 10 + 4;
   total_verts = teeth * verts_per_tooth;
   total_size = total_verts * gear->stride;

   verts = malloc(total_size);
   if (!verts) {
      printf("failed to allocate vertices\n");
      return;
   }

#define GEAR_VERT(r, n, sign)                      \
   do {                                            \
      verts[count * 6 + 0] = (r) * vx[n];          \
      verts[count * 6 + 1] = (r) * vy[n];          \
      verts[count * 6 + 2] = (sign) * width * 0.5; \
      verts[count * 6 + 3] = normal[0];            \
      verts[count * 6 + 4] = normal[1];            \
      verts[count * 6 + 5] = normal[2];            \
      count++;                                     \
   } while (0)

   count = 0;
   for (i = 0; i < teeth; i++) {
      GLfloat normal[3];
      GLfloat vx[5], vy[5];
      GLfloat u, v;

      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 0.0;

      vx[0] = cos(i * a0 + 0 * da);
      vy[0] = sin(i * a0 + 0 * da);
      vx[1] = cos(i * a0 + 1 * da);
      vy[1] = sin(i * a0 + 1 * da);
      vx[2] = cos(i * a0 + 2 * da);
      vy[2] = sin(i * a0 + 2 * da);
      vx[3] = cos(i * a0 + 3 * da);
      vy[3] = sin(i * a0 + 3 * da);
      vx[4] = cos(i * a0 + 4 * da);
      vy[4] = sin(i * a0 + 4 * da);

      /* outward faces of a tooth, 10 verts */
      normal[0] = vx[0];
      normal[1] = vy[0];
      GEAR_VERT(r1, 0,  1);
      GEAR_VERT(r1, 0, -1);

      u = r2 * vx[1] - r1 * vx[0];
      v = r2 * vy[1] - r1 * vy[0];
      normal[0] = v;
      normal[1] = -u;
      GEAR_VERT(r2, 1,  1);
      GEAR_VERT(r2, 1, -1);

      normal[0] = vx[0];
      normal[1] = vy[0];
      GEAR_VERT(r2, 2,  1);
      GEAR_VERT(r2, 2, -1);

      u = r1 * vx[3] - r2 * vx[2];
      v = r1 * vy[3] - r2 * vy[2];
      normal[0] = v;
      normal[1] = -u;
      GEAR_VERT(r1, 3,  1);
      GEAR_VERT(r1, 3, -1);

      normal[0] = vx[0];
      normal[1] = vy[0];
      GEAR_VERT(r1, 4,  1);
      GEAR_VERT(r1, 4, -1);

      /* inside radius cylinder, 4 verts */
      normal[0] = -vx[4];
      normal[1] = -vy[4];
      GEAR_VERT(r0, 4,  1);
      GEAR_VERT(r0, 4, -1);

      normal[0] = -vx[0];
      normal[1] = -vy[0];
      GEAR_VERT(r0, 0,  1);
      GEAR_VERT(r0, 0, -1);

      assert(count % verts_per_tooth == 0);
   }
   assert(count == total_verts);
#undef GEAR_VERT

   gear->vertices = verts;

   /* setup VBO */
   glGenBuffers(1, &gear->vbo);
   if (gear->vbo) {
      glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
      glBufferData(GL_ARRAY_BUFFER, total_size, verts, GL_STATIC_DRAW);
   }
}


static void
draw_gear(const struct gear *gear)
{
   GLint i;

   if (!gear->vbo && !gear->vertices) {
      printf("nothing to be drawn\n");
      return;
   }

   if (gear->vbo) {
      glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
      glVertexPointer(3, GL_FLOAT, gear->stride, (const GLvoid *) 0);
      glNormalPointer(GL_FLOAT, gear->stride, (const GLvoid *) (sizeof(GLfloat) * 3));
   } else {
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glVertexPointer(3, GL_FLOAT, gear->stride, gear->vertices);
      glNormalPointer(GL_FLOAT, gear->stride, gear->vertices + 3);
   }

   glEnableClientState(GL_VERTEX_ARRAY);

   for (i = 0; i < gear->num_teeth; i++) {
      const GLint base = (10 + 4) * i;
      GLushort indices[7];

      glShadeModel(GL_FLAT);

      /* front face */
      indices[0] = base + 12;
      indices[1] = base +  0;
      indices[2] = base +  2;
      indices[3] = base +  4;
      indices[4] = base +  6;
      indices[5] = base +  8;
      indices[6] = base + 10;

      glNormal3f(0.0, 0.0, 1.0);
      glDrawElements(GL_TRIANGLE_FAN, 7, GL_UNSIGNED_SHORT, indices);

      /* back face */
      indices[0] = base + 13;
      indices[1] = base + 11;
      indices[2] = base +  9;
      indices[3] = base +  7;
      indices[4] = base +  5;
      indices[5] = base +  3;
      indices[6] = base +  1;

      glNormal3f(0.0, 0.0, -1.0);
      glDrawElements(GL_TRIANGLE_FAN, 7, GL_UNSIGNED_SHORT, indices);

      glEnableClientState(GL_NORMAL_ARRAY);

      /* outward face of a tooth */
      glDrawArrays(GL_TRIANGLE_STRIP, base, 10);

      /* inside radius cylinder */
      glShadeModel(GL_SMOOTH);
      glDrawArrays(GL_TRIANGLE_STRIP, base + 10, 4);

      glDisableClientState(GL_NORMAL_ARRAY);
   }

   glDisableClientState(GL_VERTEX_ARRAY);
}


static void
gears_draw(void)
{
   static const GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static const GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static const GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);

   glPushMatrix();
   glTranslatef(-3.0, -2.0, 0.0);
   glRotatef(angle, 0.0, 0.0, 1.0);

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
   draw_gear(&gears[0]);

   glPopMatrix();

   glPushMatrix();
   glTranslatef(3.1, -2.0, 0.0);
   glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
   draw_gear(&gears[1]);

   glPopMatrix();

   glPushMatrix();
   glTranslatef(-3.1, 4.2, 0.0);
   glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
   draw_gear(&gears[2]);

   glPopMatrix();

   glPopMatrix();
}


static void gears_fini(void)
{
   GLint i;
   for (i = 0; i < 3; i++) {
      struct gear *gear = &gears[i];
      if (gear->vbo) {
         glDeleteBuffers(1, &gear->vbo);
         gear->vbo = 0;
      }
      if (gear->vertices) {
         free(gear->vertices);
         gear->vertices = NULL;
      }
   }
}


static void gears_init(void)
{
   static const GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };

   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_CULL_FACE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_NORMALIZE);

   init_gear(&gears[0], 1.0, 4.0, 1.0, 20, 0.7);
   init_gear(&gears[1], 0.5, 2.0, 2.0, 10, 0.7);
   init_gear(&gears[2], 1.3, 2.0, 0.5, 10, 0.7);
}


/* new window size or exposure */
static void
gears_reshape(int width, int height)
{
   GLfloat h = (GLfloat) height / (GLfloat) width;

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustumf(-1.0, 1.0, -h, h, 5.0, 60.0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);
}


static void
gears_idle(void)
{
  static double t0 = -1.;
  double dt, t = eglutGet(EGLUT_ELAPSED_TIME) / 1000.0;
  if (t0 < 0.0)
    t0 = t;
  dt = t - t0;
  t0 = t;

  angle += 70.0 * dt;  /* 70 degrees per second */
  angle = fmod(angle, 360.0); /* prevents eventual overflow */

  eglutPostRedisplay();
}


int
main(int argc, char *argv[])
{
   eglutInitWindowSize(300, 300);
   eglutInitAPIMask(EGLUT_OPENGL_ES1_BIT);
   eglutInit(argc, argv);

   eglutCreateWindow("gears");

   eglutIdleFunc(gears_idle);
   eglutReshapeFunc(gears_reshape);
   eglutDisplayFunc(gears_draw);

   gears_init();

   eglutMainLoop();

   gears_fini();

   return 0;
}
