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

GLenum    g_display_mode = 0;
GLuint    g_width        = DEFAULT_WIDTH;
GLuint    g_height       = DEFAULT_HEIGHT;
GLint     g_mouse        = GL_FALSE;
GLboolean g_redisplay    = GL_FALSE;
GLint     g_xpos         = 0;
GLint     g_ypos         = 0;

void (GLUTCALLBACK *display_func) (void)                              = 0;
void (GLUTCALLBACK *reshape_func) (int width, int height)             = 0;
void (GLUTCALLBACK *keyboard_func) (unsigned char key, int x, int y)  = 0;
void (GLUTCALLBACK *mouse_func) (int button, int state, int x, int y) = 0;
void (GLUTCALLBACK *motion_func) (int x, int y)                       = 0;
void (GLUTCALLBACK *passive_motion_func) (int x, int y)               = 0;
void (GLUTCALLBACK *entry_func) (int state)                           = 0;
void (GLUTCALLBACK *visibility_func) (int state)                      = 0;
void (GLUTCALLBACK *idle_func) (void)                                 = 0;
void (GLUTCALLBACK *menu_state_func) (int state)                      = 0;
void (GLUTCALLBACK *special_func) (int key, int x, int y)             = 0;
void (GLUTCALLBACK *spaceball_motion_func) (int x, int y, int z)      = 0;
void (GLUTCALLBACK *spaceball_rotate_func) (int x, int y, int z)      = 0;
void (GLUTCALLBACK *spaceball_button_func) (int button, int state)    = 0;
void (GLUTCALLBACK *button_box_func) (int button, int state)          = 0;
void (GLUTCALLBACK *dials_func) (int dial, int value)                 = 0;
void (GLUTCALLBACK *tablet_motion_func) (int x, int y)                = 0;
void (GLUTCALLBACK *tabled_button_func) (int button, int state, int x, int y) = 0;
void (GLUTCALLBACK *menu_status_func) (int status, int x, int y)      = 0;
void (GLUTCALLBACK *overlay_display_func) (void)                      = 0;
void (GLUTCALLBACK *window_status_func) (int state)                   = 0;
