/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int win_width, win_height;

static void
line(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	glBegin(GL_LINES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}

static void
line3(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2)
{
	glBegin(GL_LINES);
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y2, z2);
	glEnd();
}

static void
display(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(1.0, 0.0, 0.0);
	/* 2 lines clipped along xmin */
	line(-20, win_height / 2 - 20,
	      20, win_height / 2 - 20);
	line( 20, win_height / 2 + 20,
	     -20, win_height / 2 + 20);

	glColor3f(0.0, 1.0, 0.0);
	/* 2 lines clipped along ymax */
	line(win_width / 2 - 20, win_height + 20,
	     win_width / 2 - 20, win_height - 20);
	line(win_width / 2 + 20, win_height - 20,
	     win_width / 2 + 20, win_height + 20);

	glColor3f(0.0, 0.0, 1.0);
	/* 2 lines clipped along xmax */
	line(win_width - 20, win_height / 2 - 20,
	     win_width + 20, win_height / 2 - 20);
	line(win_width + 20, win_height / 2 + 20,
	     win_width - 20, win_height / 2 + 20);

	glColor3f(1.0, 1.0, 1.0);
	/* 2 lines clipped along ymin */
	line(win_width / 2 - 20,  20,
	     win_width / 2 - 20, -20);
	line(win_width / 2 + 20, -20,
	     win_width / 2 + 20,  20);

	/* 2 lines clipped along near */
	glColor3f(1.0, 0.0, 1.0);
	line3(win_width / 2 - 20 - 20, win_height / 2,       0.5,
	      win_width / 2 - 20 + 20, win_height / 2,      -0.5);
	line3(win_width / 2 - 20,      win_height / 2 - 20, -0.5,
	      win_width / 2 - 20,      win_height / 2 + 20,  0.5);

	/* 2 lines clipped along far */
	glColor3f(0.0, 1.0, 1.0);
	line3(win_width / 2 + 20 - 20, win_height / 2,      1.5,
	      win_width / 2 + 20 + 20, win_height / 2,      0.5);
	line3(win_width / 2 + 20,      win_height / 2 - 20, 0.5,
	      win_width / 2 + 20,      win_height / 2 + 20, 1.5);

	/* entirely clipped along near/far */
	glColor3f(.5, .5, .5);
	line3(win_width / 2, win_height / 2 - 20, -0.5,
	      win_width / 2, win_height / 2 + 20, -0.5);
	glColor3f(.5, .5, .5);
	line3(win_width / 2, win_height / 2 - 20, 1.5,
	      win_width / 2, win_height / 2 + 20, 1.5);

	glColor3f(1.0, 1.0, 0.0);
	/* lines clipped along both x and y limits */
	line(-5, 20,
	     20, -5); /* xmin, ymin */
	line(-5, win_height - 20,
	     20, win_height + 5); /* xmin, ymax */
	line(win_width - 20, -5,
	     win_width + 5,  20); /* xmax, ymin */
	line(win_width - 20, win_height + 5,
	     win_width + 5,  win_height - 20); /* xmax, ymax */

	glutSwapBuffers();
}

static void
reshape(int width, int height)
{
	win_width = width;
	win_height = height;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, win_width, 0, win_height, 0.0, -1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(.25, .25, 0);
}

static void key( unsigned char key, int x, int y )
{
	(void) x;
	(void) y;

	switch (key) {
	case 27: /* esc */
		exit(0);
		break;
	}

	glutPostRedisplay();
}

static void
init(void)
{
}

int
main(int argc, char *argv[])
{
	win_width = 200;
	win_height = 200;

	glutInit(&argc, argv);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(win_width, win_height);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow(argv[0]);
        glewInit();
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutDisplayFunc(display);

	init();

	glutMainLoop();
	return 0;
}
