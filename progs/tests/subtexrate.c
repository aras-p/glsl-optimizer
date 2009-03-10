/*
 * Measure glTexSubImage and glCopyTexSubImage speed
 *
 * Brian Paul
 * 26 Jan 2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static GLint WinWidth = 1024, WinHeight = 512;
static GLint TexWidth = 512, TexHeight = 512;

static GLuint TexObj = 1;

static GLenum IntFormat = GL_RGBA8;
static GLenum ReadFormat = GL_RGBA; /* for glReadPixels */

static GLboolean DrawQuad = GL_TRUE;


/**
 * draw teapot image, size TexWidth by TexHeight
 */
static void
DrawTestImage(void)
{
   GLfloat ar;

   glViewport(0, 0, TexWidth, TexHeight);
   glScissor(0, 0, TexWidth, TexHeight);
   glEnable(GL_SCISSOR_TEST);

   glClearColor(0.5, 0.5, 0.5, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   ar = (float) TexWidth / TexHeight;

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
   glRotatef(45, 0, 1, 0);
   glutSolidTeapot(2.3);
   glPopMatrix();
   glFrontFace(GL_CCW);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_LIGHTING);

   glDisable(GL_SCISSOR_TEST);

   glViewport(0, 0, WinWidth, WinHeight);
   glFinish();
}


/**
 * Do glCopyTexSubImage2D call (update texture with framebuffer data)
 * If doSubRect is true, do the copy in four pieces instead of all at once.
 */
static void
DoCopyTex(GLboolean doSubRect)
{
   if (doSubRect) {
      /* copy in four parts */
      int w = TexWidth / 2, h = TexHeight / 2;
      int x0 = 0, y0 = 0;
      int x1 = w, y1 = h;
#if 1
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, x0, y0, w, h);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x1, y0, x1, y0, w, h);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x0, y1, x0, y1, w, h);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, x1, y1, w, h);
#else
      /* scramble */
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, x1, y1, w, h);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x1, y0, x0, y1, w, h);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x0, y1, x1, y0, w, h);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, x0, y0, w, h);
#endif
   }
   else {
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, TexWidth, TexHeight);
   }
}


/**
 * Do glTexSubImage2D (update texture w/ user data)
 * If doSubRect, do update in four pieces, else all at once.
 */
static void
SubTex(GLboolean doSubRect, const GLubyte *image)
{
   if (doSubRect) {
      /* four pieces */
      int w = TexWidth / 2, h = TexHeight / 2;
      int x0 = 0, y0 = 0;
      int x1 = w, y1 = h;
      glPixelStorei(GL_UNPACK_ROW_LENGTH, TexWidth);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glPixelStorei(GL_UNPACK_SKIP_ROWS, y0);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, x0);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, w, h,
                      ReadFormat, GL_UNSIGNED_BYTE, image);

      glPixelStorei(GL_UNPACK_SKIP_ROWS, y0);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, x1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y0, w, h,
                      ReadFormat, GL_UNSIGNED_BYTE, image);

      glPixelStorei(GL_UNPACK_SKIP_ROWS, y1);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, x0);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y1, w, h,
                      ReadFormat, GL_UNSIGNED_BYTE, image);

      glPixelStorei(GL_UNPACK_SKIP_ROWS, y1);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, x1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, w, h,
                      ReadFormat, GL_UNSIGNED_BYTE, image);
   }
   else {
      /* all at once */
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TexWidth, TexHeight,
                      ReadFormat, GL_UNSIGNED_BYTE, image);
   }
}


/**
 * Measure gl[Copy]TexSubImage rate.
 * This actually also includes time to render a quad and SwapBuffers.
 */
static void
RunTest(GLboolean copyTex, GLboolean doSubRect)
{
   double t0, t1;
   int iters = 0;
   float copyRate, mbRate;
   float rot = 0.0;
   int bpp, r, g, b, a;
   int w, h;
   GLubyte *image = NULL;

   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &r);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &g);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &b);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &a);
   bpp = (r + g + b + a) / 8;

   if (!copyTex) {
      /* read image from frame buffer */
      image = (GLubyte *) malloc(TexWidth * TexHeight * bpp);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, TexWidth, TexHeight,
                   ReadFormat, GL_UNSIGNED_BYTE, image);
   }

   glEnable(GL_TEXTURE_2D);
   glViewport(WinWidth / 2, 0, WinWidth / 2, WinHeight);

   t0 = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

   do {
      if (copyTex)
         /* Framebuffer -> Texture */
         DoCopyTex(doSubRect);
      else {
         /* Main Mem -> Texture */
         SubTex(doSubRect, image);
      }

      /* draw textured quad */
      if (DrawQuad) {
         glPushMatrix();
            glRotatef(rot, 0, 0, 1);
            glTranslatef(1, 0, 0);
            glBegin(GL_POLYGON);
               glTexCoord2f(0, 0);  glVertex2f(-1, -1);
               glTexCoord2f(1, 0);  glVertex2f( 1, -1);
               glTexCoord2f(1, 1);  glVertex2f( 1,  1);
               glTexCoord2f(0, 1);  glVertex2f(-1,  1);
            glEnd();
         glPopMatrix();
      }

      iters++;
      rot += 2.0;

      t1 = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
      if (DrawQuad) {
         glutSwapBuffers();
      }
   } while (t1 - t0 < 5.0);

   glDisable(GL_TEXTURE_2D);
   if (image)
      free(image);

   if (doSubRect) {
      w = TexWidth / 2;
      h = TexHeight / 2;
      iters *= 4;
   }
   else {
      w = TexWidth;
      h = TexHeight;
   }

   copyRate = iters / (t1 - t0);
   mbRate = w * h * bpp * copyRate / (1024 * 1024);

   if (copyTex)
      printf("glCopyTexSubImage: %d x %d, %d Bpp:\n", w, h, bpp);
   else
      printf("glTexSubImage: %d x %d, %d Bpp:\n", w, h, bpp);
   printf("   %d calls in %.2f = %.2f calls/sec, %.2f MB/s\n",
          iters, t1-t0, copyRate, mbRate);
}


static void
Draw(void)
{
   glClearColor(0.2, 0.2, 0.8, 0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   DrawTestImage();
   if (!DrawQuad) {
      glutSwapBuffers();
   }

   RunTest(GL_FALSE, GL_FALSE);
   RunTest(GL_FALSE, GL_TRUE);
   RunTest(GL_TRUE, GL_FALSE);
   RunTest(GL_TRUE, GL_TRUE);

   glutSwapBuffers();

   printf("exiting\n");
   exit(0);
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
Init(void)
{
   /* create initial, empty teximage */
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexImage2D(GL_TEXTURE_2D, 0, IntFormat, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}



static void
ParseArgs(int argc, char *argv[])
{
   int i;
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-nodraw") == 0)
         DrawQuad = GL_FALSE;
   }
}


int
main(int argc, char *argv[])
{
   GLint mode = GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH;
   glutInit(&argc, argv);

   ParseArgs(argc, argv);

   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(mode);
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);

   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   Init();

   glutMainLoop();
   return 0;
}
