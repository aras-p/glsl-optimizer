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
 *   Brian Paul
 */

#include "imports.h"
#include "texformat.h"

#include "st_context.h"
#include "st_texobj.h"
#include "pipe/p_defines.h"


/**
 * Create a pipe_texture_object from a Mesa texture object.
 * Eventually, gl_texture_object may be derived from this...
 */
struct pipe_texture_object *
create_texture_object(struct gl_texture_object *texObj)
{
   struct pipe_texture_object *pto;
   const struct gl_texture_image *texImage;

   pto = calloc(1, sizeof(*pto));
   if (!pto)
      return NULL;

   /* XXX: Member not defined. Comment-out to get it compile. */
   /*assert(texObj->Complete);*/

   switch (texObj->Target) {
   case GL_TEXTURE_1D:
      pto->type = PIPE_TEXTURE_1D;
      break;
   case GL_TEXTURE_2D:
      pto->type = PIPE_TEXTURE_2D;
      break;
   case GL_TEXTURE_3D:
      pto->type = PIPE_TEXTURE_3D;
      break;
   case GL_TEXTURE_CUBE_MAP:
      pto->type = PIPE_TEXTURE_CUBE;
      break;
   default:
      assert(0);
      return NULL;
   }

   texImage = texObj->Image[0][texObj->BaseLevel];
   assert(texImage);

   switch (texImage->TexFormat->MesaFormat) {
   case MESA_FORMAT_RGBA8888:
      pto->format = PIPE_FORMAT_U_R8_G8_B8_A8;
      break;
   case MESA_FORMAT_RGB565:
      pto->format = PIPE_FORMAT_U_R5_G6_B5;
      break;

   /* XXX fill in more formats */

   default:
      assert(0);
      return NULL;
   }

   pto->width = texImage->Width;
   pto->height = texImage->Height;
   pto->depth = texImage->Depth;

   /* XXX verify this */
   pto->mipmapped = texObj->Image[0][texObj->BaseLevel + 1] != NULL;

   return pto;
}
