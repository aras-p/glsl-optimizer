/*
 * Test GL_EXT_framebuffer_object render-to-texture
 *
 * Draw a teapot into a texture image with stenciling.
 * Then draw a textured quad using that texture.
 *
 * Brian Paul
 * 18 Apr 2005
 */


#define GL_GLEXT_PROTOTYPES
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

static int Width = 400, Height = 400;
static int TexWidth = 512, TexHeight = 512;
static GLuint MyFB;
static GLuint TexObj;
static GLuint DepthRB, StencilRB;
static GLboolean Anim = GL_FALSE;
static GLfloat Rot = 0.0;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void
Idle(void)
{
   Rot = glutGet(GLUT_ELAPSED_TIME) * 0.05;
   glutPostRedisplay();
}


static void
RenderTexture(void)
{
   GLint level = 0;
   GLenum status;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   /* draw to texture image */
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             GL_TEXTURE_2D, TexObj, level);

   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      printf("Framebuffer incomplete!!!\n");
   }

   glViewport(0, 0, TexWidth, TexHeight);

   glClearColor(0.5, 0.5, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_STENCIL_TEST);
   glStencilFunc(GL_NEVER, 1, ~0);
   glStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);

   /* draw diamond-shaped stencil pattern */
   glColor3f(0, 1, 0);
   glBegin(GL_POLYGON);
   glVertex2f(-0.2,  0.0);
   glVertex2f( 0.0, -0.2);
   glVertex2f( 0.2,  0.0);
   glVertex2f( 0.0,  0.2);
   glEnd();

   /* draw teapot where stencil != 1 */
   glStencilFunc(GL_NOTEQUAL, 1, ~0);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

#if 0
   glBegin(GL_POLYGON);
   glColor3f(1, 0, 0);
   glVertex2f(-1, -1);
   glColor3f(0, 1, 0);
   glVertex2f(1, -1);
   glColor3f(0, 0, 1);
   glVertex2f(0, 1);
   glEnd();
#else
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glPushMatrix();
   glRotatef(0.5 * Rot, 1.0, 0.0, 0.0);
   glutSolidTeapot(0.5);
   glPopMatrix();
   glDisable(GL_LIGHTING);
#endif
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);

   /* Bind normal framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

   CheckError(__LINE__);
}



static void
Display(void)
{
   float ar = (float) Width / (float) Height;

   RenderTexture();

   /* draw textured quad in the window */

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -7.0);

   glViewport(0, 0, Width, Height);

   glClearColor(0.25, 0.25, 0.25, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   glPushMatrix();
   glRotatef(Rot, 0, 1, 0);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glBegin(GL_POLYGON);
   glColor3f(0.25, 0.25, 0.25);
   glTexCoord2f(0, 0);
   glVertex2f(-1, -1);
   glTexCoord2f(1, 0);
   glVertex2f(1, -1);
   glColor3f(1.0, 1.0, 1.0);
   glTexCoord2f(1, 1);
   glVertex2f(1, 1);
   glTexCoord2f(0, 1);
   glVertex2f(-1, 1);
   glEnd();
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   
   glutSwapBuffers();
   CheckError(__LINE__);
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   Width = width;
   Height = height;
}


static void
Key(unsigned char key, int x, int y)
{
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
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   GLint i;

   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      printf("GL_EXT_framebuffer_object not found!\n");
      exit(0);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* gen framebuffer id, delete it, do some assertions, just for testing */
   glGenFramebuffersEXT(1, &MyFB);
   assert(MyFB);
   assert(!glIsFramebufferEXT(MyFB));
   glDeleteFramebuffersEXT(1, &MyFB);
   assert(!glIsFramebufferEXT(MyFB));
   /* Note, continue to use MyFB below */

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   assert(glIsFramebufferEXT(MyFB));
   glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &i);
   assert(i == MyFB);

   /* make depth renderbuffer */
   glGenRenderbuffersEXT(1, &DepthRB);
   assert(DepthRB);
   assert(!glIsRenderbufferEXT(DepthRB));
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthRB);
   assert(glIsRenderbufferEXT(DepthRB));
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                            TexWidth, TexHeight);
   glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                   GL_RENDERBUFFER_DEPTH_SIZE_EXT, &i);
   printf("Depth renderbuffer size = %d bits\n", i);
   assert(i > 0);

   /* make stencil renderbuffer */
   glGenRenderbuffersEXT(1, &StencilRB);
   assert(StencilRB);
   assert(!glIsRenderbufferEXT(StencilRB));
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, StencilRB);
   assert(glIsRenderbufferEXT(StencilRB));
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX,
                            TexWidth, TexHeight);
   glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                   GL_RENDERBUFFER_STENCIL_SIZE_EXT, &i);
   printf("Stencil renderbuffer size = %d bits\n", i);
   assert(i > 0);

   /* attach DepthRB to MyFB */
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                GL_RENDERBUFFER_EXT, DepthRB);

   /* attach StencilRB to MyFB */
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                GL_RENDERBUFFER_EXT, StencilRB);


   /* bind regular framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

   /* Make texture object/image */
   glGenTextures(1, &TexObj);
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   CheckError(__LINE__);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
