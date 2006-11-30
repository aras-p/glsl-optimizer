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

#include <sys/time.h>
#include <linux/fb.h>
#include <GL/glfbdev.h>

#define MULTIHEAD   /* enable multihead hacks,
		       it allows the program to continue drawing
		       without reading input when a second fbdev
		       has keyboard focus it can cause
		       screen corruption that requires C-l to fix */
#define HAVE_GPM

#define MAX_VSYNC 200

/* this causes these symbols to not be exported */
#pragma GCC visibility push(hidden)


/* --------- fbdev ------------ */
extern int Redisplay;
extern int Visible;
extern int VisibleSwitch;
extern int Active;
extern int VisiblePoll;
extern int Swapping, VTSwitch;

void TestVisible(void);
int ParseFBModes(int, int, int, int, int, int);
void SetVideoMode(void);
void CreateBuffer(void);
void CreateVisual(void);

extern int FrameBufferFD;
extern unsigned char *FrameBuffer;
extern unsigned char *BackBuffer;
extern int DisplayMode;

extern char exiterror[256];

extern struct fb_fix_screeninfo FixedInfo;
extern struct fb_var_screeninfo VarInfo;

extern GLFBDevContextPtr Context;
extern GLFBDevBufferPtr Buffer;
extern GLFBDevVisualPtr Visual;

/* --- colormap --- */
#define REVERSECMAPSIZELOG 3
#define REVERSECMAPSIZE (1<<REVERSECMAPSIZELOG)

extern unsigned short RedColorMap[256],
                      GreenColorMap[256],
                      BlueColorMap[256];
extern unsigned char ReverseColorMap[REVERSECMAPSIZE]
                                    [REVERSECMAPSIZE]
                                    [REVERSECMAPSIZE];

void LoadColorMap(void);
void RestoreColorMap(void);

/* --- mouse --- */
extern int MouseX, MouseY;
extern int CurrentCursor;
extern int MouseVisible;
extern int LastMouseTime;
extern int NumMouseButtons;

void InitializeCursor(void);
void EraseCursor(void);
void DrawCursor(void);
void SwapCursor(void);

/* --- menus --- */
struct GlutMenu {
    int NumItems;
    int x, y;
    int width;
    int selected;
    struct {
	int value;
	int submenu;
	char *name;
    } *Items;
    void (*func)(int);
};

extern struct GlutMenu *Menus;

extern int ActiveMenu;
extern int CurrentMenu;

void InitializeMenus(void);
void FreeMenus(void);
void DrawMenus(void);

int TryMenu(int, int);
void OpenMenu(void);
void CloseMenu(void);

/* --- state --- */
extern int AccumSize, DepthSize, StencilSize, NumSamples;
extern struct timeval StartTime;
extern int KeyboardModifiers;

/* --- input --- */
#ifdef HAVE_GPM
extern int GpmMouse;
#endif

extern int CurrentVT;
extern int ConsoleFD;

extern double MouseSpeed;

extern int KeyRepeatMode;

void InitializeVT(int);
void RestoreVT(void);
void CloseMouse(void);
void InitializeMouse(void);

void ReceiveInput(void);

/* --- callback --- */
extern void (*DisplayFunc)(void);
extern void (*ReshapeFunc)(int width, int height);
extern void (*KeyboardFunc)(unsigned char key, int x, int y);
extern void (*KeyboardUpFunc)(unsigned char key, int x, int y);
extern void (*MouseFunc)(int key, int state, int x, int y);
extern void (*MotionFunc)(int x, int y);
extern void (*PassiveMotionFunc)(int x, int y);
extern void (*VisibilityFunc)(int state);
extern void (*SpecialFunc)(int key, int x, int y);
extern void (*SpecialUpFunc)(int key, int x, int y);
extern void (*IdleFunc)(void);
extern void (*MenuStatusFunc)(int state, int x, int y);
extern void (*MenuStateFunc)(int state);

/* --- timers --- */
struct GlutTimer {
    int time;
    void (*func)(int);
    int value;
    struct GlutTimer *next;
};

extern struct GlutTimer *GlutTimers;

/* ------- Game Mode -------- */
extern int GameMode;

#pragma GCC visibility pop
