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

#include <directfb.h>
#include <directfbgl.h>

#include <GL/glu.h>

#include "util/showbuffer.c"
#include "util/readtex.c"


/* the super interface */
IDirectFB *dfb;

/* the primary surface (surface of primary layer) */
IDirectFBSurface *primary;

/* the GL context */
IDirectFBGL *primary_gl;

/* our font */
IDirectFBFont *font;

/* event buffer */
IDirectFBEventBuffer *events;

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
        {                                                                      \
           err = x;                                                            \
           if (err != DFB_OK) {                                                \
              fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );           \
              DirectFBErrorFatal( #x, err );                                   \
           }                                                                   \
        }

static int screen_width, screen_height;

static unsigned long T0 = 0;
static GLint Frames = 0;
static GLfloat fps = 0;

static inline unsigned long get_millis()
{
  struct timeval tv;

  gettimeofday (&tv, NULL);
  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/*******************************/

#define DEG2RAD (3.14159/180.0)

#define TABLE_TEXTURE "../images/tile.rgb"

static GLint ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

#define MAX_OBJECTS 2
static GLint table_list;
static GLint objects_list[MAX_OBJECTS];

static GLfloat xrot, yrot;
static GLfloat spin;

static GLint Width = 400, Height = 300;
static GLenum ShowBuffer = GL_NONE;


static void make_table( void )
{
   static GLfloat table_mat[] = { 1.0, 1.0, 1.0, 0.6 };
   static GLfloat gray[] = { 0.4, 0.4, 0.4, 1.0 };

   table_list = glGenLists(1);
   glNewList( table_list, GL_COMPILE );

   /* load table's texture */
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, table_mat );
/*   glMaterialfv( GL_FRONT, GL_EMISSION, gray );*/
   glMaterialfv( GL_FRONT, GL_DIFFUSE, table_mat );
   glMaterialfv( GL_FRONT, GL_AMBIENT, gray );

   /* draw textured square for the table */
   glPushMatrix();
   glScalef( 4.0, 4.0, 4.0 );
   glBegin( GL_POLYGON );
   glNormal3f( 0.0, 1.0, 0.0 );
   glTexCoord2f( 0.0, 0.0 );   glVertex3f( -1.0, 0.0,  1.0 );
   glTexCoord2f( 1.0, 0.0 );   glVertex3f(  1.0, 0.0,  1.0 );
   glTexCoord2f( 1.0, 1.0 );   glVertex3f(  1.0, 0.0, -1.0 );
   glTexCoord2f( 0.0, 1.0 );   glVertex3f( -1.0, 0.0, -1.0 );
   glEnd();
   glPopMatrix();

   glDisable( GL_TEXTURE_2D );

   glEndList();
}


static void make_objects( void )
{
   GLUquadricObj *q;

   static GLfloat cyan[] = { 0.0, 1.0, 1.0, 1.0 };
   static GLfloat green[] = { 0.2, 1.0, 0.2, 1.0 };
   static GLfloat black[] = { 0.0, 0.0, 0.0, 0.0 };

   q = gluNewQuadric();
   gluQuadricDrawStyle( q, GLU_FILL );
   gluQuadricNormals( q, GLU_SMOOTH );

   objects_list[0] = glGenLists(1);
   glNewList( objects_list[0], GL_COMPILE );
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cyan );
   glMaterialfv( GL_FRONT, GL_EMISSION, black );
   gluCylinder( q, 0.5, 0.5,  1.0, 15, 1 );
   glEndList();

   objects_list[1] = glGenLists(1);
   glNewList( objects_list[1], GL_COMPILE );
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green );
   glMaterialfv( GL_FRONT, GL_EMISSION, black );
   gluCylinder( q, 1.5, 0.0,  2.5, 15, 1 );
   glEndList();
}


static void init( void )
{
   make_table();
   make_objects();

   Image = LoadRGBImage( TABLE_TEXTURE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", TABLE_TEXTURE);
      exit(0);
   }

   gluBuild2DMipmaps(GL_TEXTURE_2D, 3, ImgWidth, ImgHeight,
                     ImgFormat, GL_UNSIGNED_BYTE, Image);

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   xrot = 30.0;
   yrot = 50.0;
   spin = 0.0;

   glShadeModel( GL_FLAT );

   glEnable( GL_LIGHT0 );
   glEnable( GL_LIGHTING );

   glClearColor( 0.5, 0.5, 0.9, 0.0 );

   glEnable( GL_NORMALIZE );
}



static void reshape(int w, int h)
{
   GLfloat yAspect = 2.5;
   GLfloat xAspect = yAspect * (float) w / (float) h;
   Width = w;
   Height = h;
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -xAspect, xAspect, -yAspect, yAspect, 10.0, 30.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}



static void draw_objects( GLfloat eyex, GLfloat eyey, GLfloat eyez )
{
   (void) eyex;
   (void) eyey;
   (void) eyez;
#ifndef USE_ZBUFFER
   if (eyex<0.5) {
#endif
       glPushMatrix();
       glTranslatef( 1.0, 1.5, 0.0 );
       glRotatef( spin, 1.0, 0.5, 0.0 );
       glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
       glCallList( objects_list[0] );
       glPopMatrix();
    
       glPushMatrix();
       glTranslatef( -1.0, 0.85+3.0*fabs( cos(0.01*spin) ), 0.0 );
       glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
       glRotatef( spin, 1.0, 0.5, 0.0 );
       glScalef( 0.5, 0.5, 0.5 );
       glCallList( objects_list[1] );
       glPopMatrix();
#ifndef USE_ZBUFFER
   }
   else {   
       glPushMatrix();
       glTranslatef( -1.0, 0.85+3.0*fabs( cos(0.01*spin) ), 0.0 );
       glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
       glRotatef( spin, 1.0, 0.5, 0.0 );
       glScalef( 0.5, 0.5, 0.5 );
       glCallList( objects_list[1] );
       glPopMatrix();

       glPushMatrix();
       glTranslatef( 1.0, 1.5, 0.0 );
       glRotatef( spin, 1.0, 0.5, 0.0 );
       glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
       glCallList( objects_list[0] );
       glPopMatrix();
   }
#endif
}



static void draw_table( void )
{
   glCallList( table_list );
}



static void draw( void )
{
   static GLfloat light_pos[] = { 0.0, 20.0, 0.0, 1.0 };
   GLfloat dist = 20.0;
   GLfloat eyex, eyey, eyez;

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


   eyex = dist * cos(yrot*DEG2RAD) * cos(xrot*DEG2RAD);
   eyez = dist * sin(yrot*DEG2RAD) * cos(xrot*DEG2RAD);
   eyey = dist * sin(xrot*DEG2RAD);

   /* view from top */
   glPushMatrix();
   gluLookAt( eyex, eyey, eyez, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0 );

   glLightfv( GL_LIGHT0, GL_POSITION, light_pos );

   /* draw table into stencil planes */
   glDisable( GL_DEPTH_TEST );
   glEnable( GL_STENCIL_TEST );
   glStencilFunc( GL_ALWAYS, 1, 0xffffffff );
   glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
   glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
   draw_table();
   glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

   glEnable( GL_DEPTH_TEST );

   /* render view from below (reflected viewport) */
   /* only draw where stencil==1 */
   if (eyey>0.0) {
      glPushMatrix();

      glStencilFunc( GL_EQUAL, 1, 0xffffffff );  /* draw if ==1 */
      glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
      glScalef( 1.0, -1.0, 1.0 );

      /* Reposition light in reflected space. */
      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

      draw_objects(eyex, eyey, eyez);
      glPopMatrix();

      /* Restore light's original unreflected position. */
      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
   }

   glDisable( GL_STENCIL_TEST );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

   glEnable( GL_TEXTURE_2D );
   draw_table();
   glDisable( GL_TEXTURE_2D );
   glDisable( GL_BLEND );

   /* view from top */
   glPushMatrix();

   draw_objects(eyex, eyey, eyez);

   glPopMatrix();

   glPopMatrix();

   if (ShowBuffer == GL_DEPTH) {
      ShowDepthBuffer(Width, Height, 1.0, 0.0);
   }
   else if (ShowBuffer == GL_STENCIL) {
      ShowStencilBuffer(Width, Height, 255.0, 0.0);
   }
   else if (ShowBuffer == GL_ALPHA) {
      ShowAlphaBuffer(Width, Height);
   }
}

/*******************************/

int main( int argc, char *argv[] )
{
     int quit = 0;
     DFBResult err;
     DFBSurfaceDescription dsc;

     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the super interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* create an event buffer for all devices with these caps */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_ALL, DFB_FALSE, &events ));

     /* set our cooperative level to DFSCL_FULLSCREEN
        for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* get the primary surface, i.e. the surface of the
        primary layer we have exclusive access to */
     dsc.flags = DSDESC_CAPS;
     dsc.caps  = (DFBSurfaceCapabilities)(DSCAPS_PRIMARY | DSCAPS_DOUBLE);

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));

     /* get the size of the surface and fill it */
     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));
     DFBCHECK(primary->FillRectangle( primary, 0, 0,
                                      screen_width, screen_height ));

     /* create the default font and set it */
     DFBCHECK(dfb->CreateFont( dfb, NULL, NULL, &font ));
     DFBCHECK(primary->SetFont( primary, font ));

     /* get the GL context */
     DFBCHECK(primary->GetGL( primary, &primary_gl ));

     DFBCHECK(primary_gl->Lock( primary_gl ));
     
     init();
     reshape(screen_width, screen_height);

     DFBCHECK(primary_gl->Unlock( primary_gl ));
     
     T0 = get_millis();

     while (!quit) {
          DFBInputEvent evt;
          unsigned long t;

          DFBCHECK(primary_gl->Lock( primary_gl ));
          
          draw();

          DFBCHECK(primary_gl->Unlock( primary_gl ));
          
          if (fps) {
               char buf[64];

               sprintf(buf, "%4.1f FPS\n", fps);
               primary->SetColor( primary, 0xff, 0, 0, 0xff );
               primary->DrawString( primary, buf, -1, screen_width - 5, 5, DSTF_TOPRIGHT );
          }

          primary->Flip( primary, NULL, (DFBSurfaceFlipFlags)0 );
          Frames++;


          t = get_millis();
          if (t - T0 >= 1000) {
               GLfloat seconds = (t - T0) / 1000.0;

               fps = Frames / seconds;

               T0 = t;
               Frames = 0;
          }


          while (events->GetEvent( events, DFB_EVENT(&evt) ) == DFB_OK) {
               switch (evt.type) {
                    case DIET_KEYPRESS:
                         switch (DFB_LOWER_CASE(evt.key_symbol)) {
                              case DIKS_ESCAPE:
                                   quit = 1;
                                   break;
                              case DIKS_CURSOR_UP:
                                   xrot += 3.0;
                                   if ( xrot > 85 )
                                      xrot = 85;
                                   break;
                              case DIKS_CURSOR_DOWN:
                                   xrot -= 3.0;
                                   if ( xrot < 5 )
                                      xrot = 5;
                                   break;
                              case DIKS_CURSOR_LEFT:
                                   yrot += 3.0;
                                   break;
                              case DIKS_CURSOR_RIGHT:
                                   yrot -= 3.0;
                                   break;
                              case DIKS_SMALL_D:
                                   ShowBuffer = GL_DEPTH;
                                   break;
                              case DIKS_SMALL_S:
                                   ShowBuffer = GL_STENCIL;
                                   break;
                              case DIKS_SMALL_A:
                                   ShowBuffer = GL_ALPHA;
                                   break;
                              default:
                                   ShowBuffer = GL_NONE;
                         }
                         break;
                    case DIET_AXISMOTION:
                         if (evt.flags & DIEF_AXISREL) {
                              switch (evt.axis) {
                                   case DIAI_X:
                                        yrot += evt.axisrel / 2.0;
                                        break;
                                   case DIAI_Y:
                                        xrot += evt.axisrel / 2.0;
                                        break;
                                   default:
                                        ;
                              }
                         }
                         break;
                    default:
                         ;
               }
          }

          spin += 2.0;
          yrot += 3.0;
     }

     /* release our interfaces to shutdown DirectFB */
     primary_gl->Release( primary_gl );
     primary->Release( primary );
     font->Release( font );
     events->Release( events );
     dfb->Release( dfb );

     return 0;
}

