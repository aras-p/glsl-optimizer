/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file stencil_twoside.c
 * 
 * Simple test of GL_ATI_separate_stencil (or the OGL 2.0 equivalent) functionality.
 * Five squares (or six if stencil wrap is available) are drawn
 * with different stencil modes, but all should be rendered with the same
 * final color.
 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int use20syntax = 1;
static int Width = 650;
static int Height = 200;
static const GLfloat Near = 5.0, Far = 25.0;


static PFNGLSTENCILFUNCSEPARATEPROC stencil_func_separate = NULL;
static PFNGLSTENCILFUNCSEPARATEATIPROC stencil_func_separate_ati = NULL;
static PFNGLSTENCILOPSEPARATEPROC stencil_op_separate = NULL;

static void Display( void )
{
   GLint  max_stencil;
   GLint  stencil_bits;
   unsigned i;


   glGetIntegerv( GL_STENCIL_BITS, & stencil_bits );
   max_stencil = (1U << stencil_bits) - 1;
   printf( "Stencil bits = %u, maximum stencil value = 0x%08x\n",
	   stencil_bits, max_stencil );

   glClearStencil( 1 );
   glClearColor( 0.2, 0.2, 0.8, 0 );
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT 
	    | GL_STENCIL_BUFFER_BIT );


   glPushMatrix();

   /* This is the "reference" square.
    */

   glDisable(GL_STENCIL_TEST);
   glTranslatef(-7.0, 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   glEnable(GL_STENCIL_TEST);

   /* Draw the first two squares using incr for the affected face
    */

   /*************************************************************************
    * 2nd square
    */
   if (use20syntax) {
      stencil_func_separate(GL_FRONT, GL_ALWAYS, 0, ~0);
      stencil_func_separate(GL_BACK, GL_ALWAYS, 0, ~0);
   }
   else {
      stencil_func_separate_ati(GL_ALWAYS, GL_ALWAYS, 0, ~0);
   }
   stencil_op_separate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
   stencil_op_separate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR);

   glTranslatef(3.0, 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.9, 0.9, 0.9 );
   for ( i = 0 ; i < (max_stencil + 5) ; i++ ) {
      /* this should be front facing */
      glVertex2f(-1, -1);
      glVertex2f( 1, -1);
      glVertex2f( 1,  1);
      glVertex2f(-1,  1);
   }
   glEnd();

   /* stencil vals should be equal to max_stencil */
   glStencilFunc(GL_EQUAL, max_stencil, ~0);
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   /*************************************************************************
    * 3rd square
    */
   if (use20syntax) {
      stencil_func_separate(GL_FRONT, GL_ALWAYS, 0, ~0);
      stencil_func_separate(GL_BACK, GL_ALWAYS, 0, ~0);
   }
   else {
      stencil_func_separate_ati(GL_ALWAYS, GL_ALWAYS, 0, ~0);
   }
   stencil_op_separate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR);
   stencil_op_separate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR);

   glTranslatef(3.0, 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.9, 0.9, 0.9 );
   for ( i = 0 ; i < (max_stencil + 5) ; i++ ) {
      /* this should be back facing */
      glVertex2f(-1, -1);
      glVertex2f(-1,  1);
      glVertex2f( 1,  1);
      glVertex2f( 1, -1);
   }
   glEnd();

   /* stencil vals should be equal to max_stencil */
   glStencilFunc(GL_EQUAL, max_stencil, ~0);
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   /*************************************************************************
    * 4th square
    */
   if (use20syntax) {
      stencil_func_separate(GL_FRONT, GL_NEVER, 0, ~0);
      stencil_func_separate(GL_BACK, GL_ALWAYS, 0, ~0);
   }
   else {
      stencil_func_separate_ati(GL_NEVER, GL_ALWAYS, 0, ~0);
   }
   stencil_op_separate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR);
   stencil_op_separate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR);

   glTranslatef(3.0, 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.9, 0.9, 0.9 );
   for ( i = 0 ; i < (max_stencil + 5) ; i++ ) {
      /* this should be back facing */
      glVertex2f(-1, -1);
      glVertex2f(-1,  1);
      glVertex2f( 1,  1);
      glVertex2f( 1, -1);
      /* this should be front facing */
      glVertex2f(-1, -1);
      glVertex2f( 1, -1);
      glVertex2f( 1,  1);
      glVertex2f(-1,  1);
   }
   glEnd();

   /* stencil vals should be equal to max_stencil */
   glStencilFunc(GL_EQUAL, max_stencil, ~0);
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   /*************************************************************************
    * 5th square
    */
   if (use20syntax) {
      stencil_func_separate(GL_FRONT, GL_ALWAYS, 0, ~0);
      stencil_func_separate(GL_BACK, GL_ALWAYS, 0, ~0);
   }
   else {
      stencil_func_separate_ati(GL_ALWAYS, GL_ALWAYS, 0, ~0);
   }
   stencil_op_separate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
   stencil_op_separate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR);

   glTranslatef(3.0, 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.9, 0.9, 0.9 );
   for ( i = 0 ; i < (max_stencil + 5) ; i++ ) {
      /* this should be back facing */
      glVertex2f(-1, -1);
      glVertex2f(-1,  1);
      glVertex2f( 1,  1);
      glVertex2f( 1, -1);
      /* this should be front facing */
      glVertex2f(-1, -1);
      glVertex2f( 1, -1);
      glVertex2f( 1,  1);
      glVertex2f(-1,  1);
   }
   glEnd();

   glStencilFunc(GL_EQUAL, 1, ~0);
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   /*************************************************************************
    * 6th square
    */
   if (glutExtensionSupported("GL_EXT_stencil_wrap")) {
      if (use20syntax) {
         stencil_func_separate(GL_FRONT, GL_ALWAYS, 0, ~0);
         stencil_func_separate(GL_BACK, GL_ALWAYS, 0, ~0);
      }
      else {
         stencil_func_separate_ati(GL_ALWAYS, GL_ALWAYS, 0, ~0);
      }
      stencil_op_separate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
      stencil_op_separate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);

      glTranslatef(3.0, 0, 0);
      glBegin(GL_QUADS);
      glColor3f( 0.9, 0.9, 0.9 );
      for ( i = 0 ; i < (max_stencil + 5) ; i++ ) {
         /* this should be back facing */
         glVertex2f(-1, -1);
         glVertex2f(-1,  1);
         glVertex2f( 1,  1);
         glVertex2f( 1, -1);
         /* this should be front facing */
         glVertex2f(-1, -1);
         glVertex2f( 1, -1);
         glVertex2f( 1,  1);
         glVertex2f(-1,  1);
      }
      glEnd();

      glStencilFunc(GL_EQUAL, 260 - 255, ~0);
      glBegin(GL_QUADS);
      glColor3f( 0.5, 0.5, 0.5 );
      glVertex2f(-1, -1);
      glVertex2f( 1, -1);
      glVertex2f( 1,  1);
      glVertex2f(-1,  1);
      glEnd();
   }

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   GLfloat ar = (float) width / (float) height;
   Width = width;
   Height = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, Near, Far );
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


static void Init( void )
{
   const char * const ver_string = (const char * const)
       glGetString( GL_VERSION );

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   if ( !glutExtensionSupported("GL_ATI_separate_stencil") 
	&& (atof( ver_string ) < 2.0) ) {
      printf("Sorry, this program requires either GL_ATI_separate_stencil or OpenGL 2.0.\n");
      exit(1);
   }
   if (atof( ver_string ) < 2.0) {
      use20syntax = 0;
   }
   stencil_func_separate = (PFNGLSTENCILFUNCSEPARATEPROC) glutGetProcAddress( "glStencilFuncSeparate" );
   stencil_func_separate_ati = (PFNGLSTENCILFUNCSEPARATEATIPROC) glutGetProcAddress( "glStencilFuncSeparateATI" );
   stencil_op_separate = (PFNGLSTENCILOPSEPARATEPROC) glutGetProcAddress( "glStencilOpSeparate" );

   printf("\nAll 5 (or 6) squares should be the same color.\n");
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL );
   glutCreateWindow( "GL_ATI_separate_stencil test" );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
