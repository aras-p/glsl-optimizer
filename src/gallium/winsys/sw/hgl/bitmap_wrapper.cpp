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


#include <stdio.h>
#include <interface/Bitmap.h>
#include <storage/File.h>
#include <support/String.h>
#include <translation/BitmapStream.h>
#include <translation/TranslatorRoster.h>

#include "bitmap_wrapper.h"


extern "C" {
static int frameNo = 0;


Bitmap*
create_bitmap(int32 width, int32 height, color_space colorSpace)
{
	BBitmap *bb = new BBitmap(BRect(0, 0, width, height), colorSpace);
	if (bb)
		return (Bitmap*)bb;
	return NULL;
}


void
get_bitmap_size(const Bitmap* bitmap, int32* width, int32* height)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb && width && height) {
		uint32 w = bb->Bounds().IntegerWidth() + 1;
		uint32 h = bb->Bounds().IntegerHeight() + 1;
		*width = w;
		*height = h;
	}
}


color_space
get_bitmap_color_space(const Bitmap* bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb)
		return bb->ColorSpace();
	return B_NO_COLOR_SPACE;
}


void
copy_bitmap_bits(const Bitmap* bitmap, void* data, int32 length)
{
	BBitmap *bb = (BBitmap*)bitmap;

	// We assume the data is 1:1 the format of the bitmap
	if (bb)
		bb->ImportBits(data, length, bb->BytesPerRow(), 0, bb->ColorSpace());
}


void
import_bitmap_bits(const Bitmap* bitmap, void* data, int32 length,
	unsigned srcStride, color_space srcColorSpace)
{
	BBitmap *bb = (BBitmap*)bitmap;

	// Import image and adjust image format from source to dest
	if (bb)
		bb->ImportBits(data, length, srcStride, 0, srcColorSpace);
}


void
delete_bitmap(Bitmap* bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	delete bb;
}


int32
get_bitmap_bytes_per_row(const Bitmap* bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb)
		return bb->BytesPerRow();
	return 0;
}


int32
get_bitmap_bits_length(const Bitmap* bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb)
		return bb->BitsLength();
	return 0;
}


void
dump_bitmap(const Bitmap* bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (!bb)
		return;

	BString filename("/boot/home/frame_");
	filename << (int32)frameNo << ".png";

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BBitmapStream stream(bb);
	BFile dump(filename, B_CREATE_FILE | B_WRITE_ONLY);

	roster->Translate(&stream, NULL, NULL, &dump, 0);

	frameNo++;
}

}
