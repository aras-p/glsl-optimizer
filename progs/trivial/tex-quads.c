/**
 * Draw a series of quads, each with a different texture.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

#define NUM_TEX 10

static GLint Win = 0;
static GLuint Tex[NUM_TEX];


static void Init(void)
{
   int i;

   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fflush(stderr);

   glGenTextures(NUM_TEX, Tex);

   for (i = 0; i < NUM_TEX; i++) {
      glBindTexture(GL_TEXTURE_2D, Tex[i]);

      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }
}


static void Reshape(int width, int height)
{
   float ar = (float) width / height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-ar, ar, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
}


static void Key(unsigned char key, int x, int y)
{
   if (key == 27) {
      glDeleteTextures(NUM_TEX, Tex);
      glutDestroyWindow(Win);
      exit(1);
   }
   glutPostRedisplay();
}


static void Draw(void)
{
   GLubyte tex[16][16][4];
   int t, i, j;

   for (t = 0; t < NUM_TEX; t++) {

      for (i = 0; i < 16; i++) {
         for (j = 0; j < 16; j++) {
            if (i < t) {
               /* red row */
               tex[i][j][0] = 255;
               tex[i][j][1] = 0;
               tex[i][j][2] = 0;
               tex[i][j][3] = 255;
            }
            else if ((i + j) & 1) {
               tex[i][j][0] = 128;
               tex[i][j][1] = 128;
               tex[i][j][2] = 128;
               tex[i][j][3] = 255;
            }
            else {
               tex[i][j][0] = 255;
               tex[i][j][1] = 255;
               tex[i][j][2] = 255;
               tex[i][j][3] = 255;
            }
         }
      }

      glBindTexture(GL_TEXTURE_2D, Tex[t]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, tex);
   }

   glEnable(GL_TEXTURE_2D);

   glClear(GL_COLOR_BUFFER_BIT);

   for (i = 0; i < NUM_TEX; i++) {

      glBindTexture(GL_TEXTURE_2D, Tex[i]);

      glPushMatrix();
      glTranslatef(-4.0 + i, 0, 0);
      glScalef(0.5, 0.5, 1.0);

      glBegin(GL_QUADS);
      glTexCoord2f(1,0);
      glVertex3f( 0.9, -0.9, 0.0);
      glTexCoord2f(1,1);
      glVertex3f( 0.9,  0.9, 0.0);
      glTexCoord2f(0,1);
      glVertex3f(-0.9,  0.9, 0.0);
      glTexCoord2f(0,0);
      glVertex3f(-0.9,  -0.9, 0.0);
      glEnd();

      glPopMatrix();
   }


   glutSwapBuffers();
}


int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitWindowSize(900, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(*argv);
   if (!Win) {
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
