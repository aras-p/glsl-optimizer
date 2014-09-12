/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_FORMAT_H
#define ILO_FORMAT_H

#include "genhw/genhw.h"

#include "ilo_common.h"

struct ilo_screen;

void
ilo_init_format_functions(struct ilo_screen *is);

int
ilo_translate_color_format(const struct ilo_dev_info *dev,
                           enum pipe_format format);

/**
 * Translate a pipe format to a hardware surface format suitable for
 * the given purpose.  Return -1 on errors.
 *
 * This is an inline function not only for performance reasons.  There are
 * caveats that the callers should be aware of before calling this function.
 */
static inline int
ilo_translate_format(const struct ilo_dev_info *dev,
                     enum pipe_format format, unsigned bind)
{
   switch (bind) {
   case PIPE_BIND_RENDER_TARGET:
      /*
       * Some RGBX formats are not supported as render target formats.  But we
       * can use their RGBA counterparts and force the destination alpha to be
       * one when blending is enabled.
       */
      switch (format) {
      case PIPE_FORMAT_B8G8R8X8_UNORM:
         return GEN6_FORMAT_B8G8R8A8_UNORM;
      default:
         return ilo_translate_color_format(dev, format);
      }
      break;
   case PIPE_BIND_SAMPLER_VIEW:
      /*
       * For depth formats, we want the depth values to be returned as R
       * values.  But we assume in many places that the depth values are
       * returned as I values (util_make_fragment_tex_shader_writedepth() is
       * one such example).  We have to live with that at least for now.
       *
       * For ETC1 format, the texture data will be decompressed before being
       * written to the bo.  See tex_staging_sys_convert_write().
       */
      switch (format) {
      case PIPE_FORMAT_Z16_UNORM:
         return GEN6_FORMAT_I16_UNORM;
      case PIPE_FORMAT_Z32_FLOAT:
         return GEN6_FORMAT_I32_FLOAT;
      case PIPE_FORMAT_Z24X8_UNORM:
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         return GEN6_FORMAT_I24X8_UNORM;
      case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         return GEN6_FORMAT_I32X32_FLOAT;
      case PIPE_FORMAT_ETC1_RGB8:
         return GEN6_FORMAT_R8G8B8X8_UNORM;
      default:
         return ilo_translate_color_format(dev, format);
      }
      break;
   case PIPE_BIND_VERTEX_BUFFER:
      if (ilo_dev_gen(dev) >= ILO_GEN(7.5))
         return ilo_translate_color_format(dev, format);

      /*
       * Some 3-component formats are not supported as vertex element formats.
       * But since we move between vertices using vb->stride, we should be
       * good to use their 4-component counterparts if we force the W
       * component to be one.  The only exception is that the vb boundary
       * check for the last vertex may fail.
       */
      switch (format) {
      case PIPE_FORMAT_R16G16B16_FLOAT:
         return GEN6_FORMAT_R16G16B16A16_FLOAT;
      case PIPE_FORMAT_R16G16B16_UINT:
         return GEN6_FORMAT_R16G16B16A16_UINT;
      case PIPE_FORMAT_R16G16B16_SINT:
         return GEN6_FORMAT_R16G16B16A16_SINT;
      case PIPE_FORMAT_R8G8B8_UINT:
         return GEN6_FORMAT_R8G8B8A8_UINT;
      case PIPE_FORMAT_R8G8B8_SINT:
         return GEN6_FORMAT_R8G8B8A8_SINT;
      default:
         return ilo_translate_color_format(dev, format);
      }
      break;
   default:
      assert(!"cannot translate format");
      break;
   }

   return -1;
}

static inline int
ilo_translate_render_format(const struct ilo_dev_info *dev,
                            enum pipe_format format)
{
   return ilo_translate_format(dev, format, PIPE_BIND_RENDER_TARGET);
}

static inline int
ilo_translate_texture_format(const struct ilo_dev_info *dev,
                             enum pipe_format format)
{
   return ilo_translate_format(dev, format, PIPE_BIND_SAMPLER_VIEW);
}

static inline int
ilo_translate_vertex_format(const struct ilo_dev_info *dev,
                            enum pipe_format format)
{
   return ilo_translate_format(dev, format, PIPE_BIND_VERTEX_BUFFER);
}

#endif /* ILO_FORMAT_H */
