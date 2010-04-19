
/*
 * Test multisampling and polygon smoothing.
 *
 * Brian Paul
 * 4 November 2002
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static GLfloat Zrot = 0;
static GLboolean Anim = GL_TRUE;
static GLboolean HaveMultisample = GL_TRUE;
static GLboolean DoMultisample = GL_TRUE;


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
doPolygon( GLenum mode, GLint verts, GLfloat radius, GLfloat z )
{
   int i;
   glBegin(mode);
   for (i = 0; i < verts; i++) {
      float a = (i * 2.0 * 3.14159) / verts;
      float x = radius * cos(a);
      float y = radius * sin(a);
      glVertex3f(x, y, z);
   }
   glEnd();
}


static void
DrawObject( void )
{
   glLineWidth(3.0);
   glColor3f(1, 1, 1);
   doPolygon(GL_LINE_LOOP, 12, 1.2, 0);

   glLineWidth(1.0);
   glColor3f(1, 1, 1);
   doPolygon(GL_LINE_LOOP, 12, 1.1, 0);

   glColor3f(1, 0, 0);
   doPolygon(GL_POLYGON, 12, 0.4, 0.3);

   glColor3f(0, 1, 0);
   doPolygon(GL_POLYGON, 12, 0.6, 0.2);

   glColor3f(0, 0, 1);
   doPolygon(GL_POLYGON, 12, 0.8, 0.1);

   glColor3f(1, 1, 1);
   doPolygon(GL_POLYGON, 12, 1.0, 0);
}


static void
Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glColor3f(1, 1, 1);

   glRasterPos2f(-3.1, -1.6);
   PrintString("No antialiasing");

   glRasterPos2f(-0.8, -1.6);
   if (HaveMultisample) {
      if (DoMultisample)
         PrintString("  MULTISAMPLE");
      else
         PrintString("MULTISAMPLE (off)");
   }
   else
      PrintString("MULTISAMPLE (N/A)");

   glRasterPos2f(1.6, -1.6);
   PrintString("GL_POLYGON_SMOOTH");

   /* non-aa */
   glEnable(GL_DEPTH_TEST);
   glPushMatrix();
   glTranslatef(-2.5, 0, 0);
   glPushMatrix();
   glRotatef(Zrot, 0, 0, 1);
   DrawObject();
   glPopMatrix();
   glPopMatrix();
   glDisable(GL_DEPTH_TEST);

   /* multisample */
   glEnable(GL_DEPTH_TEST);
   if (HaveMultisample && DoMultisample)
      glEnable(GL_MULTISAMPLE_ARB);
   glPushMatrix();
   glTranslatef(0, 0, 0);
   glPushMatrix();
   glRotatef(Zrot, 0, 0, 1);
   DrawObject();
   glPopMatrix();
   glPopMatrix();
   glDisable(GL_MULTISAMPLE_ARB);
   glDisable(GL_DEPTH_TEST);

   /* polygon smooth */
   glEnable(GL_POLYGON_SMOOTH);
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_BLEND);
   glPushMatrix();
   glTranslatef(2.5, 0, 0);
   glPushMatrix();
   glRotatef(Zrot, 0, 0, 1);
   DrawObject();
   glPopMatrix();
   glPopMatrix();
   glDisable(GL_LINE_SMOOTH);
   glDisable(GL_POLYGON_SMOOTH);
   glDisable(GL_BLEND);

   glutSwapBuffers();
}


static void
Reshape( int width, int height )
{
   GLfloat ar = (float) width / (float) height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(-2.0*ar, 2.0*ar, -2.0, 2.0, -1.0, 1.0);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void
Idle( void )
{
   Zrot = 0.01 * glutGet(GLUT_ELAPSED_TIME);
   glutPostRedisplay();
}


static void
Key( unsigned char key, int x, int y )
{
   const GLfloat step = 1.0;
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'm':
         DoMultisample = !DoMultisample;
         break;
      case 'z':
         Zrot = (int) (Zrot - step);
         break;
      case 'Z':
         Zrot = (int) (Zrot + step);
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init( void )
{
   /* GLUT imposes the four samples/pixel requirement */
   int s;
   glGetIntegerv(GL_SAMPLES_ARB, &s);
   if (!glutExtensionSupported("GL_ARB_multisample") || s < 1) {
      printf("Warning: multisample antialiasing not supported.\n");
      HaveMultisample = GL_FALSE;
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_SAMPLES_ARB = %d\n", s);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);

   glGetIntegerv(GL_MULTISAMPLE_ARB, &s);
   printf("GL_MULTISAMPLE_ARB = %d\n", s);
}


int
main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 600, 300 );
   glutInitDisplayMode( GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE |
                        GLUT_DEPTH | GLUT_MULTISAMPLE );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc( Idle );
   Init();
   glutMainLoop();
   return 0;
}
