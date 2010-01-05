/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/*
 * Stolen from eglgears
 *
 * Creates a surface and show that on the first screen
 */

#define EGL_EGLEXT_PROTOTYPES

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define MAX_CONFIGS 10
#define MAX_MODES 100

int
main(int argc, char *argv[])
{
	int maj, min;
	EGLSurface screen_surf;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs, i;
	EGLBoolean b;
	EGLDisplay d;
	EGLint screenAttribs[10];
	EGLModeMESA mode[MAX_MODES];
	EGLScreenMESA screen;
	EGLint count;
	EGLint chosenMode = 0;
	EGLint width = 0, height = 0;

	d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(d);

	if (!eglInitialize(d, &maj, &min)) {
		printf("eglscreen: eglInitialize failed\n");
		exit(1);
	}

	printf("eglscreen: EGL version = %d.%d\n", maj, min);
	printf("eglscreen: EGL_VENDOR = %s\n", eglQueryString(d, EGL_VENDOR));

	/* XXX use ChooseConfig */
	eglGetConfigs(d, configs, MAX_CONFIGS, &numConfigs);
	eglGetScreensMESA(d, &screen, 1, &count);

	if (!eglGetModesMESA(d, screen, mode, MAX_MODES, &count) || count == 0) {
		printf("eglscreen: eglGetModesMESA failed!\n");
		return 0;
	}

	/* Print list of modes, and find the one to use */
	printf("eglscreen: Found %d modes:\n", count);
	for (i = 0; i < count; i++) {
		EGLint w, h;
		eglGetModeAttribMESA(d, mode[i], EGL_WIDTH, &w);
		eglGetModeAttribMESA(d, mode[i], EGL_HEIGHT, &h);
		printf("%3d: %d x %d\n", i, w, h);
		if (w > width && h > height) {
			width = w;
			height = h;
			chosenMode = i;
		}
	}
	printf("eglscreen: Using screen mode/size %d: %d x %d\n", chosenMode, width, height);

	/* build up screenAttribs array */
	i = 0;
	screenAttribs[i++] = EGL_WIDTH;
	screenAttribs[i++] = width;
	screenAttribs[i++] = EGL_HEIGHT;
	screenAttribs[i++] = height;
	screenAttribs[i++] = EGL_NONE;

	screen_surf = eglCreateScreenSurfaceMESA(d, configs[0], screenAttribs);
	if (screen_surf == EGL_NO_SURFACE) {
		printf("eglscreen: Failed to create screen surface\n");
		return 0;
	}

	b = eglShowScreenSurfaceMESA(d, screen, screen_surf, mode[chosenMode]);
	if (!b) {
		printf("eglscreen: Show surface failed\n");
		return 0;
	}

	usleep(5000000);

	eglDestroySurface(d, screen_surf);
	eglTerminate(d);

	return 0;
}
