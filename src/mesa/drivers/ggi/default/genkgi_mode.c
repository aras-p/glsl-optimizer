/* $Id: genkgi_mode.c,v 1.1 1999/08/21 05:59:29 jtaylor Exp $
******************************************************************************

   display-fbdev-kgicon-generic-mesa

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

//#include <linux/fb.h>

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>
#include "genkgi.h"

int GGIMesa_genkgi_getapi(ggi_visual *vis, int num, char *apiname, char *arguments)
{
	genkgi_hook_mesa *priv = GENKGI_PRIVATE(vis);
	
	gl_ggiDEBUG("Entered mesa_genkgi_getapi, num=%d\n", num);
	
	strcpy(arguments, "");

	switch(num) 
	{
		case 0:
		if (priv->have_accel)
		{
			strcpy(apiname, priv->accel);
			return 0;
		}
		break;
	}
	return -1;
}
