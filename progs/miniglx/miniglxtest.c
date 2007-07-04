/*
 * Test the mini GLX interface.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>
#define USE_MINI_GLX 1
#if USE_MINI_GLX
#include <GL/miniglx.h>
#else
#include <GL/glx.h>
#endif

static GLXContext ctx;

static GLuint NumFrames = 100;
static GLuint NumDisplays = 1;
static GLboolean Texture = GL_FALSE;
static GLboolean SingleBuffer = GL_FALSE;
static GLboolean Sleeps = GL_TRUE;


static void
rect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   glBegin(GL_QUADS);
   glTexCoord2f(0, 0);  glColor3f(0, 0, 1);  glVertex2f(x1, y1);
   glTexCoord2f(1, 0);  glColor3f(1, 0, 0);  glVertex2f(x2, y1);
   glTexCoord2f(1, 1);  glColor3f(0, 1, 0);  glVertex2f(x2, y2);
   glTexCoord2f(0, 1);  glColor3f(0, 0, 0);  glVertex2f(x1, y2);
   glEnd();
}


static void
redraw(Display *dpy, Window w, int rot)
{
   GLfloat a;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
      glRotatef(rot, 0, 0, 1);
      glScalef(.5, .5, .5);
      for (a = 0.0; a < 360.0; a += 30.0) {
         glPushMatrix();
            glRotatef(a, 0, 0, 1);
            glRotatef(40, 1, 0, 0);
            glColor3f(a / 360.0, 1-a/360.0, 0);
            rect(0.3, -0.25, 1.5, 0.25);
         glPopMatrix();
      }
   glPopMatrix();

   if (SingleBuffer)
      glFlush();
   else 
      glXSwapBuffers(dpy, w); 
}


static Window
make_window(Display *dpy, unsigned int width, unsigned int height)
{
   int attrib_single[] = { GLX_RGBA,
                           GLX_RED_SIZE, 1,
                           GLX_GREEN_SIZE, 1,
                           GLX_BLUE_SIZE, 1,
                           GLX_DEPTH_SIZE, 1,
                           None };
   int attrib_double[] = { GLX_RGBA,
                           GLX_RED_SIZE, 1,
                           GLX_GREEN_SIZE, 1,
                           GLX_BLUE_SIZE, 1,
                           GLX_DEPTH_SIZE, 1,
                           GLX_DOUBLEBUFFER, 
                           None };
   int *attrib = SingleBuffer ? attrib_single : attrib_double;
   int scrnum = 0;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   XVisualInfo *visinfo;

   root = RootWindow(dpy, scrnum);

   if (!(visinfo = glXChooseVisual(dpy, scrnum, attrib))) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   if (!(ctx = glXCreateContext(dpy, visinfo, NULL, True))) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow(dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr);
   if (!win) {
      printf("Error: XCreateWindow failed\n");
      exit(1);
   }

   glXMakeCurrent(dpy, win, ctx);

   glViewport(0, 0, width, height);

   return win;
}


static void
event_loop(Display *dpy, Window win)
{
   int i;

   printf("Drawing %d frames\n", NumFrames);

   for (i = 0; i < NumFrames; i++) {
      redraw(dpy, win, -i*2);
      if (Sleeps) {
         usleep(20000);
      }
   }
}


static int
runtest(void)
{
   Display *dpy;
   Window win;

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      printf("Error: XOpenDisplay failed\n");
      return 1;
   }

   win = make_window(dpy, 800, 600);

   srand(getpid());

   /* init GL state */
   glClearColor(0.5, 0.5, 0.5, 1.0);
   glEnable(GL_DEPTH_TEST);
   if (Texture) {
      GLubyte image[16][16][4];
      GLint i, j;
      for (i = 0; i < 16; i++) {
         for (j = 0; j < 16; j++) {
            if (((i / 2) ^ (j / 2)) & 1) {
               image[i][j][0] = 255;
               image[i][j][1] = 255;
               image[i][j][2] = 255;
               image[i][j][3] = 255;
            }
            else {
               image[i][j][0] = 128;
               image[i][j][1] = 128;
               image[i][j][2] = 128;
               image[i][j][3] = 128;
            }
         }
      }
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glEnable(GL_TEXTURE_2D);
   }
   if (SingleBuffer) {
      glDrawBuffer(GL_FRONT); 
      glReadBuffer(GL_FRONT); 
   }
   else {
      glDrawBuffer(GL_BACK);
   }

   XMapWindow(dpy, win);

   /* wait for window to get mapped */
   {
      XEvent e;
      while (1) {
	 XNextEvent(dpy, &e);
	 if (e.type == MapNotify && e.xmap.window == win) {
	    break;
	 }
      }
   }

   event_loop(dpy, win);

   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);

   XCloseDisplay(dpy);

   return 0;
}


static void
usage(void)
{
   printf("Usage:\n");
   printf("  -f N   render N frames (default %d)\n", NumFrames);
   printf("  -d N   do N display cycles\n");
   printf("  -t     texturing\n");
   printf("  -s     single buffering\n");
   printf("  -n     no usleep() delay\n");
}


static void
parse_args(int argc, char *argv[])
{
   int i;
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-f") == 0) {
         NumFrames = atoi(argv[i + 1]);
         i++;
      }
      else if (strcmp(argv[i], "-d") == 0) {
         NumDisplays = atoi(argv[i + 1]);
         i++;
      }
      else if (strcmp(argv[i], "-n") == 0) {
         Sleeps = GL_FALSE;
      }
      else if (strcmp(argv[i], "-s") == 0) {
         SingleBuffer = GL_TRUE;
      }
      else if (strcmp(argv[i], "-t") == 0) {
         Texture = GL_TRUE;
      }
      else {
         usage();
         exit(1);
      }
   }
}


int
main(int argc, char *argv[])
{
   int i;

   parse_args(argc, argv);

   for (i = 0; i < NumDisplays; i++) {
      if (runtest() != 0)
	 break;
   }

   return 0;
}
