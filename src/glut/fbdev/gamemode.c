/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * Copyright (C) 1995-2006  Brian Paul
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
 * Library for glut using mesa fbdev driver
 *
 * Written by Sean D'Epagnier (c) 2006
 */

/* NOTICE: game mode will not be fully implemented until
   glutReshapeWindow is fully implemented */

#include <stdlib.h>

#include <linux/fb.h>

#include <GL/glut.h>

#include "internal.h"

void glutGameModeString(const char *string)
{
   
}

int glutEnterGameMode(void)
{
   if(ActiveMenu)
      return 0;
   return 1;
}

void glutLeaveGameMode(void)
{
}

int glutGameModeGet(GLenum mode) {
   switch(mode) {
   case GLUT_GAME_MODE_ACTIVE:
      return 1;
   case GLUT_GAME_MODE_POSSIBLE:
      return 1;
   case GLUT_GAME_MODE_WIDTH:
      return VarInfo.xres;
   case GLUT_GAME_MODE_HEIGHT:
      return VarInfo.yres;
   case GLUT_GAME_MODE_PIXEL_DEPTH:
      return VarInfo.bits_per_pixel;
   case GLUT_GAME_MODE_REFRESH_RATE:
      if(VarInfo.pixclock) {
	 int htotal = VarInfo.left_margin + VarInfo.xres
	    + VarInfo.right_margin + VarInfo.hsync_len;
	 int vtotal = VarInfo.upper_margin + VarInfo.yres
	    + VarInfo.lower_margin + VarInfo.vsync_len;
	 return 1E12/VarInfo.pixclock/htotal/vtotal;
      }
      return 0;
   case GLUT_GAME_MODE_DISPLAY_CHANGED:
      return 0;
   }
}
