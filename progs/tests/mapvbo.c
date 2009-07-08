/*
 * Test glMapBuffer() call immediately after glDrawArrays().
 * See details below.
 *
 * NOTE: Do not use freeglut with this test!  It calls the Display()
 * callback twice right away instead of just once.
 *
 * Brian Paul
 * 27 Feb 2009
 */


#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

static GLuint BufferID;


static GLuint Win;




/*
 * Create VBO (position and color) and load with data.
 */
static void
SetupBuffers(void)
{
   static const GLfloat data[] = {
      /* vertex */     /* color */
      0, -1,  0,      1,    1,   0,
      1,  0,  0,      1,    1,   0,
      0,  1,  0,      1,    1,   0,
      -1, 0,  0,      1,    1,   0
   };

   glGenBuffersARB(1, &BufferID);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, BufferID);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(data), data,
                   GL_STATIC_DRAW_ARB);
}


static void
Draw(void)
{
   static int count = 1;

   printf("Draw Frame %d\n", count);
   count++;

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, BufferID);
   glVertexPointer(3, GL_FLOAT, 24, 0);
   glEnableClientState(GL_VERTEX_ARRAY);

   glColorPointer(3, GL_FLOAT, 24, (void*) 12);
   glEnableClientState(GL_COLOR_ARRAY);

   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

   if (0)
      glFinish();

   /* Immediately map the color buffer and change something.
    * This should not effect the first glDrawArrays above, but the
    * next time we draw we should see a black vertex.
    */
   if (1) {
      GLfloat *m = (GLfloat *) glMapBufferARB(GL_ARRAY_BUFFER_ARB,
                                              GL_WRITE_ONLY_ARB);
      m[3] = m[4] = m[5] = 0.0f; /* black vertex */
      glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
   }
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );
   Draw();
   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   float ar = (float) width / (float) height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   if (key == 27) {
      glutDestroyWindow(Win);
      exit(0);
   }
   glutPostRedisplay();
}


static void Init( void )
{
   if (!glutExtensionSupported("GL_ARB_vertex_buffer_object")) {
      printf("GL_ARB_vertex_buffer_object not found!\n");
      exit(0);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   SetupBuffers();
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 300, 300 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
