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
 * DOS/DJGPP glut driver v1.2 for Mesa 4.1
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "GL/glut.h"
#ifndef FX
#include "GL/dmesa.h"
#else
#include "GL/fxmesa.h"
#endif
#include "internal.h"



static int window;

#ifndef FX
static DMesaVisual  visual  = NULL;
static DMesaContext context = NULL;
static DMesaBuffer  buffer[MAX_WINDOWS];
#else
static void *visual = NULL;
static fxMesaContext context = NULL;
static int fx_attrib[32];
#endif



static void clean (void)
{
 int i;

 for (i=0; i<MAX_WINDOWS; i++) {
     glutDestroyWindow(i+1);
 }
#ifndef FX
 if (context) DMesaDestroyContext(context);
 if (visual)  DMesaDestroyVisual(visual);
#else
 if (context) fxMesaDestroyContext(context);
#endif

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

#ifndef FX
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
#else
    i = 0;
    if (g_display_mode & GLUT_DOUBLE) fx_attrib[i++] = FXMESA_DOUBLEBUFFER;
    if (g_display_mode & GLUT_DEPTH) { fx_attrib[i++] = FXMESA_DEPTH_SIZE; fx_attrib[i++] = DEPTH_SIZE; }
    if (g_display_mode & GLUT_STENCIL) { fx_attrib[i++] = FXMESA_STENCIL_SIZE; fx_attrib[i++] = STENCIL_SIZE; }
    if (g_display_mode & GLUT_ACCUM) { fx_attrib[i++] = FXMESA_ACCUM_SIZE; fx_attrib[i++] = ACCUM_SIZE; }
    fx_attrib[i] = FXMESA_NONE;
    if ((context=fxMesaCreateBestContext(-1, screen_w, screen_h, fx_attrib))==NULL) {
       return 0;
    }
    visual = context;
#endif
    
    pc_open_stdout();
    pc_open_stderr();
    pc_atexit(clean);
 }

#ifndef FX
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
#else
 fxMesaMakeCurrent(context);

 return 1;
#endif
}


int APIENTRY glutCreateSubWindow (int win, int x, int y, int width, int height)
{
 return GL_FALSE;
}


void APIENTRY glutDestroyWindow (int win)
{
#ifndef FX
 if (buffer[win-1]) {
    DMesaDestroyBuffer(buffer[win-1]);
    buffer[win-1] = NULL;
 }
#endif
}


void APIENTRY glutPostRedisplay (void)
{
 g_redisplay = GL_TRUE;
}


void APIENTRY glutSwapBuffers (void)
{
 if (g_mouse) pc_scare_mouse();
#ifndef FX
 DMesaSwapBuffers(buffer[window]);
#else
 fxMesaSwapBuffers();
#endif
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
#ifndef FX
 if (DMesaViewport(buffer[window], x, y, g_width, g_height)) {
    g_xpos = x;
    g_ypos = y;
 }
#endif
}


void APIENTRY glutReshapeWindow (int width, int height)
{
#ifndef FX
 if (DMesaViewport(buffer[window], g_xpos, g_ypos, width, height)) {
    g_width = width;
    g_height = height;
    if (reshape_func) {
       reshape_func(width, height);
    } else {
       glViewport(0, 0, width, height);
    }
 }
#endif
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
