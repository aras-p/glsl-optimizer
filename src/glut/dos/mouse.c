/*
 * Mesa 3-D graphics library
 * Version:  3.4
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * DOS/DJGPP glut driver v1.3 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "glutint.h"


int g_mouse;
int g_mouse_x = 0, g_mouse_y = 0;


void
__glutInitMouse (void)
{
   if ((g_mouse = pc_install_mouse())) {
      pc_mouse_area(g_curwin->xpos, g_curwin->ypos, g_curwin->xpos + g_curwin->width - 1, g_curwin->ypos + g_curwin->height - 1);

      g_curwin->show_mouse = (g_curwin->mouse || g_curwin->motion || g_curwin->passive);
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
