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

 
#ifndef INTERNAL_H_included
#define INTERNAL_H_included


#include "GL/glut.h"
/* #include "pc_hw/pc_hw.h" */


#define MAX_WINDOWS    4

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define DEFAULT_BPP    16

#define DEPTH_SIZE   16
#define STENCIL_SIZE 8
#define ACCUM_SIZE   16

extern GLenum    g_display_mode;
extern GLuint    g_width;
extern GLuint    g_height;
extern GLint     g_mouse;
extern GLboolean g_redisplay;
extern GLint     g_xpos;
extern GLint     g_ypos;

extern void (GLUTCALLBACK *display_func) (void);
extern void (GLUTCALLBACK *reshape_func) (int width, int height);
extern void (GLUTCALLBACK *keyboard_func) (unsigned char key, int x, int y);
extern void (GLUTCALLBACK *mouse_func) (int button, int state, int x, int y);
extern void (GLUTCALLBACK *motion_func) (int x, int y);
extern void (GLUTCALLBACK *passive_motion_func) (int x, int y);
extern void (GLUTCALLBACK *entry_func) (int state);
extern void (GLUTCALLBACK *visibility_func) (int state);
extern void (GLUTCALLBACK *idle_func) (void);
extern void (GLUTCALLBACK *menu_state_func) (int state);
extern void (GLUTCALLBACK *special_func) (int key, int x, int y);
extern void (GLUTCALLBACK *spaceball_motion_func) (int x, int y, int z);
extern void (GLUTCALLBACK *spaceball_rotate_func) (int x, int y, int z);
extern void (GLUTCALLBACK *spaceball_button_func) (int button, int state);
extern void (GLUTCALLBACK *button_box_func) (int button, int state);
extern void (GLUTCALLBACK *dials_func) (int dial, int value);
extern void (GLUTCALLBACK *tablet_motion_func) (int x, int y);
extern void (GLUTCALLBACK *tabled_button_func) (int button, int state, int x, int y);
extern void (GLUTCALLBACK *menu_status_func) (int status, int x, int y);
extern void (GLUTCALLBACK *overlay_display_func) (void);
extern void (GLUTCALLBACK *window_status_func) (int state);

#endif
