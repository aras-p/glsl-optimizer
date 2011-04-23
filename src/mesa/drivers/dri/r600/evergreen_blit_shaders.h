/*
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

const uint32_t evergreen_vs[] =
{
	0x00000004,
	0x80800400,
	0x0000a03c,
	0x95000688,
	0x00004000,
	0x15200688,
	0x00000000,
	0x00000000,
	0x3c000000,
	0x67961001,
#ifdef MESA_BIG_ENDIAN
	0x000a0000,
#else
	0x00080000,
#endif
	0x00000000,
	0x1c000000,
	0x67961000,
#ifdef MESA_BIG_ENDIAN
	0x00020008,
#else
	0x00000008,
#endif
	0x00000000,
};

const uint32_t evergreen_ps[] =
{
	0x00000003,
	0xa00c0000,
	0x00000008,
	0x80400000,
	0x00000000,
	0x95200688,
	0x00380400,
	0x00146b10,
	0x00380000,
	0x20146b10,
	0x00380400,
	0x40146b00,
	0x80380000,
	0x60146b00,
	0x00000000,
	0x00000000,
	0x00000010,
	0x000d1000,
	0xb0800000,
	0x00000000,
};

