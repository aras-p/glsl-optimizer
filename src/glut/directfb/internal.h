/*
 * Copyright (C) 2006 Claudio Ciccani <klan@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __GLUT_INTERNAL_H__
#define __GLUT_INTERNAL_H__

#include <stdio.h>
#include <stdlib.h>

#include <directfb.h>
#include <directfb_version.h>

#include <directfbgl.h>

#include "GL/glut.h"


#define VERSION_CODE( M, m, r )  (((M) << 16) | ((m) << 8) | ((r)))

#define DIRECTFB_VERSION_CODE    VERSION_CODE( DIRECTFB_MAJOR_VERSION, \
                                               DIRECTFB_MINOR_VERSION, \
                                               DIRECTFB_MICRO_VERSION )


#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480

/*
 * Window request flags
 */
#define WINDOW_REQUEST_POSITION   0x00000001
#define WINDOW_REQUEST_RESIZE     0x00000002
#define WINDOW_REQUEST_RESTACK    0x00000004
#define WINDOW_REQUEST_SHOW       0x00000008
#define WINDOW_REQUEST_HIDE       0x00000010
#define WINDOW_REQUEST_DESTROY    0x00000020

/*
 * GLUT Window implementation
 */
typedef struct __GlutWindow_s {
     int                    id;
     DFBWindowID            wid;

     IDirectFBWindow       *window; /* NULL = fullscreen (game mode) */
     IDirectFBSurface      *surface;
     IDirectFBGL           *gl;

     /* display mode */
     GLenum                 mode;

     /* cursor position in fullscreen mode */
     int                    cx;
     int                    cy;
     /* joystick position */
     int                    jx;
     int                    jy;
     int                    jz;
     /* pressed modifiers */
     int                    modifiers;
     /* pressed buttons */
     int                    buttons;
     /* current cursor shape */
     int                    cursor;
     
     struct {
          int               flags;
          int               x;
          int               y;
          int               w;
          int               h;
          int               z;
     } req;

     GLboolean              visible;    
     GLboolean              redisplay;
     GLboolean              reshape;
     GLboolean              visibility;

     struct __GlutWindow_s *next;
     struct __GlutWindow_s *prev;
} __GlutWindow;


/* Global Vars */
extern IDirectFB             *dfb;
extern IDirectFBDisplayLayer *primary;
extern IDirectFBEventBuffer  *events;
extern IDirectFBInputDevice  *keyboard;
extern IDirectFBInputDevice  *mouse;
extern IDirectFBInputDevice  *joystick;

extern GLenum                 g_display_mode;
extern GLuint                 g_width;
extern GLuint                 g_height;
extern GLint                  g_xpos;
extern GLint                  g_ypos;
extern GLint                  g_bpp;
extern GLboolean              g_idle;
extern __GlutWindow          *g_current;
extern __GlutWindow          *g_game;


/* Global Funcs */
/* window.c */
extern __GlutWindow* __glutCreateWindow( GLboolean fullscreen );
extern __GlutWindow* __glutFindWindow( DFBWindowID id );
extern void          __glutSetWindow( __GlutWindow *window );
extern void          __glutHandleWindows( void );
extern void          __glutDestroyWindow( __GlutWindow *window );
extern void          __glutDestroyWindows( void );
/* callback.c */
extern void          __glutHandleTimers( void );
extern GLboolean     __glutGetTimeout( int *ret_msec );
extern void          __glutFreeTimers( void );


/* Global Callbacks */
extern void (GLUTCALLBACK *display_func) (void);
extern void (GLUTCALLBACK *reshape_func) (int width, int height);
extern void (GLUTCALLBACK *keyboard_func) (unsigned char key, int x, int y);
extern void (GLUTCALLBACK *mouse_func) (int button, int state, int x, int y);
extern void (GLUTCALLBACK *motion_func) (int x, int y);
extern void (GLUTCALLBACK *passive_motion_func) (int x, int y);
extern void (GLUTCALLBACK *entry_func) (int state);
extern void (GLUTCALLBACK *visibility_func) (int state);
extern void (GLUTCALLBACK *idle_func) (void);
extern void (GLUTCALLBACK *menu_state_func) (int state);
extern void (GLUTCALLBACK *special_func) (int key, int x, int y);
extern void (GLUTCALLBACK *spaceball_motion_func) (int x, int y, int z);
extern void (GLUTCALLBACK *spaceball_rotate_func) (int x, int y, int z);
extern void (GLUTCALLBACK *spaceball_button_func) (int button, int state);
extern void (GLUTCALLBACK *button_box_func) (int button, int state);
extern void (GLUTCALLBACK *dials_func) (int dial, int value);
extern void (GLUTCALLBACK *tablet_motion_func) (int x, int y);
extern void (GLUTCALLBACK *tabled_button_func) (int button, int state, int x, int y);
extern void (GLUTCALLBACK *menu_status_func) (int status, int x, int y);
extern void (GLUTCALLBACK *overlay_display_func) (void);
extern void (GLUTCALLBACK *window_status_func) (int state);
extern void (GLUTCALLBACK *keyboard_up_func) (unsigned char key, int x, int y);
extern void (GLUTCALLBACK *special_up_func) (int key, int x, int y);
extern void (GLUTCALLBACK *joystick_func) (unsigned int buttons, int x, int y, int z);


#ifdef DEBUG
# define __glutAssert( exp ) {\
     if (!(exp)) {\
          fprintf( stderr, "(!!) *** Assertion [%s] failed in %s() ***\n",\
                           #exp, __FUNCTION__ );
          fflush( stderr );\
          exit( -1 );\
     }\
 }
#else
# define __glutAssert( exp )
#endif

#define __glutWarning( format, ... ) {\
     fprintf( stderr, "(!) GLUT: " format "!\n", ## __VA_ARGS__ );\
     fflush( stderr );\
}

#define __glutFatalError( format, ... ) {\
     fprintf( stderr, "(!) GLUT: " format "!\n", ## __VA_ARGS__ );\
     fprintf( stderr, "\t-> from %s() at line %d\n", __FUNCTION__, __LINE__ );\
     fflush( stderr );\
     exit( -1 );\
}


#endif /* __GLUT_INTERNAL_H__ */

