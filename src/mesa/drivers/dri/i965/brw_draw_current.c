/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <stdlib.h>

#include "glheader.h"
#include "context.h"
#include "state.h"
#include "api_validate.h"
#include "enums.h"

#include "brw_context.h"
#include "brw_draw.h"

#include "bufmgr.h"
#include "intel_buffer_objects.h"


void brw_init_current_values(GLcontext *ctx,
			     struct gl_client_array *arrays)
{
   GLuint i;

   memset(arrays, 0, sizeof(*arrays) * BRW_ATTRIB_MAX);

   /* Set up a constant (StrideB == 0) array for each current
    * attribute:
    */
   for (i = 0; i < BRW_ATTRIB_MAX; i++) {
      struct gl_client_array *cl = &arrays[i];

      switch (i) {
      case BRW_ATTRIB_MAT_FRONT_SHININESS:
      case BRW_ATTRIB_MAT_BACK_SHININESS:
      case BRW_ATTRIB_INDEX:
      case BRW_ATTRIB_EDGEFLAG:
	 cl->Size = 1;
	 break;
      case BRW_ATTRIB_MAT_FRONT_INDEXES:
      case BRW_ATTRIB_MAT_BACK_INDEXES:
	 cl->Size = 3;
	 break;
      default:
	 /* This is fixed for the material attributes, for others will
	  * be determined at runtime:
	  */
	 if (i >= BRW_ATTRIB_MAT_FRONT_AMBIENT)
	    cl->Size = 4;
	 else
	    cl->Size = 1;
	 break;
      }

      switch (i) {
      case BRW_ATTRIB_EDGEFLAG:
	 cl->Type = GL_UNSIGNED_BYTE;
	 cl->Ptr = (const void *)&ctx->Current.EdgeFlag;
	 break;
      case BRW_ATTRIB_INDEX:
	 cl->Type = GL_FLOAT;
	 cl->Ptr = (const void *)&ctx->Current.Index;
	 break;
      default:
	 cl->Type = GL_FLOAT;
	 if (i < BRW_ATTRIB_FIRST_MATERIAL)
	    cl->Ptr = (const void *)ctx->Current.Attrib[i];
	 else 
	    cl->Ptr = (const void *)ctx->Light.Material.Attrib[i - BRW_ATTRIB_FIRST_MATERIAL];
	 break;
      }

      cl->Stride = 0;
      cl->StrideB = 0;
      cl->Enabled = 1;
      cl->Flags = 0;
      cl->BufferObj = ctx->Array.NullBufferObj;
   }
}

