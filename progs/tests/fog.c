/*
 * Copyright 2005 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *    Brian Paul (fogcoord.c used as a skeleton)
 */

/*
 * Test to exercise fog modes and for comparison with GL_EXT_fog_coord.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Width = 600;
static int Height = 600;
static GLfloat Near = 0.0, Far = 1.0;
GLboolean has_fogcoord;

static void drawString( const char *string )
{
   glRasterPos2f(0, .5);
   while ( *string ) {
      glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_10, *string );
      string++;
   }
}

static void Display( void )
{
   GLint i, depthi;
   GLfloat fogcolor[4] = {1, 1, 1, 1};

   glEnable(GL_FOG);
   glFogfv(GL_FOG_COLOR, fogcolor);

   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   for (i = 0; i < 6; i++) {
      if (i >= 3 && !has_fogcoord)
	 break;

      glPushMatrix();
      for (depthi = 0; depthi < 5; depthi++) {
	 GLfloat depth = Near + (Far - Near) * depthi / 4;

	 switch (i % 3) {
	 case 0:
	    glFogi(GL_FOG_MODE, GL_LINEAR);
	    glFogf(GL_FOG_START, Near);
	    glFogf(GL_FOG_END, Far);
	    break;
	 case 1:
	    glFogi(GL_FOG_MODE, GL_EXP);
	    glFogf(GL_FOG_DENSITY, 2);
	    break;
	 case 2:
	    glFogi(GL_FOG_MODE, GL_EXP2);
	    glFogf(GL_FOG_DENSITY, 2);
	    break;
	 }

	 glColor4f(0, 0, 0, 0);
	 if (i < 3) {
	    if (has_fogcoord)
	       glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);

            glBegin(GL_POLYGON);
            glVertex3f(0, 0, depth);
            glVertex3f(1, 0, depth);
            glVertex3f(1, 1, depth);
            glVertex3f(0, 1, depth);
            glEnd();
	 } else {
	    glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
	    glFogCoordfEXT(depth);

            glBegin(GL_POLYGON);
            glVertex3f(0, 0, (Near + Far) / 2);
            glVertex3f(1, 0, (Near + Far) / 2);
            glVertex3f(1, 1, (Near + Far) / 2);
            glVertex3f(0, 1, (Near + Far) / 2);
            glEnd();
	 }
	 glTranslatef(1.5, 0, 0);
      }

      glTranslatef(.1, 0, 0);
      switch (i) {
      case 0:
	 drawString("GL_LINEAR");
         break;
      case 1:
	 drawString("GL_EXP");
         break;
      case 2:
	 drawString("GL_EXP2");
         break;
      case 3:
	 drawString("GL_FOGCOORD GL_LINEAR");
         break;
      case 4:
	 drawString("GL_FOGCOORD GL_EXP");
         break;
      case 5:
	 drawString("GL_FOGCOORD GL_EXP2");
         break;
      }

      glPopMatrix();
      glTranslatef(0, 1.5, 0);
   }
   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   Width = width;
   Height = height;
   glViewport( 0, 0, width, height );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0, 11, 9, 0, -Near, -Far );

   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef(.25, .25, 0);
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
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   /* setup lighting, etc */
   has_fogcoord = glutExtensionSupported("GL_EXT_fog_coord");
   if (!has_fogcoord) {
      printf("Some output of this program requires GL_EXT_fog_coord\n");
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
