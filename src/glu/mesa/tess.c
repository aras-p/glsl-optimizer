/* $Id: tess.c,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

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
 * $Log: tess.c,v $
 * Revision 1.1  1999/08/19 00:55:42  jtg
 * Initial revision
 *
 * Revision 1.11  1999/02/27 13:55:31  brianp
 * fixed BeOS-related GLU typedef problems
 *
 * Revision 1.10  1999/01/03 03:23:15  brianp
 * now using GLAPIENTRY and GLCALLBACK keywords (Ted Jump)
 *
 * Revision 1.9  1998/06/01 01:10:29  brianp
 * small update for Next/OpenStep from Alexander Mai
 *
 * Revision 1.8  1998/02/04 00:27:58  brianp
 * cygnus changes from Stephane Rehel
 *
 * Revision 1.7  1998/01/16 03:35:26  brianp
 * fixed Windows compilation warnings (Theodore Jump)
 *
 * Revision 1.6  1997/09/17 01:51:48  brianp
 * changed glu*Callback() functions to match prototype in glu.h
 *
 * Revision 1.5  1997/07/24 01:28:44  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 02:29:38  brianp
 * added support for precompiled headers (PCH), inserted APIENTRY keyword
 *
 * Revision 1.3  1996/11/12 01:23:02  brianp
 * added test to prevent free(vertex) when vertex==NULL in delete_contours()
 *
 * Revision 1.2  1996/10/22 22:57:19  brianp
 * better error handling in gluBegin/EndPolygon() from Erich Eder
 *
 * Revision 1.1  1996/09/27 01:19:39  brianp
 * Initial revision
 *
 */


/*
 * This file is part of the polygon tesselation code contributed by
 * Bogdan Sikorski
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <math.h>
#include <stdlib.h>
#include "tess.h"
#endif


/*
 * This is ugly, but seems the easiest way to do things to make the
 * code work under YellowBox for Windows
 */
#if defined(OPENSTEP) && defined(GLCALLBACK)
#undef GLCALLBACK
#define GLCALLBACK
#endif


extern void tess_test_polygon(GLUtriangulatorObj *);
extern void tess_find_contour_hierarchies(GLUtriangulatorObj *);
extern void tess_handle_holes(GLUtriangulatorObj *);
extern void tess_tesselate(GLUtriangulatorObj *);
extern void tess_tesselate_with_edge_flag(GLUtriangulatorObj *);
static void delete_contours(GLUtriangulatorObj *);

#ifdef __CYGWIN32__
#define _CALLBACK
#else
#define _CALLBACK GLCALLBACK
#endif

void init_callbacks(tess_callbacks *callbacks)
{
   callbacks->begin = ( void (_CALLBACK*)(GLenum) ) 0;
   callbacks->edgeFlag = ( void (_CALLBACK*)(GLboolean) ) 0;
   callbacks->vertex = ( void (_CALLBACK*)(void*) ) 0;
   callbacks->end = ( void (_CALLBACK*)(void) ) 0;
   callbacks->error = ( void (_CALLBACK*)(GLenum) ) 0;
}

void tess_call_user_error(GLUtriangulatorObj *tobj, GLenum gluerr)
{
   if(tobj->error==GLU_NO_ERROR)
      tobj->error=gluerr;
   if(tobj->callbacks.error!=NULL)
      (tobj->callbacks.error)(gluerr);
}

GLUtriangulatorObj* GLAPIENTRY gluNewTess( void )
{
   GLUtriangulatorObj *tobj;
   tobj = (GLUtriangulatorObj *) malloc(sizeof(struct GLUtesselator));
   if (!tobj)
      return NULL;
   tobj->contours=tobj->last_contour=NULL;
   init_callbacks(&tobj->callbacks);
   tobj->error=GLU_NO_ERROR;
   tobj->current_polygon=NULL;
   tobj->contour_cnt=0;
   return tobj;
}


void GLAPIENTRY gluTessCallback( GLUtriangulatorObj *tobj, GLenum which,
                               void (GLCALLBACK *fn)() )
{
	switch(which)
	{
		case GLU_BEGIN:
			tobj->callbacks.begin = (void (_CALLBACK*)(GLenum)) fn;
			break;
		case GLU_EDGE_FLAG:
			tobj->callbacks.edgeFlag = (void (_CALLBACK*)(GLboolean)) fn;
			break;
		case GLU_VERTEX:
			tobj->callbacks.vertex = (void (_CALLBACK*)(void *)) fn;
			break;
		case GLU_END:
			tobj->callbacks.end = (void (_CALLBACK*)(void)) fn;
			break;
		case GLU_ERROR:
			tobj->callbacks.error = (void (_CALLBACK*)(GLenum)) fn;
			break;
		default:
			tobj->error=GLU_INVALID_ENUM;
			break;
	}
}



void GLAPIENTRY gluDeleteTess( GLUtriangulatorObj *tobj )
{
	if(tobj->error==GLU_NO_ERROR && tobj->contour_cnt)
		/* was gluEndPolygon called? */
		tess_call_user_error(tobj,GLU_TESS_ERROR1);
	/* delete all internal structures */
	delete_contours(tobj);
	free(tobj);
}


void GLAPIENTRY gluBeginPolygon( GLUtriangulatorObj *tobj )
{
/*
	if(tobj->error!=GLU_NO_ERROR)
		return;
*/
        tobj->error = GLU_NO_ERROR;
	if(tobj->current_polygon!=NULL)
	{
		/* gluEndPolygon was not called */
		tess_call_user_error(tobj,GLU_TESS_ERROR1);
		/* delete all internal structures */
		delete_contours(tobj);
	}
	else
	{
		if((tobj->current_polygon=
			(tess_polygon *)malloc(sizeof(tess_polygon)))==NULL)
		{
			tess_call_user_error(tobj,GLU_OUT_OF_MEMORY);
			return;
		}
		tobj->current_polygon->vertex_cnt=0;
		tobj->current_polygon->vertices=
			tobj->current_polygon->last_vertex=NULL;
	}
}


void GLAPIENTRY gluEndPolygon( GLUtriangulatorObj *tobj )
{
	/*tess_contour *contour_ptr;*/

	/* there was an error */
	if(tobj->error!=GLU_NO_ERROR) goto end;

	/* check if gluBeginPolygon was called */
	if(tobj->current_polygon==NULL)
	{
		tess_call_user_error(tobj,GLU_TESS_ERROR2);
		return;
	}
	tess_test_polygon(tobj);
	/* there was an error */
	if(tobj->error!=GLU_NO_ERROR) goto end;

	/* any real contours? */
	if(tobj->contour_cnt==0)
	{
		/* delete all internal structures */
		delete_contours(tobj);
		return;
	}
	tess_find_contour_hierarchies(tobj);
	/* there was an error */
	if(tobj->error!=GLU_NO_ERROR) goto end;

	tess_handle_holes(tobj);
	/* there was an error */
	if(tobj->error!=GLU_NO_ERROR) goto end;

	/* if no callbacks, nothing to do */
	if(tobj->callbacks.begin!=NULL && tobj->callbacks.vertex!=NULL &&
		tobj->callbacks.end!=NULL)
	{
		if(tobj->callbacks.edgeFlag==NULL)
			tess_tesselate(tobj);
		else
			tess_tesselate_with_edge_flag(tobj);
	}

end:
	/* delete all internal structures */
	delete_contours(tobj);
}


void GLAPIENTRY gluNextContour( GLUtriangulatorObj *tobj, GLenum type )
{
	if(tobj->error!=GLU_NO_ERROR)
		return;
	if(tobj->current_polygon==NULL)
	{
		tess_call_user_error(tobj,GLU_TESS_ERROR2);
		return;
	}
	/* first contour? */
	if(tobj->current_polygon->vertex_cnt)
		tess_test_polygon(tobj);
}


void GLAPIENTRY gluTessVertex( GLUtriangulatorObj *tobj, GLdouble v[3], void *data )
{
	tess_polygon *polygon=tobj->current_polygon;
	tess_vertex *last_vertex_ptr;

	if(tobj->error!=GLU_NO_ERROR)
		return;
	if(polygon==NULL)
	{
		tess_call_user_error(tobj,GLU_TESS_ERROR2);
		return;
	}
	last_vertex_ptr=polygon->last_vertex;
	if(last_vertex_ptr==NULL)
	{
		if((last_vertex_ptr=(tess_vertex *)
			malloc(sizeof(tess_vertex)))==NULL)
		{
			tess_call_user_error(tobj,GLU_OUT_OF_MEMORY);
			return;
		}
		polygon->vertices=last_vertex_ptr;
		polygon->last_vertex=last_vertex_ptr;
		last_vertex_ptr->data=data;
		last_vertex_ptr->location[0]=v[0];
		last_vertex_ptr->location[1]=v[1];
		last_vertex_ptr->location[2]=v[2];
		last_vertex_ptr->next=NULL;
		last_vertex_ptr->previous=NULL;
		++(polygon->vertex_cnt);
	}
	else
	{
		tess_vertex *vertex_ptr;

		/* same point twice? */
		if(fabs(last_vertex_ptr->location[0]-v[0]) < EPSILON &&
			fabs(last_vertex_ptr->location[1]-v[1]) < EPSILON &&
			fabs(last_vertex_ptr->location[2]-v[2]) < EPSILON)
		{
			tess_call_user_error(tobj,GLU_TESS_ERROR6);
			return;
		}
		if((vertex_ptr=(tess_vertex *)
			malloc(sizeof(tess_vertex)))==NULL)
		{
			tess_call_user_error(tobj,GLU_OUT_OF_MEMORY);
			return;
		}
		vertex_ptr->data=data;
		vertex_ptr->location[0]=v[0];
		vertex_ptr->location[1]=v[1];
		vertex_ptr->location[2]=v[2];
		vertex_ptr->next=NULL;
		vertex_ptr->previous=last_vertex_ptr;
		++(polygon->vertex_cnt);
		last_vertex_ptr->next=vertex_ptr;
		polygon->last_vertex=vertex_ptr;
	}
}


static void delete_contours(GLUtriangulatorObj *tobj)
{
	tess_polygon *polygon=tobj->current_polygon;
	tess_contour *contour,*contour_tmp;
	tess_vertex *vertex,*vertex_tmp;

	/* remove current_polygon list - if exists due to detected error */
	if(polygon!=NULL)
	{
		if (polygon->vertices)
		{
			for(vertex=polygon->vertices;vertex!=polygon->last_vertex;)
			{
				vertex_tmp=vertex->next;
				free(vertex);
				vertex=vertex_tmp;
			}
			free(vertex);
		}
		free(polygon);
		tobj->current_polygon=NULL;
	}
	/* remove all contour data */
	for(contour=tobj->contours;contour!=NULL;)
	{
		for(vertex=contour->vertices;vertex!=contour->last_vertex;)
		{
			vertex_tmp=vertex->next;
			free(vertex);
			vertex=vertex_tmp;
		}
		free(vertex);
		contour_tmp=contour->next;
		free(contour);
		contour=contour_tmp;
	}
	tobj->contours=tobj->last_contour=NULL;
	tobj->contour_cnt=0;
}



