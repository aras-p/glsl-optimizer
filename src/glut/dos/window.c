/*
 * DOS/DJGPP Mesa Utility Toolkit
 * Version:  1.0
 *
 * Copyright (C) 2005  Daniel Borca   All Rights Reserved.
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
 * DANIEL BORCA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdio.h>

#include "internal.h"


static GLuint swaptime, swapcount;

static DMesaVisual visual = NULL;

GLUTwindow *_glut_current, *_glut_windows[MAX_WINDOWS];


static void
clean (void)
{
   int i;

   for (i=1; i<=MAX_WINDOWS; i++) {
      glutDestroyWindow(i);
   }
   if (visual) DMesaDestroyVisual(visual);

   pc_close_stdout();
   pc_close_stderr();
}


static GLUTwindow *
_glut_window (int win)
{
   if (win > 0 && --win < MAX_WINDOWS) {
      return _glut_windows[win];
   }
   return NULL;
}


int APIENTRY
glutCreateWindow (const char *title)
{
   int i;
   int m8width = (_glut_default.width + 7) & ~7;

   if (!(_glut_default.mode & GLUT_DOUBLE)) {
      return 0;
   }
    
   /* We set the Visual once. This will be our desktop (graphic mode).
    * We should do this in the `glutInit' code, but we don't have any idea
    * about its geometry. Supposedly, when we are about to create one
    * window, we have a slight idea about resolution.
    */
   if (!visual) {
      if ((visual=DMesaCreateVisual(_glut_default.x + m8width, _glut_default.y + _glut_default.height, _glut_visual.bpp, _glut_visual.refresh,
                                    GLUT_SINGLE,
                                    !(_glut_default.mode & GLUT_INDEX),
                                    (_glut_default.mode & GLUT_ALPHA  ) ? _glut_visual.alpha   : 0,
                                    (_glut_default.mode & GLUT_DEPTH  ) ? _glut_visual.depth   : 0,
                                    (_glut_default.mode & GLUT_STENCIL) ? _glut_visual.stencil : 0,
                                    (_glut_default.mode & GLUT_ACCUM  ) ? _glut_visual.accum   : 0))==NULL) {
         return 0;
      }

      DMesaGetIntegerv(DMESA_GET_SCREEN_SIZE, _glut_visual.geometry);
      DMesaGetIntegerv(DMESA_GET_DRIVER_CAPS, &_glut_visual.flags);

      /* Also hook stdio/stderr once */
      pc_open_stdout();
      pc_open_stderr();
      pc_atexit(clean);
   }

   /* Search for an empty slot.
    * Each window has its own rendering Context and its own Buffer.
    */
   for (i=0; i<MAX_WINDOWS; i++) {
      if (_glut_windows[i] == NULL) {
         DMesaContext c;
         DMesaBuffer b;
         GLUTwindow *w;

         if ((w = (GLUTwindow *)calloc(1, sizeof(GLUTwindow))) == NULL) {
            return 0;
         }

         /* Allocate the rendering Context. */
         if ((c = DMesaCreateContext(visual, NULL)) == NULL) {
            free(w);
            return 0;
         }

         /* Allocate the Buffer (displayable area).
          * We have to specify buffer size and position (inside the desktop).
          */
         if ((b = DMesaCreateBuffer(visual, _glut_default.x, _glut_default.y, m8width, _glut_default.height)) == NULL) {
            DMesaDestroyContext(c);
            free(w);
            return 0;
         }

         /* Bind Buffer to Context and make the Context the current one. */
         if (!DMesaMakeCurrent(c, b)) {
            DMesaDestroyBuffer(b);
            DMesaDestroyContext(c);
            free(w);
            return 0;
         }

         _glut_current = _glut_windows[i] = w;

         w->num = ++i;
         w->xpos = _glut_default.x;
         w->ypos = _glut_default.y;
         w->width = m8width;
         w->height = _glut_default.height;
         w->context = c;
         w->buffer = b;

         return i;
      }
   }

   return 0;
}


int APIENTRY
glutCreateSubWindow (int win, int x, int y, int width, int height)
{
   return GL_FALSE;
}


void APIENTRY
glutDestroyWindow (int win)
{
   GLUTwindow *w = _glut_window(win);
   if (w != NULL) {
      if (w->destroy) {
         w->destroy();
      }
      DMesaMakeCurrent(NULL, NULL);
      DMesaDestroyBuffer(w->buffer);
      DMesaDestroyContext(w->context);
      free(w);
      _glut_windows[win - 1] = NULL;
   }
}


void APIENTRY
glutPostRedisplay (void)
{
   _glut_current->redisplay = GL_TRUE;
}


void APIENTRY
glutSwapBuffers (void)
{
   if (_glut_current->show_mouse) {
      /* XXX scare mouse */
      DMesaSwapBuffers(_glut_current->buffer);
      /* XXX unscare mouse */
   } else {
      DMesaSwapBuffers(_glut_current->buffer);
   }

   if (_glut_fps) {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      swapcount++;
      if (swaptime == 0)
         swaptime = t;
      else if (t - swaptime > _glut_fps) {
         double time = 0.001 * (t - swaptime);
         double fps = (double)swapcount / time;
         fprintf(stderr, "GLUT: %d frames in %.2f seconds = %.2f FPS\n", swapcount, time, fps);
         swaptime = t;
         swapcount = 0;
      }
   }
}


int APIENTRY
glutGetWindow (void)
{
   return _glut_current->num;
}


void APIENTRY
glutSetWindow (int win)
{
   GLUTwindow *w = _glut_window(win);
   if (w != NULL) {
      _glut_current = w;
      DMesaMakeCurrent(_glut_current->context, _glut_current->buffer);
   }
}


void APIENTRY
glutSetWindowTitle (const char *title)
{
}


void APIENTRY
glutSetIconTitle (const char *title)
{
}


void APIENTRY
glutPositionWindow (int x, int y)
{
   if (DMesaMoveBuffer(x, y)) {
      _glut_current->xpos = x;
      _glut_current->ypos = y;
   }
}


void APIENTRY
glutReshapeWindow (int width, int height)
{ 
   if (DMesaResizeBuffer(width, height)) {
      _glut_current->width = width;
      _glut_current->height = height;
      if (_glut_current->reshape) {
         _glut_current->reshape(width, height);
      } else {
         glViewport(0, 0, width, height);
      }
   }
}


void APIENTRY
glutFullScreen (void)
{
}


void APIENTRY
glutPopWindow (void)
{
}


void APIENTRY
glutPushWindow (void)
{
}


void APIENTRY
glutIconifyWindow (void)
{
}


void APIENTRY
glutShowWindow (void)
{
}


void APIENTRY
glutHideWindow (void)
{
}


void APIENTRY
glutCloseFunc (GLUTdestroyCB destroy)
{
   _glut_current->destroy = destroy;
}


void APIENTRY
glutPostWindowRedisplay (int win)
{
   GLUTwindow *w = _glut_window(win);
   if (w != NULL) {
      w->redisplay = GL_TRUE;
   }
}


void * APIENTRY
glutGetWindowData (void)
{
   return _glut_current->data;
}


void APIENTRY
glutSetWindowData (void *data)
{
   _glut_current->data = data;
}
