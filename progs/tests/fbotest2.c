/*
 * Test GL_EXT_framebuffer_object
 *
 * Brian Paul
 * 19 Mar 2006
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Win = 0;
static int Width = 400, Height = 400;
static GLuint MyFB, ColorRb, DepthRb;
static GLboolean Animate = GL_TRUE;
static GLfloat Rotation = 0.0;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("fbotest2: GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void
Display( void )
{
   GLboolean copyPix = GL_FALSE;
   GLboolean blitPix = GL_FALSE;
   GLenum status;

   CheckError(__LINE__);

   /* draw to user framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
   glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      printf("fbotest2: Error: Framebuffer is incomplete!!!\n");
   }

   CheckError(__LINE__);

   glClearColor(0.5, 0.5, 1.0, 0.0);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);

   glPushMatrix();
   glRotatef(30.0, 1, 0, 0);
   glRotatef(Rotation, 0, 1, 0);
   glutSolidTeapot(2.0);
   glPopMatrix();

   if (copyPix) {
      glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, MyFB);
      glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
      glDrawBuffer(GL_BACK);

      glDisable(GL_DEPTH_TEST);  /* in case window has depth buffer */

      glWindowPos2iARB(0, 0);
      glCopyPixels(0, 0, Width, Height, GL_COLOR);
   }
   else if (blitPix) {
      glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, MyFB);
      glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
      glDrawBuffer(GL_BACK);

      glDisable(GL_DEPTH_TEST);  /* in case window has depth buffer */

      glBlitFramebufferEXT(0, 0, Width, Height,
                           0, 0, Width, Height,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
   }
   else {
      GLubyte *buffer = malloc(Width * Height * 4);
      /* read from user framebuffer */
      glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

      /* draw to window */
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      glDisable(GL_DEPTH_TEST);  /* in case window has depth buffer */
      glWindowPos2iARB(0, 0);
      glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

      free(buffer);
   }

   glutSwapBuffers();
   CheckError(__LINE__);
}


static void
Reshape( int width, int height )
{
   float ar = (float) width / (float) height;

   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, 5.0, 25.0 );

   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );

   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, ColorRb);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, width, height);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthRb);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                            width, height);

   Width = width;
   Height = height;
}


static void
CleanUp(void)
{
   glDeleteFramebuffersEXT(1, &MyFB);
   glDeleteRenderbuffersEXT(1, &ColorRb);
   glDeleteRenderbuffersEXT(1, &DepthRb);
   assert(!glIsFramebufferEXT(MyFB));
   assert(!glIsRenderbufferEXT(ColorRb));
   assert(!glIsRenderbufferEXT(DepthRb));
   glutDestroyWindow(Win);
   exit(0);
}


static void
Idle(void)
{
   Rotation = glutGet(GLUT_ELAPSED_TIME) * 0.1;
   glutPostRedisplay();
}


static void
Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case 'a':
      Animate = !Animate;
      if (Animate)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 27:
      CleanUp();
      break;
   }
   glutPostRedisplay();
}


static void
Init( void )
{
   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      printf("fbotest2: GL_EXT_framebuffer_object not found!\n");
      exit(0);
   }
   printf("fbotest2: GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   glGenFramebuffersEXT(1, &MyFB);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   assert(glIsFramebufferEXT(MyFB));

   /* set color buffer */
   glGenRenderbuffersEXT(1, &ColorRb);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, ColorRb);
   assert(glIsRenderbufferEXT(ColorRb));
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                GL_RENDERBUFFER_EXT, ColorRb);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);

   /* setup depth buffer */
   glGenRenderbuffersEXT(1, &DepthRb);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthRb);
   assert(glIsRenderbufferEXT(DepthRb));
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                GL_RENDERBUFFER_EXT, DepthRb);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, Width, Height);

   CheckError(__LINE__);

   /* restore to default */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   CheckError(__LINE__);
}


int
main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   if (Animate)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
