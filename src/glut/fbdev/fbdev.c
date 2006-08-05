/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * Copyright (C) 1995-2006  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Library for glut using mesa fbdev driver
 *
 * Written by Sean D'Epagnier (c) 2006
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/kd.h>

#include <linux/fb.h>
#include <linux/vt.h>

#include <GL/gl.h>
#include <GL/glfbdev.h>
#include <GL/glut.h>

#include "internal.h"

#define FBMODES "/etc/fb.modes"


struct fb_fix_screeninfo FixedInfo;
struct fb_var_screeninfo VarInfo, OrigVarInfo;

static int DesiredDepth = 0;

int FrameBufferFD = -1;
unsigned char *FrameBuffer;
unsigned char *BackBuffer = NULL;
int DisplayMode;

struct GlutTimer *GlutTimers = NULL;

struct timeval StartTime;

/* per window data */
static GLFBDevContextPtr Context;
static GLFBDevBufferPtr Buffer;
static GLFBDevVisualPtr Visual;

int Redisplay;
int Visible;
int VisibleSwitch;
int Active;
/* we have to poll to see if we are visible
   on a framebuffer that is not active */
int VisiblePoll;
static int FramebufferIndex;

static int RequiredWidth;
static int RequiredHeight;
static int InitialWidthHint;
static int InitialHeightHint;

static int Initialized;

char exiterror[256];

/* test if the active console is attached to the same framebuffer */
void TestVisible(void) {
   struct fb_con2fbmap confb;
   struct vt_stat st;
   int ret;
   ioctl(ConsoleFD, VT_GETSTATE, &st);
   confb.console = st.v_active;

   ret = ioctl(FrameBufferFD, FBIOGET_CON2FBMAP, &confb);

   if(ret == -1 || confb.framebuffer == FramebufferIndex) {
      VisibleSwitch = 1;
      Visible = 0;
      VisiblePoll = 0;
   }
}

static void Cleanup(void)
{
   if(ConsoleFD != -1)
      RestoreVT();

   /* close mouse */
   CloseMouse();

   glFBDevMakeCurrent( NULL, NULL, NULL);

   glFBDevDestroyContext(Context);
   glFBDevDestroyBuffer(Buffer);
   glFBDevDestroyVisual(Visual);

   /* restore original variable screen info */
   if(FrameBufferFD != -1) {
      if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &OrigVarInfo))
	 fprintf(stderr, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
		 strerror(errno));

      munmap(FrameBuffer, FixedInfo.smem_len);
      close(FrameBufferFD);
   }

   /* free allocated back buffer */
   if(DisplayMode & GLUT_DOUBLE)
      free(BackBuffer);

   /* free menu items */
   FreeMenus();

   if(exiterror[0])
      fprintf(stderr, "[glfbdev glut] %s", exiterror);
}

static void CrashHandler(int sig)
{
   sprintf(exiterror, "Caught signal %d, cleaning up\n", sig);
   exit(0);
}

static void removeArgs(int *argcp, char **argv, int num)
{
   int i;
   for (i = 0; argv[i+num]; i++)
      argv[i] = argv[i+num];

   argv[i] = NULL;
   *argcp -= num;
}

#define REQPARAM(PARAM)  \
    if (i >= *argcp - 1) { \
	fprintf(stderr, PARAM" requires a parameter\n"); \
	exit(0); \
    }

void glutInit (int *argcp, char **argv)
{
   int i;
   int nomouse = 0;
   int nokeyboard = 0;
   int usestdin = 0;

   /* parse out args */
   for (i = 1; i < *argcp;) {
      if (!strcmp(argv[i], "-geometry")) {
	 REQPARAM("geometry");
	 if(sscanf(argv[i+1], "%dx%d", &RequiredWidth,
		   &RequiredHeight) != 2) {
	    fprintf(stderr,"Please specify geometry as widthxheight\n");
	    exit(0);
	 }
	 removeArgs(argcp, &argv[i], 2);
      } else
      if (!strcmp(argv[i], "-bpp")) {
	 REQPARAM("bpp");
	 if(sscanf(argv[i+1], "%d", &DesiredDepth) != 1) {
	    fprintf(stderr, "Please specify a parameter for bpp\n");
	    exit(0);
	 }
	 removeArgs(argcp, &argv[i], 2);
      } else 
      if (!strcmp(argv[i], "-vt")) {
	 REQPARAM("vt");
	 if(sscanf(argv[i+1], "%d", &CurrentVT) != 1) {
	    fprintf(stderr, "Please specify a parameter for vt\n");
	    exit(0);
	 }
	 removeArgs(argcp, &argv[i], 2);
      } else 
      if (!strcmp(argv[i], "-mousespeed")) {
	 REQPARAM("mousespeed");
	 if(sscanf(argv[i+1], "%lf", &MouseSpeed) != 1) {
	    fprintf(stderr, "Please specify a mouse speed, eg: 2.5\n");
	    exit(0);
	 }
	 removeArgs(argcp, &argv[i], 2);
      } else 
      if (!strcmp(argv[i], "-nomouse")) {
	 nomouse = 1;
	 removeArgs(argcp, &argv[i], 1);
      } else 
      if (!strcmp(argv[i], "-nokeyboard")) {
	    nokeyboard = 1;
	    removeArgs(argcp, &argv[i], 1);
	 } else 
      if (!strcmp(argv[i], "-stdin")) {
	 usestdin = 1;
	 removeArgs(argcp, &argv[i], 1);
      } else 
      if (!strcmp(argv[i], "-gpmmouse")) {
#ifdef HAVE_GPM
	 GpmMouse = 1;
#else
	 fprintf(stderr, "gpm support not compiled\n");
	 exit(0);
#endif
	 removeArgs(argcp, &argv[i], 1);
      } else 
      if (!strcmp(argv[i], "--")) {
	 removeArgs(argcp, &argv[i], 1);
	 break;
      } else 
	 i++;
   }

   gettimeofday(&StartTime, 0);
   atexit(Cleanup);

   signal(SIGSEGV, CrashHandler);
   signal(SIGINT, CrashHandler);
   signal(SIGTERM, CrashHandler);

   if(nomouse == 0)
      InitializeMouse();
   if(nokeyboard == 0)
      InitializeVT(usestdin);

   Initialized = 1;
}

void glutInitDisplayMode (unsigned int mode)
{
   DisplayMode = mode;
}

void glutInitWindowPosition (int x, int y)
{
}

void glutInitWindowSize (int width, int height)
{
   InitialWidthHint = width;
   InitialHeightHint = height;
}


static void ProcessTimers(void)
{
   if(GlutTimers && GlutTimers->time < glutGet(GLUT_ELAPSED_TIME)) {
      struct GlutTimer *timer = GlutTimers;
      timer->func(timer->value);
      GlutTimers = timer->next;
      free(timer);
   }
}

void glutMainLoop(void)
{
   if(ReshapeFunc)
      ReshapeFunc(VarInfo.xres, VarInfo.yres);

   if(!DisplayFunc) {
      sprintf(exiterror, "Fatal Error: No Display Function registered\n");
      exit(0);
   }   

   for(;;) {
      ProcessTimers();

      if(Active)
	 ReceiveInput();
      else
	 if(VisiblePoll)
	    TestVisible();

      if(IdleFunc)
	 IdleFunc();

      if(VisibleSwitch) {
	 VisibleSwitch = 0;
	 if(VisibilityFunc)
	    VisibilityFunc(Visible ? GLUT_VISIBLE : GLUT_NOT_VISIBLE);
      }

      if(Visible && Redisplay) {
	 Redisplay = 0;
	 if(MouseEnabled)
	    EraseCursor();
	 DisplayFunc();
	 if(!(DisplayMode & GLUT_DOUBLE)) {
	    if(ActiveMenu)
	       DrawMenus();
	    if(MouseEnabled)
	       DrawCursor();
	 }
      }
   }
}

static void ParseFBModes(void)
{
   char buf[1024];
   struct fb_var_screeninfo vi = VarInfo;

   FILE *fbmodes = fopen(FBMODES, "r");

   if(!fbmodes) {
      sprintf(exiterror, "Warning: could not open "
	      FBMODES" using current mode\n");
      return;
   }

   if(InitialWidthHint == 0 && InitialHeightHint == 0
      && RequiredWidth == 0)
      return; /* use current mode */

   while(fgets(buf, sizeof buf, fbmodes)) {
      char *c;
      int v;

      if(!(c = strstr(buf, "geometry")))
	 continue;
      v = sscanf(c, "geometry %d %d %d %d %d", &vi.xres, &vi.yres,
		 &vi.xres_virtual, &vi.yres_virtual, &vi.bits_per_pixel);
      if(v != 5)
	 continue;

      /* now we have to decide what is best */
      if(RequiredWidth) {
	 if(RequiredWidth != vi.xres || RequiredHeight != vi.yres)
	    continue;
      } else {
	 if(VarInfo.xres < vi.xres && VarInfo.xres < InitialWidthHint)
	    v++;
	 if(VarInfo.xres > vi.xres && vi.xres > InitialWidthHint)
	    v++;

	 if(VarInfo.yres < vi.yres && VarInfo.yres < InitialHeightHint)
	    v++;
	 if(VarInfo.yres > vi.yres && vi.yres > InitialHeightHint)
	    v++;

	 if(v < 7)
	    continue;
      }

      fgets(buf, sizeof buf, fbmodes);
      if(!(c = strstr(buf, "timings")))
	 continue;

      v = sscanf(c, "timings %d %d %d %d %d %d %d", &vi.pixclock,
		 &vi.left_margin, &vi.right_margin, &vi.upper_margin,
		 &vi.lower_margin, &vi.hsync_len, &vi.vsync_len);
      if(v != 7)
	 continue;

      VarInfo = vi; /* finally found a better mode */
      if(RequiredWidth) {
	 fclose(fbmodes);
	 return;
      }
   }

   fclose(fbmodes);

   if(RequiredWidth) {
      sprintf(exiterror, "No mode (%dx%d) found in "FBMODES"\n",
	      RequiredWidth, RequiredHeight);
      exit(0);
   }
}

/* ---------- Window Management ----------*/
int glutCreateWindow (const char *title)
{
   char *fbdev;
   int attribs[9], i, mask, size;

   if(Initialized == 0) {
      int argc = 0;
      char *argv[] = {NULL};
      glutInit(&argc, argv);
   }

   if(Context)
      return 0;

   fbdev = getenv("FRAMEBUFFER");
   if(fbdev) {
#ifdef MULTIHEAD
      if(!sscanf(fbdev, "/dev/fb%d", &FramebufferIndex))
	 if(!sscanf(fbdev, "/dev/fb/%d", &FramebufferIndex))
	    sprintf(exiterror, "Could not determine Framebuffer index!\n");
#endif
   } else {
      static char fb[128];
      struct fb_con2fbmap confb;
      int fd = open("/dev/fb0", O_RDWR);

      FramebufferIndex = 0;

      confb.console = CurrentVT;
      if(ioctl(fd, FBIOGET_CON2FBMAP, &confb) != -1)
	 FramebufferIndex = confb.framebuffer;
      sprintf(fb, "/dev/fb%d", FramebufferIndex);
      fbdev = fb;
      close(fd);
   }

   /* open the framebuffer device */
   FrameBufferFD = open(fbdev, O_RDWR);
   if (FrameBufferFD < 0) {
      sprintf(exiterror, "Error opening %s: %s\n", fbdev, strerror(errno));
      exit(0);
   }

   /* Get the fixed screen info */
   if (ioctl(FrameBufferFD, FBIOGET_FSCREENINFO, &FixedInfo)) {
      sprintf(exiterror, "error: ioctl(FBIOGET_FSCREENINFO) failed: %s\n",
	      strerror(errno));
      exit(0);
   }

   /* get the variable screen info */
   if (ioctl(FrameBufferFD, FBIOGET_VSCREENINFO, &OrigVarInfo)) {
      sprintf(exiterror, "error: ioctl(FBIOGET_VSCREENINFO) failed: %s\n",
	      strerror(errno));
      exit(0);
   }

   /* operate on a copy */
   VarInfo = OrigVarInfo;

   /* set the depth, resolution, etc */
   ParseFBModes();

   if(DisplayMode & GLUT_INDEX)
      VarInfo.bits_per_pixel = 8;
   else
      if(VarInfo.bits_per_pixel == 8)
	 VarInfo.bits_per_pixel = 32;
    
   if (DesiredDepth)
      VarInfo.bits_per_pixel = DesiredDepth;

   VarInfo.xoffset = 0;
   VarInfo.yoffset = 0;
   VarInfo.nonstd = 0;
   VarInfo.vmode &= ~FB_VMODE_YWRAP; /* turn off scrolling */

   /* set new variable screen info */
   if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &VarInfo)) {
      sprintf(exiterror, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
	      strerror(errno));
      exit(0);
   }

   /* reload the screen info to update offsets */
   if (ioctl(FrameBufferFD, FBIOGET_VSCREENINFO, &VarInfo)) {
      sprintf(exiterror, "error: ioctl(FBIOGET_VSCREENINFO) failed: %s\n",
	      strerror(errno));
      exit(0);
   }

   /* reload the fixed info to update color mode */
   if (ioctl(FrameBufferFD, FBIOGET_FSCREENINFO, &FixedInfo)) {
      sprintf(exiterror, "error: ioctl(FBIOGET_FSCREENINFO) failed: %s\n",
	      strerror(errno));
      exit(0);
   }

   if (DesiredDepth && DesiredDepth !=  VarInfo.bits_per_pixel) {
      sprintf(exiterror, "error: Could not set set %d bpp\n", DesiredDepth);
      exit(0);
   }

   if(DisplayMode & GLUT_INDEX && FixedInfo.visual == FB_VISUAL_DIRECTCOLOR) {
      sprintf(exiterror, "error: Could not set 8 bit color mode\n");
      exit(0);
   }

   /* initialize colormap */
   LoadColorMap();

   /* mmap the framebuffer into our address space */
   FrameBuffer = mmap(0, FixedInfo.smem_len, PROT_READ | PROT_WRITE, 
		      MAP_SHARED, FrameBufferFD, 0);
   if (FrameBuffer == MAP_FAILED) {
      sprintf(exiterror, "error: unable to mmap framebuffer: %s\n",
	      strerror(errno));
      exit(0);
   }

   mask = DisplayMode;
   for(i=0; i<8 && mask; i++) {
      if(mask & GLUT_DOUBLE) {
	 attribs[i] = GLFBDEV_DOUBLE_BUFFER;
	 mask &= ~GLUT_DOUBLE;
	 continue;
      }

      if(mask & GLUT_INDEX) {
	 attribs[i] = GLFBDEV_COLOR_INDEX;
	 mask &= ~GLUT_INDEX;
	 continue;
      }

      if(mask & GLUT_DEPTH) {
	 attribs[i] = GLFBDEV_DEPTH_SIZE;
	 attribs[++i] = DepthSize;
	 mask &= ~GLUT_DEPTH;
	 continue;
      }

      if(mask & GLUT_STENCIL) {
	 attribs[i] = GLFBDEV_STENCIL_SIZE;
	 attribs[++i] = StencilSize;
	 mask &= ~GLUT_STENCIL;
	 continue;
      }

      if(mask & GLUT_ACCUM) {
	 attribs[i] = GLFBDEV_ACCUM_SIZE;
	 attribs[++i] = AccumSize;
	 mask &= ~GLUT_ACCUM;
	 continue;
      }

      if(mask & GLUT_ALPHA)
	 if(!(DisplayMode & GLUT_INDEX)) {
	    mask &= ~GLUT_ALPHA;
	    i--;
	    continue;
	 }
       
      sprintf(exiterror, "Invalid mode from glutInitDisplayMode\n");
      exit(0);
   }       

   attribs[i] = GLFBDEV_NONE;
   
   if(!(Visual = glFBDevCreateVisual( &FixedInfo, &VarInfo, attribs ))) {
      sprintf(exiterror, "Failure to create Visual\n");
      exit(0);
   }

   size = VarInfo.xres_virtual * VarInfo.yres_virtual
      * VarInfo.bits_per_pixel / 8;
   if(DisplayMode & GLUT_DOUBLE) {
      if(!(BackBuffer = malloc(size))) {
	 sprintf(exiterror, "Failed to allocate double buffer\n");
	 exit(0);
      }
   } else
      BackBuffer = FrameBuffer;

   if(!(Buffer = glFBDevCreateBuffer( &FixedInfo, &VarInfo, Visual,
				      FrameBuffer, BackBuffer, size))) {
      sprintf(exiterror, "Failure to create Buffer\n");
      exit(0);
   }

   if(!(Context = glFBDevCreateContext(Visual, NULL))) {
      sprintf(exiterror, "Failure to create Context\n");
      exit(0);
   }

   if(!glFBDevMakeCurrent( Context, Buffer, Buffer )) {
      sprintf(exiterror, "Failure to Make Current\n");
      exit(0);
   }

   InitializeCursor();
   InitializeMenus();

   Visible = 1;
   VisibleSwitch = 1;
   Redisplay = 1;
   return 1;
}

int glutCreateSubWindow(int win, int x, int y, int width, int height)
{
   return 0;
}

void glutSetWindow(int win)
{
}

int glutGetWindow(void)
{
   return 1;
}

void glutDestroyWindow(int win)
{
}

void glutPostRedisplay(void)
{
   Redisplay = 1;
}

void glutPostWindowRedisplay(int win)
{
   Redisplay = 1;
}

void glutSwapBuffers(void)
{
   glFlush();

   if(Visible && DisplayMode & GLUT_DOUBLE) {
      if(ActiveMenu)
	 DrawMenus();
      if(MouseEnabled)
	 DrawCursor();
      glFBDevSwapBuffers(Buffer);
   }
}

void glutPositionWindow(int x, int y) 
{
}

void glutReshapeWindow(int width, int height)
{
}

void glutFullScreen(void)
{
}

void glutPopWindow(void)
{
}

void glutPushWindow(void)
{
}

void glutShowWindow(void)
{
}

void glutHideWindow(void)
{
}

static void UnIconifyWindow(int sig)
{
   if(ConsoleFD == 0)
      InitializeVT(1);
   else
      if(ConsoleFD > 0)
	 InitializeVT(0);
   if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &VarInfo)) {
      sprintf(exiterror, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
	      strerror(errno));
      exit(0);
   }
}

void glutIconifyWindow(void)
{
   RestoreVT();
   signal(SIGCONT, UnIconifyWindow);
   if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &OrigVarInfo))
      fprintf(stderr, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
	      strerror(errno));
   raise(SIGSTOP);
}

void glutSetWindowTitle(const char *name)
{
}

void glutSetIconTitle(const char *name)
{
}
