/* $Id: paltex.c,v 1.2 1999/11/02 15:09:04 brianp Exp $ */

/*
 * Paletted texture demo.  Written by Brian Paul.
 * This program is in the public domain.
 */

/*
 * $Log: paltex.c,v $
 * Revision 1.2  1999/11/02 15:09:04  brianp
 * new texture image, cleaned-up code
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
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
   glTexCoord2f(0, 1);  glVertex2f(-1, -0.5);
   glTexCoord2f(1, 1);  glVertex2f( 1, -0.5);
   glTexCoord2f(1, 0);  glVertex2f( 1,  0.5);
   glTexCoord2f(0, 0);  glVertex2f(-1,  0.5);
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
   glTranslatef( 0.0, 0.0, -7.0 );
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
#define HEIGHT 8
#define WIDTH 32
   static char texture[HEIGHT][WIDTH] = {
         "                                ",
         "    MMM    EEEE   SSS    AAA    ",
         "   M M M  E      S   S  A   A   ",
         "   M M M  EEEE    SS    A   A   ",
         "   M M M  E         SS  AAAAA   ",
         "   M   M  E      S   S  A   A   ",
         "   M   M   EEEE   SSS   A   A   ",
         "                                "
      };
   GLubyte table[256][4];
   int i;

   if (!glutExtensionSupported("GL_EXT_paletted_texture")) {
      printf("Sorry, GL_EXT_paletted_texture not supported\n");
      exit(0);
   }

   /* load the color table for each texel-index */
   table[' '][0] = 50;
   table[' '][1] = 50;
   table[' '][2] = 50;
   table[' '][3] = 50;
   table['M'][0] = 255;
   table['M'][1] = 0;
   table['M'][2] = 0;
   table['M'][3] = 0;
   table['E'][0] = 0;
   table['E'][1] = 255;
   table['E'][2] = 0;
   table['E'][3] = 0;
   table['S'][0] = 40;
   table['S'][1] = 40;
   table['S'][2] = 255;
   table['S'][3] = 0;
   table['A'][0] = 255;
   table['A'][1] = 255;
   table['A'][2] = 0;
   table['A'][3] = 0;

#ifdef GL_EXT_paletted_texture

#if defined(GL_EXT_shared_texture_palette) && defined(USE_SHARED_PALETTE)
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
                WIDTH, HEIGHT,       /* width, height */
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
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
