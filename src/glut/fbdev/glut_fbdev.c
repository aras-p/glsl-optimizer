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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <inttypes.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/kd.h>

#include <linux/fb.h>
#include <linux/keyboard.h>
#include <linux/vt.h>

#include <GL/gl.h>
#include <GL/glfbdev.h>
#include <GL/glut.h>

#include <math.h>

#include "../../mesa/main/config.h"

#define MULTIHEAD   /* enable multihead hacks,
		       it allows the program to continue drawing
		       without reading input when a second fbdev
		       has keyboard focus it can cause
		       screen corruption that requires C-l to fix */

#define FBMODES "/etc/fb.modes"

#define HAVE_GPM

#ifdef HAVE_GPM
#include <gpm.h>
static int GpmMouse;
#endif

#define MOUSEDEV "/dev/gpmdata"

static int CurrentVT;
static int ConsoleFD = - 1;

/* save settings to restore on exit */
static int OldKDMode = -1;
static int OldMode;
struct vt_mode OldVTMode;
struct termios OldTermios;

static struct fb_fix_screeninfo FixedInfo;
static struct fb_var_screeninfo VarInfo, OrigVarInfo;
struct fb_cmap ColorMap;

static int DesiredDepth = 0;

static int FrameBufferFD = -1;
static caddr_t FrameBuffer = (caddr_t) -1;
static caddr_t BackBuffer = NULL;
static int DisplayMode;

static int AccumSize = 16; /* per channel size of accumulation buffer */
static int DepthSize = DEFAULT_SOFTWARE_DEPTH_BITS;
static int StencilSize = STENCIL_BITS;

#define MENU_FONT_WIDTH   9
#define MENU_FONT_HEIGHT 15 
#define MENU_FONT        GLUT_BITMAP_9_BY_15
#define SUBMENU_OFFSET   20

static int AttachedMenus[3];
static int ActiveMenu;
static int SelectedMenu;
static int CurrentMenu;
static int NumMenus = 1;

static struct {
    int NumItems;
    int x, y;
    int width;
    int selected;
    struct {
	int value;
	int submenu;
	char *name;
    } *Items;
    void (*func)(int);
} *Menus = NULL;

struct GlutTimer {
    int time;
    void (*func)(int);
    int value;
    struct GlutTimer *next;
};

struct GlutTimer *GlutTimers = NULL;

static struct timeval StartTime;

static int KeyboardModifiers;
static int KeyboardLedState;

static int MouseFD;
static int NumMouseButtons;
static int MouseX;
static int MouseY;
static double MouseSpeed = 0;
static int CurrentCursor = GLUT_CURSOR_LEFT_ARROW;
/* only display the mouse if there is a registered callback for it */
static int MouseEnabled = 0;

/* per window data */
static GLFBDevContextPtr Context;
static GLFBDevBufferPtr Buffer;
static GLFBDevVisualPtr Visual;
static void (*DisplayFunc)(void) = NULL;
static void (*ReshapeFunc)(int width, int height) = NULL;
static void (*KeyboardFunc)(unsigned char key, int x, int y) = NULL;
static void (*MouseFunc)(int key, int state, int x, int y) = NULL;
static void (*MotionFunc)(int x, int y) = NULL;
static void (*PassiveMotionFunc)(int x, int y) = NULL;
static void (*VisibilityFunc)(int state) = NULL;
static void (*SpecialFunc)(int key, int x, int y) = NULL;
static void (*IdleFunc)(void) = NULL;
static void (*MenuStatusFunc)(int state, int x, int y) = NULL;
static void (*MenuStateFunc)(int state) = NULL;

static int Redisplay;
static int Visible;
static int VisibleSwitch;
static int Active;
/* we have to poll to see if we are visible
   on a framebuffer that is not active */
static int VisiblePoll;
static int FramebufferIndex;

static int RequiredWidth;
static int RequiredHeight;
static int InitialWidthHint;
static int InitialHeightHint;

static char exiterror[256];

/* --------- Initialization ------------*/
/* test if the active console is attached to the same framebuffer */
static void TestVisible(void) {
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

static void VTSwitchHandler(int sig)
{
    struct vt_stat st;
    switch(sig) {
    case SIGUSR1:
	ioctl(ConsoleFD, VT_RELDISP, 1);
	Active = 0;
#ifdef MULTIHEAD
	VisiblePoll = 1;
	TestVisible();
#else
	VisibleSwitch = 1;
	Visible = 0;
#endif
	break;
    case SIGUSR2:
	ioctl(ConsoleFD, VT_GETSTATE, &st);
	if(st.v_active)
	    ioctl(ConsoleFD, VT_RELDISP, VT_ACKACQ);

	/* this is a hack to turn the cursor off */
	ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &VarInfo);

	/* restore color map */
	if(DisplayMode & GLUT_INDEX) {	
	    ColorMap.start = 0;
	    ColorMap.len = 256;

	    if (ioctl(FrameBufferFD, FBIOPUTCMAP, (void *) &ColorMap) < 0)
		fprintf(stderr, "ioctl(FBIOPUTCMAP) failed!\n");
	}

	Active = 1;
	Visible = 1;
	VisibleSwitch = 1;

	Redisplay = 1;

	break;
    }
}

static void Cleanup(void)
{
    if(ConsoleFD >= 0)
	if (tcsetattr(0, TCSANOW, &OldTermios) < 0)
	    fprintf(stderr, "tcsetattr failed\n");

    if(ConsoleFD > 0) {
	/* restore keyboard state */
	if (ioctl(ConsoleFD, VT_SETMODE, &OldVTMode) < 0)
	    fprintf(stderr, "Failed to set vtmode\n");

	if (ioctl(ConsoleFD, KDSKBMODE, OldKDMode) < 0)
	    fprintf(stderr, "ioctl KDSKBMODE failed!\n");

	if(ioctl(ConsoleFD, KDSETMODE, OldMode) < 0)
	    fprintf(stderr, "ioctl KDSETMODE failed!\n");

	close(ConsoleFD);
    }

    /* close mouse */
#ifdef HAVE_GPM
    if(GpmMouse) {
	if(NumMouseButtons)
	    Gpm_Close();
    } else
#endif
	if(MouseFD >= 0)
	    close(MouseFD);

    glFBDevMakeCurrent( NULL, NULL, NULL);

    glFBDevDestroyContext(Context);
    glFBDevDestroyBuffer(Buffer);
    glFBDevDestroyVisual(Visual);

    struct vt_mode VT;

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
    int i, j;

    for(i = 1; i<NumMenus; i++) {
	for(i = 1; i<Menus[i].NumItems; i++)
	    free(Menus[i].Items[j].name);
	free(Menus[i].Items);
    }
    free(Menus);

    if(exiterror[0])
	fprintf(stderr, "[glfbdev glut] %s", exiterror);
}

static void CrashHandler(int sig)
{
    sprintf(exiterror, "Caught signal %d, cleaning up\n", sig);
    exit(0);
}

static void InitializeVT(int usestdin)
{
    /* terminos settings for straight-through mode */
    if (tcgetattr(0, &OldTermios) < 0) {
	sprintf(exiterror, "tcgetattr failed\n");
	exit(0);
    }
   
    struct termios tio = OldTermios;
   
    tio.c_lflag &= ~(ICANON | ECHO  | ISIG);
    tio.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
    tio.c_iflag |= IGNBRK;
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &tio) < 0) {
	sprintf(exiterror, "tcsetattr failed\n");
	exit(0);
    }

    if(fcntl(0, F_SETFL, O_NONBLOCK) < 0) {
	sprintf(exiterror, "Failed to set keyboard to non-blocking\n");
	exit(0);
    }

    Active = 1;

    if(usestdin) {
	ConsoleFD = 0;
	return;
    }

    /* detect the current vt if it was not specified */
    if(CurrentVT == 0) {
	int fd = open("/dev/tty", O_RDWR | O_NDELAY, 0);
	struct vt_stat st;
	if(fd == -1) {
	    sprintf(exiterror, "Failed to open /dev/tty\n");
	    exit(0);
	}
	if(ioctl(fd, VT_GETSTATE, &st) == -1) {
	    fprintf(stderr, "Could not detect current vt, specify with -vt\n");
	    fprintf(stderr, "Defaulting to stdin input\n");
	    ConsoleFD = 0;
	    close(fd);
	    return;
	} else
	    CurrentVT =  st.v_active;

	close(fd);
    }
    
    /* open the console tty */
    char console[128];
    sprintf(console, "/dev/tty%d", CurrentVT);
    ConsoleFD = open(console, O_RDWR | O_NDELAY, 0);
    if (ConsoleFD < 0) {
	sprintf(exiterror, "error couldn't open %s,"
		" defaulting to stdin \n", console);
	ConsoleFD = 0;
	return;
    }

    signal(SIGUSR1, VTSwitchHandler);
    signal(SIGUSR2, VTSwitchHandler);

    struct vt_mode vt;

    if (ioctl(ConsoleFD, VT_GETMODE, &OldVTMode) < 0) {
	sprintf(exiterror,"Failed to grab %s, defaulting to stdin\n", console);
	close(ConsoleFD);
	ConsoleFD = 0;
	return;
    }

    vt = OldVTMode;

    vt.mode = VT_PROCESS;
    vt.waitv = 0;
    vt.relsig = SIGUSR1;
    vt.acqsig = SIGUSR2;
    if (ioctl(ConsoleFD, VT_SETMODE, &vt) < 0) {
	sprintf(exiterror, "error: ioctl(VT_SETMODE) failed: %s\n",
		strerror(errno));
	close(ConsoleFD);
	ConsoleFD = 0;
	exit(1);
    }

    if (ioctl(ConsoleFD, KDGKBMODE, &OldKDMode) < 0) {
	fprintf(stderr, "warning: ioctl KDGKBMODE failed!\n");
	OldKDMode = K_XLATE;
    }

    if(ioctl(ConsoleFD, KDGETMODE, &OldMode) < 0)
	sprintf(exiterror, "Warning: Failed to get terminal mode\n");

#ifdef HAVE_GPM
    if(!GpmMouse)
#endif
	if(ioctl(ConsoleFD, KDSETMODE, KD_GRAPHICS) < 0)
	    sprintf(exiterror,"Warning: Failed to set terminal to graphics\n");


    if (ioctl(ConsoleFD, KDSKBMODE, K_MEDIUMRAW) < 0) {
	sprintf(exiterror, "ioctl KDSKBMODE failed!\n");
	tcsetattr(0, TCSANOW, &OldTermios);
	exit(0);
    }

    if( ioctl(ConsoleFD, KDGKBLED, &KeyboardLedState) < 0)  {
	sprintf(exiterror, "ioctl KDGKBLED failed!\n");
	exit(0);
    }
}

static void InitializeMouse(void)
{
#ifdef HAVE_GPM
    if(GpmMouse) {
	Gpm_Connect conn;  
	int c;
	conn.eventMask  = ~0;   /* Want to know about all the events */
	conn.defaultMask = 0;   /* don't handle anything by default  */
	conn.minMod     = 0;    /* want everything                   */
	conn.maxMod     = ~0;   /* all modifiers included            */
	if(Gpm_Open(&conn, 0) == -1) {
	    fprintf(stderr, "Cannot open gpmctl. Continuing without Mouse\n");
	    return;
	}
	
	if(!MouseSpeed)
	    MouseSpeed = 5;
    } else
#endif
   {
       const char *mousedev = getenv("MOUSE");
       if(!mousedev)
	   mousedev = MOUSEDEV;
       if((MouseFD = open(mousedev, O_RDONLY)) < 0) {
	   fprintf(stderr,"Cannot open %s.\n"
		   "Continuing without Mouse\n", MOUSEDEV);
	   return;
       }

       if(!MouseSpeed)
	   MouseSpeed = 1;
   }

    NumMouseButtons = 3;
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
	    fprintf(stderr, "gpm support was not compiled\n");
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

/* --------- Mouse Rendering ------------*/
#include "cursors.h"
static int LastMouseX;
static int LastMouseY;
static unsigned char *MouseBuffer;

static void EraseCursor(void)
{
    int off = LastMouseY * FixedInfo.line_length 
	+ LastMouseX * VarInfo.bits_per_pixel / 8;
    int stride = CURSOR_WIDTH * VarInfo.bits_per_pixel / 8;
    int i;

    unsigned char *src = MouseBuffer;

    for(i = 0; i<CURSOR_HEIGHT; i++) {
	memcpy(BackBuffer + off, src, stride);
	src += stride;
	off += FixedInfo.line_length;
    }
}

static void SaveCursor(int x, int y)
{
    if(x < 0)
	LastMouseX = 0;
    else
    if(x > (int)VarInfo.xres - CURSOR_WIDTH)
	LastMouseX = VarInfo.xres - CURSOR_WIDTH;
    else
	LastMouseX = x;

    if(y < 0)
	LastMouseY = 0;
    else
    if(y > (int)VarInfo.yres - CURSOR_HEIGHT)
	LastMouseY = VarInfo.yres - CURSOR_HEIGHT;
    else
	LastMouseY = y;

    int off = LastMouseY * FixedInfo.line_length 
	+ LastMouseX * VarInfo.bits_per_pixel / 8;
    int stride = CURSOR_WIDTH * VarInfo.bits_per_pixel / 8;
    int i;
    unsigned char *src = MouseBuffer;
    for(i = 0; i<CURSOR_HEIGHT; i++) {
	memcpy(src, BackBuffer + off, stride);
	src += stride;
	off += FixedInfo.line_length;
    }
}

static void DrawCursor(void)
{
    if(CurrentCursor < 0 || CurrentCursor >= NUM_CURSORS)
	return;

    int px = MouseX - CursorsXOffset[CurrentCursor];
    int py = MouseY - CursorsYOffset[CurrentCursor];

    SaveCursor(px, py);

    int xoff = 0;
    if(px < 0)
	xoff = -px;

    int xlen = CURSOR_WIDTH;
    if(px + CURSOR_WIDTH > VarInfo.xres)
	xlen = VarInfo.xres - px;

    int yoff = 0;
    if(py < 0)
	yoff = -py;

    int ylen = CURSOR_HEIGHT;
    if(py + CURSOR_HEIGHT > VarInfo.yres)
	ylen = VarInfo.yres - py;

    int bypp = VarInfo.bits_per_pixel / 8;

    unsigned char *c = BackBuffer + FixedInfo.line_length * (py + yoff)
	+ (px + xoff) * bypp;

    unsigned char *d = Cursors[CurrentCursor] + (CURSOR_WIDTH * yoff + xoff)*4;
    int i, j;

    int dstride = (CURSOR_WIDTH - xlen + xoff) * 4;
    int cstride = FixedInfo.line_length - bypp * (xlen - xoff);

    switch(bypp) {
    case 1: /* no support for 8bpp mouse yet */
	break;
    case 2:
	{
	    uint16_t *e = (void*)c;
	    cstride /= 2;
	    for(i = yoff; i < ylen; i++) {
		for(j = xoff; j < xlen; j++) {
		    e[0] = ((((d[0] + (((int)(((e[0] >> 8) & 0xf8) 
			   | ((c[0] >> 11) & 0x7)) * d[3]) >> 8)) & 0xf8) << 8)
			 | (((d[1] + (((int)(((e[0] >> 3) & 0xfc)
			   | ((e[0] >> 5) & 0x3)) * d[3]) >> 8)) & 0xfc) << 3)
			 | ((d[2] + (((int)(((e[0] << 3) & 0xf8)
			   | (e[0] & 0x7)) * d[3]) >> 8)) >> 3));
		
		    e++;
		    d+=4;
		}
		d += dstride;
		e += cstride;
	    }
	}
	break;
    case 3:
    case 4:
	for(i = yoff; i < ylen; i++) {
	    for(j = xoff; j < xlen; j++) {
		c[0] = d[0] + (((int)c[0] * d[3]) >> 8);
		c[1] = d[1] + (((int)c[1] * d[3]) >> 8);
		c[2] = d[2] + (((int)c[2] * d[3]) >> 8);
		
		c+=bypp;
		d+=4;
	    }
	    d += dstride;
	    c += cstride;
	} break;
    }
}

#define MIN(x, y) x < y ? x : y
static void SwapCursor(void)
{
    int px = MouseX - CursorsXOffset[CurrentCursor];
    int py = MouseY - CursorsYOffset[CurrentCursor];

    int minx = MIN(px, LastMouseX);
    int sizex = abs(px - LastMouseX);

    int miny = MIN(py, LastMouseY);
    int sizey = abs(py - LastMouseY);

    DrawCursor();
    /* now update the portion of the screen that has changed */

    if(DisplayMode & GLUT_DOUBLE && (sizex || sizey)) {
	if(minx < 0)
	    minx = 0;
	if(miny < 0)
	    miny = 0;
	
	if(minx + sizex > VarInfo.xres)
	    sizex = VarInfo.xres - minx;
	if(miny + sizey > VarInfo.yres)
	    sizey = VarInfo.yres - miny;
	int off = FixedInfo.line_length * miny
	    + minx * VarInfo.bits_per_pixel / 8;
	int stride = (sizex + CURSOR_WIDTH) * VarInfo.bits_per_pixel / 8;
	int i;
	for(i = 0; i< sizey + CURSOR_HEIGHT; i++) {
	    memcpy(FrameBuffer+off, BackBuffer+off, stride);
	    off += FixedInfo.line_length;
	}
    }
}

/* --------- Menu Rendering ------------*/
static double MenuProjection[16];
static double MenuModelview[16];

static void InitMenuMatrices(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0,VarInfo.xres,VarInfo.yres,0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0,0,VarInfo.xres,VarInfo.yres);
    glGetDoublev(GL_PROJECTION_MATRIX, MenuProjection);
    glGetDoublev(GL_MODELVIEW_MATRIX, MenuModelview);
}

static int DrawMenu(int menu, int x, int *y)
{
    int i;
    int ret = 1;
    for(i=0; i < Menus[menu].NumItems; i++) {
	char *s = Menus[menu].Items[i].name;
	int a =0;
	if(MouseY >= *y && MouseY < *y + MENU_FONT_HEIGHT &&
	   MouseX >= x && MouseX < x + Menus[menu].width) {
	    a = 1;
	    SelectedMenu = menu;
	    ret = 0;
	    Menus[menu].selected = i;
	    glColor3f(1,0,0);
	} else
	    glColor3f(0,0,1);

	*y += MENU_FONT_HEIGHT;
	glRasterPos2i(x, *y);
	for(; *s; s++)
	    glutBitmapCharacter(MENU_FONT, *s);

	if(Menus[menu].selected == i)
	    if(Menus[menu].Items[i].submenu) 
		if(DrawMenu(Menus[menu].Items[i].submenu, x 
			    + SUBMENU_OFFSET, y)) {
		    if(!a)
			Menus[menu].selected = -1;
		} else
		    ret = 0;
    }
    return ret;
}

static void DrawMenus(void)
{
    /* save old settings */
    glPushAttrib(-1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(MenuModelview);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(MenuProjection);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    // glEnable(GL_LOGIC_OP);
    //glEnable(GL_COLOR_LOGIC_OP);
    //    glLogicOp(GL_XOR);
    
    int x = Menus[ActiveMenu].x;
    int y = Menus[ActiveMenu].y;

    if(DrawMenu(ActiveMenu, x, &y))
	Menus[ActiveMenu].selected = -1;
    
    /* restore settings */

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

/* --------- Event Processing ------------*/
#define MODIFIER(mod) \
    KeyboardModifiers = release ? KeyboardModifiers & ~mod   \
                                : KeyboardModifiers | mod;

#define READKEY read(ConsoleFD, &code, 1)

static void LedModifier(int led, int release)
{
    static int releaseflag = K_CAPS | K_NUM;    
    if(release)
	releaseflag |= led;
	else
	    if(releaseflag & led) {
		KeyboardLedState ^= led;
		releaseflag &= ~led;
	    }
    ioctl(ConsoleFD, KDSKBLED, KeyboardLedState);
    ioctl(ConsoleFD, KDSETLED, 0x80);
}

static int ReadKey(void)
{
    int x;
    unsigned char code;
    int specialkey = 0;
    if(READKEY == 0)
	return 0;

    if(code == 0)
	return 0;

    /* stdin input escape code based */
    if(ConsoleFD == 0) {
	KeyboardModifiers = 0;
    altset:
	if(code == 27 && READKEY == 1) {
	    switch(code) {
	    case 79: /* function key */
		READKEY;
		if(code == 50) {
		    READKEY;
		shiftfunc:
		    KeyboardModifiers |= GLUT_ACTIVE_SHIFT;
		    specialkey = GLUT_KEY_F1 + code - 53;
		    READKEY;
		} else {
		    READKEY;
		    specialkey = GLUT_KEY_F1 + code - 80;		    
		}
		break;
	    case 91:
		READKEY;
		switch(code) {
		case 68:
		    specialkey = GLUT_KEY_LEFT; break;
		case 65:
		    specialkey = GLUT_KEY_UP; break;
		case 67:
		    specialkey = GLUT_KEY_RIGHT; break;
		case 66:
		    specialkey = GLUT_KEY_DOWN; break;
		case 53:
		    specialkey = GLUT_KEY_PAGE_UP; READKEY; break;
		case 54:
		    specialkey = GLUT_KEY_PAGE_DOWN; READKEY; break;
		case 49:
		    specialkey = GLUT_KEY_HOME; READKEY; break;
		case 52:
		    specialkey = GLUT_KEY_END; READKEY; break;
		case 50:
		    READKEY;
		    if(code != 126)
			goto shiftfunc;
		    specialkey = GLUT_KEY_INSERT;
		    break; 
		case 51:
		    code = '\b'; goto stdkey;
		case 91:
		    READKEY;
		    specialkey = GLUT_KEY_F1 + code - 65;
		    break;
		default:
		    return 0;
		}
		break;
	    default:
		KeyboardModifiers |= GLUT_ACTIVE_ALT;
		goto altset;
	    }
	}
    stdkey:
	if(specialkey) {
	    if(SpecialFunc)
		SpecialFunc(specialkey, MouseX, MouseY);
	} else {
	    if(code >= 1 && code <= 26) {
		KeyboardModifiers |= GLUT_ACTIVE_CTRL;
		code += 'a' - 1;
	    }
	    if((code >= 43 && code <= 34) || (code == 60)
	       || (code >= 62 && code <= 90) || (code == 94)
	       || (code == 95)  || (code >= 123 && code <= 126))
		KeyboardModifiers |= GLUT_ACTIVE_SHIFT;

	    if(KeyboardFunc)
		KeyboardFunc(code, MouseX, MouseY);
	}
	return 1;
    }

    /* linux kbd reading */
    struct kbentry entry; 
    entry.kb_table = 0;
    if(KeyboardModifiers & GLUT_ACTIVE_SHIFT)
	entry.kb_table |= K_SHIFTTAB;
	    
    int release = code & 0x80;
    code &= 0x7F;
	    
    entry.kb_index = code;
	
    if (ioctl(ConsoleFD, KDGKBENT, &entry) < 0) {
	sprintf(exiterror, "ioctl(KDGKBENT) failed.\n");
	exit(0);
    }

    int labelval = entry.kb_value;

    switch(labelval) {
    case K_SHIFT:
    case K_SHIFTL:
	MODIFIER(GLUT_ACTIVE_SHIFT);
	return 0;
    case K_CTRL:
	MODIFIER(GLUT_ACTIVE_CTRL);
	return 0;
    case K_ALT:
    case K_ALTGR:
	MODIFIER(GLUT_ACTIVE_ALT);
	return 0;
    }

    if(!release && labelval >= K_F1 && labelval <= K_F12)
	if(KeyboardModifiers & GLUT_ACTIVE_ALT) {
	    /* VT switch, we must do it */
	    if(ioctl(ConsoleFD, VT_ACTIVATE, labelval - K_F1 + 1) < 0)
		sprintf(exiterror, "Error switching console\n");
	    return 0;
	}

    switch(labelval) {
    case K_CAPS:
	LedModifier(LED_CAP, release);
	return 0;
    case K_NUM:
	LedModifier(LED_NUM, release);
	return 0;
    case K_HOLD: /* scroll lock suspends glut */
	LedModifier(LED_SCR, release);
	while(KeyboardLedState & LED_SCR) {
	    usleep(10000);
	    ReadKey();
	}
	return 0;
    }

    /* we could queue keypresses here */
    if(KeyboardLedState & LED_SCR)
	return 0;

    if(release)
	return 0;

    if(labelval >= K_F1 && labelval <= K_F12)
	specialkey = GLUT_KEY_F1 + labelval - K_F1;
    else
	switch(labelval) {
	case K_LEFT:
	    specialkey = GLUT_KEY_LEFT; break;
	case K_UP:
	    specialkey = GLUT_KEY_UP; break;
	case K_RIGHT:
	    specialkey = GLUT_KEY_RIGHT; break;
	case K_DOWN:
	    specialkey = GLUT_KEY_DOWN; break;
	case K_PGUP:
	    specialkey = GLUT_KEY_PAGE_UP; break;
	case K_PGDN:
	    specialkey = GLUT_KEY_PAGE_DOWN; break;
	case K_FIND:
	    specialkey = GLUT_KEY_HOME; break;
	case K_SELECT:
	    specialkey = GLUT_KEY_END; break;
	case K_INSERT:
	    specialkey = GLUT_KEY_INSERT; break; 
	case K_REMOVE:
	    labelval = '\b'; break;
	case K_ENTER:
	    labelval = '\n'; break;
	}

    if(specialkey) {
	if(SpecialFunc)
	    SpecialFunc(specialkey, MouseX, MouseY);
    } else
	if(KeyboardFunc) {
	    char c = labelval;
	    if(KeyboardLedState & LED_CAP) {
		if(c >= 'A' && c <= 'Z')
		    c += 'a' - 'A';
		else
		if(c >= 'a' && c <= 'z')
		    c += 'A' - 'a';
	    }
	    KeyboardFunc(c, MouseX, MouseY);
	}
    return 1;
}

static void HandleMousePress(int button, int pressed)
{
    if(ActiveMenu && !pressed) {
	if(MenuStatusFunc)
	    MenuStatusFunc(GLUT_MENU_NOT_IN_USE, MouseX, MouseY);
	if(MenuStateFunc)
	    MenuStateFunc(GLUT_MENU_NOT_IN_USE);
	if(SelectedMenu > 0) {
	    int selected = Menus[SelectedMenu].selected;
	    if(selected >= 0)
		if(Menus[SelectedMenu].Items[selected].submenu == 0)
		    Menus[SelectedMenu].func(Menus[SelectedMenu].Items
					     [selected].value);
	}
	ActiveMenu = 0;
	Redisplay = 1;
	return;
    }

    if(AttachedMenus[button] && pressed) {
	ActiveMenu = AttachedMenus[button];
	if(MenuStatusFunc)
	    MenuStatusFunc(GLUT_MENU_IN_USE, MouseX, MouseY);
	if(MenuStateFunc)
	    MenuStateFunc(GLUT_MENU_IN_USE);
	Menus[ActiveMenu].x = MouseX - Menus[ActiveMenu].width/2;
	Menus[ActiveMenu].y = MouseY - Menus[ActiveMenu].NumItems*MENU_FONT_HEIGHT/2;
	Menus[ActiveMenu].selected = -1;
	Redisplay = 1;
	return;
    }
    
    if(MouseFunc)
	MouseFunc(button, pressed ? GLUT_DOWN : GLUT_UP, MouseX, MouseY);
}

static int ReadMouse(void)
{
    int l, r, m;
    static int ll, lm, lr;
    signed char dx, dy;

#ifdef HAVE_GPM
    if(GpmMouse) {
	Gpm_Event event;
	struct pollfd pfd;
	pfd.fd = gpm_fd;
	pfd.events = POLLIN;
	if(poll(&pfd, 1, 1) != 1)
	    return 0;
	
	if(Gpm_GetEvent(&event) != 1)
	    return 0;
	
	l = event.buttons & GPM_B_LEFT;
	m = event.buttons & GPM_B_MIDDLE;
	r = event.buttons & GPM_B_RIGHT;

	/* gpm is weird in that it gives a button number when the button
	   is released, with type set to GPM_UP, this is only a problem
	   if it is the last button released */
    
	if(event.type & GPM_UP)
	    if(event.buttons == GPM_B_LEFT || event.buttons == GPM_B_MIDDLE ||
	       event.buttons == GPM_B_RIGHT || event.buttons == GPM_B_FOURTH)
		l = m = r = 0;

	dx = event.dx;
	dy = event.dy;
    } else
#endif
    {
	if(MouseFD == -1)
	    return 0;

	if(fcntl(MouseFD, F_SETFL, O_NONBLOCK) == -1) {
	    close(MouseFD);
	    MouseFD = -1;
	    return 0;
	}

	char data[4];
	if(read(MouseFD, data, 4) != 4)
	    return 0;
	
	l = ((data[0] & 0x20) >> 3);
	m = ((data[3] & 0x10) >> 3);
	r = ((data[0] & 0x10) >> 4);

	dx = (((data[0] & 0x03) << 6) | (data[1] & 0x3F));
	dy = (((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
    }

    MouseX += dx * MouseSpeed;
    if(MouseX < 0)
	MouseX = 0;
    else
	if(MouseX >= VarInfo.xres)
	    MouseX = VarInfo.xres - 1;

    MouseY += dy * MouseSpeed;
    if(MouseY < 0)
	MouseY = 0;
    else
	if(MouseY >= VarInfo.yres)
	    MouseY = VarInfo.yres - 1;

    if(l != ll)
	HandleMousePress(GLUT_LEFT_BUTTON, l);
    if(m != lm)
	HandleMousePress(GLUT_MIDDLE_BUTTON, m);
    if(r != lr)
	HandleMousePress(GLUT_RIGHT_BUTTON, r);

    ll = l, lm = m, lr = r;

    if(dx || dy) {
	if(l || m || r) {
	    if(MotionFunc)
		MotionFunc(MouseX, MouseY);
	} else
	    if(PassiveMotionFunc)
		PassiveMotionFunc(MouseX, MouseY);

	EraseCursor();
	if(ActiveMenu)
	    Redisplay = 1;
	else
	    SwapCursor();
    }

    return 1;
}

static void RecieveEvents(void)
{
    while(ReadKey());
    
    if(MouseEnabled)
	while(ReadMouse());
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
	    RecieveEvents();
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

/* ---------- Window Management ----------*/
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

int glutCreateWindow (const char *title)
{
    if(ConsoleFD == -1) {
	int argc = 0;
	char *argv[] = {NULL};
	glutInit(&argc, argv);
    }

    if(Context)
	return 0;

    char *fbdev = getenv("FRAMEBUFFER");
    if(fbdev) {
#ifdef MULTIHEAD
	if(!sscanf(fbdev, "/dev/fb%d", &FramebufferIndex))
	    if(!sscanf(fbdev, "/dev/fb/%d", &FramebufferIndex))
		sprintf(exiterror, "Could not determine Framebuffer index!\n");
#endif
    } else {
	static char fb[128];
	FramebufferIndex = 0;
	struct fb_con2fbmap confb;
	int fd = open("/dev/fb0", O_RDWR);
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

    if(DisplayMode & GLUT_INDEX) {
	/* initialize colormap */
	if (FixedInfo.visual != FB_VISUAL_DIRECTCOLOR) {
	    static unsigned short red[256], green[256], blue[256];
	    /* we're assuming 256 entries here */

	    ColorMap.start = 0;
	    ColorMap.len = 256;
	    ColorMap.red   = red;
	    ColorMap.green = green;
	    ColorMap.blue  = blue;
	    ColorMap.transp = NULL;

	    if (ioctl(FrameBufferFD, FBIOGETCMAP, (void *) &ColorMap) < 0)
		sprintf(exiterror, "ioctl(FBIOGETCMAP) failed!\n");

	} else {
	    sprintf(exiterror, "error: Could not set 8 bit color mode\n");
	    exit(0);
	}
    }

    /* mmap the framebuffer into our address space */
    FrameBuffer = mmap(0, FixedInfo.smem_len, PROT_READ | PROT_WRITE, 
		       MAP_SHARED, FrameBufferFD, 0);
    if (FrameBuffer == MAP_FAILED) {
	sprintf(exiterror, "error: unable to mmap framebuffer: %s\n",
		strerror(errno));
	exit(0);
    }

    int attribs[9];
    int i;

    int mask = DisplayMode;
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

    int size = VarInfo.xres_virtual * VarInfo.yres_virtual
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

    Visible = 1;
    VisibleSwitch = 1;
    Redisplay = 1;

    /* set up mouse */
    if((MouseBuffer = malloc(CURSOR_WIDTH * CURSOR_HEIGHT
			     * VarInfo.bits_per_pixel / 8)) == NULL) {
	sprintf(exiterror, "malloc failure\n");
	exit(0);
    }

    MouseX = VarInfo.xres / 2;
    MouseY = VarInfo.yres / 2;

    /* set up menus */
    InitMenuMatrices();
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

void glutSwapBuffers(void)
{
    glFlush();

    if(DisplayMode & GLUT_DOUBLE) {
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

void glutIconifyWindow(void)
{
}

void glutSetWindowTitle(const char *name)
{
}

void glutSetIconTitle(const char *name)
{
}

void glutSetCursor(int cursor)
{
    if(cursor == GLUT_CURSOR_FULL_CROSSHAIR)
	cursor = GLUT_CURSOR_CROSSHAIR;
    CurrentCursor = cursor;
    MouseEnabled = 1;
    EraseCursor();
    SwapCursor();
}

/* --------- Overlays ------------*/
void glutEstablishOverlay(void)
{
    exit(0);
}

void glutUseLayer(GLenum layer)
{
}

void glutRemoveOverlay(void)
{
}

void glutPostOverlayRedisplay(void)
{
}

void glutShowOverlay(void)
{
}

void glutHideOverlay(void)
{
}

/* --------- Menus ------------*/
int glutCreateMenu(void (*func)(int value))
{
    MouseEnabled = 1;
    CurrentMenu = NumMenus;
    NumMenus++;
    Menus = realloc(Menus, sizeof(*Menus) * NumMenus);
    Menus[CurrentMenu].NumItems = 0;
    Menus[CurrentMenu].Items = NULL;
    Menus[CurrentMenu].func = func;
    Menus[CurrentMenu].width = 0;
    return CurrentMenu;
}

void glutSetMenu(int menu)
{
    CurrentMenu = menu;
}

int glutGetMenu(void)
{
    return CurrentMenu;
}

void glutDestroyMenu(int menu)
{
    if(menu == CurrentMenu)
	CurrentMenu = 0;
}

static void NameMenuEntry(int entry, const char *name)
{
    int cm = CurrentMenu;
    if(!(Menus[cm].Items[entry-1].name = realloc(Menus[cm].Items[entry-1].name,
						 strlen(name) + 1))) {
	sprintf(exiterror, "realloc failed in NameMenuEntry\n");
	exit(0);
    }
    strcpy(Menus[cm].Items[entry-1].name, name);
    if(strlen(name) * MENU_FONT_WIDTH > Menus[cm].width)
	Menus[cm].width = strlen(name) * MENU_FONT_WIDTH;
}

static int AddMenuItem(const char *name)
{
    int cm = CurrentMenu;
    int item = Menus[cm].NumItems++;
    if(!(Menus[cm].Items = realloc(Menus[cm].Items,
				   Menus[cm].NumItems * sizeof(*Menus[0].Items)))) {
	sprintf(exiterror, "realloc failed in AddMenuItem\n");
	exit(0);
    }
    Menus[cm].Items[item].name = NULL;
    NameMenuEntry(item+1, name);
    return item;
}

void glutAddMenuEntry(const char *name, int value)
{
    int item = AddMenuItem(name);
    Menus[CurrentMenu].Items[item].value = value;
    Menus[CurrentMenu].Items[item].submenu = 0;
}

void glutAddSubMenu(const char *name, int menu)
{
    int item = AddMenuItem(name);
    if(menu == CurrentMenu) {
	sprintf(exiterror, "Recursive menus not supported\n");
	exit(0);
    }
    Menus[CurrentMenu].Items[item].submenu = menu;
}

void glutChangeToMenuEntry(int entry, const char *name, int value)
{
    NameMenuEntry(entry, name);
    Menus[CurrentMenu].Items[entry-1].value = value;
    Menus[CurrentMenu].Items[entry-1].submenu = 0;
}

void glutChangeToSubMenu(int entry, const char *name, int menu)
{
    NameMenuEntry(entry, name);
    Menus[CurrentMenu].Items[entry-1].submenu = menu;
}

void glutRemoveMenuItem(int entry)
{
    memmove(Menus[CurrentMenu].Items + entry - 1,
	    Menus[CurrentMenu].Items + entry,
	    sizeof(*Menus[0].Items) * (Menus[CurrentMenu].NumItems - entry));
    Menus[CurrentMenu].NumItems--;
}

void glutAttachMenu(int button)
{
    AttachedMenus[button] = CurrentMenu;
}

void glutDetachMenu(int button)
{
    AttachedMenus[button] = 0;
}

/* --------- Callbacks ------------ */
void glutDisplayFunc(void (*func)(void))
{
    DisplayFunc = func;
}

void glutOverlayDisplayFunc(void (*func)(void))
{
}

void glutReshapeFunc(void (*func)(int width, int height))
{
    ReshapeFunc = func;
}

void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y))
{
    KeyboardFunc = func;
}

void glutMouseFunc(void (*func)(int button, int state, int x, int y))
{
    MouseEnabled = 1;
    MouseFunc = func;
}

void glutMotionFunc(void (*func)(int x, int y))
{
    MouseEnabled = 1;
    MotionFunc = func;
}

void glutPassiveMotionFunc(void (*func)(int x, int y))
{
    MouseEnabled = 1;
    PassiveMotionFunc = func;
}

void glutVisibilityFunc(void (*func)(int state))
{
    VisibilityFunc = func;
}

void glutEntryFunc(void (*func)(int state))
{
}

void glutSpecialFunc(void (*func)(int key, int x, int y))
{
    SpecialFunc = func;
}

void glutSpaceballMotionFunc(void (*func)(int x, int y, int z))
{
}

void glutSpaceballRotateFunc(void (*func)(int x, int y, int z))
{
}

void glutButtonBoxFunc(void (*func)(int button, int state))
{
}

void glutDialsFunc(void (*func)(int dial, int value))
{
}

void glutTabletMotionFunc(void (*func)(int x, int y))
{
}

void glutTabletButtonFunc(void (*func)(int button, int state,
				       int x, int y))
{
}

void glutMenuStatusFunc(void (*func)(int status, int x, int y))
{
    MenuStatusFunc = func;
}

void glutMenuStateFunc(void (*func)(int status))
{
    MenuStateFunc = func;
}

void glutIdleFunc(void (*func)(void))
{
    IdleFunc = func;
}

void glutTimerFunc(unsigned int msecs,
		   void (*func)(int value), int value)
{
    struct GlutTimer *timer = malloc(sizeof *timer);
    timer->time = glutGet(GLUT_ELAPSED_TIME) + msecs;
    timer->func = func;
    timer->value = value;

    struct GlutTimer **head = &GlutTimers;
    while(*head && (*head)->time < timer->time)
	head = &(*head)->next;

    timer->next = *head;
    *head = timer;
}

/* --------- Color Map ------------*/
#define TOCMAP(x) (unsigned short)((x<0?0:x>1?1:x) * (GLfloat) (2<<16))
#define FROMCMAP(x) (GLfloat)x / (GLfloat)(2<<16)

void glutSetColor(int cell, GLfloat red, GLfloat green, GLfloat blue)
{
    if(cell >=0 && cell < 256) {

	ColorMap.red[cell] = TOCMAP(red);
	ColorMap.green[cell] = TOCMAP(green); 
	ColorMap.blue[cell] = TOCMAP(blue);

	ColorMap.start = cell;
	ColorMap.len = 1;

	if (ioctl(FrameBufferFD, FBIOPUTCMAP, (void *) &ColorMap) < 0)
	    fprintf(stderr, "ioctl(FBIOPUTCMAP) failed [%d]\n", cell);
    }
}

GLfloat glutGetColor(int cell, int component)
{
    if(!(DisplayMode & GLUT_INDEX))
	return -1.0;

    if(cell < 0 || cell > 256)
	return -1.0;

    switch(component) {
    case GLUT_RED:
	return FROMCMAP(ColorMap.red[cell]);
    case GLUT_GREEN:
	return FROMCMAP(ColorMap.green[cell]);
    case GLUT_BLUE:
	return FROMCMAP(ColorMap.blue[cell]);
    }
    return -1.0;
}

void glutCopyColormap(int win)
{
}

/* --------- State ------------*/
void glutWarpPointer(int x, int y) 
{
    if(x < 0)
	x = 0;
    if(x >= VarInfo.xres) 
	x = VarInfo.xres - 1;
    MouseX = x;

    if(y < 0)
	y = 0;
    if(y >= VarInfo.yres) 
	y = VarInfo.yres - 1;
    MouseY = y;

    EraseCursor();
    SwapCursor();
}

int glutGet(GLenum state)
{
    switch(state) {
    case GLUT_WINDOW_X:
	return 0;
    case GLUT_WINDOW_Y:
	return 0;
    case GLUT_INIT_WINDOW_WIDTH:
    case GLUT_WINDOW_WIDTH:
    case GLUT_SCREEN_WIDTH:
	return VarInfo.xres;
    case GLUT_INIT_WINDOW_HEIGHT:
    case GLUT_WINDOW_HEIGHT:
    case GLUT_SCREEN_HEIGHT:
	return VarInfo.yres;
    case GLUT_WINDOW_BUFFER_SIZE:
	return VarInfo.bits_per_pixel;
    case GLUT_WINDOW_STENCIL_SIZE:
	return StencilSize;
    case GLUT_WINDOW_DEPTH_SIZE:
	return DepthSize;
    case GLUT_WINDOW_RED_SIZE:
	return VarInfo.red.length;
    case GLUT_WINDOW_GREEN_SIZE:
	return VarInfo.green.length;
    case GLUT_WINDOW_BLUE_SIZE:
	return VarInfo.green.length;
    case GLUT_WINDOW_ALPHA_SIZE:
	return VarInfo.transp.length;
    case GLUT_WINDOW_ACCUM_RED_SIZE:
    case GLUT_WINDOW_ACCUM_GREEN_SIZE:
    case GLUT_WINDOW_ACCUM_BLUE_SIZE:
    case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
	return AccumSize;
    case GLUT_WINDOW_DOUBLEBUFFER:
	if(DisplayMode & GLUT_DOUBLE)
	    return 1;
	return 0;
    case GLUT_WINDOW_RGBA:
	if(DisplayMode & GLUT_INDEX)
	    return 0;
	return 1;
    case GLUT_WINDOW_PARENT:
	return 0;
    case GLUT_WINDOW_NUM_CHILDREN:
	return 0;
    case GLUT_WINDOW_COLORMAP_SIZE:
	if(DisplayMode & GLUT_INDEX)
	    return 256;
	return 0;
    case GLUT_WINDOW_NUM_SAMPLES:
	return 0;
    case GLUT_WINDOW_STEREO:
	return 0;
    case GLUT_WINDOW_CURSOR:
	return CurrentCursor;
    case GLUT_SCREEN_WIDTH_MM:
	return VarInfo.width;
    case GLUT_SCREEN_HEIGHT_MM:
	return VarInfo.height;
    case GLUT_MENU_NUM_ITEMS:
	if(CurrentMenu)
	    return Menus[CurrentMenu].NumItems;
	return 0;
    case GLUT_DISPLAY_MODE_POSSIBLE:
	if((DisplayMode & GLUT_MULTISAMPLE)
	|| (DisplayMode & GLUT_STEREO)
	|| (DisplayMode & GLUT_LUMINANCE)
	|| (DisplayMode & GLUT_ALPHA) && (DisplayMode & GLUT_INDEX))
	    return 0;
	return 1;
    case GLUT_INIT_DISPLAY_MODE:
	return DisplayMode;
    case GLUT_INIT_WINDOW_X:
    case GLUT_INIT_WINDOW_Y:
	return 0;
    case GLUT_ELAPSED_TIME:
	{
	    static struct timeval tv;
	    gettimeofday(&tv, 0);
	    return 1000 * (tv.tv_sec - StartTime.tv_sec)
		+ (tv.tv_usec - StartTime.tv_usec) / 1000;
	}
    }
}

int glutLayerGet(GLenum info)
{
    switch(info) {
    case GLUT_OVERLAY_POSSIBLE:
	return 0;
    case GLUT_LAYER_IN_USE:
	return GLUT_NORMAL;
    case GLUT_HAS_OVERLAY:
	return 0;
    case GLUT_TRANSPARENT_INDEX:
	return -1;
    case GLUT_NORMAL_DAMAGED:
	return Redisplay;
    case GLUT_OVERLAY_DAMAGED:
	return -1;
    }
    return -1;
}

int glutDeviceGet(GLenum info)
{
    switch(info) {
    case GLUT_HAS_KEYBOARD:
	return 1;
    case GLUT_HAS_MOUSE:
    case GLUT_NUM_MOUSE_BUTTONS:
	return NumMouseButtons;
    case GLUT_HAS_SPACEBALL:
    case GLUT_HAS_DIAL_AND_BUTTON_BOX:
    case GLUT_HAS_TABLET:
	return 0;
    case GLUT_NUM_SPACEBALL_BUTTONS:
    case GLUT_NUM_BUTTON_BOX_BUTTONS:
    case GLUT_NUM_DIALS:
    case GLUT_NUM_TABLET_BUTTONS:
	return 0;
    }
    return -1;
}

int glutGetModifiers(void){
    return KeyboardModifiers;
}

/* ------------- extensions ------------ */
int glutExtensionSupported(const char *extension)
{
    const char *exts = (const char *) glGetString(GL_EXTENSIONS);
    const char *start = exts;
    int len = strlen(extension);

    for(;;) {
	const char *p = strstr(exts, extension);
	if(!p)
	    break;
	if((p == start || p[-1] == ' ') && (p[len] == ' ' || p[len] == 0))
	    return 1;
	exts = p + len;
    }
    return 0;
}

void glutReportErrors(void)
{
  GLenum error;

  while ((error = glGetError()) != GL_NO_ERROR)
    fprintf(stderr, "GL error: %s", gluErrorString(error));
}

static struct {
   const char *name;
   const GLUTproc address;
} glut_functions[] = {
   { "glutInit", (const GLUTproc) glutInit },
   { "glutInitDisplayMode", (const GLUTproc) glutInitDisplayMode },
   { "glutInitWindowPosition", (const GLUTproc) glutInitWindowPosition },
   { "glutInitWindowSize", (const GLUTproc) glutInitWindowSize },
   { "glutMainLoop", (const GLUTproc) glutMainLoop },
   { "glutCreateWindow", (const GLUTproc) glutCreateWindow },
   { "glutCreateSubWindow", (const GLUTproc) glutCreateSubWindow },
   { "glutDestroyWindow", (const GLUTproc) glutDestroyWindow },
   { "glutPostRedisplay", (const GLUTproc) glutPostRedisplay },
   { "glutSwapBuffers", (const GLUTproc) glutSwapBuffers },
   { "glutGetWindow", (const GLUTproc) glutGetWindow },
   { "glutSetWindow", (const GLUTproc) glutSetWindow },
   { "glutSetWindowTitle", (const GLUTproc) glutSetWindowTitle },
   { "glutSetIconTitle", (const GLUTproc) glutSetIconTitle },
   { "glutPositionWindow", (const GLUTproc) glutPositionWindow },
   { "glutReshapeWindow", (const GLUTproc) glutReshapeWindow },
   { "glutPopWindow", (const GLUTproc) glutPopWindow },
   { "glutPushWindow", (const GLUTproc) glutPushWindow },
   { "glutIconifyWindow", (const GLUTproc) glutIconifyWindow },
   { "glutShowWindow", (const GLUTproc) glutShowWindow },
   { "glutHideWindow", (const GLUTproc) glutHideWindow },
   { "glutFullScreen", (const GLUTproc) glutFullScreen },
   { "glutSetCursor", (const GLUTproc) glutSetCursor },
   { "glutWarpPointer", (const GLUTproc) glutWarpPointer },
   { "glutEstablishOverlay", (const GLUTproc) glutEstablishOverlay },
   { "glutRemoveOverlay", (const GLUTproc) glutRemoveOverlay },
   { "glutUseLayer", (const GLUTproc) glutUseLayer },
   { "glutPostOverlayRedisplay", (const GLUTproc) glutPostOverlayRedisplay },
   { "glutShowOverlay", (const GLUTproc) glutShowOverlay },
   { "glutHideOverlay", (const GLUTproc) glutHideOverlay },
   { "glutCreateMenu", (const GLUTproc) glutCreateMenu },
   { "glutDestroyMenu", (const GLUTproc) glutDestroyMenu },
   { "glutGetMenu", (const GLUTproc) glutGetMenu },
   { "glutSetMenu", (const GLUTproc) glutSetMenu },
   { "glutAddMenuEntry", (const GLUTproc) glutAddMenuEntry },
   { "glutAddSubMenu", (const GLUTproc) glutAddSubMenu },
   { "glutChangeToMenuEntry", (const GLUTproc) glutChangeToMenuEntry },
   { "glutChangeToSubMenu", (const GLUTproc) glutChangeToSubMenu },
   { "glutRemoveMenuItem", (const GLUTproc) glutRemoveMenuItem },
   { "glutAttachMenu", (const GLUTproc) glutAttachMenu },
   { "glutDetachMenu", (const GLUTproc) glutDetachMenu },
   { "glutDisplayFunc", (const GLUTproc) glutDisplayFunc },
   { "glutReshapeFunc", (const GLUTproc) glutReshapeFunc },
   { "glutKeyboardFunc", (const GLUTproc) glutKeyboardFunc },
   { "glutMouseFunc", (const GLUTproc) glutMouseFunc },
   { "glutMotionFunc", (const GLUTproc) glutMotionFunc },
   { "glutPassiveMotionFunc", (const GLUTproc) glutPassiveMotionFunc },
   { "glutEntryFunc", (const GLUTproc) glutEntryFunc },
   { "glutVisibilityFunc", (const GLUTproc) glutVisibilityFunc },
   { "glutIdleFunc", (const GLUTproc) glutIdleFunc },
   { "glutTimerFunc", (const GLUTproc) glutTimerFunc },
   { "glutMenuStateFunc", (const GLUTproc) glutMenuStateFunc },
   { "glutSpecialFunc", (const GLUTproc) glutSpecialFunc },
   { "glutSpaceballRotateFunc", (const GLUTproc) glutSpaceballRotateFunc },
   { "glutButtonBoxFunc", (const GLUTproc) glutButtonBoxFunc },
   { "glutDialsFunc", (const GLUTproc) glutDialsFunc },
   { "glutTabletMotionFunc", (const GLUTproc) glutTabletMotionFunc },
   { "glutTabletButtonFunc", (const GLUTproc) glutTabletButtonFunc },
   { "glutMenuStatusFunc", (const GLUTproc) glutMenuStatusFunc },
   { "glutOverlayDisplayFunc", (const GLUTproc) glutOverlayDisplayFunc },
   { "glutSetColor", (const GLUTproc) glutSetColor },
   { "glutGetColor", (const GLUTproc) glutGetColor },
   { "glutCopyColormap", (const GLUTproc) glutCopyColormap },
   { "glutGet", (const GLUTproc) glutGet },
   { "glutDeviceGet", (const GLUTproc) glutDeviceGet },
   { "glutExtensionSupported", (const GLUTproc) glutExtensionSupported },
   { "glutGetModifiers", (const GLUTproc) glutGetModifiers },
   { "glutLayerGet", (const GLUTproc) glutLayerGet },
   { "glutGetProcAddress", (const GLUTproc) glutGetProcAddress },
   { "glutBitmapCharacter", (const GLUTproc) glutBitmapCharacter },
   { "glutBitmapWidth", (const GLUTproc) glutBitmapWidth },
   { "glutStrokeCharacter", (const GLUTproc) glutStrokeCharacter },
   { "glutStrokeWidth", (const GLUTproc) glutStrokeWidth },
   { "glutBitmapLength", (const GLUTproc) glutBitmapLength },
   { "glutStrokeLength", (const GLUTproc) glutStrokeLength },
   { "glutWireSphere", (const GLUTproc) glutWireSphere },
   { "glutSolidSphere", (const GLUTproc) glutSolidSphere },
   { "glutWireCone", (const GLUTproc) glutWireCone },
   { "glutSolidCone", (const GLUTproc) glutSolidCone },
   { "glutWireCube", (const GLUTproc) glutWireCube },
   { "glutSolidCube", (const GLUTproc) glutSolidCube },
   { "glutWireTorus", (const GLUTproc) glutWireTorus },
   { "glutSolidTorus", (const GLUTproc) glutSolidTorus },
   { "glutWireDodecahedron", (const GLUTproc) glutWireDodecahedron },
   { "glutSolidDodecahedron", (const GLUTproc) glutSolidDodecahedron },
   { "glutWireTeapot", (const GLUTproc) glutWireTeapot },
   { "glutSolidTeapot", (const GLUTproc) glutSolidTeapot },
   { "glutWireOctahedron", (const GLUTproc) glutWireOctahedron },
   { "glutSolidOctahedron", (const GLUTproc) glutSolidOctahedron },
   { "glutWireTetrahedron", (const GLUTproc) glutWireTetrahedron },
   { "glutSolidTetrahedron", (const GLUTproc) glutSolidTetrahedron },
   { "glutWireIcosahedron", (const GLUTproc) glutWireIcosahedron },
   { "glutSolidIcosahedron", (const GLUTproc) glutSolidIcosahedron },
   { "glutReportErrors", (const GLUTproc) glutReportErrors },
   { NULL, NULL }
};   

GLUTproc glutGetProcAddress(const char *procName)
{
   /* Try GLUT functions first */
   int i;
   for (i = 0; glut_functions[i].name; i++) {
      if (strcmp(glut_functions[i].name, procName) == 0)
         return glut_functions[i].address;
   }

   /* Try core GL functions */
  return (GLUTproc) glFBDevGetProcAddress(procName);
}
