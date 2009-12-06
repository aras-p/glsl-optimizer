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



#define CI_OFFSET_1 16
#define CI_OFFSET_2 32

static GLuint OccQuery;

GLenum doubleBuffer;

static void Init(void)
{
   if (!glutExtensionSupported("GL_ARB_occlusion_query")) {
      fprintf(stderr, "Sorry, this program requires GL_ARB_occlusion_query\n");
      exit(1);
   }

   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

    glClearColor(0.0, 0.0, 1.0, 0.0);

   glGenQueriesARB(1, &OccQuery);
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
      default:
	break;
    }

    glutPostRedisplay();
}

static void Draw(void)
{
   GLuint passed;
   GLint ready;

   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 

   glEnable(GL_DEPTH_TEST);

   glBeginQueryARB(GL_SAMPLES_PASSED_ARB, OccQuery);

   glBegin(GL_TRIANGLES);
   glColor3f(0,0,.7); 
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(.8,0,0); 
   glVertex3f( 0.9,  0.9, -30.0);
   glColor3f(0,.9,0); 
   glVertex3f(-0.9,  0.0, -30.0);
   glEnd();

   glEndQueryARB(GL_SAMPLES_PASSED_ARB);

   do {
      /* do useful work here, if any */
      glGetQueryObjectivARB(OccQuery, GL_QUERY_RESULT_AVAILABLE_ARB, &ready);
   } while (!ready);
   glGetQueryObjectuivARB(OccQuery, GL_QUERY_RESULT_ARB, &passed);

   fprintf(stderr, " %d Fragments Visible\n", passed);

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

    glutInitWindowPosition(100, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB | GLUT_DEPTH;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow(*argv) == GL_FALSE) {
	exit(1);
    }

    glewInit();

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
