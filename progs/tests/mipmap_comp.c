/* Copyright (c) Mark J. Kilgard, 1994. */
/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

/* mipmap_comp
 * Test compressed texture mipmaps
 *
 * Based on mipmap_limits
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.h"

#define SIZE 16 /* not larger then 16 */

static GLint BaseLevel = 0, MaxLevel = 9;
static GLfloat MinLod = -1, MaxLod = 9;
static GLfloat LodBias = 0.0;
static GLboolean NearestFilter = GL_TRUE;
static GLuint texImage;


static void
initValues(void)
{
   BaseLevel = 0;
   MaxLevel = 9;
   MinLod = -1;
   MaxLod = 2;
   LodBias = 5.0;
   NearestFilter = GL_TRUE;
}


static void
makeImage(int level, int width, int height)
{
#if 0
   GLubyte img[SIZE*SIZE*3];
   int i, j;

   (void)size;
   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         int k = (i * width + j) * 3;
         img[k + 0] = 255 * ((level + 1) % 2);
         img[k + 1] = 255 * ((level + 1) % 2);
         img[k + 2] = 255 * ((level + 1) % 2);
      }
   }

   glTexImage2D(GL_TEXTURE_2D, level, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, img);
#else
   GLubyte img[128];
   GLint size[] = {
      128, /* 16x16 */
      32,  /*  8x8  */
      8,   /*  4x4  */
      8,   /*  2x2  */
      8,   /*  1x1  */
   };
   int i;
   int value = ((level + 1) % 2) * 0xffffffff;
   memset(img, 0, 128);

   /* generate black and white mipmap levels */
   if (value)
      for (i = 0; i < size[level] / 4; i += 2)
         ((int*)img)[i] = value;

   glCompressedTexImage2D(GL_TEXTURE_2D, level,
                          GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                          width, height, 0,
                          size[level], img);
#endif
}


static void
makeImages(void)
{
   int i, sz;

   for (i = 0, sz = SIZE; sz >= 1; i++, sz /= 2) {
      makeImage(i, sz, sz);
      printf("Level %d size: %d x %d\n", i, sz, sz);
   }
}


static void
myInit(void)
{

   initValues();

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   glShadeModel(GL_FLAT);

   glTranslatef(0.0, 0.0, -3.6);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glGenTextures(1, &texImage);
   glBindTexture(GL_TEXTURE_2D, texImage);
   makeImages();

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   glEnable(GL_TEXTURE_2D);
}


static void
display(void)
{
   GLfloat tcm = 1.0;
   glBindTexture(GL_TEXTURE_2D, texImage);

   printf("BASE_LEVEL=%d  MAX_LEVEL=%d  MIN_LOD=%.2g  MAX_LOD=%.2g  Bias=%.2g  Filter=%s\n",
         BaseLevel, MaxLevel, MinLod, MaxLod, LodBias,
         NearestFilter ? "NEAREST" : "LINEAR");
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, BaseLevel);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxLevel);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, MinLod);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, MaxLod);

   if (NearestFilter) {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            GL_NEAREST_MIPMAP_NEAREST);
   }
   else {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);
   }

   glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, LodBias);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0, 0.0); glVertex3f(-2.0, -1.0, 0.0);
   glTexCoord2f(0.0, tcm); glVertex3f(-2.0, 1.0, 0.0);
   glTexCoord2f(tcm * 3000.0, tcm); glVertex3f(3000.0, 1.0, -6000.0);
   glTexCoord2f(tcm * 3000.0, 0.0); glVertex3f(3000.0, -1.0, -6000.0);
   glEnd();
   glFlush();
}


static void
myReshape(int w, int h)
{
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(60.0, 1.0*(GLfloat)w/(GLfloat)h, 1.0, 30000.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
key(unsigned char k, int x, int y)
{
  (void) x;
  (void) y;
  switch (k) {
  case 'b':
     BaseLevel--;
     if (BaseLevel < 0)
        BaseLevel = 0;
     break;
  case 'B':
     BaseLevel++;
     if (BaseLevel > 10)
        BaseLevel = 10;
     break;
  case 'm':
     MaxLevel--;
     if (MaxLevel < 0)
        MaxLevel = 0;
     break;
  case 'M':
     MaxLevel++;
     if (MaxLevel > 10)
        MaxLevel = 10;
     break;
  case 'l':
     LodBias -= 0.25;
     break;
  case 'L':
     LodBias += 0.25;
     break;
  case 'n':
     MinLod -= 0.25;
     break;
  case 'N':
     MinLod += 0.25;
     break;
  case 'x':
     MaxLod -= 0.25;
     break;
  case 'X':
     MaxLod += 0.25;
     break;
  case 'f':
     NearestFilter = !NearestFilter;
     break;
  case ' ':
     initValues();
     break;
  case 27:  /* Escape */
    exit(0);
    break;
  default:
    return;
  }
  glutPostRedisplay();
}


static void
usage(void)
{
   printf("usage:\n");
   printf("  b/B    decrease/increase GL_TEXTURE_BASE_LEVEL\n");
   printf("  m/M    decrease/increase GL_TEXTURE_MAX_LEVEL\n");
   printf("  n/N    decrease/increase GL_TEXTURE_MIN_LOD\n");
   printf("  x/X    decrease/increase GL_TEXTURE_MAX_LOD\n");
   printf("  l/L    decrease/increase GL_TEXTURE_LOD_BIAS\n");
   printf("  f      toggle nearest/linear filtering\n");
   printf("  SPACE  reset values\n");
}


int
main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (600, 600);
    glutCreateWindow (argv[0]);
    glewInit();
    myInit();
    glutReshapeFunc (myReshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    usage();
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
