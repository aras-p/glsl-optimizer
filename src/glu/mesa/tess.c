/* $Id: tess.c,v 1.24 2000/02/10 17:45:52 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "gluP.h"

#include "tess.h"
#include "tess_macros.h"
#include "tess_fist.h"
#if 0
#include "tess_grid.h"
#endif


#define TESS_CHECK_ERRORS(t)	if ( (t)->error != GLU_NO_ERROR ) goto cleanup

#ifdef DEBUG
GLint		tess_dbg_level;
#endif


/*****************************************************************************
 * tess_error_callback
 *
 * Internal error handler.  Call the user-registered error callback.
 *
 * 2nd arg changed from 'errno' to 'errnum' since MSVC defines errnum as
 *  a macro (of all things) and thus breaks the build -tjump
 *****************************************************************************/
void tess_error_callback( GLUtesselator *tobj, GLenum errnum )
{
    if ( tobj->error == GLU_NO_ERROR )
    {
	tobj->error = errnum;
    }

    if ( tobj->callbacks.errorData != NULL )
    {
	( tobj->callbacks.errorData )( errnum, tobj->data );
    }
    else if ( tobj->callbacks.error != NULL )
    {
	( tobj->callbacks.error )( errnum );
    }
}


/*****************************************************************************
 * init_callbacks
 *****************************************************************************/
static void init_callbacks( tess_callbacks_t *callbacks )
{
    callbacks->begin        = ( void (GLCALLBACKPCAST)(GLenum) ) NULL;
    callbacks->beginData    = ( void (GLCALLBACKPCAST)(GLenum, void *) ) NULL;
    callbacks->edgeFlag     = ( void (GLCALLBACKPCAST)(GLboolean) ) NULL;
    callbacks->edgeFlagData = ( void (GLCALLBACKPCAST)(GLboolean, void *) ) NULL;
    callbacks->vertex       = ( void (GLCALLBACKPCAST)(void *) ) NULL;
    callbacks->vertexData   = ( void (GLCALLBACKPCAST)(void *, void *) ) NULL;
    callbacks->end          = ( void (GLCALLBACKPCAST)(void) ) NULL;
    callbacks->endData      = ( void (GLCALLBACKPCAST)(void *) ) NULL;
    callbacks->error        = ( void (GLCALLBACKPCAST)(GLenum) ) NULL;
    callbacks->errorData    = ( void (GLCALLBACKPCAST)(GLenum, void *) ) NULL;
    callbacks->combine      = ( void (GLCALLBACKPCAST)(GLdouble [3], void *[4],
						   GLfloat [4], void **) ) NULL;
    callbacks->combineData  = ( void (GLCALLBACKPCAST)(GLdouble [3], void *[4],
						   GLfloat [4], void **,
						   void *) ) NULL;
}


/*****************************************************************************
 * find_normal
 *****************************************************************************/
static GLenum find_normal( GLUtesselator *tobj )
{
    tess_contour_t	*contour = tobj->current_contour;
    tess_vertex_t	*va, *vb, *vc;
    GLdouble		a[3], b[3], c[3];

    MSG( 15, "      --> find_normal( tobj:%p )\n", tobj );

    if ( contour == NULL ) { return GLU_ERROR; }

    va = contour->vertices;
    vb = va->next;

    /* If va and vb are the same point, keep looking for a different vertex. */

    while ( IS_EQUAL_3DV( va->coords, vb->coords ) && ( vb != va ) ) {
	vb = vb->next;
    }

    if ( vb == va ) {
	/* FIXME: What error is this? */
	tess_error_callback( tobj, GLU_TESS_ERROR7 );
    }

    SUB_3V( a, vb->coords, va->coords );

    for ( vc = vb->next; vc != va; vc = vc->next )
    {
	SUB_3V( b, vc->coords, va->coords );

	CROSS_3V( c, a, b );

	if ( ! IS_ZERO_3DV( c ) )
	{
	    MSG( 15, "            using (%.2f,%.2f) -> (%.2f,%.2f) -> (%.2f,%.2f)\n",
		 va->coords[X], va->coords[Y],
		 vb->coords[X], vb->coords[Y],
		 vc->coords[X], vc->coords[Y] );

	    COPY_3V( contour->plane.normal, c );
	    NORMALIZE_3DV( contour->plane.normal );

	    contour->plane.dist = - DOT_3V( contour->plane.normal,
					    va->coords );

	    MSG( 15, "      <-- find_normal( tobj:%p ) n: (%.2f, %.2f, %.2f)\n", tobj, contour->plane.normal[X], contour->plane.normal[Y], contour->plane.normal[Z] );
	    return GLU_NO_ERROR;
	}
    }
    /* FIXME: What error is this? */
    tess_error_callback( tobj, GLU_TESS_ERROR7 );

    return GLU_ERROR;
}

/*****************************************************************************
 * twice_contour_area
 *
 * Calculate the twice the signed area of the given contour.  Used to
 * determine the contour's orientation amongst other things.
 *****************************************************************************/
GLdouble twice_contour_area( tess_contour_t *contour )
{
    tess_vertex_t	*vertex = contour->vertices;
    GLdouble		area, x, y;

    area = 0.0;

    x = vertex->v[X];
    y = vertex->v[Y];

    vertex = vertex->next;

    do
    {
	area += ( (vertex->v[X] - x) * (vertex->next->v[Y] - y) -
		  (vertex->v[Y] - y) * (vertex->next->v[X] - x) );
	vertex = vertex->next;
    }
    while ( vertex != contour->vertices );

    return area;
}

/*****************************************************************************
 * project_current_contour
 *
 * Project the contour's vertices onto the tessellation plane.  We perform
 * a complex rotation here to allow non-axis-aligned tessellation normals.
 *****************************************************************************/
static void project_current_contour( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour;
    tess_vertex_t	*vertex;
    GLdouble		zaxis[3] = { 0.0, 0.0, 1.0 }, znormal[3], xnormal[3];
    GLdouble		dot, rotx, roty;
    GLint		i;

    MSG( 15, "      --> project_current_contour( tobj:%p )\n", tobj );

    if ( current == NULL ) { return; }

    /* Rotate the plane normal around the y-axis. */

    znormal[X] = current->plane.normal[X];
    znormal[Y] = 0.0;
    znormal[Z] = current->plane.normal[Z];

    dot = DOT_3V( znormal, zaxis );
    current->roty = roty = acos( dot );

    /* Rotate the plane normal around the x-axis. */

    xnormal[X] = cos( roty ) * znormal[X] - sin( roty ) * znormal[Z];
    xnormal[Y] = znormal[Y];
    xnormal[Z] = sin( roty ) * znormal[X] + cos( roty ) * znormal[Z];

    dot = DOT_3V( xnormal, zaxis );
    current->rotx = rotx = acos( dot );

    for ( vertex = current->vertices, i = 0;
	  i < current->num_vertices; vertex = vertex->next, i++ )
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

    current->area = twice_contour_area( current );
    current->orientation = ( current->area >= 0.0 ) ? GLU_CCW : GLU_CW;

    MSG( 15, "            area: %.2f orientation: %s\n",
	 current->area, ( current->orientation == GLU_CCW ) ? "CCW" : "CW" );

    MSG( 15, "      <-- project_current_contour( tobj:%p )\n", tobj );
}

/*****************************************************************************
 * save_current_contour
 *****************************************************************************/
static GLenum save_current_contour( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour;
    tess_vertex_t	*vertex;
    GLint		i;

    if ( current == NULL ) { return GLU_ERROR; }

    if ( tobj->contours == NULL )
    {
	tobj->contours = tobj->last_contour = current;
	current->next = current->prev = NULL;

	tobj->orientation = current->orientation;
    }
    else
    {
	current->prev = tobj->last_contour;

	tobj->last_contour->next = current;
	tobj->last_contour = current;

	current->next = NULL;
    }

    for ( vertex = current->vertices, i = 0;
	  i < current->num_vertices; vertex = vertex->next, i++ )
    {
	vertex->edge_flag = GL_TRUE;
    }

    current->type = GLU_UNKNOWN;

    tobj->num_contours++;
    tobj->current_contour = NULL;

    return GLU_NO_ERROR;
}

/*****************************************************************************
 * inspect_current_contour
 *****************************************************************************/
static void inspect_current_contour( GLUtesselator *tobj )
{
    tess_contour_t	*current = tobj->current_contour;
    GLdouble		origin[3] = { 0.0, 0.0, 0.0 };
    GLboolean		calc_normal = GL_FALSE;

    MSG( 15, "    --> inspect_current_contour( tobj:%p )\n", tobj );

    if ( current->num_vertices < 3 )
    {
	MSG( 15, "          count %d < 3, deleting\n", current->num_vertices );
	delete_contour( &tobj->current_contour );
	return;
    }

    current->last_vertex->next = current->vertices;
    current->vertices->prev = current->last_vertex;

    MSG( 15, "          current normal: (%.2f, %.2f, %.2f)\n", current->plane.normal[X], current->plane.normal[Y], current->plane.normal[Z] );

    if ( IS_EQUAL_3DV( current->plane.normal, origin ) )
    {
	/* We haven't been given a normal, so let's take a guess. */
	if ( find_normal( tobj ) == GLU_ERROR ) {
	    return;
	}

	COPY_3V( tobj->plane.normal, current->plane.normal );
	tobj->plane.dist = current->plane.dist;

	calc_normal = GL_TRUE;
    }

    project_current_contour( tobj );

    if ( calc_normal && ( tobj->current_contour->orientation == GLU_CW ) )
    {
	MSG( 15, "          oops, let's try that again...\n" );

	/*
	 * FIXME: We've used a reflex angle to calculate the normal.  At
	 * the moment, we simply reverse the normal and re-project the
	 * contour, but this is sloooow...
	 */
	NEG_3V( tobj->plane.normal );
	NEG_3V( tobj->current_contour->plane.normal );

	project_current_contour( tobj );
    }

    if ( save_current_contour( tobj ) == GLU_ERROR ) {
	return;
    }

    MSG( 15, "    <-- inspect_current_contour( tobj:%p )\n", tobj );
}

/*****************************************************************************
 * reverse_contour
 *****************************************************************************/
void reverse_contour( tess_contour_t *contour )
{
    tess_vertex_t	*current = contour->vertices;
    GLint		i;

    for ( i = 0 ; i < contour->num_vertices ; i++ )
    {
	tess_vertex_t	*next = current->next;
	tess_vertex_t	*prev = current->prev;

	current->next = prev;
	current->prev = next;

	current = next;
    }

    contour->orientation =
	( contour->orientation == GLU_CCW ) ? GLU_CW : GLU_CCW;

    contour->last_vertex = contour->vertices->prev;
}

/*****************************************************************************
 * orient_contours
 *
 * Sum the signed areas of the contours, and orient the contours such that
 * this sum is nonnegative.
 *****************************************************************************/
static void orient_contours( GLUtesselator *tobj )
{
    tess_contour_t	*contour = tobj->contours;
    GLdouble		sum = 0.0;
    GLint		i;

    MSG( 15, "    --> orient_contours( tobj:%p )\n", tobj );

    /* Sum the signed areas of all contours */
    for ( i = 0 ; i < tobj->num_contours ; i++ )
    {
	sum += contour->area;
	contour = contour->next;
    }

    MSG( 15, "          signed area: %.2f\n", sum );

    if ( sum < -GLU_TESS_EPSILON )
    {
	for ( i = 0 ; i < tobj->num_contours ; i++ )
	{
	    contour->area = ABSD( contour->area );
	    reverse_contour( contour );

	    contour = contour->next;
	}
    }
    else
    {
	for ( i = 0 ; i < tobj->num_contours ; i++ )
	{
	    contour->area = ABSD( contour->area );

	    contour = contour->next;
	}
    }

    tobj->orientation = tobj->contours->orientation;

    MSG( 15, "    <-- orient_contours( tobj:%p ) orient: %s\n",
	 tobj, ( tobj->orientation == GLU_CCW ) ? "GLU_CCW" : "GLU_CW" );
}


/*****************************************************************************
 * delete_contour
 *
 * Delete the given contour and set the pointer to NULL.
 *****************************************************************************/
void delete_contour( tess_contour_t **contour )
{
    tess_vertex_t	*vertex, *next;
    GLint		i;

    if ( *contour == NULL ) { return; }

    vertex = (*contour)->vertices;

    for ( i = 0 ; i < (*contour)->num_vertices ; i++ )
    {
	next = vertex->next;
	free( vertex );
	vertex = next;
    }

    free( *contour );
    *contour = NULL;
}

/*****************************************************************************
 * delete_all_contours
 *****************************************************************************/
static void delete_all_contours( GLUtesselator *tobj )
{
    tess_contour_t	*current, *next_contour;
    GLint		i;

    if ( tobj->current_contour != NULL ) {
	delete_contour( &tobj->current_contour );
    }

    for ( current = tobj->contours, i = 0 ; i < tobj->num_contours ; i++ )
    {
	tess_vertex_t	*vertex = current->vertices, *next_vertex;
	GLint		j;

	for ( j = 0 ; j < current->num_vertices ; j ++ )
	{
	    next_vertex = vertex->next;
	    free( vertex );
	    vertex = next_vertex;
	}
	next_contour = current->next;

	free( current );
	current = next_contour;
    }

    tobj->num_contours = tobj->num_vertices = 0;
    tobj->contours = tobj->last_contour = NULL;

    CLEAR_BBOX_2DV( tobj->mins, tobj->maxs );
}


/*****************************************************************************
 * tess_cleanup
 *****************************************************************************/
static void tess_cleanup( GLUtesselator *tobj )
{
    MSG( 15, "  -> tess_cleanup( tobj:%p )\n", tobj );

    if ( tobj->current_contour != NULL ) {
	delete_contour( &tobj->current_contour );
    }
    if ( tobj->contours != NULL ) {
	delete_all_contours( tobj );
    }

    MSG( 15, "  <- tess_cleanup( tobj:%p )\n", tobj );
}


/*****************************************************************************
 * tess_msg
 *****************************************************************************/
INLINE void tess_msg( GLint level, char *format, ... )
{
#ifdef DEBUG
    va_list ap;
    va_start( ap, format );

    if ( level <= tess_dbg_level ) {
	vfprintf( DBG_STREAM, format, ap );
	fflush( DBG_STREAM );
    }

    va_end( ap );
#endif
}

INLINE void tess_info( char *file, GLint line )
{
#ifdef DEBUG
    fprintf( DBG_STREAM, "%9.9s:%d:\t ", file, line );
#endif
}



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

#ifdef DEBUG
    if ( getenv( "MESA_TESS_DBG_LEVEL" ) ) {
	tess_dbg_level = atoi( getenv( "MESA_TESS_DBG_LEVEL" ) );
    } else {
	tess_dbg_level = DBG_LEVEL_BASE;
    }
#endif

    MSG( 15, "-> gluNewTess()\n" );

    tobj = malloc( sizeof(GLUtesselator) );
    if ( tobj == NULL ) {
	return NULL;
    }

    init_callbacks( &tobj->callbacks );

    tobj->winding_rule = GLU_TESS_WINDING_ODD;
    tobj->boundary_only = GL_FALSE;
    tobj->tolerance = GLU_TESS_EPSILON;
    tobj->orientation = GLU_UNKNOWN;

    tobj->data = NULL;

    tobj->num_contours = 0;
    tobj->contours = tobj->last_contour = NULL;
    tobj->current_contour = NULL;

    CLEAR_BBOX_2DV( tobj->mins, tobj->maxs );

    tobj->num_vertices = 0;
    tobj->sorted_vertices = NULL;
#if 0
    tobj->grid = NULL;
#endif
    tobj->edge_flag = GL_FALSE;
    tobj->label = 0;

    ZERO_3V( tobj->plane.normal );
    tobj->plane.dist = 0.0;

    tobj->error = GLU_NO_ERROR;

    MSG( 15, "<- gluNewTess() tobj:%p\n", tobj );
    return tobj;
}


/*****************************************************************************
 * gluDeleteTess
 *****************************************************************************/
void GLAPIENTRY gluDeleteTess( GLUtesselator *tobj )
{
    MSG( 15, "-> gluDeleteTess( tobj:%p )\n", tobj );

    if ( ( tobj->error == GLU_NO_ERROR ) && ( tobj->num_contours > 0 ) )
    {
	/* gluEndPolygon was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR3 );
    }

    /* Delete all internal structures. */
    tess_cleanup( tobj );
    free( tobj );

    MSG( 15, "<- gluDeleteTess()\n" );
}


/*****************************************************************************
 * gluTessBeginPolygon
 *****************************************************************************/
void GLAPIENTRY gluTessBeginPolygon( GLUtesselator *tobj, void *polygon_data )
{
    MSG( 15, "-> gluTessBeginPolygon( tobj:%p data:%p )\n", tobj, polygon_data );

    tobj->error = GLU_NO_ERROR;

    if ( tobj->current_contour != NULL )
    {
	/* gluEndPolygon was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR3 );
	tess_cleanup( tobj );
    }

    tobj->data = polygon_data;
    tobj->num_vertices = 0;
    tobj->edge_flag = GL_FALSE;
    tobj->label = 0;

    MSG( 15, "<- gluTessBeginPolygon( tobj:%p data:%p )\n", tobj, polygon_data );
}


/*****************************************************************************
 * gluTessBeginContour
 *****************************************************************************/
void GLAPIENTRY gluTessBeginContour( GLUtesselator *tobj )
{
    MSG( 15, "  -> gluTessBeginContour( tobj:%p )\n", tobj );
    TESS_CHECK_ERRORS( tobj );

    if ( tobj->current_contour != NULL )
    {
	/* gluTessEndContour was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR4 );
	return;
    }

    tobj->current_contour = malloc( sizeof(tess_contour_t) );
    if ( tobj->current_contour == NULL ) {
	tess_error_callback( tobj, GLU_OUT_OF_MEMORY );
	return;
    }

    COPY_3V( tobj->current_contour->plane.normal, tobj->plane.normal );
    tobj->current_contour->plane.dist = tobj->plane.dist;

    tobj->current_contour->area = 0.0;
    tobj->current_contour->orientation = GLU_UNKNOWN;

    tobj->current_contour->label = 0;
    tobj->current_contour->winding = 0;

    /*tobj->current_contour->rotx = tobj->current_contour->roty = 0.0;*/

    CLEAR_BBOX_2DV( tobj->current_contour->mins,
		    tobj->current_contour->maxs );

    tobj->current_contour->num_vertices = 0;
    tobj->current_contour->vertices =
	tobj->current_contour->last_vertex = NULL;

    tobj->current_contour->reflex_vertices = NULL;

 cleanup:
    MSG( 15, "  <- gluTessBeginContour( tobj:%p )\n", tobj );
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

    MSG( 15, "    -> gluTessVertex( tobj:%p coords:(%.2f,%.2f,%.2f) )\n", tobj, coords[0], coords[1], coords[2] );
    TESS_CHECK_ERRORS( tobj );

    if ( current == NULL )
    {
	/* gluTessBeginContour was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR2 );
	return;
    }

    tobj->num_vertices++;

    last_vertex = current->last_vertex;

    if ( last_vertex == NULL )
    {
	last_vertex = malloc( sizeof(tess_vertex_t) );
	if ( last_vertex == NULL ) {
	    tess_error_callback( tobj, GLU_OUT_OF_MEMORY );
	    return;
	}

	current->vertices = last_vertex;
	current->last_vertex = last_vertex;

	last_vertex->index = -1;
	last_vertex->data = vertex_data;

	last_vertex->coords[X] = coords[X];
	last_vertex->coords[Y] = coords[Y];
	last_vertex->coords[Z] = coords[Z];

	last_vertex->v[X] = 0.0;
	last_vertex->v[Y] = 0.0;

	last_vertex->edge_flag = GL_TRUE;

	last_vertex->side = 0.0;

	last_vertex->next = NULL;
	last_vertex->prev = NULL;

	current->num_vertices++;
    }
    else
    {
	tess_vertex_t	*vertex;

	vertex = malloc( sizeof(tess_vertex_t) );
	if ( vertex == NULL ) {
	    tess_error_callback( tobj, GLU_OUT_OF_MEMORY );
	    return;
	}

	vertex->index = -1;
	vertex->data = vertex_data;

	vertex->coords[X] = coords[X];
	vertex->coords[Y] = coords[Y];
	vertex->coords[Z] = coords[Z];

	vertex->v[X] = 0.0;
	vertex->v[Y] = 0.0;

	vertex->edge_flag = GL_TRUE;

	vertex->side = 0.0;

	vertex->next = NULL;
	vertex->prev = last_vertex;

	current->num_vertices++;

	last_vertex->next = vertex;
	current->last_vertex = vertex;
    }

 cleanup:
    MSG( 15, "    <- gluTessVertex( tobj:%p )\n", tobj );
    return;
}


/*****************************************************************************
 * gluTessEndContour
 *****************************************************************************/
void GLAPIENTRY gluTessEndContour( GLUtesselator *tobj )
{
    MSG( 15, "  -> gluTessEndContour( tobj:%p )\n", tobj );
    TESS_CHECK_ERRORS( tobj );

    if ( tobj->current_contour == NULL )
    {
	/* gluTessBeginContour was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR2 );
	return;
    }

    if ( tobj->current_contour->num_vertices > 0 ) {
	inspect_current_contour( tobj );
    } else {
	delete_contour( &tobj->current_contour );
    }

 cleanup:
    MSG( 15, "  <- gluTessEndContour( tobj:%p )\n", tobj );
    return;
}


/*****************************************************************************
 * gluTessEndPolygon
 *****************************************************************************/
void GLAPIENTRY gluTessEndPolygon( GLUtesselator *tobj )
{
    MSG( 15, "-> gluTessEndPolygon( tobj:%p )\n", tobj );
    TESS_CHECK_ERRORS( tobj );

    if ( tobj->current_contour != NULL )
    {
	/* gluTessBeginPolygon was not called. */
	tess_error_callback( tobj, GLU_TESS_ERROR1 );
	return;
    }
    TESS_CHECK_ERRORS( tobj );

    /*
     * Ensure we have at least one contour to tessellate.  If we have none,
     *  clean up and exit gracefully.
     */
    if ( tobj->num_contours == 0 ) {
	tess_cleanup( tobj );
	return;
    }

    /* Wrap the contour list. */

    tobj->last_contour->next = tobj->contours;
    tobj->contours->prev = tobj->last_contour;

    TESS_CHECK_ERRORS( tobj );

    /* Orient the contours correctly */
    orient_contours( tobj );

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
	fist_tessellation( tobj );
    }

 cleanup:
    delete_all_contours( tobj );
    MSG( 15, "<- gluTessEndPolygon( tobj:%p )\n", tobj );
}


/*****************************************************************************
 * gluTessCallback
 *****************************************************************************/
void GLAPIENTRY gluTessCallback( GLUtesselator *tobj, GLenum which,
				 void (GLCALLBACKP fn)() )
{
    switch ( which )
    {
	/* Register the begin callbacks. */
    case GLU_TESS_BEGIN:
	tobj->callbacks.begin = (void (GLCALLBACKPCAST)(GLenum)) fn;
	break;
    case GLU_TESS_BEGIN_DATA:
	tobj->callbacks.beginData = (void (GLCALLBACKPCAST)(GLenum, void *)) fn;
	break;

	/* Register the edge flag callbacks. */
    case GLU_TESS_EDGE_FLAG:
	tobj->callbacks.edgeFlag = (void (GLCALLBACKPCAST)(GLboolean)) fn;
	break;
    case GLU_TESS_EDGE_FLAG_DATA:
	tobj->callbacks.edgeFlagData = (void (GLCALLBACKPCAST)(GLboolean, void *)) fn;
	break;

	/* Register the vertex callbacks. */
    case GLU_TESS_VERTEX:
	tobj->callbacks.vertex = (void (GLCALLBACKPCAST)(void *)) fn;
	break;
    case GLU_TESS_VERTEX_DATA:
	tobj->callbacks.vertexData = (void (GLCALLBACKPCAST)(void *, void *)) fn;
	break;

	/* Register the end callbacks. */
    case GLU_TESS_END:
	tobj->callbacks.end = (void (GLCALLBACKPCAST)(void)) fn;
	break;
    case GLU_TESS_END_DATA:
	tobj->callbacks.endData = (void (GLCALLBACKPCAST)(void *)) fn;
	break;

	/* Register the error callbacks. */
    case GLU_TESS_ERROR:
	tobj->callbacks.error = (void (GLCALLBACKPCAST)(GLenum)) fn;
	break;
    case GLU_TESS_ERROR_DATA:
	tobj->callbacks.errorData = (void (GLCALLBACKPCAST)(GLenum, void *)) fn;
	break;

	/* Register the combine callbacks. */
    case GLU_TESS_COMBINE:
	tobj->callbacks.combine = (void (GLCALLBACKPCAST)(GLdouble[3], void *[4], GLfloat [4], void **)) fn;
	break;
    case GLU_TESS_COMBINE_DATA:
	tobj->callbacks.combineData = (void (GLCALLBACKPCAST)(GLdouble[3], void *[4], GLfloat [4], void **, void *)) fn;
	break;

    default:
	MSG( 1, "  gluTessCallback( tobj:%p which:%d ) invalid enum\n", tobj, which );
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
	MSG( 15, "   gluTessProperty( tobj:%p ) tolerance: %0.9f\n", tobj, value );
	tobj->tolerance = value;
	break;

    case GLU_TESS_WINDING_RULE:
	tobj->winding_rule = (GLenum) value;
	break;

    default:
	MSG( 1, "   gluTessProperty( tobj:%p which:%d ) invalid enum\n", tobj, which );
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
	MSG( 1, "   gluGetTessProperty( tobj:%p which:%d ) invalid enum\n", tobj, which );
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
    MSG( 15, "   gluTessNormal( tobj:%p n:(%.2f,%.2f,%.2f) )\n", tobj, x, y, z );

    ASSIGN_3V( tobj->plane.normal, x, y, z );
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
