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

#include "internal.h"


int GLUTAPIENTRY 
glutCreateMenu( void (GLUTCALLBACK *func) (int) )
{
     return 0;
}

void GLUTAPIENTRY
glutDestroyMenu( int menu )
{
}


int GLUTAPIENTRY
glutGetMenu( void )
{
     return 0;
}


void GLUTAPIENTRY 
glutSetMenu( int menu )
{
}


void GLUTAPIENTRY
glutAddMenuEntry( const char *label, int value )
{
}


void GLUTAPIENTRY
glutAddSubMenu( const char *label, int submenu )
{
}


void GLUTAPIENTRY
glutChangeToMenuEntry( int item, const char *label, int value )
{
}


void GLUTAPIENTRY
glutChangeToSubMenu( int item, const char *label, int submenu )
{
}


void GLUTAPIENTRY
glutRemoveMenuItem( int item )
{
}


void GLUTAPIENTRY
glutAttachMenu( int button )
{
}


void GLUTAPIENTRY
glutDetachMenu( int button )
{
}
