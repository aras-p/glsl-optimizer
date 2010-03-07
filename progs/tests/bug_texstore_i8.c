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
/*
 * Based on the trivial/quad-tex-2d program.
 * Modified by Jakob Bornecrantz <jakob@tungstengraphics.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

static GLenum Target = GL_TEXTURE_2D;
static GLenum Filter = GL_NEAREST;
GLenum doubleBuffer;
static float Rot = 0;
static int win = 0;

static void Init(void)
{
	int internalFormat;
	int sourceFormat;

	fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
	fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
	fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));

	glClearColor(0.0, 0.0, 1.0, 0.0);

#define SIZE 16
	{
		GLubyte tex2d[SIZE][SIZE][4];
		GLint s, t;

		for (s = 0; s < SIZE; s++) {
			for (t = 0; t < SIZE; t++) {
				tex2d[t][s][0] = 0xff;
				tex2d[t][s][1] = 0xff;
				tex2d[t][s][2] = 0xff;
				tex2d[t][s][3] = 0xff;
			}
		}

		internalFormat = GL_LUMINANCE8;
		sourceFormat = GL_RGBA;

		glTexImage2D(Target,
			0,
			internalFormat,
			SIZE, SIZE,
			0,
			sourceFormat,
			GL_UNSIGNED_BYTE,
			/* GL_UNSIGNED_INT, */
			tex2d);

		glEnable(Target);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexParameterf(Target, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glTexParameterf(Target, GL_TEXTURE_MIN_FILTER, Filter);
		glTexParameterf(Target, GL_TEXTURE_MAG_FILTER, Filter);
	}
}

static void Reshape(int width, int height)
{
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#if 0
	glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
#else
	glFrustum(-1, 1, -1, 1, 10, 20);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -15);
}

static void Key(unsigned char key, int x, int y)
{
	switch (key) {
	case 'r':
		Rot += 10.0;
		break;
	case 'R':
		Rot -= 10.0;
		break;
	case 27:
		glutDestroyWindow(win);
		exit(0);
	default:
		return;
	}
	glutPostRedisplay();
}

static void Draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();
	glRotatef(Rot, 0, 1, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(1,0);
	glVertex3f( 0.9, -0.9, 0.0);
	glTexCoord2f(1,1);
	glVertex3f( 0.9,  0.9, 0.0);
	glTexCoord2f(0,1);
	glVertex3f(-0.9,  0.9, 0.0);
	glTexCoord2f(0,0);
	glVertex3f(-0.9,  -0.9, 0.0);
	glEnd();

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

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(250, 250);

	type = GLUT_RGB;
	type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
	glutInitDisplayMode(type);

	win = glutCreateWindow("Tex test");
        glewInit();
	if (!win) {
		exit(1);
	}

	Init();

	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Key);
	glutDisplayFunc(Draw);
	glutMainLoop();
	return 0;
}
