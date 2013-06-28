/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "intel_mipmap_tree.h"
#include "intel_tex_layout.h"
#include "intel_context.h"

#include "main/image.h"
#include "main/macros.h"

static unsigned int
intel_horizontal_texture_alignment_unit(struct intel_context *intel,
                                       gl_format format)
{
   /**
    * From the "Alignment Unit Size" section of various specs, namely:
    * - Gen3 Spec: "Memory Data Formats" Volume,         Section 1.20.1.4
    * - i965 and G45 PRMs:             Volume 1,         Section 6.17.3.4.
    * - Ironlake and Sandybridge PRMs: Volume 1, Part 1, Section 7.18.3.4
    * - BSpec (for Ivybridge and slight variations in separate stencil)
    *
    * +----------------------------------------------------------------------+
    * |                                        | alignment unit width  ("i") |
    * | Surface Property                       |-----------------------------|
    * |                                        | 915 | 965 | ILK | SNB | IVB |
    * +----------------------------------------------------------------------+
    * | YUV 4:2:2 format                       |  8  |  4  |  4  |  4  |  4  |
    * | BC1-5 compressed format (DXTn/S3TC)    |  4  |  4  |  4  |  4  |  4  |
    * | FXT1  compressed format                |  8  |  8  |  8  |  8  |  8  |
    * | Depth Buffer (16-bit)                  |  4  |  4  |  4  |  4  |  8  |
    * | Depth Buffer (other)                   |  4  |  4  |  4  |  4  |  4  |
    * | Separate Stencil Buffer                | N/A | N/A |  8  |  8  |  8  |
    * | All Others                             |  4  |  4  |  4  |  4  |  4  |
    * +----------------------------------------------------------------------+
    *
    * On IVB+, non-special cases can be overridden by setting the SURFACE_STATE
    * "Surface Horizontal Alignment" field to HALIGN_4 or HALIGN_8.
    */
    if (_mesa_is_format_compressed(format)) {
       /* The hardware alignment requirements for compressed textures
        * happen to match the block boundaries.
        */
      unsigned int i, j;
      _mesa_get_format_block_size(format, &i, &j);
      return i;
    }

   if (format == MESA_FORMAT_S8)
      return 8;

   /* The depth alignment requirements in the table above are for rendering to
    * depth miplevels using the LOD control fields.  We don't use LOD control
    * fields, and instead use page offsets plus intra-tile x/y offsets, which
    * require that the low 3 bits are zero.  To reduce the number of x/y
    * offset workaround blits we do, align the X to 8, which depth texturing
    * can handle (sadly, it can't handle 8 in the Y direction).
    */
   if (intel->gen >= 7 &&
       _mesa_get_format_base_format(format) == GL_DEPTH_COMPONENT)
      return 8;

   return 4;
}

static unsigned int
intel_vertical_texture_alignment_unit(struct intel_context *intel,
                                     gl_format format)
{
   /**
    * From the "Alignment Unit Size" section of various specs, namely:
    * - Gen3 Spec: "Memory Data Formats" Volume,         Section 1.20.1.4
    * - i965 and G45 PRMs:             Volume 1,         Section 6.17.3.4.
    * - Ironlake and Sandybridge PRMs: Volume 1, Part 1, Section 7.18.3.4
    * - BSpec (for Ivybridge and slight variations in separate stencil)
    *
    * +----------------------------------------------------------------------+
    * |                                        | alignment unit height ("j") |
    * | Surface Property                       |-----------------------------|
    * |                                        | 915 | 965 | ILK | SNB | IVB |
    * +----------------------------------------------------------------------+
    * | BC1-5 compressed format (DXTn/S3TC)    |  4  |  4  |  4  |  4  |  4  |
    * | FXT1  compressed format                |  4  |  4  |  4  |  4  |  4  |
    * | Depth Buffer                           |  2  |  2  |  2  |  4  |  4  |
    * | Separate Stencil Buffer                | N/A | N/A | N/A |  4  |  8  |
    * | Multisampled (4x or 8x) render target  | N/A | N/A | N/A |  4  |  4  |
    * | All Others                             |  2  |  2  |  2  |  2  |  2  |
    * +----------------------------------------------------------------------+
    *
    * On SNB+, non-special cases can be overridden by setting the SURFACE_STATE
    * "Surface Vertical Alignment" field to VALIGN_2 or VALIGN_4.
    *
    * We currently don't support multisampling.
    */
   if (_mesa_is_format_compressed(format))
      return 4;

   if (format == MESA_FORMAT_S8)
      return intel->gen >= 7 ? 8 : 4;

   GLenum base_format = _mesa_get_format_base_format(format);

   if (intel->gen >= 6 &&
       (base_format == GL_DEPTH_COMPONENT ||
	base_format == GL_DEPTH_STENCIL)) {
      return 4;
   }

   return 2;
}

void
intel_get_texture_alignment_unit(struct intel_context *intel,
				 gl_format format,
				 unsigned int *w, unsigned int *h)
{
   *w = intel_horizontal_texture_alignment_unit(intel, format);
   *h = intel_vertical_texture_alignment_unit(intel, format);
}
