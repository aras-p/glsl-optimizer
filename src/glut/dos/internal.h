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

 
#ifndef INTERNAL_H_included
#define INTERNAL_H_included

#include <GL/glut.h>

#include "GL/dmesa.h"


#define MAX_WINDOWS     2
#define MAX_TIMER_CB    8
#define RESERVED_COLORS 0


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

typedef void (GLUTCALLBACK *GLUTdestroyCB) (void);
typedef void (GLUTCALLBACK *GLUTmouseWheelCB) (int, int, int, int);
typedef void (GLUTCALLBACK *GLUTmenuDestroyCB) (void);


typedef struct {
   GLuint bpp, alpha;
   GLuint depth, stencil;
   GLuint accum;

   GLint geometry[2];
   GLuint refresh;

   GLint flags;
} GLUTvisual;

typedef struct {
   GLint x, y;
   GLint width, height;
   GLuint mode;
} GLUTdefault;

typedef struct {
   void (*func) (int);
   int value;
   int time;
} GLUTSShotCB;

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

   GLUTdestroyCB      destroy;      /* destroy */
   GLUTmouseWheelCB   mouseWheel;   /* mouse wheel */

   /* specific data */
   void *data;
} GLUTwindow;

typedef struct {
   int width, height;
   int xorig, yorig;
   int xmove;
   const unsigned char *bitmap;
} GLUTBitmapChar;

typedef struct {
   const char *name;
   int height;
   int num;
   const GLUTBitmapChar *const *table;
} GLUTBitmapFont;

typedef struct {
   const GLfloat x, y;
} GLUTStrokeVertex;

typedef struct {
   const unsigned num;
   const GLUTStrokeVertex *vertex;
} GLUTStrokeStrip;

typedef struct {
   const GLfloat right;
   const unsigned num;
   const GLUTStrokeStrip *strip;
} GLUTStrokeChar;

typedef struct {
   const char *name;
   const unsigned num;
   const GLUTStrokeChar *const *table;
   const GLfloat height;
   const GLfloat descent;
} GLUTStrokeFont;


extern char *__glutProgramName;

extern GLUTvisual _glut_visual;
extern GLUTdefault _glut_default;

extern GLuint _glut_fps;
extern GLUTidleCB _glut_idle_func;
extern GLUTmenuStatusCB _glut_menu_status_func;
extern GLUTSShotCB _glut_timer_cb[];

extern GLUTwindow *_glut_current, *_glut_windows[];

extern int _glut_mouse;                  /* number of buttons, if mouse installed */
extern int _glut_mouse_x, _glut_mouse_y; /* mouse coords, relative to current win */


extern void _glut_mouse_init (void);
extern void _glut_fatal(char *format,...);
extern void *_glut_font (void *font);


#include "pc_hw/pc_hw.h"

#endif
