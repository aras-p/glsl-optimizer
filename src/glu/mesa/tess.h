/* $Id: tess.h,v 1.15 1999/11/05 20:37:14 gareth Exp $ */

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
 * GLU 1.3 Polygon Tessellation by Gareth Hughes <garethh@bell-labs.com>
 *
 *****************************************************************************/

#ifndef __GLU_TESS_H__
#define __GLU_TESS_H__

#include <stdarg.h>
#include <stdio.h>

#include "gluP.h"

#include "tess_typedefs.h"
#include "tess_macros.h"
#include "tess_hash.h"
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
    heap_t		*ears;
    hashtable_t		*cvc_lists;
    void		*data;
    GLboolean		edge_flag;
    GLuint		label;
    GLenum		error;
};


/*****************************************************************************
 * Tessellation error handler:
 *****************************************************************************/
extern void tess_error_callback( GLUtesselator *, GLenum );


/*****************************************************************************
 * Debugging output:
 *****************************************************************************/
#ifdef DEBUG
extern	int	tess_dbg_level;

#define DBG_LEVEL_BASE		1
#define DBG_LEVEL_VERBOSE	10
#define DBG_LEVEL_ENTEREXIT	20

#ifdef _WIN32
#define DBG_STREAM	stdout
#else
#define DBG_STREAM	stderr
#endif

#ifdef __GNUC__
#define MSG( level, format, args... )					\
    if ( level <= tess_dbg_level ) {					\
	fprintf( DBG_STREAM, "%9.9s:%d:\t ", __FILE__, __LINE__ );	\
	fprintf( DBG_STREAM, format, ## args );				\
	fflush( DBG_STREAM );						\
    }
#else
#define MSG		tess_msg
#endif /* __GNUC__ */

#else
#define MSG		tess_msg
#endif /* DEBUG */

extern INLINE void tess_msg( int level, char *format, ... );
extern INLINE void tess_info( char *file, char *line );

#ifdef __cplusplus
}
#endif

#endif /* __GLU_TESS_H__ */
