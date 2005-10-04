/*
 * Test GL_EXT_framebuffer_object
 *
 * Brian Paul
 * 7 Feb 2005
 */


#define GL_GLEXT_PROTOTYPES
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

static int Width = 400, Height = 400;
static GLuint MyFB;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void
Display( void )
{
   GLubyte *buffer = malloc(Width * Height * 4);
   GLenum status;

   /* draw to user framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glReadBuffer(GL_COLOR_ATTACHMENT1_EXT);

   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      printf("Framebuffer incomplete!!!\n");
   }

   glClearColor(0.5, 0.5, 1.0, 0.0);
   glClear( GL_COLOR_BUFFER_BIT );

   glBegin(GL_POLYGON);
   glColor3f(1, 0, 0);
   glVertex2f(-1, -1);
   glColor3f(0, 1, 0);
   glVertex2f(1, -1);
   glColor3f(0, 0, 1);
   glVertex2f(0, 1);
   glEnd();

   /* read from user framebuffer */
   glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

   /* draw to window */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glWindowPos2iARB(0, 0);
   glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

   free(buffer);
   glutSwapBuffers();
}


static void
Reshape( int width, int height )
{
   float ar = (float) width / (float) height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
#if 0
   glFrustum( -ar, ar, -1.0, 1.0, 5.0, 25.0 );
#else
   glOrtho(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
#endif
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
   Width = width;
   Height = height;
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);
}


static void
Key( unsigned char key, int x, int y )
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


static void
Init( void )
{
   GLuint rb;
   GLint i;

   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      printf("GL_EXT_framebuffer_object not found!\n");
      /*exit(0);*/
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   glGenFramebuffersEXT(1, &MyFB);
   assert(MyFB);
   assert(!glIsFramebufferEXT(MyFB));
   glDeleteFramebuffersEXT(1, &MyFB);
   assert(!glIsFramebufferEXT(MyFB));
   /* Note, continue to use MyFB below */

   glGenRenderbuffersEXT(1, &rb);
   assert(rb);
   assert(!glIsRenderbufferEXT(rb));
   glDeleteRenderbuffersEXT(1, &rb);
   assert(!glIsRenderbufferEXT(rb));
   rb = 42; /* an arbitrary ID */

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   assert(glIsFramebufferEXT(MyFB));
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);
   assert(glIsRenderbufferEXT(rb));

   glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &i);
   assert(i == rb);

   glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &i);
   assert(i == MyFB);

   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                                GL_RENDERBUFFER_EXT, rb);

   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);

   {
      GLint r, g, b, a;
      glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_RED_SIZE_EXT, &r);
      glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_GREEN_SIZE_EXT, &g);
      glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_BLUE_SIZE_EXT, &b);
      glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_ALPHA_SIZE_EXT, &a);
      printf("renderbuffer RGBA sizes = %d %d %d %d\n", r, g, b, a);
   }

   CheckError(__LINE__);

   /* restore to default */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


int
main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
