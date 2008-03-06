/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
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
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
        

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "image.h"
#include "teximage.h"
#include "texstore.h"
#include "texformat.h"
#include "texmem.h"

#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_regions.h"
#include "intel_tex.h"
#include "brw_context.h"
#include "brw_defines.h"


void brw_FrameBufferTexInit( struct brw_context *brw,
			     struct intel_region *region )
{
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;
   struct gl_texture_object *obj;
   struct gl_texture_image *img;
   
   intel->frame_buffer_texobj = obj =
      ctx->Driver.NewTextureObject( ctx, (GLuint) -1, GL_TEXTURE_2D );

   obj->MinFilter = GL_NEAREST;
   obj->MagFilter = GL_NEAREST;

   img = ctx->Driver.NewTextureImage( ctx );

   _mesa_init_teximage_fields( ctx, GL_TEXTURE_2D, img,
			       region->pitch, region->height, 1, 0,
			       region->cpp == 4 ? GL_RGBA : GL_RGB );
   
   _mesa_set_tex_image( obj, GL_TEXTURE_2D, 0, img );
}

void brw_FrameBufferTexDestroy( struct brw_context *brw )
{
   if (brw->intel.frame_buffer_texobj != NULL)
      brw->intel.ctx.Driver.DeleteTexture( &brw->intel.ctx,
					   brw->intel.frame_buffer_texobj );
   brw->intel.frame_buffer_texobj = NULL;
}

/**
 * Finalizes all textures, completing any rendering that needs to be done
 * to prepare them.
 */
void brw_validate_textures( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   int i;

   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[i];

      if (texUnit->_ReallyEnabled) {
	 intel_finalize_mipmap_tree(intel, i);
      }
   }
}
