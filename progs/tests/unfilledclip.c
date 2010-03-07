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

#if 0
static void
line(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	glBegin(GL_LINES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}
#endif

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
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(1.0, 0.0, 0.0);
	/* clipped along xmin */
	glBegin(GL_TRIANGLES);
	glVertex2f(-20, win_height / 2 - 20);
	glVertex2f(20, win_height / 2);
	glVertex2f(-20, win_height / 2 + 20);
	glEnd();

	glColor3f(0.0, 1.0, 0.0);
	/* clipped along ymax */
	glBegin(GL_TRIANGLES);
	glVertex2f(win_height / 2 - 20, win_height + 20);
	glVertex2f(win_height / 2,      win_height - 20);
	glVertex2f(win_height / 2 + 20, win_height + 20);
	glEnd();

	glColor3f(0.0, 0.0, 1.0);
	/* clipped along xmax */
	glBegin(GL_TRIANGLES);
	glVertex2f(win_height + 20, win_height / 2 - 20);
	glVertex2f(win_height - 20, win_height / 2);
	glVertex2f(win_height + 20, win_height / 2 + 20);
	glEnd();

	glColor3f(1.0, 1.0, 1.0);
	/* clipped along ymin */
	glBegin(GL_TRIANGLES);
	glVertex2f(win_height / 2 - 20, -20);
	glVertex2f(win_height / 2,       20);
	glVertex2f(win_height / 2 + 20, -20);
	glEnd();

	/* clipped along near */
	glColor3f(1.0, 0.0, 1.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(win_width / 2 - 20, win_height / 2 - 20,  0.5);
	glVertex3f(win_width / 2 - 40, win_height / 2,      -0.5);
	glVertex3f(win_width / 2 - 20, win_height / 2 + 20,  0.5);
	glEnd();

	/* clipped along far */
	glColor3f(0.0, 1.0, 1.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(win_width / 2 + 20, win_height / 2 - 20, 0.5);
	glVertex3f(win_width / 2 + 40, win_height / 2,      1.5);
	glVertex3f(win_width / 2 + 20, win_height / 2 + 20, 0.5);
	glEnd();

	/* entirely clipped along near/far */
	glColor3f(.5, .5, .5);
	glBegin(GL_TRIANGLES);
	glVertex3f(win_width / 2 - 20, win_height / 2 + 20, -0.5);
	glVertex3f(win_width / 2,      win_height / 2 + 40, -0.5);
	glVertex3f(win_width / 2 + 20, win_height / 2 + 20, -0.5);
	glEnd();

	glBegin(GL_TRIANGLES);
	glVertex3f(win_width / 2 - 20, win_height / 2 - 20, 1.5);
	glVertex3f(win_width / 2,      win_height / 2 - 40, 1.5);
	glVertex3f(win_width / 2 + 20, win_height / 2 - 20, 1.5);
	glEnd();

	glColor3f(.5, .5, .5);
	line3(win_width / 2, win_height / 2 - 20, 1.5,
	      win_width / 2, win_height / 2 + 20, 1.5);

	glColor3f(1.0, 1.0, 0.0);
	/* clipped along both x and y limits */
	glBegin(GL_TRIANGLES); /* xmin, ymin */
	glVertex2f(-5, 20);
	glVertex2f(20, 20);
	glVertex2f(20, -5);
	glEnd();
	glBegin(GL_TRIANGLES); /* xmin, ymax */
	glVertex2f(-5, win_height - 20);
	glVertex2f(20, win_height - 20);
	glVertex2f(20, win_height + 5);
	glEnd();
	glBegin(GL_TRIANGLES); /* xmax, ymax */
	glVertex2f(win_width - 20, win_height + 5);
	glVertex2f(win_width - 20, win_height - 20);
	glVertex2f(win_width + 5,  win_height - 20);
	glEnd();
	glBegin(GL_TRIANGLES); /* xmax, ymin */
	glVertex2f(win_width + 5, 20);
	glVertex2f(win_width - 20, 20);
	glVertex2f(win_width - 20, -5);
	glEnd();

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
