/**
 * Test glGetTexImage()
 * Brian Paul
 * 9 June 2009
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

static int Win;


static void
TestGetTexImage(void)
{
   GLuint iter;

   for (iter = 0; iter < 20; iter++) {
      GLint p = (iter % 6) + 4;
      GLint w = (1 << p);
      GLint h = (1 << p);
      GLubyte data[512*512*4];
      GLubyte data2[512*512*4];
      GLuint i;
      GLint level = 0;

      printf("Testing %d x %d tex image\n", w, h);
      /* fill data */
      for (i = 0; i < w * h * 4; i++) {
         data[i] = i & 0xff;
         data2[i] = 0;
      }

      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, data);

      glBegin(GL_POINTS);
      glVertex2f(0, 0);
      glEnd();

      /* get */
      glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_BYTE, data2);

      /* compare */
      for (i = 0; i < w * h * 4; i++) {
         if (data2[i] != data[i]) {
            printf("Failure!\n");
            abort();
         }
      }
   }
   printf("Passed\n");
   glutDestroyWindow(Win);
   exit(0);
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   TestGetTexImage();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glEnable(GL_TEXTURE_2D);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
