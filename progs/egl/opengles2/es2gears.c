/*
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

/*
 * Ported to GLES2.
 * Kristian Høgsberg <krh@bitplanet.net>
 * May 3, 2010
 */

/*
 * Command line options:
 *    -info      print GL implementation information
 *
 */


#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "eglut.h"

struct gear {
   GLfloat *vertices;
   GLuint vbo;
   int count;
};

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static struct gear *gear1, *gear2, *gear3;
static GLfloat angle = 0.0;
static GLuint proj_location, light_location, color_location;
static GLfloat proj[16];

static GLfloat *
vert(GLfloat *p, GLfloat x, GLfloat y, GLfloat z, GLfloat *n)
{
   p[0] = x;
   p[1] = y;
   p[2] = z;
   p[3] = n[0];
   p[4] = n[1];
   p[5] = n[2];

   return p + 6;
}

/*  Draw a gear wheel.  You'll probably want to call this function when
 *  building a display list since we do a lot of trig here.
 * 
 *  Input:  inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static struct gear *
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat da;
   GLfloat *p, *v;
   struct gear *gear;
   double s[5], c[5];
   GLfloat verts[3 * 14], normal[3];
   const int tris_per_tooth = 20;

   gear = malloc(sizeof *gear);
   if (gear == NULL)
      return NULL;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / teeth / 4.0;

   gear->vertices = calloc(teeth * tris_per_tooth * 3 * 6,
			   sizeof *gear->vertices);
   s[4] = 0;
   c[4] = 1;
   v = gear->vertices;
   for (i = 0; i < teeth; i++) {
      s[0] = s[4];
      c[0] = c[4];
      sincos(i * 2.0 * M_PI / teeth + da, &s[1], &c[1]);
      sincos(i * 2.0 * M_PI / teeth + da * 2, &s[2], &c[2]);
      sincos(i * 2.0 * M_PI / teeth + da * 3, &s[3], &c[3]);
      sincos(i * 2.0 * M_PI / teeth + da * 4, &s[4], &c[4]);

      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 1.0;

      v = vert(v, r2 * c[1], r2 * s[1], width * 0.5, normal);

      v = vert(v, r2 * c[1], r2 * s[1], width * 0.5, normal);
      v = vert(v, r2 * c[2], r2 * s[2], width * 0.5, normal);
      v = vert(v, r1 * c[0], r1 * s[0], width * 0.5, normal);
      v = vert(v, r1 * c[3], r1 * s[3], width * 0.5, normal);
      v = vert(v, r0 * c[0], r0 * s[0], width * 0.5, normal);
      v = vert(v, r1 * c[4], r1 * s[4], width * 0.5, normal);
      v = vert(v, r0 * c[4], r0 * s[4], width * 0.5, normal);

      v = vert(v, r0 * c[4], r0 * s[4], width * 0.5, normal);
      v = vert(v, r0 * c[0], r0 * s[0], width * 0.5, normal);
      v = vert(v, r0 * c[4], r0 * s[4], -width * 0.5, normal);
      v = vert(v, r0 * c[0], r0 * s[0], -width * 0.5, normal);

      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = -1.0;

      v = vert(v, r0 * c[4], r0 * s[4], -width * 0.5, normal);

      v = vert(v, r0 * c[4], r0 * s[4], -width * 0.5, normal);
      v = vert(v, r1 * c[4], r1 * s[4], -width * 0.5, normal);
      v = vert(v, r0 * c[0], r0 * s[0], -width * 0.5, normal);
      v = vert(v, r1 * c[3], r1 * s[3], -width * 0.5, normal);
      v = vert(v, r1 * c[0], r1 * s[0], -width * 0.5, normal);
      v = vert(v, r2 * c[2], r2 * s[2], -width * 0.5, normal);
      v = vert(v, r2 * c[1], r2 * s[1], -width * 0.5, normal);

      v = vert(v, r1 * c[0], r1 * s[0], width * 0.5, normal);

      v = vert(v, r1 * c[0], r1 * s[0], width * 0.5, normal);
      v = vert(v, r1 * c[0], r1 * s[0], -width * 0.5, normal);
      v = vert(v, r2 * c[1], r2 * s[1], width * 0.5, normal);
      v = vert(v, r2 * c[1], r2 * s[1], -width * 0.5, normal);
      v = vert(v, r2 * c[2], r2 * s[2], width * 0.5, normal);
      v = vert(v, r2 * c[2], r2 * s[2], -width * 0.5, normal);
      v = vert(v, r1 * c[3], r1 * s[3], width * 0.5, normal);
      v = vert(v, r1 * c[3], r1 * s[3], -width * 0.5, normal);
      v = vert(v, r1 * c[4], r1 * s[4], width * 0.5, normal);
      v = vert(v, r1 * c[4], r1 * s[4], -width * 0.5, normal);

      v = vert(v, r1 * c[4], r1 * s[4], -width * 0.5, normal);
   }

   gear->count = (v - gear->vertices) / 6;

   glGenBuffers(1, &gear->vbo);
   glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
   glBufferData(GL_ARRAY_BUFFER, gear->count * 6 * 4,
		gear->vertices, GL_STATIC_DRAW);

   return gear;
}

static void
multiply(GLfloat *m, const GLfloat *n)
{
   GLfloat tmp[16];
   const GLfloat *row, *column;
   div_t d;
   int i, j;

   for (i = 0; i < 16; i++) {
      tmp[i] = 0;
      d = div(i, 4);
      row = n + d.quot * 4;
      column = m + d.rem;
      for (j = 0; j < 4; j++)
	 tmp[i] += row[j] * column[j * 4];
   }
   memcpy(m, &tmp, sizeof tmp);
}

static void
rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   double s, c;

   sincos(angle, &s, &c);
   GLfloat r[16] = {
      x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
      x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0, 
      x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
      0, 0, 0, 1
   };

   multiply(m, r);
}

static void
translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat t[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 };

   multiply(m, t);
}

static const GLfloat light[3] = { 1.0, 1.0, -1.0 };
	
static void
draw_gear(struct gear *gear, GLfloat *m,
	  GLfloat x, GLfloat y, GLfloat angle, const GLfloat *color)
{
   GLfloat tmp[16];

   memcpy(tmp, m, sizeof tmp);
   translate(tmp, x, y, 0);
   rotate(tmp, 2 * M_PI * angle / 360.0, 0, 0, 1);
   glUniformMatrix4fv(proj_location, 1, GL_FALSE, tmp);
   glUniform3fv(light_location, 1, light);
   glUniform4fv(color_location, 1, color);

   glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			 6 * sizeof(GLfloat), NULL);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			 6 * sizeof(GLfloat), (GLfloat *) 0 + 3);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, gear->count);
}

static void
gears_draw(void)
{
   const static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   const static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   const static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };
   GLfloat m[16];

   glClearColor(0.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   memcpy(m, proj, sizeof m);
   rotate(m, 2 * M_PI * view_rotx / 360.0, 1, 0, 0);
   rotate(m, 2 * M_PI * view_roty / 360.0, 0, 1, 0);
   rotate(m, 2 * M_PI * view_rotz / 360.0, 0, 0, 1);

   draw_gear(gear1, m, -3.0, -2.0, angle, red);
   draw_gear(gear2, m, 3.1, -2.0, -2 * angle - 9.0, green);
   draw_gear(gear3, m, -3.1, 4.2, -2 * angle - 25.0, blue);
}

/* new window size or exposure */
static void
gears_reshape(int width, int height)
{
   GLfloat ar, m[16] = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 0.1, 0.0,
      0.0, 0.0, 0.0, 1.0,
   };
      
   if (width < height)
      ar = width;
   else
      ar = height;

   m[0] = 0.1 * ar / width;
   m[5] = 0.1 * ar / height;
   memcpy(proj, m, sizeof proj);
   glViewport(0, 0, (GLint) width, (GLint) height);
}

static void
gears_special(int special)
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
   }
}

static void
gears_idle(void)
{
   static double tRot0 = -1.0;
   double dt, t = eglutGet(EGLUT_ELAPSED_TIME) / 1000.0;

   if (tRot0 < 0.0)
      tRot0 = t;
   dt = t - tRot0;
   tRot0 = t;

   /* advance rotation for next frame */
   angle += 70.0 * dt;  /* 70 degrees per second */
   if (angle > 3600.0)
      angle -= 3600.0;

  eglutPostRedisplay();
}

static const char vertex_shader[] =
   "uniform mat4 proj;\n"
   "attribute vec4 position;\n"
   "attribute vec4 normal;\n"
   "varying vec3 rotated_normal;\n"
   "varying vec3 rotated_position;\n"
   "vec4 tmp;\n"
   "void main()\n"
   "{\n"
   "   gl_Position = proj * position;\n"
   "   rotated_position = gl_Position.xyz;\n"
   "   tmp = proj * normal;\n"
   "   rotated_normal = tmp.xyz;\n"
   "}\n";

 static const char fragment_shader[] =
   //"precision mediump float;\n"
   "uniform vec4 color;\n"
   "uniform vec3 light;\n"
   "varying vec3 rotated_normal;\n"
   "varying vec3 rotated_position;\n"
   "vec3 light_direction;\n"
   "vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n"
   "void main()\n"
   "{\n"
   "   light_direction = normalize(light - rotated_position);\n"
   "   gl_FragColor = color + white * dot(light_direction, rotated_normal);\n"
   "}\n";

static void
gears_init(void)
{
   GLuint v, f, program;
   const char *p;
   char msg[512];

   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);

   p = vertex_shader;
   v = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(v, 1, &p, NULL);
   glCompileShader(v);
   glGetShaderInfoLog(v, sizeof msg, NULL, msg);
   printf("vertex shader info: %s\n", msg);

   p = fragment_shader;
   f = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(f, 1, &p, NULL);
   glCompileShader(f);
   glGetShaderInfoLog(f, sizeof msg, NULL, msg);
   printf("fragment shader info: %s\n", msg);

   program = glCreateProgram();
   glAttachShader(program, v);
   glAttachShader(program, f);
   glBindAttribLocation(program, 0, "position");
   glBindAttribLocation(program, 1, "normal");

   glLinkProgram(program);
   glGetProgramInfoLog(program, sizeof msg, NULL, msg);
   printf("info: %s\n", msg);

   glUseProgram(program);
   proj_location = glGetUniformLocation(program, "proj");
   light_location = glGetUniformLocation(program, "light");
   color_location = glGetUniformLocation(program, "color");

   /* make the gears */
   gear1 = gear(1.0, 4.0, 1.0, 20, 0.7);
   gear2 = gear(0.5, 2.0, 2.0, 10, 0.7);
   gear3 = gear(1.3, 2.0, 0.5, 10, 0.7);
}

int
main(int argc, char *argv[])
{
   eglutInitWindowSize(300, 300);
   eglutInitAPIMask(EGLUT_OPENGL_ES2_BIT);
   eglutInit(argc, argv);

   eglutCreateWindow("es2gears");

   eglutIdleFunc(gears_idle);
   eglutReshapeFunc(gears_reshape);
   eglutDisplayFunc(gears_draw);
   eglutSpecialFunc(gears_special);

   gears_init();

   eglutMainLoop();

   return 0;
}
