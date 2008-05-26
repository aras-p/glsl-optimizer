
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-2000  Brian Paul
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
 * This file is part of the polygon tesselation code contributed by
 * Bogdan Sikorski
 */


#ifndef TESS_H
#define TESS_H


#include "gluP.h"

#define EPSILON 1e-06		/* epsilon for double precision compares */

typedef enum
{
   OXY,
   OYZ,
   OXZ
}
projection_type;

typedef struct callbacks_str
{
   void (GLCALLBACK * begin) (GLenum mode);
   void (GLCALLBACK * edgeFlag) (GLboolean flag);
   void (GLCALLBACK * vertex) (GLvoid * v);
   void (GLCALLBACK * end) (void);
   void (GLCALLBACK * error) (GLenum err);
}
tess_callbacks;

typedef struct vertex_str
{
   void *data;
   GLdouble location[3];
   GLdouble x, y;
   GLboolean edge_flag;
   struct vertex_str *shadow_vertex;
   struct vertex_str *next, *previous;
}
tess_vertex;

typedef struct contour_str
{
   GLenum type;
   GLuint vertex_cnt;
   GLdouble area;
   GLenum orientation;
   struct vertex_str *vertices, *last_vertex;
   struct contour_str *next, *previous;
}
tess_contour;

typedef struct polygon_str
{
   GLuint vertex_cnt;
   GLdouble A, B, C, D;
   GLdouble area;
   GLenum orientation;
   struct vertex_str *vertices, *last_vertex;
}
tess_polygon;

struct GLUtesselator
{
   tess_contour *contours, *last_contour;
   GLuint contour_cnt;
   tess_callbacks callbacks;
   tess_polygon *current_polygon;
   GLenum error;
   GLdouble A, B, C, D;
   projection_type projection;
};


extern void tess_call_user_error(GLUtriangulatorObj *, GLenum);
extern void tess_test_polygon(GLUtriangulatorObj *);
extern void tess_find_contour_hierarchies(GLUtriangulatorObj *);
extern void tess_handle_holes(GLUtriangulatorObj *);
extern void tess_tesselate(GLUtriangulatorObj *);
extern void tess_tesselate_with_edge_flag(GLUtriangulatorObj *);


#endif
