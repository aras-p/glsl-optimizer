/* gears.c */

/*
 * 3-D gear wheels.  This program is in the public domain.
 *
 * Brian Paul
 * modified by Uwe Maurer (uwe_maurer@t-online.de)
 */

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ggi/ggi.h>
#include <GL/ggimesa.h>
#ifndef M_PI
#  define M_PI 3.14159265
#endif


ggi_visual_t vis;
char text[100];
int db_flag,vis_x, vis_y, vir_x, vir_y, gt;

/*
 * Draw a gear wheel.  You'll probably want to call this function when
 * building a display list since we do a lot of trig here.
 *
 * Input:  inner_radius - radius of hole at center
 *         outer_radius - radius at center of teeth
 *         width - width of gear
 *         teeth - number of teeth
 *         tooth_depth - depth of tooth
 */
static void gear( GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
		  GLint teeth, GLfloat tooth_depth )
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat angle, da;
   GLfloat u, v, len;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth/2.0;
   r2 = outer_radius + tooth_depth/2.0;

   da = 2.0*M_PI / teeth / 4.0;

   glShadeModel( GL_FLAT );

   glNormal3f( 0.0, 0.0, 1.0 );

   /* draw front face */
   glBegin( GL_QUAD_STRIP );
   for (i=0;i<=teeth;i++) {
      angle = i * 2.0*M_PI / teeth;
      glVertex3f( r0*cos(angle), r0*sin(angle), width*0.5 );
      glVertex3f( r1*cos(angle), r1*sin(angle), width*0.5 );
      glVertex3f( r0*cos(angle), r0*sin(angle), width*0.5 );
      glVertex3f( r1*cos(angle+3*da), r1*sin(angle+3*da), width*0.5 );
   }
   glEnd();

   /* draw front sides of teeth */
   glBegin( GL_QUADS );
   da = 2.0*M_PI / teeth / 4.0;
   for (i=0;i<teeth;i++) {
      angle = i * 2.0*M_PI / teeth;

      glVertex3f( r1*cos(angle),      r1*sin(angle),      width*0.5 );
      glVertex3f( r2*cos(angle+da),   r2*sin(angle+da),   width*0.5 );
      glVertex3f( r2*cos(angle+2*da), r2*sin(angle+2*da), width*0.5 );
      glVertex3f( r1*cos(angle+3*da), r1*sin(angle+3*da), width*0.5 );
   }
   glEnd();


   glNormal3f( 0.0, 0.0, -1.0 );

   /* draw back face */
   glBegin( GL_QUAD_STRIP );
   for (i=0;i<=teeth;i++) {
      angle = i * 2.0*M_PI / teeth;
      glVertex3f( r1*cos(angle), r1*sin(angle), -width*0.5 );
      glVertex3f( r0*cos(angle), r0*sin(angle), -width*0.5 );
      glVertex3f( r1*cos(angle+3*da), r1*sin(angle+3*da), -width*0.5 );
      glVertex3f( r0*cos(angle), r0*sin(angle), -width*0.5 );
   }
   glEnd();

   /* draw back sides of teeth */
   glBegin( GL_QUADS );
   da = 2.0*M_PI / teeth / 4.0;
   for (i=0;i<teeth;i++) {
      angle = i * 2.0*M_PI / teeth;

      glVertex3f( r1*cos(angle+3*da), r1*sin(angle+3*da), -width*0.5 );
      glVertex3f( r2*cos(angle+2*da), r2*sin(angle+2*da), -width*0.5 );
      glVertex3f( r2*cos(angle+da),   r2*sin(angle+da),   -width*0.5 );
      glVertex3f( r1*cos(angle),      r1*sin(angle),      -width*0.5 );
   }
   glEnd();


   /* draw outward faces of teeth */
   glBegin( GL_QUAD_STRIP );
   for (i=0;i<teeth;i++) {
      angle = i * 2.0*M_PI / teeth;

      glVertex3f( r1*cos(angle),      r1*sin(angle),       width*0.5 );
      glVertex3f( r1*cos(angle),      r1*sin(angle),      -width*0.5 );
      u = r2*cos(angle+da) - r1*cos(angle);
      v = r2*sin(angle+da) - r1*sin(angle);
      len = sqrt( u*u + v*v );
      u /= len;
      v /= len;
      glNormal3f( v, -u, 0.0 );
      glVertex3f( r2*cos(angle+da),   r2*sin(angle+da),    width*0.5 );
      glVertex3f( r2*cos(angle+da),   r2*sin(angle+da),   -width*0.5 );
      glNormal3f( cos(angle), sin(angle), 0.0 );
      glVertex3f( r2*cos(angle+2*da), r2*sin(angle+2*da),  width*0.5 );
      glVertex3f( r2*cos(angle+2*da), r2*sin(angle+2*da), -width*0.5 );
      u = r1*cos(angle+3*da) - r2*cos(angle+2*da);
      v = r1*sin(angle+3*da) - r2*sin(angle+2*da);
      glNormal3f( v, -u, 0.0 );
      glVertex3f( r1*cos(angle+3*da), r1*sin(angle+3*da),  width*0.5 );
      glVertex3f( r1*cos(angle+3*da), r1*sin(angle+3*da), -width*0.5 );
      glNormal3f( cos(angle), sin(angle), 0.0 );
   }

   glVertex3f( r1*cos(0), r1*sin(0), width*0.5 );
   glVertex3f( r1*cos(0), r1*sin(0), -width*0.5 );

   glEnd();


   glShadeModel( GL_SMOOTH );

   /* draw inside radius cylinder */
   glBegin( GL_QUAD_STRIP );
   for (i=0;i<=teeth;i++) {
      angle = i * 2.0*M_PI / teeth;
      glNormal3f( -cos(angle), -sin(angle), 0.0 );
      glVertex3f( r0*cos(angle), r0*sin(angle), -width*0.5 );
      glVertex3f( r0*cos(angle), r0*sin(angle), width*0.5 );
   }
   glEnd();
      
}


static GLfloat view_rotx=20.0, view_roty=30.0, view_rotz=0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;

static GLuint limit;
static GLuint count = 1;


static void draw( void )
{
   static int n = 0;
   glClearColor(0,0,0,0);
   glClearIndex(0);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef( view_rotx, 1.0, 0.0, 0.0 );
   glRotatef( view_roty, 0.0, 1.0, 0.0 );
   glRotatef( view_rotz, 0.0, 0.0, 1.0 );

   glPushMatrix();
   glTranslatef( -3.0, -2.0, 0.0 );
   glRotatef( angle, 0.0, 0.0, 1.0 );
   glCallList(gear1);
   glPopMatrix();

   glPushMatrix();
   glTranslatef( 3.1, -2.0, 0.0 );
   glRotatef( -2.0*angle-9.0, 0.0, 0.0, 1.0 );
   glCallList(gear2);
   glPopMatrix();

   glPushMatrix();
   glTranslatef( -3.1, 4.2, 0.0 );
   glRotatef( -2.0*angle-25.0, 0.0, 0.0, 1.0 );
   glCallList(gear3);
   glPopMatrix();

   glPopMatrix();
   glFlush();
   glFinish();

#if 0
   ggiSetGCForeground(vis,255);
   ggiPuts(vis,0,0,"Mesa  ->  GGI");
   ggiPuts(vis,0,ggiGetInfo(vis)->mode->visible.y," Mesa -> GGI");

   ggiPuts(vis,0,16,text);
   ggiPuts(vis,0,ggiGetInfo(vis)->mode->visible.y+16,text);
#endif

    if(db_flag)
   ggiMesaSwapBuffers();
 			
   count++;
   if (count==limit) {
      exit(1);
   }
    ++n;
    /*
   if (!(n%10)){
	ggi_color rgb = { 10000, 10000, 10000 };
	ggiSetSimpleMode(vis,vis_x+(n/10),vis_y+(n/10),db_flag?2:1, gt); 
	glViewport(0, 0,vis_x+(n/10),vis_y+(n/10));
	ggiSetGCForeground(vis, ggiMapColor(vis, &rgb));
	ggiDrawBox(vis, 20, 20, 100, 100);
	if(db_flag)
	  ggiSetWriteFrame(vis, 1);
    }
    */
}

static void idle( void )
{
   angle += 2.0;
   draw();
}

/* new window size or exposure */
static void reshape( int width, int height )
{
   GLfloat  h = (GLfloat) height / (GLfloat) width;

    if(db_flag)
	glDrawBuffer(GL_BACK);
    else
	glDrawBuffer(GL_FRONT);
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -h, h, 5.0, 60.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -40.0 );
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

}


static void init( void )
{
   static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0 };
   static GLfloat red[4] = {0.9, 0.9, 0.9, 1.0 };
   static GLfloat green[4] = {0.0, 0.8, 0.9, 1.0 };
   static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0 };

   glLightfv( GL_LIGHT0, GL_POSITION, pos );
   glEnable( GL_CULL_FACE );
   glEnable( GL_LIGHTING );
   glEnable( GL_LIGHT0 );
   glEnable( GL_DEPTH_TEST );

   /* make the gears */
   gear1 = glGenLists(1);
   glNewList(gear1, GL_COMPILE);
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red );
   glIndexi(1);
   gear( 1.0, 4.0, 1.0, 20, 0.7 );
   glEndList();

   gear2 = glGenLists(1);
   glNewList(gear2, GL_COMPILE);
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green );
  glIndexi(2);
   gear( 0.5, 2.0, 2.0, 10, 0.7 );
   glEndList();

   gear3 = glGenLists(1);
   glNewList(gear3, GL_COMPILE);
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue );
  glIndexi(3);
   gear( 1.3, 2.0, 0.5, 10, 0.7 );
   glEndList();

   glEnable( GL_NORMALIZE );
}

static void usage(char *s)
{
 printf("%s visible_x visible_y virtual_x virtual_y bpp db_flag\n",s);
 printf("example:\n");
 printf("%s 320 200 320 400 8 1\n",s);
 exit(1);
}

int main( int argc, char *argv[] )
{
	ggi_mesa_context_t ctx;
	ggi_mode mode;
	int bpp;

	limit=0;

	if (argc<7) usage(argv[0]);

	vis_x=atoi(argv[1]);
	vis_y=atoi(argv[2]);
	vir_x=atoi(argv[3]);
	vir_y=atoi(argv[4]);
	bpp=atoi(argv[5]);
	db_flag=atoi(argv[6]);

	switch(bpp)
	{
		case 4: gt=GT_4BIT;break;
		case 8: gt=GT_8BIT;break;
		case 15:gt=GT_15BIT;break;
		case 16:gt=GT_16BIT;break;
		case 24:gt=GT_24BIT;break;
		case 32:gt=GT_32BIT;break;
		default:
			printf("%i Bits per Pixel ???\n",bpp);
			exit(1);
	} 
	sprintf(text,"%sx%s %i colors, RGB mode, %s",
		argv[1],argv[2],1<<bpp,
		(db_flag) ? "doublebuffer" : "no doublebuffer");
 
	if (ggiInit()<0) 
	{
		printf("ggiInit() failed\n");
		exit(1);
	}
 
	if (ggiMesaInit() < 0)
	{
		printf("ggiMesaInit failed\n");
		exit(1);
	}
	
	vis=ggiOpen(NULL);
	if (vis==NULL)
	{
		printf("ggiOpen() failed\n");
		exit(1);
	}

	if (ggiSetSimpleMode(vis,vis_x,vis_y,db_flag ? 2 : 1,gt)<0) 
	{
		printf("%s: can't set graphmode (%i %i %i %i)  %i BPP\n",
			argv[0],vis_x,vis_y,vir_x,vir_y,bpp);
		exit(1);
	}

	if (ggiMesaAttach(vis) < 0)
	{
		printf("ggiMesaAttach failed\n");
		exit(1);
	}
	if (ggiMesaExtendVisual(vis, GL_FALSE, GL_FALSE, 16,
	                        0, 0, 0, 0, 0, 1)  < 0)
	{
		printf ("GGIMesaSetVisual() failed\n");
		exit(1);
	}

	ctx = ggiMesaCreateContext(vis); 
	if (ctx==NULL)
	{
		printf("GGIMesaCreateContext() failed\n");
		exit(1);
	}

	ggiMesaMakeCurrent(ctx, vis);
	ggiGetMode(vis,&mode);

	reshape(mode.visible.x,mode.visible.y);

	init();
 
	while (!ggiKbhit(vis)) { /*sleep(1);*/ idle(); }

	ggiMesaDestroyContext(ctx);
	ggiClose(vis);

	printf("%s\n",text);

	ggiExit(); 
	return 0;
}
