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
#include <sys/time.h>

#include "GL/glu.h"

#include "internal.h"


int GLUTAPIENTRY 
glutGet( GLenum type )
{   
     switch (type) {
          case GLUT_WINDOW_X:
               if (g_current && g_current->window) {
                    int x;
                    g_current->window->GetPosition( g_current->window, &x, 0 );
                    return x;
               }
               break;
          case GLUT_WINDOW_Y:
               if (g_current && g_current->window) {
                    int y;
                    g_current->window->GetPosition( g_current->window, 0, &y );
                    return y;
               }
               break;
               
          case GLUT_WINDOW_WIDTH:
               if (g_current) {
                    int w;
                    g_current->surface->GetSize( g_current->surface, &w, 0 );
                    return w;
               }
               break;
          case GLUT_WINDOW_HEIGHT:
               if (g_current) {
                    int h;
                    g_current->surface->GetSize( g_current->surface, 0, &h );
                    return h;
               }
               break;
               
          case GLUT_WINDOW_BUFFER_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.buffer_size;
               }
               break;             
          case GLUT_WINDOW_STENCIL_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.stencil_size;
               }
               break;              
          case GLUT_WINDOW_DEPTH_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.depth_size;
               }
               break;
          case GLUT_WINDOW_RED_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.red_size;
               }
               break;
          case GLUT_WINDOW_GREEN_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.green_size;
               }
               break;
          case GLUT_WINDOW_BLUE_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.blue_size;
               }
               break;
          case GLUT_WINDOW_ALPHA_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.alpha_size;
               }
               break;
          case GLUT_WINDOW_ACCUM_RED_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.accum_red_size;
               }
               break;
          case GLUT_WINDOW_ACCUM_GREEN_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.accum_green_size;
               }
               break;
          case GLUT_WINDOW_ACCUM_BLUE_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.accum_blue_size;
               }
               break;
          case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.accum_alpha_size;
               }
               break;
          case GLUT_WINDOW_DOUBLEBUFFER:
               if (g_current) {
                    DFBGLAttributes a;
                    g_current->gl->GetAttributes( g_current->gl, &a );
                    return a.double_buffer;
               }
               break;                    
          
          case GLUT_WINDOW_RGBA:
               return 1;
          
          case GLUT_WINDOW_CURSOR:
               if (g_current)
                    return g_current->cursor;
               break;
               
          case GLUT_SCREEN_WIDTH:
               if (primary) {
                    DFBDisplayLayerConfig c;
                    primary->GetConfiguration( primary, &c );
                    return c.width;
               }
               break;
          case GLUT_SCREEN_HEIGHT:
               if (primary) {
                    DFBDisplayLayerConfig c;
                    primary->GetConfiguration( primary, &c );
                    return c.height;
               }
               break;
               
          case GLUT_INIT_DISPLAY_MODE:
               return g_display_mode;
          case GLUT_INIT_WINDOW_X:
               return g_xpos;
          case GLUT_INIT_WINDOW_Y:
               return g_ypos;
          case GLUT_INIT_WINDOW_WIDTH:
               return g_width;
          case GLUT_INIT_WINDOW_HEIGHT:
               return g_height;

          case GLUT_ELAPSED_TIME:
               {
                    static long long start = -1;
                    struct timeval   t;
               
                    gettimeofday( &t, NULL );
                    if (start == -1) {
                         start = t.tv_sec * 1000ll + t.tv_usec / 1000ll;
                         return 0;
                    }
                    return (t.tv_sec * 1000ll + t.tv_usec / 1000ll - start);
               }
               break;
               
          default:
               break;
     }
     
     return 0;
}


int GLUTAPIENTRY
glutLayerGet( GLenum type )
{
     return 0;
}

void GLUTAPIENTRY
glutReportErrors( void )
{
     GLenum error;
     
     while ((error = glGetError()) != GL_NO_ERROR)
          __glutWarning( "**OpenGL Error** %s", gluErrorString( error ) );
}


