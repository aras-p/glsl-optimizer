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


#include <string.h>

#include <GL/glut.h>
#include "GL/dmesa.h"

#include "PC_HW/pc_hw.h"
#include "internal.h"


static int looping = 0;


#define DO_REDISPLAY(w, ccin, ccout) \
   do {                                                            \
      if (w->redisplay && w->display) {                            \
         int rv = GL_TRUE;                                         \
                                                                   \
         idle         = GL_FALSE;                                  \
         w->redisplay = GL_FALSE;                                  \
                                                                   \
         /* test IN condition (whether we need to `MakeCurrent') */\
         if (ccin) {                                               \
            rv = DMesaMakeCurrent(w->context, w->buffer);          \
         }                                                         \
                                                                   \
         /* do the display only if `MakeCurrent' didn't failed */  \
         if (rv) {                                                 \
            if (w->show_mouse && !(_glut_default.mode & GLUT_DOUBLE)) {\
               /* XXX scare mouse */                               \
               w->display();                                       \
               /* XXX unscare mouse */                             \
            } else {                                               \
               w->display();                                       \
            }                                                      \
                                                                   \
            /* update OUT condition */                             \
            ccout;                                                 \
         }                                                         \
      }                                                            \
   } while (0)


void APIENTRY
glutMainLoopEvent (void)
{
   int i, n;
   GLUTwindow *w;
   GLboolean idle;
   static int old_mouse_x = 0;
   static int old_mouse_y = 0;
   static int old_mouse_b = 0;

   static GLboolean virgin = GL_TRUE;
   if (virgin) {
      pc_install_keyb();
      _glut_mouse_init();

      for (i = 0; i < MAX_WINDOWS; i++) {
         w = _glut_windows[i];
         if (w != NULL) {
            glutSetWindow(w->num);
            glutPostRedisplay();
            if (w->reshape) {
               w->reshape(w->width, w->height);
            }
            if (w->visibility) {
               w->visibility(GLUT_VISIBLE);
            }
         }
      }
      virgin = GL_FALSE;
   }

   idle = GL_TRUE;

   n = 0;
   for (i = 0; i < MAX_WINDOWS; i++) {
      w = _glut_windows[i];
      if ((w != NULL) && (w != _glut_current)) {
         /* 1) redisplay `w'
          * 2) `MakeCurrent' always
          * 3) update number of non-default windows
          */
         DO_REDISPLAY(w, GL_TRUE, n++);
      }
   }
   /* 1) redisplay `_glut_current'
    * 2) `MakeCurrent' only if we previously did non-default windows
    * 3) don't update anything
    */
   DO_REDISPLAY(_glut_current, n, n);

   if (_glut_mouse) {
      int mouse_x;
      int mouse_y;
      int mouse_z;
      int mouse_b;

      /* query mouse */
      mouse_b = pc_query_mouse(&mouse_x, &mouse_y, &mouse_z);

      /* relative to window coordinates */
      _glut_mouse_x = mouse_x - _glut_current->xpos;
      _glut_mouse_y = mouse_y - _glut_current->ypos;

      /* mouse was moved? */
      if ((mouse_x != old_mouse_x) || (mouse_y != old_mouse_y)) {
         idle        = GL_FALSE;
         old_mouse_x = mouse_x;
         old_mouse_y = mouse_y;

         if (mouse_b) {
            /* any button pressed */
            if (_glut_current->motion) {
               _glut_current->motion(_glut_mouse_x, _glut_mouse_y);
            }
         } else {
            /* no button pressed */
            if (_glut_current->passive) {
               _glut_current->passive(_glut_mouse_x, _glut_mouse_y);
            }
         }
      }

      /* button state changed? */
      if (mouse_b != old_mouse_b) {
         GLUTmouseCB mouse_func;

         if ((mouse_func = _glut_current->mouse)) {
            if ((old_mouse_b & 1) && !(mouse_b & 1))
               mouse_func(GLUT_LEFT_BUTTON, GLUT_UP,     _glut_mouse_x, _glut_mouse_y);
            else if (!(old_mouse_b & 1) && (mouse_b & 1))
               mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN,   _glut_mouse_x, _glut_mouse_y);

            if ((old_mouse_b & 2) && !(mouse_b & 2))
               mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP,    _glut_mouse_x, _glut_mouse_y);
            else if (!(old_mouse_b & 2) && (mouse_b & 2))
               mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN,  _glut_mouse_x, _glut_mouse_y);

            if ((old_mouse_b & 4) && !(mouse_b & 4))
               mouse_func(GLUT_MIDDLE_BUTTON, GLUT_UP,   _glut_mouse_x, _glut_mouse_y);
            else if (!(old_mouse_b & 3) && (mouse_b & 4))
               mouse_func(GLUT_MIDDLE_BUTTON, GLUT_DOWN, _glut_mouse_x, _glut_mouse_y);
         }

         idle        = GL_FALSE;
         old_mouse_b = mouse_b;
      }
   }

   if (pc_keypressed()) {
      int key;
      int glut_key;

      idle = GL_FALSE;
      key  = pc_readkey();

      switch (key>>16) {
         case KEY_F1:     glut_key = GLUT_KEY_F1;        goto special;
         case KEY_F2:     glut_key = GLUT_KEY_F2;        goto special;
         case KEY_F3:     glut_key = GLUT_KEY_F3;        goto special;
         case KEY_F4:     glut_key = GLUT_KEY_F4;        goto special;
         case KEY_F5:     glut_key = GLUT_KEY_F5;        goto special;
         case KEY_F6:     glut_key = GLUT_KEY_F6;        goto special;
         case KEY_F7:     glut_key = GLUT_KEY_F7;        goto special;
         case KEY_F8:     glut_key = GLUT_KEY_F8;        goto special;
         case KEY_F9:     glut_key = GLUT_KEY_F9;        goto special;
         case KEY_F10:    glut_key = GLUT_KEY_F10;       goto special;
         case KEY_F11:    glut_key = GLUT_KEY_F11;       goto special;
         case KEY_F12:    glut_key = GLUT_KEY_F12;       goto special;
         case KEY_LEFT:   glut_key = GLUT_KEY_LEFT;      goto special;
         case KEY_UP:     glut_key = GLUT_KEY_UP;        goto special;
         case KEY_RIGHT:  glut_key = GLUT_KEY_RIGHT;     goto special;
         case KEY_DOWN:   glut_key = GLUT_KEY_DOWN;      goto special;
         case KEY_PGUP:   glut_key = GLUT_KEY_PAGE_UP;   goto special;
         case KEY_PGDN:   glut_key = GLUT_KEY_PAGE_DOWN; goto special;
         case KEY_HOME:   glut_key = GLUT_KEY_HOME;      goto special;
         case KEY_END:    glut_key = GLUT_KEY_END;       goto special;
         case KEY_INSERT: glut_key = GLUT_KEY_INSERT;    goto special;
         special:
            if (_glut_current->special) {
               _glut_current->special(glut_key, _glut_mouse_x, _glut_mouse_y);
            }
            break;
         default:
            if (_glut_current->keyboard) {
               _glut_current->keyboard(key & 0xFF, _glut_mouse_x, _glut_mouse_y);
            }
      }
   }

   if (idle && _glut_idle_func)
      _glut_idle_func();

   for (i = 0; i < MAX_TIMER_CB; i++) {
      int time = glutGet(GLUT_ELAPSED_TIME);
      GLUTSShotCB *cb = &_glut_timer_cb[i];
      if (cb->func && (time >= cb->time)) {
         cb->func(cb->value);
         cb->func = NULL;
      }
   }
}


void APIENTRY
glutMainLoop (void)
{
   looping++;
   while (looping) {
      glutMainLoopEvent();
   }
}


void APIENTRY
glutLeaveMainLoop (void)
{
   looping--;
}
