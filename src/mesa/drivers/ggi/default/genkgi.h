/* $Id: genkgi.h,v 1.1 1999/08/21 05:59:29 jtaylor Exp $
******************************************************************************

   GGIMesa - KGIcon specific overrides for fbcon-mesa
   API header

   Copyright (C) 1999 Creative Labs

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


#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/display/fbdev.h>

//ggifunc_setmode GGIMesa_genkgi_setmode;
ggifunc_getapi GGIMesa_genkgi_getapi;

typedef struct genkgi_hook_mesa
{
	char accel[100];
	int have_accel;
	void *accelpriv;
} genkgi_hook_mesa;

#define GENKGI_PRIVATE(vis) ((genkgi_hook_mesa *)FBDEV_PRIV_MESA(vis)->accelpriv)
