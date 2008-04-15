/*
 * Copyright 2008 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * Vertex fetch/store/convert code.  This functionality is used in two places:
 * 1. Vertex fetch/convert - to grab vertex data from incoming vertex
 *    arrays and convert to format needed by vertex shaders.
 * 2. Vertex store/emit - to convert simple float[][4] vertex attributes
 *    (which is the organization used throughout the draw/prim pipeline) to
 *    hardware-specific formats and emit into hardware vertex buffers.
 *
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */

#ifndef _TRANSLATE_H
#define _TRANSLATE_H


#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

struct translate_element 
{
   enum pipe_format input_format;
   unsigned input_buffer;
   unsigned input_offset;

   enum pipe_format output_format;
   unsigned output_offset;
};


struct translate {
   void (*destroy)( struct translate * );

   void (*set_buffer)( struct translate *,
		       unsigned i,
		       const void *ptr,
		       unsigned stride );

   void (*run_elts)( struct translate *,
		     const unsigned *elts,
		     unsigned count,
		     void *output_buffer);
};



struct translate *translate_sse2_create( unsigned output_stride,
					 const struct translate_element *elements,
					 unsigned nr_elements );

struct translate *translate_generic_create( unsigned output_stride,
					    const struct translate_element *elements,
					    unsigned nr_elements );


#endif
