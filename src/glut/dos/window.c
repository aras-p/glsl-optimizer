/*
 * Mesa 3-D graphics library
 * Version:  4.1
 * Copyright (C) 1995-1998  Brian Paul
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
 * DOS/DJGPP glut driver v1.4 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#include <stdio.h>

#include "glutint.h"
#include "GL/dmesa.h"


GLUTwindow *g_curwin;
static GLuint swaptime, swapcount;

static DMesaVisual visual = NULL;
GLUTwindow *g_windows[MAX_WINDOWS];


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


int APIENTRY
glutCreateWindow (const char *title)
{
   int i;
   int m8width = (g_init_w + 7) & ~7;

   /* We set the Visual once. This will be our desktop (graphic mode).
    * We should do this in the `glutInit' code, but we don't have any idea
    * about its geometry. Supposedly, when we are about to create one
    * window, we have a slight idea about resolution.
    */
   if (!visual) {
      if ((visual=DMesaCreateVisual(g_init_x + m8width, g_init_y + g_init_h, g_bpp, g_refresh,
                                    g_display_mode & GLUT_DOUBLE,
                                    !(g_display_mode & GLUT_INDEX),
                                    (g_display_mode & GLUT_ALPHA  ) ? g_alpha   : 0,
                                    (g_display_mode & GLUT_DEPTH  ) ? g_depth   : 0,
                                    (g_display_mode & GLUT_STENCIL) ? g_stencil : 0,
                                    (g_display_mode & GLUT_ACCUM  ) ? g_accum   : 0))==NULL) {
         return 0;
      }

      /* Also hook stdio/stderr once */
      pc_open_stdout();
      pc_open_stderr();
      pc_atexit(clean);
   }

   /* Search for an empty slot.
    * Each window has its own rendering Context and its own Buffer.
    */
   for (i=0; i<MAX_WINDOWS; i++) {
      if (g_windows[i] == NULL) {
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
         if ((b = DMesaCreateBuffer(visual, g_init_x, g_init_y, m8width, g_init_h)) == NULL) {
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

         g_curwin = g_windows[i] = w;

         w->num = ++i;
         w->xpos = g_init_x;
         w->ypos = g_init_y;
         w->width = m8width;
         w->height = g_init_h;
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
   if (g_windows[--win]) {
      GLUTwindow *w = g_windows[win];
      DMesaMakeCurrent(NULL, NULL);
      DMesaDestroyBuffer(w->buffer);
      DMesaDestroyContext(w->context);
      free(w);
      g_windows[win] = NULL;
   }
}


void APIENTRY
glutPostRedisplay (void)
{
   g_curwin->redisplay = GL_TRUE;
}


void APIENTRY
glutSwapBuffers (void)
{
   if (g_curwin->show_mouse) {
      /* XXX scare mouse */
      DMesaSwapBuffers(g_curwin->buffer);
      /* XXX unscare mouse */
   } else {
      DMesaSwapBuffers(g_curwin->buffer);
   }

   if (g_fps) {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      swapcount++;
      if (swaptime == 0)
         swaptime = t;
      else if (t - swaptime > g_fps) {
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
   return g_curwin->num;
}


void APIENTRY
glutSetWindow (int win)
{
   g_curwin = g_windows[win - 1];
   DMesaMakeCurrent(g_curwin->context, g_curwin->buffer);
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
      g_curwin->xpos = x;
      g_curwin->ypos = y;
   }
}


void APIENTRY
glutReshapeWindow (int width, int height)
{ 
   if (DMesaResizeBuffer(width, height)) {
      g_curwin->width = width;
      g_curwin->height = height;
      if (g_curwin->reshape) {
         g_curwin->reshape(width, height);
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
