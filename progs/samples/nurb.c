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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>


#ifndef CALLBACK
#define CALLBACK
#endif


#define INREAL float

#define S_NUMPOINTS 13
#define S_ORDER     3   
#define S_NUMKNOTS  (S_NUMPOINTS + S_ORDER)
#define T_NUMPOINTS 3
#define T_ORDER     3 
#define T_NUMKNOTS  (T_NUMPOINTS + T_ORDER)
#define SQRT_TWO    1.41421356237309504880


typedef INREAL Point[4];


GLenum doubleBuffer;

GLenum expectedError;
GLint rotX = 40, rotY = 40;
INREAL sknots[S_NUMKNOTS] = {
    -1.0, -1.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0,
    4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 9.0, 9.0
};
INREAL tknots[T_NUMKNOTS] = {
    1.0, 1.0, 1.0, 2.0, 2.0, 2.0
};
Point ctlpoints[S_NUMPOINTS][T_NUMPOINTS] = {
    {
	{
	    4.0, 2.0, 2.0, 1.0
	},
	{
	    4.0, 1.6, 2.5, 1.0
	},
	{
	    4.0, 2.0, 3.0, 1.0
	}
    },
    {
	{
	    5.0, 4.0, 2.0, 1.0
	},
	{
	    5.0, 4.0, 2.5, 1.0
	},
	{
	    5.0, 4.0, 3.0, 1.0
	}
    },
    {
	{
	    6.0, 5.0, 2.0, 1.0
	},
	{
	    6.0, 5.0, 2.5, 1.0
	},
	{
	    6.0, 5.0, 3.0, 1.0
	}
    },
    {
	{
	    SQRT_TWO*6.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*6.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	},
	{
	    SQRT_TWO*6.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    },
    {
	{
	    5.2, 6.7, 2.0, 1.0
	},
	{
	    5.2, 6.7, 2.5, 1.0
	},
	{
	    5.2, 6.7, 3.0, 1.0
	}
    },
    {
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	}, 
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    }, 
    {
	{
	    4.0, 5.2, 2.0, 1.0
	},
	{
	    4.0, 4.6, 2.5, 1.0
	},
	{
	    4.0, 5.2, 3.0, 1.0
	}  
    },
    {
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	},
	{
	    SQRT_TWO*4.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    },
    {
	{
	    2.8, 6.7, 2.0, 1.0
	},
	{
	    2.8, 6.7, 2.5, 1.0
	},
	{
	    2.8, 6.7, 3.0, 1.0
	}   
    },
    {
	{
	    SQRT_TWO*2.0, SQRT_TWO*6.0, SQRT_TWO*2.0, SQRT_TWO
	},
	{
	    SQRT_TWO*2.0, SQRT_TWO*6.0, SQRT_TWO*2.5, SQRT_TWO
	},
	{
	    SQRT_TWO*2.0, SQRT_TWO*6.0, SQRT_TWO*3.0, SQRT_TWO
	}  
    },
    {
	{
	    2.0, 5.0, 2.0, 1.0
	},
	{
	    2.0, 5.0, 2.5, 1.0
	},
	{
	    2.0, 5.0, 3.0, 1.0
	} 
    },
    {
	{
	    3.0, 4.0, 2.0, 1.0
	},
	{
	    3.0, 4.0, 2.5, 1.0
	},
	{
	    3.0, 4.0, 3.0, 1.0
	} 
    },
    {
	{
	    4.0, 2.0, 2.0, 1.0
	},
	{
	    4.0, 1.6, 2.5, 1.0
	},
	{
	    4.0, 2.0, 3.0, 1.0
	}    
    }
};
GLUnurbsObj *theNurbs;


static void CALLBACK ErrorCallback(GLenum which)
{

    if (which != expectedError) {
	fprintf(stderr, "Unexpected error occured (%d):\n", which);
	fprintf(stderr, "    %s\n", (char *) gluErrorString(which));
    }
}

typedef void (GLAPIENTRY *callback_t)();

static void Init(void)
{

    theNurbs = gluNewNurbsRenderer();
    gluNurbsCallback(theNurbs, GLU_ERROR, (callback_t) ErrorCallback);

    gluNurbsProperty(theNurbs, GLU_SAMPLING_TOLERANCE, 15.0);
    gluNurbsProperty(theNurbs, GLU_DISPLAY_MODE, GLU_OUTLINE_PATCH);

    expectedError = GLU_INVALID_ENUM;
    gluNurbsProperty(theNurbs, ~0, 15.0);
    expectedError = GLU_NURBS_ERROR13;
    gluEndSurface(theNurbs);
    expectedError = 0;

    glColor3f(1.0, 1.0, 1.0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-2.0, 2.0, -2.0, 2.0, 0.8, 10.0);
    gluLookAt(7.0, 4.5, 4.0, 4.5, 4.5, 2.5, 6.0, -3.0, 2.0);
    glMatrixMode(GL_MODELVIEW);
}

static void Key2(int key, int x, int y)
{

    switch (key) {
      case GLUT_KEY_DOWN:
	rotX -= 5;
	break;
      case GLUT_KEY_UP:
	rotX += 5;
	break;
      case GLUT_KEY_LEFT:
	rotY -= 5;
	break;
      case GLUT_KEY_RIGHT:
	rotY += 5;
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
    }
}

static void Draw(void)
{

    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();

    glTranslatef(4.0, 4.5, 2.5);
    glRotatef(rotY, 1, 0, 0);
    glRotatef(rotX, 0, 1, 0);
    glTranslatef(-4.0, -4.5, -2.5);

    gluBeginSurface(theNurbs);
    gluNurbsSurface(theNurbs, S_NUMKNOTS, sknots, T_NUMKNOTS, tknots,
		    4*T_NUMPOINTS, 4, &ctlpoints[0][0][0], S_ORDER,
		    T_ORDER, GL_MAP2_VERTEX_4);
    gluEndSurface(theNurbs);

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

    type = GLUT_RGB;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("NURBS Test") == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutSpecialFunc(Key2);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
