/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (tech.hmw@plus.it)
 *            Humanware s.r.l.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#if defined (WIN32)|| defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

#include <GL/glut.h>

#include "readtex.h"

#ifdef XMESA
#include "GL/xmesa.h"
static int fullscreen = 1;
#endif

static int WIDTH = 640;
static int HEIGHT = 480;

static GLint T0;
static GLint Frames;

#define MAX_LOD 9

#define TEX_SKY_WIDTH 256
#define TEX_SKY_HEIGHT TEX_SKY_WIDTH

#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define FROM_NONE   0
#define FROM_DOWN   1
#define FROM_UP     2
#define FROM_LEFT   3
#define FROM_RIGHT  4
#define FROM_FRONT  5
#define FROM_BACK   6

static int win = 0;

static float obs[3] = { 3.8, 0.0, 0.0 };
static float dir[3];
static float v = 0.0;
static float alpha = -90.0;
static float beta = 90.0;

static int fog = 1;
static int bfcull = 1;
static int usetex = 1;
static int help = 1;
static int poutline = 0;
static int normext = 1;
static int joyavailable = 0;
static int joyactive = 0;
static int LODbias = 3;
static int maxdepth = MAX_LOD;

static unsigned int totpoly = 0;

static GLuint t1id, t2id;
static GLuint skydlist, LODdlist[MAX_LOD], LODnumpoly[MAX_LOD];

static void
initlight(void)
{
   GLfloat lspec[4] = { 1.0, 1.0, 1.0, 1.0 };
   static GLfloat lightpos[4] = { 30, 15.0, 30.0, 1.0 };

   glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
   glLightfv(GL_LIGHT0, GL_SPECULAR, lspec);

   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, lspec);
}

static void
initdlists(void)
{
   static int slicetable[MAX_LOD][2] = {
      {21, 10},
      {18, 9},
      {15, 8},
      {12, 7},
      {9, 6},
      {7, 5},
      {5, 4},
      {4, 3},
      {3, 2}
   };
   GLUquadricObj *obj;
   int i, xslices, yslices;

   obj = gluNewQuadric();

   skydlist = glGenLists(1);
   glNewList(skydlist, GL_COMPILE);
   glBindTexture(GL_TEXTURE_2D, t2id);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glColor3f(1.0f, 1.0f, 1.0f);

   gluQuadricDrawStyle(obj, GLU_FILL);
   gluQuadricNormals(obj, GLU_NONE);
   gluQuadricTexture(obj, GL_TRUE);
   gluQuadricOrientation(obj, GLU_INSIDE);
   gluSphere(obj, 40.0f, 18, 9);

   glEndList();

   for (i = 0; i < MAX_LOD; i++) {
      LODdlist[i] = glGenLists(1);
      glNewList(LODdlist[i], GL_COMPILE);

      gluQuadricDrawStyle(obj, GLU_FILL);
      gluQuadricNormals(obj, GLU_SMOOTH);
      gluQuadricTexture(obj, GL_TRUE);
      gluQuadricOrientation(obj, GLU_OUTSIDE);
      xslices = slicetable[i][0];
      yslices = slicetable[i][1];
      gluSphere(obj, 1.0f, xslices, yslices);
      LODnumpoly[i] = xslices * (yslices - 2) + 2 * (xslices - 1);

      glEndList();
   }
}

static void
inittextures(void)
{
   GLubyte tsky[TEX_SKY_HEIGHT][TEX_SKY_WIDTH][3];
   GLuint x, y;
   GLfloat fact;
   GLenum gluerr;

   /* Brick */

   glGenTextures(1, &t1id);
   glBindTexture(GL_TEXTURE_2D, t1id);

   if (!LoadRGBMipmaps("../images/bw.rgb", 3)) {
      fprintf(stderr, "Error reading a texture.\n");
      exit(-1);
   }

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   /* Sky */

   glGenTextures(1, &t2id);
   glBindTexture(GL_TEXTURE_2D, t2id);

   for (y = 0; y < TEX_SKY_HEIGHT; y++)
      for (x = 0; x < TEX_SKY_WIDTH; x++)
	 if (y < TEX_SKY_HEIGHT / 2) {
	    fact = y / (GLfloat) (TEX_SKY_HEIGHT / 2);
	    tsky[y][x][0] =
	       (GLubyte) (255.0f * (0.1f * fact + 0.3f * (1.0f - fact)));
	    tsky[y][x][1] =
	       (GLubyte) (255.0f * (0.2f * fact + 1.0f * (1.0f - fact)));
	    tsky[y][x][2] = 255;
	 }
	 else {
	    tsky[y][x][0] = tsky[TEX_SKY_HEIGHT - y - 1][x][0];
	    tsky[y][x][1] = tsky[TEX_SKY_HEIGHT - y - 1][x][1];
	    tsky[y][x][2] = 255;
	 }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   if (
       (gluerr =
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TEX_SKY_WIDTH, TEX_SKY_HEIGHT,
			  GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *) (tsky)))) {
      fprintf(stderr, "GLULib%s\n", (char *) gluErrorString(gluerr));
      exit(-1);
   }

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static void
calcposobs(void)
{
   dir[0] = sin(alpha * M_PI / 180.0);
   dir[1] = cos(alpha * M_PI / 180.0) * sin(beta * M_PI / 180.0);
   dir[2] = cos(beta * M_PI / 180.0);

   if (dir[0] < 1.0e-5 && dir[0] > -1.0e-5)
      dir[0] = 0;
   if (dir[1] < 1.0e-5 && dir[1] > -1.0e-5)
      dir[1] = 0;
   if (dir[2] < 1.0e-5 && dir[2] > -1.0e-5)
      dir[2] = 0;

   obs[0] += v * dir[0];
   obs[1] += v * dir[1];
   obs[2] += v * dir[2];
}

static void
special(int k, int x, int y)
{
   switch (k) {
   case GLUT_KEY_LEFT:
      alpha -= 2.0;
      break;
   case GLUT_KEY_RIGHT:
      alpha += 2.0;
      break;
   case GLUT_KEY_DOWN:
      beta -= 2.0;
      break;
   case GLUT_KEY_UP:
      beta += 2.0;
      break;
   }
}

static void
key(unsigned char k, int x, int y)
{
   switch (k) {
   case 27:
      exit(0);
      break;

   case 'a':
      v += 0.01;
      break;
   case 'z':
      v -= 0.01;
      break;

#ifdef XMESA
   case ' ':
      fullscreen = (!fullscreen);
      XMesaSetFXmode(fullscreen ? XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW);
      break;
#endif

   case '+':
      LODbias--;
      break;
   case '-':
      LODbias++;
      break;
   case 'j':
      joyactive = (!joyactive);
      break;
   case 'h':
      help = (!help);
      break;
   case 'f':
      fog = (!fog);
      break;
   case 't':
      usetex = (!usetex);
      break;
   case 'n':
      normext = (!normext);
      break;
   case 'b':
      if (bfcull) {
	 glDisable(GL_CULL_FACE);
	 bfcull = 0;
      }
      else {
	 glEnable(GL_CULL_FACE);
	 bfcull = 1;
      }
      break;
   case 'p':
      if (poutline) {
	 glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	 poutline = 0;
	 usetex = 1;
      }
      else {
	 glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	 poutline = 1;
	 usetex = 0;
      }
      break;
   }
}

static void
reshape(int w, int h)
{
   WIDTH = w;
   HEIGHT = h;
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(90.0, w / (float) h, 0.8, 100.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glViewport(0, 0, w, h);
}

static void
printstring(void *font, char *string)
{
   int len, i;

   len = (int) strlen(string);
   for (i = 0; i < len; i++)
      glutBitmapCharacter(font, string[i]);
}

static void
printhelp(void)
{
   glEnable(GL_BLEND);
   glColor4f(0.5, 0.5, 0.5, 0.5);
   glRecti(40, 40, 600, 440);
   glDisable(GL_BLEND);

   glColor3f(1.0, 0.0, 0.0);
   glRasterPos2i(300, 420);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "Help");

   glRasterPos2i(60, 390);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "h - Toggle Help");
   glRasterPos2i(60, 360);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "t - Toggle Textures");
   glRasterPos2i(60, 330);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "f - Toggle Fog");
   glRasterPos2i(60, 300);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "b - Toggle Back face culling");
   glRasterPos2i(60, 270);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "Arrow Keys - Rotate");
   glRasterPos2i(60, 240);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "a - Increase velocity");
   glRasterPos2i(60, 210);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "z - Decrease velocity");
   glRasterPos2i(60, 180);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "p - Toggle Wire frame");
   glRasterPos2i(60, 150);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24,
	       "n - Toggle GL_EXT_rescale_normal extension");
   glRasterPos2i(60, 120);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24,
	       "+/- - Increase/decrease the Object maximum LOD");

   glRasterPos2i(60, 90);
   if (joyavailable)
      printstring(GLUT_BITMAP_TIMES_ROMAN_24,
		  "j - Toggle jostick control (Joystick control available)");
   else
      printstring(GLUT_BITMAP_TIMES_ROMAN_24,
		  "(No Joystick control available)");
}

static void
dojoy(void)
{
#ifdef _WIN32
   static UINT max[2] = { 0, 0 };
   static UINT min[2] = { 0xffffffff, 0xffffffff }, center[2];
   MMRESULT res;
   JOYINFO joy;

   res = joyGetPos(JOYSTICKID1, &joy);

   if (res == JOYERR_NOERROR) {
      joyavailable = 1;

      if (max[0] < joy.wXpos)
	 max[0] = joy.wXpos;
      if (min[0] > joy.wXpos)
	 min[0] = joy.wXpos;
      center[0] = (max[0] + min[0]) / 2;

      if (max[1] < joy.wYpos)
	 max[1] = joy.wYpos;
      if (min[1] > joy.wYpos)
	 min[1] = joy.wYpos;
      center[1] = (max[1] + min[1]) / 2;

      if (joyactive) {
	 if (fabs(center[0] - (float) joy.wXpos) > 0.1 * (max[0] - min[0]))
	    alpha -=
	       2.0 * (center[0] - (float) joy.wXpos) / (max[0] - min[0]);
	 if (fabs(center[1] - (float) joy.wYpos) > 0.1 * (max[1] - min[1]))
	    beta += 2.0 * (center[1] - (float) joy.wYpos) / (max[1] - min[1]);

	 if (joy.wButtons & JOY_BUTTON1)
	    v += 0.01;
	 if (joy.wButtons & JOY_BUTTON2)
	    v -= 0.01;
      }
   }
   else
      joyavailable = 0;
#endif
}

static void
drawipers(int depth, int from)
{
   int lod;

   if (depth == maxdepth)
      return;

   lod = depth + LODbias;
   if (lod < 0)
      lod = 0;
   if (lod >= MAX_LOD)
      return;

   switch (from) {
   case FROM_NONE:
      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_DOWN);
      drawipers(depth, FROM_UP);
      drawipers(depth, FROM_FRONT);
      drawipers(depth, FROM_BACK);
      drawipers(depth, FROM_LEFT);
      drawipers(depth, FROM_RIGHT);
      break;
   case FROM_FRONT:
      glPushMatrix();
      glTranslatef(0.0f, -1.5f, 0.0f);
      glScalef(0.5f, 0.5f, 0.5f);

      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_DOWN);
      drawipers(depth, FROM_UP);
      drawipers(depth, FROM_FRONT);
      drawipers(depth, FROM_LEFT);
      drawipers(depth, FROM_RIGHT);
      glPopMatrix();
      break;
   case FROM_BACK:
      glPushMatrix();
      glTranslatef(0.0f, 1.5f, 0.0f);
      glScalef(0.5f, 0.5f, 0.5f);

      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_DOWN);
      drawipers(depth, FROM_UP);
      drawipers(depth, FROM_BACK);
      drawipers(depth, FROM_LEFT);
      drawipers(depth, FROM_RIGHT);
      glPopMatrix();
      break;
   case FROM_LEFT:
      glPushMatrix();
      glTranslatef(-1.5f, 0.0f, 0.0f);
      glScalef(0.5f, 0.5f, 0.5f);

      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_DOWN);
      drawipers(depth, FROM_UP);
      drawipers(depth, FROM_FRONT);
      drawipers(depth, FROM_BACK);
      drawipers(depth, FROM_LEFT);
      glPopMatrix();
      break;
   case FROM_RIGHT:
      glPushMatrix();
      glTranslatef(1.5f, 0.0f, 0.0f);
      glScalef(0.5f, 0.5f, 0.5f);

      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_DOWN);
      drawipers(depth, FROM_UP);
      drawipers(depth, FROM_FRONT);
      drawipers(depth, FROM_BACK);
      drawipers(depth, FROM_RIGHT);
      glPopMatrix();
      break;
   case FROM_DOWN:
      glPushMatrix();
      glTranslatef(0.0f, 0.0f, 1.5f);
      glScalef(0.5f, 0.5f, 0.5f);

      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_DOWN);
      drawipers(depth, FROM_FRONT);
      drawipers(depth, FROM_BACK);
      drawipers(depth, FROM_LEFT);
      drawipers(depth, FROM_RIGHT);
      glPopMatrix();
      break;
   case FROM_UP:
      glPushMatrix();
      glTranslatef(0.0f, 0.0f, -1.5f);
      glScalef(0.5f, 0.5f, 0.5f);

      glCallList(LODdlist[lod]);

      depth++;
      drawipers(depth, FROM_UP);
      drawipers(depth, FROM_FRONT);
      drawipers(depth, FROM_BACK);
      drawipers(depth, FROM_LEFT);
      drawipers(depth, FROM_RIGHT);
      glPopMatrix();
      break;
   }

   totpoly += LODnumpoly[lod];
}

static void
draw(void)
{
   static char frbuf[80] = "";
   static GLfloat alpha = 0.0f;
   static GLfloat beta = 0.0f;
   static float fr = 0.0;
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;

   dojoy();

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (usetex)
      glEnable(GL_TEXTURE_2D);
   else
      glDisable(GL_TEXTURE_2D);

   if (fog)
      glEnable(GL_FOG);
   else
      glDisable(GL_FOG);

   glPushMatrix();
   calcposobs();
   gluLookAt(obs[0], obs[1], obs[2],
	     obs[0] + dir[0], obs[1] + dir[1], obs[2] + dir[2],
	     0.0, 0.0, 1.0);

   /* Scene */
   glEnable(GL_DEPTH_TEST);

   glShadeModel(GL_SMOOTH);
   glBindTexture(GL_TEXTURE_2D, t1id);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor3f(1.0f, 1.0f, 1.0f);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHTING);

   if (normext)
      glEnable(GL_RESCALE_NORMAL_EXT);
   else
      glEnable(GL_NORMALIZE);

   glPushMatrix();
   glRotatef(alpha, 0.0f, 0.0f, 1.0f);
   glRotatef(beta, 1.0f, 0.0f, 0.0f);
   totpoly = 0;
   drawipers(0, FROM_NONE);
   glPopMatrix();

   alpha += 4.f * dt;
   beta += 2.4f * dt;

   glDisable(GL_LIGHTING);
   glDisable(GL_LIGHT0);
   glShadeModel(GL_FLAT);

   if (normext)
      glDisable(GL_RESCALE_NORMAL_EXT);
   else
      glDisable(GL_NORMALIZE);

   glCallList(skydlist);

   glPopMatrix();

   /* Help Screen */

   sprintf(frbuf,
	   "Frame rate: %0.2f   LOD: %d   Tot. poly.: %d   Poly/sec: %.1f",
	   fr, LODbias, totpoly, totpoly * fr);

   glDisable(GL_TEXTURE_2D);
   glDisable(GL_FOG);
   glShadeModel(GL_FLAT);
   glDisable(GL_DEPTH_TEST);

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(-0.5, 639.5, -0.5, 479.5, -1.0, 1.0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(1.0, 0.0, 0.0);
   glRasterPos2i(10, 10);
   printstring(GLUT_BITMAP_HELVETICA_18, frbuf);
   glRasterPos2i(350, 470);
   printstring(GLUT_BITMAP_HELVETICA_10,
	       "IperS V1.0 Written by David Bucciarelli (tech.hmw@plus.it)");

   if (help)
      printhelp();

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);

   glutSwapBuffers();

   Frames++;
   {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      if (t - T0 >= 2000) {
         GLfloat seconds = (t - T0) / 1000.0;
         fr = Frames / seconds;
         T0 = t;
         Frames = 0;
      }
   }
}

int
main(int ac, char **av)
{
   float fogcolor[4] = { 0.7, 0.7, 0.7, 1.0 };

   fprintf(stderr,
	   "IperS V1.0\nWritten by David Bucciarelli (tech.hmw@plus.it)\n");

   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WIDTH, HEIGHT);
   glutInit(&ac, av);

   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

   if (!(win = glutCreateWindow("IperS"))) {
      fprintf(stderr, "Error, couldn't open window\n");
      exit(-1);
   }

   reshape(WIDTH, HEIGHT);

   glShadeModel(GL_SMOOTH);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glEnable(GL_FOG);
   glFogi(GL_FOG_MODE, GL_EXP2);
   glFogfv(GL_FOG_COLOR, fogcolor);

   glFogf(GL_FOG_DENSITY, 0.006);

   glHint(GL_FOG_HINT, GL_NICEST);

   inittextures();
   initdlists();
   initlight();

   glClearColor(fogcolor[0], fogcolor[1], fogcolor[2], fogcolor[3]);
   glClear(GL_COLOR_BUFFER_BIT);

   calcposobs();

   glutReshapeFunc(reshape);
   glutDisplayFunc(draw);
   glutKeyboardFunc(key);
   glutSpecialFunc(special);
   glutIdleFunc(draw);

   glutMainLoop();

   return 0;
}
