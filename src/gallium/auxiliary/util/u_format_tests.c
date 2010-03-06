/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "u_memory.h"
#include "u_format_tests.h"


/*
 * Helper macros to create the packed bytes for longer words.
 */

#define PACKED_1x8(x)          {x, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PACKED_2x8(x, y)       {x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PACKED_3x8(x, y, z)    {x, y, z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PACKED_4x8(x, y, z, w) {x, y, z, w, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

#define PACKED_1x16(x)          {(x) & 0xff, (x) >> 8,          0,        0,          0,        0,          0,        0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PACKED_2x16(x, y)       {(x) & 0xff, (x) >> 8, (y) & 0xff, (y) >> 8,          0,        0,          0,        0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PACKED_3x16(x, y, z)    {(x) & 0xff, (x) >> 8, (y) & 0xff, (y) >> 8, (z) & 0xff, (z) >> 8,          0,        0, 0, 0, 0, 0, 0, 0, 0, 0}
#define PACKED_4x16(x, y, z, w) {(x) & 0xff, (x) >> 8, (y) & 0xff, (y) >> 8, (z) & 0xff, (z) >> 8, (w) & 0xff, (w) >> 8, 0, 0, 0, 0, 0, 0, 0, 0}

#define PACKED_1x32(x)          {(x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, (x) >> 24,          0,                 0,                  0,         0,          0,                 0,                  0,         0,          0,                 0,                  0,         0}
#define PACKED_2x32(x, y)       {(x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, (x) >> 24, (y) & 0xff, ((y) >> 8) & 0xff, ((y) >> 16) & 0xff, (y) >> 24,          0,                 0,                  0,         0,          0,                 0,                  0,         0}
#define PACKED_3x32(x, y, z)    {(x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, (x) >> 24, (y) & 0xff, ((y) >> 8) & 0xff, ((y) >> 16) & 0xff, (y) >> 24, (z) & 0xff, ((z) >> 8) & 0xff, ((z) >> 16) & 0xff, (z) >> 24,          0,                 0,                  0,         0}
#define PACKED_4x32(x, y, z, w) {(x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, (x) >> 24, (y) & 0xff, ((y) >> 8) & 0xff, ((y) >> 16) & 0xff, (y) >> 24, (z) & 0xff, ((z) >> 8) & 0xff, ((z) >> 16) & 0xff, (z) >> 24, (w) & 0xff, ((w) >> 8) & 0xff, ((w) >> 16) & 0xff, (w) >> 24}


/**
 * Test cases.
 *
 * These were manually entered. We could generate these
 *
 * To keep this to a we cover only the corner cases, which should produce
 * good enough coverage since that pixel format transformations are afine for
 * non SRGB formats.
 */
const struct util_format_test_case
util_format_test_cases[] =
{

   /*
    * 32-bit rendertarget formats
    */

   {PIPE_FORMAT_B8G8R8A8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x000000ff), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x0000ff00), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00ff0000), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xff000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8A8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_B8G8R8X8_UNORM, PACKED_1x32(0x00ffffff), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8X8_UNORM, PACKED_1x32(0x00ffffff), PACKED_1x32(0x000000ff), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_B8G8R8X8_UNORM, PACKED_1x32(0x00ffffff), PACKED_1x32(0x0000ff00), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8X8_UNORM, PACKED_1x32(0x00ffffff), PACKED_1x32(0x00ff0000), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8X8_UNORM, PACKED_1x32(0x00ffffff), PACKED_1x32(0xff000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B8G8R8X8_UNORM, PACKED_1x32(0x00ffffff), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_A8R8G8B8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x000000ff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x0000ff00), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00ff0000), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xff000000), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_A8R8G8B8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_X8R8G8B8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8R8G8B8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x000000ff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8R8G8B8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x0000ff00), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8R8G8B8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x00ff0000), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8R8G8B8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0xff000000), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_X8R8G8B8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_A8B8G8R8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8B8G8R8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x000000ff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_A8B8G8R8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x0000ff00), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_A8B8G8R8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00ff0000), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8B8G8R8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xff000000), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8B8G8R8_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_X8B8G8R8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8B8G8R8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x000000ff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8B8G8R8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x0000ff00), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_X8B8G8R8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0x00ff0000), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8B8G8R8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0xff000000), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_X8B8G8R8_UNORM, PACKED_1x32(0xffffff00), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R10G10B10A2_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R10G10B10A2_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x000003ff), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R10G10B10A2_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x000ffc00), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R10G10B10A2_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x3ff00000), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R10G10B10A2_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xc0000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R10G10B10A2_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   /*
    * 16-bit rendertarget formats
    */

   {PIPE_FORMAT_B5G5R5A1_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x001f), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x03e0), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x7c00), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x8000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G5R5A1_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_B4G4R4A4_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B4G4R4A4_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x000f), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_B4G4R4A4_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x00f0), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_B4G4R4A4_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0f00), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_B4G4R4A4_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xf000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B4G4R4A4_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_B5G6R5_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x001f), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x07e0), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xf800), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_B5G6R5_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {1.0, 1.0, 1.0, 1.0}},

   /*
    * Luminance/intensity/alpha formats
    */

   {PIPE_FORMAT_L8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0x00), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_L8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0xff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_A8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0x00), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_A8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0xff), {0.0, 0.0, 0.0, 1.0}},

   {PIPE_FORMAT_I8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0x00), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_I8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0xff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_L8A8_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_L8A8_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x00ff), {1.0, 1.0, 1.0, 0.0}},
   {PIPE_FORMAT_L8A8_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xff00), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_L8A8_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_L16_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_L16_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {1.0, 1.0, 1.0, 1.0}},

   /*
    * TODO: SRGB formats
    */

   /*
    * Mixed-signed formats
    */

   {PIPE_FORMAT_R8SG8SB8UX8U_NORM, PACKED_4x8(0xff, 0xff, 0xff, 0x00), PACKED_4x8(0x00, 0x00, 0x00, 0x00), { 0.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8SG8SB8UX8U_NORM, PACKED_4x8(0xff, 0xff, 0xff, 0x00), PACKED_4x8(0x7f, 0x00, 0x00, 0x00), { 1.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8SG8SB8UX8U_NORM, PACKED_4x8(0xff, 0xff, 0xff, 0x00), PACKED_4x8(0x81, 0x00, 0x00, 0x00), {-1.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8SG8SB8UX8U_NORM, PACKED_4x8(0xff, 0xff, 0xff, 0x00), PACKED_4x8(0x00, 0x7f, 0x00, 0x00), { 0.0,  1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8SG8SB8UX8U_NORM, PACKED_4x8(0xff, 0xff, 0xff, 0x00), PACKED_4x8(0x00, 0x81, 0x00, 0x00), { 0.0, -1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8SG8SB8UX8U_NORM, PACKED_4x8(0xff, 0xff, 0xff, 0x00), PACKED_4x8(0x00, 0x00, 0xff, 0x00), { 0.0,  0.0, 1.0, 1.0}},

   {PIPE_FORMAT_R5SG5SB6U_NORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), { 0.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5SG5SB6U_NORM, PACKED_1x16(0xffff), PACKED_1x16(0x000f), { 1.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5SG5SB6U_NORM, PACKED_1x16(0xffff), PACKED_1x16(0x0011), {-1.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5SG5SB6U_NORM, PACKED_1x16(0xffff), PACKED_1x16(0x01e0), { 0.0,  1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5SG5SB6U_NORM, PACKED_1x16(0xffff), PACKED_1x16(0x0220), { 0.0, -1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R5SG5SB6U_NORM, PACKED_1x16(0xffff), PACKED_1x16(0xfc00), { 0.0,  0.0, 1.0, 1.0}},

   /*
    * TODO: Depth-stencil formats
    */

   /*
    * TODO: YUV formats
    */

   /*
    * TODO: Compressed formats
    */

   /*
    * Standard 8-bit integer formats
    */

   {PIPE_FORMAT_R8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0x00), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8_UNORM, PACKED_1x8(0xff), PACKED_1x8(0xff), {1.0, 0.0, 0.0, 1.0}},

   {PIPE_FORMAT_R8G8_UNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x00), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8_UNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0xff, 0x00), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8_UNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0xff), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8_UNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0xff, 0xff), {1.0, 1.0, 0.0, 1.0}},

   {PIPE_FORMAT_R8G8B8_UNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x00), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_UNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0xff, 0x00, 0x00), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_UNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0xff, 0x00), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_UNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0xff), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_UNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0xff, 0xff, 0xff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R8G8B8A8_UNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x00), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0xff, 0x00, 0x00, 0x00), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0xff, 0x00, 0x00), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0xff, 0x00), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0xff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8A8_UNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0xff, 0xff, 0xff, 0xff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R8_USCALED, PACKED_1x8(0xff), PACKED_1x8(0x00), {  0.0,   0.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8_USCALED, PACKED_1x8(0xff), PACKED_1x8(0xff), {255.0,   0.0,   0.0, 1.0}},

   {PIPE_FORMAT_R8G8_USCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x00), {  0.0,   0.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8G8_USCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0xff, 0x00), {255.0,   0.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8G8_USCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0xff), {  0.0, 255.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8G8_USCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0xff, 0xff), {255.0, 255.0,   0.0, 1.0}},

   {PIPE_FORMAT_R8G8B8_USCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x00), {  0.0,   0.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_USCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0xff, 0x00, 0x00), {255.0,   0.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_USCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0xff, 0x00), {  0.0, 255.0,   0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_USCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0xff), {  0.0,   0.0, 255.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_USCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0xff, 0xff, 0xff), {255.0, 255.0, 255.0, 1.0}},

   {PIPE_FORMAT_R8G8B8A8_USCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x00), {  0.0,   0.0,   0.0,   0.0}},
   {PIPE_FORMAT_R8G8B8A8_USCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0xff, 0x00, 0x00, 0x00), {255.0,   0.0,   0.0,   0.0}},
   {PIPE_FORMAT_R8G8B8A8_USCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0xff, 0x00, 0x00), {  0.0, 255.0,   0.0,   0.0}},
   {PIPE_FORMAT_R8G8B8A8_USCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0xff, 0x00), {  0.0,   0.0, 255.0,   0.0}},
   {PIPE_FORMAT_R8G8B8A8_USCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0xff), {  0.0,   0.0,   0.0, 255.0}},
   {PIPE_FORMAT_R8G8B8A8_USCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0xff, 0xff, 0xff, 0xff), {255.0, 255.0, 255.0, 255.0}},

   {PIPE_FORMAT_R8_SNORM, PACKED_1x8(0xff), PACKED_1x8(0x00), { 0.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8_SNORM, PACKED_1x8(0xff), PACKED_1x8(0x7f), { 1.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8_SNORM, PACKED_1x8(0xff), PACKED_1x8(0x81), {-1.0,  0.0,  0.0,  1.0}},

   {PIPE_FORMAT_R8G8_SNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x00), { 0.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8_SNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x7f, 0x00), { 1.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8_SNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x81, 0x00), {-1.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8_SNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x7f), { 0.0,  1.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8_SNORM, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x81), { 0.0, -1.0,  0.0,  1.0}},

   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x00), { 0.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x7f, 0x00, 0x00), { 1.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x81, 0x00, 0x00), {-1.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x7f, 0x00), { 0.0,  1.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x81, 0x00), { 0.0, -1.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x7f), { 0.0,  0.0,  1.0,  1.0}},
   {PIPE_FORMAT_R8G8B8_SNORM, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x81), { 0.0,  0.0, -1.0,  1.0}},

   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x00), { 0.0,  0.0,  0.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x7f, 0x00, 0x00, 0x00), { 1.0,  0.0,  0.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x81, 0x00, 0x00, 0x00), {-1.0,  0.0,  0.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x7f, 0x00, 0x00), { 0.0,  1.0,  0.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x81, 0x00, 0x00), { 0.0, -1.0,  0.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x7f, 0x00), { 0.0,  0.0,  1.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x81, 0x00), { 0.0,  0.0, -1.0,  0.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x7f), { 0.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R8G8B8A8_SNORM, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x81), { 0.0,  0.0,  0.0, -1.0}},

   {PIPE_FORMAT_R8_SSCALED, PACKED_1x8(0xff), PACKED_1x8(0x00), {   0.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8_SSCALED, PACKED_1x8(0xff), PACKED_1x8(0x7f), { 127.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8_SSCALED, PACKED_1x8(0xff), PACKED_1x8(0x80), {-128.0,    0.0,    0.0, 1.0}},

   {PIPE_FORMAT_R8G8_SSCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x00), {   0.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8_SSCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x7f, 0x00), { 127.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8_SSCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x80, 0x00), {-128.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8_SSCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x7f), {   0.0,  127.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8_SSCALED, PACKED_2x8(0xff, 0xff), PACKED_2x8(0x00, 0x80), {   0.0, -128.0,    0.0, 1.0}},

   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x00), {   0.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x7f, 0x00, 0x00), { 127.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x80, 0x00, 0x00), {-128.0,    0.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x7f, 0x00), {   0.0,  127.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x80, 0x00), {   0.0, -128.0,    0.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x7f), {   0.0,    0.0,  127.0, 1.0}},
   {PIPE_FORMAT_R8G8B8_SSCALED, PACKED_3x8(0xff, 0xff, 0xff), PACKED_3x8(0x00, 0x00, 0x80), {   0.0,    0.0, -128.0, 1.0}},

   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x00), {   0.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x7f, 0x00, 0x00, 0x00), { 127.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x80, 0x00, 0x00, 0x00), {-128.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x7f, 0x00, 0x00), {   0.0,  127.0,    0.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x80, 0x00, 0x00), {   0.0, -128.0,    0.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x7f, 0x00), {   0.0,    0.0,  127.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x80, 0x00), {   0.0,    0.0, -128.0,    0.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x7f), {   0.0,    0.0,    0.0,  127.0}},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, PACKED_4x8(0xff, 0xff, 0xff, 0xff), PACKED_4x8(0x00, 0x00, 0x00, 0x80), {   0.0,    0.0,    0.0, -128.0}},

   /*
    * Standard 16-bit integer formats
    */

   {PIPE_FORMAT_R16_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16_UNORM, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {1.0, 0.0, 0.0, 1.0}},

   {PIPE_FORMAT_R16G16_UNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x0000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16_UNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0xffff, 0x0000), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16_UNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0xffff), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16_UNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0xffff, 0xffff), {1.0, 1.0, 0.0, 1.0}},

   {PIPE_FORMAT_R16G16B16_UNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x0000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16B16_UNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0xffff, 0x0000, 0x0000), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16B16_UNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0xffff, 0x0000), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16B16_UNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0xffff), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_R16G16B16_UNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0xffff, 0xffff, 0xffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R16G16B16A16_UNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x0000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R16G16B16A16_UNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0xffff, 0x0000, 0x0000, 0x0000), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R16G16B16A16_UNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0xffff, 0x0000, 0x0000), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R16G16B16A16_UNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0xffff, 0x0000), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R16G16B16A16_UNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0xffff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R16G16B16A16_UNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R16_USCALED, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {    0.0,     0.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16_USCALED, PACKED_1x16(0xffff), PACKED_1x16(0xffff), {65535.0,     0.0,     0.0,   1.0}},

   {PIPE_FORMAT_R16G16_USCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x0000), {    0.0,     0.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16G16_USCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0xffff, 0x0000), {65535.0,     0.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16G16_USCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0xffff), {    0.0, 65535.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16G16_USCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0xffff, 0xffff), {65535.0, 65535.0,     0.0,   1.0}},

   {PIPE_FORMAT_R16G16B16_USCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x0000), {    0.0,     0.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_USCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0xffff, 0x0000, 0x0000), {65535.0,     0.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_USCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0xffff, 0x0000), {    0.0, 65535.0,     0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_USCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0xffff), {    0.0,     0.0, 65535.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_USCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0xffff, 0xffff, 0xffff), {65535.0, 65535.0, 65535.0,   1.0}},

   {PIPE_FORMAT_R16G16B16A16_USCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x0000), {    0.0,     0.0,     0.0,     0.0}},
   {PIPE_FORMAT_R16G16B16A16_USCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0xffff, 0x0000, 0x0000, 0x0000), {65535.0,     0.0,     0.0,     0.0}},
   {PIPE_FORMAT_R16G16B16A16_USCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0xffff, 0x0000, 0x0000), {    0.0, 65535.0,     0.0,     0.0}},
   {PIPE_FORMAT_R16G16B16A16_USCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0xffff, 0x0000), {    0.0,     0.0, 65535.0,     0.0}},
   {PIPE_FORMAT_R16G16B16A16_USCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0xffff), {    0.0,     0.0,     0.0, 65535.0}},
   {PIPE_FORMAT_R16G16B16A16_USCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), {65535.0, 65535.0, 65535.0, 65535.0}},

   {PIPE_FORMAT_R16_SNORM, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16_SNORM, PACKED_1x16(0xffff), PACKED_1x16(0x7fff), {   1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16_SNORM, PACKED_1x16(0xffff), PACKED_1x16(0x8001), {  -1.0,    0.0,    0.0,    1.0}},

   {PIPE_FORMAT_R16G16_SNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x0000), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16_SNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x7fff, 0x0000), {   1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16_SNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x8001, 0x0000), {  -1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16_SNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x7fff), {   0.0,    1.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16_SNORM, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x8001), {   0.0,   -1.0,    0.0,    1.0}},

   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x0000), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x7fff, 0x0000, 0x0000), {   1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x8001, 0x0000, 0x0000), {  -1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x7fff, 0x0000), {   0.0,    1.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x8001, 0x0000), {   0.0,   -1.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x7fff), {   0.0,    0.0,    1.0,    1.0}},
   {PIPE_FORMAT_R16G16B16_SNORM, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x8001), {   0.0,    0.0,   -1.0,    1.0}},

   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x0000), {   0.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x7fff, 0x0000, 0x0000, 0x0000), {   1.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x8001, 0x0000, 0x0000, 0x0000), {  -1.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x7fff, 0x0000, 0x0000), {   0.0,    1.0,    0.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x8001, 0x0000, 0x0000), {   0.0,   -1.0,    0.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x7fff, 0x0000), {   0.0,    0.0,    1.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x8001, 0x0000), {   0.0,    0.0,   -1.0,    0.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x7fff), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R16G16B16A16_SNORM, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x8001), {   0.0,    0.0,    0.0,   -1.0}},

   {PIPE_FORMAT_R16_SSCALED, PACKED_1x16(0xffff), PACKED_1x16(0x0000), {     0.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16_SSCALED, PACKED_1x16(0xffff), PACKED_1x16(0x7fff), { 32767.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16_SSCALED, PACKED_1x16(0xffff), PACKED_1x16(0x8000), {-32768.0,      0.0,      0.0,   1.0}},

   {PIPE_FORMAT_R16G16_SSCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x0000), {     0.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16_SSCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x7fff, 0x0000), { 32767.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16_SSCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x8000, 0x0000), {-32768.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16_SSCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x7fff), {     0.0,  32767.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16_SSCALED, PACKED_2x16(0xffff, 0xffff), PACKED_2x16(0x0000, 0x8000), {     0.0, -32768.0,      0.0,   1.0}},

   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x0000), {     0.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x7fff, 0x0000, 0x0000), { 32767.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x8000, 0x0000, 0x0000), {-32768.0,      0.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x7fff, 0x0000), {     0.0,  32767.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x8000, 0x0000), {     0.0, -32768.0,      0.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x7fff), {     0.0,      0.0,  32767.0,   1.0}},
   {PIPE_FORMAT_R16G16B16_SSCALED, PACKED_3x16(0xffff, 0xffff, 0xffff), PACKED_3x16(0x0000, 0x0000, 0x8000), {     0.0,      0.0, -32768.0,   1.0}},

   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x0000), {     0.0,      0.0,      0.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x7fff, 0x0000, 0x0000, 0x0000), { 32767.0,      0.0,      0.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x8000, 0x0000, 0x0000, 0x0000), {-32768.0,      0.0,      0.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x7fff, 0x0000, 0x0000), {     0.0,  32767.0,      0.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x8000, 0x0000, 0x0000), {     0.0, -32768.0,      0.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x7fff, 0x0000), {     0.0,      0.0,  32767.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x8000, 0x0000), {     0.0,      0.0, -32768.0,      0.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x7fff), {     0.0,      0.0,      0.0,  32767.0}},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, PACKED_4x16(0xffff, 0xffff, 0xffff, 0xffff), PACKED_4x16(0x0000, 0x0000, 0x0000, 0x8000), {     0.0,      0.0,      0.0, -32768.0}},

   /*
    * Standard 32-bit integer formats
    *
    * NOTE: We can't accurately represent integers larger than +/-0x1000000
    * with single precision floats, so that's as far as we test.
    */

   {PIPE_FORMAT_R32_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32_UNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0xffffffff), {1.0, 0.0, 0.0, 1.0}},

   {PIPE_FORMAT_R32G32_UNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x00000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_UNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0xffffffff, 0x00000000), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_UNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0xffffffff), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_UNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0xffffffff, 0xffffffff), {1.0, 1.0, 0.0, 1.0}},

   {PIPE_FORMAT_R32G32B32_UNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x00000000), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_UNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0xffffffff, 0x00000000, 0x00000000), {1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_UNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0xffffffff, 0x00000000), {0.0, 1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_UNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0xffffffff), {0.0, 0.0, 1.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_UNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R32G32B32A32_UNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x00000000), {0.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R32G32B32A32_UNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0xffffffff, 0x00000000, 0x00000000, 0x00000000), {1.0, 0.0, 0.0, 0.0}},
   {PIPE_FORMAT_R32G32B32A32_UNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0xffffffff, 0x00000000, 0x00000000), {0.0, 1.0, 0.0, 0.0}},
   {PIPE_FORMAT_R32G32B32A32_UNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0xffffffff, 0x00000000), {0.0, 0.0, 1.0, 0.0}},
   {PIPE_FORMAT_R32G32B32A32_UNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0xffffffff), {0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32A32_UNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), {1.0, 1.0, 1.0, 1.0}},

   {PIPE_FORMAT_R32_USCALED, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {       0.0,        0.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32_USCALED, PACKED_1x32(0xffffffff), PACKED_1x32(0x01000000), {16777216.0,        0.0,        0.0,   1.0}},

   {PIPE_FORMAT_R32G32_USCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x00000000), {       0.0,        0.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32G32_USCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x01000000, 0x00000000), {16777216.0,        0.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32G32_USCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x01000000), {       0.0, 16777216.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32G32_USCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x01000000, 0x01000000), {16777216.0, 16777216.0,        0.0,   1.0}},

   {PIPE_FORMAT_R32G32B32_USCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x00000000), {       0.0,        0.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_USCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x01000000, 0x00000000, 0x00000000), {16777216.0,        0.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_USCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x01000000, 0x00000000), {       0.0, 16777216.0,        0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_USCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x01000000), {       0.0,        0.0, 16777216.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_USCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x01000000, 0x01000000, 0x01000000), {16777216.0, 16777216.0, 16777216.0,   1.0}},

   {PIPE_FORMAT_R32G32B32A32_USCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x00000000), {       0.0,        0.0,        0.0,        0.0}},
   {PIPE_FORMAT_R32G32B32A32_USCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x01000000, 0x00000000, 0x00000000, 0x00000000), {16777216.0,        0.0,        0.0,        0.0}},
   {PIPE_FORMAT_R32G32B32A32_USCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x01000000, 0x00000000, 0x00000000), {       0.0, 16777216.0,        0.0,        0.0}},
   {PIPE_FORMAT_R32G32B32A32_USCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x01000000, 0x00000000), {       0.0,        0.0, 16777216.0,        0.0}},
   {PIPE_FORMAT_R32G32B32A32_USCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x01000000), {       0.0,        0.0,        0.0, 16777216.0}},
   {PIPE_FORMAT_R32G32B32A32_USCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x01000000, 0x01000000, 0x01000000, 0x01000000), {16777216.0, 16777216.0, 16777216.0, 16777216.0}},

   {PIPE_FORMAT_R32_SNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32_SNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x7fffffff), {   1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32_SNORM, PACKED_1x32(0xffffffff), PACKED_1x32(0x80000001), {  -1.0,    0.0,    0.0,    1.0}},

   {PIPE_FORMAT_R32G32_SNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x00000000), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32_SNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x7fffffff, 0x00000000), {   1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32_SNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x80000001, 0x00000000), {  -1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32_SNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x7fffffff), {   0.0,    1.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32_SNORM, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x80000001), {   0.0,   -1.0,    0.0,    1.0}},

   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x00000000), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x7fffffff, 0x00000000, 0x00000000), {   1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x80000001, 0x00000000, 0x00000000), {  -1.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x7fffffff, 0x00000000), {   0.0,    1.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x80000001, 0x00000000), {   0.0,   -1.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x7fffffff), {   0.0,    0.0,    1.0,    1.0}},
   {PIPE_FORMAT_R32G32B32_SNORM, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x80000001), {   0.0,    0.0,   -1.0,    1.0}},

   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x00000000), {   0.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x7fffffff, 0x00000000, 0x00000000, 0x00000000), {   1.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x80000001, 0x00000000, 0x00000000, 0x00000000), {  -1.0,    0.0,    0.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x7fffffff, 0x00000000, 0x00000000), {   0.0,    1.0,    0.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x80000001, 0x00000000, 0x00000000), {   0.0,   -1.0,    0.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x7fffffff, 0x00000000), {   0.0,    0.0,    1.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x80000001, 0x00000000), {   0.0,    0.0,   -1.0,    0.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x7fffffff), {   0.0,    0.0,    0.0,    1.0}},
   {PIPE_FORMAT_R32G32B32A32_SNORM, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x80000001), {   0.0,    0.0,    0.0,   -1.0}},

   {PIPE_FORMAT_R32_SSCALED, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {        0.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32_SSCALED, PACKED_1x32(0xffffffff), PACKED_1x32(0x01000000), { 16777216.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32_SSCALED, PACKED_1x32(0xffffffff), PACKED_1x32(0xff000000), {-16777216.0,         0.0,         0.0,   1.0}},

   {PIPE_FORMAT_R32G32_SSCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x00000000), {        0.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32_SSCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x01000000, 0x00000000), { 16777216.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32_SSCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0xff000000, 0x00000000), {-16777216.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32_SSCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x01000000), {        0.0,  16777216.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32_SSCALED, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0xff000000), {        0.0, -16777216.0,         0.0,   1.0}},

   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x00000000), {        0.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x01000000, 0x00000000, 0x00000000), { 16777216.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0xff000000, 0x00000000, 0x00000000), {-16777216.0,         0.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x01000000, 0x00000000), {        0.0,  16777216.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0xff000000, 0x00000000), {        0.0, -16777216.0,         0.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x01000000), {        0.0,         0.0,  16777216.0,   1.0}},
   {PIPE_FORMAT_R32G32B32_SSCALED, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0xff000000), {        0.0,         0.0, -16777216.0,   1.0}},

   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x00000000), {        0.0,         0.0,         0.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x01000000, 0x00000000, 0x00000000, 0x00000000), { 16777216.0,         0.0,         0.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0xff000000, 0x00000000, 0x00000000, 0x00000000), {-16777216.0,         0.0,         0.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x01000000, 0x00000000, 0x00000000), {        0.0,  16777216.0,         0.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0xff000000, 0x00000000, 0x00000000), {        0.0, -16777216.0,         0.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x01000000, 0x00000000), {        0.0,         0.0,  16777216.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0xff000000, 0x00000000), {        0.0,         0.0, -16777216.0,         0.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x01000000), {        0.0,         0.0,         0.0,  16777216.0}},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0xff000000), {        0.0,         0.0,         0.0, -16777216.0}},

   /*
    * Standard 32-bit float formats
    */

   {PIPE_FORMAT_R32_FLOAT, PACKED_1x32(0xffffffff), PACKED_1x32(0x00000000), {  0.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32_FLOAT, PACKED_1x32(0xffffffff), PACKED_1x32(0x3f800000), {  1.0, 0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32_FLOAT, PACKED_1x32(0xffffffff), PACKED_1x32(0xbf800000), { -1.0, 0.0, 0.0, 1.0}},

   {PIPE_FORMAT_R32G32_FLOAT, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x00000000), { 0.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_FLOAT, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x3f800000, 0x00000000), { 1.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_FLOAT, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0xbf800000, 0x00000000), {-1.0,  0.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_FLOAT, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0x3f800000), { 0.0,  1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_FLOAT, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x00000000, 0xbf800000), { 0.0, -1.0, 0.0, 1.0}},
   {PIPE_FORMAT_R32G32_FLOAT, PACKED_2x32(0xffffffff, 0xffffffff), PACKED_2x32(0x3f800000, 0x3f800000), { 1.0,  1.0, 0.0, 1.0}},

   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x00000000), { 0.0,  0.0,  0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x3f800000, 0x00000000, 0x00000000), { 1.0,  0.0,  0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0xbf800000, 0x00000000, 0x00000000), {-1.0,  0.0,  0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x3f800000, 0x00000000), { 0.0,  1.0,  0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0xbf800000, 0x00000000), { 0.0, -1.0,  0.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0x3f800000), { 0.0,  0.0,  1.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x00000000, 0x00000000, 0xbf800000), { 0.0,  0.0, -1.0, 1.0}},
   {PIPE_FORMAT_R32G32B32_FLOAT, PACKED_3x32(0xffffffff, 0xffffffff, 0xffffffff), PACKED_3x32(0x3f800000, 0x3f800000, 0x3f800000), { 1.0,  1.0,  1.0, 1.0}},

   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x00000000), { 0.0,  0.0,  0.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x3f800000, 0x00000000, 0x00000000, 0x00000000), { 1.0,  0.0,  0.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0xbf800000, 0x00000000, 0x00000000, 0x00000000), {-1.0,  0.0,  0.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x3f800000, 0x00000000, 0x00000000), { 0.0,  1.0,  0.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0xbf800000, 0x00000000, 0x00000000), { 0.0, -1.0,  0.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x3f800000, 0x00000000), { 0.0,  0.0,  1.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0xbf800000, 0x00000000), { 0.0,  0.0, -1.0,  0.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0x3f800000), { 0.0,  0.0,  0.0,  1.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x00000000, 0x00000000, 0x00000000, 0xbf800000), { 0.0,  0.0,  0.0, -1.0}},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, PACKED_4x32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), PACKED_4x32(0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000), { 1.0,  1.0,  1.0,  1.0}},
};


const unsigned util_format_nr_test_cases = Elements(util_format_test_cases);
