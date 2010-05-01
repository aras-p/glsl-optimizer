/*
 * Copyright © 2009 Intel Corporation
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

/** @file msctest.c
 * Simple test for MSC functionality.
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

void (*get_sync_values)(Display *dpy, Window winGL, int64_t *ust, int64_t *msc, int64_t *sbc);
void (*wait_sync)(Display *dpy, Window winGL, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc);

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
static char optstr[] = "v";

static void usage(char *name)
{
	printf("usage: %s\n", name);
	exit(-1);
}

int main(int argc, char *argv[])
{
	Display *disp;
	XVisualInfo *pvi;
	XSetWindowAttributes swa;
	int attrib[14];
	Window winGL;
	GLXContext context;
	int dummy;
	Atom wmDelete;
	int verbose = 0, width = 200, height = 200;
	int c, i = 1;
	int64_t ust, msc, sbc;

	opterr = 0;
	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
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

	if (!GLXExtensionSupported(disp, "GLX_OML_sync_control")) {
		fprintf(stderr, "GLX_OML_sync_control not supported, exiting\n");
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
	attrib[8] = GLX_DOUBLEBUFFER;
	attrib[9] = 1;
	attrib[10] = None;

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

	XSetStandardProperties(disp, winGL, "msc test", "msc text",
			       None, NULL, 0, NULL);

	XMapRaised(disp, winGL);

	glXMakeCurrent(disp, winGL, context);

	get_sync_values = (void *)glXGetProcAddress((unsigned char *)"glXGetSyncValuesOML");
	wait_sync = (void *)glXGetProcAddress((unsigned char *)"glXWaitForMscOML");

	if (!get_sync_values || !wait_sync) {
		fprintf(stderr, "failed to get sync values function\n");
		return -1;
	}

	while (i++) {
		get_sync_values(disp, winGL, &ust, &msc, &sbc);
		fprintf(stderr, "ust: %llu, msc: %llu, sbc: %llu\n", ust, msc,
			sbc);

		/* Alternate colors to make tearing obvious */
		if (i & 1)
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		else
			glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glXSwapBuffers(disp, winGL);
		wait_sync(disp, winGL, 0, 60, 0, &ust, &msc, &sbc);
		fprintf(stderr,
			"wait returned ust: %llu, msc: %llu, sbc: %llu\n",
			ust, msc, sbc);
		sleep(1);
	}

	XDestroyWindow(disp, winGL);
	glXDestroyContext(disp, context);
	XCloseDisplay(disp);

	return 0;
}
