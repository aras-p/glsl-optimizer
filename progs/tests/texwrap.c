/* $Id: texwrap.c,v 1.1 2001/03/26 19:45:57 brianp Exp $ */

/*
 * Test texture wrap modes.
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


#define SIZE 4
static GLubyte TexImage[SIZE+2][SIZE+2][4];


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

   glEnable(GL_TEXTURE_2D);

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
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, modes[j]);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, modes[j]);

         glPushMatrix();
            glTranslatef(j * 150 + 10, i * 150 + 25, 0);

            glBegin(GL_POLYGON);
            glTexCoord2f(-0.2, -0.2);  glVertex2f(  0,   0);
            glTexCoord2f( 1.2, -0.2);  glVertex2f(140,   0);
            glTexCoord2f( 1.2,  1.2);  glVertex2f(140, 140);
            glTexCoord2f(-0.2,  1.2);  glVertex2f(  0, 140);
            glEnd();

         glPopMatrix();
      }
   }

   glDisable(GL_TEXTURE_2D);
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
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   static const GLubyte border[4] = { 0, 255, 0, 255 };
   GLint i, j;

   for (i = 0; i < SIZE+2; i++) {
      for (j = 0; j < SIZE+2; j++) {
         if (i == 0 || j == 0 || i == SIZE+1 || j == SIZE+1) {
            /* border color */
            TexImage[i][j][0] = border[0];
            TexImage[i][j][1] = border[1];
            TexImage[i][j][2] = border[2];
            TexImage[i][j][3] = border[3];
         }
         else if ((i + j) & 1) {
            /* white */
            TexImage[i][j][0] = 255;
            TexImage[i][j][1] = 255;
            TexImage[i][j][2] = 255;
            TexImage[i][j][3] = 255;
         }
         else {
            /* black */
            TexImage[i][j][0] = 0;
            TexImage[i][j][1] = 0;
            TexImage[i][j][2] = 0;
            TexImage[i][j][3] = 0;
         }
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE+2, SIZE+2, 1,
                GL_RGBA, GL_UNSIGNED_BYTE, (void *) TexImage);
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
