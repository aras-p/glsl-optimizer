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


#include <signal.h>
#include "GL/glut.h"
#include "internal.h"


static void *old_sig_int  = NULL;


static void signal_handler (int num)
{
 signal(SIGINT, old_sig_int);

 raise(num);
}


void APIENTRY glutInit (int *argcp, char **argv)
{
 /* Hack alert:
    only SIGINT (but not Ctrl-Break)
    calls the destructors and will safely clean up
 */
 old_sig_int = signal(SIGINT, signal_handler);
}


void APIENTRY glutInitDisplayMode (unsigned int mode)
{
 g_display_mode = mode;

 pc_install_keyb();
 g_mouse = pc_install_mouse();
}


void APIENTRY glutInitWindowPosition (int x, int y)
{
 g_xpos = x;
 g_ypos = y;
}


void APIENTRY glutInitWindowSize (int width, int height)
{
 g_width  = width;
 g_height = height;
}


void APIENTRY glutMainLoop (void)
{
 GLboolean idle;
 static int old_mouse_x = 0;
 static int old_mouse_y = 0;
 static int old_mouse_b = 0;

 glutPostRedisplay();
 if (reshape_func) reshape_func(g_width, g_height);
 if (visibility_func) visibility_func(GLUT_VISIBLE);
 if (g_mouse) pc_show_mouse();

 while (GL_TRUE) {
       idle = GL_TRUE;

       if (g_redisplay && display_func) {
          idle        = GL_FALSE;
          g_redisplay = GL_FALSE;

          if (g_mouse && !(g_display_mode & GLUT_DOUBLE)) pc_scare_mouse();
          display_func();
          if (g_mouse && !(g_display_mode & GLUT_DOUBLE)) pc_unscare_mouse();
       }

       if (pc_keypressed()) {
          int key;

          idle = GL_FALSE;
          key  = pc_readkey();

          switch (key>>16) {
                 case KEY_F1:     if (special_func) special_func(GLUT_KEY_F1,        0, 0); break;
                 case KEY_F2:     if (special_func) special_func(GLUT_KEY_F2,        0, 0); break;
                 case KEY_F3:     if (special_func) special_func(GLUT_KEY_F3,        0, 0); break;
                 case KEY_F4:     if (special_func) special_func(GLUT_KEY_F4,        0, 0); break;
                 case KEY_F5:     if (special_func) special_func(GLUT_KEY_F5,        0, 0); break;
                 case KEY_F6:     if (special_func) special_func(GLUT_KEY_F6,        0, 0); break;
                 case KEY_F7:     if (special_func) special_func(GLUT_KEY_F7,        0, 0); break;
                 case KEY_F8:     if (special_func) special_func(GLUT_KEY_F8,        0, 0); break;
                 case KEY_F9:     if (special_func) special_func(GLUT_KEY_F9,        0, 0); break;
                 case KEY_F10:    if (special_func) special_func(GLUT_KEY_F10,       0, 0); break;
                 case KEY_F11:    if (special_func) special_func(GLUT_KEY_F11,       0, 0); break;
                 case KEY_F12:    if (special_func) special_func(GLUT_KEY_F12,       0, 0); break;
                 case KEY_LEFT:   if (special_func) special_func(GLUT_KEY_LEFT,      0, 0); break;
                 case KEY_UP:     if (special_func) special_func(GLUT_KEY_UP,        0, 0); break;
                 case KEY_RIGHT:  if (special_func) special_func(GLUT_KEY_RIGHT,     0, 0); break;
                 case KEY_DOWN:   if (special_func) special_func(GLUT_KEY_DOWN,      0, 0); break;
                 case KEY_PGUP:   if (special_func) special_func(GLUT_KEY_PAGE_UP,   0, 0); break;
                 case KEY_PGDN:   if (special_func) special_func(GLUT_KEY_PAGE_DOWN, 0, 0); break;
                 case KEY_HOME:   if (special_func) special_func(GLUT_KEY_HOME,      0, 0); break;
                 case KEY_END:    if (special_func) special_func(GLUT_KEY_END,       0, 0); break;
                 case KEY_INSERT: if (special_func) special_func(GLUT_KEY_INSERT,    0, 0); break;
                 default:         if (keyboard_func) keyboard_func(key & 0xFF, 0, 0);
          }
       }

       if (g_mouse && motion_func && ((pc_mouse_x != old_mouse_x) || (pc_mouse_y != old_mouse_y))) {
          idle        = GL_FALSE;
          old_mouse_x = pc_mouse_x;
          old_mouse_y = pc_mouse_y;

          motion_func(old_mouse_x, old_mouse_y);
       }

       if (g_mouse && mouse_func && (pc_mouse_b != old_mouse_b)) {
          int new_mouse_b = pc_mouse_b;

          if ((old_mouse_b & 1) && !(new_mouse_b & 1))
             mouse_func(GLUT_LEFT_BUTTON, GLUT_UP,   pc_mouse_x, pc_mouse_y);
          else if (!(old_mouse_b & 1) && (new_mouse_b & 1))
             mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN, pc_mouse_x, pc_mouse_y);

          if ((old_mouse_b & 2) && !(new_mouse_b & 2))
             mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP,   pc_mouse_x, pc_mouse_y);
          else if (!(old_mouse_b & 2) && (new_mouse_b & 2))
             mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN, pc_mouse_x, pc_mouse_y);

          if ((old_mouse_b & 4) && !(new_mouse_b & 4))
             mouse_func(GLUT_MIDDLE_BUTTON, GLUT_UP,   pc_mouse_x, pc_mouse_y);
          else if (!(old_mouse_b & 3) && (new_mouse_b & 4))
             mouse_func(GLUT_MIDDLE_BUTTON, GLUT_DOWN, pc_mouse_x, pc_mouse_y);

          idle        = GL_FALSE;
          old_mouse_b = new_mouse_b;
       }

       if (idle && idle_func)
          idle_func();
 }
}
