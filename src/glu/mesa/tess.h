/* $Id: tess.h,v 1.3 1999/09/10 03:05:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*****************************************************************************
 *
 * GLU 1.3 Polygon Tessellation by Gareth Hughes <garethh@lucent.com>
 *
 *****************************************************************************/

#ifndef __GLU_TESS_H__
#define __GLU_TESS_H__

#include <stdarg.h>
#include <stdio.h>

#include "gluP.h"

#include "tess_typedefs.h"
#include "tess_heap.h"
#if 0
#include "tess_grid.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * The GLUtesselator structure:
 *****************************************************************************/
struct GLUtesselator
{
    tess_callbacks_t	callbacks;
    GLboolean		boundary_only;
    GLenum		winding_rule;
    GLdouble		tolerance;
    tess_plane_t	plane;
    GLuint		contour_count;
    tess_contour_t	*contours, *last_contour;
    tess_contour_t	*current_contour;
    GLdouble		mins[2], maxs[2];
    GLuint		vertex_count;
    tess_vertex_t	**sorted_vertices;
#if 0
    tess_grid_t		*grid;			/* Not currently used... */
#endif
    heap_t		*heap;
    GLenum		error;
};


/*****************************************************************************
 * Tessellation error handler:
 *****************************************************************************/
extern void tess_error_callback( GLUtesselator *, GLenum, void * );


/*****************************************************************************
 * Debugging output:  (to be removed...)
 *****************************************************************************/
extern	int	tess_debug_level;
int vdebugstr( char *format_str, ... );

#ifdef _DEBUG
#define DEBUGP(level, body)						\
    do {								\
	if ( tess_debug_level >= level ) {				\
	    vdebugstr( "%11.11s:%-5d ", __FILE__, __LINE__, level );	\
	    vdebugstr body;						\
	    fflush ( stderr );						\
	}								\
    } while ( 0 )
#define DEBUGIF(level)          do { if ( tess_debug_level >= level ) {
#define DEBUGENDIF              } } while ( 0 )
#else
#define DEBUGP(level, body)
#define DEBUGIF(level)		while(0) {
#define DEBUGENDIF		}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GLU_TESS_H__ */
