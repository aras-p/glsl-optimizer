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

#include "draw/draw_pt.h"
#include "draw/draw_private.h"

/* Neat get_elt func that also works for varrays drawing by encoding
 * the start value into a pointer.  
 */

static unsigned elt_uint( const void *elts, unsigned idx )
{
   return *(((const uint *)elts) + idx);
}

static unsigned elt_ushort( const void *elts, unsigned idx )
{
   return *(((const ushort *)elts) + idx);
}

static unsigned elt_ubyte( const void *elts, unsigned idx )
{
   return *(((const ubyte *)elts) + idx);
}

static unsigned elt_vert( const void *elts, unsigned idx )
{
   /* unsigned index is packed in the pointer */
   return (unsigned)(uintptr_t)elts + idx;
}

pt_elt_func draw_pt_elt_func( struct draw_context *draw )
{
   switch (draw->pt.user.eltSize) {
   case 0: return &elt_vert;
   case 1: return &elt_ubyte;
   case 2: return &elt_ushort; 
   case 4: return &elt_uint;
   default: return NULL;
   }
}     

const void *draw_pt_elt_ptr( struct draw_context *draw,
                             unsigned start )
{
   const char *elts = draw->pt.user.elts;

   switch (draw->pt.user.eltSize) {
   case 0: 
      return (const void *)(((const ubyte *)NULL) + start);
   case 1: 
      return (const void *)(((const ubyte *)elts) + start);
   case 2: 
      return (const void *)(((const ushort *)elts) + start);
   case 4: 
      return (const void *)(((const uint *)elts) + start);
   default:
      return NULL;
   }
}
