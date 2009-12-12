/**
 * Test glFramebufferBlit()
 * Brian Paul
 * 27 Oct 2009
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static int Win;
static int WinWidth = 1100, WinHeight = 600;

static int SrcWidth = 512, SrcHeight = 512;
static int DstWidth = 512, DstHeight = 512;

static GLuint SrcFB, DstFB;
static GLuint SrcTex, DstTex;

#if 0
static GLenum SrcTexTarget = GL_TEXTURE_2D, SrcTexFace = GL_TEXTURE_2D;
#else
static GLenum SrcTexTarget = GL_TEXTURE_CUBE_MAP, SrcTexFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
#endif

static GLenum DstTexTarget = GL_TEXTURE_2D, DstTexFace = GL_TEXTURE_2D;

static GLuint SrcTexLevel = 01, DstTexLevel = 0;


static void
Draw(void)
{
   GLboolean rp = GL_FALSE;
   GLubyte *buf;
   GLint srcWidth = SrcWidth >> SrcTexLevel;
   GLint srcHeight = SrcHeight >> SrcTexLevel;
   GLint dstWidth = DstWidth >> DstTexLevel;
   GLint dstHeight = DstHeight >> DstTexLevel;
   GLenum status;

   /* clear window */
   glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);


   /* clear src buf */
   glBindFramebufferEXT(GL_FRAMEBUFFER, SrcFB);
   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   assert(status == GL_FRAMEBUFFER_COMPLETE_EXT);
   glClearColor(0, 1, 0, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* clear dst buf */
   glBindFramebufferEXT(GL_FRAMEBUFFER, DstFB);
   status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   assert(status == GL_FRAMEBUFFER_COMPLETE_EXT);
   glClearColor(1, 0, 0, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* blit src -> dst */
   glBindFramebufferEXT(GL_READ_FRAMEBUFFER, SrcFB);
   glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, DstFB);
   glBlitFramebufferEXT(0, 0, srcWidth, srcHeight,
                        0, 0, dstWidth, dstHeight,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if 01
   /* read src results */
   buf = malloc(4 * srcWidth * srcHeight);
   memset(buf, 0x88, 4 * srcWidth * srcHeight);
   glBindFramebufferEXT(GL_FRAMEBUFFER, SrcFB);
   if (rp)
      glReadPixels(0, 0, srcWidth, srcHeight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   else {
      glBindTexture(SrcTexTarget, SrcTex);
      glGetTexImage(SrcTexFace, SrcTexLevel, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   }

   /* draw dst in window */
   glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
   glWindowPos2i(0, 0);
   glDrawPixels(srcWidth, srcHeight, GL_RGBA, GL_UNSIGNED_BYTE, buf);

   printf("Src Pix[0] = %d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
   free(buf);
#endif

   glFinish();

   /* read dst results */
   buf = malloc(4 * dstWidth * dstHeight);
   memset(buf, 0x88, 4 * dstWidth * dstHeight);
   glBindFramebufferEXT(GL_FRAMEBUFFER, DstFB);
   if (rp)
      glReadPixels(0, 0, dstWidth, dstHeight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   else {
      glBindTexture(DstTexTarget, DstTex);
      glGetTexImage(DstTexFace, DstTexLevel, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   }

   /* draw dst in window */
   glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
   glWindowPos2i(srcWidth + 2, 0);
   glDrawPixels(dstWidth, dstHeight, GL_RGBA, GL_UNSIGNED_BYTE, buf);

   printf("Dst Pix[0] = %d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
   free(buf);

   glFinish();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case 27:
      glutDestroyWindow(Win);
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   }
   glutPostRedisplay();
}


static void
InitFBOs(void)
{
   GLuint w, h, lvl;

   /* Src */
   glGenTextures(1, &SrcTex);
   glBindTexture(SrcTexTarget, SrcTex);
   w = SrcWidth;
   h = SrcHeight;
   lvl = 0;
   for (lvl = 0; ; lvl++) {
      if (SrcTexTarget == GL_TEXTURE_CUBE_MAP) {
         GLuint f;
         for (f = 0; f < 6; f++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, lvl, GL_RGBA8,
                         w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
         }
      }
      else {
         /* single face */
         glTexImage2D(SrcTexFace, lvl, GL_RGBA8, w, h, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      }
      if (w == 1 && h == 1)
         break;
      if (w > 1)
         w /= 2;
      if (h > 1)
         h /= 2;
   }

   glGenFramebuffersEXT(1, &SrcFB);
   glBindFramebufferEXT(GL_FRAMEBUFFER, SrcFB);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             SrcTexFace, SrcTex, SrcTexLevel);

   /* Dst */
   glGenTextures(1, &DstTex);
   glBindTexture(DstTexTarget, DstTex);
   w = DstWidth;
   h = DstHeight;
   lvl = 0;
   for (lvl = 0; ; lvl++) {
      glTexImage2D(DstTexFace, lvl, GL_RGBA8, w, h, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      if (w == 1 && h == 1)
         break;
      if (w > 1)
         w /= 2;
      if (h > 1)
         h /= 2;
   }

   glGenFramebuffersEXT(1, &DstFB);
   glBindFramebufferEXT(GL_FRAMEBUFFER, DstFB);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             DstTexFace, DstTex, DstTexLevel);
}


static void
Init(void)
{
   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      fprintf(stderr, "This test requires GL_EXT_framebuffer_object\n");
      exit(1);
   }

   if (!glutExtensionSupported("GL_EXT_framebuffer_blit")) {
      fprintf(stderr, "This test requires GL_EXT_framebuffer_blit,\n");
      exit(1);
   }

   InitFBOs();

   printf("Left rect = src FBO, Right rect = dst FBO.\n");
   printf("Both should be green.\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
