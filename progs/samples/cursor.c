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


GLenum rgb, doubleBuffer, windType;
int windX, windY;
int cursor;


#include "tkmap.c"

static void Init(void)
{
    cursor = 0;
    glutSetCursor(cursor);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);
}

static void Reshape(int width, int height)
{

    windX = width;
    windY = height;
    glViewport(0, 0, windX, windY);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windX, 0, windY);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
        exit(1);
      case 32:
	cursor++;
	if (cursor > 19) {
	    cursor = 0;
	}
	glutSetCursor(cursor);
    }
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_POLYGON);
	SetColor(COLOR_BLACK);
	glVertex2i(0, 0);
	SetColor(COLOR_RED);
	glVertex2i(windX, 0);
	SetColor(COLOR_GREEN);
	glVertex2i(windX, windY);
	SetColor(COLOR_BLUE);
	glVertex2i(0, windY);
    glEnd();

    glFlush();

    if (doubleBuffer) {
	glutSwapBuffers();
    }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    rgb = GL_TRUE;
    doubleBuffer = GL_FALSE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-ci") == 0) {
	    rgb = GL_FALSE;
	} else if (strcmp(argv[i], "-rgb") == 0) {
	    rgb = GL_TRUE;
	} else if (strcmp(argv[i], "-sb") == 0) {
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
    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    windX = 300;
    windY = 300;
    glutInitWindowPosition(0, 0); glutInitWindowSize( windX, windY);

    windType = (rgb) ? GLUT_RGB : GLUT_INDEX;
    windType |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(windType);

    if (glutCreateWindow("Cursor Test") == GL_FALSE) {
	exit(1);
    }

    InitMap();

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
