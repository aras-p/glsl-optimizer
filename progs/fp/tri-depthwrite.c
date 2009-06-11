
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>


static void Init(void)
{
   static const char *modulate2D =
      "!!ARBfp1.0\n"
      "MUL result.depth.z, fragment.color.z, {.1}.x; \n"
      "MOV result.color.xy, fragment.color; \n"
      "END"
      ;
   GLuint modulateProg;

   if (!GLEW_ARB_fragment_program) {
      printf("Error: GL_ARB_fragment_program not supported!\n");
      exit(1);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* Setup the fragment program */
   glGenProgramsARB(1, &modulateProg);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, modulateProg);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(modulate2D), (const GLubyte *)modulate2D);

   printf("glGetError = 0x%x\n", (int) glGetError());
   printf("glError(GL_PROGRAM_ERROR_STRING_ARB) = %s\n",
          (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));

   glEnable(GL_FRAGMENT_PROGRAM_ARB);

   glClearColor(.3, .3, .3, 0);
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
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);


    glBegin(GL_TRIANGLES);
    glColor4f(.8,0,.5,0);
	glVertex3f( 0.9, -0.9, -30.0);
	glVertex3f( 0.9,  0.9, -30.0);
	glVertex3f(-0.9,  0.0, -30.0);

    glColor4f(0,.8,.7,0);
	glVertex3f(-0.9, -0.9, -40.0);
    glColor4f(0,.8,.7,0);
	glVertex3f(-0.9,  0.9, -40.0);
    glColor4f(0,.8,.3,0);
	glVertex3f( 0.9,  0.0, -40.0);
    glEnd();

    glFlush();
}


int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    glutInitWindowPosition(0, 0); glutInitWindowSize( 300, 300);

    glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_SINGLE);

    if (glutCreateWindow("Depth Test") == GL_FALSE) {
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
