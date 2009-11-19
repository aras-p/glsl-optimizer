/*
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the name of
 * Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF
 * ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <GL/glew.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>



GLenum doubleBuffer;

static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

   glClearColor(0.0, 0.0, 1.0, 0.0);
}

static void Reshape(int width, int height)
{
    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, 0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
	exit(1);
    default:
	break;
    }
    glutPostRedisplay();
}


static float
expected(float z, float size, const float atten[3])
{
   float dist = fabs(z);
   const GLfloat q = atten[0] + dist * (atten[1] + dist * atten[2]);
   const GLfloat a = sqrt(1.0 / q);
   return size * a;
}


static void Draw(void)
{
   static GLfloat atten[3] = { 0.0, 0.1, .01 };
   float size = 40.0;
   int i;

   glClear(GL_COLOR_BUFFER_BIT); 

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glPointSize(size);
   glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, atten); 

   glColor3f(1,0,0); 

   printf("Expected point sizes:\n");
   glBegin(GL_POINTS);
   for (i = 0; i < 5; i++) {
      float x = -0.8 + i * 0.4;
      float z = -i * 20 - 10;
      glVertex3f( x, 0.0, z);
      printf(" %f\n", expected(z, size, atten));
   }
   glEnd();

   glFlush();

   if (doubleBuffer) {
      glutSwapBuffers();
   }
}


static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else {
	    fprintf(stderr, "%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}


int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB | GLUT_ALPHA;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow(argv[0]) == GL_FALSE) {
	exit(1);
    }

    glewInit();

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
    return 0;
}
