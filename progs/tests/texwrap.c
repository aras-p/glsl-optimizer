/* $Id: texwrap.c,v 1.2 2001/04/12 20:50:26 brianp Exp $ */

/*
 * Test texture wrap modes.
 * Press 'b' to toggle texture image borders.  You should see the same
 * rendering whether or not you're using borders.
 *
 * Brian Paul   March 2001
 */


#define GL_GLEXT_PROTOTYPES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


#ifndef GL_CLAMP_TO_BORDER_ARB
#define GL_CLAMP_TO_BORDER_ARB 0x812D
#endif


#define BORDER_TEXTURE 1
#define NO_BORDER_TEXTURE 2

#define SIZE 8
static GLubyte BorderImage[SIZE+2][SIZE+2][4];
static GLubyte NoBorderImage[SIZE][SIZE][4];
static GLuint Border = 1;


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void Display( void )
{
   static const GLenum modes[] = {
      GL_REPEAT,
      GL_CLAMP,
      GL_CLAMP_TO_EDGE,
      GL_CLAMP_TO_BORDER_ARB
   };
   static const char *names[] = {
      "GL_REPEAT",
      "GL_CLAMP",
      "GL_CLAMP_TO_EDGE",
      "GL_CLAMP_TO_BORDER_ARB"
   };

   GLint i, j;
   GLint numModes;

   numModes = glutExtensionSupported("GL_ARB_texture_border_clamp") ? 4 : 3;

   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear( GL_COLOR_BUFFER_BIT );

#if 0
   /* draw texture as image */
   glDisable(GL_TEXTURE_2D);
   glWindowPos2iMESA(1, 1);
   glDrawPixels(6, 6, GL_RGBA, GL_UNSIGNED_BYTE, (void *) TexImage);
#endif

   glBindTexture(GL_TEXTURE_2D, Border ? BORDER_TEXTURE : NO_BORDER_TEXTURE);

   /* loop over min/mag filters */
   for (i = 0; i < 2; i++) {
      if (i) {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

      /* loop over border modes */
      for (j = 0; j < numModes; j++) {
         const GLfloat x0 = 0, y0 = 0, x1 = 140, y1 = 140;
         const GLfloat b = 0.2;
         const GLfloat s0 = -b, t0 = -b, s1 = 1.0+b, t1 = 1.0+b;
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, modes[j]);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, modes[j]);

         glPushMatrix();
            glTranslatef(j * 150 + 10, i * 150 + 25, 0);

            glEnable(GL_TEXTURE_2D);
            glColor3f(1, 1, 1);
            glBegin(GL_POLYGON);
            glTexCoord2f(s0, t0);  glVertex2f(x0, y0);
            glTexCoord2f(s1, t0);  glVertex2f(x1, y0);
            glTexCoord2f(s1, t1);  glVertex2f(x1, y1);
            glTexCoord2f(s0, t1);  glVertex2f(x0, y1);
            glEnd();

            /* draw red outline showing bounds of texture at s=0,1 and t=0,1 */
            glDisable(GL_TEXTURE_2D);
            glColor3f(1, 0, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x0 + b * (x1-x0) / (s1-s0), y0 + b * (y1-y0) / (t1-t0));
            glVertex2f(x1 - b * (x1-x0) / (s1-s0), y0 + b * (y1-y0) / (t1-t0));
            glVertex2f(x1 - b * (x1-x0) / (s1-s0), y1 - b * (y1-y0) / (t1-t0));
            glVertex2f(x0 + b * (x1-x0) / (s1-s0), y1 - b * (y1-y0) / (t1-t0));
            glEnd();

         glPopMatrix();
      }
   }

   glDisable(GL_TEXTURE_2D);
   glColor3f(1, 1, 1);
   for (i = 0; i < numModes; i++) {
      glWindowPos2iMESA( i * 150 + 10, 5);
      PrintString(names[i]);
   }

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         Border = !Border;
         printf("Texture Border Size = %d\n", Border);
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   static const GLubyte border[4] = { 0, 255, 0, 255 };
   static const GLfloat borderf[4] = { 0, 1.0, 0, 1.0 };
   GLint i, j;

   for (i = 0; i < SIZE+2; i++) {
      for (j = 0; j < SIZE+2; j++) {
         if (i == 0 || j == 0 || i == SIZE+1 || j == SIZE+1) {
            /* border color */
            BorderImage[i][j][0] = border[0];
            BorderImage[i][j][1] = border[1];
            BorderImage[i][j][2] = border[2];
            BorderImage[i][j][3] = border[3];
         }
         else if ((i + j) & 1) {
            /* white */
            BorderImage[i][j][0] = 255;
            BorderImage[i][j][1] = 255;
            BorderImage[i][j][2] = 255;
            BorderImage[i][j][3] = 255;
         }
         else {
            /* black */
            BorderImage[i][j][0] = 0;
            BorderImage[i][j][1] = 0;
            BorderImage[i][j][2] = 0;
            BorderImage[i][j][3] = 0;
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, BORDER_TEXTURE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE+2, SIZE+2, 1,
                GL_RGBA, GL_UNSIGNED_BYTE, (void *) BorderImage);


   for (i = 0; i < SIZE; i++) {
      for (j = 0; j < SIZE; j++) {
         if ((i + j) & 1) {
            /* white */
            NoBorderImage[i][j][0] = 255;
            NoBorderImage[i][j][1] = 255;
            NoBorderImage[i][j][2] = 255;
            NoBorderImage[i][j][3] = 255;
         }
         else {
            /* black */
            NoBorderImage[i][j][0] = 0;
            NoBorderImage[i][j][1] = 0;
            NoBorderImage[i][j][2] = 0;
            NoBorderImage[i][j][3] = 0;
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, NO_BORDER_TEXTURE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, (void *) NoBorderImage);
   glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderf);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 650, 340 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
