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


#define OPENGL_WIDTH 24
#define OPENGL_HEIGHT 13


char string[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";
GLenum rgb, doubleBuffer, windType;
float angleX = 0.0, angleY = 0.0, angleZ = 0.0;
float scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0;
float shiftX = 0.0, shiftY = 0.0, shiftZ = 0.0;


#include "tkmap.c"


static void DrawBitmapString(void *font, const char *string)
{
    int i;

    for (i = 0; string[i]; i++)
	glutBitmapCharacter(font, string[i]);
}

static void DrawStrokeString(void *font, const char *string)
{
    int i;

    for (i = 0; string[i]; i++)
	glutStrokeCharacter(font, string[i]);
}

static void Init(void)
{

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(0.0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-400.0, 400.0, -200.0, 200.0, -400.0, 400.0);
    glMatrixMode(GL_MODELVIEW);
}

static void Key2(int key, int x, int y)
{

    switch (key) {
      case GLUT_KEY_LEFT:
	shiftX -= 20.0;
	break;
      case GLUT_KEY_RIGHT:
	shiftX += 20.0;
	break;
      case GLUT_KEY_UP:
	shiftY += 20.0;
	break;
      case GLUT_KEY_DOWN:
	shiftY -= 20.0;
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
        exit(1);

      case 'n':
	shiftZ += 20.0;
	break;
      case 'm':
	shiftZ -= 20.0;
	break;

      case 'q':
	scaleX -= 0.1;
	if (scaleX < 0.1) {
	    scaleX = 0.1;
	}
	break;
      case 'w':
	scaleX += 0.1;
	break;
      case 'a':
	scaleY -= 0.1;
	if (scaleY < 0.1) {
	    scaleY = 0.1;
	}
	break;
      case 's':
	scaleY += 0.1;
	break;
      case 'z':
	scaleZ -= 0.1;
	if (scaleZ < 0.1) {
	    scaleZ = 0.1;
	}
	break;
      case 'x':
	scaleZ += 0.1;
	break;

      case 'e':
	angleX -= 5.0;
	if (angleX < 0.0) {
	    angleX = 360.0 + angleX;
	}
	break;
      case 'r':
	angleX += 5.0;
	if (angleX > 360.0) {
	    angleX = angleX - 360.0;
	}
	break;
      case 'd':
	angleY -= 5.0;
	if (angleY < 0.0) {
	    angleY = 360.0 + angleY;
	}
	break;
      case 'f':
	angleY += 5.0;
	if (angleY > 360.0) {
	    angleY = angleY - 360.0;
	}
	break;
      case 'c':
	angleZ -= 5.0;
	if (angleZ < 0.0) {
	    angleZ = 360.0 + angleZ;
	}
	break;
      case 'v':
	angleZ += 5.0;
	if (angleZ > 360.0) {
	    angleZ = angleZ - 360.0;
	}
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT);

    SetColor(COLOR_WHITE);

    glPushMatrix();

    glTranslatef(shiftX, shiftY, shiftZ);
    glRotatef(angleX, 1.0, 0.0, 0.0);
    glRotatef(angleY, 0.0, 1.0, 0.0);
    glRotatef(angleZ, 0.0, 0.0, 1.0);
    glScalef(scaleX, scaleY, scaleZ);

    glPushMatrix();
    glRasterPos2f(-390.5, 0.5);
    DrawBitmapString(GLUT_BITMAP_9_BY_15, string);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-390.5, -30.5, 0.0);
    DrawStrokeString(GLUT_STROKE_ROMAN, string);
    glPopMatrix();

    glPopMatrix();

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

    glutInitWindowPosition(0, 0); glutInitWindowSize( 800, 400);

    windType = (rgb) ? GLUT_RGB : GLUT_INDEX;
    windType |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(windType);

    if (glutCreateWindow("Font Test") == GL_FALSE) {
	exit(1);
    }

    InitMap();

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutSpecialFunc(Key2);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
