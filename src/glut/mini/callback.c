/*
 * Mesa 3-D graphics library
 * Version:  3.4
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
#include "internal.h"


void APIENTRY glutDisplayFunc (void (GLUTCALLBACK *func) (void))
{
 display_func = func;
}


void APIENTRY glutReshapeFunc (void (GLUTCALLBACK *func) (int width, int height))
{
 reshape_func = func;
}


void APIENTRY glutKeyboardFunc (void (GLUTCALLBACK *func) (unsigned char key, int x, int y))
{
 keyboard_func = func;
}


void APIENTRY glutMouseFunc (void (GLUTCALLBACK *func) (int button, int state, int x, int y))
{
 mouse_func = func;
}


void APIENTRY glutMotionFunc (void (GLUTCALLBACK *func) (int x, int y))
{
 motion_func = func;
}


void APIENTRY glutPassiveMotionFunc (void (GLUTCALLBACK *func) (int x, int y))
{
 passive_motion_func = func;
}


void APIENTRY glutEntryFunc (void (GLUTCALLBACK *func) (int state))
{
 entry_func = func;
}


void APIENTRY glutVisibilityFunc (void (GLUTCALLBACK *func) (int state))
{
 visibility_func = func;
}


void APIENTRY glutIdleFunc (void (GLUTCALLBACK *func) (void))
{
 idle_func = func;
}


void APIENTRY glutTimerFunc (unsigned int millis, void (GLUTCALLBACK *func) (int value), int value)
{
}


void APIENTRY glutMenuStateFunc (void (GLUTCALLBACK *func) (int state))
{
 menu_state_func = func;
}


void APIENTRY glutSpecialFunc (void (GLUTCALLBACK *func) (int key, int x, int y))
{
 special_func = func;
}


void APIENTRY glutSpaceballMotionFunc (void (GLUTCALLBACK *func) (int x, int y, int z))
{
}


void APIENTRY glutSpaceballRotateFunc (void (GLUTCALLBACK *func) (int x, int y, int z))
{
}


void APIENTRY glutSpaceballButtonFunc (void (GLUTCALLBACK *func) (int button, int state))
{
}


void APIENTRY glutButtonBoxFunc (void (GLUTCALLBACK *func) (int button, int state))
{
}


void APIENTRY glutDialsFunc (void (GLUTCALLBACK *func) (int dial, int value))
{
}


void APIENTRY glutTabletMotionFunc (void (GLUTCALLBACK *func) (int x, int y))
{
}


void APIENTRY glutTabletButtonFunc (void (GLUTCALLBACK *func) (int button, int state, int x, int y))
{
}


void APIENTRY glutMenuStatusFunc (void (GLUTCALLBACK *func) (int status, int x, int y))
{
}


void APIENTRY glutOverlayDisplayFunc (void (GLUTCALLBACK *func) (void))
{
}


void APIENTRY glutWindowStatusFunc (void (GLUTCALLBACK *func) (int state))
{
}
