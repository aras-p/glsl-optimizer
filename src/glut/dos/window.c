/*
 * Mesa 3-D graphics library
 * Version:  4.0
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
 * DOS/DJGPP glut driver v1.0 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "GL/glut.h"
#include "GL/dmesa.h"
#include "internal.h"



static int window;

static DMesaVisual  visual  = NULL;
static DMesaContext context = NULL;
static DMesaBuffer  buffer[MAX_WINDOWS];



static void clean (void)
{
 int i;

 for (i=0; i<MAX_WINDOWS; i++) {
     glutDestroyWindow(i+1);
 }
 if (context) DMesaDestroyContext(context);
 if (visual)  DMesaDestroyVisual(visual);

 pc_close_stdout();
 pc_close_stderr();
}



int APIENTRY glutCreateWindow (const char *title)
{
 int i;

 if (!visual) {
    int screen_w = DEFAULT_WIDTH;
    int screen_h = DEFAULT_HEIGHT;

    if ((g_width<=640) && (g_height<=480)) {
       screen_w = 640;
       screen_h = 480;
    } else if ((g_width<=800) && (g_height<=600)) {
       screen_w = 800;
       screen_h = 600;
    } else if ((g_width<=1024) && (g_height<=768)) {
       screen_w = 1024;
       screen_h = 768;
    }

    if ((visual=DMesaCreateVisual(screen_w, screen_h, DEFAULT_BPP,
                                  g_display_mode & GLUT_DOUBLE,
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
     if (!buffer[i]) {
        DMesaBuffer b;
     
        if ((b=DMesaCreateBuffer(visual, g_xpos, g_ypos, g_width, g_height))==NULL) {
           return 0;
        }
        if (!DMesaMakeCurrent(context, b)) {
           DMesaDestroyBuffer(b);
           return 0;
        }
        if (g_mouse) {
           pc_mouse_area(g_xpos, g_ypos, g_xpos + g_width - 1, g_ypos + g_height - 1);
        }

        buffer[window = i] = b;
        return i+1;
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
 if (buffer[win-1]) {
    DMesaDestroyBuffer(buffer[win-1]);
    buffer[win-1] = NULL;
 }
}


void APIENTRY glutPostRedisplay (void)
{
 g_redisplay = GL_TRUE;
}


void APIENTRY glutSwapBuffers (void)
{
 if (g_mouse) pc_scare_mouse();
 DMesaSwapBuffers(buffer[window]);
 if (g_mouse) pc_unscare_mouse();
}


int APIENTRY glutGetWindow (void)
{
 return window + 1;
}


void APIENTRY glutSetWindow (int win)
{
 window = win - 1;
}


void APIENTRY glutSetWindowTitle (const char *title)
{
}


void APIENTRY glutSetIconTitle (const char *title)
{
}


void APIENTRY glutPositionWindow (int x, int y)
{
 if (DMesaViewport(buffer[window], x, y, g_width, g_height)) {
    g_xpos = x;
    g_ypos = y;
 }
}


void APIENTRY glutReshapeWindow (int width, int height)
{
 if (DMesaViewport(buffer[window], g_xpos, g_ypos, width, height)) {
    g_width = width;
    g_height = height;
    if (reshape_func) {
       reshape_func(width, height);
    } else {
       glViewport(0, 0, width, height);
    }
 }
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
