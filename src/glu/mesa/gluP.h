/* $Id: gluP.h,v 1.2 1999/12/07 09:02:12 joukj Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * Copyright (C) 1995-1999  Brian Paul
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
 * $Log: gluP.h,v $
 * Revision 1.2  1999/12/07 09:02:12  joukj
 *
 *  Committing in .
 *
 *  Make supportupdate for VMS
 *
 *  Modified Files:
 *  	Mesa/src-glu/descrip.mms Mesa/src-glu/gluP.h
 *  ----------------------------------------------------------------------
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.4  1999/01/03 03:23:15  brianp
 * now using GLAPIENTRY and GLCALLBACK keywords (Ted Jump)
 *
 * Revision 1.3  1997/08/01 22:25:27  brianp
 * check for Cygnus Win32 (Stephen Rehel)
 *
 * Revision 1.2  1997/05/27 02:59:46  brianp
 * added defines for APIENTRY and CALLBACK if not compiling on Win32
 *
 * Revision 1.1  1996/09/27 01:19:39  brianp
 * Initial revision
 *
 */



/*
 * This file allows the GLU code to be compiled either with the Mesa
 * headers or with the real OpenGL headers.
 */


#ifndef GLUP_H
#define GLUP_H


#include "GL/gl.h"
#include "GL/glu.h"

#include <string.h>

#ifndef MESA
   /* If we're using the real OpenGL header files... */
#  define GLU_TESS_ERROR9	100159
#endif


#define GLU_NO_ERROR		GL_NO_ERROR


/* for Sun: */
#ifdef SUNOS4
#define MEMCPY( DST, SRC, BYTES) \
	memcpy( (char *) (DST), (char *) (SRC), (int) (BYTES) )
#else
#define MEMCPY( DST, SRC, BYTES) \
	memcpy( (void *) (DST), (void *) (SRC), (size_t) (BYTES) )
#endif


#ifndef NULL
#  define NULL 0
#endif


#endif
