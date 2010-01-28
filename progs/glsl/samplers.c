/**
 * Exercise all available GLSL texture samplers.
 *
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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

/**
 * We generate a fragment shader which uses the maximum number of supported
 * texture samplers.
 * For each sampler we create a separate texture.  Each texture has a
 * single strip of color at a different intensity.  The fragment shader
 * samples all the textures at the same coordinate and sums the values.
 * The result should be a quad with rows of colors of increasing intensity
 * from bottom to top.
 *
 * Brian Paul
 * 1 Jan 2009
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include "GL/glut.h"
#include "shaderutil.h"


#define MAX_SAMPLERS 128


static const char *Demo = "samplers";

static GLuint Program;
static GLint NumSamplers;
static GLuint Textures[MAX_SAMPLERS];
static GLfloat Xrot = 0.0, Yrot = .0, Zrot = 0.0;
static GLfloat EyeDist = 10;
static GLboolean Anim = GL_FALSE;


static void
DrawPolygon(GLfloat size)
{
   glPushMatrix();
   glNormal3f(0, 0, 1);
   glBegin(GL_POLYGON);

   glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
   glVertex2f(-size, -size);

   glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
   glVertex2f( size, -size);

   glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
   glVertex2f( size,  size);

   glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
   glVertex2f(-size,  size);

   glEnd();
   glPopMatrix();
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
      glTranslatef(0.0, 0.0, -EyeDist);
      glRotatef(Zrot, 0, 0, 1);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Xrot, 1, 0, 0);

      DrawPolygon(3.0);

   glPopMatrix();

   glutSwapBuffers();
}


static void
idle(void)
{
   GLfloat t = 0.05 * glutGet(GLUT_ELAPSED_TIME);
   Yrot = t;
   glutPostRedisplay();
}


static void
key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
   case ' ':
   case 'a':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'z':
      EyeDist -= 0.5;
      if (EyeDist < 3.0)
         EyeDist = 3.0;
      break;
   case 'Z':
      EyeDist += 0.5;
      if (EyeDist > 90.0)
         EyeDist = 90;
      break;
   case 27:
      exit(0);
   }
   glutPostRedisplay();
}


static void
specialkey(int key, int x, int y)
{
   GLfloat step = 2.0;
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      Xrot += step;
      break;
   case GLUT_KEY_DOWN:
      Xrot -= step;
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


/* new window size or exposure */
static void
Reshape(int width, int height)
{
   GLfloat ar = (float) width / (float) height;
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-2.0*ar, 2.0*ar, -2.0, 2.0, 4.0, 100.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
InitTextures(void)
{
   const GLint size = MAX_SAMPLERS;
   GLubyte *texImage;
   GLenum filter = GL_NEAREST;
   GLint stripeSize;
   GLint s;

   texImage = (GLubyte *) malloc(size * size * 4);

   glGenTextures(NumSamplers, Textures);

   /* size of texels stripe */
   stripeSize = size / NumSamplers;

   /* create a texture for each sampler */
   for (s = 0; s < NumSamplers; s++) {
      GLint x, y, ypos;
      GLubyte intensity = 31 + s * (256-32) / (NumSamplers - 1);

      printf("Texture %d: color = %d, %d, %d\n", s,
             (int) intensity, 0, (int) intensity );

      /* initialize the texture to black */
      memset(texImage, 0, size * size * 4);

      /* set a stripe of texels to the intensity value */
      ypos = s * stripeSize;
      for (y = 0; y < stripeSize; y++) {
         for (x = 0; x < size; x++) {
            GLint k = 4 * ((ypos + y) * size + x);
            if (x < size / 2) {
               texImage[k + 0] = intensity;
               texImage[k + 1] = intensity;
               texImage[k + 2] = 0;
               texImage[k + 3] = 255;
            }
            else {
               texImage[k + 0] = 255 - intensity;
               texImage[k + 1] = 0;
               texImage[k + 2] = 0;
               texImage[k + 3] = 255;
            }
         }
      }

      glActiveTexture(GL_TEXTURE0 + s);
      glBindTexture(GL_TEXTURE_2D, Textures[s]);
      gluBuild2DMipmaps(GL_TEXTURE_2D, 4, size, size,
                        GL_RGBA, GL_UNSIGNED_BYTE, texImage);
     
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
   }

   free(texImage);
}


/**
 * Generate a fragment shader that uses the given number of samplers.
 */
static char *
GenFragmentShader(GLint numSamplers)
{
   const int maxLen = 10 * 1000;
   char *prog = (char *) malloc(maxLen);
   char *p = prog;
   int s;

   p += sprintf(p, "// Generated fragment shader:\n");
#ifndef SAMPLERS_ARRAY
   for (s = 0; s < numSamplers; s++) {
      p += sprintf(p, "uniform sampler2D tex%d;\n", s);
   }
#else
   p += sprintf(p, "uniform sampler2D tex[%d];\n", numSamplers);
#endif
   p += sprintf(p, "void main()\n");
   p += sprintf(p, "{\n");
   p += sprintf(p, "   vec4 color = vec4(0.0);\n");
   for (s = 0; s < numSamplers; s++) {
#ifndef SAMPLERS_ARRAY
      p += sprintf(p, "   color += texture2D(tex%d, gl_TexCoord[0].xy);\n", s);
#else
      p += sprintf(p, "   color += texture2D(tex[%d], gl_TexCoord[0].xy);\n", s);
#endif
   }
   p += sprintf(p, "   gl_FragColor = color;\n");
   p += sprintf(p, "}\n");

   assert(p - prog < maxLen);
   return prog;
}


/** Create & bind shader program */
static GLuint
CreateProgram(void)
{
   GLuint fragShader, vertShader, program;
   const char *vertShaderText = 
      "void main() \n"
      "{ \n"
      "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
      "   gl_Position = ftransform(); \n"
      "} \n";
   char *fragShaderText = GenFragmentShader(NumSamplers);

   printf("%s", fragShaderText);

   vertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   assert(vertShader);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   free(fragShaderText);

   return program;
}


static void
InitProgram(void)
{
   GLint s;

   Program = CreateProgram();

   /* init sampler uniforms */
   for (s = 0; s < NumSamplers; s++) {
      char uname[10];
      GLint loc;

#ifndef SAMPLERS_ARRAY
      sprintf(uname, "tex%d", s);
#else
      sprintf(uname, "tex[%d]", s);
#endif
      loc = glGetUniformLocation(Program, uname);
      assert(loc >= 0);

      glUniform1i(loc, s);
   }
}


static void
InitGL(void)
{
   if (!ShadersSupported()) {
      printf("GLSL not supported!\n");
      exit(1);
   }

   printf("GL_RENDERER = %s\n", (const char *) glGetString(GL_RENDERER));

   glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &NumSamplers);
   if (NumSamplers > MAX_SAMPLERS)
      NumSamplers = MAX_SAMPLERS;
   printf("Testing %d samplers\n", NumSamplers);

   InitTextures();
   InitProgram();

   glClearColor(.6, .6, .9, 0);
   glColor3f(1.0, 1.0, 1.0);

   printf("Each color corresponds to a separate sampler/texture.\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(500, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutCreateWindow(Demo);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(key);
   glutSpecialFunc(specialkey);
   glutDisplayFunc(draw);
   if (Anim)
      glutIdleFunc(idle);
   InitGL();
   glutMainLoop();
   return 0;
}
