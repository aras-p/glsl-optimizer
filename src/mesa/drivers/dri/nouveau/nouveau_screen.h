/*
 * Copyright (C) 2009 Francisco Jerez.
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

#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

struct nouveau_context;

struct nouveau_screen {
	__DRIscreen *dri_screen;

	struct nouveau_device *device;
	struct nouveau_channel *chan;

	struct nouveau_notifier *ntfy;
	struct nouveau_grobj *eng3d;
	struct nouveau_grobj *eng3dm;
	struct nouveau_grobj *surf3d;
	struct nouveau_grobj *m2mf;
	struct nouveau_grobj *surf2d;
	struct nouveau_grobj *rop;
	struct nouveau_grobj *patt;
	struct nouveau_grobj *rect;
	struct nouveau_grobj *swzsurf;
	struct nouveau_grobj *sifm;

	const struct nouveau_driver *driver;
	struct nouveau_context *context;
};

#endif
