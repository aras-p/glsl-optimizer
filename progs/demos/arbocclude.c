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
#include <GL/glew.h>
#include <GL/glut.h>

#define NUM_OCC 10

static GLboolean Anim = GL_TRUE;
static GLfloat Xpos[NUM_OCC], Ypos[NUM_OCC];
static GLfloat Sign[NUM_OCC];
static GLuint OccQuery[NUM_OCC];
static GLint Win = 0;


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
   int time = glutGet(GLUT_ELAPSED_TIME);
   float step;
   int i;

   if (lastTime == 0)
      lastTime = time;
   else if (time - lastTime < 20)  /* 50Hz update */
      return;

   for (i = 0; i < NUM_OCC; i++) {

      step = (time - lastTime) / 1000.0 * Sign[i];

      Xpos[i] += step;

      if (Xpos[i] > 2.5) {
         Xpos[i] = 2.5;
         Sign[i] = -1;
      }
      else if (Xpos[i] < -2.5) {
         Xpos[i] = -2.5;
         Sign[i] = +1;
      }

   }

   lastTime = time;

   glutPostRedisplay();
}


static void Display( void )
{
   int i;

   glClearColor(0.25, 0.25, 0.25, 0.0);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );

   /* draw the occluding polygons */
   glColor3f(0, 0.4, 0.6);
   glBegin(GL_QUADS);
   glVertex2f(-1.6, -2.5);
   glVertex2f(-0.4, -2.5);
   glVertex2f(-0.4,  2.5);
   glVertex2f(-1.6,  2.5);
   glVertex2f( 0.4, -2.5);
   glVertex2f( 1.6, -2.5);
   glVertex2f( 1.6,  2.5);
   glVertex2f( 0.4,  2.5);
   glEnd();


   glColorMask(0, 0, 0, 0);
   glDepthMask(GL_FALSE);

   /* draw the test polygons with occlusion testing */
   for (i = 0; i < NUM_OCC; i++) {
      glPushMatrix();
         glTranslatef(Xpos[i], Ypos[i], -0.5);
         glScalef(0.2, 0.2, 1.0);
         glRotatef(-90.0 * Xpos[i], 0, 0, 1);

         glBeginQueryARB(GL_SAMPLES_PASSED_ARB, OccQuery[i]);
         glBegin(GL_POLYGON);
         glVertex3f(-1, -1, 0);
         glVertex3f( 1, -1, 0);
         glVertex3f( 1,  1, 0);
         glVertex3f(-1,  1, 0);
         glEnd();
         glEndQueryARB(GL_SAMPLES_PASSED_ARB);

      glPopMatrix();
   }

   glColorMask(1, 1, 1, 1);
   glDepthMask(GL_TRUE);

   /* Draw the rectangles now.
    * Draw orange if result was ready
    * Draw red if result was not ready.
    */
   for (i = 0; i < NUM_OCC; i++) {
      GLuint passed;
      GLint ready;

      glGetQueryObjectivARB(OccQuery[i], GL_QUERY_RESULT_AVAILABLE_ARB, &ready);

      glGetQueryObjectuivARB(OccQuery[i], GL_QUERY_RESULT_ARB, &passed);

      if (!ready)
         glColor3f(1, 0, 0);
      else
         glColor3f(0.8, 0.5, 0);

      if (!ready || passed) {
         glPushMatrix();
            glTranslatef(Xpos[i], Ypos[i], -0.5);
            glScalef(0.2, 0.2, 1.0);
            glRotatef(-90.0 * Xpos[i], 0, 0, 1);

            glBegin(GL_POLYGON);
            glVertex3f(-1, -1, 0);
            glVertex3f( 1, -1, 0);
            glVertex3f( 1,  1, 0);
            glVertex3f(-1,  1, 0);
            glEnd();

         glPopMatrix();
      }

      {
         char s[10];
         glRasterPos3f(0.45, Ypos[i], 1.0);
         sprintf(s, "%4d", passed);
         PrintString(s);
      }
   }

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
         glutDestroyWindow(Win);
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
   int i;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_LEFT:
         for (i = 0; i < NUM_OCC; i++)
            Xpos[i] -= step;
         break;
      case GLUT_KEY_RIGHT:
         for (i = 0; i < NUM_OCC; i++)
            Xpos[i] += step;
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   const char *ext = (const char *) glGetString(GL_EXTENSIONS);
   GLint bits;
   int i;

   if (!strstr(ext, "GL_ARB_occlusion_query")) {
      printf("Sorry, this demo requires the GL_ARB_occlusion_query extension\n");
      exit(-1);
   }

   glGetQueryivARB(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &bits);
   if (!bits) {
      printf("Hmmm, GL_QUERY_COUNTER_BITS_ARB is zero!\n");
      exit(-1);
   }

   glGetIntegerv(GL_DEPTH_BITS, &bits);
   printf("Depthbits: %d\n", bits);

   glGenQueriesARB(NUM_OCC, OccQuery);

   glEnable(GL_DEPTH_TEST);

   for (i = 0; i < NUM_OCC; i++) {
      float t = (float) i / (NUM_OCC - 1);
      Xpos[i] = 2.5 * t;
      Ypos[i] = 4.0 * (t - 0.5);
      Sign[i] = 1.0;
   }

}


int main( int argc, char *argv[] )
{
   glutInitWindowSize( 400, 400 );
   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   if (Anim)
      glutIdleFunc(Idle);
   else
      glutIdleFunc(NULL);
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
