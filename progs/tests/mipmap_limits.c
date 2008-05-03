/* Test GL_TEXTURE_BASE_LEVEL and GL_TEXTURE_MAX_LEVEL
 * Brian Paul
 * 10 May 2006
 */


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
/*  mipmap.c
 *  This program demonstrates using mipmaps for texture maps.
 *  To overtly show the effect of mipmaps, each mipmap reduction
 *  level has a solidly colored, contrasting texture image.
 *  Thus, the quadrilateral which is drawn is drawn with several
 *  different colors.
 */
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

static GLint BaseLevel = 0, MaxLevel = 8;
static GLfloat MinLod = -1, MaxLod = 9;
static GLfloat LodBias = 0.0;
static GLboolean NearestFilter = GL_TRUE;


static void
InitValues(void)
{
   BaseLevel = 0;
   MaxLevel = 8;
   MinLod = -1;
   MaxLod = 9;
   LodBias = 0.0;
   NearestFilter = GL_TRUE;
}


static void MakeImage(int level, int width, int height, const GLubyte color[4])
{
   const int makeStripes = 0;
   GLubyte img[256*256*3];
   int i, j;
   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         int k = (i * width + j) * 3;
         int p = (i/8) & makeStripes;
         if (p == 0) {
            img[k + 0] = color[0];
            img[k + 1] = color[1];
            img[k + 2] = color[2];
         }
         else {
            img[k + 0] = 0;
            img[k + 1] = 0;
            img[k + 2] = 0;
         }
      }
   }

   glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, width, height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, img);
}


static void makeImages(void)
{
   static const GLubyte colors[8][3] = {
      {128, 128, 128 },
      { 0, 255, 255 },
      { 255, 255, 0 },
      { 255, 0, 255 },
      { 255, 0, 0 },
      { 0, 255, 0 },
      { 0, 0, 255 },
      { 255, 255, 255 }
   };
   int i, sz = 128;

   for (i = 0; i < 8; i++) {
      MakeImage(i, sz, sz, colors[i]);
      sz /= 2;
   }
}

static void myinit(void)
{
    InitValues();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_FLAT);

    glTranslatef(0.0, 0.0, -3.6);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    makeImages();
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glEnable(GL_TEXTURE_2D);
}

static void display(void)
{
   GLfloat tcm = 4.0;
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
    glTexCoord2f(tcm, tcm); glVertex3f(3000.0, 1.0, -6000.0);
    glTexCoord2f(tcm, 0.0); glVertex3f(3000.0, -1.0, -6000.0);
    glEnd();
    glFlush();
}

static void myReshape(int w, int h)
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
     InitValues();
     break;
  case 27:  /* Escape */
    exit(0);
    break;
  default:
    return;
  }
  glutPostRedisplay();
}


static void usage(void)
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


int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (600, 600);
    glutCreateWindow (argv[0]);
    myinit();
    glutReshapeFunc (myReshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    usage();
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
