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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>


static void Init(void)
{
   glShadeModel(GL_FLAT);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glClearStencil(0);
    glStencilMask(1);
    glEnable(GL_STENCIL_TEST);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-5.0, 5.0, -5.0, 5.0, -5.0, 5.0);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
    }
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glColor3ub(200, 0, 0);
    glBegin(GL_POLYGON);        
	glVertex3i(-4, -4, 0);
	glVertex3i( 4, -4, 0);
	glVertex3i( 0,  4, 0);
    glEnd();

    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_INCR, GL_KEEP, GL_DECR);

    glColor3ub(0, 200, 0);
    glBegin(GL_POLYGON);
	glVertex3i(3, 3, 0);
	glVertex3i(-3, 3, 0);
	glVertex3i(-3, -3, 0);
	glVertex3i(3, -3, 0);
    glEnd();

    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glColor3ub(0, 0, 200);
    glBegin(GL_POLYGON);
	glVertex3i(3, 3, 0);
	glVertex3i(-3, 3, 0);
	glVertex3i(-3, -3, 0);
	glVertex3i(3, -3, 0);
    glEnd();

    glFlush();
}

static GLenum Args(int argc, char **argv)
{
    GLint i;


    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-dr") == 0) {
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

    glutInitWindowPosition(0, 0); glutInitWindowSize( 300, 300);

    type = GLUT_RGB | GLUT_SINGLE | GLUT_STENCIL;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Stencil Test") == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
