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
 * DOS/DJGPP glut driver v0.1 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "GL/glut.h"
#include "GL/dmesa.h"
#include "internal.h"


static DMesaVisual  visual  = NULL;
static DMesaContext context = NULL;
static DMesaBuffer  buffer  = NULL;


static void clean (void)
{
 __asm__("movw $3, %%ax; int $0x10":::"%eax");
 pc_close_stdout();
 pc_close_stderr();
}

int APIENTRY glutCreateWindow (const char *title)
{
 if ((visual=DMesaCreateVisual(COLOR_DEPTH,
                               g_display_mode & GLUT_DOUBLE,
                               g_display_mode & GLUT_DEPTH  ?DEPTH_SIZE  :0,
                               g_display_mode & GLUT_STENCIL?STENCIL_SIZE:0,
                               g_display_mode & GLUT_ACCUM  ?ACCUM_SIZE  :0))==NULL) {
    return GL_FALSE;
 }

 if ((context=DMesaCreateContext(visual, NULL))==NULL) {
    DMesaDestroyVisual(visual);
    return GL_FALSE;
 }

 if ((buffer=DMesaCreateBuffer(visual, g_width, g_height, g_xpos, g_ypos))==NULL) {
    DMesaDestroyContext(context);
    DMesaDestroyVisual(visual);
    return GL_FALSE;
 }

 if (!DMesaMakeCurrent(context, buffer)) {
    DMesaDestroyContext(context);
    DMesaDestroyVisual(visual);
    return GL_FALSE;
 }

 pc_open_stdout();
 pc_open_stderr();
 pc_atexit(clean);

 return GL_TRUE;
}


int APIENTRY glutCreateSubWindow (int win, int x, int y, int width, int height)
{
 return GL_FALSE;
}


void APIENTRY glutDestroyWindow (int win)
{
}


void APIENTRY glutPostRedisplay (void)
{
 g_redisplay = GL_TRUE;
}


void APIENTRY glutSwapBuffers (void)
{
 if (g_mouse) pc_scare_mouse();
 DMesaSwapBuffers(buffer);
 if (g_mouse) pc_unscare_mouse();
}


int APIENTRY glutGetWindow (void)
{
 return 0;
}


void APIENTRY glutSetWindow (int win)
{
}


void APIENTRY glutSetWindowTitle (const char *title)
{
}


void APIENTRY glutSetIconTitle (const char *title)
{
}


void APIENTRY glutPositionWindow (int x, int y)
{
}


void APIENTRY glutReshapeWindow (int width, int height)
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
