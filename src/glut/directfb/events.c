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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "internal.h"


int GLUTAPIENTRY
glutGetModifiers( void )
{
     if (g_current)
          return g_current->modifiers;
     return 0;
}


void GLUTAPIENTRY 
glutIgnoreKeyRepeat( int ignore )
{
}


void GLUTAPIENTRY
glutSetKeyRepeat( int mode )
{
}


void GLUTAPIENTRY
glutForceJoystickFunc( void )
{
     if (g_game && joystick && joystick_func) {
          joystick_func( g_game->buttons, 
                         g_game->cx, g_game->cy, g_game->cz );
     }
}


static int 
__glutSpecialKey( DFBInputDeviceKeySymbol key )
{
     switch (key) {
          case DIKS_F1:
               return GLUT_KEY_F1;
          case DIKS_F2:
               return GLUT_KEY_F2;
          case DIKS_F3:
               return GLUT_KEY_F3;
          case DIKS_F4:
               return GLUT_KEY_F4;
          case DIKS_F5:
               return GLUT_KEY_F5;
          case DIKS_F6:
               return GLUT_KEY_F6;
          case DIKS_F7:
               return GLUT_KEY_F7;
          case DIKS_F8:
               return GLUT_KEY_F8;
          case DIKS_F9:
               return GLUT_KEY_F9;
          case DIKS_F10:
               return GLUT_KEY_F10;
          case DIKS_F11:
               return GLUT_KEY_F11;
          case DIKS_F12:
               return GLUT_KEY_F12;
          case DIKS_CURSOR_LEFT:
               return GLUT_KEY_LEFT;
          case DIKS_CURSOR_UP:
               return GLUT_KEY_UP;
          case DIKS_CURSOR_RIGHT:
               return GLUT_KEY_RIGHT;
          case DIKS_CURSOR_DOWN:
               return GLUT_KEY_DOWN;
          case DIKS_PAGE_UP:
               return GLUT_KEY_PAGE_UP;
          case DIKS_PAGE_DOWN:
               return GLUT_KEY_PAGE_DOWN;
          case DIKS_HOME:
               return GLUT_KEY_HOME;
          case DIKS_END:
               return GLUT_KEY_END;
          case DIKS_INSERT:
               return GLUT_KEY_INSERT;
          default:
               break;
     }
     
     return 0;
}


static int 
__glutButton( DFBInputDeviceButtonIdentifier button )
{
     switch (button) {
          case DIBI_LEFT:
               return GLUT_LEFT_BUTTON;
          case DIBI_MIDDLE:
               return GLUT_MIDDLE_BUTTON;
          case DIBI_RIGHT:
               return GLUT_RIGHT_BUTTON;
          default:
               break;
     }
     
     return 0;
}


static int 
__glutModifiers( DFBInputDeviceModifierMask mask )
{
     return ((mask & DIMM_SHIFT) ? GLUT_ACTIVE_SHIFT : 0)  |
            ((mask & DIMM_CONTROL) ? GLUT_ACTIVE_CTRL : 0) |
            ((mask & DIMM_ALT) ? GLUT_ACTIVE_ALT : 0);
}


static void 
__glutWindowEvent( DFBWindowEvent *e )
{
     __GlutWindow *window;
     
     window = __glutFindWindow( e->window_id );
     if (!window) /* window was destroyed */
          return;
     
     switch (e->type) {
          case DWET_KEYDOWN:
               window->modifiers = __glutModifiers( e->modifiers );
               if (DFB_KEY_IS_ASCII( e->key_symbol )) {
                    if (keyboard_func) {
                         __glutSetWindow( window );
                         keyboard_func( e->key_symbol, e->x, e->y );
                    }
               }
               else {
                    int key = __glutSpecialKey( e->key_symbol );
                    if (key && special_func) {
                         __glutSetWindow( window );
                         special_func( key, e->x, e->y );
                    }
               }
               break;
          case DWET_KEYUP:
               window->modifiers = __glutModifiers( e->modifiers );
               if (DFB_KEY_IS_ASCII( e->key_symbol )) {
                    if (keyboard_up_func) {
                         __glutSetWindow( window );
                         keyboard_up_func( e->key_symbol, e->x, e->y );
                    }
               }
               else {
                    int key = __glutSpecialKey( e->key_symbol );
                    if (key && special_up_func) {
                         __glutSetWindow( window );
                         special_up_func( key, e->x, e->y );
                    }
               }
               break;
          case DWET_BUTTONDOWN:
               if (mouse_func) {
                    __glutSetWindow( window );
                    mouse_func( __glutButton( e->button ), GLUT_DOWN, e->x, e->y );
               }
               break;
          case DWET_BUTTONUP:
               if (mouse_func) {
                    __glutSetWindow( window );
                    mouse_func( __glutButton( e->button ), GLUT_UP, e->x, e->y );
               }
               break;
          case DWET_MOTION:
               if (e->buttons) {
                    if (motion_func) {
                         __glutSetWindow( window );
                         motion_func( e->cx, e->cy );
                    }
               }
               else {
                    if (passive_motion_func) {
                         __glutSetWindow( window );
                         passive_motion_func( e->cx, e->cy );
                    }
               }
               break;
          case DWET_ENTER:
               if (entry_func) {
                    __glutSetWindow( window );
                    entry_func( GLUT_ENTERED );
               }
               break;
          case DWET_LEAVE:
               if (entry_func) {
                    __glutSetWindow( window );
                    entry_func( GLUT_LEFT );
               }
               break;
          case DWET_SIZE:
               window->reshape = GL_TRUE;
               window->redisplay = GL_TRUE;
               break;
          default:
               break;
     }
}


static void 
__glutInputEvent( DFBInputEvent *e )
{
     __glutAssert( g_game != NULL );
     
     switch (e->type) {
          case DIET_KEYPRESS:
               g_game->modifiers = __glutModifiers( e->modifiers ); 
               if (DFB_KEY_IS_ASCII( e->key_symbol )) {
                    if (keyboard_func) {
                         __glutSetWindow( g_game );
                         keyboard_func( e->key_symbol, g_game->cx, g_game->cy );
                    }
               }
               else {
                    int key = __glutSpecialKey( e->key_symbol );
                    if (key && special_func) {
                         __glutSetWindow( g_game );
                         special_func( key, g_game->cx, g_game->cy );
                    }
               }
               break;
          case DIET_KEYRELEASE:
               g_game->modifiers = __glutModifiers( e->modifiers ); 
               if (DFB_KEY_IS_ASCII( e->key_symbol )) {
                    if (keyboard_up_func) {
                         __glutSetWindow( g_game );
                         keyboard_up_func( e->key_symbol, g_game->cx, g_game->cy );
                    }
               }
               else {
                    int key = __glutSpecialKey( e->key_symbol );
                    if (key && special_up_func) {
                         __glutSetWindow( g_game );
                         special_up_func( key, g_game->cx, g_game->cy );
                    }
               }
               break;
          case DIET_BUTTONPRESS:
               if (e->device_id == DIDID_JOYSTICK) {
                    g_game->buttons = e->buttons;
                    if (joystick_func) {
                         __glutSetWindow( g_game );
                         joystick_func( g_game->buttons,
                                        g_game->cx, g_game->cy, g_game->cz );
                    }
               }
               else {
                    if (mouse_func) {
                         __glutSetWindow( g_game );
                         mouse_func( __glutButton( e->button ), 
                                     GLUT_DOWN, g_game->cx, g_game->cy );
                    }
               }
               break;
          case DIET_BUTTONRELEASE:
               if (e->device_id == DIDID_JOYSTICK) {
                    g_game->buttons = e->buttons;
                    if (joystick_func) {
                         __glutSetWindow( g_game );
                         joystick_func( g_game->buttons,
                                        g_game->cx, g_game->cy, g_game->cz );
                    }
               }
               else {
                    if (mouse_func) {
                         __glutSetWindow( g_game );
                         mouse_func( __glutButton( e->button ), 
                                     GLUT_UP, g_game->cx, g_game->cy );
                    }
               }
               break;
          case DIET_AXISMOTION:
               switch (e->axis) {
                    case DIAI_X:
                         if (e->flags & DIEF_AXISABS)
                              g_game->cx = e->axisabs;
                         else if (e->flags & DIEF_AXISREL)
                              g_game->cx += e->axisrel;
                         break;
                    case DIAI_Y:
                         if (e->flags & DIEF_AXISABS)
                              g_game->cy = e->axisabs;
                         else if (e->flags & DIEF_AXISREL)
                              g_game->cy += e->axisrel;
                         break;
                    case DIAI_Z:
                         if (e->flags & DIEF_AXISABS)
                              g_game->cz = e->axisabs;
                         else if (e->flags & DIEF_AXISREL)
                              g_game->cz += e->axisrel;
                         break;
                    default:
                         return;
               }
               if (e->device_id == DIDID_JOYSTICK) {
                    if (joystick_func) {
                         __glutSetWindow( g_game );
                         joystick_func( g_game->buttons,
                                        g_game->cx, g_game->cy, g_game->cz );
                    }
               }
               else if (e->axis != DIAI_Z) {                    
                    if (e->buttons && motion_func) {
                         __glutSetWindow( g_game );
                         motion_func( g_game->cx, g_game->cy );
                    }
                    else if (!e->buttons && passive_motion_func) {
                         __glutSetWindow( g_game );
                         passive_motion_func( g_game->cx, g_game->cy );
                    }
               }
               break;
          default:
               break;
     }
}


void GLUTAPIENTRY 
glutMainLoop( void )
{
     __glutAssert( events != NULL );
     
     while (GL_TRUE) {
          DFBEvent evt;
          
          g_idle = GL_TRUE;
          
          __glutHandleTimers();
          __glutHandleWindows();
          
          while (events->GetEvent( events, &evt ) == DFB_OK) {
               g_idle = GL_FALSE;
               
               if (evt.clazz == DFEC_WINDOW)
                    __glutWindowEvent( &evt.window );
               else
                    __glutInputEvent( &evt.input );
                         
               __glutHandleTimers();
          }
          
          if (g_idle) {
               if (idle_func) {
                    idle_func();
               }
               else {
                    __glutSetWindow( NULL );
                    usleep( 500 );
               }
          }
     }
}

