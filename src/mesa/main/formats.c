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


/**
 * Information about texture formats.
 */
struct gl_format_info
{
   gl_format Name;

   /** text name for debugging */
   const char *StrName;

   /**
    * Base format is one of GL_RGB, GL_RGBA, GL_ALPHA, GL_LUMINANCE,
    * GL_LUMINANCE_ALPHA, GL_INTENSITY, GL_YCBCR_MESA, GL_COLOR_INDEX,
    * GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL.
    */
   GLenum BaseFormat;

   /**
    * Logical data type: one of  GL_UNSIGNED_NORMALIZED, GL_SIGNED_NORMALED,
    * GL_UNSIGNED_INT, GL_SIGNED_INT, GL_FLOAT.
    */
   GLenum DataType;

   GLubyte RedBits;
   GLubyte GreenBits;
   GLubyte BlueBits;
   GLubyte AlphaBits;
   GLubyte LuminanceBits;
   GLubyte IntensityBits;
   GLubyte IndexBits;
   GLubyte DepthBits;
   GLubyte StencilBits;

   /**
    * To describe compressed formats.  If not compressed, Width=Height=1.
    */
   GLubyte BlockWidth, BlockHeight;
   GLubyte BytesPerBlock;
};


/**
 * Info about each format.
 * These must be in the same order as the MESA_FORMAT_* enums so that
 * we can do lookups without searching.
 */
static struct gl_format_info format_info[MESA_FORMAT_COUNT] =
{
   {
      MESA_FORMAT_NONE,            /* Name */
      "MESA_FORMAT_NONE",          /* StrName */
      GL_NONE,                     /* BaseFormat */
      GL_NONE,                     /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      0, 0, 0                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA8888,        /* Name */
      "MESA_FORMAT_RGBA8888",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA8888_REV,    /* Name */
      "MESA_FORMAT_RGBA8888_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB8888,        /* Name */
      "MESA_FORMAT_ARGB8888",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB8888_REV,    /* Name */
      "MESA_FORMAT_ARGB8888_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_XRGB8888,        /* Name */
      "MESA_FORMAT_XRGB8888",      /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_XRGB8888_REV,    /* Name */
      "MESA_FORMAT_XRGB8888_REV",  /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB888,          /* Name */
      "MESA_FORMAT_RGB888",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 3                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_BGR888,          /* Name */
      "MESA_FORMAT_BGR888",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 3                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB565,          /* Name */
      "MESA_FORMAT_RGB565",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 6, 5, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB565_REV,      /* Name */
      "MESA_FORMAT_RGB565_REV",    /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 6, 5, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB4444,        /* Name */
      "MESA_FORMAT_ARGB4444",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB4444_REV,    /* Name */
      "MESA_FORMAT_ARGB4444_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA5551,        /* Name */
      "MESA_FORMAT_RGBA5551",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB1555,        /* Name */
      "MESA_FORMAT_ARGB1555",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB1555_REV,    /* Name */
      "MESA_FORMAT_ARGB1555_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL88,            /* Name */
      "MESA_FORMAT_AL88",          /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL88_REV,        /* Name */
      "MESA_FORMAT_AL88_REV",      /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL1616,          /* Name */
      "MESA_FORMAT_AL1616",        /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 16,                 /* Red/Green/Blue/AlphaBits */
      16, 0, 0, 0, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL1616_REV,      /* Name */
      "MESA_FORMAT_AL1616_REV",    /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 16,                 /* Red/Green/Blue/AlphaBits */
      16, 0, 0, 0, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB332,          /* Name */
      "MESA_FORMAT_RGB332",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      3, 3, 2, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_A8,              /* Name */
      "MESA_FORMAT_A8",            /* StrName */
      GL_ALPHA,                    /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_L8,              /* Name */
      "MESA_FORMAT_L8",            /* StrName */
      GL_LUMINANCE,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_I8,              /* Name */
      "MESA_FORMAT_I8",            /* StrName */
      GL_INTENSITY,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 8, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_CI8,             /* Name */
      "MESA_FORMAT_CI8",           /* StrName */
      GL_COLOR_INDEX,              /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 8, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_YCBCR,           /* Name */
      "MESA_FORMAT_YCBCR",         /* StrName */
      GL_YCBCR_MESA,               /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_YCBCR_REV,       /* Name */
      "MESA_FORMAT_YCBCR_REV",     /* StrName */
      GL_YCBCR_MESA,               /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z24_S8,          /* Name */
      "MESA_FORMAT_Z24_S8",        /* StrName */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_S8_Z24,          /* Name */
      "MESA_FORMAT_S8_Z24",        /* StrName */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z16,             /* Name */
      "MESA_FORMAT_Z16",           /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 16, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_X8_Z24,          /* Name */
      "MESA_FORMAT_X8_Z24",        /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z24_X8,          /* Name */
      "MESA_FORMAT_Z24_X8",        /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z32,             /* Name */
      "MESA_FORMAT_Z32",           /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 32, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_S8,              /* Name */
      "MESA_FORMAT_S8",            /* StrName */
      GL_STENCIL_INDEX,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 8,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_SRGB8,
      "MESA_FORMAT_SRGB8",
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      1, 1, 3
   },
   {
      MESA_FORMAT_SRGBA8,
      "MESA_FORMAT_SRGBA8",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SARGB8,
      "MESA_FORMAT_SARGB8",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SL8,
      "MESA_FORMAT_SL8",
      GL_LUMINANCE,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 0,
      8, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_SLA8,
      "MESA_FORMAT_SLA8",
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SRGB_DXT1,       /* Name */
      "MESA_FORMAT_SRGB_DXT1",     /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 0,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT1,
      "MESA_FORMAT_SRGBA_DXT1",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT3,
      "MESA_FORMAT_SRGBA_DXT3",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT5,
      "MESA_FORMAT_SRGBA_DXT5",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },

   {
      MESA_FORMAT_RGB_FXT1,
      "MESA_FORMAT_RGB_FXT1",
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 0,                  /* approx Red/Green/BlueBits */
      0, 0, 0, 0, 0,
      8, 4, 16                     /* 16 bytes per 8x4 block */
   },
   {
      MESA_FORMAT_RGBA_FXT1,
      "MESA_FORMAT_RGBA_FXT1",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 1,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,
      8, 4, 16                     /* 16 bytes per 8x4 block */
   },

   {
      MESA_FORMAT_RGB_DXT1,        /* Name */
      "MESA_FORMAT_RGB_DXT1",      /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 0,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT1,
      "MESA_FORMAT_RGBA_DXT1",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT3,
      "MESA_FORMAT_RGBA_DXT3",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT5,
      "MESA_FORMAT_RGBA_DXT5",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_FLOAT32,
      "MESA_FORMAT_RGBA_FLOAT32",
      GL_RGBA,
      GL_FLOAT,
      32, 32, 32, 32,
      0, 0, 0, 0, 0,
      1, 1, 16
   },
   {
      MESA_FORMAT_RGBA_FLOAT16,
      "MESA_FORMAT_RGBA_FLOAT16",
      GL_RGBA,
      GL_FLOAT,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RGB_FLOAT32,
      "MESA_FORMAT_RGB_FLOAT32",
      GL_RGB,
      GL_FLOAT,
      32, 32, 32, 0,
      0, 0, 0, 0, 0,
      1, 1, 12
   },
   {
      MESA_FORMAT_RGB_FLOAT16,
      "MESA_FORMAT_RGB_FLOAT16",
      GL_RGB,
      GL_FLOAT,
      16, 16, 16, 0,
      0, 0, 0, 0, 0,
      1, 1, 6
   },
   {
      MESA_FORMAT_ALPHA_FLOAT32,
      "MESA_FORMAT_ALPHA_FLOAT32",
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 32,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_ALPHA_FLOAT16,
      "MESA_FORMAT_ALPHA_FLOAT16",
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 16,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT32,
      "MESA_FORMAT_LUMINANCE_FLOAT32",
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 0,
      32, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT16,
      "MESA_FORMAT_LUMINANCE_FLOAT16",
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 0,
      16, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
      "MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32",
      GL_LUMINANCE_ALPHA,
      GL_FLOAT,
      0, 0, 0, 32,
      32, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
      "MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16",
      GL_LUMINANCE_ALPHA,
      GL_FLOAT,
      0, 0, 0, 16,
      16, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT32,
      "MESA_FORMAT_INTENSITY_FLOAT32",
      GL_INTENSITY,
      GL_FLOAT,
      0, 0, 0, 0,
      0, 32, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT16,
      "MESA_FORMAT_INTENSITY_FLOAT16",
      GL_INTENSITY,
      GL_FLOAT,
      0, 0, 0, 0,
      0, 16, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_DUDV8,
      "MESA_FORMAT_DUDV8",
      GL_DUDV_ATI,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888,
      "MESA_FORMAT_SIGNED_RGBA8888",
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888_REV,
      "MESA_FORMAT_SIGNED_RGBA8888_REV",
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SIGNED_RGBA_16,
      "MESA_FORMAT_SIGNED_RGBA_16",
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   }
};



static const struct gl_format_info *
_mesa_get_format_info(gl_format format)
{
   const struct gl_format_info *info = &format_info[format];
   assert(info->Name == format);
   return info;
}


/** Return string name of format (for debugging) */
const char *
_mesa_get_format_name(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   ASSERT(info->BytesPerBlock);
   return info->StrName;
}



/**
 * Return bytes needed to store a block of pixels in the given format.
 * Normally, a block is 1x1 (a single pixel).  But for compressed formats
 * a block may be 4x4 or 8x4, etc.
 */
GLuint
_mesa_get_format_bytes(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   ASSERT(info->BytesPerBlock);
   return info->BytesPerBlock;
}


/**
 * Return bits per component for the given format.
 * \param format  one of MESA_FORMAT_x
 * \param pname  the component, such as GL_RED_BITS, GL_TEXTURE_BLUE_BITS, etc.
 */
GLint
_mesa_get_format_bits(gl_format format, GLenum pname)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   switch (pname) {
   case GL_RED_BITS:
   case GL_TEXTURE_RED_SIZE:
   case GL_RENDERBUFFER_RED_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
      return info->RedBits;
   case GL_GREEN_BITS:
   case GL_TEXTURE_GREEN_SIZE:
   case GL_RENDERBUFFER_GREEN_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
      return info->GreenBits;
   case GL_BLUE_BITS:
   case GL_TEXTURE_BLUE_SIZE:
   case GL_RENDERBUFFER_BLUE_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
      return info->BlueBits;
   case GL_ALPHA_BITS:
   case GL_TEXTURE_ALPHA_SIZE:
   case GL_RENDERBUFFER_ALPHA_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
      return info->AlphaBits;
   case GL_TEXTURE_INTENSITY_SIZE:
      return info->IntensityBits;
   case GL_TEXTURE_LUMINANCE_SIZE:
      return info->LuminanceBits;
   case GL_INDEX_BITS:
   case GL_TEXTURE_INDEX_SIZE_EXT:
      return info->IndexBits;
   case GL_DEPTH_BITS:
   case GL_TEXTURE_DEPTH_SIZE_ARB:
   case GL_RENDERBUFFER_DEPTH_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
      return info->DepthBits;
   case GL_STENCIL_BITS:
   case GL_TEXTURE_STENCIL_SIZE_EXT:
   case GL_RENDERBUFFER_STENCIL_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
      return info->StencilBits;
   default:
      _mesa_problem(NULL, "bad pname in _mesa_get_format_bits()");
      return 0;
   }
}


/**
 * Return the data type (or more specifically, the data representation)
 * for the given format.
 * The return value will be one of:
 *    GL_UNSIGNED_NORMALIZED = unsigned int representing [0,1]
 *    GL_SIGNED_NORMALIZED = signed int representing [-1, 1]
 *    GL_UNSIGNED_INT = an ordinary unsigned integer
 *    GL_FLOAT = an ordinary float
 */
GLenum
_mesa_get_format_datatype(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->DataType;
}


/**
 * Return the basic format for the given type.  The result will be
 * one of GL_RGB, GL_RGBA, GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA,
 * GL_INTENSITY, GL_YCBCR_MESA, GL_COLOR_INDEX, GL_DEPTH_COMPONENT,
 * GL_STENCIL_INDEX, GL_DEPTH_STENCIL.
 */
GLenum
_mesa_get_format_base_format(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BaseFormat;
}


/**
 * Return the block size (in pixels) for the given format.  Normally
 * the block size is 1x1.  But compressed formats will have block sizes
 * of 4x4 or 8x4 pixels, etc.
 * \param bw  returns block width in pixels
 * \param bh  returns block height in pixels
 */
void
_mesa_get_format_block_size(gl_format format, GLuint *bw, GLuint *bh)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   *bw = info->BlockWidth;
   *bh = info->BlockHeight;
}


/** Is the given format a compressed format? */
GLboolean
_mesa_is_format_compressed(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BlockWidth > 1 || info->BlockHeight > 1;
}


/**
 * Return color encoding for given format.
 * \return GL_LINEAR or GL_SRGB
 */
GLenum
_mesa_get_format_color_encoding(gl_format format)
{
   /* XXX this info should be encoded in gl_format_info */
   switch (format) {
   case MESA_FORMAT_SRGB8:
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_SL8:
   case MESA_FORMAT_SLA8:
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
      return GL_SRGB;
   default:
      return GL_LINEAR;
   }
}


/**
 * Return number of bytes needed to store an image of the given size
 * in the given format.
 */
GLuint
_mesa_format_image_size(gl_format format, GLsizei width,
                        GLsizei height, GLsizei depth)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format */
      const GLuint bw = info->BlockWidth, bh = info->BlockHeight;
      const GLuint wblocks = (width + bw - 1) / bw;
      const GLuint hblocks = (height + bh - 1) / bh;
      const GLuint sz = wblocks * hblocks * info->BytesPerBlock;
      return sz;
   }
   else {
      /* non-compressed */
      const GLuint sz = width * height * depth * info->BytesPerBlock;
      return sz;
   }
}



GLint
_mesa_format_row_stride(gl_format format, GLsizei width)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format */
      const GLuint bw = info->BlockWidth;
      const GLuint wblocks = (width + bw - 1) / bw;
      const GLint stride = wblocks * info->BytesPerBlock;
      return stride;
   }
   else {
      const GLint stride = width * info->BytesPerBlock;
      return stride;
   }
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
            (void) t;
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
 * Return datatype and number of components per texel for the given gl_format.
 * Only used for mipmap generation code.
 */
void
_mesa_format_to_type_and_comps(gl_format format,
                               GLenum *datatype, GLuint *comps)
{
   switch (format) {
   case MESA_FORMAT_RGBA8888:
   case MESA_FORMAT_RGBA8888_REV:
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_ARGB8888_REV:
   case MESA_FORMAT_XRGB8888:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB888:
   case MESA_FORMAT_BGR888:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB565:
   case MESA_FORMAT_RGB565_REV:
      *datatype = GL_UNSIGNED_SHORT_5_6_5;
      *comps = 3;
      return;

   case MESA_FORMAT_ARGB4444:
   case MESA_FORMAT_ARGB4444_REV:
      *datatype = GL_UNSIGNED_SHORT_4_4_4_4;
      *comps = 4;
      return;

   case MESA_FORMAT_ARGB1555:
   case MESA_FORMAT_ARGB1555_REV:
      *datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_AL88:
   case MESA_FORMAT_AL88_REV:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_AL1616:
   case MESA_FORMAT_AL1616_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_RGB332:
      *datatype = GL_UNSIGNED_BYTE_3_3_2;
      *comps = 3;
      return;

   case MESA_FORMAT_A8:
   case MESA_FORMAT_L8:
   case MESA_FORMAT_I8:
   case MESA_FORMAT_CI8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;

   case MESA_FORMAT_YCBCR:
   case MESA_FORMAT_YCBCR_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_Z24_S8:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1; /* XXX OK? */
      return;

   case MESA_FORMAT_S8_Z24:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1; /* XXX OK? */
      return;

   case MESA_FORMAT_Z16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;

   case MESA_FORMAT_X8_Z24:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z24_X8:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_DUDV8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_SIGNED_RGBA8888:
   case MESA_FORMAT_SIGNED_RGBA8888_REV:
      *datatype = GL_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_SIGNED_RGBA_16:
      *datatype = GL_SHORT;
      *comps = 4;
      return;

#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_SL8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_SLA8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;
#endif

#if FEATURE_texture_fxt1
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
#endif
#if FEATURE_texture_s3tc
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
#endif
      /* XXX generate error instead? */
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 0;
      return;
#endif

   case MESA_FORMAT_RGBA_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 4;
      return;
   case MESA_FORMAT_RGBA_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 3;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 2;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 2;
      return;
   case MESA_FORMAT_ALPHA_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
   case MESA_FORMAT_INTENSITY_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 1;
      return;
   case MESA_FORMAT_ALPHA_FLOAT16:
   case MESA_FORMAT_LUMINANCE_FLOAT16:
   case MESA_FORMAT_INTENSITY_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 1;
      return;

   default:
      _mesa_problem(NULL, "bad format in _mesa_format_to_type_and_comps");
      *datatype = 0;
      *comps = 1;
   }
}
