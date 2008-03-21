/*
 * Measure glCopyPixels speed
 *
 * Brian Paul
 * 26 Jan 2006
 */

#define GL_GLEXT_PROTOTYPES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

static GLint WinWidth = 1000, WinHeight = 800;
static GLint ImgWidth, ImgHeight;

static GLenum Buffer = GL_FRONT;
static GLenum AlphaTest = GL_FALSE;
static GLboolean UseBlit = GL_FALSE;

static PFNGLBLITFRAMEBUFFEREXTPROC glBlitFramebufferEXT_func = NULL;


/**
 * draw teapot in lower-left corner of window
 */
static void
DrawTestImage(void)
{
   GLfloat ar;

   ImgWidth = WinWidth / 3;
   ImgHeight = WinHeight / 3;

   glViewport(0, 0, ImgWidth, ImgHeight);
   glScissor(0, 0, ImgWidth, ImgHeight);
   glEnable(GL_SCISSOR_TEST);

   glClearColor(0.5, 0.5, 0.5, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   ar = (float) WinWidth / WinHeight;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glFrontFace(GL_CW);
   glPushMatrix();
   glRotatef(45, 1, 0, 0);
   glutSolidTeapot(2.0);
   glPopMatrix();
   glFrontFace(GL_CCW);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_LIGHTING);

   glDisable(GL_SCISSOR_TEST);

   glViewport(0, 0, WinWidth, WinHeight);
   glFinish();
}


static int
Rand(int max)
{
   return ((int) random()) % max;
}


static void
BlitOne(void)
{
   int x, y;

   do {
      x = Rand(WinWidth);
      y = Rand(WinHeight);
   } while (x <= ImgWidth && y <= ImgHeight);

#ifdef GL_EXT_framebuffer_blit
   if (UseBlit)
   {
      glBlitFramebufferEXT_func(0, 0, ImgWidth, ImgHeight,
                                x, y, x + ImgWidth, y + ImgHeight,
                                GL_COLOR_BUFFER_BIT, GL_LINEAR);
   }
   else
#endif
   {
      glWindowPos2iARB(x, y);
      glCopyPixels(0, 0, ImgWidth, ImgHeight, GL_COLOR);
   }
}


/**
 * Measure glCopyPixels rate
 */
static void
RunTest(void)
{
   double t1, t0 = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   int iters = 0;
   float copyRate, mbRate;
   int r, g, b, a, bpp;

   if (AlphaTest) {
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GREATER, 0.0);
   }

   glGetIntegerv(GL_RED_BITS, &r);
   glGetIntegerv(GL_GREEN_BITS, &g);
   glGetIntegerv(GL_BLUE_BITS, &b);
   glGetIntegerv(GL_ALPHA_BITS, &a);
   bpp = (r + g + b + a) / 8;

   do {
      BlitOne();

      if (Buffer == GL_FRONT)
         glFinish(); /* XXX to view progress */

      iters++;

      t1 = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   } while (t1 - t0 < 5.0);

   glDisable(GL_ALPHA_TEST);

   copyRate = iters / (t1 - t0);
   mbRate = ImgWidth * ImgHeight * bpp * copyRate / (1024 * 1024);

   printf("Image size: %d x %d, %d Bpp\n", ImgWidth, ImgHeight, bpp);
   printf("%d copies in %.2f = %.2f copies/sec, %.2f MB/s\n",
          iters, t1-t0, copyRate, mbRate);
}


static void
Draw(void)
{
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glClearColor(0.2, 0.2, 0.8, 0);
   glReadBuffer(Buffer);
   glDrawBuffer(Buffer);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   DrawTestImage();

   RunTest();

   if (Buffer == GL_FRONT)
      glFinish();
   else
      glutSwapBuffers();

#if 1
   printf("exiting\n");
   exit(0);
#endif
}


static void
Reshape(int width, int height)
{
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
   case 'b':
      BlitOne();
      break;
   case 27:
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
      case GLUT_KEY_UP:
         break;
      case GLUT_KEY_DOWN:
         break;
      case GLUT_KEY_LEFT:
         break;
      case GLUT_KEY_RIGHT:
         break;
   }
   glutPostRedisplay();
}


static void
ParseArgs(int argc, char *argv[])
{
   int i;
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-back") == 0)
         Buffer = GL_BACK;
      else if (strcmp(argv[i], "-alpha") == 0)
         AlphaTest = GL_TRUE;
      else if (strcmp(argv[i], "-blit") == 0)
         UseBlit = GL_TRUE;
   }
}


static void
Init(void)
{
   if (glutExtensionSupported("GL_EXT_framebuffer_blit")) {
      glBlitFramebufferEXT_func = (PFNGLBLITFRAMEBUFFEREXTPROC)
         glutGetProcAddress("glBlitFramebufferEXT");
   }
   else if (UseBlit) {
      printf("Warning: GL_EXT_framebuffer_blit not supported.\n");
      UseBlit = GL_FALSE;
   }
}


int
main(int argc, char *argv[])
{
   GLint mode = GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH;
   glutInit(&argc, argv);

   ParseArgs(argc, argv);
   if (AlphaTest)
      mode |= GLUT_ALPHA;

   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(mode);
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);

   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   printf("Draw Buffer: %s\n", (Buffer == GL_BACK) ? "Back" : "Front");
   Init();

   glutMainLoop();
   return 0;
}
