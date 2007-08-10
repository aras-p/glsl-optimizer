/*
 * glRead/DrawPixels test
 */


#define GL_GLEXT_PROTOTYPES
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>

static int Width = 250, Height = 250;

static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   glClearColor(0.3, 0.1, 0.3, 0.0);
}

static void Reshape(int width, int height)
{
   Width = width / 2;
   Height = height;
   /* draw on left half (we'll read that area) */
   glViewport(0, 0, Width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{
   switch (key) {
   case 27:
      exit(0);
   default:
      return;
   }
   glutPostRedisplay();
}

static void Draw(void)
{
   GLfloat *image = (GLfloat *) malloc(Width * Height * 4 * sizeof(GLfloat));

   glClear(GL_COLOR_BUFFER_BIT); 

   glBegin(GL_TRIANGLES);
   glColor3f(.8,0,0); 
   glVertex3f(-0.9, -0.9, -30.0);
   glColor3f(0,.9,0); 
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(0,0,.7); 
   glVertex3f( 0.0,  0.9, -30.0);
   glEnd();

   glReadPixels(0, 0, Width, Height, GL_RGBA, GL_FLOAT, image);
   printf("Pixel(0,0) = %f, %f, %f, %f\n",
          image[0], image[1], image[2], image[3]);
   /* draw to right half of window */
   glWindowPos2iARB(Width, 0);
   glDrawPixels(Width, Height, GL_RGBA, GL_FLOAT, image);
   free(image);

   glutSwapBuffers();
}

int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width*2, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE);
   if (glutCreateWindow(argv[0]) == GL_FALSE) {
      exit(1);
   }

   Init();

   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   glutMainLoop();
   return 0;
}
