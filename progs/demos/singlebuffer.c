/*
 * Demo of (nearly) flicker-free drawing with a single color buffer.
 *
 * Basically, draw the scene into the Z buffer first, then draw the
 * scene into the color buffer.  Finally, "clear" the background by
 * setting the fragments we didn't hit earlier.
 *
 * This won't work if you need blending.  The technique works best
 * when the scene is relatively simple and can be rendered quickly
 * (i.e. with hardware), and when the objects don't move too much from
 * one frame to the next.
 *
 * Brian Paul
 * 25 August 2005
 *
 * See Mesa license for terms.
 */


#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>


#define FLICKER 0
#define NO_FLICKER 1

static GLint Mode = NO_FLICKER;
static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLboolean Anim = GL_TRUE;
static GLfloat ClearColor[4] = {0.2, 0.2, 0.9, 0.0};
static GLfloat NearClip = 5.0, FarClip = 25.0, ViewDist = 7.0;
static double PrevTime = -1;

struct box {
   float tx, ty, tz;
   float rx, ry, rz, ra;
   float sx, sy, sz;
   float color[4];
};

#define NUM_BOXES 25

struct box Boxes[NUM_BOXES];


/* Return random float in [0,1] */
static float
Random(void)
{
   int i = rand();
   return (float) (i % 1000) / 1000.0;
}


static void
MakeBoxes(void)
{
   int i;
   for (i = 0; i < NUM_BOXES; i++) {
      Boxes[i].tx = -1.0 + 2.0 * Random();
      Boxes[i].ty = -1.0 + 2.0 * Random();
      Boxes[i].tz = -1.0 + 2.0 * Random();
      Boxes[i].sx = 0.1 + Random() * 0.4;
      Boxes[i].sy = 0.1 + Random() * 0.4;
      Boxes[i].sz = 0.1 + Random() * 0.4;
      Boxes[i].rx = Random();
      Boxes[i].ry = Random();
      Boxes[i].rz = Random();
      Boxes[i].ra = Random() * 360.0;
      Boxes[i].color[0] = Random();
      Boxes[i].color[1] = Random();
      Boxes[i].color[2] = Random();
      Boxes[i].color[3] = 1.0;
   }
}


static void
DrawBoxes(void)
{
   int i;
   for (i = 0; i < NUM_BOXES; i++) {
      glPushMatrix();
      glTranslatef(Boxes[i].tx, Boxes[i].ty, Boxes[i].tz);
      glRotatef(Boxes[i].ra, Boxes[i].rx, Boxes[i].ry, Boxes[i].rz);
      glScalef(Boxes[i].sx, Boxes[i].sy, Boxes[i].sz);
      glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Boxes[i].color);
      glutSolidCube(1.0);
      glPopMatrix();
   }
}


static void
Idle(void)
{
   double dt, t = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   if (PrevTime < 0.0)
      PrevTime = t;
   dt = t - PrevTime;
   PrevTime = t;
   Xrot += 16.0 * dt;
   Yrot += 12.0 * dt;
   Zrot += 8.0 * dt;
   glutPostRedisplay();
}


static void
Draw(void)
{
   if (Mode == FLICKER) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }
   else {
      /* don't clear color buffer */
      glClear(GL_DEPTH_BUFFER_BIT);
      /* update Z buffer only */
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   }

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   DrawBoxes();

   if (Mode == NO_FLICKER) {
      /* update color buffer now */
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDepthFunc(GL_EQUAL);
      DrawBoxes();
      glDepthFunc(GL_LESS);
   }

   glPopMatrix();

   if (Mode == NO_FLICKER) {
      /* "clear" the untouched pixels now.
       * Note: if you comment-out this code you'll see something interesting.
       */
      GLfloat x = FarClip / NearClip;
      GLfloat z = -(FarClip - ViewDist - 1.0);
      glDisable(GL_LIGHTING);
      glColor4fv(ClearColor);
      glBegin(GL_POLYGON);
      glVertex3f(-x, -x, z);
      glVertex3f( x, -x, z);
      glVertex3f( x,  x, z);
      glVertex3f(-x,  x, z);
      glEnd();
      glEnable(GL_LIGHTING);
   }

   /* This is where you'd normally do SwapBuffers */
   glFinish();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, NearClip, FarClip);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -ViewDist);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         PrevTime = -1;
         break;
      case 'm':
         Mode = !Mode;
         break;
      case 'b':
         MakeBoxes();
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Xrot -= step;
         break;
      case GLUT_KEY_DOWN:
         Xrot += step;
         break;
      case GLUT_KEY_LEFT:
         Yrot -= step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot += step;
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_CULL_FACE);
   glEnable(GL_NORMALIZE);
   MakeBoxes();
}


static void
Usage(void)
{
   printf("Keys:\n");
   printf("  m       - toggle drawing mode (flicker vs. no flicker)\n");
   printf("  a       - toggle animation\n");
   printf("  b       - generate new boxes\n");
   printf("  ARROWS  - rotate scene\n");
   printf("  ESC     - exit\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(800, 800);
   glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH);
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   Usage();
   glutMainLoop();
   return 0;
}
