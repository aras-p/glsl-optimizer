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
 * DOS/DJGPP glut driver v1.5 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <stdio.h>

#include "glutint.h"


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
         return g_curwin->xpos;
      case GLUT_WINDOW_Y:
         return g_curwin->ypos;
      case GLUT_WINDOW_WIDTH:
         return g_curwin->width;
      case GLUT_WINDOW_HEIGHT:
         return g_curwin->height;
      case GLUT_WINDOW_STENCIL_SIZE:
         return g_stencil;
      case GLUT_WINDOW_DEPTH_SIZE:
         return g_depth;
      case GLUT_WINDOW_RGBA:
         return !(g_display_mode & GLUT_INDEX);
      case GLUT_WINDOW_COLORMAP_SIZE:
         return (g_display_mode & GLUT_INDEX) ? (256 - RESERVED_COLORS) : 0;
      case GLUT_SCREEN_WIDTH:
         return g_screen_w;
      case GLUT_SCREEN_HEIGHT:
         return g_screen_h;
      case GLUT_INIT_WINDOW_X:
         return g_init_x;
      case GLUT_INIT_WINDOW_Y:
         return g_init_y;
      case GLUT_INIT_WINDOW_WIDTH:
         return g_init_w;
      case GLUT_INIT_WINDOW_HEIGHT:
         return g_init_h;
      case GLUT_INIT_DISPLAY_MODE:
         return g_display_mode;
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
         return (g_mouse != 0);
      case GLUT_NUM_MOUSE_BUTTONS:
         return g_mouse;
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


/* GAME MODE
 * Hack alert: incomplete... what is GameMode, anyway?
 */
GLint g_game;
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
         return game_possible && !g_curwin;
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
      g_bpp = game_bpp;
      g_refresh = game_refresh;

      glutInitWindowSize(game_width, game_height);

      if ((g_game = glutCreateWindow("<game>")) > 0) {
         game_active = GL_TRUE;
      }

      return g_game;
   } else {
      return 0;
   }
}


void GLUTAPIENTRY
glutLeaveGameMode (void)
{
   if (glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
      game_active = GL_FALSE;

      glutDestroyWindow(g_game);
   }
}
