/* $Id: glstate.h,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

/*
 * Print GL state information (for debugging)
 * Copyright (C) 1998  Brian Paul
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
 * $Log: glstate.h,v $
 * Revision 1.1  1999/08/19 00:55:42  jtg
 * Initial revision
 *
 * Revision 1.2  1999/06/19 01:36:43  brianp
 * more features added
 *
 * Revision 1.1  1998/11/24 03:41:16  brianp
 * Initial revision
 *
 */


#ifndef GLSTATE_H
#define GLSTATE_H


#include <GL/gl.h>


extern const char *GetNameString( GLenum var );

extern void PrintState( int indent, GLenum var );

extern void PrintAttribState( GLbitfield attrib );

extern void PrintPixelStoreState( void );


#endif
