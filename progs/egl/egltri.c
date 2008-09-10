/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  Jakob Bornecrantz   All Rights Reserved.
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
 * This program is based on eglgears and xegl_tri.
 * Remixed by Jakob Bornecrantz
 *
 * No command line options.
 * Program runs for 5 seconds then exits, outputing framerate to console
 */

#define EGL_EGLEXT_PROTOTYPES

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define MAX_CONFIGS 10
#define MAX_MODES 100


/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static double
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}


static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;

static void draw()
{
	static const GLfloat verts[3][2] = {
		{ -1, -1 },
		{  1, -1 },
		{  0,  1 }
	};
	static const GLfloat colors[3][3] = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
	};

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glRotatef(view_rotx, 1, 0, 0);
	glRotatef(view_roty, 0, 1, 0);
	glRotatef(view_rotz, 0, 0, 1);

	{
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glColorPointer(3, GL_FLOAT, 0, colors);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	glPopMatrix();
}

static void init()
{
	glClearColor(0.4, 0.4, 0.4, 0.0);
}

/* new window size or exposure */
static void reshape(int width, int height)
{
	GLfloat ar = (GLfloat) width / (GLfloat) height;

	glViewport(0, 0, (GLint) width, (GLint) height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-ar, ar, -1, 1, 5.0, 60.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -10.0);
}

static void run(EGLDisplay dpy, EGLSurface surf, int ttr)
{
	double st = current_time();
	double ct = st;
	int frames = 0;

	while (ct - st < ttr)
	{
		double tt = current_time();
		double dt = tt - ct;
		ct = tt;

		draw();

		eglSwapBuffers(dpy, surf);

		frames++;
	}

	GLfloat seconds = ct - st;
	GLfloat fps = frames / seconds;
	printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
}

int main(int argc, char *argv[])
{
	int maj, min;
	EGLContext ctx;
	EGLSurface screen_surf;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs, i;
	EGLBoolean b;
	EGLDisplay d;
	EGLint screenAttribs[10];
	EGLModeMESA mode[MAX_MODES];
	EGLScreenMESA screen;
	EGLint count, chosenMode = 0;
	GLboolean printInfo = GL_FALSE;
	EGLint width = 0, height = 0;

	/* parse cmd line args */
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-info") == 0)
		{
			printInfo = GL_TRUE;
		}
		else
			printf("Warning: unknown parameter: %s\n", argv[i]);
	}

	/* DBR : Create EGL context/surface etc */
	d = eglGetDisplay((EGLNativeDisplayType)"!EGL_i915");
	assert(d);

	if (!eglInitialize(d, &maj, &min)) {
		printf("egltri: eglInitialize failed\n");
		exit(1);
	}

	printf("egltri: EGL version = %d.%d\n", maj, min);
	printf("egltri: EGL_VENDOR = %s\n", eglQueryString(d, EGL_VENDOR));

	/* XXX use ChooseConfig */
	eglGetConfigs(d, configs, MAX_CONFIGS, &numConfigs);
	eglGetScreensMESA(d, &screen, 1, &count);

	if (!eglGetModesMESA(d, screen, mode, MAX_MODES, &count) || count == 0) {
		printf("egltri: eglGetModesMESA failed!\n");
		return 0;
	}

	/* Print list of modes, and find the one to use */
	printf("egltri: Found %d modes:\n", count);
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
	printf("egltri: Using screen mode/size %d: %d x %d\n", chosenMode, width, height);

	ctx = eglCreateContext(d, configs[0], EGL_NO_CONTEXT, NULL);
	if (ctx == EGL_NO_CONTEXT) {
		printf("egltri: failed to create context\n");
		return 0;
	}

	/* build up screenAttribs array */
	i = 0;
	screenAttribs[i++] = EGL_WIDTH;
	screenAttribs[i++] = width;
	screenAttribs[i++] = EGL_HEIGHT;
	screenAttribs[i++] = height;
	screenAttribs[i++] = EGL_NONE;

	screen_surf = eglCreateScreenSurfaceMESA(d, configs[0], screenAttribs);
	if (screen_surf == EGL_NO_SURFACE) {
		printf("egltri: failed to create screen surface\n");
		return 0;
	}

	b = eglShowScreenSurfaceMESA(d, screen, screen_surf, mode[chosenMode]);
	if (!b) {
		printf("egltri: show surface failed\n");
		return 0;
	}

	b = eglMakeCurrent(d, screen_surf, screen_surf, ctx);
	if (!b) {
		printf("egltri: make current failed\n");
		return 0;
	}

	if (printInfo)
	{
		printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
	}

	init();
	reshape(width, height);

	glDrawBuffer( GL_BACK );

	run(d, screen_surf, 5.0);

	eglDestroySurface(d, screen_surf);
	eglDestroyContext(d, ctx);
	eglTerminate(d);

	return 0;
}
