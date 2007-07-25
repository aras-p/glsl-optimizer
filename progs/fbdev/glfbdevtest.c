/*
 * Test the GLFBDev interface.   Only tested with radeonfb driver!!!!
 *
 * Written by Brian Paul
 */


#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <GL/gl.h>
#include <GL/glfbdev.h>
#include <math.h>


/**
 * Choose one of these modes
 */
/*static const int XRes = 1280, YRes = 1024, Hz = 75;*/
/*static const int XRes = 1280, YRes = 1024, Hz = 70;*/
/*static const int XRes = 1280, YRes = 1024, Hz = 60;*/
static const int XRes = 1024, YRes = 768, Hz = 70;

static int DesiredDepth = 32;

static int NumFrames = 100;

static struct fb_fix_screeninfo FixedInfo;
static struct fb_var_screeninfo VarInfo, OrigVarInfo;
static int OriginalVT = -1;
static int ConsoleFD = -1;
static int FrameBufferFD = -1;
static caddr_t FrameBuffer = (caddr_t) -1;
static caddr_t MMIOAddress = (caddr_t) -1;


static void
print_fixed_info(const struct fb_fix_screeninfo *fixed, const char *s)
{
   static const char *visuals[] = {
      "MONO01", "MONO10", "TRUECOLOR", "PSEUDOCOLOR",
      "DIRECTCOLOR", "STATIC_PSEUDOCOLOR"
   };

   printf("%s info -----------------------\n", s);
   printf("id = %16s\n", fixed->id);
   printf("smem_start = 0x%lx\n", fixed->smem_start);
   printf("smem_len = %d (0x%x)\n", fixed->smem_len, fixed->smem_len);
   printf("type = 0x%x\n", fixed->type);
   printf("type_aux = 0x%x\n", fixed->type_aux);
   printf("visual = 0x%x (%s)\n", fixed->visual, visuals[fixed->visual]);
   printf("xpanstep = %d\n", fixed->xpanstep);
   printf("ypanstep = %d\n", fixed->ypanstep);
   printf("ywrapstep = %d\n", fixed->ywrapstep);
   printf("line_length = %d\n", fixed->line_length);
   printf("mmio_start = 0x%lx\n", fixed->mmio_start);
   printf("mmio_len = %d (0x%x)\n", fixed->mmio_len, fixed->mmio_len);
   printf("accel = 0x%x\n", fixed->accel);
}


static void
print_var_info(const struct fb_var_screeninfo *var, const char *s)
{
   printf("%s info -----------------------\n", s);
   printf("xres = %d\n", var->xres);
   printf("yres = %d\n", var->yres);
   printf("xres_virtual = %d\n", var->xres_virtual);
   printf("yres_virtual = %d\n", var->yres_virtual);
   printf("xoffset = %d\n", var->xoffset);
   printf("yoffset = %d\n", var->yoffset);
   printf("bits_per_pixel = %d\n", var->bits_per_pixel);
   printf("grayscale = %d\n", var->grayscale);

   printf("red.offset = %d  length = %d  msb_right = %d\n",
          var->red.offset, var->red.length, var->red.msb_right);
   printf("green.offset = %d  length = %d  msb_right = %d\n",
          var->green.offset, var->green.length, var->green.msb_right);
   printf("blue.offset = %d  length = %d  msb_right = %d\n",
          var->blue.offset, var->blue.length, var->blue.msb_right);
   printf("transp.offset = %d  length = %d  msb_right = %d\n",
          var->transp.offset, var->transp.length, var->transp.msb_right);

   printf("nonstd = %d\n", var->nonstd);
   printf("activate = %d\n", var->activate);
   printf("height = %d mm\n", var->height);
   printf("width = %d mm\n", var->width);
   printf("accel_flags = 0x%x\n", var->accel_flags);
   printf("pixclock = %d\n", var->pixclock);
   printf("left_margin = %d\n", var->left_margin);
   printf("right_margin = %d\n", var->right_margin);
   printf("upper_margin = %d\n", var->upper_margin);
   printf("lower_margin = %d\n", var->lower_margin);
   printf("hsync_len = %d\n", var->hsync_len);
   printf("vsync_len = %d\n", var->vsync_len);
   printf("sync = %d\n", var->sync);
   printf("vmode = %d\n", var->vmode);
}


static void
signal_handler(int signumber)
{
   signal(signumber, SIG_IGN); /* prevent recursion! */
   fprintf(stderr, "error: got signal %d (exiting)\n", signumber);
   exit(1);
}


static void
initialize_fbdev( void )
{
   char ttystr[1000];
   int fd, vtnumber, ttyfd;
   int sz;

   (void) sz;

   if (geteuid()) {
      fprintf(stderr, "error: you need to be root\n");
      exit(1);
   }

#if 1
   /* open the framebuffer device */
   FrameBufferFD = open("/dev/fb0", O_RDWR);
   if (FrameBufferFD < 0) {
      fprintf(stderr, "Error opening /dev/fb0: %s\n", strerror(errno));
      exit(1);
   }
#endif

   /* open /dev/tty0 and get the vt number */
   if ((fd = open("/dev/tty0", O_WRONLY, 0)) < 0) {
      fprintf(stderr, "error opening /dev/tty0\n");
      exit(1);
   }
   if (ioctl(fd, VT_OPENQRY, &vtnumber) < 0 || vtnumber < 0) {
      fprintf(stderr, "error: couldn't get a free vt\n");
      exit(1);
   }
   close(fd);

   /* open the console tty */
   sprintf(ttystr, "/dev/tty%d", vtnumber);  /* /dev/tty1-64 */
   ConsoleFD = open(ttystr, O_RDWR | O_NDELAY, 0);
   if (ConsoleFD < 0) {
      fprintf(stderr, "error couldn't open console fd\n");
      exit(1);
   }

   /* save current vt number */
   {
      struct vt_stat vts;
      if (ioctl(ConsoleFD, VT_GETSTATE, &vts) == 0)
         OriginalVT = vts.v_active;
   }

   /* disconnect from controlling tty */
   ttyfd = open("/dev/tty", O_RDWR);
   if (ttyfd >= 0) {
      ioctl(ttyfd, TIOCNOTTY, 0);
      close(ttyfd);
   }

   /* some magic to restore the vt when we exit */
   {
      struct vt_mode vt;
      if (ioctl(ConsoleFD, VT_ACTIVATE, vtnumber) != 0)
         printf("ioctl VT_ACTIVATE: %s\n", strerror(errno));
      if (ioctl(ConsoleFD, VT_WAITACTIVE, vtnumber) != 0)
         printf("ioctl VT_WAITACTIVE: %s\n", strerror(errno));

      if (ioctl(ConsoleFD, VT_GETMODE, &vt) < 0) {
         fprintf(stderr, "error: ioctl VT_GETMODE: %s\n", strerror(errno));
         exit(1);
      }

      vt.mode = VT_PROCESS;
      vt.relsig = SIGUSR1;
      vt.acqsig = SIGUSR1;
      if (ioctl(ConsoleFD, VT_SETMODE, &vt) < 0) {
         fprintf(stderr, "error: ioctl(VT_SETMODE) failed: %s\n",
                 strerror(errno));
         exit(1);
      }
   }

   /* go into graphics mode */
   if (ioctl(ConsoleFD, KDSETMODE, KD_GRAPHICS) < 0) {
      fprintf(stderr, "error: ioctl(KDSETMODE, KD_GRAPHICS) failed: %s\n",
              strerror(errno));
      exit(1);
   }


#if 0
   /* open the framebuffer device */
   FrameBufferFD = open("/dev/fb0", O_RDWR);
   if (FrameBufferFD < 0) {
      fprintf(stderr, "Error opening /dev/fb0: %s\n", strerror(errno));
      exit(1);
   }
#endif

   /* Get the fixed screen info */
   if (ioctl(FrameBufferFD, FBIOGET_FSCREENINFO, &FixedInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_FSCREENINFO) failed: %s\n",
              strerror(errno));
      exit(1);
   }

   print_fixed_info(&FixedInfo, "Fixed");


  /* get the variable screen info */
   if (ioctl(FrameBufferFD, FBIOGET_VSCREENINFO, &OrigVarInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_VSCREENINFO) failed: %s\n",
              strerror(errno));
      exit(1);
   }

   print_var_info(&OrigVarInfo, "Orig Var");

   /* operate on a copy */
   VarInfo = OrigVarInfo;

   /* set the depth, resolution, etc */
   if (DesiredDepth)
      VarInfo.bits_per_pixel = DesiredDepth;

   if (VarInfo.bits_per_pixel == 16) {
      VarInfo.red.offset = 11;
      VarInfo.green.offset = 5;
      VarInfo.blue.offset = 0;
      VarInfo.red.length = 5;
      VarInfo.green.length = 6;
      VarInfo.blue.length = 5;
      VarInfo.transp.offset = 0;
      VarInfo.transp.length = 0;
   }
   else if (VarInfo.bits_per_pixel == 32) {
      VarInfo.red.offset = 16;
      VarInfo.green.offset = 8;
      VarInfo.blue.offset = 0;
      VarInfo.transp.offset = 24;
      VarInfo.red.length = 8;
      VarInfo.green.length = 8;
      VarInfo.blue.length = 8;
      VarInfo.transp.length = 8;
   }

   /* timing values taken from /etc/fb.modes */
   if (XRes == 1280 && YRes == 1024) {
      VarInfo.xres_virtual = VarInfo.xres = XRes;
      VarInfo.yres_virtual = VarInfo.yres = YRes;
      if (Hz == 75) {
         VarInfo.pixclock = 7408;
         VarInfo.left_margin = 248;
         VarInfo.right_margin = 16;
         VarInfo.upper_margin = 38;
         VarInfo.lower_margin = 1;
         VarInfo.hsync_len = 144;
         VarInfo.vsync_len = 3;
      }
      else if (Hz == 70) {
         VarInfo.pixclock = 7937;
         VarInfo.left_margin = 216;
         VarInfo.right_margin = 80;
         VarInfo.upper_margin = 36;
         VarInfo.lower_margin = 1;
         VarInfo.hsync_len = 112;
         VarInfo.vsync_len = 5;
      }
      else if (Hz == 60) {
         VarInfo.pixclock = 9260;
         VarInfo.left_margin = 248;
         VarInfo.right_margin = 48;
         VarInfo.upper_margin = 38;
         VarInfo.lower_margin = 1;
         VarInfo.hsync_len = 112;
         VarInfo.vsync_len = 3;
      }
      else {
         fprintf(stderr, "invalid rate for 1280x1024\n");
         exit(1);
      }
   }
   else if (XRes == 1024 && YRes == 768 && Hz == 70) {
      VarInfo.xres_virtual = VarInfo.xres = XRes;
      VarInfo.yres_virtual = VarInfo.yres = YRes;
      if (Hz == 70) {
         VarInfo.pixclock = 13334;
         VarInfo.left_margin = 144;
         VarInfo.right_margin = 24;
         VarInfo.upper_margin = 29;
         VarInfo.lower_margin = 3;
         VarInfo.hsync_len = 136;
         VarInfo.vsync_len = 6;
      }
      else {
         fprintf(stderr, "invalid rate for 1024x768\n");
         exit(1);
      }
   }

   VarInfo.xoffset = 0;
   VarInfo.yoffset = 0;
   VarInfo.nonstd = 0;
   VarInfo.vmode &= ~FB_VMODE_YWRAP; /* turn off scrolling */

   /* set new variable screen info */
   if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &VarInfo)) {
      fprintf(stderr, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
              strerror(errno));
      exit(1);
   }

   print_var_info(&VarInfo, "New Var");

   if (FixedInfo.visual != FB_VISUAL_TRUECOLOR &&
       FixedInfo.visual != FB_VISUAL_DIRECTCOLOR) {
      fprintf(stderr, "non-TRUE/DIRECT-COLOR visuals (0x%x) not supported by this demo.\n", FixedInfo.visual);
      exit(1);
   }

   /* initialize colormap */
   if (FixedInfo.visual == FB_VISUAL_DIRECTCOLOR) {
      struct fb_cmap cmap;
      unsigned short red[256], green[256], blue[256];
      int i;

      /* we're assuming 256 entries here */
      printf("initializing directcolor colormap\n");
      cmap.start = 0;
      cmap.len = 256;
      cmap.red   = red;
      cmap.green = green;
      cmap.blue  = blue;
      cmap.transp = NULL;
      for (i = 0; i < cmap.len; i++) {
         red[i] = green[i] = blue[i] = (i << 8) | i;
      }
      if (ioctl(FrameBufferFD, FBIOPUTCMAP, (void *) &cmap) < 0) {
         fprintf(stderr, "ioctl(FBIOPUTCMAP) failed [%d]\n", i);
      }
   }

   /*
    * fbdev says the frame buffer is at offset zero, and the mmio region
    * is immediately after.
    */

   /* mmap the framebuffer into our address space */
   FrameBuffer = (caddr_t) mmap(0, /* start */
                                FixedInfo.smem_len, /* bytes */
                                PROT_READ | PROT_WRITE, /* prot */
                                MAP_SHARED, /* flags */
                                FrameBufferFD, /* fd */
                                0 /* offset */);
   if (FrameBuffer == (caddr_t) - 1) {
      fprintf(stderr, "error: unable to mmap framebuffer: %s\n",
              strerror(errno));
      exit(1);
   }
   printf("FrameBuffer = %p\n", FrameBuffer);

#if 1
   /* mmap the MMIO region into our address space */
   MMIOAddress = (caddr_t) mmap(0, /* start */
                                FixedInfo.mmio_len, /* bytes */
                                PROT_READ | PROT_WRITE, /* prot */
                                MAP_SHARED, /* flags */
                                FrameBufferFD, /* fd */
                                FixedInfo.smem_len /* offset */);
   if (MMIOAddress == (caddr_t) - 1) {
      fprintf(stderr, "error: unable to mmap mmio region: %s\n",
              strerror(errno));
   }
   printf("MMIOAddress = %p\n", MMIOAddress);

   /* try out some simple MMIO register reads */
   if (0)
   {
      typedef unsigned int CARD32;
      typedef unsigned char CARD8;
#define RADEON_CONFIG_MEMSIZE               0x00f8
#define RADEON_MEM_SDRAM_MODE_REG           0x0158
#define MMIO_IN32(base, offset) \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset))
#define INREG(addr)         MMIO_IN32(MMIOAddress, addr)
      int sz, type;
      const char *typeStr[] = {"SDR", "DDR", "64-bit SDR"};
      sz = INREG(RADEON_CONFIG_MEMSIZE);
      type = INREG(RADEON_MEM_SDRAM_MODE_REG);
      printf("RADEON_CONFIG_MEMSIZE = %d (%d MB)\n", sz, sz / 1024 / 1024);
      printf("RADEON_MEM_SDRAM_MODE_REG >> 30 = %d (%s)\n",
             type >> 30, typeStr[type>>30]);
   }
#endif

}


static void
shutdown_fbdev( void )
{
   struct vt_mode VT;

   printf("cleaning up...\n");
   /* restore original variable screen info */
   if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &OrigVarInfo)) {
      fprintf(stderr, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
              strerror(errno));
      exit(1);
   }

   munmap(MMIOAddress, FixedInfo.mmio_len);
   munmap(FrameBuffer, FixedInfo.smem_len);
   close(FrameBufferFD);

   /* restore text mode */
   ioctl(ConsoleFD, KDSETMODE, KD_TEXT);

   /* set vt */
   if (ioctl(ConsoleFD, VT_GETMODE, &VT) != -1) {
      VT.mode = VT_AUTO;
      ioctl(ConsoleFD, VT_SETMODE, &VT);
   }

   /* restore original vt */
   if (OriginalVT >= 0) {
      ioctl(ConsoleFD, VT_ACTIVATE, OriginalVT);
      OriginalVT = -1;
   }

   close(ConsoleFD);
}


/* Borrowed from GLUT */
static void
doughnut(GLfloat r, GLfloat R, GLint nsides, GLint rings)
{
  int i, j;
  GLfloat theta, phi, theta1;
  GLfloat cosTheta, sinTheta;
  GLfloat cosTheta1, sinTheta1;
  GLfloat ringDelta, sideDelta;

  ringDelta = 2.0 * M_PI / rings;
  sideDelta = 2.0 * M_PI / nsides;

  theta = 0.0;
  cosTheta = 1.0;
  sinTheta = 0.0;
  for (i = rings - 1; i >= 0; i--) {
    theta1 = theta + ringDelta;
    cosTheta1 = cos(theta1);
    sinTheta1 = sin(theta1);
    glBegin(GL_QUAD_STRIP);
    phi = 0.0;
    for (j = nsides; j >= 0; j--) {
      GLfloat cosPhi, sinPhi, dist;

      phi += sideDelta;
      cosPhi = cos(phi);
      sinPhi = sin(phi);
      dist = R + r * cosPhi;

      glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
      glVertex3f(cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
      glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
      glVertex3f(cosTheta * dist, -sinTheta * dist,  r * sinPhi);
    }
    glEnd();
    theta = theta1;
    cosTheta = cosTheta1;
    sinTheta = sinTheta1;
  }
}


static void
gltest( void )
{
   static const int attribs[] = {
      GLFBDEV_DOUBLE_BUFFER,
      GLFBDEV_DEPTH_SIZE, 16,
      GLFBDEV_NONE
   };
   GLFBDevContextPtr ctx;
   GLFBDevBufferPtr buf;
   GLFBDevVisualPtr vis;
   int bytes, r, g, b, a;
   float ang;
   int i;

   printf("GLFBDEV_VENDOR = %s\n", glFBDevGetString(GLFBDEV_VENDOR));
   printf("GLFBDEV_VERSION = %s\n", glFBDevGetString(GLFBDEV_VERSION));

   /* framebuffer size */
   bytes = VarInfo.xres_virtual * VarInfo.yres_virtual * VarInfo.bits_per_pixel / 8;

   vis = glFBDevCreateVisual( &FixedInfo, &VarInfo, attribs );
   assert(vis);

   buf = glFBDevCreateBuffer( &FixedInfo, &VarInfo, vis, FrameBuffer, NULL, bytes );
   assert(buf);

   ctx = glFBDevCreateContext( vis, NULL );
   assert(buf);

   b = glFBDevMakeCurrent( ctx, buf, buf );
   assert(b);

   /*printf("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));*/
   glGetIntegerv(GL_RED_BITS, &r);
   glGetIntegerv(GL_GREEN_BITS, &g);
   glGetIntegerv(GL_BLUE_BITS, &b);
   glGetIntegerv(GL_ALPHA_BITS, &a);
   printf("RED_BITS=%d GREEN_BITS=%d BLUE_BITS=%d ALPHA_BITS=%d\n",
          r, g, b, a);

   glClearColor(0.5, 0.5, 1.0, 0);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1, 1, -1, 1, 2, 30);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -15);
   glViewport(0, 0, VarInfo.xres_virtual, VarInfo.yres_virtual);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   printf("Drawing %d frames...\n", NumFrames);

   ang = 0.0;
   for (i = 0; i < NumFrames; i++) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glPushMatrix();
      glRotatef(ang, 1, 0, 0);
      doughnut(1, 3, 40, 20);
      glPopMatrix();
      glFBDevSwapBuffers(buf);
      ang += 15.0;
   }

   /* clean up */
   b = glFBDevMakeCurrent( NULL, NULL, NULL);
   assert(b);

   glFBDevDestroyContext(ctx);
   glFBDevDestroyBuffer(buf);
   glFBDevDestroyVisual(vis);
}


static void
parse_args(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-f") == 0) {
         NumFrames = atoi(argv[i+1]);
         i++;
      }
   }
}


int
main( int argc, char *argv[] )
{
   signal(SIGUSR1, signal_handler);  /* exit if someone tries a vt switch */
   signal(SIGSEGV, signal_handler);  /* catch segfaults */

   parse_args(argc, argv);

   printf("Setting mode to %d x %d @ %d Hz, %d bpp\n", XRes, YRes, Hz, DesiredDepth);
   initialize_fbdev();
   gltest();
   shutdown_fbdev();

   return 0;
}
