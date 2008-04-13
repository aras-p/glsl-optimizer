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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#ifndef DRAW_PT_H
#define DRAW_PT_H

#include "pipe/p_compiler.h"

typedef unsigned (*pt_elt_func)( const void *elts, unsigned idx );

struct draw_pt_middle_end;
struct draw_context;

/* We use the top couple of bits in the vertex fetch index to convey a
 * little API information.  This limits the number of vertices we can
 * address to only 1 billion -- if that becomes a problem, these could
 * be moved out & passed separately.
 */
#define DRAW_PT_EDGEFLAG      (1<<30)
#define DRAW_PT_RESET_STIPPLE (1<<31)
#define DRAW_PT_FLAG_MASK     (3<<30)


/* The "front end" - prepare sets of fetch, draw elements for the
 * middle end.
 *
 * Currenly one version of this:
 *    - vcache - catchall implementation, decomposes to TRI/LINE/POINT prims
 * Later:
 *    - varray, varray_split
 *    - velement, velement_split
 *
 * Currenly only using the vcache version.
 */
struct draw_pt_front_end {
   void (*prepare)( struct draw_pt_front_end *,
                    unsigned prim,
                    struct draw_pt_middle_end * );

   void (*run)( struct draw_pt_front_end *,
                pt_elt_func elt_func,
                const void *elt_ptr,
                unsigned count );

   void (*finish)( struct draw_pt_front_end * );
   void (*destroy)( struct draw_pt_front_end * );
};


/* The "middle end" - prepares actual hardware vertices for the
 * hardware backend.
 *
 * Currently two versions of this:
 *     - fetch, vertex shade, cliptest, prim-pipeline
 *     - fetch, emit (ie passthrough)
 * Later:
 *     - fetch, vertex shade, cliptest, maybe-pipeline, maybe-emit
 *     - fetch, vertex shade, emit
 *
 * Currenly only using the passthrough version.
 */
struct draw_pt_middle_end {
   void (*prepare)( struct draw_pt_middle_end *,
                    unsigned prim );

   void (*run)( struct draw_pt_middle_end *,
                const unsigned *fetch_elts,
                unsigned fetch_count,
                const ushort *draw_elts,
                unsigned draw_count );

   void (*finish)( struct draw_pt_middle_end * );
   void (*destroy)( struct draw_pt_middle_end * );
};


/* The "back end" - supplied by the driver, defined in draw_vbuf.h.
 *
 * Not sure whether to wrap the prim pipeline up as an alternate
 * backend.  Would be a win for everything except pure passthrough
 * mode...  
 */
struct vbuf_render;


/* Helper functions.
 */
pt_elt_func draw_pt_elt_func( struct draw_context *draw );
const void *draw_pt_elt_ptr( struct draw_context *draw,
                             unsigned start );

/* Implementations:
 */
struct draw_pt_front_end *draw_pt_vcache( struct draw_context *draw );
struct draw_pt_middle_end *draw_pt_fetch_emit( struct draw_context *draw );
struct draw_pt_middle_end *draw_pt_fetch_pipeline( struct draw_context *draw );
struct draw_pt_middle_end *draw_pt_fetch_pipeline_or_emit(struct draw_context *draw);


#endif
