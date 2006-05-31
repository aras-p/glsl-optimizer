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


/*****************************************************************************/

static int g_display_changed = 0;

/*****************************************************************************/


void GLUTAPIENTRY
glutGameModeString( const char *string )
{
     int   x, y, bpp;
     char *tmp;
     
     if (!string)
          return;
          
     tmp = strchr( string, 'x' );
     if (tmp) {
          x = strtol( string, NULL, 10 );
          y = strtol( tmp+1, NULL, 10 );
          
          if (x > 0 && y > 0) {
               g_width  = x;
               g_height = y;
          }
     }
     
     tmp = strchr( string, ':' );
     if (tmp) {
          bpp = strtol( tmp+1, NULL, 10 );
          
          if (bpp > 0)
               g_bpp = bpp;
     }
}


int GLUTAPIENTRY
glutEnterGameMode( void )
{
     DFBDisplayLayerConfig prev, cur;

     glutInit( NULL, NULL );
     
     primary->GetConfiguration( primary, &prev );
     primary->SetCooperativeLevel( primary, DLSCL_EXCLUSIVE );
     
     if (g_game)
          __glutDestroyWindow( g_game );
          
     g_game = __glutCreateWindow( GL_TRUE );
     if (!g_game)
          return 0;
          
     __glutSetWindow( g_game );
     g_game->cursor = GLUT_CURSOR_NONE;

     primary->GetConfiguration( primary, &cur );
     g_display_changed = (cur.width       != prev.width      || 
                          cur.height      != prev.height     ||
                          cur.pixelformat != prev.pixelformat);
     
     return g_game->id;
}


void GLUTAPIENTRY
glutLeaveGameMode( void )
{
     if (g_game)
          __glutDestroyWindow( g_game );

     primary->SetCooperativeLevel( primary, DLSCL_ADMINISTRATIVE );
}


int GLUTAPIENTRY
glutGameModeGet( GLenum type )
{
     switch (type) {
          case GLUT_GAME_MODE_ACTIVE:
               return (g_game != NULL);
          case GLUT_GAME_MODE_POSSIBLE:
               if (primary) {
                    DFBDisplayLayerConfig c;
                    c.flags  = DLCONF_WIDTH | DLCONF_HEIGHT;
                    c.width  = g_width;
                    c.height = g_height;
                    /* XXX: bpp */
                    if (primary->TestConfiguration( primary, &c, 0 ) == DFB_OK)
                         return 1;
               }
               break;
          case GLUT_GAME_MODE_WIDTH:
               if (g_game) {
                    int w;
                    g_game->surface->GetSize( g_game->surface, &w, 0 );
                    return w;
               }
               break;
          case GLUT_GAME_MODE_HEIGHT:
               if (g_game) {
                    int h;
                    g_game->surface->GetSize( g_game->surface, 0, &h );
                    return h;
               }
               break;
          case GLUT_GAME_MODE_PIXEL_DEPTH:
               if (g_game) {
                    DFBSurfacePixelFormat f;
                    g_game->surface->GetPixelFormat( g_game->surface, &f );
                    return DFB_COLOR_BITS_PER_PIXEL( f );
               }
               break;
          case GLUT_GAME_MODE_REFRESH_RATE:
               return 60; /* assume 60hz */
          case GLUT_GAME_MODE_DISPLAY_CHANGED:
               return g_display_changed;
          default:
               break;
     }
     
     return 0;
}


        
