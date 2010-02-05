/*
 * Test cylindrical texcoord wrapping
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

static int Win;
static int WinWidth = 600, WinHeight = 400;
static GLfloat Xrot = 0, Yrot = 0;
static GLboolean CylWrap = GL_TRUE;
static GLboolean Lines = GL_FALSE;



static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
DrawSample(GLboolean wrap)
{
   float p;

   glEnable(GL_TEXTURE_2D);

   if (Lines) {
      /* texured lines */
      float t;
      for (t = 0; t <= 1.0; t += 0.125) {
         float y = -1.0 + 2.0 * t;
         glBegin(GL_LINE_STRIP);
         for (p = 0.0; p <= 1.001; p += 0.05) {
            float x = -2.0 + p * 4.0;
            float s = p + 0.5;
            if (wrap && s > 1.0)
               s -= 1.0;
            glTexCoord2f(s, t);  glVertex2f(x, y);
         }
         glEnd();
      }
   }
   else {
      /* texured quads */
      glBegin(GL_QUAD_STRIP);
      for (p = 0.0; p <= 1.001; p += 0.1) {
         float x = -2.0 + p * 4.0;
         float s = p + 0.5;
         if (wrap && s > 1.0)
            s -= 1.0;
         glTexCoord2f(s, 0);  glVertex2f(x, -1);
         glTexCoord2f(s, 1);  glVertex2f(x, +1);
      }
      glEnd();
   }

   glDisable(GL_TEXTURE_2D);

   /* hash marks */
   glColor3f(0,0,0);
   glBegin(GL_LINES);
   for (p = 0.0; p <= 1.001; p += 0.1) {
      float x = -2.0 + p * 4.0;
      glVertex2f(x, -1.1);
      glVertex2f(x, -0.8);
   }
   glEnd();

   /* labels */
   glColor3f(1,1,1);
   for (p = 0.0; p <= 1.001; p += 0.1) {
      char str[100];
      float x = -2.0 + p * 4.0;
      float s = p + 0.5;

      if (wrap && s > 1.0)
         s -= 1.0;

      sprintf(str, "%3.1f", s);
      glRasterPos2f(x, -1.2);
      glBitmap(0, 0, 0, 0, -11, 0, NULL);
      PrintString(str);
      if (p == 0.0) {
         glBitmap(0, 0, 0, 0, -55, 0, NULL);
         PrintString("s =");
      }
   }
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glPushMatrix();
      glRotatef(Xrot, 1, 0, 0);
      glRotatef(Yrot, 0, 1, 0);

      glPushMatrix();
         glTranslatef(0, +1.2, 0);
         DrawSample(GL_FALSE);
      glPopMatrix();

      /* set Mesa back-door state for testing cylindrical wrap mode */
      if (CylWrap)
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.125);

      glPushMatrix();
         glTranslatef(0, -1.2, 0);
         DrawSample(GL_TRUE);
      glPopMatrix();

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1.0);

   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 3.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case 'c':
   case 'C':
      CylWrap = !CylWrap;
      if (CylWrap)
         printf("Cylindrical wrap on.\n");
      else
         printf("Cylindrical wrap off.\n");
      break;
   case 'l':
   case 'L':
      Lines = !Lines;
      break;
   case 27:
      glutDestroyWindow(Win);
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      Xrot -= step;
      break;
   case GLUT_KEY_DOWN:
      Xrot += step;
      break;
   case GLUT_KEY_LEFT:
      Yrot -= step;
      break;
   case GLUT_KEY_RIGHT:
      Yrot += step;
      break;
   }
   glutPostRedisplay();
}


static void
MakeSineWaveTexture(void)
{
   GLubyte tex[128][512][4];
   int i, j;

   for (j = 0; j < 128; j++) {
      for (i = 0; i < 512; i++) {
         float x = i / 511.0 * 2.0 * M_PI + M_PI * 0.5;
         float y0 = sin(x) * 0.5 + 0.5;
         int jy0 = y0 * 128;
         float y1 = sin(x + M_PI) * 0.5 + 0.5;
         int jy1 = y1 * 128;
         if (j < jy0)
            tex[j][i][0] = 0xff;
         else
            tex[j][i][0] = 0;
         if (j < jy1)
            tex[j][i][1] = 0xff;
         else
            tex[j][i][1] = 0;
         tex[j][i][2] = 0;
         tex[j][i][3] = 0xff;
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 128, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


static void
Init(void)
{
   glBindTexture(GL_TEXTURE_2D, 5);
   MakeSineWaveTexture();

   glClearColor(0.5, 0.5, 0.5, 0.0);
   glPointSize(3.0);

   printf("Press 'c' to toggle cylindrical wrap mode.\n");
   printf("Press 'l' to toggle line / quad drawing.\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutSpecialFunc(SpecialKey);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
