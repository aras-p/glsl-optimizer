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
 * DOS/DJGPP glut driver v1.6 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */

 
#ifndef __glutint_h__
#define __glutint_h__

#include <GL/glut.h>

#include "GL/dmesa.h"


/* GLUT  function types */
typedef void (GLUTCALLBACK *GLUTdisplayCB) (void);
typedef void (GLUTCALLBACK *GLUTreshapeCB) (int, int);
typedef void (GLUTCALLBACK *GLUTkeyboardCB) (unsigned char, int, int);
typedef void (GLUTCALLBACK *GLUTmouseCB) (int, int, int, int);
typedef void (GLUTCALLBACK *GLUTmotionCB) (int, int);
typedef void (GLUTCALLBACK *GLUTpassiveCB) (int, int);
typedef void (GLUTCALLBACK *GLUTentryCB) (int);
typedef void (GLUTCALLBACK *GLUTvisibilityCB) (int);
typedef void (GLUTCALLBACK *GLUTwindowStatusCB) (int);
typedef void (GLUTCALLBACK *GLUTidleCB) (void);
typedef void (GLUTCALLBACK *GLUTtimerCB) (int);
typedef void (GLUTCALLBACK *GLUTmenuStateCB) (int);  /* DEPRECATED. */
typedef void (GLUTCALLBACK *GLUTmenuStatusCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTselectCB) (int);
typedef void (GLUTCALLBACK *GLUTspecialCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTspaceMotionCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTspaceRotateCB) (int, int, int);
typedef void (GLUTCALLBACK *GLUTspaceButtonCB) (int, int);
typedef void (GLUTCALLBACK *GLUTdialsCB) (int, int);
typedef void (GLUTCALLBACK *GLUTbuttonBoxCB) (int, int);
typedef void (GLUTCALLBACK *GLUTtabletMotionCB) (int, int);
typedef void (GLUTCALLBACK *GLUTtabletButtonCB) (int, int, int, int);
typedef void (GLUTCALLBACK *GLUTjoystickCB) (unsigned int, int, int, int);

typedef struct GLUTwindow {
   int num;                         /* window id */

   DMesaContext context;
   DMesaBuffer buffer;

   int show_mouse;
   GLboolean redisplay;

   /* GLUT settable or visible window state. */
   int xpos;
   int ypos;
   int width;                       /* window width in pixels */
   int height;                      /* window height in pixels */

   /* Per-window callbacks. */
   GLUTdisplayCB      display;      /* redraw */
   GLUTreshapeCB      reshape;      /* resize (width,height) */
   GLUTmouseCB        mouse;        /* mouse (button,state,x,y) */
   GLUTmotionCB       motion;       /* motion (x,y) */
   GLUTpassiveCB      passive;      /* passive motion (x,y) */
   GLUTentryCB        entry;        /* window entry/exit (state) */
   GLUTkeyboardCB     keyboard;     /* keyboard (ASCII,x,y) */
   GLUTkeyboardCB     keyboardUp;   /* keyboard up (ASCII,x,y) */
   GLUTwindowStatusCB windowStatus; /* window status */
   GLUTvisibilityCB   visibility;   /* visibility */
   GLUTspecialCB      special;      /* special key */
   GLUTspecialCB      specialUp;    /* special up key */
   GLUTbuttonBoxCB    buttonBox;    /* button box */
   GLUTdialsCB        dials;        /* dials */
   GLUTspaceMotionCB  spaceMotion;  /* Spaceball motion */
   GLUTspaceRotateCB  spaceRotate;  /* Spaceball rotate */
   GLUTspaceButtonCB  spaceButton;  /* Spaceball button */
   GLUTtabletMotionCB tabletMotion; /* tablet motion */
   GLUTtabletButtonCB tabletButton; /* tablet button */
   GLUTjoystickCB     joystick;     /* joystick */
} GLUTwindow;

extern GLUTidleCB g_idle_func;
extern GLUTmenuStatusCB g_menu_status_func;

extern GLuint g_bpp;                  /* HW: bits per pixel */
extern GLuint g_alpha;                /* HW: alpha bits */
extern GLuint g_depth;                /* HW: depth bits */
extern GLuint g_stencil;              /* HW: stencil bits */
extern GLuint g_accum;                /* HW: accum bits */
extern GLuint g_refresh;              /* HW: vertical refresh rate */
extern GLuint g_screen_w, g_screen_h; /* HW: physical screen size */
extern GLint g_driver_caps;

extern GLuint g_fps;

extern GLuint g_display_mode;         /* display bits */
extern int g_init_x, g_init_y;        /* initial window position */
extern GLuint g_init_w, g_init_h;     /* initial window size */

extern int g_mouse;                   /* non-zero if mouse installed */
extern int g_mouse_x, g_mouse_y;      /* mouse coords, relative to current win */

extern GLUTwindow *g_curwin;          /* current window */
extern GLUTwindow *g_windows[];

extern char *__glutProgramName;       /* program name */

extern void __glutInitMouse (void);

/* private routines from glut_util.c */
extern char * __glutStrdup(const char *string);
extern void __glutWarning(char *format,...);
extern void __glutFatalError(char *format,...);
extern void __glutFatalUsage(char *format,...);
/* Private routines from util.c */
#ifdef GLUT_IMPORT_LIB
extern void *__glutFont(void *font);
#endif


/* hmmm... */
#include "pc_hw/pc_hw.h"

typedef struct {
   void (*func) (int); /* function to call */
   int value;          /* value to pass to callback */
   int time;           /* end time */
} GLUTSShotCB;

extern GLUTSShotCB g_sscb[];

#define MAX_WINDOWS 2
#define MAX_SSHOT_CB 8
#define RESERVED_COLORS 0

#endif /* __glutint_h__ */
