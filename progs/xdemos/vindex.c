
/*
 * Test Linux 8-bit SVGA/Mesa color index mode
 *
 * Compile with:  gcc vindex.c -I../include -L../lib -lMesaGL -lX11 -lXext
 *   -lvga -lm -o vindex
 *
 * This program is in the public domain.
 * Brian Paul, January 1996
 */



#include <vga.h>
#include "GL/svgamesa.h"
#include "GL/gl.h"



static GLint width = 640, height = 480;



static void display( void )
{
   int i, j;
   int w, h;

   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, (GLfloat) width, 0.0, (GLfloat) height, -1.0, 1.0 );

   glClear( GL_COLOR_BUFFER_BIT );

   w = width / 16;
   h = height / 16;
   for (i=0;i<16;i++) {
      for (j=0;j<16;j++) {
         glIndexi( i*16+j );
         glRecti( i*w, j*h, i*w+w, j*h+h );
      }
   }
}



int main( int argc, char *argv[] )
{
   SVGAMesaContext vmc;
   int i;

   vga_init();
   vga_setmode( G640x480x256 );

   vmc = SVGAMesaCreateContext( GL_FALSE );
   SVGAMesaMakeCurrent( vmc );

   display();
   sleep(3);

   SVGAMesaDestroyContext( vmc );
   vga_setmode( TEXT );
   return 0;
}
