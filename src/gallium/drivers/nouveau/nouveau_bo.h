/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_BO_H__
#define __NOUVEAU_BO_H__

/* Relocation/Buffer type flags */
#define NOUVEAU_BO_VRAM  (1 << 0)
#define NOUVEAU_BO_GART  (1 << 1)
#define NOUVEAU_BO_RD    (1 << 2)
#define NOUVEAU_BO_WR    (1 << 3)
#define NOUVEAU_BO_RDWR  (NOUVEAU_BO_RD | NOUVEAU_BO_WR)
#define NOUVEAU_BO_MAP   (1 << 4)
#define NOUVEAU_BO_PIN   (1 << 5)
#define NOUVEAU_BO_LOW   (1 << 6)
#define NOUVEAU_BO_HIGH  (1 << 7)
#define NOUVEAU_BO_OR    (1 << 8)
#define NOUVEAU_BO_LOCAL (1 << 9)
#define NOUVEAU_BO_TILED (1 << 10)
#define NOUVEAU_BO_ZTILE (1 << 11)
#define NOUVEAU_BO_DUMMY (1 << 31)

struct nouveau_bo {
	struct nouveau_device *device;
	uint64_t handle;

	uint64_t size;
	void *map;

	uint32_t flags;
	uint64_t offset;
};

#endif
