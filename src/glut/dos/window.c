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
 * DOS/DJGPP glut driver v1.3 for Mesa 5.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <stdio.h>

#include "glutint.h"
#include "GL/dmesa.h"



GLUTwindow *g_curwin;
static GLuint swaptime, swapcount;

static DMesaVisual  visual  = NULL;
static DMesaContext context = NULL;
static GLUTwindow *windows[MAX_WINDOWS];



static void clean (void)
{
 int i;

 for (i=1; i<=MAX_WINDOWS; i++) {
     glutDestroyWindow(i);
 }
 if (context) DMesaDestroyContext(context);
 if (visual)  DMesaDestroyVisual(visual);

 pc_close_stdout();
 pc_close_stderr();
}



int APIENTRY glutCreateWindow (const char *title)
{
 int i;
 int m8width = (g_init_w + 7) & ~7;

 if (!visual) {
    if ((visual=DMesaCreateVisual(g_init_x + m8width, g_init_y + g_init_h, g_bpp, g_refresh,
                                  g_display_mode & GLUT_DOUBLE,
                                  !(g_display_mode & GLUT_INDEX),
                                  g_display_mode & GLUT_ALPHA,
                                  g_display_mode & GLUT_DEPTH  ?DEPTH_SIZE  :0,
                                  g_display_mode & GLUT_STENCIL?STENCIL_SIZE:0,
                                  g_display_mode & GLUT_ACCUM  ?ACCUM_SIZE  :0))==NULL) {
       return 0;
    }

    if ((context=DMesaCreateContext(visual, NULL))==NULL) {
       DMesaDestroyVisual(visual);
       return 0;
    }

    pc_open_stdout();
    pc_open_stderr();
    pc_atexit(clean);
 }

 for (i=0; i<MAX_WINDOWS; i++) {
     if (windows[i] == NULL) {
        DMesaBuffer b;
        GLUTwindow *w;

        if ((w=(GLUTwindow *)calloc(1, sizeof(GLUTwindow))) == NULL) {
           return 0;
        }

        if ((b=DMesaCreateBuffer(visual, g_init_x, g_init_y, m8width, g_init_h))==NULL) {
           free(w);
           return 0;
        }
        if (!DMesaMakeCurrent(context, b)) {
           DMesaDestroyBuffer(b);
           free(w);
           return 0;
        }

        g_curwin = windows[i] = w;

        w->num = ++i;
        w->xpos = g_init_x;
        w->ypos = g_init_y;
        w->width = m8width;
        w->height = g_init_h;
        w->buffer = b;

        return i;
     }
 }

 return 0;
}



int APIENTRY glutCreateSubWindow (int win, int x, int y, int width, int height)
{
 return GL_FALSE;
}



void APIENTRY glutDestroyWindow (int win)
{
 if (windows[--win]) {
    DMesaDestroyBuffer(windows[win]->buffer);
    free(windows[win]);
    windows[win] = NULL;
 }
}



void APIENTRY glutPostRedisplay (void)
{
 g_redisplay = GL_TRUE;
}



void APIENTRY glutSwapBuffers (void)
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



int APIENTRY glutGetWindow (void)
{
 return g_curwin->num;
}



void APIENTRY glutSetWindow (int win)
{
 g_curwin = windows[win - 1];
}



void APIENTRY glutSetWindowTitle (const char *title)
{
}



void APIENTRY glutSetIconTitle (const char *title)
{
}



void APIENTRY glutPositionWindow (int x, int y)
{
 if (DMesaMoveBuffer(x, y)) {
    g_curwin->xpos = x;
    g_curwin->ypos = y;
 }
}



void APIENTRY glutReshapeWindow (int width, int height)
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



void APIENTRY glutFullScreen (void)
{
}



void APIENTRY glutPopWindow (void)
{
}



void APIENTRY glutPushWindow (void)
{
}



void APIENTRY glutIconifyWindow (void)
{
}



void APIENTRY glutShowWindow (void)
{
}



void APIENTRY glutHideWindow (void)
{
}
