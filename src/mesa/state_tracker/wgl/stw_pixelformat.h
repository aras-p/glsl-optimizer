/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef PIXELFORMAT_H
#define PIXELFORMAT_H

#define PF_FLAG_DOUBLEBUFFER  0x00000001
#define PF_FLAG_MULTISAMPLED  0x00000002

struct pixelformat_color_info
{
   uint redbits;
   uint redshift;
   uint greenbits;
   uint greenshift;
   uint bluebits;
   uint blueshift;
};

struct pixelformat_alpha_info
{
   uint alphabits;
   uint alphashift;
};

struct pixelformat_depth_info
{
   uint depthbits;
   uint stencilbits;
};

struct pixelformat_info
{
   uint flags;
   struct pixelformat_color_info color;
   struct pixelformat_alpha_info alpha;
   struct pixelformat_depth_info depth;
};

void
pixelformat_init( void );

uint
pixelformat_get_count( void );

uint
pixelformat_get_extended_count( void );

const struct pixelformat_info *
pixelformat_get_info( uint index );

#endif /* PIXELFORMAT_H */
