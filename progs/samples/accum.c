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


GLenum doubleBuffer;
GLint thing1, thing2;


static void Init(void)
{

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearAccum(0.0, 0.0, 0.0, 0.0);

    thing1 = glGenLists(1);
    glNewList(thing1, GL_COMPILE);
	glColor3f(1.0, 0.0, 0.0);
	glRectf(-1.0, -1.0, 1.0, 0.0);
    glEndList();

    thing2 = glGenLists(1);
    glNewList(thing2, GL_COMPILE);
	glColor3f(0.0, 1.0, 0.0);
	glRectf(0.0, -1.0, 1.0, 1.0);
    glEndList();
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void Key(unsigned char key, int x, int y)
{
    (void) x;
    (void) y;
    switch (key) {
      case 27:
	exit(1);
      case '1':
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;
      case '2':
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void Draw(void)
{

    glPushMatrix();

    glScalef(0.8, 0.8, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glCallList(thing1);
    glAccum(GL_LOAD, 0.5);

    glClear(GL_COLOR_BUFFER_BIT);
    glCallList(thing2);
    glAccum(GL_ACCUM, 0.5);

    glAccum(GL_RETURN, 1.0);

    glPopMatrix();

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
	    printf("%s (Bad option).\n", argv[i]);
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

    glutInitWindowPosition(0, 0);
    glutInitWindowSize( 300, 300);

    type = GLUT_RGB | GLUT_ACCUM;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Accum Test") == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
