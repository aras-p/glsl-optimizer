/* $Id: osdemo.c,v 1.2 2000/01/15 06:11:33 rjfrank Exp $ */

/*
 * Demo of off-screen Mesa rendering
 *
 * See Mesa/include/GL/osmesa.h for documentation of the OSMesa functions.
 *
 * If you want to render BIG images you'll probably have to increase
 * MAX_WIDTH and MAX_HEIGHT in src/config.h.
 *
 * This program is in the public domain.
 *
 * Brian Paul
 *
 * PPM output provided by Joerg Schmalzl.
 * ASCII PPM output added by Brian Paul.
 */


/*
 * $Log: osdemo.c,v $
 * Revision 1.2  2000/01/15 06:11:33  rjfrank
 * Added test for the occlusion test code.
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include "GL/osmesa.h"
#include "GL/glut.h"



#define WIDTH 400
#define HEIGHT 400



static void render_image( void )
{
   GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
   GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
   GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
   GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
   GLfloat red_mat[]   = { 1.0, 0.2, 0.2, 1.0 };
   GLfloat green_mat[] = { 0.2, 1.0, 0.2, 1.0 };
   GLfloat blue_mat[]  = { 0.2, 0.2, 1.0, 1.0 };


   glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
   glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
   glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-2.5, 2.5, -2.5, 2.5, -10.0, 10.0);
   glMatrixMode(GL_MODELVIEW);

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef(20.0, 1.0, 0.0, 0.0);

   glPushMatrix();
   glTranslatef(-0.75, 0.5, 0.0); 
   glRotatef(90.0, 1.0, 0.0, 0.0);
   glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red_mat );
   glutSolidTorus(0.275, 0.85, 20, 20);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-0.75, -0.5, 0.0); 
   glRotatef(270.0, 1.0, 0.0, 0.0);
   glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green_mat );
   glutSolidCone(1.0, 2.0, 16, 1);
   glPopMatrix();

#ifdef OSMESA_OCCLUSION_TEST_RESULT_HP
   {
      GLboolean bRet;
      OSMesaGetBooleanv(OSMESA_OCCLUSION_TEST_RESULT_HP,&bRet);
      glDepthMask(GL_FALSE);
      glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

      glPushMatrix();
      glTranslatef(0.75, 0.0, -1.0); 
      glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue_mat );
      glutSolidSphere(1.0, 20, 20);
      glPopMatrix();

      OSMesaGetBooleanv(OSMESA_OCCLUSION_TEST_RESULT_HP,&bRet);
      printf("Occlusion test 1 (result should be 1): %d\n",bRet);

      glDepthMask(GL_TRUE);
      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
   }
#endif

   glPushMatrix();
   glTranslatef(0.75, 0.0, -1.0); 
   glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue_mat );
   glutSolidSphere(1.0, 20, 20);
   glPopMatrix();

#ifdef OSMESA_OCCLUSION_TEST_RESULT_HP
   {
      GLboolean bRet;

      OSMesaGetBooleanv(OSMESA_OCCLUSION_TEST_RESULT_HP,&bRet);
      glDepthMask(GL_FALSE);
      glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

      /* draw a sphere inside the previous sphere */
      glPushMatrix();
      glTranslatef(0.75, 0.0, -1.0); 
      glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue_mat );
      glutSolidSphere(0.5, 20, 20);
      glPopMatrix();

      OSMesaGetBooleanv(OSMESA_OCCLUSION_TEST_RESULT_HP,&bRet);
      printf("Occlusion test 2 (result should be 0): %d\n",bRet);

      glDepthMask(GL_TRUE);
      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
   }
#endif

   glPopMatrix();
}



int main( int argc, char *argv[] )
{
   OSMesaContext ctx;
   void *buffer;

   /* Create an RGBA-mode context */
   ctx = OSMesaCreateContext( GL_RGBA, NULL );

   /* Allocate the image buffer */
   buffer = malloc( WIDTH * HEIGHT * 4 );

   /* Bind the buffer to the context and make it current */
   OSMesaMakeCurrent( ctx, buffer, GL_UNSIGNED_BYTE, WIDTH, HEIGHT );

   render_image();

   if (argc>1) {
      /* write PPM file */
      FILE *f = fopen( argv[1], "w" );
      if (f) {
         int i, x, y;
         GLubyte *ptr = (GLubyte *) buffer;
#define BINARY 0
#if BINARY
         fprintf(f,"P6\n");
         fprintf(f,"# ppm-file created by %s\n",  argv[0]);
         fprintf(f,"%i %i\n", WIDTH,HEIGHT);
         fprintf(f,"255\n");
         fclose(f);
         f = fopen( argv[1], "ab" );  /* reopen in binary append mode */
         for (y=HEIGHT-1; y>=0; y--) {
            for (x=0; x<WIDTH; x++) {
               i = (y*WIDTH + x) * 4;
               fputc(ptr[i], f);   /* write red */
               fputc(ptr[i+1], f); /* write green */
               fputc(ptr[i+2], f); /* write blue */
            }
         }
#else /*ASCII*/
         int counter = 0;
         fprintf(f,"P3\n");
         fprintf(f,"# ascii ppm file created by %s\n", argv[0]);
         fprintf(f,"%i %i\n", WIDTH, HEIGHT);
         fprintf(f,"255\n");
         for (y=HEIGHT-1; y>=0; y--) {
            for (x=0; x<WIDTH; x++) {
               i = (y*WIDTH + x) * 4;
               fprintf(f, " %3d %3d %3d", ptr[i], ptr[i+1], ptr[i+2]);
               counter++;
               if (counter % 5 == 0)
                  fprintf(f, "\n");
            }
         }
#endif
         fclose(f);
      }
   }
   else {
      printf("Specify a filename if you want to make a ppm file\n");
   }

   printf("all done\n");

   /* free the image buffer */
   free( buffer );

   /* destroy the context */
   OSMesaDestroyContext( ctx );

   return 0;
}
