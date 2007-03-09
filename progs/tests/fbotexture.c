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
#include <GL/glut.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* For debug */
#define DEPTH 1
#define STENCIL 1
#define DRAW 1


static int Win = 0;
static int Width = 400, Height = 400;

static GLenum TexTarget = GL_TEXTURE_2D; /*GL_TEXTURE_RECTANGLE_ARB;*/
static int TexWidth = 512, TexHeight = 512;
/*static int TexWidth = 600, TexHeight = 600;*/

static GLuint MyFB;
static GLuint TexObj;
static GLuint DepthRB, StencilRB;
static GLboolean Anim = GL_FALSE;
static GLfloat Rot = 0.0;
static GLboolean UsePackedDepthStencil = GL_FALSE;
static GLuint TextureLevel = 1;  /* which texture level to render to */
static GLenum TexIntFormat = GL_RGB; /* either GL_RGB or GL_RGBA */


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
   Rot = glutGet(GLUT_ELAPSED_TIME) * 0.1;
   glutPostRedisplay();
}


static void
RenderTexture(void)
{
   GLenum status;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   /* draw to texture image */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);

   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      printf("Framebuffer incomplete!!!\n");
   }

   glViewport(0, 0, TexWidth, TexHeight);

   glClearColor(0.5, 0.5, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
   CheckError(__LINE__);

#if DEPTH
   glEnable(GL_DEPTH_TEST);
#endif

#if STENCIL
   glEnable(GL_STENCIL_TEST);
   glStencilFunc(GL_NEVER, 1, ~0);
   glStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);
#endif

   CheckError(__LINE__);

#if DEPTH || STENCIL
   /* draw diamond-shaped stencil pattern */
   glColor3f(0, 1, 0);
   glBegin(GL_POLYGON);
   glVertex2f(-0.2,  0.0);
   glVertex2f( 0.0, -0.2);
   glVertex2f( 0.2,  0.0);
   glVertex2f( 0.0,  0.2);
   glEnd();
#endif

   /* draw teapot where stencil != 1 */
#if STENCIL
   glStencilFunc(GL_NOTEQUAL, 1, ~0);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
#endif

   CheckError(__LINE__);

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
   /*
   PrintStencilHistogram(TexWidth, TexHeight);
   */
#endif

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);

#if DRAW
   /* Bind normal framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif

   CheckError(__LINE__);
}



static void
Display(void)
{
   float ar = (float) Width / (float) Height;

   RenderTexture();

   /* draw textured quad in the window */
#if DRAW
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
   glEnable(TexTarget);
   glBindTexture(TexTarget, TexObj);
   glBegin(GL_POLYGON);
   glColor3f(0.25, 0.25, 0.25);
   if (TexTarget == GL_TEXTURE_2D) {
      glTexCoord2f(0, 0);
      glVertex2f(-1, -1);
      glTexCoord2f(1, 0);
      glVertex2f(1, -1);
      glColor3f(1.0, 1.0, 1.0);
      glTexCoord2f(1, 1);
      glVertex2f(1, 1);
      glTexCoord2f(0, 1);
      glVertex2f(-1, 1);
   }
   else {
      assert(TexTarget == GL_TEXTURE_RECTANGLE_ARB);
      glTexCoord2f(0, 0);
      glVertex2f(-1, -1);
      glTexCoord2f(TexWidth, 0);
      glVertex2f(1, -1);
      glColor3f(1.0, 1.0, 1.0);
      glTexCoord2f(TexWidth, TexHeight);
      glVertex2f(1, 1);
      glTexCoord2f(0, TexHeight);
      glVertex2f(-1, 1);
   }
   glEnd();
   glPopMatrix();
   glDisable(TexTarget);
#endif

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
CleanUp(void)
{
#if DEPTH
   glDeleteRenderbuffersEXT(1, &DepthRB);
#endif
#if STENCIL
   if (!UsePackedDepthStencil)
      glDeleteRenderbuffersEXT(1, &StencilRB);
#endif
   glDeleteFramebuffersEXT(1, &MyFB);

   glDeleteTextures(1, &TexObj);

   glutDestroyWindow(Win);

   exit(0);
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
      case 's':
         Rot += 2.0;
         break;
      case 27:
         CleanUp();
         break;
   }
   glutPostRedisplay();
}


static void
Init(int argc, char *argv[])
{
   static const GLfloat mat[4] = { 1.0, 0.5, 0.5, 1.0 };
   GLint i;

   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      printf("GL_EXT_framebuffer_object not found!\n");
      exit(0);
   }

   if (argc > 1 && strcmp(argv[1], "-ds") == 0) {
      if (!glutExtensionSupported("GL_EXT_packed_depth_stencil")) {
         printf("GL_EXT_packed_depth_stencil not found!\n");
         exit(0);
      }
      UsePackedDepthStencil = GL_TRUE;
      printf("Using GL_EXT_packed_depth_stencil\n");
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

   /* Make texture object/image */
   glGenTextures(1, &TexObj);
   glBindTexture(TexTarget, TexObj);
   /* make two image levels */
   glTexImage2D(TexTarget, 0, TexIntFormat, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexImage2D(TexTarget, 1, TexIntFormat, TexWidth/2, TexHeight/2, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   TexWidth = TexWidth >> TextureLevel;
   TexHeight = TexHeight >> TextureLevel;

   glTexParameteri(TexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(TexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glTexParameteri(TexTarget, GL_TEXTURE_BASE_LEVEL, TextureLevel);
   glTexParameteri(TexTarget, GL_TEXTURE_MAX_LEVEL, TextureLevel);

   CheckError(__LINE__);

   /* Render color to texture */
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             TexTarget, TexObj, TextureLevel);


#if DEPTH
   /* make depth renderbuffer */
   glGenRenderbuffersEXT(1, &DepthRB);
   assert(DepthRB);
   assert(!glIsRenderbufferEXT(DepthRB));
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthRB);
   assert(glIsRenderbufferEXT(DepthRB));
   if (UsePackedDepthStencil)
      glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_STENCIL_EXT,
                               TexWidth, TexHeight);
   else
      glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                               TexWidth, TexHeight);
   CheckError(__LINE__);
   glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                   GL_RENDERBUFFER_DEPTH_SIZE_EXT, &i);
   CheckError(__LINE__);
   printf("Depth renderbuffer size = %d bits\n", i);
   assert(i > 0);

   /* attach DepthRB to MyFB */
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                GL_RENDERBUFFER_EXT, DepthRB);
#endif

   CheckError(__LINE__);

#if STENCIL
   if (UsePackedDepthStencil) {
      /* DepthRb is a combined depth/stencil renderbuffer */
      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                   GL_STENCIL_ATTACHMENT_EXT,
                                   GL_RENDERBUFFER_EXT, DepthRB);
   }
   else {
      /* make stencil renderbuffer */
      glGenRenderbuffersEXT(1, &StencilRB);
      assert(StencilRB);
      assert(!glIsRenderbufferEXT(StencilRB));
      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, StencilRB);
      assert(glIsRenderbufferEXT(StencilRB));
      glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX,
                               TexWidth, TexHeight);
      /* attach StencilRB to MyFB */
      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                   GL_STENCIL_ATTACHMENT_EXT,
                                   GL_RENDERBUFFER_EXT, StencilRB);
   }
   glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
                                   GL_RENDERBUFFER_STENCIL_SIZE_EXT, &i);
   CheckError(__LINE__);
   printf("Stencil renderbuffer size = %d bits\n", i);
   assert(i > 0);
#endif

   CheckError(__LINE__);

   /* bind regular framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);


   /* lighting */
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   if (Anim)
      glutIdleFunc(Idle);
   Init(argc, argv);
   glutMainLoop();
   return 0;
}
