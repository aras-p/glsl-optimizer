/* $XFree86$ */
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
 */

#include "imports.h" 
#include "simple_list.h" 
#include "t_vtx_api.h"

#if defined(USE_SSE_ASM)

/* Build specialized versions of the immediate calls on the fly for
 * the current state.  ???P4 SSE2 versions???
 */


static struct dynfn *makeSSENormal3fv( struct _vb *vb, int key )
{
   /* Requires P4 (sse2?)
    */
   static unsigned char temp[] = {
      0x8b, 0x44, 0x24, 0x04,          	/*  mov    0x4(%esp,1),%eax */
      0xba, 0x78, 0x56, 0x34, 0x12,   	/*  mov    $0x12345678,%edx */
      0xf3, 0x0f, 0x7e, 0x00,          	/*  movq   (%eax),%xmm0 */
      0x66, 0x0f, 0x6e, 0x48, 0x08,    	/*  movd   0x8(%eax),%xmm1 */
      0x66, 0x0f, 0xd6, 0x42, 0x0c,    	/*  movq   %xmm0,0xc(%edx) */
      0x66, 0x0f, 0x7e, 0x4a, 0x14,    	/*  movd   %xmm1,0x14(%edx) */
      0xc3,                   	        /*  ret     */
   };


   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   insert_at_head( &vb->dfn_cache.Normal3fv, dfn );
   dfn->key = key;

   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 5, 0x0, (int)vb->normalptr); 
   return dfn;
}

void _tnl_InitSSECodegen( struct dfn_generators *gen )
{
   /* Need to: 
    *    - check kernel sse support
    *    - check p4/sse2
    */
   (void) makeSSENormal3fv;
}


#else 

void _tnl_InitSSECodegen( struct dfn_generators *gen )
{
   (void) gen;
}

#endif




