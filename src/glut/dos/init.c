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
 * DOS/DJGPP glut driver v1.5 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#include <string.h>

#include "glutint.h"

#define DEFAULT_WIDTH  300
#define DEFAULT_HEIGHT 300
#define DEFAULT_BPP    16

#define ALPHA_SIZE   8
#define DEPTH_SIZE   16
#define STENCIL_SIZE 8
#define ACCUM_SIZE   16


GLuint g_bpp = DEFAULT_BPP;
GLuint g_alpha = ALPHA_SIZE;
GLuint g_depth = DEPTH_SIZE;
GLuint g_stencil = STENCIL_SIZE;
GLuint g_accum = ACCUM_SIZE;
GLuint g_refresh = 0;
GLuint g_screen_w, g_screen_h;
GLint g_driver_caps;

GLuint g_fps = 0;

GLuint g_display_mode = 0;
int g_init_x = 0, g_init_y = 0;
GLuint g_init_w = DEFAULT_WIDTH, g_init_h = DEFAULT_HEIGHT;

char *__glutProgramName = NULL;


void APIENTRY
glutInit (int *argc, char **argv)
{
   char *str;
   const char *env;

   if ((env = getenv("DMESA_GLUT_BPP")) != NULL) {
      g_bpp = atoi(env);
   }
   if ((env = getenv("DMESA_GLUT_ALPHA")) != NULL) {
      g_alpha = atoi(env);
   }
   if ((env = getenv("DMESA_GLUT_DEPTH")) != NULL) {
      g_depth = atoi(env);
   }
   if ((env = getenv("DMESA_GLUT_STENCIL")) != NULL) {
      g_stencil = atoi(env);
   }
   if ((env = getenv("DMESA_GLUT_ACCUM")) != NULL) {
      g_accum = atoi(env);
   }
   if ((env = getenv("DMESA_GLUT_REFRESH")) != NULL) {
      g_refresh = atoi(env);
   }

   /* Determine program name. */
   str = strrchr(argv[0], '/');
   if (str == NULL) {
      str = argv[0];
   } else {
      str++;
   }
   __glutProgramName = __glutStrdup(str);

   /* check if GLUT_FPS env var is set */
   if ((env = getenv("GLUT_FPS")) != NULL) {
      if ((g_fps = atoi(env)) <= 0) {
         g_fps = 5000; /* 5000 milliseconds */
      }
   }

   /* Initialize timer */
   glutGet(GLUT_ELAPSED_TIME);
}


void APIENTRY
glutInitDisplayMode (unsigned int mode)
{
   g_display_mode = mode;
}


void APIENTRY
glutInitWindowPosition (int x, int y)
{
   g_init_x = x;
   g_init_y = y;
}


void APIENTRY
glutInitWindowSize (int width, int height)
{
   g_init_w = width;
   g_init_h = height;
}


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
            if (w->show_mouse && !(g_display_mode & GLUT_DOUBLE)) {\
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
glutMainLoop (void)
{
   int i, n;
   GLUTwindow *w;
   GLboolean idle;
   static int old_mouse_x = 0;
   static int old_mouse_y = 0;
   static int old_mouse_b = 0;

   {
      GLint screen_size[2];
      DMesaGetIntegerv(DMESA_GET_SCREEN_SIZE, screen_size);
      g_screen_w = screen_size[0];
      g_screen_h = screen_size[1];
      DMesaGetIntegerv(DMESA_GET_DRIVER_CAPS, &g_driver_caps);
   }

   pc_install_keyb();
   __glutInitMouse();

   for (i = 0; i < MAX_WINDOWS; i++) {
      w = g_windows[i];
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

   while (GL_TRUE) {
      idle = GL_TRUE;

      n = 0;
      for (i = 0; i < MAX_WINDOWS; i++) {
         w = g_windows[i];
         if ((w != NULL) && (w != g_curwin)) {
            /* 1) redisplay `w'
             * 2) `MakeCurrent' always
             * 3) update number of non-default windows
             */
            DO_REDISPLAY(w, GL_TRUE, n++);
         }
      }
      /* 1) redisplay `g_curwin'
       * 2) `MakeCurrent' only if we previously did non-default windows
       * 3) don't update anything
       */
      DO_REDISPLAY(g_curwin, n, n);

      if (g_mouse) {
         int mouse_x;
         int mouse_y;
         int mouse_z;
         int mouse_b;

         /* query mouse */
         mouse_b = pc_query_mouse(&mouse_x, &mouse_y, &mouse_z);

         /* relative to window coordinates */
         g_mouse_x = mouse_x - g_curwin->xpos;
         g_mouse_y = mouse_y - g_curwin->ypos;

         /* mouse was moved? */
         if ((mouse_x != old_mouse_x) || (mouse_y != old_mouse_y)) {
            idle        = GL_FALSE;
            old_mouse_x = mouse_x;
            old_mouse_y = mouse_y;

            if (mouse_b) {
               /* any button pressed */
               if (g_curwin->motion) {
                  g_curwin->motion(g_mouse_x, g_mouse_y);
               }
            } else {
               /* no button pressed */
               if (g_curwin->passive) {
                  g_curwin->passive(g_mouse_x, g_mouse_y);
               }
            }
         }

         /* button state changed? */
         if (mouse_b != old_mouse_b) {
            GLUTmouseCB mouse_func;

            if ((mouse_func = g_curwin->mouse)) {
               if ((old_mouse_b & 1) && !(mouse_b & 1))
                  mouse_func(GLUT_LEFT_BUTTON, GLUT_UP,     g_mouse_x, g_mouse_y);
               else if (!(old_mouse_b & 1) && (mouse_b & 1))
                  mouse_func(GLUT_LEFT_BUTTON, GLUT_DOWN,   g_mouse_x, g_mouse_y);

               if ((old_mouse_b & 2) && !(mouse_b & 2))
                  mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP,    g_mouse_x, g_mouse_y);
               else if (!(old_mouse_b & 2) && (mouse_b & 2))
                  mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN,  g_mouse_x, g_mouse_y);

               if ((old_mouse_b & 4) && !(mouse_b & 4))
                  mouse_func(GLUT_MIDDLE_BUTTON, GLUT_UP,   g_mouse_x, g_mouse_y);
               else if (!(old_mouse_b & 3) && (mouse_b & 4))
                  mouse_func(GLUT_MIDDLE_BUTTON, GLUT_DOWN, g_mouse_x, g_mouse_y);
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
               if (g_curwin->special) {
                  g_curwin->special(glut_key, g_mouse_x, g_mouse_y);
               }
               break;
            default:
               if (g_curwin->keyboard) {
                  g_curwin->keyboard(key & 0xFF, g_mouse_x, g_mouse_y);
               }
         }
      }

      if (idle && g_idle_func)
         g_idle_func();

      for (i = 0; i < MAX_SSHOT_CB; i++) {
         int time = glutGet(GLUT_ELAPSED_TIME);
         GLUTSShotCB *cb = &g_sscb[i];
         if (cb->func && (time >= cb->time)) {
            cb->func(cb->value);
            cb->func = NULL;
         }
      }
   }
}
