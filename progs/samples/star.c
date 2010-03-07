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
#include <math.h>
#include <GL/glut.h>


#ifndef PI
#define PI 3.141592657
#endif

enum {
    NORMAL = 0,
    WEIRD = 1
};

enum {
    STREAK = 0,
    CIRCLE = 1
};

#define MAXSTARS 400
#define MAXPOS 10000
#define MAXWARP 500
#define MAXANGLES 6000


typedef struct _starRec {
    GLint type;
    float x[2], y[2], z[2];
    float offsetX, offsetY, offsetR, rotation;
} starRec;


GLenum doubleBuffer;
GLint windW, windH;

GLenum flag = NORMAL;
GLint starCount = MAXSTARS / 2;
float speed = 1.0;
GLint nitro = 0;
starRec stars[MAXSTARS];
float sinTable[MAXANGLES];


static float Sin(float angle)
{

    return (sinTable[(GLint)angle]);
}

static float Cos(float angle)
{

    return (sinTable[((GLint)angle+(MAXANGLES/4))%MAXANGLES]);
}

static void NewStar(GLint n, GLint d)
{

    if (rand()%4 == 0) {
	stars[n].type = CIRCLE;
    } else {
	stars[n].type = STREAK;
    }
    stars[n].x[0] = (float)(rand() % MAXPOS - MAXPOS / 2);
    stars[n].y[0] = (float)(rand() % MAXPOS - MAXPOS / 2);
    stars[n].z[0] = (float)(rand() % MAXPOS + d);
    if (rand()%4 == 0 && flag == WEIRD) {
	stars[n].offsetX = (float)(rand() % 100 - 100 / 2);
	stars[n].offsetY = (float)(rand() % 100 - 100 / 2);
	stars[n].offsetR = (float)(rand() % 25 - 25 / 2);
    } else {
	stars[n].offsetX = 0.0;
	stars[n].offsetY = 0.0;
	stars[n].offsetR = 0.0;
    }
}

static void RotatePoint(float *x, float *y, float rotation)
{
    float tmpX, tmpY;

    tmpX = *x * Cos(rotation) - *y * Sin(rotation);
    tmpY = *y * Cos(rotation) + *x * Sin(rotation);
    *x = tmpX;
    *y = tmpY;
}

static void MoveStars(void)
{
    float offset;
    GLint n;
    static double t0 = -1.;
    double t, dt;
    t = glutGet(GLUT_ELAPSED_TIME) / 1000.;
    if (t0 < 0.)
       t0 = t;
    dt = 85.*(t - t0);
    t0 = t;

    offset = speed * 60.0;

    for (n = 0; n < starCount; n++) {
	stars[n].x[1] = stars[n].x[0];
	stars[n].y[1] = stars[n].y[0];
	stars[n].z[1] = stars[n].z[0];
	stars[n].x[0] += stars[n].offsetX*dt;
	stars[n].y[0] += stars[n].offsetY*dt;
	stars[n].z[0] -= offset*dt;
        stars[n].rotation += stars[n].offsetR*dt;
        if (stars[n].rotation > MAXANGLES) {
            stars[n].rotation = 0.0;
	}
        else if (stars[n].rotation < 0.0) {
           stars[n].rotation += 360.0;
        }
    }
}

static GLenum StarPoint(GLint n)
{
    float x0, y0, x1, y1, width;
    GLint i;

    x0 = stars[n].x[0] * windW / stars[n].z[0];
    y0 = stars[n].y[0] * windH / stars[n].z[0];
    RotatePoint(&x0, &y0, stars[n].rotation);
    x0 += windW / 2.0;
    y0 += windH / 2.0;

    if (x0 >= 0.0 && x0 < windW && y0 >= 0.0 && y0 < windH) {
	if (stars[n].type == STREAK) {
	    x1 = stars[n].x[1] * windW / stars[n].z[1];
	    y1 = stars[n].y[1] * windH / stars[n].z[1];
	    RotatePoint(&x1, &y1, stars[n].rotation);
	    x1 += windW / 2.0;
	    y1 += windH / 2.0;

	    glLineWidth(MAXPOS/100.0/stars[n].z[0]+1.0);
	    glColor3f(1.0, (MAXWARP-speed)/MAXWARP, (MAXWARP-speed)/MAXWARP);
	    if (fabs(x0-x1) < 1.0 && fabs(y0-y1) < 1.0) {
		glBegin(GL_POINTS);
		    glVertex2f(x0, y0);
		glEnd();
	    } else {
		glBegin(GL_LINES);
		    glVertex2f(x0, y0);
		    glVertex2f(x1, y1);
		glEnd();
	    }
	} else {
	    width = MAXPOS / 10.0 / stars[n].z[0] + 1.0;
	    glColor3f(1.0, 0.0, 0.0);
	    glBegin(GL_POLYGON);
		for (i = 0; i < 8; i++) {
		    float x = x0 + width * Cos((float)i*MAXANGLES/8.0);
		    float y = y0 + width * Sin((float)i*MAXANGLES/8.0);
		    glVertex2f(x, y);
		};
	    glEnd();
	}
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

static void ShowStars(void)
{
    GLint n;

    glClear(GL_COLOR_BUFFER_BIT);

    for (n = 0; n < starCount; n++) {
	if (stars[n].z[0] > speed || (stars[n].z[0] > 0.0 && speed < MAXWARP)) {
	    if (StarPoint(n) == GL_FALSE) {
		NewStar(n, MAXPOS);
	    }
	} else {
	    NewStar(n, MAXPOS);
	}
    }
}

static void Init(void)
{
    float angle;
    GLint n;

    srand((unsigned int) glutGet(GLUT_ELAPSED_TIME) );

    for (n = 0; n < MAXSTARS; n++) {
	NewStar(n, 100);
    }

    angle = 0.0;
    for (n = 0; n < MAXANGLES ; n++) {
	sinTable[n] = sin(angle);
        angle += PI / (MAXANGLES / 2.0);
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glDisable(GL_DITHER);
}

static void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;

    glViewport(0, 0, windW, windH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.5, windW+0.5, -0.5, windH+0.5);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
      case 32:
	flag = (flag == NORMAL) ? WEIRD : NORMAL;
	break;
      case 't':
	nitro = 1;
	break;
      default:
	return;
    }
}

static void Draw(void)
{

    MoveStars();
    ShowStars();
    if (nitro > 0) {
	speed = (float)(nitro / 10) + 1.0;
	if (speed > MAXWARP) {
	    speed = MAXWARP;
	}
	if (++nitro > MAXWARP*10) {
	    nitro = -nitro;
	}
    } else if (nitro < 0) {
	nitro++;
	speed = (float)(-nitro / 10) + 1.0;
	if (speed > MAXWARP) {
	    speed = MAXWARP;
	}
    }

    glFlush();
    if (doubleBuffer) {
	glutSwapBuffers();
    }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_TRUE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	}
    }
    return GL_TRUE;
}

#if !defined(GLUTCALLBACK)
#define GLUTCALLBACK
#endif

static void GLUTCALLBACK glut_post_redisplay_p(void)
{
      glutPostRedisplay();
}

int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    windW = 300;
    windH = 300;
    glutInitWindowPosition(0, 0); glutInitWindowSize( 300, 300);

    type = GLUT_RGB;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Stars") == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutIdleFunc(glut_post_redisplay_p);
    glutMainLoop();
	return 0;
}
