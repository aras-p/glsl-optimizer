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


GLenum rgb, doubleBuffer;

GLenum antiAlias, stipple;
GLubyte stippleBits[32*4] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
};


#include "tkmap.c"

static void Init(void)
{
    GLint i;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);

    if (!rgb) {
	for (i = 0; i < 16; i++) {
	    glutSetColor(i+CI_OFFSET_1, 0.0, 0.0, i/15.0);
	    glutSetColor(i+CI_OFFSET_2, 0.0, i/15.0, 0.0);
	}
    }

    glPolygonStipple(stippleBits);

    antiAlias = GL_FALSE;
    stipple = GL_FALSE;
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
      case '1':
	antiAlias = !antiAlias;
	break;
      case '2':
	stipple = !stipple;
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void Draw(void)
{
    GLint ci1, ci2;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if (antiAlias) {
	ci1 = CI_OFFSET_1;
	ci2 = CI_OFFSET_2;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glDisable(GL_DEPTH_TEST);
    } else {
	ci1 = COLOR_BLUE;
	ci2 = COLOR_GREEN;
	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);
	glEnable(GL_DEPTH_TEST);
    }

    if (stipple) {
	glEnable(GL_POLYGON_STIPPLE);
    } else {
	glDisable(GL_POLYGON_STIPPLE);
    }

    glBegin(GL_TRIANGLES);
	(rgb) ? glColor3fv(RGBMap[COLOR_BLUE]) : glIndexi(ci1);
	glVertex3f( 0.9, -0.9, -30.0);
	glVertex3f( 0.9,  0.9, -30.0);
	glVertex3f(-0.9,  0.0, -30.0);
	(rgb) ? glColor3fv(RGBMap[COLOR_GREEN]) : glIndexi(ci2);
	glVertex3f(-0.9, -0.9, -40.0);
	glVertex3f(-0.9,  0.9, -40.0);
	glVertex3f( 0.9,  0.0, -25.0);
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
    GLenum type;

    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    glutInitWindowPosition(0, 0); glutInitWindowSize( 300, 300);

    type = GLUT_DEPTH;
    type |= (rgb) ? GLUT_RGB : GLUT_INDEX;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Depth Test") == GL_FALSE) {
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
