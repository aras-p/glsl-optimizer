/* $Id: cubemap.c,v 1.3 2000/06/27 17:04:43 brianp Exp $ */

/*
 * GL_ARB_texture_cube_map demo
 *
 * Brian Paul
 * May 2000
 *
 *
 * Copyright (C) 2000  Brian Paul   All Rights Reserved.
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
 * This is a pretty minimalistic demo for now.  Eventually, use some
 * interesting cube map textures and 3D objects.
 * For now, we use 6 checkerboard "walls" and a sphere (good for
 * verification purposes).
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/glut.h"

static GLfloat Xrot = 0, Yrot = 0;


static void draw( void )
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glutSolidSphere(2.0, 20, 20);
   glMatrixMode(GL_MODELVIEW);

   glutSwapBuffers();
}


static void idle(void)
{
   Yrot += 5.0;
   glutPostRedisplay();
}


static void set_mode(GLuint mode)
{
   if (mode == 0) {
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
      glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
      printf("GL_REFLECTION_MAP_ARB mode\n");
   }
   else if (mode == 1) {
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
      glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
      printf("GL_NORMAL_MAP_ARB mode\n");
   }
   glEnable(GL_TEXTURE_GEN_S);
   glEnable(GL_TEXTURE_GEN_T);
   glEnable(GL_TEXTURE_GEN_R);
}


static void key(unsigned char k, int x, int y)
{
   static GLboolean anim = GL_TRUE;
   static GLuint mode = 0;
   (void) x;
   (void) y;
   switch (k) {
      case ' ':
         anim = !anim;
         if (anim)
            glutIdleFunc(idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'm':
         mode = !mode;
         set_mode(mode);
         break;
      case 27:
         exit(0);
   }
   glutPostRedisplay();
}


static void specialkey(int key, int x, int y)
{
   GLfloat step = 10;
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


/* new window size or exposure */
static void reshape(int width, int height)
{
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -2.0, 2.0, -2.0, 2.0, 6.0, 20.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -8.0 );
}


static void init( void )
{
#define CUBE_TEX_SIZE 64
   GLubyte image[CUBE_TEX_SIZE][CUBE_TEX_SIZE][3];
   static const GLubyte colors[6][3] = {
      { 255,   0,   0 },
      {   0, 255, 255 },
      {   0, 255,   0 },
      { 255,   0, 255 },
      {   0,   0, 255 },
      { 255, 255,   0 }
   };
   static const GLenum targets[6] = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
   };

   GLint i, j, f;

   /* check for extension */
   {
      char *exten = (char *) glGetString(GL_EXTENSIONS);
      if (!strstr(exten, "GL_ARB_texture_cube_map")) {
         printf("Sorry, this demo requires GL_ARB_texture_cube_map\n");
         exit(0);
      }
   }


   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   /* make colored checkerboard cube faces */
   for (f = 0; f < 6; f++) {
      for (i = 0; i < CUBE_TEX_SIZE; i++) {
         for (j = 0; j < CUBE_TEX_SIZE; j++) {
            if ((i/4 + j/4) & 1) {
               image[i][j][0] = colors[f][0];
               image[i][j][1] = colors[f][1];
               image[i][j][2] = colors[f][2];
            }
            else {
               image[i][j][0] = 255;
               image[i][j][1] = 255;
               image[i][j][2] = 255;
            }
         }
      }

      glTexImage2D(targets[f], 0, GL_RGB, CUBE_TEX_SIZE, CUBE_TEX_SIZE, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, image);
   }

#if 1
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glEnable(GL_TEXTURE_CUBE_MAP_ARB);

   glClearColor(.3, .3, .3, 0);
   glColor3f( 1.0, 1.0, 1.0 );

   set_mode(0);
}


static void usage(void)
{
   printf("keys:\n");
   printf("  SPACE - toggle animation\n");
   printf("  CURSOR KEYS - rotation\n");
   printf("  m - toggle texgen reflection mode\n");
}


int main( int argc, char *argv[] )
{
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(300, 300);
   glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );
   glutCreateWindow("Texture Cube Maping");
   init();
   glutReshapeFunc( reshape );
   glutKeyboardFunc( key );
   glutSpecialFunc( specialkey );
   glutIdleFunc( idle );
   glutDisplayFunc( draw );
   usage();
   glutMainLoop();
   return 0;
}
