
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>



static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));

    glClearColor(0.5, 0.5, 0.5, 0.0);
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
	return;
    }

    glutPostRedisplay();
}

static void Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT); 
   glShadeModel(GL_FLAT);

   glBegin(GL_TRIANGLES);
   glColor3f(0,0,1); 
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(1,0,0); 
   glVertex3f( 0.9,  0.9, -30.0);
   glColor3f(0,1,0); 
   glVertex3f(-0.9,  0.0, -30.0);
   glEnd();

   glFlush();



   /* Exit after first frame
    */
   exit(0);
}


int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);



    glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB;
    type |= GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("First Tri") == GL_FALSE) {
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
