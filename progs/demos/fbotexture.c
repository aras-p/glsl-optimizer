/*
 * Test GL_EXT_framebuffer_object render-to-texture
 *
 * Draw a teapot into a texture image with stenciling.
 * Then draw a textured quad using that texture.
 *
 * Brian Paul
 * 18 Apr 2005
 */


#include <GL/glut.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "extfuncs.h"

/* For debug */
#define DEPTH 1
#define STENCIL 1
#define DRAW 1


static int Win = 0;
static int Width = 400, Height = 400;

#if 1
static GLenum TexTarget = GL_TEXTURE_2D;
static int TexWidth = 512, TexHeight = 512;
static GLenum TexIntFormat = GL_RGB; /* either GL_RGB or GL_RGBA */
#else
static GLenum TexTarget = GL_TEXTURE_RECTANGLE_ARB;
static int TexWidth = 200, TexHeight = 200;
static GLenum TexIntFormat = GL_RGB5; /* either GL_RGB or GL_RGBA */
#endif
static GLuint TextureLevel = 0;  /* which texture level to render to */

static GLuint MyFB;
static GLuint TexObj;
static GLuint DepthRB = 0, StencilRB = 0;
static GLboolean Anim = GL_FALSE;
static GLfloat Rot = 0.0;
static GLboolean UsePackedDepthStencil = GL_FALSE;
static GLboolean UsePackedDepthStencilBoth = GL_FALSE;
static GLboolean Use_ARB_fbo = GL_FALSE;
static GLboolean Cull = GL_FALSE;
static GLboolean Wireframe = GL_FALSE;


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
   glBindFramebuffer_func(GL_FRAMEBUFFER_EXT, MyFB);

   status = glCheckFramebufferStatus_func(GL_FRAMEBUFFER_EXT);
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

   if (Wireframe) {
      glPolygonMode(GL_FRONT, GL_LINE);
   }
   else {
      glPolygonMode(GL_FRONT, GL_FILL);
   }

   if (Cull) {
      /* cull back */
      glCullFace(GL_BACK);
      glEnable(GL_CULL_FACE);
   }
   else {
      glDisable(GL_CULL_FACE);
   }

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
   glFrontFace(GL_CW); /* Teapot patches backward */
   glutSolidTeapot(0.5);
   glFrontFace(GL_CCW);
   glPopMatrix();
   glDisable(GL_LIGHTING);
   /*
   PrintStencilHistogram(TexWidth, TexHeight);
   */
#endif

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_CULL_FACE);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#if DRAW
   /* Bind normal framebuffer */
   glBindFramebuffer_func(GL_FRAMEBUFFER_EXT, 0);
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
   glDeleteRenderbuffers_func(1, &DepthRB);
#endif
#if STENCIL
   glDeleteRenderbuffers_func(1, &StencilRB);
#endif
   glDeleteFramebuffers_func(1, &MyFB);

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
   case 'c':
      Cull = !Cull;
      break;
   case 'w':
      Wireframe = !Wireframe;
      break;
   case 's':
      Rot += 2.0;
      break;
   case 'S':
      Rot -= 2.0;
      break;
   case 27:
      CleanUp();
      break;
   }
   glutPostRedisplay();
}


/**
 * Attach depth and stencil renderbuffer(s) to the given framebuffer object.
 * \param tryDepthStencil  if true, try to use a combined depth+stencil buffer
 * \param bindDepthStencil  if true, and tryDepthStencil is true, bind with
 *                          the GL_DEPTH_STENCIL_ATTACHMENT target.
 * \return GL_TRUE for success, GL_FALSE for failure
 */
static GLboolean
AttachDepthAndStencilBuffers(GLuint fbo,
                             GLsizei width, GLsizei height,
                             GLboolean tryDepthStencil,
                             GLboolean bindDepthStencil,
                             GLuint *depthRbOut, GLuint *stencilRbOut)
{
   GLenum status;

   *depthRbOut = *stencilRbOut = 0;

   glBindFramebuffer_func(GL_FRAMEBUFFER_EXT, fbo);

   if (tryDepthStencil) {
      GLuint rb;

      glGenRenderbuffers_func(1, &rb);
      glBindRenderbuffer_func(GL_RENDERBUFFER_EXT, rb);
      glRenderbufferStorage_func(GL_RENDERBUFFER_EXT,
                               GL_DEPTH24_STENCIL8_EXT,
                               width, height);
      if (glGetError())
         return GL_FALSE;

      if (bindDepthStencil) {
         /* attach to both depth and stencil at once */
         glFramebufferRenderbuffer_func(GL_FRAMEBUFFER_EXT,
                                      GL_DEPTH_STENCIL_ATTACHMENT,
                                      GL_RENDERBUFFER_EXT, rb);
         if (glGetError())
            return GL_FALSE;
      }
      else {
         /* attach to depth attachment point */
         glFramebufferRenderbuffer_func(GL_FRAMEBUFFER_EXT,
                                      GL_DEPTH_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT, rb);
         if (glGetError())
            return GL_FALSE;

         /* and attach to stencil attachment point */
         glFramebufferRenderbuffer_func(GL_FRAMEBUFFER_EXT,
                                      GL_STENCIL_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT, rb);
         if (glGetError())
            return GL_FALSE;
      }

      status = glCheckFramebufferStatus_func(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
         return GL_FALSE;

      *depthRbOut = *stencilRbOut = rb;
      return GL_TRUE;
   }

   /* just depth renderbuffer */
   {
      GLuint rb;

      glGenRenderbuffers_func(1, &rb);
      glBindRenderbuffer_func(GL_RENDERBUFFER_EXT, rb);
      glRenderbufferStorage_func(GL_RENDERBUFFER_EXT,
                               GL_DEPTH_COMPONENT,
                               width, height);
      if (glGetError())
         return GL_FALSE;

      /* attach to depth attachment point */
      glFramebufferRenderbuffer_func(GL_FRAMEBUFFER_EXT,
                                   GL_DEPTH_ATTACHMENT_EXT,
                                   GL_RENDERBUFFER_EXT, rb);
      if (glGetError())
         return GL_FALSE;

      status = glCheckFramebufferStatus_func(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
         return GL_FALSE;

      *depthRbOut = rb;
   }

   /* just stencil renderbuffer */
   {
      GLuint rb;

      glGenRenderbuffers_func(1, &rb);
      glBindRenderbuffer_func(GL_RENDERBUFFER_EXT, rb);
      glRenderbufferStorage_func(GL_RENDERBUFFER_EXT,
                               GL_STENCIL_INDEX,
                               width, height);
      if (glGetError())
         return GL_FALSE;

      /* attach to depth attachment point */
      glFramebufferRenderbuffer_func(GL_FRAMEBUFFER_EXT,
                                   GL_STENCIL_ATTACHMENT_EXT,
                                   GL_RENDERBUFFER_EXT, rb);
      if (glGetError())
         return GL_FALSE;

      status = glCheckFramebufferStatus_func(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         glDeleteRenderbuffers_func(1, depthRbOut);
         *depthRbOut = 0;
         glDeleteRenderbuffers_func(1, &rb);
         return GL_FALSE;
      }

      *stencilRbOut = rb;
   }

   return GL_TRUE;
}


static void
ParseArgs(int argc, char *argv[])
{
   GLint i;
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-ds") == 0) {
         if (!glutExtensionSupported("GL_EXT_packed_depth_stencil")) {
            printf("GL_EXT_packed_depth_stencil not found!\n");
            exit(0);
         }
         UsePackedDepthStencil = GL_TRUE;
         printf("Using GL_EXT_packed_depth_stencil\n");
      }
      else if (strcmp(argv[i], "-ds2") == 0) {
         if (!glutExtensionSupported("GL_EXT_packed_depth_stencil")) {
            printf("GL_EXT_packed_depth_stencil not found!\n");
            exit(0);
         }
         if (!glutExtensionSupported("GL_ARB_framebuffer_object")) {
            printf("GL_ARB_framebuffer_object not found!\n");
            exit(0);
         }
         UsePackedDepthStencilBoth = GL_TRUE;
         printf("Using GL_EXT_packed_depth_stencil and GL_DEPTH_STENCIL attachment point\n");
      }
      else if (strcmp(argv[i], "-arb") == 0) {
         if (!glutExtensionSupported("GL_ARB_framebuffer_object")) {
            printf("Sorry, GL_ARB_framebuffer object not supported!\n");
         }
         else {
            Use_ARB_fbo = GL_TRUE;
         }
      }
      else {
         printf("Unknown option: %s\n", argv[i]);
      }
   }
}


static void
SetupFunctionPointers(void)
{
   GetExtensionFuncs();

   if (Use_ARB_fbo) {
      /* no-op: use the ARB functions as-is */
   }
   else {
      /* set the ARB-flavor function pointers to point to the EXT functions */
      glIsRenderbuffer_func = glIsRenderbufferEXT_func;
      glBindRenderbuffer_func = glBindRenderbufferEXT_func;
      glDeleteRenderbuffers_func = glDeleteRenderbuffersEXT_func;
      glGenRenderbuffers_func = glGenRenderbuffersEXT_func;
      glRenderbufferStorage_func = glRenderbufferStorageEXT_func;
      glGetRenderbufferParameteriv_func = glGetRenderbufferParameterivEXT_func;
      glIsFramebuffer_func = glIsFramebufferEXT_func;
      glBindFramebuffer_func = glBindFramebufferEXT_func;
      glDeleteFramebuffers_func = glDeleteFramebuffersEXT_func;
      glGenFramebuffers_func = glGenFramebuffersEXT_func;
      glCheckFramebufferStatus_func = glCheckFramebufferStatusEXT_func;
      glFramebufferTexture1D_func = glFramebufferTexture1DEXT_func;
      glFramebufferTexture2D_func = glFramebufferTexture2DEXT_func;
      glFramebufferTexture3D_func = glFramebufferTexture3DEXT_func;
      glFramebufferRenderbuffer_func = glFramebufferRenderbufferEXT_func;
      glGetFramebufferAttachmentParameteriv_func = glGetFramebufferAttachmentParameterivEXT_func;
      glGenerateMipmap_func = glGenerateMipmapEXT_func;
   }
}


/*
 * Make FBO to render into given texture.
 */
static GLuint
MakeFBO_RenderTexture(GLuint texObj)
{
   GLuint fb;
   GLint sizeFudge = 0;

   glGenFramebuffers_func(1, &fb);
   glBindFramebuffer_func(GL_FRAMEBUFFER_EXT, fb);
   /* Render color to texture */
   glFramebufferTexture2D_func(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             TexTarget, texObj, TextureLevel);

   if (Use_ARB_fbo) {
      /* use a smaller depth buffer to see what happens */
      sizeFudge = 90;
   }

   /* Setup depth and stencil buffers */
   {
      GLboolean b;
      b = AttachDepthAndStencilBuffers(fb,
                                       TexWidth - sizeFudge,
                                       TexHeight - sizeFudge,
                                       UsePackedDepthStencil,
                                       UsePackedDepthStencilBoth,
                                       &DepthRB, &StencilRB);
      if (!b) {
         /* try !UsePackedDepthStencil */
         b = AttachDepthAndStencilBuffers(fb,
                                          TexWidth - sizeFudge,
                                          TexHeight - sizeFudge,
                                          !UsePackedDepthStencil,
                                          UsePackedDepthStencilBoth,
                                          &DepthRB, &StencilRB);
      }
      if (!b) {
         printf("Unable to create/attach depth and stencil renderbuffers "
                " to FBO!\n");
         exit(1);
      }
   }

   /* queries */
   {
      GLint bits, w, h, name;

      glBindRenderbuffer_func(GL_RENDERBUFFER_EXT, DepthRB);
      glGetRenderbufferParameteriv_func(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_WIDTH_EXT, &w);
      glGetRenderbufferParameteriv_func(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_HEIGHT_EXT, &h);
      printf("Color/Texture size: %d x %d\n", TexWidth, TexHeight);
      printf("Depth buffer size: %d x %d\n", w, h);

      glGetRenderbufferParameteriv_func(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_DEPTH_SIZE_EXT, &bits);
      printf("Depth renderbuffer size = %d bits\n", bits);

      glBindRenderbuffer_func(GL_RENDERBUFFER_EXT, StencilRB);
      glGetRenderbufferParameteriv_func(GL_RENDERBUFFER_EXT,
                                      GL_RENDERBUFFER_STENCIL_SIZE_EXT, &bits);
      printf("Stencil renderbuffer size = %d bits\n", bits);

      glGetFramebufferAttachmentParameteriv_func(GL_FRAMEBUFFER_EXT,
                                      GL_COLOR_ATTACHMENT0,
                                      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,
                                      &name);
      printf("Render to texture name: %d\n", texObj);
      printf("Color attachment[0] name: %d\n", name);
      assert(texObj == name);

      glGetFramebufferAttachmentParameteriv_func(GL_FRAMEBUFFER_EXT,
                                      GL_STENCIL_ATTACHMENT,
                                      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,
                                      &name);
      printf("Stencil attachment name: %d\n", name);

      glGetFramebufferAttachmentParameteriv_func(GL_FRAMEBUFFER_EXT,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,
                                      &name);
      printf("Depth attachment name: %d\n", name);

   }
   /* bind the regular framebuffer */
   glBindFramebuffer_func(GL_FRAMEBUFFER_EXT, 0);

   return fb;
}


static void
Init(void)
{
   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      printf("GL_EXT_framebuffer_object not found!\n");
      exit(0);
   }

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   SetupFunctionPointers();

   /* lighting */
   {
      static const GLfloat mat[4] = { 1.0, 0.5, 0.5, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat);
   }

   /*
    * Make texture object/image (we'll render into this texture)
    */
   {
      glGenTextures(1, &TexObj);
      glBindTexture(TexTarget, TexObj);

      /* make two image levels */
      glTexImage2D(TexTarget, 0, TexIntFormat, TexWidth, TexHeight, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      if (TexTarget == GL_TEXTURE_2D) {
         glTexImage2D(TexTarget, 1, TexIntFormat, TexWidth/2, TexHeight/2, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, NULL);
         TexWidth = TexWidth >> TextureLevel;
         TexHeight = TexHeight >> TextureLevel;
         glTexParameteri(TexTarget, GL_TEXTURE_MAX_LEVEL, TextureLevel);
      }

      glTexParameteri(TexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(TexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(TexTarget, GL_TEXTURE_BASE_LEVEL, TextureLevel);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   }

   MyFB = MakeFBO_RenderTexture(TexObj);
}


static void
Usage(void)
{
   printf("Usage:\n");
   printf("  -ds  Use combined depth/stencil renderbuffer\n");
   printf("  -arb Try GL_ARB_framebuffer_object's mismatched buffer sizes\n");
   printf("  -ds2 Try GL_ARB_framebuffer_object's GL_DEPTH_STENCIL_ATTACHMENT\n");
   printf("Keys:\n");
   printf("  a    Toggle animation\n");
   printf("  s/s  Step/rotate\n");
   printf("  c    Toggle back-face culling\n");
   printf("  w    Toggle wireframe mode (front-face only)\n");
   printf("  Esc  Exit\n");
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

   ParseArgs(argc, argv);
   Init();
   Usage();

   glutMainLoop();
   return 0;
}
