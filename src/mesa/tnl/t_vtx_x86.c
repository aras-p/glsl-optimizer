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

#include <stdio.h>
#include <assert.h>
#include "mem.h" 
#include "mmath.h" 
#include "simple_list.h" 
#include "tnl_vtxfmt.h"

#if defined(USE_X86_ASM)


struct dynfn *tnl_makeX86Vertex2f( TNLcontext *tnl, int key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (RADEON_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   switch (tnl->vertex_size) {
   default: {
      /* Repz convenient as it's possible to emit code for any size
       * vertex with little tweaking.  Might as well read vertsize
       * though, and have only one of these.
       */
      static  char temp[] = {
	 0x57,                     	/* push   %edi */
	 0x56,                     	/* push   %esi */
	 0xbe, 0, 0, 0, 0,         	/* mov    $VERTEX+2,%esi */
	 0x8b, 0x3d, 0, 0, 0, 0,    	/* mov    DMAPTR,%edi */
	 0x8b, 0x44, 0x24, 0x0c,        /* mov    0x0c(%esp,1),%eax */
	 0x8b, 0x54, 0x24, 0x10,        /* mov    0x10(%esp,1),%edx */
	 0x89, 0x07,                	/* mov    %eax,(%edi) */
	 0x89, 0x57, 0x04,             	/* mov    %edx,0x4(%edi) */
	 0x83, 0xc7, 0x08,             	/* add    $0x8,%edi */
	 0xb9, 0, 0, 0, 0,         	/* mov    $VERTSIZE-2,%ecx */
	 0xf3, 0xa5,                	/* repz movsl %ds:(%esi),%es:(%edi)*/
	 0xa1, 0, 0, 0, 0,         	/* mov    COUNTER,%eax */
	 0x89, 0x3d, 0, 0, 0, 0,    	/* mov    %edi,DMAPTR */
	 0x48,                     	/* dec    %eax */
	 0xa3, 0, 0, 0, 0,          	/* mov    %eax,COUNTER */
	 0x5e,                     	/* pop    %esi */
	 0x5f,                     	/* pop    %edi */
	 0x74, 0x01,                	/* je     +1 */
	 0xc3,                     	/* ret     */
	 0xff, 0x25, 0, 0, 0, 0    	/* jmp    NOTIFY */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 3, 0x0, (int)&tnl->vertex[2]);
      FIXUP(dfn->code, 9, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 37, 0x0, tnl->vertex_size-2);
      FIXUP(dfn->code, 44, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 50, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 56, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 67, 0x0, (int)&tnl->notify);
   break;
   }
   }

   insert_at_head( &tnl->dfn_cache.Vertex3f, dfn );
   dfn->key = key;
   return dfn;
}

/* Build specialized versions of the immediate calls on the fly for
 * the current state.  Generic x86 versions.
 */

struct dynfn *tnl_makeX86Vertex3f( TNLcontext *tnl, int key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (RADEON_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   switch (tnl->vertex_size) {
   case 4: {
      static  char temp[] = {
	 0x8b, 0x0d, 0,0,0,0,    	/* mov    DMAPTR,%ecx */
	 0x8b, 0x44, 0x24, 0x04,       	/* mov    0x4(%esp,1),%eax */
	 0x8b, 0x54, 0x24, 0x08,       	/* mov    0x8(%esp,1),%edx */
	 0x89, 0x01,                	/* mov    %eax,(%ecx) */
	 0x89, 0x51, 0x04,             	/* mov    %edx,0x4(%ecx) */
	 0x8b, 0x44, 0x24, 0x0c,       	/* mov    0xc(%esp,1),%eax */
	 0x8b, 0x15, 0,0,0,0,    	/* mov    VERTEX[3],%edx */
	 0x89, 0x41, 0x08,             	/* mov    %eax,0x8(%ecx) */
	 0x89, 0x51, 0x0c,             	/* mov    %edx,0xc(%ecx) */
	 0xa1, 0, 0, 0, 0,       	/* mov    COUNTER,%eax */
	 0x83, 0xc1, 0x10,             	/* add    $0x10,%ecx */
	 0x48,                   	/* dec    %eax */
	 0x89, 0x0d, 0,0,0,0,    	/* mov    %ecx,DMAPTR */
	 0xa3, 0, 0, 0, 0,      	/* mov    %eax,COUNTER */
	 0x74, 0x01,                	/* je     +1 */
	 0xc3,                   	/* ret     */
	 0xff, 0x25, 0,0,0,0    	/* jmp    *NOTIFY */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 2, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 25, 0x0, (int)&tnl->vertex[3]);
      FIXUP(dfn->code, 36, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 46, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 51, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 60, 0x0, (int)&tnl->notify);
      break;
   }
   case 6: {
      static  char temp[] = {
	 0x57,                   	/* push   %edi */
	 0x8b, 0x3d, 0, 0, 0, 0,    	/* mov    DMAPTR,%edi */
	 0x8b, 0x44, 0x24, 0x8,       	/* mov    0x8(%esp,1),%eax */
	 0x8b, 0x54, 0x24, 0xc,       	/* mov    0xc(%esp,1),%edx */
	 0x8b, 0x4c, 0x24, 0x10,       	/* mov    0x10(%esp,1),%ecx */
	 0x89, 0x07,                	/* mov    %eax,(%edi) */
	 0x89, 0x57, 0x04,             	/* mov    %edx,0x4(%edi) */
	 0x89, 0x4f, 0x08,             	/* mov    %ecx,0x8(%edi) */
	 0xa1, 0, 0, 0, 0,       	/* mov    VERTEX[3],%eax */
	 0x8b, 0x15, 0, 0, 0, 0,    	/* mov    VERTEX[4],%edx */
	 0x8b, 0x0d, 0, 0, 0, 0,    	/* mov    VERTEX[5],%ecx */
	 0x89, 0x47, 0x0c,             	/* mov    %eax,0xc(%edi) */
	 0x89, 0x57, 0x10,             	/* mov    %edx,0x10(%edi) */
	 0x89, 0x4f, 0x14,             	/* mov    %ecx,0x14(%edi) */
	 0x83, 0xc7, 0x18,             	/* add    $0x18,%edi */
	 0xa1, 0, 0, 0, 0,       	/* mov    COUNTER,%eax */
	 0x89, 0x3d, 0, 0, 0, 0,    	/* mov    %edi,DMAPTR */
	 0x48,                   	/* dec    %eax */
	 0x5f,                   	/* pop    %edi */
	 0xa3, 0, 0, 0, 0,       	/* mov    %eax,COUNTER */
	 0x74, 0x01,                	/* je     +1 */
	 0xc3,                   	/* ret     */
	 0xff, 0x25, 0,0,0,0,    	/* jmp    *NOTIFY */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 3, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 28, 0x0, (int)&tnl->vertex[3]);
      FIXUP(dfn->code, 34, 0x0, (int)&tnl->vertex[4]);
      FIXUP(dfn->code, 40, 0x0, (int)&tnl->vertex[5]);
      FIXUP(dfn->code, 57, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 63, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 70, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 79, 0x0, (int)&tnl->notify);
      break;
   }
   default: {
      /* Repz convenient as it's possible to emit code for any size
       * vertex with little tweaking.  Might as well read vertsize
       * though, and have only one of these.
       */
      static  char temp[] = {
	 0x57,                     	/* push   %edi */
	 0x56,                     	/* push   %esi */
	 0xbe, 0, 0, 0, 0,         	/* mov    $VERTEX+3,%esi */
	 0x8b, 0x3d, 0, 0, 0, 0,    	/* mov    DMAPTR,%edi */
	 0x8b, 0x44, 0x24, 0x0c,        /* mov    0x0c(%esp,1),%eax */
	 0x8b, 0x54, 0x24, 0x10,        /* mov    0x10(%esp,1),%edx */
	 0x8b, 0x4c, 0x24, 0x14,        /* mov    0x14(%esp,1),%ecx */
	 0x89, 0x07,                	/* mov    %eax,(%edi) */
	 0x89, 0x57, 0x04,             	/* mov    %edx,0x4(%edi) */
	 0x89, 0x4f, 0x08,             	/* mov    %ecx,0x8(%edi) */
	 0x83, 0xc7, 0x0c,             	/* add    $0xc,%edi */
	 0xb9, 0, 0, 0, 0,         	/* mov    $VERTSIZE-3,%ecx */
	 0xf3, 0xa5,                	/* repz movsl %ds:(%esi),%es:(%edi)*/
	 0xa1, 0, 0, 0, 0,         	/* mov    COUNTER,%eax */
	 0x89, 0x3d, 0, 0, 0, 0,    	/* mov    %edi,DMAPTR */
	 0x48,                     	/* dec    %eax */
	 0xa3, 0, 0, 0, 0,          	/* mov    %eax,COUNTER */
	 0x5e,                     	/* pop    %esi */
	 0x5f,                     	/* pop    %edi */
	 0x74, 0x01,                	/* je     +1 */
	 0xc3,                     	/* ret     */
	 0xff, 0x25, 0, 0, 0, 0    	/* jmp    NOTIFY */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 3, 0x0, (int)&tnl->vertex[3]);
      FIXUP(dfn->code, 9, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 37, 0x0, tnl->vertex_size-3);
      FIXUP(dfn->code, 44, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 50, 0x0, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 56, 0x0, (int)&tnl->counter);
      FIXUP(dfn->code, 67, 0x0, (int)&tnl->notify);
   break;
   }
   }

   insert_at_head( &tnl->dfn_cache.Vertex3f, dfn );
   dfn->key = key;
   return dfn;
}



struct dynfn *tnl_makeX86Vertex3fv( TNLcontext *tnl, int key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   switch (tnl->vertex_size) {
   case 6: {
      static  char temp[] = {
	 0xa1, 0x00, 0x00, 0, 0,       	/* mov    0x0,%eax */
	 0x8b, 0x4c, 0x24, 0x04,       	/* mov    0x4(%esp,1),%ecx */
	 0x8b, 0x11,                	/* mov    (%ecx),%edx */
	 0x89, 0x10,                	/* mov    %edx,(%eax) */
	 0x8b, 0x51, 0x04,             	/* mov    0x4(%ecx),%edx */
	 0x8b, 0x49, 0x08,             	/* mov    0x8(%ecx),%ecx */
	 0x89, 0x50, 0x04,             	/* mov    %edx,0x4(%eax) */
	 0x89, 0x48, 0x08,             	/* mov    %ecx,0x8(%eax) */
	 0x8b, 0x15, 0x1c, 0, 0, 0,    	/* mov    0x1c,%edx */
	 0x8b, 0x0d, 0x20, 0, 0, 0,    	/* mov    0x20,%ecx */
	 0x89, 0x50, 0x0c,             	/* mov    %edx,0xc(%eax) */
	 0x89, 0x48, 0x10,             	/* mov    %ecx,0x10(%eax) */
	 0x8b, 0x15, 0x24, 0, 0, 0,    	/* mov    0x24,%edx */
	 0x89, 0x50, 0x14,             	/* mov    %edx,0x14(%eax) */
	 0x83, 0xc0, 0x18,             	/* add    $0x18,%eax */
	 0xa3, 0x00, 0x00, 0, 0,       	/* mov    %eax,0x0 */
	 0xa1, 0x04, 0x00, 0, 0,       	/* mov    0x4,%eax */
	 0x48,                   	/* dec    %eax */
	 0xa3, 0x04, 0x00, 0, 0,       	/* mov    %eax,0x4 */
	 0x74, 0x01,                	/* je     2a4 <.f11> */
	 0xc3,                   	/* ret     */
	 0xff, 0x25, 0x08, 0, 0, 0,    	/* jmp    *0x8 */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 1, 0x00000000, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 27, 0x0000001c, (int)&tnl->vertex[3]);
      FIXUP(dfn->code, 33, 0x00000020, (int)&tnl->vertex[4]);
      FIXUP(dfn->code, 45, 0x00000024, (int)&tnl->vertex[5]);
      FIXUP(dfn->code, 56, 0x00000000, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 61, 0x00000004, (int)&tnl->counter);
      FIXUP(dfn->code, 67, 0x00000004, (int)&tnl->counter);
      FIXUP(dfn->code, 76, 0x00000008, (int)&tnl->notify);
      break;
   }
   

   case 8: {
      static  char temp[] = {
	 0xa1, 0x00, 0x00, 0, 0,       	/* mov    0x0,%eax */
	 0x8b, 0x4c, 0x24, 0x04,       	/* mov    0x4(%esp,1),%ecx */
	 0x8b, 0x11,                	/* mov    (%ecx),%edx */
	 0x89, 0x10,                	/* mov    %edx,(%eax) */
	 0x8b, 0x51, 0x04,             	/* mov    0x4(%ecx),%edx */
	 0x8b, 0x49, 0x08,             	/* mov    0x8(%ecx),%ecx */
	 0x89, 0x50, 0x04,             	/* mov    %edx,0x4(%eax) */
	 0x89, 0x48, 0x08,             	/* mov    %ecx,0x8(%eax) */
	 0x8b, 0x15, 0x1c, 0, 0, 0,    	/* mov    0x1c,%edx */
	 0x8b, 0x0d, 0x20, 0, 0, 0,    	/* mov    0x20,%ecx */
	 0x89, 0x50, 0x0c,             	/* mov    %edx,0xc(%eax) */
	 0x89, 0x48, 0x10,             	/* mov    %ecx,0x10(%eax) */
	 0x8b, 0x15, 0x1c, 0, 0, 0,    	/* mov    0x1c,%edx */
	 0x8b, 0x0d, 0x20, 0, 0, 0,    	/* mov    0x20,%ecx */
	 0x89, 0x50, 0x14,             	/* mov    %edx,0x14(%eax) */
	 0x89, 0x48, 0x18,             	/* mov    %ecx,0x18(%eax) */
	 0x8b, 0x15, 0x24, 0, 0, 0,    	/* mov    0x24,%edx */
	 0x89, 0x50, 0x1c,             	/* mov    %edx,0x1c(%eax) */
	 0x83, 0xc0, 0x20,             	/* add    $0x20,%eax */
	 0xa3, 0x00, 0x00, 0, 0,       	/* mov    %eax,0x0 */
	 0xa1, 0x04, 0x00, 0, 0,       	/* mov    0x4,%eax */
	 0x48,                   	/* dec    %eax */
	 0xa3, 0x04, 0x00, 0, 0,       	/* mov    %eax,0x4 */
	 0x74, 0x01,                	/* je     2a4 <.f11> */
	 0xc3,                   	/* ret     */
	 0xff, 0x25, 0x08, 0, 0, 0,    	/* jmp    *0x8 */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 1, 0x00000000, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 27, 0x0000001c, (int)&tnl->vertex[3]);
      FIXUP(dfn->code, 33, 0x00000020, (int)&tnl->vertex[4]);
      FIXUP(dfn->code, 45, 0x0000001c, (int)&tnl->vertex[5]);
      FIXUP(dfn->code, 51, 0x00000020, (int)&tnl->vertex[6]);
      FIXUP(dfn->code, 63, 0x00000024, (int)&tnl->vertex[7]);
      FIXUP(dfn->code, 74, 0x00000000, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 79, 0x00000004, (int)&tnl->counter);
      FIXUP(dfn->code, 85, 0x00000004, (int)&tnl->counter);
      FIXUP(dfn->code, 94, 0x00000008, (int)&tnl->notify);
      break;
   }
   


   default: {
      /* Repz convenient as it's possible to emit code for any size
       * vertex with little tweaking.  Might as well read vertsize
       * though, and have only one of these.
       */
      static  char temp[] = {
	 0x8b, 0x54, 0x24, 0x04,        /* mov    0x4(%esp,1),%edx */
	 0x57,                   	/* push   %edi */
	 0x56,                   	/* push   %esi */
	 0x8b, 0x3d, 1,1,1,1,    	/* mov    DMAPTR,%edi */
	 0x8b, 0x02,                	/* mov    (%edx),%eax */
	 0x8b, 0x4a, 0x04,             	/* mov    0x4(%edx),%ecx */
	 0x8b, 0x72, 0x08,             	/* mov    0x8(%edx),%esi */
	 0x89, 0x07,                	/* mov    %eax,(%edi) */
	 0x89, 0x4f, 0x04,             	/* mov    %ecx,0x4(%edi) */
	 0x89, 0x77, 0x08,             	/* mov    %esi,0x8(%edi) */
	 0x83, 0xc7, 0x0c,             	/* add    $0xc,%edi */
	 0xb9, 0x06, 0x00, 0x00, 0x00,  /* mov    $VERTSIZE-3,%ecx */
	 0xbe, 0x58, 0x00, 0x00, 0x00,	/* mov    $VERTEX[3],%esi */
	 0xf3, 0xa5,                	/* repz movsl %ds:(%esi),%es:(%edi)*/
	 0x89, 0x3d, 1, 1, 1, 1,    	/* mov    %edi,DMAPTR */
	 0xa1, 2, 2, 2, 2,       	/* mov    COUNTER,%eax */
	 0x5e,                   	/* pop    %esi */
	 0x5f,                   	/* pop    %edi */
	 0x48,                   	/* dec    %eax */
	 0xa3, 2, 2, 2, 2,       	/* mov    %eax,COUNTER */
	 0x74, 0x01,                	/* je     +1 */
	 0xc3,                     	/* ret     */
	 0xff, 0x25, 0, 0, 0, 0    	/* jmp    NOTIFY */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 8, 0x01010101, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 32, 0x00000006, tnl->vertex_size-3);
      FIXUP(dfn->code, 37, 0x00000058, (int)&tnl->vertex[3]);
      FIXUP(dfn->code, 45, 0x01010101, (int)&tnl->dmaptr);
      FIXUP(dfn->code, 50, 0x02020202, (int)&tnl->counter);
      FIXUP(dfn->code, 58, 0x02020202, (int)&tnl->counter);
      FIXUP(dfn->code, 67, 0x0, (int)&tnl->notify);
   break;
   }
   }

   insert_at_head( &tnl->dfn_cache.Vertex3fv, dfn );
   dfn->key = key;
   return dfn;
}


struct dynfn *tnl_makeX86Attr4fv( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0x8b, 0x44, 0x24, 0x04,          	/* mov    0x4(%esp,1),%eax */
      0xba, 0, 0, 0, 0,         	/* mov    $DEST,%edx */
      0x8b, 0x08,                	/* mov    (%eax),%ecx */
      0x89, 0x0a,                	/* mov    %ecx,(%edx) */
      0x8b, 0x48, 0x04,             	/* mov    0x4(%eax),%ecx */
      0x89, 0x4a, 0x04,             	/* mov    %ecx,0x4(%edx) */
      0x8b, 0x48, 0x08,             	/* mov    0x8(%eax),%ecx */
      0x89, 0x4a, 0x08,             	/* mov    %ecx,0x8(%edx) */
      0x8b, 0x48, 0x0a,             	/* mov    0xa(%eax),%ecx */
      0x89, 0x4a, 0x0a,             	/* mov    %ecx,0xa(%edx) */
      0xc3,                     	/* ret    */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.Normal3fv, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 5, 0x0, (int)tnl->normalptr); 
   return dfn;
}

struct dynfn *tnl_makeX86Attr4f( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0xba, 0x78, 0x56, 0x34, 0x12,    	/*  mov    $DEST,%edx */
      0x8b, 0x44, 0x24, 0x04,          	/*  mov    0x4(%esp,1),%eax */
      0x89, 0x02,                	/*  mov    %eax,(%edx) */
      0x8b, 0x44, 0x24, 0x08,          	/*  mov    0x8(%esp,1),%eax */
      0x89, 0x42, 0x04,             	/*  mov    %eax,0x4(%edx) */
      0x8b, 0x44, 0x24, 0x0c,          	/*  mov    0xc(%esp,1),%eax */
      0x89, 0x42, 0x08,             	/*  mov    %eax,0x8(%edx) */
      0x8b, 0x44, 0x24, 0x10,          	/*  mov    0x10(%esp,1),%eax */
      0x89, 0x42, 0x0a,             	/*  mov    %eax,0xa(%edx) */
      0xc3,                     	/*  ret     */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.Normal3f, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 1, 0x12345678, (int)tnl->normalptr); 
   return dfn;
}


struct dynfn *tnl_makeX86Attr3fv( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0x8b, 0x44, 0x24, 0x04,          	/* mov    0x4(%esp,1),%eax */
      0xba, 0, 0, 0, 0,         	/* mov    $DEST,%edx */
      0x8b, 0x08,                	/* mov    (%eax),%ecx */
      0x89, 0x0a,                	/* mov    %ecx,(%edx) */
      0x8b, 0x48, 0x04,             	/* mov    0x4(%eax),%ecx */
      0x89, 0x4a, 0x04,             	/* mov    %ecx,0x4(%edx) */
      0x8b, 0x48, 0x08,             	/* mov    0x8(%eax),%ecx */
      0x89, 0x4a, 0x08,             	/* mov    %ecx,0x8(%edx) */
      0xc3,                     	/* ret    */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.Normal3fv, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 5, 0x0, (int)tnl->normalptr); 
   return dfn;
}

struct dynfn *tnl_makeX86Attr3f( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0xba, 0x78, 0x56, 0x34, 0x12,    	/*  mov    $DEST,%edx */
      0x8b, 0x44, 0x24, 0x04,          	/*  mov    0x4(%esp,1),%eax */
      0x89, 0x02,                	/*  mov    %eax,(%edx) */
      0x8b, 0x44, 0x24, 0x08,          	/*  mov    0x8(%esp,1),%eax */
      0x89, 0x42, 0x04,             	/*  mov    %eax,0x4(%edx) */
      0x8b, 0x44, 0x24, 0x0c,          	/*  mov    0xc(%esp,1),%eax */
      0x89, 0x42, 0x08,             	/*  mov    %eax,0x8(%edx) */
      0xc3,                     	/*  ret     */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.Normal3f, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 1, 0x12345678, (int)tnl->normalptr); 
   return dfn;
}

struct dynfn *tnl_makeX86Attr4ubv( TNLcontext *tnl, int key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   insert_at_head( &tnl->dfn_cache.Color4ubv, dfn );
   dfn->key = key;

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   if (key & TNL_CP_VC_FRMT_PKCOLOR) {
      static  char temp[] = {
	 0x8b, 0x44, 0x24, 0x04,        /*  mov    0x4(%esp,1),%eax */
	 0xba, 0x78, 0x56, 0x34, 0x12,  /*  mov    $DEST,%edx */
	 0x8b, 0x00,                	/*  mov    (%eax),%eax */
	 0x89, 0x02,               	/*  mov    %eax,(%edx) */
	 0xc3,                     	/*  ret     */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 5, 0x12345678, (int)tnl->ubytecolorptr); 
      return dfn;
   } 
   else {
      static  char temp[] = {
	 0x53,					/* push   %ebx */
	 0xba, 0x00, 0x00, 0x00, 0x00,       	/* mov    $0x0,%edx */
	 0x31, 0xc0,                    	/* xor    %eax,%eax */
	 0x31, 0xc9,                    	/* xor    %ecx,%ecx */
	 0x8b, 0x5c, 0x24, 0x08,		/* mov	  0x8(%esp,1), %ebx */
	 0x8b, 0x1b,				/* mov    (%ebx), %ebx */
	 0x88, 0xd8,				/* mov    %bl, %al */
	 0x88, 0xf9, 	         		/* mov    %bh, %cl */
	 0x8b, 0x04, 0x82,              	/* mov    (%edx,%eax,4),%eax */
	 0x8b, 0x0c, 0x8a,              	/* mov    (%edx,%ecx,4),%ecx */
	 0xa3, 0xaf, 0xbe, 0xad, 0xde,       	/* mov    %eax,0xdeadbeaf */
	 0x89, 0x0d, 0xaf, 0xbe, 0xad, 0xde,   	/* mov    %ecx,0xdeadbeaf */
	 0x31, 0xc0,                    	/* xor    %eax,%eax */
	 0x31, 0xc9,                    	/* xor    %ecx,%ecx */
	 0xc1, 0xeb, 0x10,			/* shr    $0x10, %ebx */
	 0x88, 0xd8,				/* mov    %bl, %al */
	 0x88, 0xf9, 	         		/* mov    %bh, %cl */
	 0x8b, 0x04, 0x82,               	/* mov    (%edx,%eax,4),%eax */
	 0x8b, 0x0c, 0x8a,               	/* mov    (%edx,%ecx,4),%ecx */
	 0xa3, 0xaf, 0xbe, 0xad, 0xde,       	/* mov    %eax,0xdeadbeaf */
	 0x89, 0x0d, 0xaf, 0xbe, 0xad, 0xde,   	/* mov    %ecx,0xdeadbeaf */
	 0x5b,                      		/* pop    %ebx */
	 0xc3,                          	/* ret     */
      };

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 2, 0x00000000, (int)_mesa_ubyte_to_float_color_tab); 
      FIXUP(dfn->code, 27, 0xdeadbeaf, (int)tnl->floatcolorptr); 
      FIXUP(dfn->code, 33, 0xdeadbeaf, (int)tnl->floatcolorptr+4); 
      FIXUP(dfn->code, 55, 0xdeadbeaf, (int)tnl->floatcolorptr+8); 
      FIXUP(dfn->code, 61, 0xdeadbeaf, (int)tnl->floatcolorptr+12); 
      return dfn;
   }
}

struct dynfn *tnl_makeX86Attr4ub( TNLcontext *tnl, int key )
{
   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   if (key & TNL_CP_VC_FRMT_PKCOLOR) {
      /* XXX push/pop */
      static  char temp[] = {
	 0x53,                     	/* push   %ebx */
	 0x8b, 0x44, 0x24, 0x08,          	/* mov    0x8(%esp,1),%eax */
	 0x8b, 0x54, 0x24, 0x0c,          	/* mov    0xc(%esp,1),%edx */
	 0x8b, 0x4c, 0x24, 0x10,          	/* mov    0x10(%esp,1),%ecx */
	 0x8b, 0x5c, 0x24, 0x14,          	/* mov    0x14(%esp,1),%ebx */
	 0xa2, 0, 0, 0, 0,		/* mov    %al,DEST */
	 0x88, 0x15, 0, 0, 0, 0,	/* mov    %dl,DEST+1 */
	 0x88, 0x0d, 0, 0, 0, 0,	/* mov    %cl,DEST+2 */
	 0x88, 0x1d, 0, 0, 0, 0,	/* mov    %bl,DEST+3 */
	 0x5b,                      	/* pop    %ebx */
	 0xc3,                     	/* ret     */
      };

      struct dynfn *dfn = MALLOC_STRUCT( dynfn );
      insert_at_head( &tnl->dfn_cache.Color4ub, dfn );
      dfn->key = key;

      dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
      memcpy (dfn->code, temp, sizeof(temp));
      FIXUP(dfn->code, 18, 0x0, (int)tnl->ubytecolorptr); 
      FIXUP(dfn->code, 24, 0x0, (int)tnl->ubytecolorptr+1); 
      FIXUP(dfn->code, 30, 0x0, (int)tnl->ubytecolorptr+2); 
      FIXUP(dfn->code, 36, 0x0, (int)tnl->ubytecolorptr+3); 
      return dfn;
   }
   else
      return 0;
}



struct dynfn *tnl_makeX86Attr2fv( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0x8b, 0x44, 0x24, 0x04,          	/* mov    0x4(%esp,1),%eax */
      0xba, 0x78, 0x56, 0x34, 0x12,     /* mov    $DEST,%edx */
      0x8b, 0x08,                	/* mov    (%eax),%ecx */
      0x8b, 0x40, 0x04,             	/* mov    0x4(%eax),%eax */
      0x89, 0x0a,                	/* mov    %ecx,(%edx) */
      0x89, 0x42, 0x04,             	/* mov    %eax,0x4(%edx) */
      0xc3,                     	/* ret     */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.TexCoord2fv, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 5, 0x12345678, (int)tnl->texcoordptr[0]); 
   return dfn;
}

struct dynfn *tnl_makeX86Attr2f( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0xba, 0x78, 0x56, 0x34, 0x12,    	/* mov    $DEST,%edx */
      0x8b, 0x44, 0x24, 0x04,          	/* mov    0x4(%esp,1),%eax */
      0x8b, 0x4c, 0x24, 0x08,          	/* mov    0x8(%esp,1),%ecx */
      0x89, 0x02,                	/* mov    %eax,(%edx) */
      0x89, 0x4a, 0x04,             	/* mov    %ecx,0x4(%edx) */
      0xc3,                     	/* ret     */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.TexCoord2f, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 1, 0x12345678, (int)tnl->texcoordptr[0]); 
   return dfn;
}


struct dynfn *tnl_makeX86Attr1fv( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0x8b, 0x44, 0x24, 0x04,          	/* mov    0x4(%esp,1),%eax */
      0xba, 0x78, 0x56, 0x34, 0x12,     /* mov    $DEST,%edx */
      0x8b, 0x08,                	/* mov    (%eax),%ecx */
      0x89, 0x0a,                	/* mov    %ecx,(%edx) */
      0xc3,                     	/* ret     */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.TexCoord2fv, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 5, 0x12345678, (int)tnl->texcoordptr[0]); 
   return dfn;
}

struct dynfn *tnl_makeX86Attr1f( TNLcontext *tnl, int key )
{
   static  char temp[] = {
      0xba, 0x78, 0x56, 0x34, 0x12,    	/* mov    $DEST,%edx */
      0x8b, 0x44, 0x24, 0x04,          	/* mov    0x4(%esp,1),%eax */
      0x89, 0x02,                	/* mov    %eax,(%edx) */
      0xc3,                     	/* ret     */
   };

   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (TNL_DEBUG & DEBUG_CODEGEN)
      _mesa_debug(NULL,  "%s 0x%08x\n", __FUNCTION__, key );

   insert_at_head( &tnl->dfn_cache.TexCoord2f, dfn );
   dfn->key = key;
   dfn->code = ALIGN_MALLOC( sizeof(temp), 16 );
   memcpy (dfn->code, temp, sizeof(temp));
   FIXUP(dfn->code, 1, 0x12345678, (int)tnl->texcoordptr[0]); 
   return dfn;
}



void _tnl_InitX86Codegen( struct dfn_generators *gen )
{
   gen->Attr1f = tnl_makeX86Attr1f;
   gen->Attr1fv = tnl_makeX86Attr1fv;
   gen->Attr2f = tnl_makeX86Attr2f;
   gen->Attr2fv = tnl_makeX86Attr2fv;
   gen->Attr3f = tnl_makeX86Attr3f;
   gen->Attr3fv = tnl_makeX86Attr3fv;
   gen->Attr4f = tnl_makeX86Attr4f;
   gen->Attr4fv = tnl_makeX86Attr4fv;
   gen->Attr4ub = tnl_makeX86Attr4ub; 
   gen->Attr4ubv = tnl_makeX86Attr4ubv;
   gen->Vertex3f = tnl_makeX86Vertex3f;
   gen->Vertex3fv = tnl_makeX86Vertex3fv;
}


#else 

void _tnl_InitX86Codegen( struct dfn_generators *gen )
{
   (void) gen;
}

#endif
