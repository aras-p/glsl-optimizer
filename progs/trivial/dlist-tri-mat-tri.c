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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>


#define CI_OFFSET_1 16
#define CI_OFFSET_2 32


GLenum doubleBuffer;
GLint list;

static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
static GLfloat blue[4] = {0.2, 0.2, .9, 1.0};

static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

   glClear(GL_COLOR_BUFFER_BIT); 

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);

   glClearColor(0.0, 0.0, 1.0, 0.0);

   list = glGenLists(1);
   glNewList(list, GL_COMPILE); 

   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);

   glBegin(GL_TRIANGLES);
   glNormal3f(0,0,.7);
   glVertex3f( 0.9, -0.9, -30.0);
   glVertex3f( 0.9,  0.9, -30.0);
   glVertex3f(-0.9,  0.0, -30.0);
   glEnd();

   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);

   glBegin(GL_TRIANGLES);
   glVertex3f( -0.9,  0.9, -30.0);
   glVertex3f( -0.9, -0.9, -30.0);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
   glVertex3f(  0.9,  0.0, -30.0);
   glEnd();

   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
   glBegin(GL_TRIANGLES);
   glVertex3f( -0.5,  0.5, -30.0);
   glVertex3f( -0.5, -0.5, -30.0);
   glVertex3f(  0.5,  0.0, -30.0);
   glEnd();

   glEndList();
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
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




static void Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT); 

   glShadeModel( GL_SMOOTH );
   glCallList(list);

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

    if (glutCreateWindow(*argv) == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
