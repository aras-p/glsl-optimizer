
/*
 * Test SVGA/Mesa interface in 32K color mode.
 *
 * Compile with:  gcc vtest.c -I../include -L../lib -lMesaGL -lX11 -lXext
 *   -lvga -lm -o vtest
 *
 * This program is in the public domain.
 * Brian Paul, January 1996
 */



#include <vga.h>
#include "GL/svgamesa.h"
#include "GL/gl.h"


SVGAMesaContext vmc;



void setup( void )
{
   vga_init();

   vga_setmode(G800x600x32K);
/*   gl_setcontextvga(G800x600x32K);*/

   vmc = SVGAMesaCreateContext( GL_FALSE );  /* single buffered */
   SVGAMesaMakeCurrent( vmc );
}


void test( void )
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho( -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );
   glMatrixMode(GL_MODELVIEW);

   glClear( GL_COLOR_BUFFER_BIT );

   glBegin( GL_LINES );
   glColor3f( 1.0, 0.0, 0.0 );
   glVertex2f( -0.5, 0.5 );
   glVertex2f(  0.5, 0.5 );
   glColor3f( 0.0, 1.0, 0.0 );
   glVertex2f( -0.5, 0.25 );
   glVertex2f(  0.5, 0.25 );
   glColor3f( 0.0, 0.0, 1.0 );
   glVertex2f( -0.5, 0.0 );
   glVertex2f(  0.5, 0.0 );
   glEnd();

   glBegin( GL_POLYGON );
   glColor3f( 1.0, 0.0, 0.0 );
   glVertex2f( 0.0, 0.7 );
   glColor3f( 0.0, 1.0, 0.0 );
   glVertex2f( -0.5, -0.5 );
   glColor3f( 0.0, 0.0, 1.0 );
   glVertex2f(  0.5, -0.5 );
   glEnd();

   sleep(3);
}

void end( void )
{
   SVGAMesaDestroyContext( vmc );

   vga_setmode( TEXT );
}


int main( int argc, char *argv[] )
{
   setup();
   test();
   end();
   return 0;
}
