/*
 * Copyright (C) 2006 Claudio Ciccani <klan@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

#include "cursors.h"


void GLUTAPIENTRY
glutSetCursor( int type )
{
     const unsigned char   *cursor;
     DFBSurfaceDescription  dsc;
     IDirectFBSurface      *shape;
     __u8                  *src, *msk;
     void                  *dst;
     int                    pitch;
     int                    x, y;
     
     if (!g_current || !g_current->window)
          return;

     if (g_current->cursor == type)
          return;
          
     switch (type) {
          case GLUT_CURSOR_RIGHT_ARROW:
               cursor = &cur_right_arrow[0];
               break;
          case GLUT_CURSOR_LEFT_ARROW:
               cursor = &cur_left_arrow[0];
               break;
          case GLUT_CURSOR_INFO:
               cursor = &cur_info[0];
               break;
          case GLUT_CURSOR_DESTROY:
               cursor = &cur_destroy[0];
               break;
          case GLUT_CURSOR_HELP:
               cursor = &cur_help[0];
               break;
          case GLUT_CURSOR_CYCLE:
               cursor = &cur_cycle[0];
               break;
          case GLUT_CURSOR_SPRAY:
               cursor = &cur_spray[0];
               break;
          case GLUT_CURSOR_WAIT:
               cursor = &cur_wait[0];
               break;
          case GLUT_CURSOR_TEXT:
               cursor = &cur_text[0];
               break;
          case GLUT_CURSOR_CROSSHAIR:
               cursor = &cur_crosshair[0];
               break;
          case GLUT_CURSOR_UP_DOWN:
               cursor = &cur_up_down[0];
               break;
          case GLUT_CURSOR_LEFT_RIGHT:
               cursor = &cur_left_right[0];
               break;
          case GLUT_CURSOR_TOP_SIDE:
               cursor = &cur_top_side[0];
               break;
          case GLUT_CURSOR_BOTTOM_SIDE:
               cursor = &cur_bottom_side[0];
               break;
          case GLUT_CURSOR_LEFT_SIDE:
               cursor = &cur_left_side[0];
               break;
          case GLUT_CURSOR_RIGHT_SIDE:
               cursor = &cur_right_side[0];
               break;
          case GLUT_CURSOR_TOP_LEFT_CORNER:
               cursor = &cur_top_left[0];
               break;
          case GLUT_CURSOR_TOP_RIGHT_CORNER:
               cursor = &cur_top_right[0];
               break;
          case GLUT_CURSOR_BOTTOM_RIGHT_CORNER:
               cursor = &cur_bottom_right[0];
               break;
          case GLUT_CURSOR_BOTTOM_LEFT_CORNER:
               cursor = &cur_bottom_left[0];
               break;
          default:
               cursor = &cur_right_arrow[0];
               break;
     }

     dsc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     dsc.width       = cursor[0];
     dsc.height      = cursor[0];
     dsc.pixelformat = DSPF_ARGB;
         
     if (dfb->CreateSurface( dfb, &dsc, &shape ))
          return;

     if (shape->Lock( shape, DSLF_WRITE, &dst, &pitch )) {
          shape->Release( shape );
          return;
     }
     
     src = (__u8*) &cursor[3];
     msk = src + cursor[0]*cursor[0]/8;
          
     for (y = 0; y < cursor[0]; y++) {
          for (x = 0; x < cursor[0]; x++) {
               int p = x & 7;
               
               ((__u32*)dst)[x] = ((src[x>>3] & (0x80 >> p)) ? 0 : 0x00ffffff) |
                                  ((msk[x>>3] & (0x80 >> p)) ? 0xff000000 : 0);
          }
               
          dst += pitch;
          src += 2;
          msk += 2;
     }

     shape->Unlock( shape );
          
     g_current->window->SetCursorShape( g_current->window, 
                                        shape, cursor[1], cursor[2] );          
     g_current->cursor = type;
     
     shape->Release( shape );           
}


void GLUTAPIENTRY
glutWarpPointer( int x, int y )
{
     if (g_current) {
          if (!g_game) {
               int wx, wy;
               g_current->window->GetPosition( g_current->window, &wx, &wy );
               primary->WarpCursor( primary, wx+x, wy+y );
          }
          else {
               g_current->cx = x;
               g_current->cy = y;
          }
     }
}


