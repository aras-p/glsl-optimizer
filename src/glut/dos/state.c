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


#include <stdio.h>

#include "internal.h"


#define FREQUENCY 100 /* set this to zero to use the default timer */


static int timer_installed;
#if FREQUENCY
static volatile int ticks;


static void
ticks_timer (void *p)
{
   (void)p;
   ticks++;
} ENDOFUNC(ticks_timer)
#else
#include <time.h>

static struct timeval then;
#endif


int APIENTRY
glutGet (GLenum type)
{
   switch (type) {
      case GLUT_WINDOW_X:
         return _glut_current->xpos;
      case GLUT_WINDOW_Y:
         return _glut_current->ypos;
      case GLUT_WINDOW_WIDTH:
         return _glut_current->width;
      case GLUT_WINDOW_HEIGHT:
         return _glut_current->height;
      case GLUT_WINDOW_STENCIL_SIZE:
         return _glut_visual.stencil;
      case GLUT_WINDOW_DEPTH_SIZE:
         return _glut_visual.depth;
      case GLUT_WINDOW_RGBA:
         return !(_glut_default.mode & GLUT_INDEX);
      case GLUT_WINDOW_COLORMAP_SIZE:
         return (_glut_default.mode & GLUT_INDEX) ? (256 - RESERVED_COLORS) : 0;
      case GLUT_SCREEN_WIDTH:
         return _glut_visual.geometry[0];
      case GLUT_SCREEN_HEIGHT:
         return _glut_visual.geometry[1];
      case GLUT_INIT_WINDOW_X:
         return _glut_default.x;
      case GLUT_INIT_WINDOW_Y:
         return _glut_default.y;
      case GLUT_INIT_WINDOW_WIDTH:
         return _glut_default.width;
      case GLUT_INIT_WINDOW_HEIGHT:
         return _glut_default.height;
      case GLUT_INIT_DISPLAY_MODE:
         return _glut_default.mode;
      case GLUT_ELAPSED_TIME:
#if FREQUENCY
         if (!timer_installed) {
            timer_installed = GL_TRUE;
            LOCKDATA(ticks);
            LOCKFUNC(ticks_timer);
            pc_install_int(ticks_timer, NULL, FREQUENCY);
         }
         return ticks * 1000 / FREQUENCY;
#else
         if (!timer_installed) {
            timer_installed = GL_TRUE;
            gettimeofday(&then, NULL);
            return 0;
         } else {
            struct timeval now;
            gettimeofday(&now, NULL);
            return (now.tv_usec - then.tv_usec) / 1000 +
                   (now.tv_sec  - then.tv_sec)  * 1000;
         }
#endif
      default:
         return -1;
   }
}


int APIENTRY
glutDeviceGet (GLenum type)
{
   switch (type) {
      case GLUT_HAS_KEYBOARD:
         return GL_TRUE;
      case GLUT_HAS_MOUSE:
         return (_glut_mouse != 0);
      case GLUT_NUM_MOUSE_BUTTONS:
         return _glut_mouse;
      case GLUT_HAS_SPACEBALL:
      case GLUT_HAS_DIAL_AND_BUTTON_BOX:
      case GLUT_HAS_TABLET:
         return GL_FALSE;
      case GLUT_NUM_SPACEBALL_BUTTONS:
      case GLUT_NUM_BUTTON_BOX_BUTTONS:
      case GLUT_NUM_DIALS:
      case GLUT_NUM_TABLET_BUTTONS:
         return 0;
      default:
         return -1;
   }
}


int APIENTRY
glutGetModifiers (void)
{
   int mod = 0;
   int shifts = pc_keyshifts();

   if (shifts & (KB_SHIFT_FLAG | KB_CAPSLOCK_FLAG)) {
      mod |= GLUT_ACTIVE_SHIFT;
   }

   if (shifts & KB_ALT_FLAG) {
      mod |= GLUT_ACTIVE_ALT;
   }

   if (shifts & KB_CTRL_FLAG) {
      mod |= GLUT_ACTIVE_CTRL;
   }

   return mod;
}


void APIENTRY
glutReportErrors (void)
{
   /* reports all the OpenGL errors that happened till now */
}


/* GAME MODE
 * Hack alert: incomplete... what is GameMode, anyway?
 */
static GLint game;
static GLboolean game_possible;
static GLboolean game_active;
static GLuint game_width;
static GLuint game_height;
static GLuint game_bpp;
static GLuint game_refresh;


void APIENTRY
glutGameModeString (const char *string)
{
   if (sscanf(string, "%ux%u:%u@%u", &game_width, &game_height, &game_bpp, &game_refresh) == 4) {
      game_possible = GL_TRUE;
   }
}


int APIENTRY
glutGameModeGet (GLenum mode)
{
   switch (mode) {
      case GLUT_GAME_MODE_ACTIVE:
         return game_active;
      case GLUT_GAME_MODE_POSSIBLE:
         return game_possible && !_glut_current;
      case GLUT_GAME_MODE_WIDTH:
         return game_active ? (int)game_width : -1;
      case GLUT_GAME_MODE_HEIGHT:
         return game_active ? (int)game_height : -1;
      case GLUT_GAME_MODE_PIXEL_DEPTH:
         return game_active ? (int)game_bpp : -1;
      case GLUT_GAME_MODE_REFRESH_RATE:
         return game_active ? (int)game_refresh : -1;
      default:
         return -1;
   }
}


int APIENTRY
glutEnterGameMode (void)
{
   if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
      _glut_visual.bpp = game_bpp;
      _glut_visual.refresh = game_refresh;

      glutInitWindowSize(game_width, game_height);

      if ((game = glutCreateWindow("<game>")) > 0) {
         game_active = GL_TRUE;
      }

      return game;
   } else {
      return 0;
   }
}


void GLUTAPIENTRY
glutLeaveGameMode (void)
{
   if (glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
      game_active = GL_FALSE;

      glutDestroyWindow(game);
   }
}
