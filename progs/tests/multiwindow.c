
/*
 * A skeleton/template GLUT program
 *
 * Written by Brian Paul and in the public domain.
 */


/*
 * Revision 1.1  2001/08/21 14:25:31  brianp
 * simple multi-window GLUT test prog
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.2  1998/11/07 14:20:14  brianp
 * added simple rotation, animation of cube
 *
 * Revision 1.1  1998/11/07 14:14:37  brianp
 * Initial revision
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static GLint Window[2];

static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLboolean Anim = GL_TRUE;


static void Idle( void )
{
   Xrot += 3.0;
   Yrot += 4.0;
   Zrot += 2.0;

   glutSetWindow(Window[0]);
   glutPostRedisplay();
   glutSetWindow(Window[1]);
   glutPostRedisplay();
}


static void Display0( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   glColor3f(0, 1, 0);
   glutSolidCube(2.0);

   glPopMatrix();

   glutSwapBuffers();
}


static void Display1( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   glShadeModel(GL_FLAT);

   glBegin(GL_TRIANGLE_STRIP);
   glColor3f(1, 0, 0);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glColor3f(1, 0, 0);
   glVertex2f( -1, 1);
   glColor3f(0, 0, 1);
   glVertex2f( 1, 1);
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
   const GLfloat step = 3.0;
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
      case 'z':
         Zrot -= step;
         break;
      case 'Z':
         Zrot += step;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}




int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );

   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 400 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Window[0] = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display0 );
   glutIdleFunc(Idle);
   printf("GL_RENDERER[0] = %s\n", (char *) glGetString(GL_RENDERER));

   glutInitWindowPosition( 500, 0 );
   glutInitWindowSize( 400, 400 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Window[1] = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display1 );
   glutIdleFunc(Idle);
   printf("GL_RENDERER[1] = %s\n", (char *) glGetString(GL_RENDERER));

   glutMainLoop();

   return 0;
}
