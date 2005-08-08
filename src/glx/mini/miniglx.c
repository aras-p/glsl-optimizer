/**
 * \file miniglx.c
 * \brief Mini GLX interface functions.
 * \author Brian Paul
 *
 * The Mini GLX interface is a subset of the GLX interface, plus a
 * minimal set of Xlib functions.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.0.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \mainpage Mini GLX
 *
 * \section miniglxIntro Introduction
 *
 * The Mini GLX interface facilitates OpenGL rendering on embedded devices. The
 * interface is a subset of the GLX interface, plus a minimal set of Xlib-like
 * functions.
 *
 * Programs written to the Mini GLX specification should run unchanged
 * on systems with the X Window System and the GLX extension (after
 * recompilation). The intention is to allow flexibility for
 * prototyping and testing.
 *
 * The files in the src/miniglx/ directory are compiled to build the
 * libGL.so library.  This is the library which applications link with.
 * libGL.so in turn, loads the hardware-specific device driver.
 *
 *
 * \section miniglxDoxygen About Doxygen
 *
 * For a list of all files, select <b>File List</b>.  Choose a file from
 * the list for a list of all functions in the file.
 *
 * For a list of all functions, types, constants, etc.
 * select <b>File Members</b>.
 *
 *
 * \section miniglxReferences References
 *
 * - <A HREF="file:../../docs/MiniGLX.html">Mini GLX Specification</A>,
 *   Tungsten Graphics, Inc.
 * - OpenGL Graphics with the X Window System, Silicon Graphics, Inc.,
 *   ftp://ftp.sgi.com/opengl/doc/opengl1.2/glx1.3.ps
 * - Xlib - C Language X Interface, X Consortium Standard, X Version 11,
 *   Release 6.4, ftp://ftp.x.org/pub/R6.4/xc/doc/hardcopy/X11/xlib.PS.gz
 * - XFree86 Man pages, The XFree86 Project, Inc.,
 *   http://www.xfree86.org/current/manindex3.html
 *   
 */

/**
 * \page datatypes Notes on the XVisualInfo, Visual, and __GLXvisualConfig data types
 * 
 * -# X (unfortunately) has two (or three) data types which
 *    describe visuals.  Ideally, there would just be one.
 * -# We need the #__GLXvisualConfig type to augment #XVisualInfo and #Visual
 *    because we need to describe the GLX-specific attributes of visuals.
 * -# In this interface there is a one-to-one-to-one correspondence between
 *    the three types and they're all interconnected.
 * -# The #XVisualInfo type has a pointer to a #Visual.  The #Visual structure
 *    (aka MiniGLXVisualRec) has a pointer to the #__GLXvisualConfig.  The
 *    #Visual structure also has a pointer pointing back to the #XVisualInfo.
 * -# The #XVisualInfo structure is the only one who's contents are public.
 * -# The glXChooseVisual() and XGetVisualInfo() are the only functions that
 *    return #XVisualInfo structures.  They can be freed with XFree(), though
 *    there is a small memory leak.
 */


#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>    /* for gettimeofday */
#include <linux/kd.h>
#include <linux/vt.h>

#include "miniglxP.h"
#include "dri_util.h"

#include "imports.h"
#include "glcontextmodes.h"
#include "glapi.h"


static GLboolean __glXCreateContextWithConfig(__DRInativeDisplay *dpy,
        int screen, int fbconfigID, void *contextID,
        drm_context_t *hHWContext);

static GLboolean __glXGetDrawableInfo(__DRInativeDisplay *dpy, int scrn,
        __DRIid draw, unsigned int * index, unsigned int * stamp,
        int * x, int * y, int * width, int * height,
        int * numClipRects, drm_clip_rect_t ** pClipRects,
        int * backX, int * backY,
        int * numBackClipRects, drm_clip_rect_t ** pBackClipRects);

static __DRIscreen * __glXFindDRIScreen(__DRInativeDisplay *dpy, int scrn);

static GLboolean __glXWindowExists(__DRInativeDisplay *dpy, __DRIid draw);

static int __glXGetUST( int64_t * ust );

static GLboolean __glXGetMscRate(__DRInativeDisplay * dpy, __DRIid drawable,
    int32_t * numerator, int32_t * denominator);

static GLboolean xf86DRI_DestroyContext(__DRInativeDisplay *dpy, int screen,
    __DRIid context_id );

static GLboolean xf86DRI_CreateDrawable(__DRInativeDisplay *dpy, int screen,
    __DRIid drawable, drm_drawable_t *hHWDrawable );

static GLboolean xf86DRI_DestroyDrawable(__DRInativeDisplay *dpy, int screen,
    __DRIid drawable);


/** Wrapper around either malloc() */
void *
_mesa_malloc(size_t bytes)
{
   return malloc(bytes);
}

/** Wrapper around either calloc() */
void *
_mesa_calloc(size_t bytes)
{
   return calloc(1, bytes);
}

/** Wrapper around either free() */
void
_mesa_free(void *ptr)
{
   free(ptr);
}


/**
 * \brief Current GLX context.
 *
 * \sa glXGetCurrentContext().
 */
static GLXContext CurrentContext = NULL;



static Display *SignalDisplay = 0;

static void SwitchVT(int sig)
{
   fprintf(stderr, "SwitchVT %d dpy %p\n", sig, SignalDisplay);

   if (SignalDisplay) {
      SignalDisplay->vtSignalFlag = 1;
      switch( sig )
      {
      case SIGUSR1:                                /* vt has been released */
	 SignalDisplay->haveVT = 0;
	 break;
      case SIGUSR2:                                /* vt has been acquired */
	 SignalDisplay->haveVT = 1;
	 break;
      }
   }
}

/**********************************************************************/
/** \name Framebuffer device functions                                */
/**********************************************************************/
/*@{*/

/**
 * \brief Do the first part of setting up the framebuffer device.
 *
 * \param dpy the display handle.
 * \param use_vt use a VT for display or not
 *
 * \return GL_TRUE on success, or GL_FALSE on failure.
 * 
 * \sa This is called during XOpenDisplay().
 *
 * \internal
 * Gets the VT number, opens the respective console TTY device. Saves its state
 * to restore when exiting and goes into graphics mode.
 *
 * Opens the framebuffer device and make a copy of the original variable screen
 * information and gets the fixed screen information.  Maps the framebuffer and
 * MMIO region into the process address space.
 */
static GLboolean
OpenFBDev( Display *dpy, int use_vt )
{
   char ttystr[1000];
   int fd, vtnumber, ttyfd;

   assert(dpy);

   if (geteuid()) {
      fprintf(stderr, "error: you need to be root\n");
      return GL_FALSE;
   }
   
   if (use_vt) {
       
       /* open /dev/tty0 and get the VT number */
       if ((fd = open("/dev/tty0", O_WRONLY, 0)) < 0) {
	   fprintf(stderr, "error opening /dev/tty0\n");
	   return GL_FALSE;
       }
       if (ioctl(fd, VT_OPENQRY, &vtnumber) < 0 || vtnumber < 0) {
	   fprintf(stderr, "error: couldn't get a free vt\n");
	   return GL_FALSE;
       }
       
       fprintf(stderr, "*** got vt nr: %d\n", vtnumber);
       close(fd);
       
       /* open the console tty */
       sprintf(ttystr, "/dev/tty%d", vtnumber);  /* /dev/tty1-64 */
       dpy->ConsoleFD = open(ttystr, O_RDWR | O_NDELAY, 0);
       if (dpy->ConsoleFD < 0) {
	   fprintf(stderr, "error couldn't open console fd\n");
	   return GL_FALSE;
       }
       
       /* save current vt number */
       {
	   struct vt_stat vts;
	   if (ioctl(dpy->ConsoleFD, VT_GETSTATE, &vts) == 0)
	       dpy->OriginalVT = vts.v_active;
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
	   struct sigaction sig_tty;
	   
	   /* Set-up tty signal handler to catch the signal we request below */
	   SignalDisplay = dpy;
	   memset( &sig_tty, 0, sizeof( sig_tty ) );
	   sig_tty.sa_handler = SwitchVT;
	   sigemptyset( &sig_tty.sa_mask );
	   if( sigaction( SIGUSR1, &sig_tty, &dpy->OrigSigUsr1 ) ||
	       sigaction( SIGUSR2, &sig_tty, &dpy->OrigSigUsr2 ) )
	   {
	       fprintf(stderr, "error: can't set up signal handler (%s)",
		       strerror(errno) );
	       return GL_FALSE;
	   }
	   
	   
	   
	   vt.mode = VT_PROCESS;
	   vt.waitv = 0;
	   vt.relsig = SIGUSR1;
	   vt.acqsig = SIGUSR2;
	   if (ioctl(dpy->ConsoleFD, VT_SETMODE, &vt) < 0) {
	       fprintf(stderr, "error: ioctl(VT_SETMODE) failed: %s\n",
		       strerror(errno));
	       return GL_FALSE;
	   }
	   
	   
	   if (ioctl(dpy->ConsoleFD, VT_ACTIVATE, vtnumber) != 0)
	       printf("ioctl VT_ACTIVATE: %s\n", strerror(errno));
	   if (ioctl(dpy->ConsoleFD, VT_WAITACTIVE, vtnumber) != 0)
	       printf("ioctl VT_WAITACTIVE: %s\n", strerror(errno));
	   
	   if (ioctl(dpy->ConsoleFD, VT_GETMODE, &vt) < 0) {
	       fprintf(stderr, "error: ioctl VT_GETMODE: %s\n", strerror(errno));
	       return GL_FALSE;
	   }
	   
	   
	   
       }
       
       /* go into graphics mode */
       if (ioctl(dpy->ConsoleFD, KDSETMODE, KD_GRAPHICS) < 0) {
	   fprintf(stderr, "error: ioctl(KDSETMODE, KD_GRAPHICS) failed: %s\n",
		   strerror(errno));
	   return GL_FALSE;
       }
   }

   /* open the framebuffer device */
   dpy->FrameBufferFD = open(dpy->fbdevDevice, O_RDWR);
   if (dpy->FrameBufferFD < 0) {
      fprintf(stderr, "Error opening /dev/fb0: %s\n", strerror(errno));
      return GL_FALSE;
   }

  /* get the original variable screen info */
   if (ioctl(dpy->FrameBufferFD, FBIOGET_VSCREENINFO, &dpy->OrigVarInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_VSCREENINFO) failed: %s\n",
              strerror(errno));
      return GL_FALSE;
   }

   /* make copy */
   dpy->VarInfo = dpy->OrigVarInfo;  /* structure copy */

   /* Turn off hw accels (otherwise mmap of mmio region will be
    * refused)
    */
   dpy->VarInfo.accel_flags = 0; 
   if (ioctl(dpy->FrameBufferFD, FBIOPUT_VSCREENINFO, &dpy->VarInfo)) {
      fprintf(stderr, "error: ioctl(FBIOPUT_VSCREENINFO) failed: %s\n",
	      strerror(errno));
      return GL_FALSE;
   }



   /* Get the fixed screen info */
   if (ioctl(dpy->FrameBufferFD, FBIOGET_FSCREENINFO, &dpy->FixedInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_FSCREENINFO) failed: %s\n",
              strerror(errno));
      return GL_FALSE;
   }



   /* mmap the framebuffer into our address space */
   dpy->driverContext.FBStart = dpy->FixedInfo.smem_start;
   dpy->driverContext.FBSize = dpy->FixedInfo.smem_len;
   dpy->driverContext.shared.fbSize = dpy->FixedInfo.smem_len;
   dpy->driverContext.FBAddress = (caddr_t) mmap(0, /* start */
                                     dpy->driverContext.shared.fbSize, /* bytes */
                                     PROT_READ | PROT_WRITE, /* prot */
                                     MAP_SHARED, /* flags */
                                     dpy->FrameBufferFD, /* fd */
                                     0 /* offset */);
   if (dpy->driverContext.FBAddress == (caddr_t) - 1) {
      fprintf(stderr, "error: unable to mmap framebuffer: %s\n",
              strerror(errno));
      return GL_FALSE;
   }
	    
   /* mmap the MMIO region into our address space */
   dpy->driverContext.MMIOStart = dpy->FixedInfo.mmio_start;
   dpy->driverContext.MMIOSize = dpy->FixedInfo.mmio_len;
   dpy->driverContext.MMIOAddress = (caddr_t) mmap(0, /* start */
                                     dpy->driverContext.MMIOSize, /* bytes */
                                     PROT_READ | PROT_WRITE, /* prot */
                                     MAP_SHARED, /* flags */
                                     dpy->FrameBufferFD, /* fd */
                                     dpy->FixedInfo.smem_len /* offset */);
   if (dpy->driverContext.MMIOAddress == (caddr_t) - 1) {
      fprintf(stderr, "error: unable to mmap mmio region: %s\n",
              strerror(errno));
      return GL_FALSE;
   }

   fprintf(stderr, "got MMIOAddress %p offset %d\n",
           dpy->driverContext.MMIOAddress,
	   dpy->FixedInfo.smem_len);

   return GL_TRUE;
}




/**
 * \brief Setup up the desired framebuffer device mode.  
 *
 * \param dpy the display handle.
 * 
 * \return GL_TRUE on success, or GL_FALSE on failure.
 * 
 * \sa This is called during __miniglx_StartServer().
 *
 * \internal
 *
 * Bumps the size of the window the the next supported mode. Sets the
 * variable screen information according to the desired mode and asks
 * the driver to validate the mode. Certifies that a DirectColor or
 * TrueColor visual is used from the updated fixed screen information.
 * In the case of DirectColor visuals, sets up an 'identity' colormap to
 * mimic a TrueColor visual.
 *
 * Calls the driver hooks 'ValidateMode' and 'PostValidateMode' to
 * allow the driver to make modifications to the chosen mode according
 * to hardware constraints, or to save and restore videocard registers
 * that may be clobbered by the fbdev driver.
 *
 * \todo Timings are hard-coded in the source for a set of supported modes.
 */
static GLboolean
SetupFBDev( Display *dpy )
{
   int width, height;

   assert(dpy);

   width = dpy->driverContext.shared.virtualWidth;
   height = dpy->driverContext.shared.virtualHeight;
   
   if (width==832)
	width=800;
   /* Bump size up to next supported mode.
    */
   if (width <= 720 && height <= 480) { 
      width = 720; height = 480; 
   } 
   else if (width <= 960 && height <= 540) {
      width = 960; height = 540; 
   }  
   else if (width <= 800 && height <= 600) {
      width = 800; height = 600; 
   }  
   else if (width <= 1024 && height <= 768) { 
      width = 1024; height = 768; 
   } 
   else if (width <= 768 && height <= 1024) {
      width = 768; height = 1024; 
   }  
   else if (width <= 1280 && height <= 1024) { 
      width = 1280; height = 1024; 
   } 


   dpy->driverContext.shared.fbStride = width * (dpy->driverContext.bpp / 8);
   
   /* set the depth, resolution, etc */
   dpy->VarInfo = dpy->OrigVarInfo;
   dpy->VarInfo.bits_per_pixel = dpy->driverContext.bpp;
   dpy->VarInfo.xres_virtual = dpy->driverContext.shared.virtualWidth;
   dpy->VarInfo.yres_virtual = dpy->driverContext.shared.virtualHeight;
   dpy->VarInfo.xres = width;
   dpy->VarInfo.yres = height;
   dpy->VarInfo.xoffset = 0;
   dpy->VarInfo.yoffset = 0;
   dpy->VarInfo.nonstd = 0;
   dpy->VarInfo.vmode &= ~FB_VMODE_YWRAP; /* turn off scrolling */

   if (dpy->VarInfo.bits_per_pixel == 32) {
      dpy->VarInfo.red.offset = 16;
      dpy->VarInfo.green.offset = 8;
      dpy->VarInfo.blue.offset = 0;
      dpy->VarInfo.transp.offset = 24;
      dpy->VarInfo.red.length = 8;
      dpy->VarInfo.green.length = 8;
      dpy->VarInfo.blue.length = 8;
      dpy->VarInfo.transp.length = 8;
   }
   else if (dpy->VarInfo.bits_per_pixel == 16) {
      dpy->VarInfo.red.offset = 11;
      dpy->VarInfo.green.offset = 5;
      dpy->VarInfo.blue.offset = 0;
      dpy->VarInfo.red.length = 5;
      dpy->VarInfo.green.length = 6;
      dpy->VarInfo.blue.length = 5;
      dpy->VarInfo.transp.offset = 0;
      dpy->VarInfo.transp.length = 0;
   }
   else {
      fprintf(stderr, "Only 32bpp and 16bpp modes supported at the moment\n");
      return 0;
   }

   if (!dpy->driver->validateMode( &dpy->driverContext )) {
      fprintf(stderr, "Driver validateMode() failed\n");
      return 0;
   }

   /* These should be calculated with the gtf.c program, and then we could
      remove all this... AlanH. */
   if (dpy->VarInfo.xres == 1280 && 
       dpy->VarInfo.yres == 1024) {
      /* timing values taken from /etc/fb.modes (1280x1024 @ 75Hz) */
      dpy->VarInfo.pixclock = 7408;
      dpy->VarInfo.left_margin = 248;
      dpy->VarInfo.right_margin = 16;
      dpy->VarInfo.upper_margin = 38;
      dpy->VarInfo.lower_margin = 1;
      dpy->VarInfo.hsync_len = 144;
      dpy->VarInfo.vsync_len = 3;
   }
   else if (dpy->VarInfo.xres == 1024 && 
	    dpy->VarInfo.yres == 768) {
      /* timing values taken from /etc/fb.modes (1024x768 @ 75Hz) */
      dpy->VarInfo.pixclock = 12699;
      dpy->VarInfo.left_margin = 176;
      dpy->VarInfo.right_margin = 16;
      dpy->VarInfo.upper_margin = 28;
      dpy->VarInfo.lower_margin = 1;
      dpy->VarInfo.hsync_len = 96;
      dpy->VarInfo.vsync_len = 3;
   }
   else if (dpy->VarInfo.xres == 800 &&
	    dpy->VarInfo.yres == 600) {
      /* timing values taken from /etc/fb.modes (800x600 @ 75Hz) */
      dpy->VarInfo.pixclock = 27778;
      dpy->VarInfo.left_margin = 128;
      dpy->VarInfo.right_margin = 24;
      dpy->VarInfo.upper_margin = 22;
      dpy->VarInfo.lower_margin = 1;
      dpy->VarInfo.hsync_len = 72;
      dpy->VarInfo.vsync_len = 2;
   }
   else if (dpy->VarInfo.xres == 720 &&
	    dpy->VarInfo.yres == 480) {
      dpy->VarInfo.pixclock = 37202;
      dpy->VarInfo.left_margin = 88;
      dpy->VarInfo.right_margin = 16;
      dpy->VarInfo.upper_margin = 14;
      dpy->VarInfo.lower_margin = 1;
      dpy->VarInfo.hsync_len = 72;
      dpy->VarInfo.vsync_len = 3;
   }
   else if (dpy->VarInfo.xres == 960 &&
	    dpy->VarInfo.yres == 540) {
      dpy->VarInfo.pixclock = 24273;
      dpy->VarInfo.left_margin = 128;
      dpy->VarInfo.right_margin = 32;
      dpy->VarInfo.upper_margin = 16;
      dpy->VarInfo.lower_margin = 1;
      dpy->VarInfo.hsync_len = 96;
      dpy->VarInfo.vsync_len = 3;
   }
   else if (dpy->VarInfo.xres == 768 &&
	    dpy->VarInfo.yres == 1024) {
      /* timing values for 768x1024 @ 75Hz */
      dpy->VarInfo.pixclock = 11993;
      dpy->VarInfo.left_margin = 136;
      dpy->VarInfo.right_margin = 32;
      dpy->VarInfo.upper_margin = 41;
      dpy->VarInfo.lower_margin = 1;
      dpy->VarInfo.hsync_len = 80;
      dpy->VarInfo.vsync_len = 3;
   }
   else {
      /* XXX need timings for other screen sizes */
      fprintf(stderr, "XXXX screen size %d x %d not supported at this time!\n",
	      dpy->VarInfo.xres, dpy->VarInfo.yres);
      return GL_FALSE;
   }

   fprintf(stderr, "[miniglx] Setting mode: visible %dx%d virtual %dx%dx%d\n",
	   dpy->VarInfo.xres, dpy->VarInfo.yres,
	   dpy->VarInfo.xres_virtual, dpy->VarInfo.yres_virtual,
	   dpy->VarInfo.bits_per_pixel);

   /* set variable screen info */
   if (ioctl(dpy->FrameBufferFD, FBIOPUT_VSCREENINFO, &dpy->VarInfo)) {
      fprintf(stderr, "error: ioctl(FBIOPUT_VSCREENINFO) failed: %s\n",
	      strerror(errno));
      return GL_FALSE;
   }

   /* get the variable screen info, in case it has been modified */
   if (ioctl(dpy->FrameBufferFD, FBIOGET_VSCREENINFO, &dpy->VarInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_VSCREENINFO) failed: %s\n",
              strerror(errno));
      return GL_FALSE;
   }


   fprintf(stderr, "[miniglx] Readback mode: visible %dx%d virtual %dx%dx%d\n",
	   dpy->VarInfo.xres, dpy->VarInfo.yres,
	   dpy->VarInfo.xres_virtual, dpy->VarInfo.yres_virtual,
	   dpy->VarInfo.bits_per_pixel);

   /* Get the fixed screen info */
   if (ioctl(dpy->FrameBufferFD, FBIOGET_FSCREENINFO, &dpy->FixedInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_FSCREENINFO) failed: %s\n",
              strerror(errno));
      return GL_FALSE;
   }

   if (dpy->FixedInfo.visual != FB_VISUAL_TRUECOLOR &&
       dpy->FixedInfo.visual != FB_VISUAL_DIRECTCOLOR) {
      fprintf(stderr, "non-TRUECOLOR visuals not supported.\n");
      return GL_FALSE;
   }

   if (dpy->FixedInfo.visual == FB_VISUAL_DIRECTCOLOR) {
      struct fb_cmap cmap;
      unsigned short red[256], green[256], blue[256];
      int rcols = 1 << dpy->VarInfo.red.length;
      int gcols = 1 << dpy->VarInfo.green.length;
      int bcols = 1 << dpy->VarInfo.blue.length;
      int i;

      cmap.start = 0;      
      cmap.len = gcols;
      cmap.red   = red;
      cmap.green = green;
      cmap.blue  = blue;
      cmap.transp = NULL;

      for (i = 0; i < rcols ; i++) 
         red[i] = (65536/(rcols-1)) * i;

      for (i = 0; i < gcols ; i++) 
         green[i] = (65536/(gcols-1)) * i;

      for (i = 0; i < bcols ; i++) 
         blue[i] = (65536/(bcols-1)) * i;
      
      if (ioctl(dpy->FrameBufferFD, FBIOPUTCMAP, (void *) &cmap) < 0) {
         fprintf(stderr, "ioctl(FBIOPUTCMAP) failed [%d]\n", i);
	 exit(1);
      }
   }

   /* May need to restore regs fbdev has clobbered:
    */
   if (!dpy->driver->postValidateMode( &dpy->driverContext )) {
      fprintf(stderr, "Driver postValidateMode() failed\n");
      return 0;
   }

   return GL_TRUE;
}


/**
 * \brief Restore the framebuffer device to state it was in before we started
 *
 * Undoes the work done by SetupFBDev().
 * 
 * \param dpy the display handle.
 *
 * \return GL_TRUE on success, or GL_FALSE on failure.
 * 
 * \sa Called from XDestroyWindow().
 *
 * \internal
 * Restores the original variable screen info.
 */
static GLboolean
RestoreFBDev( Display *dpy )
{
   /* restore original variable screen info */
   if (ioctl(dpy->FrameBufferFD, FBIOPUT_VSCREENINFO, &dpy->OrigVarInfo)) {
      fprintf(stderr, "ioctl(FBIOPUT_VSCREENINFO failed): %s\n",
              strerror(errno));
      return GL_FALSE;
   }
   dpy->VarInfo = dpy->OrigVarInfo;

   return GL_TRUE;
}


/**
 * \brief Close the framebuffer device.  
 *
 * \param dpy the display handle.
 * 
 * \sa Called from XCloseDisplay().
 *
 * \internal
 * Unmaps the framebuffer and MMIO region.  Restores the text mode and the
 * original virtual terminal. Closes the console and framebuffer devices.
 */
static void
CloseFBDev( Display *dpy )
{
   struct vt_mode VT;

   munmap(dpy->driverContext.FBAddress, dpy->driverContext.FBSize);
   munmap(dpy->driverContext.MMIOAddress, dpy->driverContext.MMIOSize);

   if (dpy->ConsoleFD) {
       /* restore text mode */
       ioctl(dpy->ConsoleFD, KDSETMODE, KD_TEXT);
       
       /* set vt */
       if (ioctl(dpy->ConsoleFD, VT_GETMODE, &VT) != -1) {
	   VT.mode = VT_AUTO;
	   ioctl(dpy->ConsoleFD, VT_SETMODE, &VT);
       }
       
       /* restore original vt */
       if (dpy->OriginalVT >= 0) {
	   ioctl(dpy->ConsoleFD, VT_ACTIVATE, dpy->OriginalVT);
	   dpy->OriginalVT = -1;
       }
       
       close(dpy->ConsoleFD);
   }
   close(dpy->FrameBufferFD);
}

/*@}*/


/**********************************************************************/
/** \name Misc functions needed for DRI drivers                       */
/**********************************************************************/
/*@{*/

/**
 * \brief Find the DRI screen dependent methods associated with the display.
 *
 * \param dpy a display handle, as returned by XOpenDisplay().
 * \param scrn the screen number. Not referenced.
 * 
 * \returns a pointer to a __DRIscreenRec structure.
 * 
 * \internal
 * Returns the MiniGLXDisplayRec::driScreen attribute.
 */
static __DRIscreen *
__glXFindDRIScreen(__DRInativeDisplay *dpy, int scrn)
{
   (void) scrn;
   return &((Display*)dpy)->driScreen;
}

/**
 * \brief Validate a drawable.
 *
 * \param dpy a display handle, as returned by XOpenDisplay().
 * \param draw drawable to validate.
 * 
 * \internal
 * Since Mini GLX only supports one window, compares the specified drawable with
 * the MiniGLXDisplayRec::TheWindow attribute.
 */
static GLboolean
__glXWindowExists(__DRInativeDisplay *dpy, __DRIid draw)
{
   const Display * const display = (Display*)dpy;
   if (display->TheWindow == (Window) draw)
      return True;
   else
      return False;
}

/**
 * \brief Get current thread ID.
 *
 * \return thread ID.
 *
 * \internal
 * Always returns 0. 
 */
/*unsigned long
_glthread_GetID(void)
{
   return 0;
}*/

/*@}*/


/**
 * \brief Scan Linux /prog/bus/pci/devices file to determine hardware
 * chipset based on supplied bus ID.
 * 
 * \return probed chipset (non-zero) on success, zero otherwise.
 * 
 * \internal 
 */
static int get_chipset_from_busid( Display *dpy )
{
   char buf[0x200];
   FILE *file;
   const char *fname = "/proc/bus/pci/devices";
   int retval = 0;

   if (!(file = fopen(fname,"r"))) {
      fprintf(stderr, "couldn't open %s: %s\n", fname, strerror(errno));
      return 0;
   }

   while (fgets(buf, sizeof(buf)-1, file)) {
      unsigned int nr, bus, dev, fn, vendor, device, encode;
      nr = sscanf(buf, "%04x\t%04x%04x", &encode, 
		  &vendor, &device);
      
      bus = encode >> 8;
      dev = (encode & 0xFF) >> 3;
      fn = encode & 0x7;

      if (nr != 3)
	 break;

      if (bus == dpy->driverContext.pciBus &&
          dev == dpy->driverContext.pciDevice &&
          fn  == dpy->driverContext.pciFunc) {
	 retval = device;
	 break;
      }
   }

   fclose(file);

   if (retval)
      fprintf(stderr, "[miniglx] probed chipset 0x%x\n", retval);
   else
      fprintf(stderr, "[miniglx] failed to probe chipset\n");

   return retval;
}


/**
 * \brief Read settings from a configuration file.
 * 
 * The configuration file is usually "/etc/miniglx.conf", but can be overridden
 * with the MINIGLX_CONF environment variable. 
 *
 * The format consists in \code option = value \endcode lines. The option names 
 * corresponds to the fields in MiniGLXDisplayRec.
 * 
 * \param dpy the display handle as.
 *
 * \return non-zero on success, zero otherwise.
 * 
 * \internal 
 * Sets some defaults. Opens and parses the the Mini GLX configuration file and
 * fills in the MiniGLXDisplayRec field that corresponds for each option.
 */
static int __read_config_file( Display *dpy )
{
   FILE *file;
   const char *fname;

   /* Fallback/defaults
    */
   dpy->fbdevDevice = "/dev/fb0";
   dpy->clientDriverName = "fb_dri.so";
   dpy->driverContext.pciBus = 0;
   dpy->driverContext.pciDevice = 0;
   dpy->driverContext.pciFunc = 0;
   dpy->driverContext.chipset = 0;   
   dpy->driverContext.pciBusID = 0;
   dpy->driverContext.shared.virtualWidth = 1280;
   dpy->driverContext.shared.virtualHeight = 1024;
   dpy->driverContext.bpp = 32;
   dpy->driverContext.cpp = 4;
   dpy->rotateMode = 0;
   dpy->driverContext.agpmode = 1;
   dpy->driverContext.isPCI = 0;
   dpy->driverContext.colorTiling = 0;

   fname = getenv("MINIGLX_CONF");
   if (!fname) fname = "/etc/miniglx.conf";

   file = fopen(fname, "r");
   if (!file) {
      fprintf(stderr, "couldn't open config file %s: %s\n", fname, strerror(errno));
      return 0;
   }


   while (!feof(file)) {
      char buf[81], *opt = buf, *val, *tmp1, *tmp2;
      fgets(buf, sizeof(buf), file); 

      /* Parse 'opt = val' -- must be easier ways to do this.
       */
      while (isspace(*opt)) opt++;
      val = opt;
      if (*val == '#') continue; /* comment */
      while (!isspace(*val) && *val != '=' && *val) val++;
      tmp1 = val;
      while (isspace(*val)) val++;
      if (*val != '=') continue;
      *tmp1 = 0; 
      val++;
      while (isspace(*val)) val++;
      tmp2 = val;
      while (!isspace(*tmp2) && *tmp2 != '\n' && *tmp2) tmp2++;
      *tmp2 = 0;


      if (strcmp(opt, "fbdevDevice") == 0) 
	 dpy->fbdevDevice = strdup(val);
      else if (strcmp(opt, "clientDriverName") == 0)
	 dpy->clientDriverName = strdup(val);
      else if (strcmp(opt, "rotateMode") == 0)
	 dpy->rotateMode = atoi(val) ? 1 : 0;
      else if (strcmp(opt, "pciBusID") == 0) {
	 if (sscanf(val, "PCI:%d:%d:%d",
		    &dpy->driverContext.pciBus,
                    &dpy->driverContext.pciDevice,
                    &dpy->driverContext.pciFunc) != 3) {
	    fprintf(stderr, "malformed bus id: %s\n", val);
	    continue;
	 }
   	 dpy->driverContext.pciBusID = strdup(val);
      }
      else if (strcmp(opt, "chipset") == 0) {
	 if (sscanf(val, "0x%x", &dpy->driverContext.chipset) != 1)
	    fprintf(stderr, "malformed chipset: %s\n", opt);
      }
      else if (strcmp(opt, "virtualWidth") == 0) {
	 if (sscanf(val, "%d", &dpy->driverContext.shared.virtualWidth) != 1)
	    fprintf(stderr, "malformed virtualWidth: %s\n", opt);
      }
      else if (strcmp(opt, "virtualHeight") == 0) {
	 if (sscanf(val, "%d", &dpy->driverContext.shared.virtualHeight) != 1)
	    fprintf(stderr, "malformed virutalHeight: %s\n", opt);
      }
      else if (strcmp(opt, "bpp") == 0) {
	 if (sscanf(val, "%d", &dpy->driverContext.bpp) != 1)
	    fprintf(stderr, "malformed bpp: %s\n", opt);
	 dpy->driverContext.cpp = dpy->driverContext.bpp / 8;
      }
      else if (strcmp(opt, "agpmode") == 0) {
         if (sscanf(val, "%d", &dpy->driverContext.agpmode) != 1)
            fprintf(stderr, "malformed agpmode: %s\n", opt);
      }
      else if (strcmp(opt, "isPCI") == 0) {
	 dpy->driverContext.isPCI = atoi(val) ? 1 : 0;
      }
      else if (strcmp(opt, "colorTiling") == 0) {
	 dpy->driverContext.colorTiling = atoi(val) ? 1 : 0;
      }
   }

   fclose(file);

   if (dpy->driverContext.chipset == 0 && dpy->driverContext.pciBusID != 0) 
      dpy->driverContext.chipset = get_chipset_from_busid( dpy );

   return 1;
}

/**
 * Versioned name of the expected \c __driCreateNewScreen function.
 * 
 * The version of the last incompatible loader/driver inteface change is
 * appended to the name of the \c __driCreateNewScreen function.  This
 * prevents loaders from trying to load drivers that are too old.
 * 
 * \todo
 * Create a macro or something so that this is automatically updated.
 */
static const char createNewScreenName[] = "__driCreateNewScreen_20050727";


static int InitDriver( Display *dpy )
{
   /*
    * Begin DRI setup.
    * We're kind of combining the per-display and per-screen information
    * which was kept separate in XFree86/DRI's libGL.
    */
   dpy->dlHandle = dlopen(dpy->clientDriverName, RTLD_NOW | RTLD_GLOBAL);
   if (!dpy->dlHandle) {
      fprintf(stderr, "Unable to open %s: %s\n", dpy->clientDriverName,
	      dlerror());
      goto failed;
   }

   /* Pull in Mini GLX specific hooks:
    */
   dpy->driver = (struct DRIDriverRec *) dlsym(dpy->dlHandle,
                                               "__driDriver");
   if (!dpy->driver) {
      fprintf(stderr, "Couldn't find __driDriver in %s\n",
              dpy->clientDriverName);
      goto failed;
   }

   /* Pull in standard DRI client-side driver hooks:
    */
   dpy->createNewScreen = (PFNCREATENEWSCREENFUNC)
           dlsym(dpy->dlHandle, createNewScreenName);
   if (!dpy->createNewScreen) {
      fprintf(stderr, "Couldn't find %s in %s\n", createNewScreenName,
              dpy->clientDriverName);
      goto failed;
   }

   return GL_TRUE;

failed:
   if (dpy->dlHandle) {
       dlclose(dpy->dlHandle);
       dpy->dlHandle = 0;
   }
   return GL_FALSE;
}


/**********************************************************************/
/** \name Public API functions (Xlib and GLX)                         */
/**********************************************************************/
/*@{*/


/**
 * \brief Initialize the graphics system.
 * 
 * \param display_name currently ignored. It is recommended to pass it as NULL.
 * \return a pointer to a #Display if the function is able to initialize
 * the graphics system, NULL otherwise.
 * 
 * Allocates a MiniGLXDisplayRec structure and fills in with information from a
 * configuration file. 
 *
 * Calls OpenFBDev() to open the framebuffer device and calls
 * DRIDriverRec::initFBDev to do the client-side initialization on it.
 *
 * Loads the DRI driver and pulls in Mini GLX specific hooks into a
 * DRIDriverRec structure, and the standard DRI \e __driCreateScreen hook.
 * Asks the driver for a list of supported visuals.  Performs the per-screen
 * client-side initialization.  Also setups the callbacks in the screen private
 * information.
 *
 * Does the framebuffer device setup. Calls __miniglx_open_connections() to
 * serve clients.
 */
Display *
__miniglx_StartServer( const char *display_name )
{
   Display *dpy;
   int use_vt = 0;

   dpy = (Display *)calloc(1, sizeof(Display));
   if (!dpy)
      return NULL;

   dpy->IsClient = False;

   if (!__read_config_file( dpy )) {
      fprintf(stderr, "Couldn't get configuration details\n");
      free(dpy);
      return NULL;
   }

   /* Open the fbdev device
    */
   if (!OpenFBDev(dpy, use_vt)) {
      fprintf(stderr, "OpenFBDev failed\n");
      free(dpy);
      return NULL;
   }

   if (!InitDriver(dpy)) {
      fprintf(stderr, "InitDriver failed\n");
      free(dpy);
      return NULL;
   }

   /* Perform the initialization normally done in the X server 
    */
   if (!dpy->driver->initFBDev( &dpy->driverContext )) {
      fprintf(stderr, "%s: __driInitFBDev failed\n", __FUNCTION__);
      dlclose(dpy->dlHandle);
      return GL_FALSE;
   }

   /* do fbdev setup
    */
   if (!SetupFBDev(dpy)) {
      fprintf(stderr, "SetupFBDev failed\n");
      free(dpy);
      return NULL;
   }

   /* unlock here if not using VT -- JDS */
   if (!use_vt) {
       if (dpy->driver->restoreHardware)
	   dpy->driver->restoreHardware( &dpy->driverContext ); 
       DRM_UNLOCK( dpy->driverContext.drmFD,
		   dpy->driverContext.pSAREA,
		   dpy->driverContext.serverContext );
       dpy->hwActive = 1;
   }

   /* Ready for clients:
    */
   if (!__miniglx_open_connections(dpy)) {
      free(dpy);
      return NULL;
   }
      
   return dpy;
}


/**
 * Implement \c __DRIinterfaceMethods::getProcAddress.
 */
static __DRIfuncPtr get_proc_address( const char * proc_name )
{
    (void) proc_name;
    return NULL;
}


/**
 * Table of functions exported by the loader to the driver.
 */
static const __DRIinterfaceMethods interface_methods = {
    get_proc_address,

    _gl_context_modes_create,
    _gl_context_modes_destroy,
      
    __glXFindDRIScreen,
    __glXWindowExists,
      
    __glXCreateContextWithConfig,
    xf86DRI_DestroyContext,

    xf86DRI_CreateDrawable,
    xf86DRI_DestroyDrawable,
    __glXGetDrawableInfo,

    __glXGetUST,
    __glXGetMscRate,
};


static void *
CallCreateNewScreen(Display *dpy, int scrn, __DRIscreen *psc)
{
    void *psp = NULL;
    drm_handle_t hSAREA;
    drmAddress pSAREA;
    const char *BusID;
    int   i;
    __DRIversion   ddx_version;
    __DRIversion   dri_version;
    __DRIversion   drm_version;
    __DRIframebuffer  framebuffer;
    int   fd = -1;
    int   status;
    const char * err_msg;
    const char * err_extra;
    drmVersionPtr version;
    drm_handle_t  hFB;
    drm_magic_t magic;


    hSAREA = dpy->driverContext.shared.hSAREA;
    BusID = dpy->driverContext.pciBusID;

    fd = drmOpen(NULL, BusID);

    err_msg = "open DRM";
    err_extra = strerror( -fd );

    if (fd < 0) goto done;

    err_msg = "drmGetMagic";
    err_extra = NULL;

    if (drmGetMagic(fd, &magic)) goto done;
    
    dpy->authorized = False;
    send_char_msg( dpy, 0, _Authorize );
    send_msg( dpy, 0, &magic, sizeof(magic));
    
    /* force net buffer flush */
    while (!dpy->authorized)
      handle_fd_events( dpy, 0 );

    version = drmGetVersion(fd);
    if (version) {
        drm_version.major = version->version_major;
        drm_version.minor = version->version_minor;
        drm_version.patch = version->version_patchlevel;
        drmFreeVersion(version);
    }
    else {
        drm_version.major = -1;
        drm_version.minor = -1;
        drm_version.patch = -1;
    }

    /*
     * Get device name (like "tdfx") and the ddx version numbers.
     * We'll check the version in each DRI driver's "createScreen"
     * function.
     */
    ddx_version.major = 4;
    ddx_version.minor = 0;
    ddx_version.patch = 0;

    /*
     * Get the DRI X extension version.
     */
    dri_version.major = 4;
    dri_version.minor = 0;
    dri_version.patch = 0;

    /*
     * Get device-specific info.  pDevPriv will point to a struct
     * (such as DRIRADEONRec in xfree86/driver/ati/radeon_dri.h)
     * that has information about the screen size, depth, pitch,
     * ancilliary buffers, DRM mmap handles, etc.
     */
    hFB = dpy->driverContext.shared.hFrameBuffer;
    framebuffer.size = dpy->driverContext.shared.fbSize;
    framebuffer.stride = dpy->driverContext.shared.fbStride;
    framebuffer.dev_priv_size = dpy->driverContext.driverClientMsgSize;
    framebuffer.dev_priv = dpy->driverContext.driverClientMsg;
    framebuffer.width = dpy->driverContext.shared.virtualWidth;
    framebuffer.height = dpy->driverContext.shared.virtualHeight;

    /*
     * Map the framebuffer region.
     */
    status = drmMap(fd, hFB, framebuffer.size, 
            (drmAddressPtr)&framebuffer.base);

    err_msg = "drmMap of framebuffer";
    err_extra = strerror( -status );

    if ( status != 0 ) goto done;

    /*
     * Map the SAREA region.  Further mmap regions may be setup in
     * each DRI driver's "createScreen" function.
     */
    status = drmMap(fd, hSAREA, SAREA_MAX, &pSAREA);

    err_msg = "drmMap of sarea";
    err_extra = strerror( -status );

    if ( status == 0 ) {
        err_msg = "InitDriver";
        err_extra = NULL;
        psp = dpy->createNewScreen(dpy, scrn, psc, NULL,
                & ddx_version,
                & dri_version,
                & drm_version,
                & framebuffer,
                pSAREA,
                fd,
                20050727,
		& interface_methods,
                (__GLcontextModes **) &dpy->driver_modes);

	/* fill in dummy visual ids */
	{
	  __GLcontextModes *temp;
	  temp = (__GLcontextModes *)dpy->driver_modes;
	  i = 1;
	  while (temp)
	  {
	    temp->visualID = i++;
	    temp=temp->next;
	  }
	}
    }
    
done:
    if ( psp == NULL ) {
        if ( pSAREA != MAP_FAILED ) {
            (void)drmUnmap(pSAREA, SAREA_MAX);
        }

        if ( framebuffer.base != MAP_FAILED ) {
            (void)drmUnmap((drmAddress)framebuffer.base, framebuffer.size);
        }

        if ( framebuffer.dev_priv != NULL ) {
            free(framebuffer.dev_priv);
        }

        if ( fd >= 0 ) {
            (void)drmClose(fd);
        }

        if ( err_extra != NULL ) {
            fprintf(stderr, "libGL error: %s failed (%s)\n", err_msg,
                    err_extra);
        }
        else {
            fprintf(stderr, "libGL error: %s failed\n", err_msg );
        }

        fprintf(stderr, "libGL error: reverting to (slow) indirect rendering\n");
    }

    return psp;
}

/**
 * \brief Initialize the graphics system.
 * 
 * \param display_name currently ignored. It is recommended to pass it as NULL.
 * \return a pointer to a #Display if the function is able to initialize
 * the graphics system, NULL otherwise.
 * 
 * Allocates a MiniGLXDisplayRec structure and fills in with information from a
 * configuration file. 
 *
 * Calls __miniglx_open_connections() to connect to the server.
 * 
 * Loads the DRI driver and pulls in Mini GLX specific hooks into a
 * DRIDriverRec structure, and the standard DRI \e __driCreateScreen hook.
 * Asks the driver for a list of supported visuals.  Performs the per-screen
 * client-side initialization.  Also setups the callbacks in the screen private
 * information.
 *
 * \todo
 *   - read config file
 *      - what about virtualWidth, etc?
 *   - determine dpy->driverClientMsgSize,
 *   - allocate dpy->driverClientMsg
 */
Display *
XOpenDisplay( const char *display_name )
{
   Display *dpy;

   dpy = (Display *)calloc(1, sizeof(Display));
   if (!dpy)
      return NULL;

   dpy->IsClient = True;

   /* read config file 
    */
   if (!__read_config_file( dpy )) {
      fprintf(stderr, "Couldn't get configuration details\n");
      free(dpy);
      return NULL;
   }

   /* Connect to the server and receive driverClientMsg
    */
   if (!__miniglx_open_connections(dpy)) {
      free(dpy);
      return NULL;
   }

   /* dlopen the driver .so file
    */
   if (!InitDriver(dpy)) {
      fprintf(stderr, "InitDriver failed\n");
      free(dpy);
      return NULL;
   }

   /* Perform the client-side initialization.  
    *
    * Clearly there is a limit of one on the number of windows in
    * existence at any time.
    *
    * Need to shut down DRM and free DRI data in XDestroyWindow(), too.
    */
   dpy->driScreen.private = CallCreateNewScreen(dpy, 0, &dpy->driScreen);
   if (!dpy->driScreen.private) {
      fprintf(stderr, "%s: __driCreateScreen failed\n", __FUNCTION__);
      dlclose(dpy->dlHandle);
      free(dpy);
      return NULL;
   }
   
   /* Anything more to do?
    */
   return dpy;
}


/**
 * \brief Release display resources.
 * 
 * When the application is about to exit, the resources associated with the
 * graphics system can be released by calling this function.
 * 
 * \param dpy display handle. It becomes invalid at this point.
 * 
 * Destroys the window if any, and destroys the per-screen
 * driver private information.
 * Calls __miniglx_close_connections().
 * 
 * If a server, puts the the framebuffer back into the initial state.
 *
 * Finally frees the display structure.
 */
void
XCloseDisplay( Display *dpy )
{
   glXMakeCurrent( dpy, NULL, NULL);

   if (dpy->NumWindows) 
      XDestroyWindow( dpy, dpy->TheWindow );

   /* As this is done in XOpenDisplay, need to undo it here:
    */
   dpy->driScreen.destroyScreen(dpy, 0, dpy->driScreen.private);

   __miniglx_close_connections( dpy );

   if (!dpy->IsClient) {
      /* put framebuffer back to initial state 
       */
      (*dpy->driver->haltFBDev)( &dpy->driverContext );
      RestoreFBDev(dpy);
      CloseFBDev(dpy);
   }

   dlclose(dpy->dlHandle);
   free(dpy);
}


/**
 * \brief Window creation.
 *
 * \param display a display handle, as returned by XOpenDisplay().
 * \param parent the parent window for the new window. For Mini GLX this should
 * be 
 * \code RootWindow(display, 0) \endcode
 * \param x the window abscissa. For Mini GLX, it should be zero.
 * \param y the window ordinate. For Mini GLX, it should be zero.
 * \param width the window width. For Mini GLX, this specifies the desired
 * screen width such as 1024 or 1280. 
 * \param height the window height. For Mini GLX, this specifies the desired
 * screen height such as 768 or 1024.
 * \param border_width the border width. For Mini GLX, it should be zero.
 * \param depth the window pixel depth. For Mini GLX, this should be the depth
 * found in the #XVisualInfo object returned by glXChooseVisual() 
 * \param winclass the window class. For Mini GLX this value should be
 * #InputOutput.
 * \param visual the visual type. It should be the visual field of the
 * #XVisualInfo object returned by glXChooseVisual().
 * \param valuemask which fields of the XSetWindowAttributes() are to be used.
 * For Mini GLX this is typically the bitmask 
 * \code CWBackPixel | CWBorderPixel | CWColormap \endcode
 * \param attributes initial window attributes. The
 * XSetWindowAttributes::background_pixel, XSetWindowAttributes::border_pixel
 * and XSetWindowAttributes::colormap fields should be set.
 *
 * \return a window handle if it succeeds or zero if it fails.
 * 
 * \note For Mini GLX, windows are full-screen; they cover the entire frame
 * buffer.  Also, Mini GLX imposes a limit of one window. A second window
 * cannot be created until the first one is destroyed.
 *
 * This function creates and initializes a ::MiniGLXWindowRec structure after
 * ensuring that there is no other window created.  Performs the per-drawable
 * client-side initialization calling the __DRIscreenRec::createDrawable
 * method.
 * 
 */
Window
XCreateWindow( Display *dpy, Window parent, int x, int y,
               unsigned int width, unsigned int height,
               unsigned int border_width, int depth, unsigned int winclass,
               Visual *visual, unsigned long valuemask,
               XSetWindowAttributes *attributes )
{
   const int empty_attribute_list[1] = { None };

   Window win;

   /* ignored */
   (void) x;
   (void) y;
   (void) border_width;
   (void) depth;
   (void) winclass;
   (void) valuemask;
   (void) attributes;

   if (!dpy->IsClient) {
      fprintf(stderr, "Server process may not create windows (currently)\n");
      return NULL;
   }

   if (dpy->NumWindows > 0)
      return NULL;  /* only allow one window */

   assert(dpy->TheWindow == NULL);

   win = malloc(sizeof(struct MiniGLXWindowRec));
   if (!win)
      return NULL;

   /* In rotated mode, translate incoming x,y,width,height into
    * 'normal' coordinates.
    */
   if (dpy->rotateMode) {
      int tmp;
      tmp = width; width = height; height = tmp;
      tmp = x; x = y; y = tmp;
   }

   /* init other per-window fields */
   win->x = 0;
   win->y = 0;
   win->w = width;
   win->h = height;
   win->visual = visual;  /* ptr assignment */

   win->bytesPerPixel = dpy->driverContext.cpp;
   win->rowStride = dpy->driverContext.shared.virtualWidth * win->bytesPerPixel;
   win->size = win->rowStride * height; 
   win->frontStart = dpy->driverContext.FBAddress;
   win->frontBottom = (GLubyte *) win->frontStart + (height-1) * win->rowStride;

   /* This is incorrect: the hardware driver could put the backbuffer
    * just about anywhere.  These fields, including the above are
    * hardware dependent & don't really belong here.
    */
   if (visual->mode->doubleBufferMode) {
      win->backStart = (GLubyte *) win->frontStart +
	 win->rowStride * dpy->driverContext.shared.virtualHeight;
      win->backBottom = (GLubyte *) win->backStart
	 + (height - 1) * win->rowStride;
      win->curBottom = win->backBottom;
   }
   else {
      /* single buffered */
      win->backStart = NULL;
      win->backBottom = NULL;
      win->curBottom = win->frontBottom;
   }

   dpy->driScreen.createNewDrawable(dpy, visual->mode, (int) win,
           &win->driDrawable, GLX_WINDOW_BIT, empty_attribute_list);

   if (!win->driDrawable.private) {
      fprintf(stderr, "%s: dri.createDrawable failed\n", __FUNCTION__);
      free(win);
      return NULL;
   }

   dpy->NumWindows++;
   dpy->TheWindow = win;

   return win; 
}


/**
 * \brief Destroy window.
 *
 * \param display display handle.
 * \param w window handle.
 *
 * This function calls XUnmapWindow() and frees window \p w.
 * 
 * In case of destroying the current buffer first unbinds the GLX context
 * by calling glXMakeCurrent() with no drawable.
 */
void
XDestroyWindow( Display *display, Window win )
{
   if (display && display->IsClient && win) {
      /* check if destroying the current buffer */
      Window curDraw = glXGetCurrentDrawable();
      if (win == curDraw) {
         glXMakeCurrent( display, NULL, NULL);
      }

      XUnmapWindow( display, win );

      /* Destroy the drawable. */
      win->driDrawable.destroyDrawable(display, win->driDrawable.private);
      free(win);
      
      /* unlink window from display */
      display->NumWindows--;
      assert(display->NumWindows == 0);
      display->TheWindow = NULL;
   }
}




/**
 * \brief Create color map structure.
 *
 * \param dpy the display handle as returned by XOpenDisplay().
 * \param w the window on whose screen you want to create a color map. This
 * parameter is ignored by Mini GLX but should be the value returned by the
 * \code RootWindow(display, 0) \endcode macro.
 * \param visual a visual type supported on the screen. This parameter is
 * ignored by Mini GLX but should be the XVisualInfo::visual returned by
 * glXChooseVisual().
 * \param alloc the color map entries to be allocated. This parameter is ignored
 * by Mini GLX but should be set to #AllocNone.
 *
 * \return the color map.
 * 
 * This function is only provided to ease porting.  Practically a no-op -
 * returns a pointer to a dynamically allocated chunk of memory (one byte).
 */
Colormap
XCreateColormap( Display *dpy, Window w, Visual *visual, int alloc )
{
   (void) dpy;
   (void) w;
   (void) visual;
   (void) alloc;
   return (Colormap) malloc(1);
}


/**
 * \brief Destroy color map structure.
 *
 * \param display The display handle as returned by XOpenDisplay().
 * \param colormap the color map to destroy.
 *
 * This function is only provided to ease porting.  Practically a no-op. 
 *
 * Frees the memory pointed by \p colormap.
 */
void
XFreeColormap( Display *display, Colormap colormap )
{
   (void) display;
   (void) colormap;
   free(colormap);
}


/**
 * \brief Free client data.
 *
 * \param data the data that is to be freed.
 *
 * Frees the memory pointed by \p data.
 */
void
XFree( void *data )
{
   free(data);
}


/**
 * \brief Query available visuals.
 *
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param vinfo_mask a bitmask indicating which fields of the \p vinfo_template
 * are to be matched.  The value must be \c VisualScreenMask.
 * \param vinfo_template a template whose fields indicate which visual
 * attributes must be matched by the results.  The XVisualInfo::screen field of
 * this structure must be zero.
 * \param nitens_return will hold the number of visuals returned.
 *
 * \return the address of an array of all available visuals.
 * 
 * An example of using XGetVisualInfo() to get all available visuals follows:
 * 
 * \code
 * XVisualInfo vinfo_template, *results;
 * int nitens_return;
 * Display *dpy = XOpenDisplay(NULL);
 * vinfo_template.screen = 0;
 * results = XGetVisualInfo(dpy, VisualScreenMask, &vinfo_template, &nitens_return);
 * \endcode
 * 
 * Returns the list of all ::XVisualInfo available, one per
 * ::__GLcontextMode stored in MiniGLXDisplayRec::modes.
 */
XVisualInfo *
XGetVisualInfo( Display *dpy, long vinfo_mask, XVisualInfo *vinfo_template, int *nitens_return )
{
   const __GLcontextModes *mode;
   XVisualInfo *results;
   Visual *visResults;
   int i, n=0;

   //   ASSERT(vinfo_mask == VisualScreenMask);
   ASSERT(vinfo_template.screen == 0);

   if (vinfo_mask == VisualIDMask)
   {
     for ( mode = dpy->driver_modes ; mode != NULL ; mode= mode->next )
       if (mode->visualID == vinfo_template->visualid)
	 n=1;

     if (n==0)
       return NULL;
     
     results = (XVisualInfo *)calloc(1, n * sizeof(XVisualInfo));
     if (!results) {
       *nitens_return = 0;
       return NULL;
     }
     
     visResults = (Visual *)calloc(1, n * sizeof(Visual));
     if (!results) {
       free(results);
       *nitens_return = 0;
       return NULL;
     }

     for ( mode = dpy->driver_modes ; mode != NULL ; mode= mode->next )
       if (mode->visualID == vinfo_template->visualid)
       {
	 visResults[0].mode=mode;
	 visResults[0].visInfo = results;
	 visResults[0].dpy = dpy;
	 if (dpy->driverContext.bpp == 32)
	   visResults[0].pixelFormat = PF_B8G8R8A8; /* XXX: FIX ME */
	 else
	   visResults[0].pixelFormat = PF_B5G6R5; /* XXX: FIX ME */
       
	 results[0].visual = visResults;
	 results[0].visualid = mode->visualID;
#if defined(__cplusplus) || defined(c_plusplus)
	 results[0].c_class = TrueColor;
#else
	 results[0].class = TrueColor;
#endif
	 results[0].depth = mode->redBits +
	   mode->redBits +
	   mode->redBits +
	   mode->redBits;
	 results[0].bits_per_rgb = dpy->driverContext.bpp;
	 
       }
     
   }
   else // if (vinfo_mask == VisualScreenMask)
   {
     n = 0;
     for ( mode = dpy->driver_modes ; mode != NULL ; mode = mode->next )
       n++;
     
     results = (XVisualInfo *)calloc(1, n * sizeof(XVisualInfo));
     if (!results) {
       *nitens_return = 0;
       return NULL;
     }
     
     visResults = (Visual *)calloc(1, n * sizeof(Visual));
     if (!results) {
       free(results);
       *nitens_return = 0;
       return NULL;
     }
     
     for ( mode = dpy->driver_modes, i = 0 ; mode != NULL ; mode = mode->next, i++ ) {
       visResults[i].mode = mode;
       visResults[i].visInfo = results + i;
       visResults[i].dpy = dpy;
       
       if (dpy->driverContext.bpp == 32)
	 visResults[i].pixelFormat = PF_B8G8R8A8; /* XXX: FIX ME */
       else
	 visResults[i].pixelFormat = PF_B5G6R5; /* XXX: FIX ME */
       
       results[i].visual = visResults + i;
       results[i].visualid = mode->visualID;
#if defined(__cplusplus) || defined(c_plusplus)
       results[i].c_class = TrueColor;
#else
       results[i].class = TrueColor;
#endif
       results[i].depth = mode->redBits +
	 mode->redBits +
	 mode->redBits +
	 mode->redBits;
       results[i].bits_per_rgb = dpy->driverContext.bpp;
     }
   }
   *nitens_return = n;
   return results;
}


/**
 * \brief Return a visual that matches specified attributes.
 *
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param screen the screen number. It is currently ignored by Mini GLX and
 * should be zero.
 * \param attribList a list of GLX attributes which describe the desired pixel
 * format. It is terminated by the token \c None. 
 *
 * The attributes are as follows:
 * \arg GLX_USE_GL:
 * This attribute should always be present in order to maintain compatibility
 * with GLX.
 * \arg GLX_RGBA:
 * If present, only RGBA pixel formats will be considered. Otherwise, only
 * color index formats are considered.
 * \arg GLX_DOUBLEBUFFER:
 * if present, only double-buffered pixel formats will be chosen.
 * \arg GLX_RED_SIZE \e n:
 * Must be followed by a non-negative integer indicating the minimum number of
 * bits per red pixel component that is acceptable.
 * \arg GLX_GREEN_SIZE \e n:
 * Must be followed by a non-negative integer indicating the minimum number of
 * bits per green pixel component that is acceptable.
 * \arg GLX_BLUE_SIZE \e n:
 * Must be followed by a non-negative integer indicating the minimum number of
 * bits per blue pixel component that is acceptable.
 * \arg GLX_ALPHA_SIZE \e n:
 * Must be followed by a non-negative integer indicating the minimum number of
 * bits per alpha pixel component that is acceptable.
 * \arg GLX_STENCIL_SIZE \e n:
 * Must be followed by a non-negative integer indicating the minimum number of
 * bits per stencil value that is acceptable.
 * \arg GLX_DEPTH_SIZE \e n:
 * Must be followed by a non-negative integer indicating the minimum number of
 * bits per depth component that is acceptable.
 * \arg None:
 * This token is used to terminate the attribute list.
 *
 * \return a pointer to an #XVisualInfo object which most closely matches the
 * requirements of the attribute list. If there is no visual which matches the
 * request, \c NULL will be returned.
 *
 * \note Visuals with accumulation buffers are not available.
 *
 * This function searches the list of available visual configurations in
 * MiniGLXDisplayRec::configs for a configuration which best matches the GLX
 * attribute list parameter.  A new ::XVisualInfo object is created which
 * describes the visual configuration.  The match criteria is described in the
 * specification.
 */
XVisualInfo*
glXChooseVisual( Display *dpy, int screen, int *attribList )
{
   const __GLcontextModes *mode;
   Visual *vis;
   XVisualInfo *visInfo;
   const int *attrib;
   GLboolean rgbFlag = GL_FALSE, dbFlag = GL_FALSE, stereoFlag = GL_FALSE;
   GLint redBits = 0, greenBits = 0, blueBits = 0, alphaBits = 0;
   GLint indexBits = 0, depthBits = 0, stencilBits = 0;
   GLint numSamples = 0;
   int i=0;

   /*
    * XXX in the future, <screen> might be interpreted as a VT
    */
   ASSERT(dpy);
   ASSERT(screen == 0);

   vis = (Visual *)calloc(1, sizeof(Visual));
   if (!vis)
      return NULL;

   visInfo = (XVisualInfo *)malloc(sizeof(XVisualInfo));
   if (!visInfo) {
      free(vis);
      return NULL;
   }

   visInfo->visual = vis;
   vis->visInfo = visInfo;
   vis->dpy = dpy;

   /* parse the attribute list */
   for (attrib = attribList; attrib && *attrib != None; attrib++) {
      switch (attrib[0]) {
      case GLX_DOUBLEBUFFER:
         dbFlag = GL_TRUE;
         break;
      case GLX_RGBA:
         rgbFlag = GL_TRUE;
         break;
      case GLX_RED_SIZE:
         redBits = attrib[1];
         attrib++;
         break;
      case GLX_GREEN_SIZE:
         greenBits = attrib[1];
         attrib++;
         break;
      case GLX_BLUE_SIZE:
         blueBits = attrib[1];
         attrib++;
         break;
      case GLX_ALPHA_SIZE:
         alphaBits = attrib[1];
         attrib++;
         break;
      case GLX_STENCIL_SIZE:
         stencilBits = attrib[1];
         attrib++;
         break;
      case GLX_DEPTH_SIZE:
         depthBits = attrib[1];
         attrib++;
         break;
#if 0
      case GLX_ACCUM_RED_SIZE:
         accumRedBits = attrib[1];
         attrib++;
         break;
      case GLX_ACCUM_GREEN_SIZE:
         accumGreenBits = attrib[1];
         attrib++;
         break;
      case GLX_ACCUM_BLUE_SIZE:
         accumBlueBits = attrib[1];
         attrib++;
         break;
      case GLX_ACCUM_ALPHA_SIZE:
         accumAlphaBits = attrib[1];
         attrib++;
         break;
      case GLX_LEVEL:
         /* ignored for now */
         break;
#endif
      default:
         /* unexpected token */
         fprintf(stderr, "unexpected token in glXChooseVisual attrib list\n");
         free(vis);
         free(visInfo);
         return NULL;
      }
   }

   /* search screen configs for suitable visual */
   (void) numSamples;
   (void) indexBits;
   (void) redBits;
   (void) greenBits;
   (void) blueBits;
   (void) alphaBits;
   (void) stereoFlag;
   for ( mode = dpy->driver_modes ; mode != NULL ; mode = mode->next ) {
     i++;
      if (mode->rgbMode == rgbFlag &&
          mode->doubleBufferMode == dbFlag &&
          mode->redBits >= redBits &&
          mode->greenBits >= greenBits &&
          mode->blueBits >= blueBits &&
          mode->alphaBits >= alphaBits &&
          mode->depthBits >= depthBits &&
          mode->stencilBits >= stencilBits) {
         /* found it */
         visInfo->visualid = i;
         vis->mode = mode;
         break;
      }          
   }
   if (!vis->mode)
       return NULL;

   /* compute depth and bpp */
   if (rgbFlag) {
      /* XXX maybe support depth 16 someday */
#if defined(__cplusplus) || defined(c_plusplus)
      visInfo->c_class = TrueColor;
#else
      visInfo->class = TrueColor;
#endif
      visInfo->depth = dpy->driverContext.bpp;
      visInfo->bits_per_rgb = dpy->driverContext.bpp;
      if (dpy->driverContext.bpp == 32)
	 vis->pixelFormat = PF_B8G8R8A8;
      else
	 vis->pixelFormat = PF_B5G6R5;
   }
   else {
      /* color index mode */
#if defined(__cplusplus) || defined(c_plusplus)
      visInfo->c_class = PseudoColor;
#else
      visInfo->class = PseudoColor;
#endif
      visInfo->depth = 8;
      visInfo->bits_per_rgb = 8;  /* bits/pixel */
      vis->pixelFormat = PF_CI8;
   }

   return visInfo;
}


/**
 * \brief Return information about GLX visuals.
 *
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param vis the visual to be queried, as returned by glXChooseVisual().
 * \param attrib the visual attribute to be returned.
 * \param value pointer to an integer in which the result of the query will be
 * stored.
 * 
 * \return zero if no error occurs, \c GLX_INVALID_ATTRIBUTE if the attribute
 * parameter is invalid, or \c GLX_BAD_VISUAL if the \p vis parameter is
 * invalid.
 *
 * Returns the appropriate attribute of ::__GLXvisualConfig pointed by
 * MiniGLXVisualRec::glxConfig of XVisualInfo::visual.
 *
 * \sa data types.
 */
int
glXGetConfig( Display *dpy, XVisualInfo *vis, int attrib, int *value )
{
   const __GLcontextModes *mode = vis->visual->mode;
   if (!mode) {
      *value = 0;
      return GLX_BAD_VISUAL;
   }

   switch (attrib) {
   case GLX_USE_GL:
      *value = True;
      return 0;
   case GLX_RGBA:
      *value = mode->rgbMode;
      return 0;
   case GLX_DOUBLEBUFFER:
      *value = mode->doubleBufferMode;
      return 0;
   case GLX_RED_SIZE:
      *value = mode->redBits;
      return 0;
   case GLX_GREEN_SIZE:
      *value = mode->greenBits;
      return 0;
   case GLX_BLUE_SIZE:
      *value = mode->blueBits;
      return 0;
   case GLX_ALPHA_SIZE:
      *value = mode->alphaBits;
      return 0;
   case GLX_DEPTH_SIZE:
      *value = mode->depthBits;
      return 0;
   case GLX_STENCIL_SIZE:
      *value = mode->stencilBits;
      return 0;
   default:
      *value = 0;
      return GLX_BAD_ATTRIBUTE;
   }
   return 0;
}


/**
 * \brief Create a new GLX rendering context.
 *
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param vis the visual that defines the frame buffer resources available to
 * the rendering context, as returned by glXChooseVisual().
 * \param shareList If non-zero, texture objects and display lists are shared
 * with the named rendering context. If zero, texture objects and display lists
 * will (initially) be private to this context. They may be shared when a
 * subsequent context is created.
 * \param direct whether direct or indirect rendering is desired. For Mini GLX
 * this value is ignored but it should be set to \c True.
 *
 * \return a ::GLXContext handle if it succeeds or zero if it fails due to
 * invalid parameter or insufficient resources.
 *
 * This function creates and initializes a ::MiniGLXContextRec structure and
 * calls the __DRIscreenRec::createContext method to initialize the client
 * private data.
 */ 
GLXContext
glXCreateContext( Display *dpy, XVisualInfo *vis,
                        GLXContext shareList, Bool direct )
{
   GLXContext ctx;
   void *sharePriv;

   ASSERT(vis);

   ctx = (struct MiniGLXContextRec *)calloc(1, sizeof(struct MiniGLXContextRec));
   if (!ctx)
      return NULL;

   ctx->vid = vis->visualid;
 
   if (shareList)
      sharePriv = shareList->driContext.private;
   else
      sharePriv = NULL;
  
   ctx->driContext.mode = vis->visual->mode;
   ctx->driContext.private = dpy->driScreen.createNewContext(dpy, vis->visual->mode,
           GLX_WINDOW_BIT, sharePriv, &ctx->driContext);

   if (!ctx->driContext.private) {
      free(ctx);
      return NULL;
   }

   return ctx;
}


/**
 * \brief Destroy a GLX context.
 *
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param ctx the GLX context to be destroyed.
 * 
 * This function frees the \p ctx parameter after unbinding the current context
 * by calling the __DRIcontextRec::bindContext method with zeros and calling
 * the __DRIcontextRec::destroyContext method.
 */
void
glXDestroyContext( Display *dpy, GLXContext ctx )
{
   GLXContext glxctx = glXGetCurrentContext();

   if (ctx) {
      if (glxctx == ctx) {
         /* destroying current context */
         ctx->driContext.bindContext(dpy, 0, 0, 0, 0);
	 CurrentContext = 0;
      }
      ctx->driContext.destroyContext(dpy, 0, ctx->driContext.private);
      free(ctx);
   }
}


/**
 * \brief Bind a GLX context to a window or a pixmap.
 *
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param drawable the window or drawable to bind to the rendering context.
 * This should be the value returned by XCreateWindow().
 * \param ctx the GLX context to be destroyed.
 *
 * \return \c True if it succeeds, \c False otherwise to indicate an invalid
 * display, window or context parameter.
 *
 * The current rendering context may be unbound by calling glXMakeCurrent()
 * with the window and context parameters set to zero.
 * 
 * An application may create any number of rendering contexts and bind them as
 * needed. Note that binding a rendering context is generally not a
 * light-weight operation.  Most simple OpenGL applications create only one
 * rendering context.
 *
 * This function first unbinds any old context via
 * __DRIcontextRec::unbindContext and binds the new one via
 * __DRIcontextRec::bindContext.
 *
 * If \p drawable is zero it unbinds the GLX context by calling
 * __DRIcontextRec::bindContext with zeros.
 */
Bool
glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
   if (dpy && drawable && ctx) {
      GLXContext oldContext = glXGetCurrentContext();
      GLXDrawable oldDrawable = glXGetCurrentDrawable();
      /* unbind old */
      if (oldContext) {
         oldContext->driContext.unbindContext(dpy, 0,
                 (__DRIid) oldDrawable, (__DRIid) oldDrawable,
                 &oldContext->driContext);
      }
      /* bind new */
      CurrentContext = ctx;
      ctx->driContext.bindContext(dpy, 0, (__DRIid) drawable,
              (__DRIid) drawable, &ctx->driContext);
      ctx->drawBuffer = drawable;
      ctx->curBuffer = drawable;
   }
   else if (ctx && dpy) {
      /* unbind */
      ctx->driContext.bindContext(dpy, 0, 0, 0, 0);
   }
   else if (dpy) {
      CurrentContext = 0;	/* kw:  this seems to be intended??? */
   }

   return True;
}


/**
 * \brief Exchange front and back buffers.
 * 
 * \param dpy the display handle, as returned by XOpenDisplay().
 * \param drawable the drawable whose buffers are to be swapped.
 * 
 * Any pending rendering commands will be completed before the buffer swap
 * takes place.
 * 
 * Calling glXSwapBuffers() on a window which is single-buffered has no effect.
 *
 * This function just calls the __DRIdrawableRec::swapBuffers method to do the
 * work.
 */
void
glXSwapBuffers( Display *dpy, GLXDrawable drawable )
{
   if (!dpy || !drawable)
      return;

   drawable->driDrawable.swapBuffers(dpy, drawable->driDrawable.private);
}


/**
 * \brief Return the current context
 *
 * \return the current context, as specified by glXMakeCurrent(), or zero if no
 * context is currently bound.
 *
 * \sa glXCreateContext(), glXMakeCurrent()
 *
 * Returns the value of the ::CurrentContext global variable.
 */
GLXContext
glXGetCurrentContext( void )
{
   return CurrentContext;
}


/**
 * \brief Return the current drawable.
 *
 * \return the current drawable, as specified by glXMakeCurrent(), or zero if
 * no drawable is currently bound.
 *
 * This function gets the current context via glXGetCurrentContext() and
 * returns the MiniGLXContextRec::drawBuffer attribute.
 */
GLXDrawable
glXGetCurrentDrawable( void )
{
   GLXContext glxctx = glXGetCurrentContext();
   if (glxctx)
      return glxctx->drawBuffer;
   else
      return NULL;
}


static GLboolean
__glXCreateContextWithConfig(__DRInativeDisplay *dpy, int screen,
        int fbconfigID, void *contextID, drm_context_t *hHWContext)
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;

    pDRIScreen = __glXFindDRIScreen(dpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }

    psp = (__DRIscreenPrivate *) pDRIScreen->private;

    if (psp->fd) {
        if (drmCreateContext(psp->fd, hHWContext)) {
            fprintf(stderr, ">>> drmCreateContext failed\n");
            return GL_FALSE;
        }
        *(void**)contextID = (void*) *hHWContext;
    }

    return GL_TRUE;
}


static GLboolean
__glXGetDrawableInfo(__DRInativeDisplay *dpy, int scrn,
        __DRIid draw, unsigned int * index, unsigned int * stamp,
        int * x, int * y, int * width, int * height,
        int * numClipRects, drm_clip_rect_t ** pClipRects,
        int * backX, int * backY,
        int * numBackClipRects, drm_clip_rect_t ** pBackClipRects)
{
    GLXDrawable drawable = (GLXDrawable) draw;
    drm_clip_rect_t * cliprect;
    Display* display = (Display*)dpy;
    __DRIcontextPrivate *pcp = (__DRIcontextPrivate *)CurrentContext->driContext.private;
    if (drawable == 0) {
        return GL_FALSE;
    }

    cliprect = (drm_clip_rect_t*) _mesa_malloc(sizeof(drm_clip_rect_t));
    cliprect->x1 = drawable->x;
    cliprect->y1 = drawable->y;
    cliprect->x2 = drawable->x + drawable->w;
    cliprect->y2 = drawable->y + drawable->h;
    
    /* the drawable index is by client id */
    *index = display->clientID;

    *stamp = pcp->driScreenPriv->pSAREA->drawableTable[display->clientID].stamp;
    *x = drawable->x;
    *y = drawable->y;
    *width = drawable->w;
    *height = drawable->h;
    *numClipRects = 1;
    *pClipRects = cliprect;
    
    *backX = drawable->x;
    *backY = drawable->y;
    *numBackClipRects = 0;
    *pBackClipRects = 0;

    return GL_TRUE;
}


static GLboolean
xf86DRI_DestroyContext(__DRInativeDisplay *dpy, int screen, __DRIid context_id )
{
    return GL_TRUE;
}


static GLboolean
xf86DRI_CreateDrawable(__DRInativeDisplay *dpy, int screen, __DRIid drawable,
        drm_drawable_t *hHWDrawable )
{
    return GL_TRUE;
}


static GLboolean
xf86DRI_DestroyDrawable(__DRInativeDisplay *dpy, int screen, __DRIid drawable)
{
    return GL_TRUE;
}


/**
 * \brief Query function address.
 *
 * The glXGetProcAddress() function will return the address of any available
 * OpenGL or Mini GLX function.
 * 
 * \param procName name of the function to be returned.
 *
 * \return If \p procName is a valid function name, a pointer to that function
 * will be returned.  Otherwise, \c NULL will be returned.
 *
 * The purpose of glXGetProcAddress() is to facilitate using future extensions
 * to OpenGL or Mini GLX. If a future version of the library adds new extension
 * functions they'll be accessible via glXGetProcAddress(). The alternative is
 * to hard-code calls to the new functions in the application but doing so will
 * prevent linking the application with older versions of the library.
 * 
 * Returns the function address by looking up its name in a static (name,
 * address) pair list.
 */
void (*glXGetProcAddress(const GLubyte *procname))( void ) 
{
   struct name_address {
      const char *name;
      const void *func;
   };
   static const struct name_address functions[] = {
      { "glXChooseVisual", (void *) glXChooseVisual },
      { "glXCreateContext", (void *) glXCreateContext },
      { "glXDestroyContext", (void *) glXDestroyContext },
      { "glXMakeCurrent", (void *) glXMakeCurrent },
      { "glXSwapBuffers", (void *) glXSwapBuffers },
      { "glXGetCurrentContext", (void *) glXGetCurrentContext },
      { "glXGetCurrentDrawable", (void *) glXGetCurrentDrawable },
      { "glXGetProcAddress", (void *) glXGetProcAddress },
      { "XOpenDisplay", (void *) XOpenDisplay },
      { "XCloseDisplay", (void *) XCloseDisplay },
      { "XCreateWindow", (void *) XCreateWindow },
      { "XDestroyWindow", (void *) XDestroyWindow },
      { "XMapWindow", (void *) XMapWindow },
      { "XCreateColormap", (void *) XCreateColormap },
      { "XFreeColormap", (void *) XFreeColormap },
      { "XFree", (void *) XFree },
      { "XGetVisualinfo", (void *) XGetVisualInfo },
      { "glXCreatePbuffer", (void *) glXCreatePbuffer },
      { "glXDestroyPbuffer", (void *) glXDestroyPbuffer },
      { "glXChooseFBConfig", (void *) glXChooseFBConfig },
      { "glXGetVisualFromFBConfig", (void *) glXGetVisualFromFBConfig },
      { NULL, NULL }
   };
   const struct name_address *entry;
   for (entry = functions; entry->name; entry++) {
      if (strcmp(entry->name, (const char *) procname) == 0) {
         return entry->func;
      }
   }
   return _glapi_get_proc_address((const char *) procname);
}


/**
 * \brief Query the Mini GLX version.
 *
 * \param dpy the display handle. It is currently ignored, but should be the
 * value returned by XOpenDisplay().
 * \param major receives the major version number of Mini GLX.
 * \param minor receives the minor version number of Mini GLX.
 *
 * \return \c True if the function succeeds, \c False if the function fails due
 * to invalid parameters.
 *
 * \sa #MINI_GLX_VERSION_1_0.
 * 
 * Returns the hard-coded Mini GLX version.
 */
Bool
glXQueryVersion( Display *dpy, int *major, int *minor )
{
   (void) dpy;
   *major = 1;
   *minor = 0;
   return True;
}


/**
 * \brief Create a new pbuffer.
 */
GLXPbuffer
glXCreatePbuffer( Display *dpy, GLXFBConfig config, const int *attribList )
{
   return NULL;
}


void
glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf )
{
   free(pbuf);
}


GLXFBConfig *
glXChooseFBConfig( Display *dpy, int screen, const int *attribList,
                   int *nitems )
{
   GLXFBConfig *f = (GLXFBConfig *) malloc(sizeof(GLXFBConfig));
   f->visInfo = glXChooseVisual( dpy, screen, (int *) attribList );
   if (f->visInfo) { 
      *nitems = 1;
      return f;
   }
   else {
      *nitems = 0;
      free(f);
      return NULL;
   }
}


XVisualInfo *
glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config )
{
   /* XVisualInfo and GLXFBConfig are the same structure */
   (void) dpy;
   return config.visInfo;
}

void *glXAllocateMemoryMESA(Display *dpy, int scrn,
                            size_t size, float readFreq,
                            float writeFreq, float priority)
{
    if (dpy->driScreen.private && dpy->driScreen.allocateMemory) {
	return (*dpy->driScreen.allocateMemory)( dpy, scrn, size,
						 readFreq, writeFreq,
						 priority );
    }

    return NULL;
}

void glXFreeMemoryMESA(Display *dpy, int scrn, void *pointer)
{
    if (dpy->driScreen.private && dpy->driScreen.freeMemory) {
	(*dpy->driScreen.freeMemory)( dpy, scrn, pointer );
    }
}

GLuint glXGetMemoryOffsetMESA( Display *dpy, int scrn,
                               const void *pointer )
{
    if (dpy->driScreen.private && dpy->driScreen.memoryOffset) {
	return (*dpy->driScreen.memoryOffset)( dpy, scrn, pointer );
    }

    return 0;
}


/**
 * Get the unadjusted system time (UST).  Currently, the UST is measured in
 * microseconds since Epoc.  The actual resolution of the UST may vary from
 * system to system, and the units may vary from release to release.
 * Drivers should not call this function directly.  They should instead use
 * \c glXGetProcAddress to obtain a pointer to the function.
 *
 * \param ust Location to store the 64-bit UST
 * \returns Zero on success or a negative errno value on failure.
 *
 * \note
 * This function was copied directly from src/glx/x11/glxcmds.c.
 */
static int __glXGetUST( int64_t * ust )
{
    struct timeval  tv;
    
    if ( ust == NULL ) {
	return -EFAULT;
    }

    if ( gettimeofday( & tv, NULL ) == 0 ) {
	ust[0] = (tv.tv_sec * 1000000) + tv.tv_usec;
	return 0;
    } else {
	return -errno;
    }
}


/**
 * 
 * \bug
 * This needs to be implemented for miniGlx.
 */
static GLboolean __glXGetMscRate(__DRInativeDisplay * dpy, __DRIid drawable,
				 int32_t * numerator, int32_t * denominator)
{
    *numerator = 0;
    *denominator = 0;
    return False;
}
/*@}*/
