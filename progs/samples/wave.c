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
#define PI 3.14159265358979323846
#endif

#define GETCOORD(frame, x, y) (&(theMesh.coords[frame*theMesh.numCoords+(x)+(y)*(theMesh.widthX+1)]))
#define GETFACET(frame, x, y) (&(theMesh.facets[frame*theMesh.numFacets+(x)+(y)*theMesh.widthX]))


GLenum rgb, doubleBuffer;

#include "tkmap.c"

GLint colorIndexes1[3];
GLint colorIndexes2[3];
GLenum clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

GLenum smooth = GL_FALSE;
GLenum lighting = GL_TRUE;
GLenum depth = GL_TRUE;
GLenum stepMode = GL_FALSE;
GLenum spinMode = GL_FALSE;
GLint contouring = 0;

GLint widthX, widthY;
GLint checkerSize;
float height;

GLint frames, curFrame = 0, nextFrame = 0;

struct facet {
    float color[3];
    float normal[3];
};
struct coord {
    float vertex[3];
    float normal[3];
};
struct mesh {
    GLint widthX, widthY;
    GLint numFacets;
    GLint numCoords;
    GLint frames;
    struct coord *coords;
    struct facet *facets;
} theMesh;

GLubyte contourTexture1[] = {
    255, 255, 255, 255,
    255, 255, 255, 255,
    255, 255, 255, 255,
    127, 127, 127, 127,
};
GLubyte contourTexture2[] = {
    255, 255, 255, 255,
    255, 127, 127, 127,
    255, 127, 127, 127,
    255, 127, 127, 127,
};

void GLUTCALLBACK glut_post_redisplay_p(void)
{
      glutPostRedisplay();
}

static void Animate(void)
{
    struct coord *coord;
    struct facet *facet;
    float *lastColor;
    float *thisColor;
    GLint i, j;

    glClear(clearMask);

    if (nextFrame || !stepMode) {
	curFrame++;
    }
    if (curFrame >= theMesh.frames) {
	curFrame = 0;
    }

    if ((nextFrame || !stepMode) && spinMode) {
	glRotatef(5.0, 0.0, 0.0, 1.0);
    }
    nextFrame = 0;

    for (i = 0; i < theMesh.widthX; i++) {
	glBegin(GL_QUAD_STRIP);
	lastColor = NULL;
	for (j = 0; j < theMesh.widthY; j++) {
	    facet = GETFACET(curFrame, i, j);
	    if (!smooth && lighting) {
		glNormal3fv(facet->normal);
	    }
	    if (lighting) {
		if (rgb) {
		    thisColor = facet->color;
		    glColor3fv(facet->color);
		} else {
		    thisColor = facet->color;
		    glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, 
				 facet->color);
		}
	    } else {
		if (rgb) {
		    thisColor = facet->color;
		    glColor3fv(facet->color);
		} else {
		    thisColor = facet->color;
		    glIndexf(facet->color[1]);
		}
	    }

	    if (!lastColor || (thisColor[0] != lastColor[0] && smooth)) {
		if (lastColor) {
		    glEnd();
		    glBegin(GL_QUAD_STRIP);
		}
		coord = GETCOORD(curFrame, i, j);
		if (smooth && lighting) {
		    glNormal3fv(coord->normal);
		}
		glVertex3fv(coord->vertex);

		coord = GETCOORD(curFrame, i+1, j);
		if (smooth && lighting) {
		    glNormal3fv(coord->normal);
		}
		glVertex3fv(coord->vertex);
	    }

	    coord = GETCOORD(curFrame, i, j+1);
	    if (smooth && lighting) {
		glNormal3fv(coord->normal);
	    }
	    glVertex3fv(coord->vertex);

	    coord = GETCOORD(curFrame, i+1, j+1);
	    if (smooth && lighting) {
		glNormal3fv(coord->normal);
	    }
	    glVertex3fv(coord->vertex);

	    lastColor = thisColor;
	}
	glEnd();
    }

    glFlush();
    if (doubleBuffer) {
	glutSwapBuffers();
    }
}

static void SetColorMap(void) 
{
    static float green[3] = {0.2, 1.0, 0.2};
    static float red[3] = {1.0, 0.2, 0.2};
    float *color, percent;
    GLint *indexes, entries, i, j;

    entries = glutGet(GLUT_WINDOW_COLORMAP_SIZE);

    colorIndexes1[0] = 1;
    colorIndexes1[1] = 1 + (GLint)((entries - 1) * 0.3);
    colorIndexes1[2] = (GLint)((entries - 1) * 0.5);
    colorIndexes2[0] = 1 + (GLint)((entries - 1) * 0.5);
    colorIndexes2[1] = 1 + (GLint)((entries - 1) * 0.8);
    colorIndexes2[2] = entries - 1;

    for (i = 0; i < 2; i++) {
	switch (i) {
	  case 0:
	    color = green;
	    indexes = colorIndexes1;
	    break;
	  case 1:
	    color = red;
	    indexes = colorIndexes2;
	    break;
	}

	for (j = indexes[0]; j < indexes[1]; j++) {
	    percent = 0.2 + 0.8 * (j - indexes[0]) /
		      (float)(indexes[1] - indexes[0]);
	    glutSetColor(j, percent*color[0], percent*color[1],
			   percent*color[2]);
	}
	for (j=indexes[1]; j<=indexes[2]; j++) {
	    percent = (j - indexes[1]) / (float)(indexes[2] - indexes[1]);
	    glutSetColor(j, percent*(1-color[0])+color[0],
			   percent*(1-color[1])+color[1],
			   percent*(1-color[2])+color[2]);
	}
    }
}

static void InitMesh(void)
{
    struct coord *coord;
    struct facet *facet;
    float dp1[3], dp2[3];
    float *pt1, *pt2, *pt3;
    float angle, d, x, y;
    GLint numFacets, numCoords, frameNum, i, j;

    theMesh.widthX = widthX;
    theMesh.widthY = widthY;
    theMesh.frames = frames;

    numFacets = widthX * widthY;
    numCoords = (widthX + 1) * (widthY + 1);

    theMesh.numCoords = numCoords;
    theMesh.numFacets = numFacets;

    theMesh.coords = (struct coord *)malloc(frames*numCoords*
					    sizeof(struct coord));
    theMesh.facets = (struct facet *)malloc(frames*numFacets*
					    sizeof(struct facet));
    if (theMesh.coords == NULL || theMesh.facets == NULL) {
	printf("Out of memory.\n");
	exit(1);
    }

    for (frameNum = 0; frameNum < frames; frameNum++) {
	for (i = 0; i <= widthX; i++) {
	    x = i / (float)widthX;
	    for (j = 0; j <= widthY; j++) {
		y = j / (float)widthY;

		d = sqrt(x*x+y*y);
		if (d == 0.0) {
		    d = 0.0001;
		}
		angle = 2 * PI * d + (2 * PI / frames * frameNum);

		coord = GETCOORD(frameNum, i, j);

		coord->vertex[0] = x - 0.5;
		coord->vertex[1] = y - 0.5;
		coord->vertex[2] = (height - height * d) * cos(angle);

		coord->normal[0] = -(height / d) * x * ((1 - d) * 2 * PI *
				   sin(angle) + cos(angle));
		coord->normal[1] = -(height / d) * y * ((1 - d) * 2 * PI *
				   sin(angle) + cos(angle));
		coord->normal[2] = -1;

		d = 1.0 / sqrt(coord->normal[0]*coord->normal[0]+
			       coord->normal[1]*coord->normal[1]+1);
		coord->normal[0] *= d;
		coord->normal[1] *= d;
		coord->normal[2] *= d;
	    }
	}
	for (i = 0; i < widthX; i++) {
	    for (j = 0; j < widthY; j++) {
		facet = GETFACET(frameNum, i, j);
		if (((i/checkerSize)%2)^(j/checkerSize)%2) {
		    if (rgb) {
			facet->color[0] = 1.0;
			facet->color[1] = 0.2;
			facet->color[2] = 0.2;
		    } else {
			facet->color[0] = colorIndexes1[0];
			facet->color[1] = colorIndexes1[1];
			facet->color[2] = colorIndexes1[2];
		    }
		} else {
		    if (rgb) {
			facet->color[0] = 0.2;
			facet->color[1] = 1.0;
			facet->color[2] = 0.2;
		    } else {
			facet->color[0] = colorIndexes2[0];
			facet->color[1] = colorIndexes2[1];
			facet->color[2] = colorIndexes2[2];
		    }
		}
		pt1 = GETCOORD(frameNum, i, j)->vertex;
		pt2 = GETCOORD(frameNum, i, j+1)->vertex;
		pt3 = GETCOORD(frameNum, i+1, j+1)->vertex;

		dp1[0] = pt2[0] - pt1[0];
		dp1[1] = pt2[1] - pt1[1];
		dp1[2] = pt2[2] - pt1[2];

		dp2[0] = pt3[0] - pt2[0];
		dp2[1] = pt3[1] - pt2[1];
		dp2[2] = pt3[2] - pt2[2];

		facet->normal[0] = dp1[1] * dp2[2] - dp1[2] * dp2[1];
		facet->normal[1] = dp1[2] * dp2[0] - dp1[0] * dp2[2];
		facet->normal[2] = dp1[0] * dp2[1] - dp1[1] * dp2[0];

		d = 1.0 / sqrt(facet->normal[0]*facet->normal[0]+
			       facet->normal[1]*facet->normal[1]+
			       facet->normal[2]*facet->normal[2]);

		facet->normal[0] *= d;
		facet->normal[1] *= d;
		facet->normal[2] *= d;
	    }
	}
    }
}

static void InitMaterials(void)
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static float position[] = {90.0, 90.0, 150.0, 0.0};
    static float front_mat_shininess[] = {60.0};
    static float front_mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static float front_mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    static float back_mat_shininess[] = {60.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_TRUE};

    glMatrixMode(GL_PROJECTION);
    gluPerspective(90.0, 1.0, 0.5, 10.0);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    glMaterialfv(GL_FRONT, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_BACK, GL_SHININESS, back_mat_shininess);
    glMaterialfv(GL_BACK, GL_SPECULAR, back_mat_specular);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
    if (rgb) {
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    }

    if (rgb) {
	glEnable(GL_COLOR_MATERIAL);
    } else {
	SetColorMap();
    }
}

static void InitTexture(void)
{

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void Init(void)
{

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glShadeModel(GL_FLAT);
    
    glFrontFace(GL_CW);

    glEnable(GL_DEPTH_TEST);

    InitMaterials();
    InitTexture();
    InitMesh();

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0.0, 0.4, -1.8);
    glScalef(2.0, 2.0, 2.0);
    glRotatef(-35.0, 1.0, 0.0, 0.0);
    glRotatef(35.0, 0.0, 0.0, 1.0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
      case 'c':
	contouring++;
	if (contouring == 1) {
	    static GLfloat map[4] = {0, 0, 20, 0};

	    glTexImage2D(GL_TEXTURE_2D, 0, 3, 4, 4, 0, GL_LUMINANCE,
			 GL_UNSIGNED_BYTE, (GLvoid *)contourTexture1);
	    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	    glTexGenfv(GL_S, GL_OBJECT_PLANE, map);
	    glTexGenfv(GL_T, GL_OBJECT_PLANE, map);
	    glEnable(GL_TEXTURE_2D);
	    glEnable(GL_TEXTURE_GEN_S);
	    glEnable(GL_TEXTURE_GEN_T);
	} else if (contouring == 2) {
	    static GLfloat map[4] = {0, 0, 20, 0};

	    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	    glPushMatrix();
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
	    glTexGenfv(GL_S, GL_EYE_PLANE, map);
	    glTexGenfv(GL_T, GL_EYE_PLANE, map);
	    glPopMatrix();
	} else {
	    contouring = 0;
	    glDisable(GL_TEXTURE_GEN_S);
	    glDisable(GL_TEXTURE_GEN_T);
	    glDisable(GL_TEXTURE_2D);
	}
	break;
      case 's':
	smooth = !smooth;
	if (smooth) {
	    glShadeModel(GL_SMOOTH);
	} else {
	    glShadeModel(GL_FLAT);
	}
	break;
      case 'l':
	lighting = !lighting;
	if (lighting) {
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    if (rgb) {
		glEnable(GL_COLOR_MATERIAL);
	    }
	} else {
	    glDisable(GL_LIGHTING);
	    glDisable(GL_LIGHT0);
	    if (rgb) {
		glDisable(GL_COLOR_MATERIAL);
	    }
	}
	break;
      case 'd':
	depth = !depth;
	if (depth) {
	    glEnable(GL_DEPTH_TEST);
	    clearMask |= GL_DEPTH_BUFFER_BIT;
	} else {
	    glDisable(GL_DEPTH_TEST);
	    clearMask &= ~GL_DEPTH_BUFFER_BIT;
	}
	break;
      case 32:
	stepMode = !stepMode;
	if (stepMode) {
	    glutIdleFunc(0);
	} else {
	    glutIdleFunc(glut_post_redisplay_p);
	}
	break;
      case 'n':
	if (stepMode) {
	    nextFrame = 1;
	}
	break;
      case 'a':
	spinMode = !spinMode;
	break;
      default:
	return;
    }
    glutPostRedisplay();
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    rgb = GL_TRUE;
    doubleBuffer = GL_FALSE;
    frames = 10;
    widthX = 10;
    widthY = 10;
    checkerSize = 2;
    height = 0.2;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-ci") == 0) {
	    rgb = GL_FALSE;
	} else if (strcmp(argv[i], "-rgb") == 0) {
	    rgb = GL_TRUE;
	} else if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else if (strcmp(argv[i], "-grid") == 0) {
	    if (i+2 >= argc || argv[i+1][0] == '-' || argv[i+2][0] == '-') {
		printf("-grid (No numbers).\n");
		return GL_FALSE;
	    } else {
		widthX = atoi(argv[++i]);
		widthY = atoi(argv[++i]);
	    }
	} else if (strcmp(argv[i], "-size") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-checker (No number).\n");
		return GL_FALSE;
	    } else {
		checkerSize = atoi(argv[++i]);
	    }
	} else if (strcmp(argv[i], "-wave") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-wave (No number).\n");
		return GL_FALSE;
	    } else {
		height = atof(argv[++i]);
	    }
	} else if (strcmp(argv[i], "-frames") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-frames (No number).\n");
		return GL_FALSE;
	    } else {
		frames = atoi(argv[++i]);
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

    glutInitWindowPosition(0, 0); glutInitWindowSize( 300, 300);

    type = GLUT_DEPTH;
    type |= (rgb) ? GLUT_RGB : GLUT_INDEX;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Wave Demo") == GL_FALSE) {
	exit(1);
    }

    InitMap();

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Animate);
    glutIdleFunc(glut_post_redisplay_p);
    glutMainLoop();
	return 0;
}
