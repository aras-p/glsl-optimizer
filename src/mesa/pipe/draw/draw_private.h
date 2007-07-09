/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef G_PRIM_H
#define G_PRIM_H

#include "glheader.h"
#include "sp_headers.h"

struct softpipe_context;

struct prim_stage *prim_setup( struct softpipe_context *context );
struct prim_stage *prim_unfilled( struct softpipe_context *context );
struct prim_stage *prim_twoside( struct softpipe_context *context );
struct prim_stage *prim_offset( struct softpipe_context *context );
struct prim_stage *prim_clip( struct softpipe_context *context );
struct prim_stage *prim_flatshade( struct softpipe_context *context );
struct prim_stage *prim_cull( struct softpipe_context *context );


/* Internal structs and helpers for the primitive clip/setup pipeline:
 */
struct prim_stage {
   struct softpipe_context *softpipe;

   struct prim_stage *next;

   struct vertex_header **tmp;
   GLuint nr_tmps;

   void (*begin)( struct prim_stage * );

   void (*point)( struct prim_stage *,
		  struct prim_header * );

   void (*line)( struct prim_stage *,
		 struct prim_header * );

   void (*tri)( struct prim_stage *,
		struct prim_header * );
   
   void (*end)( struct prim_stage * );
};



/* Get a writeable copy of a vertex:
 */
static INLINE struct vertex_header *
dup_vert( struct prim_stage *stage,
	  const struct vertex_header *vert,
	  GLuint idx )
{   
   struct vertex_header *tmp = stage->tmp[idx];
   memcpy(tmp, vert, stage->softpipe->prim.vertex_size );
   return tmp;
}

void prim_free_tmps( struct prim_stage *stage );
void prim_alloc_tmps( struct prim_stage *stage, GLuint nr );


#endif
