/*
 * Demo of a reflective, texture-mapped surface with OpenGL.
 * Brian Paul   August 14, 1995   This file is in the public domain.
 *
 * Hardware texture mapping is highly recommended!
 *
 * The basic steps are:
 *    1. Render the reflective object (a polygon) from the normal viewpoint,
 *       setting the stencil planes = 1.
 *    2. Render the scene from a special viewpoint:  the viewpoint which
 *       is on the opposite side of the reflective plane.  Only draw where
 *       stencil = 1.  This draws the objects in the reflective surface.
 *    3. Render the scene from the original viewpoint.  This draws the
 *       objects in the normal fashion.  Use blending when drawing
 *       the reflective, textured surface.
 *
 * This is a very crude demo.  It could be much better.
 */
 
/*
 * Authors:
 *   Brian Paul
 *   Dirk Reiners (reiners@igd.fhg.de) made some modifications to this code.
 *   Mark Kilgard (April 1997)
 *   Brian Paul (April 2000 - added keyboard d/s options)
 *   Brian Paul (August 2005 - added multi window feature)
 */


#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "GL/glut.h"
#include "showbuffer.h"
#include "readtex.h"


#define DEG2RAD (3.14159/180.0)
#define TABLE_TEXTURE "../images/tile.rgb"
#define MAX_OBJECTS 2
#define INIT_WIDTH 400
#define INIT_HEIGHT 300

#ifdef _WIN32
#undef CreateWindowA
#endif

struct window {
   int id;               /* returned by glutCreateWindow() */
   int width, height;
   GLboolean anim;
   GLfloat xrot, yrot;
   GLfloat spin;
   GLenum showBuffer;
   GLenum drawBuffer;
   GLuint table_list;
   GLuint objects_list[MAX_OBJECTS];
   double t0;
   struct window *next;
};


static struct window *FirstWindow = NULL;


static void
CreateWindow(void);


static struct window *
CurrentWindow(void)
{
   int id = glutGetWindow();
   struct window *w;
   for (w = FirstWindow; w; w = w->next) {
      if (w->id == id)
         return w;
   }
   return NULL;
}


static GLboolean
AnyAnimating(void)
{
   struct window *w;
   for (w = FirstWindow; w; w = w->next) {
      if (w->anim)
         return 1;
   }
   return 0;
}


static void
KillWindow(struct window *w)
{
   struct window *win, *prev = NULL;
   for (win = FirstWindow; win; win = win->next) {
      if (win == w) {
         if (prev) {
            prev->next = win->next;
         }
         else {
            FirstWindow = win->next;
         }
         glutDestroyWindow(win->id);
         win->next = NULL;
         free(win);
         return;
      }
      prev = win;
   }
}


static void
KillAllWindows(void)
{
   while (FirstWindow)
      KillWindow(FirstWindow);
}


static GLuint
MakeTable(void)
{
   static GLfloat table_mat[] = { 1.0, 1.0, 1.0, 0.6 };
   static GLfloat gray[] = { 0.4, 0.4, 0.4, 1.0 };
   GLuint table_list;

   table_list = glGenLists(1);
   glNewList( table_list, GL_COMPILE );

   /* load table's texture */
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, table_mat );
   /*glMaterialfv( GL_FRONT, GL_EMISSION, gray );*/
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
   return table_list;
}


static void
MakeObjects(GLuint *objects_list)
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

   gluDeleteQuadric(q);
}


static void
InitWindow(struct window *w)
{
   GLint imgWidth, imgHeight;
   GLenum imgFormat;
   GLubyte *image = NULL;

   w->table_list = MakeTable();
   MakeObjects(w->objects_list);

   image = LoadRGBImage( TABLE_TEXTURE, &imgWidth, &imgHeight, &imgFormat );
   if (!image) {
      printf("Couldn't read %s\n", TABLE_TEXTURE);
      exit(0);
   }

   gluBuild2DMipmaps(GL_TEXTURE_2D, 3, imgWidth, imgHeight,
                     imgFormat, GL_UNSIGNED_BYTE, image);
   free(image);

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   glShadeModel( GL_FLAT );
   
   glEnable( GL_LIGHT0 );
   glEnable( GL_LIGHTING );

   glClearColor( 0.5, 0.5, 0.9, 0.0 );

   glEnable( GL_NORMALIZE );
}


static void
Reshape(int width, int height)
{
   struct window *w = CurrentWindow();
   GLfloat yAspect = 2.5;
   GLfloat xAspect = yAspect * (float) width / (float) height;
   w->width = width;
   w->height = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -xAspect, xAspect, -yAspect, yAspect, 10.0, 30.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
DrawObjects(struct window *w, GLfloat eyex, GLfloat eyey, GLfloat eyez)
{
   (void) eyex;
   (void) eyey;
   (void) eyez;
#ifndef USE_ZBUFFER
   if (eyex<0.5) {
#endif
      glPushMatrix();
      glTranslatef( 1.0, 1.5, 0.0 );
      glRotatef( w->spin, 1.0, 0.5, 0.0 );
      glRotatef( 0.5*w->spin, 0.0, 0.5, 1.0 );
      glCallList( w->objects_list[0] );
      glPopMatrix();
      
      glPushMatrix();
      glTranslatef( -1.0, 0.85+3.0*fabs( cos(0.01*w->spin) ), 0.0 );
      glRotatef( 0.5*w->spin, 0.0, 0.5, 1.0 );
      glRotatef( w->spin, 1.0, 0.5, 0.0 );
      glScalef( 0.5, 0.5, 0.5 );
      glCallList( w->objects_list[1] );
      glPopMatrix();
#ifndef USE_ZBUFFER
   }
   else {	
      glPushMatrix();
      glTranslatef( -1.0, 0.85+3.0*fabs( cos(0.01*w->spin) ), 0.0 );
      glRotatef( 0.5*w->spin, 0.0, 0.5, 1.0 );
      glRotatef( w->spin, 1.0, 0.5, 0.0 );
      glScalef( 0.5, 0.5, 0.5 );
      glCallList( w->objects_list[1] );
      glPopMatrix();

      glPushMatrix();
      glTranslatef( 1.0, 1.5, 0.0 );
      glRotatef( w->spin, 1.0, 0.5, 0.0 );
      glRotatef( 0.5*w->spin, 0.0, 0.5, 1.0 );
      glCallList( w->objects_list[0] );
      glPopMatrix();
   }
#endif
}


static void
DrawTable(struct window *w)
{
   glCallList(w->table_list);
}


static void
DrawWindow(void)
{
   struct window *w = CurrentWindow();
   static GLfloat light_pos[] = { 0.0, 20.0, 0.0, 1.0 };
   GLfloat dist = 20.0;
   GLfloat eyex, eyey, eyez;

   if (w->drawBuffer == GL_NONE) {
      glDrawBuffer(GL_BACK);
      glReadBuffer(GL_BACK);
   }
   else {
      glDrawBuffer(w->drawBuffer);
      glReadBuffer(w->drawBuffer);
   }

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   if (w->drawBuffer == GL_NONE) {
      glDrawBuffer(GL_NONE);
   }

   eyex = dist  *  cos(w->yrot * DEG2RAD)  *  cos(w->xrot * DEG2RAD);
   eyez = dist  *  sin(w->yrot * DEG2RAD)  *  cos(w->xrot * DEG2RAD);
   eyey = dist  *  sin(w->xrot * DEG2RAD);

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
   DrawTable(w);
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

      DrawObjects(w, eyex, eyey, eyez);
      glPopMatrix();

      /* Restore light's original unreflected position. */
      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
   }

   glDisable( GL_STENCIL_TEST );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

   glEnable( GL_TEXTURE_2D );
   DrawTable(w);
   glDisable( GL_TEXTURE_2D );
   glDisable( GL_BLEND );

   /* view from top */
   glPushMatrix();

   DrawObjects(w, eyex, eyey, eyez);

   glPopMatrix();

   glPopMatrix();

   if (w->showBuffer == GL_DEPTH) {
      ShowDepthBuffer(w->width, w->height, 1.0, 0.0);
   }
   else if (w->showBuffer == GL_STENCIL) {
      ShowStencilBuffer(w->width, w->height, 255.0, 0.0);
   }
   else if (w->showBuffer == GL_ALPHA) {
      ShowAlphaBuffer(w->width, w->height);
   }

   if (w->drawBuffer == GL_FRONT)
      glFinish();
   else
      glutSwapBuffers();

   /* calc/show frame rate */
   {
      static GLint t0 = 0;
      static GLint frames = 0;
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      frames++;
      if (t - t0 >= 5000) {
         GLfloat seconds = (t - t0) / 1000.0;
         GLfloat fps = frames / seconds;
         printf("%d frames in %g seconds = %g FPS\n", frames, seconds, fps);
         t0 = t;
         frames = 0;
      }
   }
}


static void
Idle(void)
{
   double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   struct window *w;
   for (w = FirstWindow; w; w = w->next) {
      if (w->anim) {
         double dt;
         if (w->t0 < 0.0)
            w->t0 = t;
         dt = t - w->t0;
         w->t0 = t;
         w->spin += 60.0 * dt;
         w->yrot += 90.0 * dt;
         assert(w->id);
         glutSetWindow(w->id);
         glutPostRedisplay();
      }
   }
}


static void
UpdateIdleFunc(void)
{
   if (AnyAnimating())
      glutIdleFunc(Idle);
   else
      glutIdleFunc(NULL);
}

static void
Key(unsigned char key, int x, int y)
{
   struct window *w = CurrentWindow();
   (void) x;
   (void) y;

   switch (key) {
   case 'd':
      w->showBuffer = GL_DEPTH;
      glutPostRedisplay();
      break;
   case 's':
      w->showBuffer = GL_STENCIL;
      glutPostRedisplay();
      break;
   case 'a':
      w->showBuffer = GL_ALPHA;
      glutPostRedisplay();
      break;
   case 'c':
      w->showBuffer = GL_NONE;
      glutPostRedisplay();
      break;
   case 'f':
      if (w->drawBuffer == GL_FRONT)
         w->drawBuffer = GL_BACK;
      else
         w->drawBuffer = GL_FRONT;
      glutPostRedisplay();
      break;
   case '0':
      w->drawBuffer = GL_NONE;
      glutPostRedisplay();
      break;
   case ' ':
      w->anim = !w->anim;
      w->t0 = -1;
      UpdateIdleFunc();
      glutPostRedisplay();
      break;
   case 'n':
      CreateWindow();
      UpdateIdleFunc();
      break;
   case 'k':
      KillWindow(w);
      if (FirstWindow == NULL)
         exit(0);
      break;
   case 27:
      KillAllWindows();
      exit(0);
      break;
   default:
      ;
   }
}


static void
SpecialKey(int key, int x, int y)
{
   struct window *w = CurrentWindow();
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         w->xrot += 3.0;
         if (w->xrot > 85)
            w->xrot = 85;
         break;
      case GLUT_KEY_DOWN:
         w->xrot -= 3.0;
         if (w->xrot < 5)
            w->xrot = 5;
         break;
      case GLUT_KEY_LEFT:
         w->yrot += 3.0;
         break;
      case GLUT_KEY_RIGHT:
         w->yrot -= 3.0;
         break;
   }
   glutPostRedisplay();
}


static void
CreateWindow(void)
{
   char title[1000];
   struct window *w = (struct window *) calloc(1, sizeof(struct window));
   
   glutInitWindowSize(INIT_WIDTH, INIT_HEIGHT);
   w->id = glutCreateWindow("foo");
   sprintf(title, "reflect window %d", w->id);
   glutSetWindowTitle(title);
   assert(w->id);
   w->width = INIT_WIDTH;
   w->height = INIT_HEIGHT;
   w->anim = GL_TRUE;
   w->xrot = 30.0;
   w->yrot = 50.0;
   w->spin = 0.0;
   w->showBuffer = GL_NONE;
   w->drawBuffer = GL_BACK;

   InitWindow(w);

   glutReshapeFunc(Reshape);
   glutDisplayFunc(DrawWindow);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);

   /* insert at head of list */
   w->next = FirstWindow;
   FirstWindow = w;
}


static void
Usage(void)
{
   printf("Keys:\n");
   printf("  a      - show alpha buffer\n");
   printf("  d      - show depth buffer\n");
   printf("  s      - show stencil buffer\n");
   printf("  c      - show color buffer\n");
   printf("  f      - toggle rendering to front/back color buffer\n");
   printf("  n      - create new window\n");
   printf("  k      - kill window\n");
   printf("  SPACE  - toggle animation\n");
   printf("  ARROWS - rotate scene\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH |
                       GLUT_STENCIL | GLUT_ALPHA);
   CreateWindow();
   glutIdleFunc(Idle);
   Usage();
   glutMainLoop();
   return 0;
}
