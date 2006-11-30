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

#include <GL/glut.h>

#include "internal.h"

void (*DisplayFunc)(void) = NULL;
void (*ReshapeFunc)(int width, int height) = NULL;
void (*KeyboardFunc)(unsigned char key, int x, int y) = NULL;
void (*KeyboardUpFunc)(unsigned char key, int x, int y) = NULL;
void (*MouseFunc)(int key, int state, int x, int y) = NULL;
void (*MotionFunc)(int x, int y) = NULL;
void (*PassiveMotionFunc)(int x, int y) = NULL;
void (*VisibilityFunc)(int state) = NULL;
void (*SpecialFunc)(int key, int x, int y) = NULL;
void (*SpecialUpFunc)(int key, int x, int y) = NULL;
void (*IdleFunc)(void) = NULL;
void (*MenuStatusFunc)(int state, int x, int y) = NULL;
void (*MenuStateFunc)(int state) = NULL;

void glutDisplayFunc(void (*func)(void))
{
   DisplayFunc = func;
}

void glutOverlayDisplayFunc(void (*func)(void))
{
}

void glutWindowStatusFunc(void (*func)(int state))
{
}

void glutReshapeFunc(void (*func)(int width, int height))
{
   ReshapeFunc = func;
}

void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y))
{
   KeyboardFunc = func;
}

void glutKeyboardUpFunc(void (*func)(unsigned char key, int x, int y))
{
   KeyboardUpFunc = func;
}

void glutMouseFunc(void (*func)(int button, int state, int x, int y))
{
   MouseFunc = func;
}

void glutMotionFunc(void (*func)(int x, int y))
{
   MotionFunc = func;
}

void glutPassiveMotionFunc(void (*func)(int x, int y))
{
   PassiveMotionFunc = func;
}

void glutJoystickFunc(void (*func)(unsigned int buttonMask,
				   int x, int y, int z), int pollInterval)
{
}

void glutVisibilityFunc(void (*func)(int state))
{
   VisibilityFunc = func;
}

void glutEntryFunc(void (*func)(int state))
{
}

void glutSpecialFunc(void (*func)(int key, int x, int y))
{
   SpecialFunc = func;
}

void glutSpecialUpFunc(void (*func)(int key, int x, int y))
{
   SpecialUpFunc = func;
}

void glutSpaceballMotionFunc(void (*func)(int x, int y, int z))
{
}

void glutSpaceballRotateFunc(void (*func)(int x, int y, int z))
{
}

void glutSpaceballButtonFunc(void (*func)(int button, int state))
{
}

void glutButtonBoxFunc(void (*func)(int button, int state))
{
}

void glutDialsFunc(void (*func)(int dial, int value))
{
}

void glutTabletMotionFunc(void (*func)(int x, int y))
{
}

void glutTabletButtonFunc(void (*func)(int button, int state,
				       int x, int y))
{
}

void glutMenuStatusFunc(void (*func)(int status, int x, int y))
{
   MenuStatusFunc = func;
}

void glutMenuStateFunc(void (*func)(int status))
{
   MenuStateFunc = func;
}

void glutIdleFunc(void (*func)(void))
{
   IdleFunc = func;
}

void glutTimerFunc(unsigned int msecs,
		   void (*func)(int value), int value)
{
   struct GlutTimer **head = &GlutTimers, *timer = malloc(sizeof *timer);
   timer->time = glutGet(GLUT_ELAPSED_TIME) + msecs;
   timer->func = func;
   timer->value = value;

   while(*head && (*head)->time < timer->time)
      head = &(*head)->next;

   timer->next = *head;
   *head = timer;
}
