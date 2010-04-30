/*
 * (C) Copyright IBM Corporation 2007
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * \file arraytexture.c
 *
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#if !defined(GL_EXT_texture_array) && !defined(GL_MESA_texture_array)
# error "This demo requires enums for either GL_EXT_texture_array or GL_MESA_texture_array to build."
#endif

#include "readtex.h"

#define GL_CHECK_ERROR() \
    do { \
       GLenum err = glGetError(); \
       if (err) { \
          printf("%s:%u: %s (0x%04x)\n", __FILE__, __LINE__, \
                    gluErrorString(err), err); \
       } \
    } while (0)

static const char *const textures[] = {
   "../images/girl.rgb",
   "../images/girl2.rgb",
   "../images/arch.rgb",
   "../images/s128.rgb",

   "../images/tree3.rgb",
   "../images/bw.rgb",
   "../images/reflect.rgb",
   "../images/wrs_logo.rgb",
   NULL
};

static const char frag_prog[] =
  "!!ARBfp1.0\n"
  "OPTION MESA_texture_array;\n"
  "TEX    result.color, fragment.texcoord[0], texture[0], ARRAY2D;\n"
  "END\n";

static GLfloat Xrot = 0, Yrot = -30, Zrot = 0;
static GLfloat texZ = 0.0;
static GLfloat texZ_dir = 0.01;
static GLint num_layers;


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void Idle(void)
{
   static int lastTime = 0;
   int t = glutGet(GLUT_ELAPSED_TIME);

   if (lastTime == 0)
      lastTime = t;
   else if (t - lastTime < 10)
      return;

   lastTime = t;

   texZ += texZ_dir;
   if ((texZ < 0.0) || ((GLint) texZ > num_layers)) {
      texZ_dir = -texZ_dir;
   }

   glutPostRedisplay();
}


static void Display(void)
{
   char str[100];

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
   glColor3f(1,1,1);
   glRasterPos3f(-0.9, -0.9, 0.0);
   sprintf(str, "Texture Z coordinate = %4.1f", texZ);
   PrintString(str);

   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 1);
   GL_CHECK_ERROR();
   glEnable(GL_TEXTURE_2D_ARRAY_EXT);
   GL_CHECK_ERROR();

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -8.0);

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   glBegin(GL_QUADS);
   glTexCoord3f(0.0, 0.0, texZ);  glVertex2f(-1.0, -1.0);
   glTexCoord3f(2.0, 0.0, texZ);  glVertex2f(1.0, -1.0);
   glTexCoord3f(2.0, 2.0, texZ);  glVertex2f(1.0,  1.0);
   glTexCoord3f(0.0, 2.0, texZ);  glVertex2f(-1.0,  1.0);
   glEnd();

   glPopMatrix();

   glDisable(GL_TEXTURE_2D_ARRAY_EXT);
   GL_CHECK_ERROR();
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
   GL_CHECK_ERROR();

   glutSwapBuffers();
}


static void Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
}


static void Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey(int key, int x, int y)
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


static int FindLine(const char *program, int position)
{
   int i, line = 1;
   for (i = 0; i < position; i++) {
      if (program[i] == '\n')
          line++;
   }
   return line;
}


static void
compile_fragment_program(GLuint id, const char *prog)
{
   int errorPos;
   int err;

   err = glGetError();
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, id);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                     strlen(prog), (const GLubyte *) prog);

   glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
   err = glGetError();
   if (err != GL_NO_ERROR || errorPos != -1) {
      int l = FindLine(prog, errorPos);

      printf("Fragment Program Error (err=%d, pos=%d line=%d): %s\n",
             err, errorPos, l,
             (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
      exit(0);
   }
}


static void require_extension(const char *ext)
{
   if (!glutExtensionSupported(ext)) {
      printf("Sorry, %s not supported by this renderer.\n", ext);
      exit(1);
   }
}


static void Init(void)
{
   const char *const ver_string = (const char *) glGetString(GL_VERSION);
   unsigned i;

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   require_extension("GL_ARB_fragment_program");
   require_extension("GL_MESA_texture_array");
   require_extension("GL_SGIS_generate_mipmap");

   for (num_layers = 0; textures[num_layers] != NULL; num_layers++)
      /* empty */ ;

   glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, 1);
   glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGB8,
                256, 256, num_layers, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
   GL_CHECK_ERROR();

   glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_GENERATE_MIPMAP_SGIS, 
                   GL_TRUE);

   for (i = 0; textures[i] != NULL; i++) {
      GLint width, height;
      GLenum format;

      GLubyte *image = LoadRGBImage(textures[i], &width, &height, &format);
      if (!image) {
         printf("Error: could not load texture image %s\n", textures[i]);
         exit(1);
      }

      /* resize to 256 x 256 */
      if (width != 256 || height != 256) {
         GLubyte *newImage = malloc(256 * 256 * 4);
         gluScaleImage(format, width, height, GL_UNSIGNED_BYTE, image,
                       256, 256, GL_UNSIGNED_BYTE, newImage);
         free(image);
         image = newImage;
      }

      glTexSubImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0,
                      0, 0, i, 256, 256, 1,
                      format, GL_UNSIGNED_BYTE, image);
      free(image);
   }
   GL_CHECK_ERROR();

   glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   GL_CHECK_ERROR();
   glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   GL_CHECK_ERROR();

   compile_fragment_program(1, frag_prog);
   GL_CHECK_ERROR();
}


int main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(350, 350);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   glutCreateWindow("Array texture test");
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Display);
   glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
