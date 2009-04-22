
/* This is a good test for glXSwapBuffers on non-current windows,
 * and the glXCopyContext function.  Fixed several Mesa/DRI bugs with
 * this program on 15 June 2002.
 *
 * Joe's comments follow:
 *
 * I have tried some different approaches for being able to
 * draw to multiple windows using one context, or a copied
 * context. Mesa/indirect rendering works to use one context
 * for multiple windows, but crashes with glXCopyContext.
 * DRI is badly broken, at least for ATI.
 *
 * I also noticed that glXMakeCurrent allows a window and context
 * from different visuals to be attached (haven't tested recently).
 *
 * Joe Krahn  <jkrahn@nc.rr.com>
 */

#include <GL/glx.h>
#include <GL/gl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159
#endif

#define DEGTOR (M_PI/180.0)

static int AttributeList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };

int main(int argc, char **argv)
{
    Window win1, win2;
    XVisualInfo *vi;
    XSetWindowAttributes swa;
    Display *dpy;
    GLXContext ctx1, ctx2;
    float angle;
    int test;

    if (argc < 2) {
       fprintf(stderr, "This program tests GLX context switching.\n");
       fprintf(stderr, "Usage: jkrahntest <n>\n");
       fprintf(stderr, "Where n is:\n");
       fprintf(stderr, "\t1) Use two contexts and swap only when the context is current (typical case).\n");
       fprintf(stderr, "\t2) Use two contexts and swap at the same time.\n");
       fprintf(stderr, "\t\t Used to crash Mesa & nVidia, and DRI artifacts. Seems OK now.\n");
       fprintf(stderr, "\t3) Use one context, but only swap when a context is current.\n");
       fprintf(stderr, "\t\t Serious artifacts for DRI at least with ATI.\n");
       fprintf(stderr, "\t4) Use one context, swap both windows at the same time, so the left\n");
       fprintf(stderr, "\t\t window has no context at swap time. Severe artifacts for DRI.\n");
       fprintf(stderr, "\t5) Use two contexts, copying one to the other when switching windows.\n");
       fprintf(stderr, "\t\t DRI gives an error, indirect rendering crashes server.\n");

	exit(1);
    }
    test = atoi(argv[1]);

    /* get a connection */
    dpy = XOpenDisplay(NULL);

    /* Get an appropriate visual */
    vi = glXChooseVisual(dpy, DefaultScreen(dpy), AttributeList);
    if (vi == 0) {
	fprintf(stderr, "No matching visuals found.\n");
	exit(-1);
    }

    /* Create two GLX contexts, with list sharing */
    ctx1 = glXCreateContext(dpy, vi, 0, True);
    ctx2 = glXCreateContext(dpy, vi, ctx1, True);

    /* create a colormap */
    swa.colormap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
       vi->visual, AllocNone);
    swa.border_pixel = 0;

    /* Create two windows */
    win1 = XCreateWindow(dpy, RootWindow(dpy, vi->screen),
       10, 10, 200, 200,
       0, vi->depth, InputOutput, vi->visual,
       CWBorderPixel | CWColormap, &swa);
    XStoreName(dpy, win1, "Test [L]");
    XMapWindow(dpy, win1);
    XMoveWindow(dpy, win1, 10, 10); /* Initial requested x,y may not be honored */
   {
      XSizeHints sizehints;
      static const char *name = "window";
      sizehints.x = 10;
      sizehints.y = 10;
      sizehints.width  = 200;
      sizehints.height = 200;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win1, &sizehints);
      XSetStandardProperties(dpy, win1, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }


    win2 = XCreateWindow(dpy, RootWindow(dpy, vi->screen),
       250, 10, 200, 200,
       0, vi->depth, InputOutput, vi->visual,
       CWBorderPixel | CWColormap, &swa);
    XStoreName(dpy, win1, "Test [R]");
    XMapWindow(dpy, win2);
    XMoveWindow(dpy, win2, 260, 10);
   {
      XSizeHints sizehints;
      static const char *name = "window";
      sizehints.x = 10;
      sizehints.y = 10;
      sizehints.width  = 200;
      sizehints.height = 200;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win2, &sizehints);
      XSetStandardProperties(dpy, win2, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }


    /* Now draw some spinning things */
    for (angle = 0; angle < 360*4; angle += 10.0) {
	/* Connect the context to window 1 */
	glXMakeCurrent(dpy, win1, ctx1);

	/* Clear and draw in window 1 */
	glDrawBuffer(GL_BACK);
	glClearColor(1, 1, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1, 0, 0);
	glBegin(GL_TRIANGLES);
	glVertex2f(0, 0);
	glVertex2f(cos(angle * DEGTOR), sin(angle * DEGTOR));
	glVertex2f(cos((angle + 20.0) * DEGTOR),
	   sin((angle + 20.0) * DEGTOR));
	glEnd();
        glFlush();

	if (test == 1 || test == 3 || test == 5)
	    glXSwapBuffers(dpy, win1);

	if (test == 5)
	    glXCopyContext(dpy, ctx1, ctx2, GL_ALL_ATTRIB_BITS);
	/* Connect the context to window 2 */
	if (test == 3 || test == 4) {
	    glXMakeCurrent(dpy, win2, ctx1);
	} else {
	    glXMakeCurrent(dpy, win2, ctx2);
	}

	/* Clear and draw in window 2 */
	glDrawBuffer(GL_BACK);
	glClearColor(0, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1, 1, 0);
	glBegin(GL_TRIANGLES);
	glVertex2f(0, 0);
	glVertex2f(cos(angle * DEGTOR), sin(angle * DEGTOR));
	glVertex2f(cos((angle + 20.0) * DEGTOR),
	   sin((angle + 20.0) * DEGTOR));
	glEnd();
        glFlush();

	/* Swap buffers */
	if (test == 2 || test == 4)
	    glXSwapBuffers(dpy, win1);
	glXSwapBuffers(dpy, win2);

	/* wait a while */
	glXWaitX();
        usleep(20000);
    }

    return 0;
}
