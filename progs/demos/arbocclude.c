/*
 * GL_ARB_occlusion_query demo
 *
 * Brian Paul
 * 12 June 2003
 *
 * Copyright (C) 2003  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

#define TEST_DISPLAY_LISTS 0

static GLboolean Anim = GL_TRUE;
static GLfloat Xpos = 0;
static GLuint OccQuery;



static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}



static void Idle(void)
{
   static int lastTime = 0;
   static int sign = +1;
   int time = glutGet(GLUT_ELAPSED_TIME);
   float step;

   if (lastTime == 0)
      lastTime = time;
   else if (time - lastTime < 20)  /* 50Hz update */
      return;

   step = (time - lastTime) / 1000.0 * sign;
   lastTime = time;

   Xpos += step;

   if (Xpos > 2.5) {
      Xpos = 2.5;
      sign = -1;
   }
   else if (Xpos < -2.5) {
      Xpos = -2.5;
      sign = +1;
   }
   glutPostRedisplay();
}


static void Display( void )
{
   GLuint passed;
   GLint ready;
   char s[100];

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );

   /* draw the occluding polygons */
   glColor3f(0, 0.6, 0.8);
   glBegin(GL_QUADS);
   glVertex2f(-1.6, -1.5);
   glVertex2f(-0.4, -1.5);
   glVertex2f(-0.4,  1.5);
   glVertex2f(-1.6,  1.5);

   glVertex2f( 0.4, -1.5);
   glVertex2f( 1.6, -1.5);
   glVertex2f( 1.6,  1.5);
   glVertex2f( 0.4,  1.5);
   glEnd();

   /* draw the test polygon with occlusion testing */
   glPushMatrix();
   glTranslatef(Xpos, 0, -0.5);
   glScalef(0.3, 0.3, 1.0);
   glRotatef(-90.0 * Xpos, 0, 0, 1);

#if defined(GL_ARB_occlusion_query)
#if TEST_DISPLAY_LISTS
   glNewList(10, GL_COMPILE);
   glBeginQueryARB(GL_SAMPLES_PASSED_ARB, OccQuery);
   glEndList();
   glCallList(10);
#else
   glBeginQueryARB(GL_SAMPLES_PASSED_ARB, OccQuery);
#endif

   glColorMask(0, 0, 0, 0);
   glDepthMask(GL_FALSE);

   glBegin(GL_POLYGON);
   glVertex3f(-1, -1, 0);
   glVertex3f( 1, -1, 0);
   glVertex3f( 1,  1, 0);
   glVertex3f(-1,  1, 0);
   glEnd();

#if TEST_DISPLAY_LISTS
   glNewList(11, GL_COMPILE);
   glEndQueryARB(GL_SAMPLES_PASSED_ARB);
   glEndList();
   glCallList(11);
#else
   glEndQueryARB(GL_SAMPLES_PASSED_ARB);
#endif

   do {
      /* do useful work here, if any */
      glGetQueryObjectivARB(OccQuery, GL_QUERY_RESULT_AVAILABLE_ARB, &ready);
   } while (!ready);
   glGetQueryObjectuivARB(OccQuery, GL_QUERY_RESULT_ARB, &passed);

   /* turn off occlusion testing */
   glColorMask(1, 1, 1, 1);
   glDepthMask(GL_TRUE);
#endif /* GL_ARB_occlusion_query */

   /* draw the orange rect, so we can see what's going on */
   glColor3f(0.8, 0.5, 0);
   glBegin(GL_POLYGON);
   glVertex3f(-1, -1, 0);
   glVertex3f( 1, -1, 0);
   glVertex3f( 1,  1, 0);
   glVertex3f(-1,  1, 0);
   glEnd();

   glPopMatrix();


   /* Print result message */
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();

   glColor3f(1, 1, 1);
#if defined(GL_ARB_occlusion_query)
   sprintf(s, " %4d Fragments Visible", passed);
   glRasterPos3f(-0.50, -0.7, 0);
   PrintString(s);
   if (!passed) {
      glRasterPos3f(-0.25, -0.8, 0);
      PrintString("Fully Occluded");
   }
#else
   glRasterPos3f(-0.25, -0.8, 0);
   PrintString("GL_ARB_occlusion_query not available at compile time");
#endif /* GL_ARB_occlusion_query */

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
      case ' ':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   const GLfloat step = 0.1;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_LEFT:
         Xpos -= step;
         break;
      case GLUT_KEY_RIGHT:
         Xpos += step;
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   const char *ext = (const char *) glGetString(GL_EXTENSIONS);
   GLint bits;

   if (!strstr(ext, "GL_ARB_occlusion_query")) {
      printf("Sorry, this demo requires the GL_ARB_occlusion_query extension\n");
      exit(-1);
   }

#if defined(GL_ARB_occlusion_query)
   glGetQueryivARB(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &bits);
   if (!bits) {
      printf("Hmmm, GL_QUERY_COUNTER_BITS_ARB is zero!\n");
      exit(-1);
   }
#endif /* GL_ARB_occlusion_query */

   glGetIntegerv(GL_DEPTH_BITS, &bits);
   printf("Depthbits: %d\n", bits);

#if defined(GL_ARB_occlusion_query)
   glGenQueriesARB(1, &OccQuery);
   assert(OccQuery > 0);
#endif /* GL_ARB_occlusion_query */

   glEnable(GL_DEPTH_TEST);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 400 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutIdleFunc( Idle );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
