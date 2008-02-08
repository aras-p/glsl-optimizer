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
#include <unistd.h>

#include "internal.h"


/*****************************************************************************/

static __GlutWindow *g_stack = NULL;

/*****************************************************************************/


__GlutWindow* 
__glutCreateWindow( GLboolean fullscreen )
{
     __GlutWindow *new;
     DFBResult     ret;
     static int    curid = 1;

     new = calloc( 1, sizeof(__GlutWindow) );
     if (!new)
          __glutFatalError( "out of memory" );
     
     new->id = curid++;

     if (fullscreen) {
          DFBDisplayLayerConfig      config;
          DFBDisplayLayerConfigFlags fail = 0;
          
          config.flags  = DLCONF_WIDTH | DLCONF_HEIGHT |
                          DLCONF_BUFFERMODE;
          config.width  = g_width;
          config.height = g_height;
          
          if (g_display_mode & GLUT_DOUBLE)
               config.buffermode = DLBM_BACKVIDEO;
          else
               config.buffermode = DLBM_FRONTONLY;
               
          if (g_bpp) {
               config.flags |= DLCONF_PIXELFORMAT;
               
               switch (g_bpp) {
                    case 8:
                         config.pixelformat = DSPF_RGB332;
                         break;
                    case 12:
                         config.pixelformat = DSPF_ARGB4444;
                         break;
                    case 15:
                         config.pixelformat = DSPF_ARGB1555;
                         break;
                    case 16:
                         config.pixelformat = DSPF_RGB16;
                         break;
                    case 24:
                    case 32:
                         config.pixelformat = DSPF_RGB32;
                         break;
                    default:
                         config.flags &= ~DLCONF_PIXELFORMAT;
                         break;
               }
          }
               
          primary->TestConfiguration( primary, &config, &fail );
          config.flags &= ~fail;
          primary->SetConfiguration( primary, &config );
               
          ret = primary->GetSurface( primary, &new->surface );
          if (ret) {
               DirectFBError( "IDirectFBDisplayLayer::GetSurface()", ret );
               free( new );
               return NULL;
          }
          
          ret = new->surface->GetGL( new->surface, &new->gl );
          if (ret) {
               DirectFBError( "IDirectFBSurface::GetGL()", ret );
               new->surface->Release( new->surface );
               free( new );
               return NULL;
          }
          
          events->Reset( events );
          if (keyboard)
               keyboard->AttachEventBuffer( keyboard, events );
          if (mouse)
               mouse->AttachEventBuffer( mouse, events );
          if (joystick)
               joystick->AttachEventBuffer( joystick, events );
               
          new->visible = GL_TRUE;    
     }
     else {
          DFBWindowDescription dsc;
     
          dsc.flags  = DWDESC_CAPS | DWDESC_POSX | DWDESC_POSY | 
                       DWDESC_WIDTH | DWDESC_HEIGHT;
          dsc.caps   = DWCAPS_NONE;
          dsc.posx   = g_xpos;
          dsc.posy   = g_ypos;
          dsc.width  = g_width;
          dsc.height = g_height;

          if (g_display_mode & GLUT_DOUBLE)
               dsc.caps |= DWCAPS_DOUBLEBUFFER;
          if (g_display_mode & GLUT_ALPHA)
               dsc.caps |= DWCAPS_ALPHACHANNEL;

          ret = primary->CreateWindow( primary, &dsc, &new->window );
          if (ret) {
               DirectFBError( "IDirectFBDisplayLayer::CreateWindow()", ret );
               free( new );
               return NULL;
          }

          new->window->GetID( new->window, &new->wid );
     
          ret = new->window->GetSurface( new->window, &new->surface );
          if (ret) {
               DirectFBError( "IDirectFBWindow::GetSurface()", ret );
               new->window->Release( new->window );
               free( new );
               return NULL;
          }

          ret = new->surface->GetGL( new->surface, &new->gl );
          if (ret) {
               DirectFBError( "IDirectFBSurface::GetGl()", ret );
               new->surface->Release( new->surface );
               new->window->Release( new->window );
               free( new );
               return NULL;
          }
          
          new->window->AttachEventBuffer( new->window, events );
          /* enable only handled events */
          new->window->DisableEvents( new->window, DWET_ALL );
          new->window->EnableEvents( new->window, DWET_KEYDOWN    | DWET_KEYUP    |
                                                  DWET_BUTTONDOWN | DWET_BUTTONUP |
                                                  DWET_ENTER      | DWET_LEAVE    |
                                                  DWET_MOTION     | DWET_SIZE );
          
          new->req.flags |= WINDOW_REQUEST_SHOW;
     }

     new->mode = g_display_mode;
     
     new->reshape    = GL_TRUE;
     new->visibility = GL_TRUE;
     new->redisplay  = GL_TRUE;
     
     if (g_stack) {
          new->prev = g_stack->prev;
          g_stack->prev->next = new;
          g_stack->prev = new;
     }
     else {
          new->prev = new;
          g_stack = new;
     }     
   
     return new;
}


__GlutWindow*
__glutFindWindow( DFBWindowID id )
{
     __GlutWindow *cur;

     for (cur = g_stack; cur; cur = cur->next) {
          if (cur->wid == id)
               return cur;
     }

     __glutFatalError( "Window %d not found", id );
     
     return NULL;
}


void
__glutSetWindow( __GlutWindow *window )
{
     if (g_current) {
          if (g_current == window)
               return;
          g_current->gl->Unlock( g_current->gl );
     }
     
     if (window)     
          window->gl->Lock( window->gl );
     g_current = window;
}


void
__glutHandleWindows( void )
{
     __GlutWindow *cur = g_stack;
     
     while (cur) {
          __GlutWindow *next      = cur->next;
          GLboolean     displayed = GL_FALSE;
          
          if (cur->window && cur->req.flags) {
               if (cur == g_current)
                    cur->gl->Unlock( cur->gl );

               if (cur->req.flags & WINDOW_REQUEST_DESTROY) {
                    __glutDestroyWindow( cur );
                    cur = next;
                    continue;
               }
     
               if (cur->req.flags & WINDOW_REQUEST_POSITION) {
                    cur->window->MoveTo( cur->window, 
                                         cur->req.x, cur->req.y );
               }
           
               if (cur->req.flags & WINDOW_REQUEST_RESIZE) {
                    cur->window->Resize( cur->window,
                                        cur->req.w, cur->req.h );
                    cur->reshape = GL_TRUE;
                    cur->redisplay = GL_TRUE;
               }
     
               if (cur->req.flags & WINDOW_REQUEST_RESTACK) {
                    while (cur->req.z > 0) {
                         if (cur->req.z >= +1000) {
                              cur->window->RaiseToTop( cur->window );
                              cur->req.z = 0;
                              break;
                         }
               
                         cur->window->Raise( cur->window );
                         cur->req.z--;
                    }
          
                    while (cur->req.z < 0) {
                         if (cur->req.z <= -1000) {
                              cur->window->LowerToBottom( cur->window );
                              cur->req.z = 0;
                              break;
                         }
               
                         cur->window->Lower( cur->window );
                         cur->req.z++;
                    }
               }
     
               if (cur->req.flags & WINDOW_REQUEST_SHOW) {
                    cur->window->SetOpacity( cur->window, 0xff );
                    cur->visible = GL_TRUE;
                    cur->visibility = GL_TRUE;
               }
               else if (cur->req.flags & WINDOW_REQUEST_HIDE) {
                    cur->window->SetOpacity( cur->window, 0x00 );
                    cur->visible = GL_FALSE;
                    cur->visibility = GL_TRUE;
               }
 
               cur->req.flags = 0;

               if (cur == g_current)
                    cur->gl->Lock( cur->gl );
          }
          
          if (cur->reshape && reshape_func) {
               int w, h;
               g_idle = GL_FALSE;                    
               cur->surface->GetSize( cur->surface, &w, &h ); 
               __glutSetWindow( cur );
               reshape_func( w, h );
               displayed = GL_TRUE;
          }
          
          if (cur->visibility && visibility_func) {
               g_idle = GL_FALSE;
               __glutSetWindow( cur );
               visibility_func( cur->visible ? GLUT_VISIBLE : GLUT_NOT_VISIBLE );
               displayed = GL_TRUE;
          }

          if (cur->redisplay && display_func) {
               g_idle = GL_FALSE;
               __glutSetWindow( cur );
               display_func();
               displayed = GL_TRUE;
          }
          
          if (displayed && cur->window && cur->visible) {
               if (!(cur->mode & GLUT_DOUBLE)) {
                    cur->gl->Unlock( cur->gl );
                    cur->surface->Flip( cur->surface, NULL, 0 );
                    cur->gl->Lock( cur->gl );
               }
          }
               
          cur->reshape    = GL_FALSE;
          cur->visibility = GL_FALSE;
          cur->redisplay  = GL_FALSE;

          cur = next;
     }
}


void
__glutDestroyWindow( __GlutWindow *window )
{
     __GlutWindow *next = window->next;
     __GlutWindow *prev = window->prev;
     
     __glutAssert( window != NULL );
     
     if (window == g_current)
          g_current = NULL;
     if (window == g_game)
          g_game = NULL;
     
     window->gl->Unlock( window->gl );
     window->gl->Release( window->gl );
     window->surface->Release( window->surface );
     
     if (window->window) {
#if DIRECTFB_VERSION_CODE >= VERSION_CODE(0,9,26)
          window->window->DetachEventBuffer( window->window, events );
#else
          window->window->Destroy( window->window );
#endif
          window->window->Release( window->window );
     }
     else {
#if DIRECTFB_VERSION_CODE >= VERSION_CODE(0,9,26)
          if (joystick)
               joystick->DetachEventBuffer( joystick, events );
          if (mouse)
               mouse->DetachEventBuffer( mouse, events );
          if (keyboard)
               keyboard->DetachEventBuffer( keyboard, events );
#endif
          events->Reset( events );
     }
     
     free( window );

     if (next)
          next->prev = prev;
     else
          g_stack->prev = prev;

     if (window == g_stack)
          g_stack = next;
     else
          prev->next = next;
}


void
__glutDestroyWindows( void )
{
     __GlutWindow *cur = g_stack;
     
     while (cur) {
          __GlutWindow *next = cur->next;
          __glutDestroyWindow( cur );
          cur = next;
     }
} 


int GLUTAPIENTRY 
glutCreateWindow( const char *title )
{
     __GlutWindow *window;
     
     if (getenv( "__GLUT_GAME_MODE" ))
          return glutEnterGameMode();

     glutInit( NULL, NULL );
     
     window = __glutCreateWindow( GL_FALSE );
     if (!window)
          return 0;
          
     __glutSetWindow( window );
     glutSetCursor( GLUT_CURSOR_INHERIT );
   
     return window->id;
}


int GLUTAPIENTRY 
glutCreateSubWindow( int win, int x, int y, int width, int height )
{
     return GL_FALSE;
}


void GLUTAPIENTRY 
glutDestroyWindow( int win )
{
     __GlutWindow *cur;
     
     for (cur = g_stack; cur; cur = cur->next) {
          if (cur->id == win) {
               if (cur->window)
                    cur->window->Destroy( cur->window );
               
               cur->req.flags |= WINDOW_REQUEST_DESTROY;  
               break;
          }
     }
}


void GLUTAPIENTRY 
glutPostRedisplay( void )
{
     if (g_current)
          g_current->redisplay = GL_TRUE;
}


void GLUTAPIENTRY 
glutPostWindowRedisplay( int win )
{
     __GlutWindow *cur;
     
     for (cur = g_stack; cur; cur = cur->next) {
          if (cur->id == win) {
               cur->redisplay = GL_TRUE;
               break;
          }
     }
}


void GLUTAPIENTRY 
glutSwapBuffers( void )
{
     if (g_current) {
          g_current->gl->Unlock( g_current->gl );    
          g_current->surface->Flip( g_current->surface, NULL, 0 );    
          g_current->gl->Lock( g_current->gl );
     }
}


int GLUTAPIENTRY 
glutGetWindow( void )
{
     return (g_current) ? g_current->id : 0;
}


void GLUTAPIENTRY 
glutSetWindow( int win )
{
     __GlutWindow *cur;

     if (g_current && g_current->id == win)
          return;
     
     for (cur = g_stack; cur; cur = cur->next) {
          if (cur->id == win) {
               __glutSetWindow( cur );
               break;
          }
     }
}


void GLUTAPIENTRY 
glutSetWindowTitle( const char *title )
{
}


void GLUTAPIENTRY 
glutSetIconTitle( const char *title )
{
}


void GLUTAPIENTRY 
glutFullScreen( void )
{          
     if (g_current && !g_game) {
          DFBDisplayLayerConfig config;
          
          primary->GetConfiguration( primary, &config );
          
          g_current->req.flags |= WINDOW_REQUEST_POSITION |
                                  WINDOW_REQUEST_RESIZE   |
                                  WINDOW_REQUEST_RESTACK;
          g_current->req.x = 0;
          g_current->req.y = 0;
          g_current->req.w = config.width;
          g_current->req.h = config.height;
          g_current->req.z = 1000;
     }
}   


void GLUTAPIENTRY 
glutPositionWindow( int x, int y )
{
     if (g_current && !g_game) {
          g_current->req.flags |= WINDOW_REQUEST_POSITION; 
          g_current->req.x = x;
          g_current->req.y = y;
     }
}


void GLUTAPIENTRY 
glutReshapeWindow( int width, int height )
{
     if (g_current && !g_game) {
          g_current->req.flags |= WINDOW_REQUEST_RESIZE;
          g_current->req.w = width;
          g_current->req.h = height;
     }
}


void GLUTAPIENTRY 
glutPopWindow( void )
{
     if (g_current && !g_game) {
          g_current->req.flags |= WINDOW_REQUEST_RESTACK;
          g_current->req.z--;
     }
}


void GLUTAPIENTRY 
glutPushWindow( void )
{
     if (g_current && !g_game) {
          g_current->req.flags |= WINDOW_REQUEST_RESTACK;
          g_current->req.z++;
     }
}


void GLUTAPIENTRY 
glutIconifyWindow( void )
{
}


void GLUTAPIENTRY 
glutShowWindow( void )
{
     if (g_current && !g_game) {
          g_current->req.flags |= WINDOW_REQUEST_SHOW;
          g_current->req.flags &= ~WINDOW_REQUEST_HIDE;
     }
}


void GLUTAPIENTRY 
glutHideWindow( void )
{
     if (g_current && !g_game) {
          g_current->req.flags |= WINDOW_REQUEST_HIDE;
          g_current->req.flags &= ~WINDOW_REQUEST_SHOW;
     }
}

