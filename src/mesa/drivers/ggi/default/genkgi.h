/* 
******************************************************************************

   GGIMesa - KGIcon specific overrides for fbcon-mesa
   API header

   Copyright (C) 1999 Jon Taylor [taylorj@ggi-project.org]

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#ifndef _GENKGI_MESA_H
#define _GENKGI_MESA_H

#undef KGI_USE_PPBUFS

#include <unistd.h>
#include <sys/mman.h>

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/display_fbdev.h>
#include <kgi/kgi.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

/* FIXME: LibGGI needs to export its genkgi.h */
struct genkgi_priv
{
	ggi_gc *mapped_gc;
	unsigned int gc_size;
	ggifunc_drawline *drawline;
	ggifunc_drawbox *drawbox;
	ggifunc_fillscreen *fillscreen;
	int fd_gc;
	int close_gc;
	int fd_kgicommand;
	uint8 *mapped_kgicommand;
	uint8 *kgicommand_ptr;
	unsigned int kgicommand_buffersize;
};

#define GENKGI_PRIV(vis) ((struct genkgi_priv *)FBDEV_PRIV(vis)->accelpriv)

extern ggifunc_getapi GGIMesa_genkgi_getapi;
extern ggifunc_flush  GGIMesa_genkgi_flush;

struct genkgi_priv_mesa
{
	char accel[100];
	int have_accel;
	void *accelpriv; /* Private data of subdrivers */
	struct genkgi_priv *oldpriv; /* LibGGI's private data */
};

#define GENKGI_PRIV_MESA(vis) ((struct genkgi_priv_mesa *)FBDEV_PRIV_MESA(vis)->accelpriv)

#endif /* _GENKHI_MESA_H */
