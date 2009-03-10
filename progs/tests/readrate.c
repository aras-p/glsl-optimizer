/*
 * Test glReadPixels speed
 * Brian Paul
 * 9 April 2004
 *
 * Compile:
 * gcc readrate.c -L/usr/X11R6/lib -lglut -lGLU -lGL -lX11 -o readrate
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

/* Hack, to test drawing instead of reading */
#define DRAW 0

#define MAX_WIDTH 1280
#define MAX_HEIGHT 1024

#define NUM_WIDTHS 4
#define NUM_HEIGHTS 4
static const GLint Widths[] = {256, 512, 1024, 1280};
static const GLint Heights[] = {4, 32, 256, 512, 768, 1024};
static int WidthIndex = 1, HeightIndex = 3;
static GLubyte *Buffer = NULL;
static GLboolean Benchmark = GL_TRUE;

#define NUM_PBO 2

static GLuint PBObjects[4];

static GLboolean HavePBO = GL_FALSE;


struct format_type {
   const char *Name;
   GLuint Bytes;
   GLenum Format;
   GLenum Type;
};

static struct format_type Formats[] = {
   { "GL_RGB, GLubyte", 3, GL_RGB, GL_UNSIGNED_BYTE },
   { "GL_BGR, GLubyte", 3, GL_BGR, GL_UNSIGNED_BYTE },
   { "GL_RGBA, GLubyte", 4, GL_RGBA, GL_UNSIGNED_BYTE },
   { "GL_BGRA, GLubyte", 4, GL_BGRA, GL_UNSIGNED_BYTE },
   { "GL_ABGR, GLubyte", 4, GL_ABGR_EXT, GL_UNSIGNED_BYTE },
   { "GL_RGBA, GLuint_8_8_8_8", 4, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8 },
   { "GL_BGRA, GLuint_8_8_8_8", 4, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8 },
   { "GL_BGRA, GLuint_8_8_8_8_rev", 4, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV },
#ifdef GL_EXT_packed_depth_stencil
   { "GL_DEPTH_STENCIL_EXT, GLuint24+8", 4, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT },
#endif
   { "GL_DEPTH_COMPONENT, GLfloat", 4, GL_DEPTH_COMPONENT, GL_FLOAT },
   { "GL_DEPTH_COMPONENT, GLuint", 4, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT }
};

#define NUM_FORMATS (sizeof(Formats) / sizeof(struct format_type))


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
MeasureFormat(struct format_type *fmt, GLint width, GLint height, GLuint pbo)
{
   double t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   double t1;
   int j;

   for (j = 0; ; j++) {

      glBegin(GL_POINTS);
      glVertex2f(1,1);
      glEnd();

#if DRAW
      glWindowPos2iARB(0,0);
      glDrawPixels(width, height,
                   fmt->Format, fmt->Type, Buffer);
      glFinish();
#else
      if (pbo) {
         glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, PBObjects[j % NUM_PBO]);
         glReadPixels(0, 0, width, height,
                      fmt->Format, fmt->Type, 0);
      }
      else {
         glReadPixels(0, 0, width, height,
                      fmt->Format, fmt->Type, Buffer);
      }
#endif

      t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      if (t1 - t0 > 2.0) {
         GLdouble rate = width * height / (1024.0 * 1024.0) * j / (t1 - t0);
#if DRAW
         printf("%-32s  %.2f draws/sec %.2f MPixels/sec  %.2f MBytes/sec\n",
                fmt->Name, j / (t1-t0), rate, rate * fmt->Bytes);
#else
         printf("%-32s  %.2f reads/sec %.2f MPixels/sec  %.2f MBytes/sec\n",
                fmt->Name, j / (t1-t0), rate, rate * fmt->Bytes);
#endif
         break;
      }

      if (j == 0) {
         /* check for error */
         GLenum err = glGetError();
         if (err) {
            printf("GL Error 0x%x for %s\n", err, fmt->Name);
            return;
         }
      }
   }
}



static void
Draw(void)
{
   char str[1000];
   int width = Widths[WidthIndex];
   int height = Heights[HeightIndex];
   int y = MAX_HEIGHT - 50;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glWindowPos2iARB(10, y);
   sprintf(str, "ReadPixels size: %d x %d", width, height);
   PrintString(str);
   y -= 14;

   glWindowPos2iARB(10, y);
   PrintString("Press up/down/left/right to change image size.");
   y -= 14;

   glWindowPos2iARB(10, y);
   PrintString("Press 'b' to run benchmark test.");
   y -= 14;

   if (Benchmark) {
      glWindowPos2iARB(10, y);
      PrintString("Testing...");
   }

   glutSwapBuffers();

   if (Benchmark) {
      GLuint i, pbo;
#if DRAW
      printf("Draw size: Width=%d Height=%d\n", width, height);
#else
      printf("Read size: Width=%d Height=%d\n", width, height);
#endif
      for (pbo = 0; pbo <= HavePBO; pbo++) {
         printf("Pixel Buffer Object: %d\n", pbo);

         if (pbo == 0) {
            glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, 0);
         }

         for (i = 0; i < NUM_FORMATS; i++) {
            MeasureFormat(Formats + i, width, height, pbo);
         }
      }

      Benchmark = GL_FALSE;

      /* redraw window text */
      glutPostRedisplay();
   }

}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         Benchmark = 1;
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
         if (HeightIndex + 1 < NUM_WIDTHS)
            HeightIndex++;
         break;
      case GLUT_KEY_DOWN:
         if (HeightIndex > 0)
            HeightIndex--;
         break;
      case GLUT_KEY_LEFT:
         if (WidthIndex > 0)
            WidthIndex--;
         break;
      case GLUT_KEY_RIGHT:
         if (WidthIndex + 1 < NUM_HEIGHTS)
            WidthIndex++;
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   Buffer = malloc(MAX_WIDTH * MAX_HEIGHT * 4);
   assert(Buffer);
#if DRAW
   printf("glDrawPixels test report:\n");
#else
   printf("glReadPixels test report:\n");
#endif
   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION: %s\n", (char *) glGetString(GL_VERSION));

   if (glutExtensionSupported("GL_ARB_pixel_buffer_object")) {
      int i;
      HavePBO = 1;
      glGenBuffersARB(NUM_PBO, PBObjects);
      for (i = 0; i < NUM_PBO; i++) {
         glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, PBObjects[i]);
         glBufferDataARB(GL_PIXEL_PACK_BUFFER_EXT,
                         MAX_WIDTH * MAX_HEIGHT * 4, NULL, GL_STREAM_READ);
      }
   }
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(MAX_WIDTH, MAX_HEIGHT);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
