/**************************************************************************
 *
 * Copyright 2009 Artur Wyszynski <harakash@gmail.com>
 * Copyright 2013 Alexander von Gluck IV <kallisti5@unixzen.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/
#ifndef __BBITMAP_WRAPPER_H__
#define __BBITMAP_WRAPPER_H__


#include <interface/GraphicsDefs.h>
#include <support/SupportDefs.h>


typedef void Bitmap;

#ifdef __cplusplus
extern "C" {
#endif


Bitmap* create_bitmap(int32 width, int32 height, color_space colorSpace);
void delete_bitmap(Bitmap* bitmap);

void copy_bitmap_bits(const Bitmap* bitmap, void* data, int32 length);
void import_bitmap_bits(const Bitmap* bitmap, void* data, int32 length,
	unsigned srcStride, color_space srcColorSpace);

void get_bitmap_size(const Bitmap* bitmap, int32* width, int32* height);
color_space get_bitmap_color_space(const Bitmap* bitmap);
int32 get_bitmap_bytes_per_row(const Bitmap* bitmap);
int32 get_bitmap_bits_length(const Bitmap* bitmap);

void dump_bitmap(const Bitmap* bitmap);


#ifdef __cplusplus
}
#endif


#endif /* __BBITMAP_WRAPPER_H__ */
