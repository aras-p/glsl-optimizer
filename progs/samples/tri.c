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


#define	SOLID 1
#define	LINE 2
#define	POINT 3


GLenum rgb, doubleBuffer, windType;
GLint windW, windH;

GLenum dithering = GL_TRUE;
GLenum showVerticies = GL_TRUE;
GLenum hideBottomTriangle = GL_FALSE;
GLenum outline = GL_TRUE;
GLenum culling = GL_FALSE;
GLenum winding = GL_FALSE;
GLenum face = GL_FALSE;
GLenum state = SOLID;
GLenum aaMode = GL_FALSE;
GLenum shade = GL_TRUE;

GLint color1, color2, color3;

float zRotation = 90.0;
float zoom = 1.0;

float boxA[3] = {-100, -100, 0};
float boxB[3] = { 100, -100, 0};
float boxC[3] = { 100,  100, 0};
float boxD[3] = {-100,  100, 0};

float p0[3] = {-125,-80, 0};
float p1[3] = {-125, 80, 0};
float p2[3] = { 172,  0, 0};


#include "tkmap.c"

static void Init(void)
{
    float r, g, b;
    float percent1, percent2;
    GLint i, j;

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glLineStipple(1, 0xF0F0);

    glEnable(GL_SCISSOR_TEST);

    if (!rgb) {
	for (j = 0; j <= 12; j++) {
	    if (j <= 6) {
		percent1 = j / 6.0;
		r = 1.0 - 0.8 * percent1;
		g = 0.2 + 0.8 * percent1;
		b = 0.2;
	    } else {
		percent1 = (j - 6) / 6.0;
		r = 0.2;
		g = 1.0 - 0.8 * percent1;
		b = 0.2 + 0.8 * percent1;
	    }
	    glutSetColor(j+18, r, g, b);
	    for (i = 0; i < 16; i++) {
		percent2 = i / 15.0;
		glutSetColor(j*16+1+32, r*percent2, g*percent2, b*percent2);
	    }
	}
	color1 = 18;
	color2 = 24;
	color3 = 30;
    }
}

static void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;
}

static void Key2(int key, int x, int y)
{

    switch (key) {
      case GLUT_KEY_LEFT:
	zRotation += 0.5;
	break;
      case GLUT_KEY_RIGHT:
	zRotation -= 0.5;
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
      case 'Z':
	zoom *= 0.75;
	break;
      case 'z':
	zoom /= 0.75;
	if (zoom > 10) {
	    zoom = 10;
	}
	break;
      case '1':
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	break;
      case '2':
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	break;
      case '3':
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;
      case '4':
	state = POINT;
	break;
      case '5':
	state = LINE;
	break;
      case '6':
	state = SOLID;
	break;
      case '7':
	culling = !culling;
	break;
      case '8':
	winding = !winding;
	break;
      case '9':
	face = !face;
	break;
      case 'v':
	showVerticies = !showVerticies;
	break;
      case 's':
	shade = !shade;
	(shade) ? glShadeModel(GL_SMOOTH) : glShadeModel(GL_FLAT);
	break;
      case 'h':
	hideBottomTriangle = !hideBottomTriangle;
	break;
      case 'o':
	outline = !outline;
	break;
      case 'm':
	dithering = !dithering;
	break;
      case '0':
	aaMode = !aaMode;
	if (aaMode) {
	    glEnable(GL_POLYGON_SMOOTH);
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	    if (!rgb) {
		color1 = 32;
		color2 = 128;
		color3 = 224;
	    }
	} else {
	    glDisable(GL_POLYGON_SMOOTH);
	    glDisable(GL_BLEND);
	    if (!rgb) {
		color1 = 18;
		color2 = 24;
		color3 = 30;
	    }
	}
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void BeginPrim(void)
{

    switch (state) {
      case SOLID:
	glBegin(GL_POLYGON);
	break;
      case LINE:
	glBegin(GL_LINE_LOOP);
	break;
      case POINT:
	glBegin(GL_POINTS);
	break;
      default:
        break;
    }
}

static void EndPrim(void)
{

    glEnd();
}

static void Draw(void)
{
    float scaleX, scaleY;

    glViewport(0, 0, windW, windH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-175, 175, -175, 175);
    glMatrixMode(GL_MODELVIEW);

    glScissor(0, 0, windW, windH);

    (culling) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    (winding) ? glFrontFace(GL_CCW) : glFrontFace(GL_CW);
    (face) ? glCullFace(GL_FRONT) : glCullFace(GL_BACK);

    (dithering) ? glEnable(GL_DITHER) : glDisable(GL_DITHER);

    glClear(GL_COLOR_BUFFER_BIT);

    SetColor(COLOR_GREEN);
    glBegin(GL_LINE_LOOP);
	glVertex3fv(boxA);
	glVertex3fv(boxB);
	glVertex3fv(boxC);
	glVertex3fv(boxD);
    glEnd();

    if (!hideBottomTriangle) {
	glPushMatrix();

	glScalef(zoom, zoom, zoom);
	glRotatef(zRotation, 0, 0, 1);

	SetColor(COLOR_BLUE);
	BeginPrim();
	    glVertex3fv(p0);
	    glVertex3fv(p1);
	    glVertex3fv(p2);
	EndPrim();

	if (showVerticies) {
	    (rgb) ? glColor3fv(RGBMap[COLOR_RED]) : glIndexf(color1);
	    glRectf(p0[0]-2, p0[1]-2, p0[0]+2, p0[1]+2);
	    (rgb) ? glColor3fv(RGBMap[COLOR_GREEN]) : glIndexf(color2);
	    glRectf(p1[0]-2, p1[1]-2, p1[0]+2, p1[1]+2);
	    (rgb) ? glColor3fv(RGBMap[COLOR_BLUE]) : glIndexf(color3);
	    glRectf(p2[0]-2, p2[1]-2, p2[0]+2, p2[1]+2);
	}

	glPopMatrix();
    }

    scaleX = (float)(windW - 20) / 2 / 175 * (175 - 100) + 10;
    scaleY = (float)(windH - 20) / 2 / 175 * (175 - 100) + 10;

    glViewport(scaleX, scaleY, windW-2*scaleX, windH-2*scaleY);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-100, 100, -100, 100);
    glMatrixMode(GL_MODELVIEW);

    glScissor(scaleX, scaleY, windW-2*scaleX, windH-2*scaleY);

    glPushMatrix();

    glScalef(zoom, zoom, zoom);
    glRotatef(zRotation, 0,0,1);

    glPointSize(10);
    glLineWidth(5);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_STIPPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SetColor(COLOR_RED);
    BeginPrim();
	(rgb) ? glColor3fv(RGBMap[COLOR_RED]) : glIndexf(color1);
	glVertex3fv(p0);
	(rgb) ? glColor3fv(RGBMap[COLOR_GREEN]) : glIndexf(color2);
	glVertex3fv(p1);
	(rgb) ? glColor3fv(RGBMap[COLOR_BLUE]) : glIndexf(color3);
	glVertex3fv(p2);
    EndPrim();

    glPointSize(1);
    glLineWidth(1);
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_STIPPLE);
    glBlendFunc(GL_ONE, GL_ZERO);

    if (outline) {
	SetColor(COLOR_WHITE);
	glBegin(GL_LINE_LOOP);
	    glVertex3fv(p0);
	    glVertex3fv(p1);
	    glVertex3fv(p2);
	glEnd();
    }

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

    windW = 600;
    windH = 300;
    glutInitWindowPosition(0, 0); glutInitWindowSize( windW, windH);

    windType = (rgb) ? GLUT_RGB : GLUT_INDEX;
    windType |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(windType);

    if (glutCreateWindow("Triangle Test") == GL_FALSE) {
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
