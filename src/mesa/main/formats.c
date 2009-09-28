/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "imports.h"
#include "formats.h"
#include "config.h"
#include "texstore.h"


/**
 * Info about each format.
 * These must be in the same order as the MESA_FORMAT_* enums so that
 * we can do lookups without searching.
 */
static struct gl_format_info format_info[MESA_FORMAT_COUNT] =
{
   {
      MESA_FORMAT_NONE,            /* Name */
      GL_NONE,                     /* BaseFormat */
      GL_NONE,                     /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      0, 0, 0                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA8888,        /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA8888_REV,    /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB8888,        /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB8888_REV,    /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB888,          /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 3                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_BGR888,          /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 3                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB565,          /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 6, 5, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB565_REV,      /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 6, 5, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA4444,        /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB4444,        /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB4444_REV,    /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA5551,        /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB1555,        /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB1555_REV,    /* Name */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL88,            /* Name */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL88_REV,        /* Name */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB332,          /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      3, 3, 2, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_A8,              /* Name */
      GL_ALPHA,                    /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_L8,              /* Name */
      GL_LUMINANCE,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_I8,              /* Name */
      GL_INTENSITY,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 8, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_CI8,             /* Name */
      GL_COLOR_INDEX,              /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 8, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_YCBCR,           /* Name */
      GL_YCBCR_MESA,               /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_YCBCR_REV,       /* Name */
      GL_YCBCR_MESA,               /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z24_S8,          /* Name */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_S8_Z24,          /* Name */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z16,             /* Name */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 16, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z32,             /* Name */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 32, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_S8,              /* Name */
      GL_STENCIL_INDEX,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 8,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },

#if FEATURE_EXT_texture_sRGB
   {
      MESA_FORMAT_SRGB8,
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      1, 1, 3
   },
   {
      MESA_FORMAT_SRGBA8,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SARGB8,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SL8,
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SLA8,
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
#if FEATURE_texture_s3tc
   {
      MESA_FORMAT_SRGB_DXT1,       /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 0,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT1,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT3,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT5,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
#endif
#endif

#if FEATURE_texture_fxt1
   {
      MESA_FORMAT_RGB_FXT1,
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      8, 4, 16                     /* 16 bytes per 8x4 block */
   },
   {
      MESA_FORMAT_RGBA_FXT1,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      8, 4, 16                     /* 16 bytes per 8x4 block */
   },
#endif

#if FEATURE_texture_s3tc
   {
      MESA_FORMAT_RGB_DXT1,        /* Name */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 0,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT1,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT3,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT5,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
#endif

   {
      MESA_FORMAT_RGBA,
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      CHAN_BITS, CHAN_BITS, CHAN_BITS, CHAN_BITS,
      0, 0, 0, 0, 0,
      1, 1, 4 * CHAN_BITS / 8
   },
   {
      MESA_FORMAT_RGB,
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,    
      CHAN_BITS, CHAN_BITS, CHAN_BITS, 0,
      0, 0, 0, 0, 0,
      1, 1, 3 * CHAN_BITS / 8
   },
   {
      MESA_FORMAT_ALPHA,
      GL_ALPHA,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, CHAN_BITS,
      0, 0, 0, 0, 0,
      1, 1, 1 * CHAN_BITS / 8
   },
   {
      MESA_FORMAT_LUMINANCE,
      GL_LUMINANCE,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 0,
      CHAN_BITS, 0, 0, 0, 0,
      1, 1, 1 * CHAN_BITS / 8
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA,
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, CHAN_BITS,
      CHAN_BITS, 0, 0, 0, 0,
      1, 1, 2 * CHAN_BITS / 8
   },
   {
      MESA_FORMAT_INTENSITY,
      GL_INTENSITY,
      GL_UNSIGNED_NORMALIZED,
      0, 0, 0, 0,
      0, CHAN_BITS, 0, 0, 0,
      1, 1, 1 * CHAN_BITS / 8
   },
   {
      MESA_FORMAT_RGBA_FLOAT32,
      GL_RGBA,
      GL_FLOAT,
      32, 32, 32, 32,
      0, 0, 0, 0, 0,
      1, 1, 16
   },
   {
      MESA_FORMAT_RGBA_FLOAT16,
      GL_RGBA,
      GL_FLOAT,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RGB_FLOAT32,
      GL_RGB,
      GL_FLOAT,
      32, 32, 32, 0,
      0, 0, 0, 0, 0,
      1, 1, 12
   },
   {
      MESA_FORMAT_RGB_FLOAT16,
      GL_RGB,
      GL_FLOAT,
      16, 16, 16, 0,
      0, 0, 0, 0, 0,
      1, 1, 6
   },
   {
      MESA_FORMAT_ALPHA_FLOAT32,
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 32,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_ALPHA_FLOAT16,
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 16,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT32,
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 0,
      32, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT16,
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 0,
      16, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
      GL_LUMINANCE_ALPHA,
      GL_FLOAT,
      0, 0, 0, 32,
      32, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
      GL_LUMINANCE_ALPHA,
      GL_FLOAT,
      0, 0, 0, 16,
      16, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT32,
      GL_INTENSITY,
      GL_FLOAT,
      0, 0, 0, 0,
      0, 32, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT16,
      GL_INTENSITY,
      GL_FLOAT,
      0, 0, 0, 0,
      0, 16, 0, 0, 0,
      1, 1, 2
   },

   {
      MESA_FORMAT_DUDV8,
      GL_DUDV_ATI,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },

   {
      MESA_FORMAT_SIGNED_RGBA8888,
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888_REV,
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },

};



static const struct gl_format_info *
_mesa_get_format_info(gl_format format)
{
   const struct gl_format_info *info = &format_info[format];
   assert(info->Name == format);
   return info;
}


GLuint
_mesa_get_format_bytes(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   ASSERT(info->BytesPerBlock);
   return info->BytesPerBlock;
}


GLenum
_mesa_get_format_base_format(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BaseFormat;
}


GLboolean
_mesa_is_format_compressed(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BlockWidth > 1 || info->BlockHeight > 1;
}


/**
 * Do sanity checking of the format info table.
 */
void
_mesa_test_formats(void)
{
   GLuint i;

   assert(Elements(format_info) == MESA_FORMAT_COUNT);

   for (i = 0; i < MESA_FORMAT_COUNT; i++) {
      const struct gl_format_info *info = _mesa_get_format_info(i);
      assert(info);

      assert(info->Name == i);

      if (info->Name == MESA_FORMAT_NONE)
         continue;

      if (info->BlockWidth == 1 && info->BlockHeight == 1) {
         if (info->RedBits > 0) {
            GLuint t = info->RedBits + info->GreenBits
               + info->BlueBits + info->AlphaBits;
            assert(t / 8 == info->BytesPerBlock);
         }
      }

      assert(info->DataType == GL_UNSIGNED_NORMALIZED ||
             info->DataType == GL_SIGNED_NORMALIZED ||
             info->DataType == GL_UNSIGNED_INT ||
             info->DataType == GL_FLOAT);

      if (info->BaseFormat == GL_RGB) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits > 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RGBA) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits > 0);
         assert(info->AlphaBits > 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_LUMINANCE) {
         assert(info->RedBits == 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits > 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_INTENSITY) {
         assert(info->RedBits == 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits > 0);
      }

   }
}


/**
 * XXX possible replacement for _mesa_format_to_type_and_comps()
 * Used for mipmap generation.
 */
void
_mesa_format_to_type_and_comps2(gl_format format,
                                GLenum *datatype, GLuint *comps)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   /* We use a bunch of heuristics here.  If this gets too ugly we could
    * just encode the info the in the gl_format_info structures.
    */
   if (info->BaseFormat == GL_RGB ||
       info->BaseFormat == GL_RGBA ||
       info->BaseFormat == GL_ALPHA) {
      *comps = ((info->RedBits > 0) +
                (info->GreenBits > 0) +
                (info->BlueBits > 0) +
                (info->AlphaBits > 0));

      if (info->DataType== GL_FLOAT) {
         if (info->RedBits == 32)
            *datatype = GL_FLOAT;
         else
            *datatype = GL_HALF_FLOAT;
      }
      else if (info->GreenBits == 3) {
         *datatype = GL_UNSIGNED_BYTE_3_3_2;
      }
      else if (info->GreenBits == 4) {
         *datatype = GL_UNSIGNED_SHORT_4_4_4_4;
      }
      else if (info->GreenBits == 6) {
         *datatype = GL_UNSIGNED_SHORT_5_6_5;
      }
      else if (info->GreenBits == 5) {
         *datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      }
      else if (info->RedBits == 8) {
         *datatype = GL_UNSIGNED_BYTE;
      }
      else {
         ASSERT(info->RedBits == 16);
         *datatype = GL_UNSIGNED_SHORT;
      }
   }
   else if (info->BaseFormat == GL_LUMINANCE ||
            info->BaseFormat == GL_LUMINANCE_ALPHA) {
      *comps = ((info->LuminanceBits > 0) +
                (info->AlphaBits > 0));
      if (info->LuminanceBits == 8) {
         *datatype = GL_UNSIGNED_BYTE;
      }
      else if (info->LuminanceBits == 16) {
         *datatype = GL_UNSIGNED_SHORT;
      }
      else {
         *datatype = GL_FLOAT;
      }
   }
   else if (info->BaseFormat == GL_INTENSITY) {
      *comps = 1;
      if (info->IntensityBits == 8) {
         *datatype = GL_UNSIGNED_BYTE;
      }
      else if (info->IntensityBits == 16) {
         *datatype = GL_UNSIGNED_SHORT;
      }
      else {
         *datatype = GL_FLOAT;
      }
   }
   else if (info->BaseFormat == GL_COLOR_INDEX) {
      *comps = 1;
      *datatype = GL_UNSIGNED_BYTE;
   }
   else if (info->BaseFormat == GL_DEPTH_COMPONENT) {
      *comps = 1;
      if (info->DepthBits == 16) {
         *datatype = GL_UNSIGNED_SHORT;
      }
      else {
         ASSERT(info->DepthBits == 32);
         *datatype = GL_UNSIGNED_INT;
      }
   }
   else if (info->BaseFormat == GL_DEPTH_STENCIL) {
      *comps = 1;
      *datatype = GL_UNSIGNED_INT;
   }
   else if (info->BaseFormat == GL_YCBCR_MESA) {
      *comps = 2;
      *datatype = GL_UNSIGNED_SHORT;
   }
   else if (info->BaseFormat == GL_DUDV_ATI) {
      *comps = 2;
      *datatype = GL_BYTE;
   }
   else {
      /* any other formats? */
      ASSERT(0);
      *comps = 1;
      *datatype = GL_UNSIGNED_BYTE;
   }
}


GLint
_mesa_get_format_bits(gl_format format, GLenum pname)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   switch (pname) {
   case GL_TEXTURE_RED_SIZE:
      return info->RedBits;
   case GL_TEXTURE_GREEN_SIZE:
      return info->GreenBits;
   case GL_TEXTURE_BLUE_SIZE:
      return info->BlueBits;
   case GL_TEXTURE_ALPHA_SIZE:
      return info->AlphaBits;
   case GL_TEXTURE_INTENSITY_SIZE:
      return info->IntensityBits;
   case GL_TEXTURE_LUMINANCE_SIZE:
      return info->LuminanceBits;
   case GL_TEXTURE_INDEX_SIZE_EXT:
      return info->IndexBits;
   case GL_TEXTURE_DEPTH_SIZE_ARB:
      return info->DepthBits;
   case GL_TEXTURE_STENCIL_SIZE_EXT:
      return info->StencilBits;
   default:
      _mesa_problem(NULL, "bad pname in _mesa_get_format_bits()");
      return 0;
   }
}
