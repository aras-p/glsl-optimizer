/*
 * Test GL_EXT_framebuffer_object
 * Like fbotest2.c but use a texture for the Z buffer / renderbuffer.
 * Note: the Z texture is never resized so that limits what can be
 * rendered if the window is resized.
 *
 * This tests a bug reported by Christoph Bumiller on 1 Feb 2010
 * on mesa3d-dev.
 *
 * XXX this should be made into a piglit test.
 *
 * Brian Paul
 * 1 Feb 2010
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static int Win = 0;
static int Width = 400, Height = 400;
static GLuint Tex = 0;
static GLuint MyFB, ColorRb, DepthRb;
static GLboolean Animate = GL_FALSE;
static GLfloat Rotation = 0.0;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("fbotest3: GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void
Display( void )
{
   GLubyte *buffer = malloc(Width * Height * 4);
   GLenum status;

   CheckError(__LINE__);

   /* draw to user framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
   glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      printf("fbotest3: Error: Framebuffer is incomplete!!!\n");
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

   /* read from user framebuffer */
   glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

   /* draw to window */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glDisable(GL_DEPTH_TEST);  /* in case window has depth buffer */
   glWindowPos2iARB(0, 0);
   glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

   free(buffer);
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

   Width = width;
   Height = height;
}


static void
CleanUp(void)
{
   glDeleteFramebuffersEXT(1, &MyFB);
   glDeleteRenderbuffersEXT(1, &ColorRb);
   glDeleteRenderbuffersEXT(1, &DepthRb);
   glDeleteTextures(1, &Tex);
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
      printf("fbotest3: GL_EXT_framebuffer_object not found!\n");
      exit(0);
   }
   printf("fbotest3: GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* create initial tex obj as an RGBA texture */
   glGenTextures(1, &Tex);
   glBindTexture(GL_TEXTURE_2D, Tex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glEnable(GL_TEXTURE_2D);

   /* draw something to make sure the texture is used */
   glBegin(GL_POINTS);
   glVertex2f(0, 0);
   glEnd();

   /* done w/ texturing */
   glDisable(GL_TEXTURE_2D);

   /* Create my Framebuffer Object */
   glGenFramebuffersEXT(1, &MyFB);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   assert(glIsFramebufferEXT(MyFB));

   /* Setup color renderbuffer */
   glGenRenderbuffersEXT(1, &ColorRb);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, ColorRb);
   assert(glIsRenderbufferEXT(ColorRb));
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                GL_RENDERBUFFER_EXT, ColorRb);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);

   /* Setup depth renderbuffer (a texture) */
   glGenRenderbuffersEXT(1, &DepthRb);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthRb);
   assert(glIsRenderbufferEXT(DepthRb));
   /* replace RGBA texture with Z texture */
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Width, Height, 0,
                GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                             GL_TEXTURE_2D, Tex, 0);

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
