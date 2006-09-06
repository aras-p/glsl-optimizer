/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#ifndef __BRW_EXEC_H__
#define __BRW_EXEC_H__

#include "mtypes.h"
#include "brw_attrib.h"
#include "brw_draw.h"


#define BRW_MAX_PRIM 64

/* Wierd implementation stuff:
 */
#define BRW_VERT_BUFFER_SIZE (1024*16)	/* dwords == 64k */
#define BRW_MAX_ATTR_CODEGEN 16 
#define ERROR_ATTRIB 16




struct brw_exec_eval1_map {
   struct gl_1d_map *map;
   GLuint sz;
};

struct brw_exec_eval2_map {
   struct gl_2d_map *map;
   GLuint sz;
};



struct brw_exec_copied_vtx {
   GLfloat buffer[BRW_ATTRIB_MAX * 4 * BRW_MAX_COPIED_VERTS];
   GLuint nr;
};


typedef void (*brw_attrfv_func)( const GLfloat * );


struct brw_exec_context
{
   GLcontext *ctx;   
   GLvertexformat vtxfmt;

   struct {
      struct gl_buffer_object *bufferobj;
      GLubyte *buffer_map;

      GLuint vertex_size;

      struct brw_draw_prim prim[BRW_MAX_PRIM];
      GLuint prim_count;

      GLfloat *vbptr;		     /* cursor, points into buffer */
      GLfloat vertex[BRW_ATTRIB_MAX*4]; /* current vertex */

      GLfloat *current[BRW_ATTRIB_MAX]; /* points into ctx->Current, ctx->Light.Material */
      GLfloat CurrentFloatEdgeFlag;

      GLuint vert_count;
      GLuint max_vert;
      struct brw_exec_copied_vtx copied;

      GLubyte attrsz[BRW_ATTRIB_MAX];
      GLubyte active_sz[BRW_ATTRIB_MAX];

      GLfloat *attrptr[BRW_ATTRIB_MAX]; 
      struct gl_client_array arrays[BRW_ATTRIB_MAX];
      const struct gl_client_array *inputs[BRW_ATTRIB_MAX];
   } vtx;

   
   struct {
      GLboolean recalculate_maps;
      struct brw_exec_eval1_map map1[BRW_ATTRIB_MAX];
      struct brw_exec_eval2_map map2[BRW_ATTRIB_MAX];
   } eval;

   struct {
      const struct gl_client_array *inputs[BRW_ATTRIB_MAX];

      struct gl_buffer_object *index_obj;
   } array;
};



/* External API:
 */
void brw_exec_init( GLcontext *ctx );
void brw_exec_destroy( GLcontext *ctx );
void brw_exec_invalidate_state( GLcontext *ctx, GLuint new_state );
void brw_exec_FlushVertices( GLcontext *ctx, GLuint flags );
void brw_exec_wakeup( GLcontext *ctx );


/* Internal functions:
 */
void brw_exec_array_init( struct brw_exec_context *exec );
void brw_exec_array_destroy( struct brw_exec_context *exec );


void brw_exec_vtx_init( struct brw_exec_context *exec );
void brw_exec_vtx_destroy( struct brw_exec_context *exec );
void brw_exec_vtx_flush( struct brw_exec_context *exec );
void brw_exec_vtx_wrap( struct brw_exec_context *exec );

void brw_exec_eval_update( struct brw_exec_context *exec );

void brw_exec_do_EvalCoord2f( struct brw_exec_context *exec, 
				     GLfloat u, GLfloat v );

void brw_exec_do_EvalCoord1f( struct brw_exec_context *exec,
				     GLfloat u);

#endif
