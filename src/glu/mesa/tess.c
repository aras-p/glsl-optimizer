/* $Id: tess.c,v 1.4 1999/09/13 22:20:13 gareth Exp $ */

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

/*
 * $Log: tess.c,v $
 * Revision 1.4  1999/09/13 22:20:13  gareth
 * Fixed file headers.  Tracking down macro bugs.
 *
 */

/*****************************************************************************
 *
 * GLU 1.3 Polygon Tessellation by Gareth Hughes <garethh@lucent.com>
 *
 *****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glu.h>

#include "tess.h"
#include "tess_macros.h"
#include "tess_fist.h"
#if 0
#include "tess_grid.h"
#endif

/*****************************************************************************
 * Internal function prototypes:
 *****************************************************************************/

static void init_callbacks( tess_callbacks_t *callbacks );

static void tess_cleanup( GLUtesselator *tobj );
static void inspect_current_contour( GLUtesselator *tobj );

static void delete_current_contour( GLUtesselator *tobj );
static void delete_all_contours( GLUtesselator *tobj );

#define TESS_CHECK_ERRORS(t)	if ( (t)->error != GLU_NO_ERROR ) goto cleanup

int	tess_debug_level = 0;
GLdouble origin[3] = { 0.0, 0.0, 0.0 };


/*****************************************************************************
 *
 *			GLU TESSELLATION FUNCTIONS
 *
 *****************************************************************************/


/*****************************************************************************
 * gluNewTess
 *****************************************************************************/
GLUtesselator* GLAPIENTRY gluNewTess( void )
{
    GLUtesselator *tobj;

    if ( ( tobj = (GLUtesselator *)
	   malloc( sizeof(GLUtesselator) ) ) == NULL )
    {
	return NULL;
    }

    init_callbacks( &tobj->callbacks );

    tobj->boundary_only = GL_FALSE;
    tobj->winding_rule = GLU_TESS_WINDING_ODD;
    tobj->tolerance = 0.0;

    tobj->plane.normal[X] = 0.0;
    tobj->plane.normal[Y] = 0.0;
    tobj->plane.normal[Z] = 0.0;
    tobj->plane.dist = 0.0;

    tobj->contour_count = 0;
    tobj->contours = tobj->last_contour = NULL;
    tobj->current_contour = NULL;

    CLEAR_BBOX_2DV( tobj->mins, tobj->maxs );

    tobj->vertex_count = 0;
    tobj->sorted_vertices = NULL;
#if 0
    tobj->grid = NULL;
#endif

    tobj->error = GLU_NO_ERROR;

    return tobj;
}


/*****************************************************************************
 * gluDeleteTess
 *****************************************************************************/
void GLAPIENTRY gluDeleteTess( GLUtesselator *tobj )
{
    if ( tobj->error == GLU_NO_ERROR && ( tobj->contour_count > 0 ) )
    {
	/* Was gluEndPolygon called? */
	tess_error_callback( tobj, GLU_TESS_ERROR3, NULL );
    }

    /* Delete all internal structures */
    tess_cleanup( tobj );
    free( tobj );
}


/*****************************************************************************
 * gluTessBeginPolygon
 *****************************************************************************/
void GLAPIENTRY gluTessBeginPolygon( GLUtesselator *tobj, void *polygon_data )
{
    tobj->error = GLU_NO_ERROR;

    if ( tobj->current_contour != NULL )
    {
	/* gluEndPolygon was not called */
	tess_error_callback( tobj, GLU_TESS_ERROR3, NULL );

	tess_cleanup( tobj );
    }
}


/*****************************************************************************
 * gluTessBeginContour
 *****************************************************************************/
void GLAPIENTRY gluTessBeginContour( GLUtesselator *tobj )
{
    TESS_CHECK_ERRORS( tobj );

    if ( tobj->current_contour != NULL )
    {
	/* gluTessEndContour was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR4, NULL );
	return;
    }

    if ( ( tobj->current_contour =
	   (tess_contour_t *) malloc( sizeof(tess_contour_t) ) ) == NULL )
    {
	tess_error_callback( tobj, GLU_OUT_OF_MEMORY, NULL );
	return;
    }

    COPY_3V( tobj->plane.normal, tobj->current_contour->plane.normal );
    tobj->current_contour->plane.dist = tobj->plane.dist;

    tobj->current_contour->vertex_count = 0;
    tobj->current_contour->vertices =
	tobj->current_contour->last_vertex = NULL;

    tobj->current_contour->reflex_count = 0;
    tobj->current_contour->reflex_vertices =
	tobj->current_contour->last_reflex = NULL;

    tobj->current_contour->orientation = GLU_UNKNOWN;
    tobj->current_contour->area = 0.0;

    tobj->current_contour->winding = 0;
    CLEAR_BBOX_2DV( tobj->current_contour->mins,
		    tobj->current_contour->maxs );

 cleanup:
    return;
}


/*****************************************************************************
 * gluTessVertex
 *****************************************************************************/
void GLAPIENTRY gluTessVertex( GLUtesselator *tobj, GLdouble coords[3],
			       void *vertex_data )
{
    tess_contour_t		*current = tobj->current_contour;
    tess_vertex_t		*last_vertex;

    TESS_CHECK_ERRORS( tobj );

    if ( current == NULL )
    {
	/* gluTessBeginContour was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR2, NULL );
	return;
    }

    tobj->vertex_count++;

    last_vertex = current->last_vertex;

    if ( last_vertex == NULL )
    {
	if ( ( last_vertex = (tess_vertex_t *)
	       malloc( sizeof(tess_vertex_t) ) ) == NULL )
	{
	    tess_error_callback( tobj, GLU_OUT_OF_MEMORY, NULL );
	    return;
	}

	current->vertices = last_vertex;
	current->last_vertex = last_vertex;

	last_vertex->index = -1;
	last_vertex->data = vertex_data;

	last_vertex->coords[X] = coords[X];
	last_vertex->coords[Y] = coords[Y];
	last_vertex->coords[Z] = coords[Z];

	last_vertex->next = NULL;
	last_vertex->previous = NULL;

	current->vertex_count++;
    }
    else
    {
	tess_vertex_t	*vertex;

	if ( ( vertex = (tess_vertex_t *)
	       malloc( sizeof(tess_vertex_t) ) ) == NULL )
	{
	    tess_error_callback( tobj, GLU_OUT_OF_MEMORY, NULL );
	    return;
	}

	vertex->index = -1;
	vertex->data = vertex_data;

	vertex->coords[X] = coords[X];
	vertex->coords[Y] = coords[Y];
	vertex->coords[Z] = coords[Z];

	vertex->next = NULL;
	vertex->previous = last_vertex;

	current->vertex_count++;

	last_vertex->next = vertex;
	current->last_vertex = vertex;
    }

 cleanup:
    return;
}


/*****************************************************************************
 * gluTessEndContour
 *****************************************************************************/
void GLAPIENTRY gluTessEndContour( GLUtesselator *tobj )
{
    TESS_CHECK_ERRORS( tobj );

    if ( tobj->current_contour == NULL )
    {
	/* gluTessBeginContour was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR2, NULL );
	return;
    }

    if ( tobj->current_contour->vertex_count > 0 )
    {
	inspect_current_contour( tobj );
    }

 cleanup:
    return;
}


/*****************************************************************************
 * gluTessEndPolygon
 *****************************************************************************/
void GLAPIENTRY gluTessEndPolygon( GLUtesselator *tobj )
{
    TESS_CHECK_ERRORS( tobj );

    if ( tobj->current_contour != NULL )
    {
	/* gluTessBeginPolygon was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR1, NULL );
	return;
    }
    TESS_CHECK_ERRORS( tobj );

    /*
     * Ensure we have at least one contour to tessellate.  If we have none,
     *  clean up and exit gracefully.
     */
    if ( tobj->contour_count == 0 )
    {
	tess_cleanup( tobj );
	return;
    }

    /* Wrap the contour list. */

    tobj->last_contour->next = tobj->contours;
    tobj->contours->previous = tobj->last_contour;

    /* tess_find_contour_hierarchies(tobj); */

    TESS_CHECK_ERRORS( tobj );

    /* tess_handle_holes(tobj); */

    TESS_CHECK_ERRORS( tobj );

    /*
     * Before we tessellate the contours, ensure we have the appropriate
     *  callbacks registered.  We at least need the begin, vertex and end
     *  callbacks to do any meaningful work.
     */
    if ( ( ( tobj->callbacks.begin != NULL ) ||
	   ( tobj->callbacks.beginData != NULL ) ) &&
	 ( ( tobj->callbacks.vertex != NULL ) ||
	   ( tobj->callbacks.vertexData != NULL ) ) &&
	 ( ( tobj->callbacks.end != NULL ) ||
	   ( tobj->callbacks.endData != NULL ) ) )
    {
	if ( ( tobj->callbacks.edgeFlag == NULL ) &&
	     ( tobj->callbacks.edgeFlagData == NULL ) )
	{
	    fist_tessellation( tobj );
	}
	else
	{
	    fist_tessellation( tobj );
	}
    }

 cleanup:
    delete_all_contours( tobj );
}


/*****************************************************************************
 * gluTessCallback
 *****************************************************************************/
void GLAPIENTRY gluTessCallback( GLUtesselator *tobj, GLenum which,
				 void (GLCALLBACK *fn)() )
{
    switch ( which )
    {
	/* Register the begin callbacks. */
    case GLU_TESS_BEGIN:
	tobj->callbacks.begin = (void (GLCALLBACK*)(GLenum)) fn;
	break;
    case GLU_TESS_BEGIN_DATA:
	tobj->callbacks.beginData = (void (GLCALLBACK*)(GLenum, void *)) fn;
	break;

	/* Register the edge flag callbacks. */
    case GLU_TESS_EDGE_FLAG:
	tobj->callbacks.edgeFlag = (void (GLCALLBACK*)(GLboolean)) fn;
	break;
    case GLU_TESS_EDGE_FLAG_DATA:
	tobj->callbacks.edgeFlagData =
	    (void (GLCALLBACK*)(GLboolean, void *)) fn;
	break;

	/* Register the vertex callbacks. */
    case GLU_TESS_VERTEX:
	tobj->callbacks.vertex = (void (GLCALLBACK*)(void *)) fn;
	break;
    case GLU_TESS_VERTEX_DATA:
	tobj->callbacks.vertexData = (void (GLCALLBACK*)(void *, void *)) fn;
	break;

	/* Register the end callbacks. */
    case GLU_TESS_END:
	tobj->callbacks.end = (void (GLCALLBACK*)(void)) fn;
	break;
    case GLU_TESS_END_DATA:
	tobj->callbacks.endData = (void (GLCALLBACK*)(void *)) fn;
	break;

	/* Register the error callbacks. */
    case GLU_TESS_ERROR:
	tobj->callbacks.error = (void (GLCALLBACK*)(GLenum)) fn;
	break;
    case GLU_TESS_ERROR_DATA:
	tobj->callbacks.errorData = (void (GLCALLBACK*)(GLenum, void *)) fn;
	break;

	/* Register the combine callbacks. */
    case GLU_TESS_COMBINE:
	tobj->callbacks.combine =
	    (void (GLCALLBACK*)(GLdouble[3], void *[4],
				GLfloat [4], void **)) fn;
	break;
    case GLU_TESS_COMBINE_DATA:
	tobj->callbacks.combineData =
	    (void (GLCALLBACK*)(GLdouble[3], void *[4], GLfloat [4],
				void **, void *)) fn;
	break;

    default:
	tobj->error = GLU_INVALID_ENUM;
	break;
    }
}


/*****************************************************************************
 * gluTessProperty
 *
 * Set the current value of the given property.
 *****************************************************************************/
void GLAPIENTRY gluTessProperty( GLUtesselator *tobj, GLenum which,
				 GLdouble value )
{
    switch ( which )
    {
    case GLU_TESS_BOUNDARY_ONLY:
	tobj->boundary_only = (GLboolean) value;
	break;

    case GLU_TESS_TOLERANCE:
	tobj->tolerance = value;
	break;

    case GLU_TESS_WINDING_RULE:
	tobj->winding_rule = (GLenum) value;
	break;

    default:
	tobj->error = GLU_INVALID_ENUM;
	break;
    }
}


/*****************************************************************************
 * gluGetTessProperty
 *
 * Return the current value of the given property.
 *****************************************************************************/
void GLAPIENTRY gluGetTessProperty( GLUtesselator *tobj, GLenum which,
				    GLdouble *value )
{
    switch ( which )
    {
    case GLU_TESS_BOUNDARY_ONLY:
	*value = tobj->boundary_only;
	break;

    case GLU_TESS_TOLERANCE:
	*value = tobj->tolerance;
	break;

    case GLU_TESS_WINDING_RULE:
	*value = tobj->winding_rule;
	break;

    default:
	tobj->error = GLU_INVALID_ENUM;
	break;
    }
}


/*****************************************************************************
 * gluTessNormal
 *
 * Set the current tessellation normal.
 *****************************************************************************/
void GLAPIENTRY gluTessNormal( GLUtesselator *tobj, GLdouble x,
			       GLdouble y, GLdouble z )
{
    tobj->plane.normal[X] = x;
    tobj->plane.normal[Y] = y;
    tobj->plane.normal[Z] = z;
}



/*****************************************************************************
 *
 *			OBSOLETE TESSELLATION FUNCTIONS
 *
 *****************************************************************************/

void GLAPIENTRY gluBeginPolygon( GLUtesselator *tobj )
{
    gluTessBeginPolygon( tobj, NULL );
    gluTessBeginContour( tobj );
}

void GLAPIENTRY gluNextContour( GLUtesselator *tobj, GLenum type )
{
    gluTessEndContour( tobj );
    gluTessBeginContour( tobj );
}

void GLAPIENTRY gluEndPolygon( GLUtesselator *tobj )
{
    gluTessEndContour( tobj );
    gluTessEndPolygon( tobj );
}



/*****************************************************************************
 * tess_error_callback
 *
 * Internal error handler.  Call the user-registered error callback.
 *****************************************************************************/
void tess_error_callback( GLUtesselator *tobj, GLenum errno, void *data )
{
    if ( tobj->error == GLU_NO_ERROR )
    {
	tobj->error = errno;
    }

    if ( tobj->callbacks.errorData != NULL )
    {
	( tobj->callbacks.errorData )( errno, data );
    }
    else if ( tobj->callbacks.error != NULL )
    {
	( tobj->callbacks.error )( errno );
    }
}



/*****************************************************************************
 *
 *				INTERNAL FUNCTIONS
 *
 *****************************************************************************/


/*****************************************************************************
 * init_callbacks
 *****************************************************************************/
static void init_callbacks( tess_callbacks_t *callbacks )
{
    callbacks->begin		= ( void (GLCALLBACK*)(GLenum) ) NULL;
    callbacks->beginData = ( void (GLCALLBACK*)(GLenum, void *) ) NULL;
    callbacks->edgeFlag		= ( void (GLCALLBACK*)(GLboolean) ) NULL;
    callbacks->edgeFlagData	= ( void (GLCALLBACK*)(GLboolean, void *) ) NULL;
    callbacks->vertex		= ( void (GLCALLBACK*)(void *) ) NULL;
    callbacks->vertexData	= ( void (GLCALLBACK*)(void *, void *) ) NULL;
    callbacks->end		= ( void (GLCALLBACK*)(void) ) NULL;
    callbacks->endData	= ( void (GLCALLBACK*)(void *) ) NULL;
    callbacks->error		= ( void (GLCALLBACK*)(GLenum) ) NULL;
    callbacks->errorData	= ( void (GLCALLBACK*)(GLenum, void *) ) NULL;
    callbacks->combine		= ( void (GLCALLBACK*)(GLdouble [3], void *[4],
						       GLfloat [4], void **) ) NULL;
    callbacks->combineData	= ( void (GLCALLBACK*)(GLdouble [3], void *[4],
						       GLfloat [4], void **,
						       void *) ) NULL;
}


/*****************************************************************************
 * tess_cleanup
 *****************************************************************************/
static void tess_cleanup( GLUtesselator *tobj )
{
    if ( tobj->current_contour != NULL )
    {
	delete_current_contour( tobj );
    }

    if ( tobj->contours != NULL )
    {
	delete_all_contours( tobj );
    }
}


/*****************************************************************************
 * inspect_current_contour
 *****************************************************************************/
static GLenum	find_normal( GLUtesselator *tobj );
static void	project_current_contour( GLUtesselator *tobj );
static GLenum	save_current_contour( GLUtesselator *tobj );

static void inspect_current_contour( GLUtesselator *tobj )
{
    tess_contour_t *current = tobj->current_contour;

    if ( current->vertex_count < 3 )
    {
	delete_current_contour( tobj );
	return;
    }

    current->last_vertex->next = current->vertices;
    current->vertices->previous = current->last_vertex;

    if ( ( tobj->contours == NULL ) &&
	 ( COMPARE_3DV( current->plane.normal, origin ) ) )
    {
	/* We haven't been given a normal, so let's take a guess. */
	if ( find_normal( tobj ) == GLU_ERROR ) {
	    return;
	}
	COPY_3V( current->plane.normal, tobj->plane.normal );
	tobj->plane.dist = current->plane.dist;
    }

    project_current_contour( tobj );

    if ( save_current_contour( tobj ) == GLU_ERROR ) {
	return;
    }
}

/*****************************************************************************
 * find_normal
 *****************************************************************************/
static GLenum find_normal( GLUtesselator *tobj )
{
    tess_contour_t	*contour = tobj->current_contour;
    tess_vertex_t	*va, *vb, *vc;
    GLdouble		a[3], b[3], c[3];

    if ( contour == NULL ) { return GLU_ERROR; }

    va = contour->vertices;
    vb = va->next;

    /* If va and vb are the same point, keep looking for a different vertex. */

    while ( COMPARE_3DV( va->coords, vb->coords ) && ( vb != va ) ) {
	vb = vb->next;
    }

    if ( vb == va ) {
	tess_error_callback( tobj, GLU_TESS_ERROR7, NULL );
    }

    SUB_3V( a, vb->coords, va->coords );

    for ( vc = vb->next; vc != va; vc = vc->next )
    {
	SUB_3V( b, vc->coords, va->coords );

	CROSS3( c, a, b );

	if ( ( ABSD( c[X] ) > EQUAL_EPSILON ) ||
	     ( ABSD( c[Y] ) > EQUAL_EPSILON ) ||
	     ( ABSD( c[Z] ) > EQUAL_EPSILON ) )
	{
	    COPY_3V( contour->plane.normal, c );
	    NORMALIZE_3DV( contour->plane.normal );

	    contour->plane.dist = - DOT3( contour->plane.normal, va->coords );

	    return GLU_NO_ERROR;
	}
    }
    tess_error_callback( tobj, GLU_TESS_ERROR7, NULL );

    return GLU_ERROR;
}

/*****************************************************************************
 * project_current_contour
 *****************************************************************************/
static GLdouble twice_contour_area( tess_vertex_t *vertex,
				    tess_vertex_t *last_vertex );

static void project_current_contour( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour;
    tess_vertex_t	*vertex;
    GLdouble		area;
    GLdouble		zaxis[3] = { 0.0, 0.0, 1.0 }, znormal[3], xnormal[3];
    GLdouble		dot, rotx, roty;
    GLuint		i;

    if ( current == NULL ) { return; }

    /* Rotate the plane normal around the y-axis. */

    znormal[X] = current->plane.normal[X];
    znormal[Y] = 0.0;
    znormal[Z] = current->plane.normal[Z];

    dot = DOT3( znormal, zaxis );
    roty = acos( dot );

    /* Rotate the plane normal around the x-axis. */

    xnormal[X] = cos( roty ) * znormal[X] - sin( roty ) * znormal[Z];
    xnormal[Y] = znormal[Y];
    xnormal[Z] = sin( roty ) * znormal[X] + cos( roty ) * znormal[Z];

    dot = DOT3( xnormal, zaxis );
    rotx = acos( dot );

    for ( vertex = current->vertices, i = 0;
	  i < current->vertex_count; vertex = vertex->next, i++ )
    {
	tess_plane_t	*plane = &current->plane;
	GLdouble	proj[3], yrot[3], xrot[3];

	/* FIXME: This needs a cleanup, 'cos I'm sure it's inefficient. */

	proj[X] = vertex->coords[X] - plane->dist * plane->normal[X];
	proj[Y] = vertex->coords[Y] - plane->dist * plane->normal[Y];
	proj[Z] = vertex->coords[Z] - plane->dist * plane->normal[Z];

	yrot[X] = cos( roty ) * proj[X] - sin( roty ) * proj[Z];
	yrot[Y] = proj[Y];
	yrot[Z] = sin( roty ) * proj[X] + cos( roty ) * proj[Z];

	xrot[X] = yrot[X];
	xrot[Y] = cos( rotx ) * yrot[Y] - sin( rotx ) * yrot[Z];
	xrot[Z] = sin( rotx ) * yrot[Y] + cos( rotx ) * yrot[Z];

	vertex->v[X] = xrot[X];
	vertex->v[Y] = xrot[Y];

	ACC_BBOX_2V( vertex->v, tobj->mins, tobj->maxs );
	ACC_BBOX_2V( vertex->v, current->mins, current->maxs );
    }

    area = twice_contour_area( current->vertices,
			       current->last_vertex );
    if ( area >= 0.0 )
    {
	current->orientation = GLU_CCW;
	current->area = area;
    }
    else
    {
	current->orientation = GLU_CW;
	current->area = -area;
    }
}

/*****************************************************************************
 * twice_contour_area
 *****************************************************************************/
static GLdouble twice_contour_area( tess_vertex_t *vertex,
				    tess_vertex_t *last_vertex )
{
    tess_vertex_t	*next;
    GLdouble		area, x, y;

    area = 0.0;

    x = vertex->v[X];
    y = vertex->v[Y];

    vertex = vertex->next;

    while ( vertex != last_vertex )
    {
	next = vertex->next;
	area +=
	    (vertex->v[X] - x) * (next->v[Y] - y) -
	    (vertex->v[Y] - y) * (next->v[X] - x);

	vertex = vertex->next;
    }
    return area;
}


/*****************************************************************************
 * save_current_contour
 *****************************************************************************/
static GLenum save_current_contour( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour;
    tess_vertex_t	*vertex;
    GLuint			i;

    if ( current == NULL ) { return GLU_ERROR; }

    if ( tobj->contours == NULL )
    {
	tobj->contours = tobj->last_contour = current;
	current->next = current->previous = NULL;
    }
    else
    {
	current->previous = tobj->last_contour;

	tobj->last_contour->next = current;
	tobj->last_contour = current;

	current->next = NULL;
    }

    for ( vertex = current->vertices, i = 0;
	  i < current->vertex_count; vertex = vertex->next, i++ )
    {
	vertex->shadow_vertex = NULL;
	vertex->edge_flag = GL_TRUE;
    }

    current->type = GLU_UNKNOWN;

    tobj->contour_count++;
    tobj->current_contour = NULL;

    return GLU_NO_ERROR;
}

/*****************************************************************************
 * delete_current_contour
 *****************************************************************************/
static void delete_current_contour( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour;
    tess_vertex_t	*vertex, *next;
    GLuint			i;

    if ( current == NULL ) { return; }

    for ( vertex = current->vertices, i = 0; i < current->vertex_count; i++)
    {
	next = vertex->next;
	free( vertex );
	vertex = next;
    }

    free( current );
    tobj->current_contour = NULL;
}

/*****************************************************************************
 * delete_all_contours
 *****************************************************************************/
static void delete_all_contours( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour, *next_contour;
    tess_vertex_t	*vertex, *next_vertex;
    GLuint			i;

    if ( current != NULL )
    {
	delete_current_contour( tobj );
    }

    for ( current = tobj->contours, i = 0; i < tobj->contour_count; i++ )
    {
	vertex = current->vertices;

	while ( vertex != current->last_vertex )
	{
	    next_vertex = vertex->next;
	    free( vertex );
	    vertex = next_vertex;
	}
	free( vertex );
	next_contour = current->next;

	free( current );
	current = next_contour;
    }

    tobj->contour_count = tobj->vertex_count = 0;
    tobj->contours = tobj->last_contour = NULL;

    CLEAR_BBOX_2DV( tobj->mins, tobj->maxs );

    ZERO_3V( tobj->plane.normal );
    tobj->plane.dist = 0.0;
}



/*****************************************************************************
 * Debugging output
 *****************************************************************************/
#ifdef _DEBUG
int vdebugstr( char *format_str, ... )
{
    va_list ap;
    va_start( ap, format_str );

    vfprintf( stderr, format_str, ap );
    va_end( ap );
    return 0;
}
#endif
