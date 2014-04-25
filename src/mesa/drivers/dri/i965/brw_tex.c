/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

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

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/teximage.h"

#include "intel_tex.h"
#include "brw_context.h"

/**
 * Finalizes all textures, completing any rendering that needs to be done
 * to prepare them.
 */
void brw_validate_textures( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->ctx;
   int i;
   int maxEnabledUnit = ctx->Texture._MaxEnabledTexImageUnit;

   for (i = 0; i <= maxEnabledUnit; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      if (texUnit->_Current) {
	 intel_finalize_mipmap_tree(brw, i);
      }
   }
}
