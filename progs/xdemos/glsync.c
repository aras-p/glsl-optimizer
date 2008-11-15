/*
 * Copyright © 2007 Intel Corporation
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

/** @file glsync.c
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
 * Other options include '-w <width>' and '-h <height' to set the
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

void (*video_sync_get)();
void (*video_sync)();

static int GLXExtensionSupported(Display *dpy, const char *extension)
{
	const char *extensionsString, *client_extensions, *pos;

	extensionsString = glXQueryExtensionsString(dpy, DefaultScreen(dpy));
	client_extensions = glXGetClientString(dpy, GLX_EXTENSIONS);

	pos = strstr(extensionsString, extension);

	if (pos != NULL && (pos == extensionsString || pos[-1] == ' ') &&
	    (pos[strlen(extension)] == ' ' || pos[strlen(extension)] == '\0'))
		return 1;

	pos = strstr(client_extensions, extension);

	if (pos != NULL && (pos == extensionsString || pos[-1] == ' ') &&
	    (pos[strlen(extension)] == ' ' || pos[strlen(extension)] == '\0'))
		return 1;

	return 0;
}

extern char *optarg;
extern int optind, opterr, optopt;
static char optstr[] = "w:h:s:v";

enum sync_type {
	none = 0,
	sgi_video_sync,
	buffer_swap,
};

static void usage(char *name)
{
	printf("usage: %s [-w <width>] [-h <height>] [-s<sync method>] "
	       "[-vc]\n", name);
	printf("\t-s<sync method>:\n");
	printf("\t\tn: none\n");
	printf("\t\ts: SGI video sync extension\n");
	printf("\t\tb: buffer swap\n");
	printf("\t-v: verbose (print count)\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	Display *disp;
	XVisualInfo *pvi;
	XSetWindowAttributes swa;
	int attrib[14];
	GLint last_val = -1, count = 0;
	Window winGL;
	int dummy;
	Atom wmDelete;
	enum sync_type waitforsync = none;
	int width = 500, height = 500, verbose = 0,
		countonly = 0;
	int c, i = 1;

	opterr = 0;
	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
		case 'w':
			width = atoi(optarg);
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 's':
			switch (optarg[0]) {
			case 'n':
				waitforsync = none;
				break;
			case 's':
				waitforsync = sgi_video_sync;
				break;
			case 'b':
				waitforsync = buffer_swap;
				break;
			default:
				usage(argv[0]);
				break;
			}
			break;
		case 'v':
			verbose = 1;
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

	if (!GLXExtensionSupported(disp, "GLX_SGI_video_sync")) {
		fprintf(stderr, "GLX_SGI_video_sync not supported, exiting\n");
		return -1;
	}

	attrib[0] = GLX_RGBA;
	attrib[1] = 1;
	attrib[2] = GLX_RED_SIZE;
	attrib[3] = 1;
	attrib[4] = GLX_GREEN_SIZE;
	attrib[5] = 1;
	attrib[6] = GLX_BLUE_SIZE;
	attrib[7] = 1;
	if (waitforsync != buffer_swap)
		attrib[8] = None;
	else {
		attrib[8] = GLX_DOUBLEBUFFER;
		attrib[9] = 1;
		attrib[10] = None;
	}

	GLXContext context;
	pvi = glXChooseVisual(disp, DefaultScreen(disp), attrib);
	if (!pvi) {
		fprintf(stderr, "failed to choose visual, exiting\n");
		return -1;
	}

	context = glXCreateContext(disp, pvi, None, GL_TRUE);
	if (!context) {
		fprintf(stderr, "failed to create glx context\n");
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

	XSetStandardProperties(disp, winGL, "glsync test", "glsync text",
			       None, NULL, 0, NULL);

	XMapRaised(disp, winGL);

	glXMakeCurrent(disp, winGL, context);

	video_sync_get = glXGetProcAddress((unsigned char *)"glXGetVideoSyncSGI");
	video_sync = glXGetProcAddress((unsigned char *)"glXWaitVideoSyncSGI");

	if (!video_sync_get || !video_sync) {
		fprintf(stderr, "failed to get sync functions\n");
		return -1;
	}

	video_sync_get(&count);
	count++;
	while (i++) {
		/* Wait for vsync */
		if (waitforsync == sgi_video_sync) {
			if (verbose)
				fprintf(stderr, "waiting on count %d\n", count);
			video_sync(2, (count + 1) % 2, &count);
			if (count < last_val)
				fprintf(stderr, "error:  vblank count went backwards: %d -> %d\n", last_val, count);
			if (count == last_val)
				fprintf(stderr, "error:  count didn't change: %d\n", count);
			last_val = count;
		} else if (waitforsync == buffer_swap) {
			glXSwapBuffers(disp, winGL);
		}

		if (countonly) {
			video_sync(2, 1, &count);
			fprintf(stderr, "current count: %d\n", count);
			sleep(1);
			continue;
		}

		/* Alternate colors to make tearing obvious */
		if (i & 1)
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		else
			glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glFlush();
	}

	XDestroyWindow(disp, winGL);
	glXDestroyContext(disp, context);
	XCloseDisplay(disp);

	return 0;
}
