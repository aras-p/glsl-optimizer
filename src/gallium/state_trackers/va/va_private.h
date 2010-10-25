/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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
 
 #ifndef VA_PRIVATE_H
 #define VA_PRIVATE_H
 
 #include <va/va.h>
 #include <va/va_backend.h>
 #define VA_DEBUG(_str,...) debug_printf("[Gallium VA backend]: " _str,__VA_ARGS__)
 #define VA_INFO(_str,...) VA_DEBUG("INFO: " _str,__VA_ARGS__)
 #define VA_WARNING(_str,...) VA_DEBUG("WARNING: " _str,__VA_ARGS__)
 #define VA_ERROR(_str,...) VA_DEBUG("ERROR: " _str,__VA_ARGS__)

// Public functions:
VAStatus __vaDriverInit_0_31 (VADriverContextP ctx);

// Private functions:
struct VADriverVTable vlVaGetVtable();
VAStatus vlVaQueryImageFormats (VADriverContextP ctx,VAImageFormat *format_list,int *num_formats);
VAStatus vlVaQuerySubpictureFormats(VADriverContextP ctx,VAImageFormat *format_list,unsigned int *flags,unsigned int *num_formats);
VAStatus vlVaCreateImage(VADriverContextP ctx,VAImageFormat *format,int width,int height,VAImage *image);
 
 #endif // VA_PRIVATE_H
