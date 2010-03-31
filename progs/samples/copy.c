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
#include <GL/glew.h>
#include <GL/glut.h>


#include "loadppm.c"

GLenum doubleBuffer;
GLint windW, windH;

char *fileName = 0;
PPMImage *image;
float zoom;
GLint x, y;

static void Init(void)
{

    glClearColor(0.0, 0.0, 0.0, 0.0);

    x = 0;
    y = windH;
    zoom = 1.8;
}

static void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;

    glViewport(0, 0, windW, windH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windW, 0, windH);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
        exit(1);
      case 'Z':
	zoom += 0.2;
	break;
      case 'z':
	zoom -= 0.2;
	if (zoom < 0.2) {
	    zoom = 0.2;
	}
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void Mouse(int button, int state, int mouseX, int mouseY)
{
    if (state != GLUT_DOWN)
	return;
    x = (GLint)mouseX;
    y = (GLint)mouseY;

    glutPostRedisplay();
}

static void Draw(void)
{
    GLint src[3], dst[3];

    glClear(GL_COLOR_BUFFER_BIT);

    src[0] = (int) ((windW / 2.0) - (image->sizeX / 2.0));
    src[1] = (int) ((windH / 2.0) - (image->sizeY / 2.0));
    src[2] = 0;
    glWindowPos3ivARB(src);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelZoom(1.0, 1.0);
    glDrawPixels(image->sizeX, image->sizeY, GL_RGB, GL_UNSIGNED_BYTE,
		 image->data);

    dst[0] = x;
    dst[1] = windH - y;
    dst[2] = 0;
    glWindowPos3ivARB(dst);

    glPixelZoom(zoom, zoom);
    glCopyPixels(src[0], src[1],
		 image->sizeX, image->sizeY, GL_COLOR);

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
	} else if (strcmp(argv[i], "-f") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-f (No file name).\n");
		return GL_FALSE;
	    } else {
		fileName = argv[++i];
	    }
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

    if (fileName == 0) {
	printf("No image file.\n");
	exit(1);
    }

    image = LoadPPM(fileName);

    windW = 2*300;
    windH = 2*300;
    glutInitWindowPosition(0, 0); glutInitWindowSize( windW, windH);

    type = GLUT_RGB;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Copy Test") == GL_FALSE) {
	exit(1);
    }

    glewInit();
    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutMouseFunc(Mouse);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
