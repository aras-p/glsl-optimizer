/**
 * Draw a series of textured quads after each quad, use glTexSubImage()
 * to change one row of the texture image.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>


static GLint Win = 0;
static GLuint Tex = 0;


static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fflush(stderr);

   glGenTextures(1, &Tex);
   glBindTexture(GL_TEXTURE_2D, Tex);

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
      glDeleteTextures(1, &Tex);
      glutDestroyWindow(Win);
      exit(1);
   }
   glutPostRedisplay();
}


static void Draw(void)
{
   GLubyte tex[16][16][4];
   GLubyte row[16][4];
   int i, j;

   for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
         if ((i + j) & 1) {
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

   for (i = 0; i < 16; i++) {
      row[i][0] = 255;
      row[i][1] = 0;
      row[i][2] = 0;
      row[i][3] = 255;
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, tex);
   glEnable(GL_TEXTURE_2D);

   glClear(GL_COLOR_BUFFER_BIT);

   for (i = 0; i < 9; i++) {

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

      /* replace a row of the texture image with red texels */
      if (i * 2 < 16)
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, i*2, 16, 1,
                         GL_RGBA, GL_UNSIGNED_BYTE, row);
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
