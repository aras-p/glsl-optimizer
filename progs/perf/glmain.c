/*
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * OpenGL/GLUT common code for perf programs.
 * Brian Paul
 * 15 Sep 2009
 */


#include <stdio.h>
#include "glmain.h"
#include <GL/glut.h>


static int Win;
static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;


/** Return time in seconds */
double
PerfGetTime(void)
{
   return glutGet(GLUT_ELAPSED_TIME) * 0.001;
}


void
PerfSwapBuffers(void)
{
   glutSwapBuffers();
}


/** make simple checkerboard texture object */
GLuint
PerfCheckerTexture(GLsizei width, GLsizei height)
{
   const GLenum filter = GL_NEAREST;
   GLubyte *img = (GLubyte *) malloc(width * height * 4);
   GLint i, j, k;
   GLuint obj;

   k = 0;
   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         GLubyte color;
         if (((i / 8) ^ (j / 8)) & 1) {
            color = 0xff;
         }
         else {
            color = 0x0;
         }
         img[k++] = color;
         img[k++] = color;
         img[k++] = color;
         img[k++] = color;
      }
   }

   glGenTextures(1, &obj);
   glBindTexture(GL_TEXTURE_2D, obj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, img);
   free(img);

   return obj;
}


static GLuint
CompileShader(GLenum type, const char *shader)
{
   GLuint sh;
   GLint stat;

   sh = glCreateShader(type);
   glShaderSource(sh, 1, (const GLchar **) &shader, NULL);

   glCompileShader(sh);
      
   glGetShaderiv(sh, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog(sh, 1000, &len, log);
      fprintf(stderr, "Error: problem compiling shader: %s\n", log);
      exit(1);
   }

   return sh;
}


/** Make shader program from given vert/frag shader text */
GLuint
PerfShaderProgram(const char *vertShader, const char *fragShader)
{
   GLuint prog;
   GLint stat;

   {
      const char *version = (const char *) glGetString(GL_VERSION);
      if ((version[0] != '2' && 
           version[0] != '3') || version[1] != '.') {
         fprintf(stderr, "Error: GL version 2.x or better required\n");
         exit(1);
      }
   }

   prog = glCreateProgram();

   if (vertShader) {
      GLuint vs = CompileShader(GL_VERTEX_SHADER, vertShader);
      glAttachShader(prog, vs);
   }
   if (fragShader) {
      GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragShader);
      glAttachShader(prog, fs);
   }

   glLinkProgram(prog);
   glGetProgramiv(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog(prog, 1000, &len, log);
      fprintf(stderr, "Shader link error:\n%s\n", log);
      exit(1);
   }

   return prog;
}


int
PerfReshapeWindow( unsigned w, unsigned h )
{
   if (glutGet(GLUT_SCREEN_WIDTH) < w ||
       glutGet(GLUT_SCREEN_HEIGHT) < h)
      return 0;

   glutReshapeWindow( w, h );
   glutPostRedisplay();
   return 1;
}


GLboolean
PerfExtensionSupported(const char *ext)
{
   return glutExtensionSupported(ext);
}


static void
Idle(void)
{
   PerfNextRound();
}


static void
Draw(void)
{
   PerfDraw();
   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
   case 'z':
      Zrot -= step;
      break;
   case 'Z':
      Zrot += step;
      break;
   case 27:
      glutDestroyWindow(Win);
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      Xrot -= step;
      break;
   case GLUT_KEY_DOWN:
      Xrot += step;
      break;
   case GLUT_KEY_LEFT:
      Yrot -= step;
      break;
   case GLUT_KEY_RIGHT:
      Yrot += step;
      break;
   }
   glutPostRedisplay();
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   glutIdleFunc(Idle);
   PerfInit();
   glutMainLoop();
   return 0;
}
