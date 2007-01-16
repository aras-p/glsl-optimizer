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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <linux/fb.h>

#include <GL/glut.h>

#include "internal.h"

#define MENU_FONT_WIDTH   9
#define MENU_FONT_HEIGHT 15 
#define MENU_FONT        GLUT_BITMAP_9_BY_15
#define SUBMENU_OFFSET   20

struct GlutMenu *Menus;
int ActiveMenu;
int CurrentMenu;

static double MenuProjection[16];

static int AttachedMenus[3];
static int NumMenus = 1;
static int SelectedMenu;

void InitializeMenus(void)
{
   glPushAttrib(GL_TRANSFORM_BIT);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   gluOrtho2D(0.0, VarInfo.xres, VarInfo.yres, 0.0);
   glGetDoublev(GL_PROJECTION_MATRIX, MenuProjection);

   glPopMatrix();
   glPopAttrib();
}

void FreeMenus(void)
{
   int i, j;
	
   for(i = 1; i<NumMenus; i++) {
      for(j = 0; j<Menus[i].NumItems; j++)
	 free(Menus[i].Items[j].name);
      free(Menus[i].Items);
   }
   
   free(Menus);
}

int TryMenu(int button, int pressed)
{
   if(ActiveMenu && !pressed) {
      ActiveMenu = 0;
      CloseMenu();
      Redisplay = 1;
      return 1;
   }

   if(AttachedMenus[button] && pressed) {
      ActiveMenu = AttachedMenus[button];
      OpenMenu();
      Redisplay = 1;
      return 1;
   }
   return 0;
}

static int DrawMenu(int menu, int x, int *y)
{
   int i;
   int ret = 1;

   for(i=0; i < Menus[menu].NumItems; i++) {
      char *s = Menus[menu].Items[i].name;
      int a = 0;
      if(MouseY >= *y && MouseY < *y + MENU_FONT_HEIGHT &&
	 MouseX >= x && MouseX < x + Menus[menu].width) {
	 a = 1;
	 SelectedMenu = menu;
	 ret = 0;
	 Menus[menu].selected = i;
	 glColor3f(1,0,0);
      } else
	 glColor3f(1,1,1);

      *y += MENU_FONT_HEIGHT;
      glRasterPos2i(x, *y);
      for(; *s; s++)
	 glutBitmapCharacter(MENU_FONT, *s);

      if(Menus[menu].selected == i)
	 if(Menus[menu].Items[i].submenu) 
	    if(DrawMenu(Menus[menu].Items[i].submenu, x 
			+ SUBMENU_OFFSET, y)) {
	       if(!a)
		  Menus[menu].selected = -1;
	    } else
	       ret = 0;
   }
   return ret;
}

void DrawMenus(void)
{
   int x, y;

   if(GameMode)
      return;

   x = Menus[ActiveMenu].x;
   y = Menus[ActiveMenu].y;

   /* save old settings */
   glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT
		| GL_ENABLE_BIT | GL_VIEWPORT_BIT);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(MenuProjection);
   glViewport(0, 0, VarInfo.xres, VarInfo.yres);

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_ALPHA_TEST);
   glDisable(GL_LIGHTING);
   glDisable(GL_FOG);
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_COLOR_LOGIC_OP);
   glLogicOp(GL_AND_REVERSE);

   if(DrawMenu(ActiveMenu, x, &y))
      Menus[ActiveMenu].selected = -1;
    
   /* restore settings */
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glPopAttrib();
}

void OpenMenu(void)
{
   if(MenuStatusFunc)
      MenuStatusFunc(GLUT_MENU_IN_USE, MouseX, MouseY);
   if(MenuStateFunc)
      MenuStateFunc(GLUT_MENU_IN_USE);
   Menus[ActiveMenu].x = MouseX-Menus[ActiveMenu].width/2;

   if(Menus[ActiveMenu].x < 0)
      Menus[ActiveMenu].x = 0;
   if(Menus[ActiveMenu].x + Menus[ActiveMenu].width >= VarInfo.xres)
     Menus[ActiveMenu].x = VarInfo.xres - Menus[ActiveMenu].width - 1;

   Menus[ActiveMenu].y = MouseY-Menus[ActiveMenu].NumItems*MENU_FONT_HEIGHT/2;
   Menus[ActiveMenu].selected = -1;
}

void CloseMenu(void)
{
   if(MenuStatusFunc)
      MenuStatusFunc(GLUT_MENU_NOT_IN_USE, MouseX, MouseY);
   if(MenuStateFunc)
      MenuStateFunc(GLUT_MENU_NOT_IN_USE);
   if(SelectedMenu > 0) {
      int selected = Menus[SelectedMenu].selected;
      if(selected >= 0)
	 if(Menus[SelectedMenu].Items[selected].submenu == 0)
	    Menus[SelectedMenu].func(Menus[SelectedMenu].Items
				     [selected].value);
   }

}

/* glut menu functions */

int glutCreateMenu(void (*func)(int value))
{
   CurrentMenu = NumMenus;
   NumMenus++;
   Menus = realloc(Menus, sizeof(*Menus) * NumMenus);
   Menus[CurrentMenu].NumItems = 0;
   Menus[CurrentMenu].Items = NULL;
   Menus[CurrentMenu].func = func;
   Menus[CurrentMenu].width = 0;
   return CurrentMenu;
}

void glutSetMenu(int menu)
{
   CurrentMenu = menu;
}

int glutGetMenu(void)
{
   return CurrentMenu;
}

void glutDestroyMenu(int menu)
{
   if(menu == CurrentMenu)
      CurrentMenu = 0;
}

static void NameMenuEntry(int entry, const char *name)
{
   int cm = CurrentMenu;
   if(!(Menus[cm].Items[entry-1].name = realloc(Menus[cm].Items[entry-1].name,
						strlen(name) + 1))) {
      sprintf(exiterror, "realloc failed in NameMenuEntry\n");
      exit(0);
   }
   strcpy(Menus[cm].Items[entry-1].name, name);
   if(strlen(name) * MENU_FONT_WIDTH > Menus[cm].width)
      Menus[cm].width = strlen(name) * MENU_FONT_WIDTH;
}

static int AddMenuItem(const char *name)
{
   int cm = CurrentMenu;
   int item = Menus[cm].NumItems++;
   if(!(Menus[cm].Items = realloc(Menus[cm].Items,
				  Menus[cm].NumItems * sizeof(*Menus[0].Items)))) {
      sprintf(exiterror, "realloc failed in AddMenuItem\n");
      exit(0);
   }
   Menus[cm].Items[item].name = NULL;
   NameMenuEntry(item+1, name);
   return item;
}

void glutAddMenuEntry(const char *name, int value)
{
   int item = AddMenuItem(name);
   Menus[CurrentMenu].Items[item].value = value;
   Menus[CurrentMenu].Items[item].submenu = 0;
}

void glutAddSubMenu(const char *name, int menu)
{
   int item = AddMenuItem(name);
   if(menu == CurrentMenu) {
      sprintf(exiterror, "Recursive menus not supported\n");
      exit(0);
   }
   Menus[CurrentMenu].Items[item].submenu = menu;
}

void glutChangeToMenuEntry(int entry, const char *name, int value)
{
   NameMenuEntry(entry, name);
   Menus[CurrentMenu].Items[entry-1].value = value;
   Menus[CurrentMenu].Items[entry-1].submenu = 0;
}

void glutChangeToSubMenu(int entry, const char *name, int menu)
{
   NameMenuEntry(entry, name);
   Menus[CurrentMenu].Items[entry-1].submenu = menu;
}

void glutRemoveMenuItem(int entry)
{
   memmove(Menus[CurrentMenu].Items + entry - 1,
	   Menus[CurrentMenu].Items + entry,
	   sizeof(*Menus[0].Items) * (Menus[CurrentMenu].NumItems - entry));
   Menus[CurrentMenu].NumItems--;
}

void glutAttachMenu(int button)
{
   AttachedMenus[button] = CurrentMenu;
}

void glutDetachMenu(int button)
{
   AttachedMenus[button] = 0;
}
