/* $Id: paltex.c,v 1.1 1999/08/19 00:55:40 jtg Exp $ */

/*
 * Paletted texture demo.  Written by Brian Paul.  This file in public domain.
 */

/*
 * $Log: paltex.c,v $
 * Revision 1.1  1999/08/19 00:55:40  jtg
 * Initial revision
 *
 * Revision 3.1  1999/03/28 18:20:49  brianp
 * minor clean-up
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static float Rot = 0.0;


static void Idle( void )
{
   Rot += 5.0;
   glutPostRedisplay();
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Rot, 0, 0, 1);

   glBegin(GL_POLYGON);
   glTexCoord2f(0, 1);  glVertex2f(-1, -1);
   glTexCoord2f(1, 1);  glVertex2f( 1, -1);
   glTexCoord2f(1, 0);  glVertex2f( 1,  1);
   glTexCoord2f(0, 0);  glVertex2f(-1,  1);
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
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


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         break;
      case GLUT_KEY_DOWN:
         break;
      case GLUT_KEY_LEFT:
         break;
      case GLUT_KEY_RIGHT:
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   GLubyte texture[8][8] = {  /* PT = Paletted Texture! */
      {  0,   0,   0,   0,   0,   0,   0,   0},
      {  0, 100, 100, 100,   0, 180, 180, 180},
      {  0, 100,   0, 100,   0,   0, 180,   0},
      {  0, 100,   0, 100,   0,   0, 180,   0},
      {  0, 100, 100, 100,   0,   0, 180,   0},
      {  0, 100,   0,   0,   0,   0, 180,   0},
      {  0, 100,   0,   0,   0,   0, 180,   0},
      {  0, 100, 255,   0,   0,   0, 180, 250},
   };

   GLubyte table[256][4];
   int i;

   if (!glutExtensionSupported("GL_EXT_paletted_texture")) {
      printf("Sorry, GL_EXT_paletted_texture not supported\n");
      exit(0);
   }

   /* put some wacky colors into the texture palette */
   for (i=0;i<256;i++) {
      table[i][0] = i;
      table[i][1] = 0;
      table[i][2] = 127 + i / 2;
      table[i][3] = 255;
   }

#ifdef GL_EXT_paletted_texture

#if defined(GL_EXT_shared_texture_palette) && defined(SHARED_PALETTE)
   printf("Using shared palette\n");
   glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */
   glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
#else
   glColorTableEXT(GL_TEXTURE_2D,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */
#endif

   glTexImage2D(GL_TEXTURE_2D,       /* target */
                0,                   /* level */
                GL_COLOR_INDEX8_EXT, /* internal format */
                8, 8,                /* width, height */
                0,                   /* border */
                GL_COLOR_INDEX,      /* texture format */
                GL_UNSIGNED_BYTE,    /* texture type */
                texture);            /* teh texture */
#endif

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glEnable(GL_TEXTURE_2D);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 400 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   glutCreateWindow(argv[0]);

   Init();

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
