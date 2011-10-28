/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <sched.h>
#include <errno.h>

#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/context.h"
#include "swrast/swrast.h"



#include "radeon_common.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "radeon_reg.h"

#define R200_TIMEOUT             512
#define R200_IDLE_RETRY           16

/* ================================================================
 * Buffer clear
 */
static void r200Clear( struct gl_context *ctx, GLbitfield mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint flags = 0;
   GLuint orig_mask = mask;

   if ( R200_DEBUG & RADEON_IOCTL ) {
	   if (rmesa->radeon.sarea)
	       fprintf( stderr, "r200Clear %x %d\n", mask, rmesa->radeon.sarea->pfCurrentPage);
	   else
	       fprintf( stderr, "r200Clear %x radeon->sarea is NULL\n", mask);
   }

   radeonFlush( ctx );

   if ( mask & BUFFER_BIT_FRONT_LEFT ) {
      flags |= RADEON_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ( mask & BUFFER_BIT_BACK_LEFT ) {
      flags |= RADEON_BACK;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ( mask & BUFFER_BIT_DEPTH ) {
      flags |= RADEON_DEPTH;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if ( (mask & BUFFER_BIT_STENCIL) ) {
      flags |= RADEON_STENCIL;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   if ( mask ) {
      if (R200_DEBUG & RADEON_FALLBACKS)
	 fprintf(stderr, "%s: swrast clear, mask: %x\n", __FUNCTION__, mask);
      _swrast_Clear( ctx, mask );
   }

   if ( !flags )
      return;

   if (rmesa->using_hyperz) {
      flags |= RADEON_USE_COMP_ZBUF;
/*      if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R200)
	 flags |= RADEON_USE_HIERZ; */
      if (!((flags & RADEON_DEPTH) && (flags & RADEON_STENCIL) &&
	    ((rmesa->radeon.state.stencil.clear & R200_STENCIL_WRITE_MASK) == R200_STENCIL_WRITE_MASK))) {
	  flags |= RADEON_CLEAR_FASTZ;
      }
   }

   radeonUserClear(ctx, orig_mask);
}

GLboolean r200IsGartMemory( r200ContextPtr rmesa, const GLvoid *pointer,
			   GLint size )
{
   ptrdiff_t offset = (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;
   int valid = (size >= 0 &&
		offset >= 0 &&
		offset + size < rmesa->radeon.radeonScreen->gartTextures.size);

   if (R200_DEBUG & RADEON_IOCTL)
      fprintf(stderr, "r200IsGartMemory( %p ) : %d\n", pointer, valid );

   return valid;
}


GLuint r200GartOffsetFromVirtual( r200ContextPtr rmesa, const GLvoid *pointer )
{
   ptrdiff_t offset = (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;

   if (offset < 0 || offset > rmesa->radeon.radeonScreen->gartTextures.size)
      return ~0;
   else
      return rmesa->radeon.radeonScreen->gart_texture_offset + offset;
}



void r200InitIoctlFuncs( struct dd_function_table *functions )
{
    functions->Clear = r200Clear;
    functions->Finish = radeonFinish;
    functions->Flush = radeonFlush;
}

