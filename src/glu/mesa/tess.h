/* $Id: tess.h,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * Copyright (C) 1995-1998  Brian Paul
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
 * $Log: tess.h,v $
 * Revision 1.1  1999/08/19 00:55:42  jtg
 * Initial revision
 *
 * Revision 1.5  1999/02/27 13:55:31  brianp
 * fixed BeOS-related GLU typedef problems
 *
 * Revision 1.4  1999/01/03 03:23:15  brianp
 * now using GLAPIENTRY and GLCALLBACK keywords (Ted Jump)
 *
 * Revision 1.3  1997/10/29 02:02:20  brianp
 * various MS Windows compiler changes (David Bucciarelli, v20 3dfx driver)
 *
 * Revision 1.2  1997/05/24 13:30:58  brianp
 * added TESS_H multi-inclusion prevention test
 *
 * Revision 1.1  1996/09/27 01:19:39  brianp
 * Initial revision
 *
 */


/*
 * This file is part of the polygon tesselation code contributed by
 * Bogdan Sikorski
 */


#ifndef TESS_H
#define TESS_H


#include "gluP.h"

#define EPSILON 1e-06 /* epsilon for double precision compares */

typedef enum
{
	OXY,
	OYZ,
	OXZ
} projection_type;

typedef struct callbacks_str
{
	void (GLCALLBACK *begin)( GLenum mode );
	void (GLCALLBACK *edgeFlag)( GLboolean flag );
	void (GLCALLBACK *vertex)( GLvoid *v );
	void (GLCALLBACK *end)( void );
	void (GLCALLBACK *error)( GLenum err );
} tess_callbacks;

typedef struct vertex_str
{
	void				*data;
	GLdouble			location[3];
	GLdouble			x,y;
	GLboolean			edge_flag;
	struct vertex_str	*shadow_vertex;
	struct vertex_str	*next,*previous;
} tess_vertex;

typedef struct contour_str
{
	GLenum				type;
	GLuint				vertex_cnt;
	GLdouble			area;
	GLenum				orientation;
	struct vertex_str	*vertices,*last_vertex;
	struct contour_str	*next,*previous;
} tess_contour;

typedef struct polygon_str
{
	GLuint				vertex_cnt;
	GLdouble			A,B,C,D;
	GLdouble			area;
	GLenum				orientation;
	struct vertex_str	*vertices,*last_vertex;
} tess_polygon;

struct GLUtesselator
{
	tess_contour		*contours,*last_contour;
	GLuint				contour_cnt;
	tess_callbacks		callbacks;
	tess_polygon		*current_polygon;
	GLenum				error;
	GLdouble			A,B,C,D;
	projection_type		projection;
};


extern void tess_call_user_error(GLUtriangulatorObj *,GLenum);


#endif
