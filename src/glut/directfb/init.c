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

#include "internal.h"


static void 
__glutExit( void )
{
     __glutFreeTimers();
     __glutDestroyWindows();
     
     if (events) {
          events->Release( events );
          events = NULL;
     }
     
     if (joystick) {
          joystick->Release( joystick );
          joystick = NULL;
     }
     
     if (mouse) {
          mouse->Release( mouse );
          mouse = NULL;
     }
     
     if (keyboard) {
          keyboard->Release( keyboard );
          keyboard = NULL;
     }
     
     if (primary) {
          primary->Release( primary );
          primary = NULL;
     }
     
     if (dfb) {
          dfb->Release( dfb );
          dfb = NULL;
     }
}


void GLUTAPIENTRY 
glutInit( int *argcp, char **argv )
{
     DFBResult ret;
     
     if (dfb)
          return;
     
     glutGet( GLUT_ELAPSED_TIME );

     ret = DirectFBInit( argcp, argv ? &argv : NULL );
     if (ret)
          DirectFBErrorFatal( "DirectFBInit()", ret );
     
     ret = DirectFBCreate( &dfb );
     if (ret)
          DirectFBErrorFatal( "DirectFBCreate()", ret );
     
     ret = dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &primary );
     if (ret)
          DirectFBErrorFatal( "IDirectFB::GetDisplayLayer()", ret );
          
     ret = dfb->CreateEventBuffer( dfb, &events );
     if (ret)
          DirectFBErrorFatal( "IDirectFB::CreateEventBuffer()", ret );
          
     dfb->GetInputDevice( dfb, DIDID_KEYBOARD, &keyboard );
     dfb->GetInputDevice( dfb, DIDID_MOUSE, &mouse );
     dfb->GetInputDevice( dfb, DIDID_JOYSTICK, &joystick );
    
     primary->SetCooperativeLevel( primary, DLSCL_ADMINISTRATIVE );
     
     atexit( __glutExit );
}


void GLUTAPIENTRY 
glutInitDisplayMode( unsigned int mode )
{
     g_display_mode = mode;
}


void GLUTAPIENTRY 
glutInitWindowPosition( int x, int y )
{
     g_xpos = x;
     g_ypos = y;
}


void GLUTAPIENTRY 
glutInitWindowSize( int width, int height )
{
     g_width  = width;
     g_height = height;
}


void GLUTAPIENTRY
glutInitDisplayString( const char *string )
{
}

