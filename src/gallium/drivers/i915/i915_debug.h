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

#ifndef I915_DEBUG_H
#define I915_DEBUG_H

#include <stdarg.h>

struct i915_context;

struct debug_stream 
{
   unsigned offset;		/* current gtt offset */
   char *ptr;		/* pointer to gtt offset zero */
   char *end;		/* pointer to gtt offset zero */
   unsigned print_addresses;
};


/* Internal functions
 */
void i915_disassemble_program(struct debug_stream *stream, 
			      const unsigned *program, unsigned sz);

void i915_print_ureg(const char *msg, unsigned ureg);


#define DEBUG_BATCH	 0x1
#define DEBUG_BLIT       0x2
#define DEBUG_BUFFER     0x4
#define DEBUG_CONSTANTS  0x8
#define DEBUG_CONTEXT    0x10
#define DEBUG_DRAW	 0x20
#define DEBUG_DYNAMIC	 0x40
#define DEBUG_FLUSH      0x80
#define DEBUG_MAP	 0x100
#define DEBUG_PROGRAM	 0x200
#define DEBUG_REGIONS    0x400
#define DEBUG_SAMPLER	 0x800
#define DEBUG_STATIC	 0x1000
#define DEBUG_SURFACE    0x2000
#define DEBUG_WINSYS     0x4000

#include "pipe/p_compiler.h"

#if defined(DEBUG) && defined(FILE_DEBUG_FLAG)

#include "util/u_simple_screen.h"

static INLINE void
I915_DBG(
   struct i915_context  *i915,
   const char           *fmt,
                        ... )
{
   if ((i915)->debug & FILE_DEBUG_FLAG) {
      va_list  args;

      va_start( args, fmt );
      debug_vprintf( fmt, args );
      va_end( args );
   }
}

#else

static INLINE void
I915_DBG(
   struct i915_context  *i915,
   const char           *fmt,
                        ... )
{
   (void) i915;
   (void) fmt;
}

#endif


struct intel_batchbuffer;

void i915_dump_batchbuffer( struct intel_batchbuffer *i915 );

void i915_debug_init( struct i915_context *i915 );


#endif
