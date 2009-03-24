/*
 * glClear + glScissor + Undefined content of framebuffer
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>


GLenum doubleBuffer;
GLint Width = 200, Height = 150;

static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fprintf(stderr, "Top right corner should be red\n");
   fflush(stderr);
}

static void Reshape(int width, int height)
{
   Width = width;
   Height = height;

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
	glutPostRedisplay();
	return;
    }

}

static void Draw(void)
{
   glColor4f(1.0, 0.0, 0.0, 1.0);
   glBegin(GL_QUADS);
   glVertex2d(0.0, 0.0);
   glVertex2d(0.0, 1.0);
   glVertex2d(1.0, 1.0);
   glVertex2d(1.0, 0.0);
   glEnd();

   glBegin(GL_QUADS);
   glVertex2d(0.0,  0.0);
   glVertex2d(0.0, -1.0);
   glVertex2d(1.0, -1.0);
   glVertex2d(1.0,  0.0);
   glEnd();

   glEnable(GL_SCISSOR_TEST);
   glClearColor(1, 1, 0, 0);
   glScissor(Width / 2, 0, Width - Width / 2, Height / 2);
   glClear(GL_COLOR_BUFFER_BIT);

   glClearColor(0, 0, 1, 0);
   glScissor(0, Height / 2, Width / 2, Height - Height / 2);
   glClear(GL_COLOR_BUFFER_BIT);
   glDisable(GL_SCISSOR_TEST);

   glColor4f(0.0, 1.0, 0.0, 1.0);
   glBegin(GL_QUADS);
   glVertex2d( 0.0,  0.0);
   glVertex2d( 0.0, -1.0);
   glVertex2d(-1.0, -1.0);
   glVertex2d(-1.0,  0.0);
   glEnd();

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

    glutInitWindowPosition(0, 0); glutInitWindowSize( Width, Height );


    type = GLUT_RGB | GLUT_ALPHA;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow(argv[0]) == GL_FALSE) {
	exit(1);
    }

    Init();
	Reshape(Width, Height);

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
    return 0;
}
