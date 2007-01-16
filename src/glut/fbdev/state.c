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

#include <string.h>
#include <sys/time.h>

#include <linux/fb.h>

#include <GL/glut.h>

#include "../../mesa/main/config.h"

#include "internal.h"

int AccumSize = 16; /* per channel size of accumulation buffer */
int DepthSize = DEFAULT_SOFTWARE_DEPTH_BITS;
int StencilSize = STENCIL_BITS;
int NumSamples = 4;

int glutGet(GLenum state)
{
   switch(state) {
   case GLUT_WINDOW_X:
      return 0;
   case GLUT_WINDOW_Y:
      return 0;
   case GLUT_INIT_WINDOW_WIDTH:
   case GLUT_WINDOW_WIDTH:
   case GLUT_SCREEN_WIDTH:
      return VarInfo.xres;
   case GLUT_INIT_WINDOW_HEIGHT:
   case GLUT_WINDOW_HEIGHT:
   case GLUT_SCREEN_HEIGHT:
      return VarInfo.yres;
   case GLUT_WINDOW_BUFFER_SIZE:
      return VarInfo.bits_per_pixel;
   case GLUT_WINDOW_STENCIL_SIZE:
      return StencilSize;
   case GLUT_WINDOW_DEPTH_SIZE:
      return DepthSize;
   case GLUT_WINDOW_RED_SIZE:
      return VarInfo.red.length;
   case GLUT_WINDOW_GREEN_SIZE:
      return VarInfo.green.length;
   case GLUT_WINDOW_BLUE_SIZE:
      return VarInfo.green.length;
   case GLUT_WINDOW_ALPHA_SIZE:
      return VarInfo.transp.length;
   case GLUT_WINDOW_ACCUM_RED_SIZE:
   case GLUT_WINDOW_ACCUM_GREEN_SIZE:
   case GLUT_WINDOW_ACCUM_BLUE_SIZE:
   case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
      return AccumSize;
   case GLUT_WINDOW_DOUBLEBUFFER:
      if(DisplayMode & GLUT_DOUBLE)
	 return 1;
      return 0;
   case GLUT_WINDOW_RGBA:
      if(DisplayMode & GLUT_INDEX)
	 return 0;
      return 1;
   case GLUT_WINDOW_PARENT:
      return 0;
   case GLUT_WINDOW_NUM_CHILDREN:
      return 0;
   case GLUT_WINDOW_COLORMAP_SIZE:
      if(DisplayMode & GLUT_INDEX)
	 return 256;
      return 0;
   case GLUT_WINDOW_NUM_SAMPLES:
      return NumSamples;
   case GLUT_WINDOW_STEREO:
      return 0;
   case GLUT_WINDOW_CURSOR:
      return CurrentCursor;
   case GLUT_SCREEN_WIDTH_MM:
      return VarInfo.width;
   case GLUT_SCREEN_HEIGHT_MM:
      return VarInfo.height;
   case GLUT_MENU_NUM_ITEMS:
      if(CurrentMenu)
	 return Menus[CurrentMenu].NumItems;
      return 0;
   case GLUT_DISPLAY_MODE_POSSIBLE:
      if((DisplayMode & GLUT_MULTISAMPLE)
	 || (DisplayMode & GLUT_STEREO)
	 || (DisplayMode & GLUT_LUMINANCE)
	 || (DisplayMode & GLUT_ALPHA) && (DisplayMode & GLUT_INDEX))
	 return 0;
      return 1;
   case GLUT_INIT_DISPLAY_MODE:
      return DisplayMode;
   case GLUT_INIT_WINDOW_X:
   case GLUT_INIT_WINDOW_Y:
      return 0;
   case GLUT_ELAPSED_TIME:
      {
	 static struct timeval tv;
	 gettimeofday(&tv, 0);
	 return 1000 * (tv.tv_sec - StartTime.tv_sec)
	    + (tv.tv_usec - StartTime.tv_usec) / 1000;
      }
   }
   return -1;
}

int glutLayerGet(GLenum info)
{
   switch(info) {
   case GLUT_OVERLAY_POSSIBLE:
      return 0;
   case GLUT_LAYER_IN_USE:
      return GLUT_NORMAL;
   case GLUT_HAS_OVERLAY:
      return 0;
   case GLUT_TRANSPARENT_INDEX:
      return -1;
   case GLUT_NORMAL_DAMAGED:
      return Redisplay;
   case GLUT_OVERLAY_DAMAGED:
      return -1;
   }
   return -1;
}

int glutDeviceGet(GLenum info)
{
   switch(info) {
   case GLUT_HAS_KEYBOARD:
      return ConsoleFD != -1 ? 1 : 0;
   case GLUT_HAS_MOUSE:
   case GLUT_NUM_MOUSE_BUTTONS:
      return NumMouseButtons;
   case GLUT_HAS_SPACEBALL:
   case GLUT_HAS_DIAL_AND_BUTTON_BOX:
   case GLUT_HAS_TABLET:
      return 0;
   case GLUT_NUM_SPACEBALL_BUTTONS:
   case GLUT_NUM_BUTTON_BOX_BUTTONS:
   case GLUT_NUM_DIALS:
   case GLUT_NUM_TABLET_BUTTONS:
      return 0;
   case GLUT_DEVICE_IGNORE_KEY_REPEAT:
      return KeyRepeatMode == GLUT_KEY_REPEAT_OFF;
   case GLUT_DEVICE_KEY_REPEAT:
      return KeyRepeatMode;
   case GLUT_JOYSTICK_POLL_RATE:
   case GLUT_HAS_JOYSTICK:
   case GLUT_JOYSTICK_BUTTONS:
   case GLUT_JOYSTICK_AXES:
      return 0;
   }
   return -1;
}

int glutGetModifiers(void){
   return KeyboardModifiers;
}

int glutExtensionSupported(const char *extension)
{
   const char *exts = (const char *) glGetString(GL_EXTENSIONS);
   const char *start = exts;
   int len = strlen(extension);
   for(;;) {
      const char *p = strstr(exts, extension);
      if(!p)
	 break;
      if((p == start || p[-1] == ' ') && (p[len] == ' ' || p[len] == 0))
	 return 1;
      exts = p + len;
   }
   return 0;
}
