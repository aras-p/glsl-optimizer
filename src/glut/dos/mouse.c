/*
 * DOS/DJGPP Mesa Utility Toolkit
 * Version:  1.0
 *
 * Copyright (C) 2005  Daniel Borca   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DANIEL BORCA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "internal.h"


int _glut_mouse;
int _glut_mouse_x = 0, _glut_mouse_y = 0;


void
_glut_mouse_init (void)
{
   if ((_glut_mouse = pc_install_mouse())) {
      pc_mouse_area(_glut_current->xpos, _glut_current->ypos, _glut_current->xpos + _glut_current->width - 1, _glut_current->ypos + _glut_current->height - 1);

      _glut_current->show_mouse = (_glut_current->mouse || _glut_current->motion || _glut_current->passive);
   }
}


void APIENTRY
glutSetCursor (int cursor)
{
   /* XXX completely futile until full mouse support (maybe never) */
}


void APIENTRY
glutWarpPointer (int x, int y)
{
   pc_warp_mouse(x, y);
}
