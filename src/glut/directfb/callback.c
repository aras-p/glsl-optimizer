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
#include <string.h>
#include <sys/time.h>

#include "internal.h"


typedef void (GLUTCALLBACK *__GlutTimerCallback) ( int value );

typedef struct __GlutTimer_s {
     struct timeval        interval;
     struct timeval        expire;
     
     __GlutTimerCallback   func;
     int                   value;
     
     struct __GlutTimer_s *next;
} __GlutTimer;

/*****************************************************************************/

static __GlutTimer *g_timers = NULL;

/*****************************************************************************/


void GLUTAPIENTRY 
glutDisplayFunc( void (GLUTCALLBACK *func) (void) )
{
     display_func = func;
}


void GLUTAPIENTRY 
glutReshapeFunc( void (GLUTCALLBACK *func) (int width, int height) )
{
     reshape_func = func;
}


void GLUTAPIENTRY
glutKeyboardFunc( void (GLUTCALLBACK *func) (unsigned char key, int x, int y) )
{
     keyboard_func = func;
}


void GLUTAPIENTRY
glutMouseFunc( void (GLUTCALLBACK *func) (int button, int state, int x, int y) )
{
     mouse_func = func;
}


void GLUTAPIENTRY
glutMotionFunc( void (GLUTCALLBACK *func) (int x, int y) )
{
     motion_func = func;
}


void GLUTAPIENTRY
glutPassiveMotionFunc( void (GLUTCALLBACK *func) (int x, int y) )
{
     passive_motion_func = func;
}


void GLUTAPIENTRY
glutEntryFunc( void (GLUTCALLBACK *func) (int state) )
{
     entry_func = func;
}


void GLUTAPIENTRY
glutVisibilityFunc( void (GLUTCALLBACK *func) (int state) )
{
     visibility_func = func;
}


void GLUTAPIENTRY
glutMenuStateFunc( void (GLUTCALLBACK *func) (int state) )
{
     menu_state_func = func;
}


void GLUTAPIENTRY
glutSpecialFunc( void (GLUTCALLBACK *func) (int key, int x, int y) )
{
     special_func = func;
}


void GLUTAPIENTRY
glutSpaceballMotionFunc( void (GLUTCALLBACK *func) (int x, int y, int z) )
{
}


void GLUTAPIENTRY 
glutSpaceballRotateFunc( void (GLUTCALLBACK *func) (int x, int y, int z) )
{
}


void GLUTAPIENTRY
glutSpaceballButtonFunc( void (GLUTCALLBACK *func) (int button, int state) )
{
}


void GLUTAPIENTRY
glutButtonBoxFunc( void (GLUTCALLBACK *func) (int button, int state) )
{
}


void GLUTAPIENTRY
glutDialsFunc( void (GLUTCALLBACK *func) (int dial, int value) )
{
}


void GLUTAPIENTRY
glutTabletMotionFunc( void (GLUTCALLBACK *func) (int x, int y) )
{
}


void GLUTAPIENTRY
glutTabletButtonFunc( void (GLUTCALLBACK *func) (int button, int state, int x, int y) )
{
}


void GLUTAPIENTRY
glutMenuStatusFunc( void (GLUTCALLBACK *func) (int status, int x, int y) )
{
}


void GLUTAPIENTRY
glutOverlayDisplayFunc( void (GLUTCALLBACK *func) (void) )
{
}


void GLUTAPIENTRY
glutWindowStatusFunc( void (GLUTCALLBACK *func) (int state) )
{
}


void GLUTAPIENTRY
glutKeyboardUpFunc( void (GLUTCALLBACK *func) (unsigned char key, int x, int y) )
{
     keyboard_up_func = func;
}


void GLUTAPIENTRY
glutSpecialUpFunc( void (GLUTCALLBACK *func) (int key, int x, int y) )
{
     special_up_func = func;
}


void GLUTAPIENTRY 
glutJoystickFunc( void (GLUTCALLBACK *func)(unsigned int buttons, int x, int y, int z), int pollInterval )
{
     joystick_func = func;
     /* FIXME: take care of pollInterval */
}


void GLUTAPIENTRY
glutIdleFunc( void (GLUTCALLBACK *func) (void) )
{
     idle_func = func;
}


void GLUTAPIENTRY
glutTimerFunc( unsigned int msec, void (GLUTCALLBACK *func) (int value), int value )
{
     __GlutTimer *timer;
     
     if (!func)
          return;
          
     timer = calloc( 1, sizeof(__GlutTimer) );
     if (!timer)
          __glutFatalError( "out of memory" );
     
     timer->interval.tv_sec  = msec / 1000;
     timer->interval.tv_usec = (msec % 1000) * 1000;

     gettimeofday( &timer->expire, NULL );
     timer->expire.tv_usec += timer->interval.tv_usec;
     timer->expire.tv_sec  += timer->interval.tv_sec + timer->expire.tv_usec/1000000;
     timer->expire.tv_usec %= 1000000;
     
     timer->func  = func;
     timer->value = value;
     
     timer->next = g_timers;
     g_timers    = timer;
}


void
__glutHandleTimers( void )
{
     __GlutTimer    *cur;
     struct timeval  now;
          
     for (cur = g_timers; cur; cur = cur->next ) {
          gettimeofday( &now, NULL );
          
          if (cur->expire.tv_sec > now.tv_sec ||
             (cur->expire.tv_sec == now.tv_sec && 
              cur->expire.tv_usec >= now.tv_usec))
          {
               g_idle = GL_FALSE;
               
               cur->func( cur->value );
              
               cur->expire.tv_usec += cur->interval.tv_usec;
               cur->expire.tv_sec  += cur->interval.tv_sec + cur->expire.tv_usec/1000000;
               cur->expire.tv_usec %= 1000000;
          }
     }
}


GLboolean
__glutGetTimeout( int *ret_msec )
{
     __GlutTimer    *cur;
     struct timeval *time = NULL;
     struct timeval  now;
     
     for (cur = g_timers; cur; cur = cur->next) {
          if (time == NULL ||
              time->tv_sec > cur->expire.tv_sec ||
             (time->tv_sec == cur->expire.tv_sec && 
              time->tv_usec > cur->expire.tv_usec)) {
               time = &cur->expire;
          }
     }
     
     if (time == NULL)
          return GL_FALSE;
          
     gettimeofday( &now, NULL );
     
     *ret_msec = (time->tv_sec  - now.tv_sec) * 1000 +
                 (time->tv_usec - now.tv_usec + 999) / 1000;
                 
     return GL_TRUE;
}
     
          
void 
__glutFreeTimers( void )
{
     __GlutTimer *cur = g_timers;
     
     while (cur) {
          __GlutTimer *next = cur->next;
          free( cur );
          cur = next;
     }
     
     g_timers = NULL;
}

