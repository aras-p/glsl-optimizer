/*
 * Copyright © 2007-2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Jesse Barnes <jesse.barnes@intel.com>
 *
 */

/** @file omlsync.c
 * The program is simple:  it paints a window alternating colors (red &
 * white) either as fast as possible or synchronized to vblank events
 *
 * If run normally, the program should display a window that exhibits
 * significant tearing between red and white colors (e.g. you might get
 * a "waterfall" effect of red and white horizontal bars).
 *
 * If run with the '-s b' option, the program should synchronize the
 * window color changes with the vertical blank period, resulting in a
 * window that looks orangish with a high frequency flicker (which may
 * be invisible).  If the window is moved to another screen, this
 * property should be preserved.  If the window spans two screens, it
 * shouldn't tear on whichever screen most of the window is on; the
 * portion on the other screen may show some tearing (like the
 * waterfall effect above).
 *
 * Other options include '-w <width>' and '-h <height>' to set the
 * window size.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

Bool (*glXGetSyncValuesOML)(Display *dpy, GLXDrawable drawable,
			    int64_t *ust, int64_t *msc, int64_t *sbc);
Bool (*glXGetMscRateOML)(Display *dpy, GLXDrawable drawable, int32_t *numerator,
			 int32_t *denominator);
int64_t (*glXSwapBuffersMscOML)(Display *dpy, GLXDrawable drawable,
				int64_t target_msc, int64_t divisor,
				int64_t remainder);
Bool (*glXWaitForMscOML)(Display *dpy, GLXDrawable drawable, int64_t target_msc,
			 int64_t divisor, int64_t remainder, int64_t *ust,
			 int64_t *msc, int64_t *sbc);
Bool (*glXWaitForSbcOML)(Display *dpy, GLXDrawable drawable, int64_t target_sbc,
			 int64_t *ust, int64_t *msc, int64_t *sbc);
int (*glXSwapInterval)(int interval);

static int GLXExtensionSupported(Display *dpy, const char *extension)
{
	const char *extensionsString, *pos;

	extensionsString = glXQueryExtensionsString(dpy, DefaultScreen(dpy));

	pos = strstr(extensionsString, extension);

	if (pos != NULL && (pos == extensionsString || pos[-1] == ' ') &&
	    (pos[strlen(extension)] == ' ' || pos[strlen(extension)] == '\0'))
		return 1;

	return 0;
}

extern char *optarg;
extern int optind, opterr, optopt;
static char optstr[] = "w:h:vd:r:n:i:";

static void usage(char *name)
{
	printf("usage: %s [-w <width>] [-h <height>] ...\n", name);
	printf("\t-d<divisor> - divisor for OML swap\n");
	printf("\t-r<remainder> - remainder for OML swap\n");
	printf("\t-n<interval> - wait interval for OML WaitMSC\n");
	printf("\t-i<swap interval> - swap at most once every n frames\n");
	printf("\t-v: verbose (print count)\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	Display *disp;
	XVisualInfo *pvi;
	XSetWindowAttributes swa;
	Window winGL;
	GLXContext context;
	int dummy;
	Atom wmDelete;
	int64_t ust, msc, sbc;
	int width = 500, height = 500, verbose = 0, divisor = 0, remainder = 0,
		wait_interval = 0, swap_interval = 1;
	int c, i = 1;
	int ret;
	int db_attribs[] = { GLX_RGBA,
                     GLX_RED_SIZE, 1,
                     GLX_GREEN_SIZE, 1,
                     GLX_BLUE_SIZE, 1,
                     GLX_DOUBLEBUFFER,
                     GLX_DEPTH_SIZE, 1,
                     None };
	XSizeHints sizehints;

	opterr = 0;
	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
		case 'w':
			width = atoi(optarg);
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'd':
			divisor = atoi(optarg);
			break;
		case 'r':
			remainder = atoi(optarg);
			break;
		case 'n':
			wait_interval = atoi(optarg);
			break;
		case 'i':
			swap_interval = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	disp = XOpenDisplay(NULL);
	if (!disp) {
		fprintf(stderr, "failed to open display\n");
		return -1;
	}

	if (!glXQueryExtension(disp, &dummy, &dummy)) {
		fprintf(stderr, "glXQueryExtension failed\n");
		return -1;
	}

	if (!GLXExtensionSupported(disp, "GLX_OML_sync_control")) {
		fprintf(stderr, "GLX_OML_sync_control not supported\n");
		return -1;
	}

	if (!GLXExtensionSupported(disp, "GLX_MESA_swap_control")) {
		fprintf(stderr, "GLX_MESA_swap_control not supported\n");
		return -1;
	}

	pvi = glXChooseVisual(disp, DefaultScreen(disp), db_attribs);

	if (!pvi) {
		fprintf(stderr, "failed to choose visual, exiting\n");
		return -1;
	}

	pvi->screen = DefaultScreen(disp);

	swa.colormap = XCreateColormap(disp, RootWindow(disp, pvi->screen),
				       pvi->visual, AllocNone);
	swa.border_pixel = 0;
	swa.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
		StructureNotifyMask;
	winGL = XCreateWindow(disp, RootWindow(disp, pvi->screen),
			      0, 0,
			      width, height,
			      0, pvi->depth, InputOutput, pvi->visual,
			      CWBorderPixel | CWColormap | CWEventMask, &swa);
	if (!winGL) {
		fprintf(stderr, "window creation failed\n");
		return -1;
	}
        wmDelete = XInternAtom(disp, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(disp, winGL, &wmDelete, 1);

	sizehints.x = 0;
	sizehints.y = 0;
	sizehints.width  = width;
	sizehints.height = height;
	sizehints.flags = USSize | USPosition;

	XSetNormalHints(disp, winGL, &sizehints);
	XSetStandardProperties(disp, winGL, "glsync test", "glsync text",
			       None, NULL, 0, &sizehints);

	context = glXCreateContext(disp, pvi, NULL, GL_TRUE);
	if (!context) {
		fprintf(stderr, "failed to create glx context\n");
		return -1;
	}

	XMapWindow(disp, winGL);
	ret = glXMakeCurrent(disp, winGL, context);
	if (!ret) {
		fprintf(stderr, "failed to make context current: %d\n", ret);
	}

	glXGetSyncValuesOML = (void *)glXGetProcAddress((unsigned char *)"glXGetSyncValuesOML");
	glXGetMscRateOML = (void *)glXGetProcAddress((unsigned char *)"glXGetMscRateOML");
	glXSwapBuffersMscOML = (void *)glXGetProcAddress((unsigned char *)"glXSwapBuffersMscOML");
	glXWaitForMscOML = (void *)glXGetProcAddress((unsigned char *)"glXWaitForMscOML");
	glXWaitForSbcOML = (void *)glXGetProcAddress((unsigned char *)"glXWaitForSbcOML");
	glXSwapInterval = (void *)glXGetProcAddress((unsigned char *)"glXSwapIntervalMESA");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glXSwapInterval(swap_interval);
	fprintf(stderr, "set swap interval to %d\n", swap_interval);

	glXGetSyncValuesOML(disp, winGL, &ust, &msc, &sbc);
	while (i++) {
		/* Alternate colors to make tearing obvious */
		if (i & 1) {
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glColor3f(1.0f, 1.0f, 1.0f);
		} else {
			glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
			glColor3f(1.0f, 0.0f, 0.0f);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glRectf(0, 0, width, height);

		if (!wait_interval)
			glXSwapBuffersMscOML(disp, winGL, 0, divisor,
					     remainder);
		else {
			glXWaitForMscOML(disp, winGL, msc + wait_interval,
					 divisor, remainder, &ust, &msc, &sbc);
			glXSwapBuffersMscOML(disp, winGL, 0, 0, 0);
		}
	}

	XDestroyWindow(disp, winGL);
	glXDestroyContext(disp, context);
	XCloseDisplay(disp);

	return 0;
}
