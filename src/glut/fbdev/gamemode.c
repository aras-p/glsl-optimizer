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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/fb.h>

#include <GL/glut.h>

#include "internal.h"

int GameMode;

static int ModePossible, DispChanged;
static struct fb_var_screeninfo NormVarInfo, GameVarInfo;

static GLFBDevContextPtr GameContext;
static GLFBDevVisualPtr NormVisual;

/* storage for non-gamemode callbacks */
void (*KeyFuncs[2])(unsigned char key, int x, int y);
static void (*NormFuncs[8])();

static const char*SetOpers(const char *p, unsigned int *min, unsigned int *max)
{
   char *endptr;
   int comp = *p, val, neq = 0;

   if(p[1] == '=') {
      neq = 0;
      p++;
   }

   val = strtol(p+1, &endptr, 10);
   if(endptr == p+1)
      return p;

   switch(comp) {
   case '=':
      *min = *max = val;
      break;
   case '!':
      *min = val + 1;
      *max = val - 1;
      break;
   case '<':
      *max = val - neq;
      break;
   case '>':
      *min = val + neq;
      break;
   }
   return endptr;
}

void glutGameModeString(const char *string)
{
   const char *p = string;
   unsigned int minb = 15, maxb = 32;
   unsigned int minw = 0, maxw = -1;
   unsigned int minh, maxh = -1;
   unsigned int minf = 0, maxf = MAX_VSYNC;
   char *endptr;
   int count = -1, val;

   ModePossible = 0;

   if(DisplayMode & GLUT_INDEX)
      minb = maxb = 8;

 again:
   count++;
   if((val = strtol(p, &endptr, 10)) && *endptr=='x') {
      maxw = minw = val;
      p = endptr + 1;
      maxh = minh = strtol(p, &endptr, 10);
      p = endptr;
      goto again;
   }

   if(*p == ':') {
      minb = strtol(p+1, &endptr, 10);
      p = endptr;
      if(DisplayMode & GLUT_INDEX) {
	 if(minb != 8)
	    return;
      } else
	 if(minb != 15 && minb != 16 && minb != 24 && minb != 32)
	    return;
      maxb = minb;
      goto again;
   }

   if(*p == '@') {
      minf = strtol(p+1, &endptr, 10) - 5;
      maxf = minf + 10;
      p = endptr;
      goto again;
   }

   if(count == 0)
      while(*p) {
	 if(*p == ' ')
	    p++;
	 else
	 if(memcmp(p, "bpp", 3) == 0)
	    p = SetOpers(p+3, &minb, &maxb);
	 else
	 if(memcmp(p, "height", 6) == 0)
	    p = SetOpers(p+6, &minh, &maxh);
	 else
	 if(memcmp(p, "hertz", 5) == 0)
	    p = SetOpers(p+5, &minf, &maxf);
	 else
	 if(memcmp(p, "width", 5) == 0)
	    p = SetOpers(p+5, &minw, &maxw);
	 else
	 if(p = strchr(p, ' '))
	    p++;
	 else
	    break;
   }

   NormVarInfo = VarInfo;
   if(!ParseFBModes(minw, maxw, minh, maxh, minf, maxf))
      return;

   GameVarInfo = VarInfo;
   VarInfo = NormVarInfo;

   /* determine optimal bitdepth, make sure we have enough video memory */
   if(VarInfo.bits_per_pixel && VarInfo.bits_per_pixel <= maxb)
      GameVarInfo.bits_per_pixel = VarInfo.bits_per_pixel;
   else
      GameVarInfo.bits_per_pixel = maxb;

   while(FixedInfo.smem_len < GameVarInfo.xres * GameVarInfo.yres
	 * GameVarInfo.bits_per_pixel / 8) {
      if(GameVarInfo.bits_per_pixel < minb)
	 return;
      GameVarInfo.bits_per_pixel = ((GameVarInfo.bits_per_pixel+1)/8)*8-8;
   }
  
   ModePossible = 1;
}
   
int glutEnterGameMode(void)
{
   if(ActiveMenu)
      return 0;

   if(!ModePossible)
      return 0;

   if(GameMode) {
      if(!memcmp(&GameVarInfo, &VarInfo, sizeof VarInfo)) {
	 DispChanged = 0;
	 return 1;
      }
      glutLeaveGameMode();
   }

   if (ioctl(FrameBufferFD, FBIOPUT_VSCREENINFO, &GameVarInfo))
      return 0;

   NormVarInfo = VarInfo;
   VarInfo = GameVarInfo;

   NormVisual = Visual;
   SetVideoMode();
   CreateVisual();
   CreateBuffer();

   if(!(GameContext = glFBDevCreateContext(Visual, NULL))) {
      sprintf(exiterror, "Failure to create Context\n");
      exit(0);
   }

   if(!glFBDevMakeCurrent( GameContext, Buffer, Buffer )) {
      sprintf(exiterror, "Failure to Make Game Current\n");
      exit(0);
   }

   InitializeCursor();

   KeyFuncs[0] = KeyboardFunc;
   KeyFuncs[1] = KeyboardUpFunc;

   NormFuncs[0] = DisplayFunc;
   NormFuncs[1] = ReshapeFunc;
   NormFuncs[2] = MouseFunc;
   NormFuncs[3] = MotionFunc;
   NormFuncs[4] = PassiveMotionFunc;
   NormFuncs[5] = VisibilityFunc;
   NormFuncs[6] = SpecialFunc;
   NormFuncs[7] = SpecialUpFunc;

   DisplayFunc = NULL;
   ReshapeFunc = NULL;
   KeyboardFunc = NULL;
   KeyboardUpFunc = NULL;
   MouseFunc = NULL;
   MotionFunc = NULL;
   PassiveMotionFunc = NULL;
   VisibilityFunc = NULL;
   SpecialFunc = SpecialUpFunc = NULL;

   DispChanged = 1;
   GameMode = 1;
   Visible = 1;
   VisibleSwitch = 1;
   Redisplay = 1;
   return 1;
}

void glutLeaveGameMode(void)
{
   if(!GameMode)
      return;

   glFBDevDestroyContext(GameContext);
   glFBDevDestroyVisual(Visual);

   VarInfo = NormVarInfo;
   Visual = NormVisual;
   
   if(Visual) {
      SetVideoMode();
      CreateBuffer();
   
      if(!glFBDevMakeCurrent( Context, Buffer, Buffer )) {
	 sprintf(exiterror, "Failure to Make Current\n");
	 exit(0);
      }
      
      Redisplay = 1;
   }

   KeyboardFunc = KeyFuncs[0];
   KeyboardUpFunc = KeyFuncs[1];

   DisplayFunc = NormFuncs[0];
   ReshapeFunc = NormFuncs[1];
   MouseFunc = NormFuncs[2];
   MotionFunc = NormFuncs[3];
   PassiveMotionFunc = NormFuncs[4];
   VisibilityFunc = NormFuncs[5];
   SpecialFunc = NormFuncs[6];
   SpecialUpFunc = NormFuncs[7];

   GameMode = 0;
}

int glutGameModeGet(GLenum mode) {
   switch(mode) {
   case GLUT_GAME_MODE_ACTIVE:
      return GameMode;
   case GLUT_GAME_MODE_POSSIBLE:
      return ModePossible;
   case GLUT_GAME_MODE_DISPLAY_CHANGED:
      return DispChanged;
   }

   if(!ModePossible)
      return -1;
   
   switch(mode) {
   case GLUT_GAME_MODE_WIDTH:
      return GameVarInfo.xres;
   case GLUT_GAME_MODE_HEIGHT:
      return GameVarInfo.yres;
   case GLUT_GAME_MODE_PIXEL_DEPTH:
      return GameVarInfo.bits_per_pixel;
   case GLUT_GAME_MODE_REFRESH_RATE:
      return 1E12/GameVarInfo.pixclock
	 / (GameVarInfo.left_margin + GameVarInfo.xres
	   + GameVarInfo.right_margin + GameVarInfo.hsync_len)
	 / (GameVarInfo.upper_margin + GameVarInfo.yres 
	   + GameVarInfo.lower_margin + GameVarInfo.vsync_len);
   }
}
