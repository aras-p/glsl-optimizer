/*
   (c) Copyright 2001  convergence integrated media GmbH.
   All rights reserved.

   Written by Denis Oliver Kropp <dok@convergence.de> and
              Andreas Hundt <andi@convergence.de>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <directfb.h>
#include <directfbgl.h>


typedef struct {
     IDirectFBWindow  *window;
     IDirectFBSurface *surface;
     IDirectFBGL      *gl;

     int               width;
     int               height;
     
     unsigned long     last_time;
     int               frames;
     float             fps;
} Context;

static const GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};

static IDirectFB             *dfb;
static IDirectFBDisplayLayer *layer;
static IDirectFBFont         *font;
static IDirectFBEventBuffer  *events = NULL;

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
        do {                                                                   \
           ret = x;                                                            \
           if (ret != DFB_OK) {                                                \
              fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );           \
              DirectFBErrorFatal( #x, ret );                                   \
           }                                                                   \
        } while (0)


static inline unsigned long get_millis()
{
  struct timeval tv;

  gettimeofday (&tv, NULL);
  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


static void
setup( Context *context )
{
     GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};

     context->surface->GetSize( context->surface,
                                &context->width, &context->height );

     context->gl->Lock( context->gl );

     glLightfv(GL_LIGHT0, GL_POSITION, pos);
     glEnable(GL_CULL_FACE);
     glEnable(GL_LIGHTING);
     glEnable(GL_LIGHT0);
     glEnable(GL_DEPTH_TEST);
     
     glViewport(0, 0, context->width, context->height);
     
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     gluPerspective(70.0, context->width / (float) context->height, 1.0, 80.0);
     
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     glTranslatef(0.0, 0.0, -40.0);
     
     context->gl->Unlock( context->gl );
}

static void
update( Context *context )
{
     unsigned long     t;
     IDirectFBSurface *surface = context->surface;
     static __u8  r = 0, g = 0, b = 0;
    
     
     context->gl->Lock( context->gl );

     glClearColor( r++/255.0, g++/255.0, b++/255.0, 1.0 );
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     
     context->gl->Unlock( context->gl );
     
     if (context->fps) {
          char buf[16];

          snprintf(buf, sizeof(buf), "%.1f FPS\n", context->fps);

          surface->SetColor( surface, 0xff, 0x00, 0x00, 0xff );
          surface->DrawString( surface, buf, -1,
                               context->width - 5, 5, DSTF_TOPRIGHT );
     }
     
     surface->Flip( surface, NULL, 0 );

     context->frames++;

     t = get_millis();
     if (t - context->last_time >= 2000) {
          float seconds = (t - context->last_time) / 1000.0f;

          context->fps = context->frames / seconds;

          context->last_time = t;
          context->frames    = 0;
     }
}

int
main( int argc, char *argv[] )
{
     DFBResult ret;
     int       i;
     int       quit = 0;
     const int num = 2;
     Context   contexts[num];

     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the super interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ));

     /* create the default font */
     DFBCHECK(dfb->CreateFont( dfb, NULL, NULL, &font ));

     for (i=0; i<num; i++) {
          IDirectFBWindow      *window;
          IDirectFBSurface     *surface;
          IDirectFBGL          *gl;
          DFBWindowDescription  desc;

          desc.flags  = DWDESC_POSX | DWDESC_POSY |
                        DWDESC_WIDTH | DWDESC_HEIGHT;
          desc.posx   = (i%3) * 200 + 10;
          desc.posy   = (i/3) * 200 + 10;
          desc.width  = 180;
          desc.height = 180;

          DFBCHECK(layer->CreateWindow( layer, &desc, &window ));
          DFBCHECK(window->GetSurface( window, &surface ));
          DFBCHECK(surface->GetGL( surface, &gl ));

          contexts[i].window    = window;
          contexts[i].surface   = surface;
          contexts[i].gl        = gl;

          contexts[i].last_time = get_millis();
          contexts[i].frames    = 0;
          contexts[i].fps       = 0;

          setup( &contexts[i] );

          if (events)
               DFBCHECK(window->AttachEventBuffer( window, events ));
          else
               DFBCHECK(window->CreateEventBuffer( window, &events ));
          
          DFBCHECK(surface->SetFont( surface, font ));

          window->SetOpacity( window, 0xff );
     }
     
     while (!quit) {
          DFBWindowEvent evt;

          for (i=0; i<num; i++)
               update( &contexts[i] );
          
          while (events->GetEvent( events, DFB_EVENT(&evt) ) == DFB_OK) {
               switch (evt.type) {
                    case DWET_KEYDOWN:
                         switch (evt.key_symbol) {
                              case DIKS_ESCAPE:
                                   quit = 1;
                                   break;

                              default:
                                   break;
                         }
                         break;

                    default:
                         break;
               }
          }
     }

     events->Release( events );

     for (i=0; i<num; i++) {
          contexts[i].gl->Release( contexts[i].gl );
          contexts[i].surface->Release( contexts[i].surface );
          contexts[i].window->Release( contexts[i].window );
     }

     font->Release( font );
     layer->Release( layer );
     dfb->Release( dfb );

     return 0;
}

