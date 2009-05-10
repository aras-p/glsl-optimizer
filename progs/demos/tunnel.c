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

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include "readtex.h"
#include "tunneldat.h"

#ifdef XMESA
#include "GL/xmesa.h"
static int fullscreen = 1;
#endif

static int WIDTH = 640;
static int HEIGHT = 480;

static GLint T0 = 0;
static GLint Frames = 0;
static GLint NiceFog = 1;

#define NUMBLOC 5

#ifndef M_PI
#define M_PI 3.1415926535
#endif

/*
extern int striplength_skin_13[];
extern float stripdata_skin_13[];

extern int striplength_skin_12[];
extern float stripdata_skin_12[];

extern int striplength_skin_11[];
extern float stripdata_skin_11[];

extern int striplength_skin_9[];
extern float stripdata_skin_9[];
*/

static int win = 0;

static float obs[3] = { 1000.0, 0.0, 2.0 };
static float dir[3];
static float v = 30.;
static float alpha = 90.0;
static float beta = 90.0;

static int fog = 1;
static int bfcull = 1;
static int usetex = 1;
static int cstrip = 0;
static int help = 1;
static int joyavailable = 0;
static int joyactive = 0;

static GLuint t1id, t2id;

static void
inittextures(void)
{
   glGenTextures(1, &t1id);
   glBindTexture(GL_TEXTURE_2D, t1id);

   if (!LoadRGBMipmaps("../images/tile.rgb", GL_RGB)) {
      fprintf(stderr, "Error reading a texture.\n");
      exit(-1);
   }

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_NEAREST);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glGenTextures(1, &t2id);
   glBindTexture(GL_TEXTURE_2D, t2id);

   if (!LoadRGBMipmaps("../images/bw.rgb", GL_RGB)) {
      fprintf(stderr, "Error reading a texture.\n");
      exit(-1);
   }

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void
drawobjs(const int *l, const float *f)
{
   int mend, j;

   if (cstrip) {
      float r = 0.33, g = 0.33, b = 0.33;

      for (; (*l) != 0;) {
	 mend = *l++;

	 r += 0.33;
	 if (r > 1.0) {
	    r = 0.33;
	    g += 0.33;
	    if (g > 1.0) {
	       g = 0.33;
	       b += 0.33;
	       if (b > 1.0)
		  b = 0.33;
	    }
	 }

	 glColor3f(r, g, b);
	 glBegin(GL_TRIANGLE_STRIP);
	 for (j = 0; j < mend; j++) {
	    f += 4;
	    glTexCoord2fv(f);
	    f += 2;
	    glVertex3fv(f);
	    f += 3;
	 }
	 glEnd();
      }
   }
   else
      for (; (*l) != 0;) {
	 mend = *l++;

	 glBegin(GL_TRIANGLE_STRIP);
	 for (j = 0; j < mend; j++) {
	    glColor4fv(f);
	    f += 4;
	    glTexCoord2fv(f);
	    f += 2;
	    glVertex3fv(f);
	    f += 3;
	 }
	 glEnd();
      }
}

static void
calcposobs(void)
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;

   dir[0] = sin(alpha * M_PI / 180.0);
   dir[1] = cos(alpha * M_PI / 180.0) * sin(beta * M_PI / 180.0);
   dir[2] = cos(beta * M_PI / 180.0);

   if (dir[0] < 1.0e-5 && dir[0] > -1.0e-5)
      dir[0] = 0;
   if (dir[1] < 1.0e-5 && dir[1] > -1.0e-5)
      dir[1] = 0;
   if (dir[2] < 1.0e-5 && dir[2] > -1.0e-5)
      dir[2] = 0;

   obs[0] += v * dir[0] * dt;
   obs[1] += v * dir[1] * dt;
   obs[2] += v * dir[2] * dt;
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
cleanup(void)
{
   glDeleteTextures(1, &t1id);
   glDeleteTextures(1, &t2id);
}

static void
key(unsigned char k, int x, int y)
{
   switch (k) {
   case 27:
      cleanup();
      exit(0);
      break;

   case 'a':
      v += 5.;
      break;
   case 'z':
      v -= 5.;
      break;

#ifdef XMESA
   case ' ':
      fullscreen = (!fullscreen);
      XMesaSetFXmode(fullscreen ? XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW);
      break;
#endif
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
   case 'm':
      cstrip = (!cstrip);
      break;

   case 'd':
      fprintf(stderr, "Deleting textures...\n");
      glDeleteTextures(1, &t1id);
      glDeleteTextures(1, &t2id);
      fprintf(stderr, "Loading textures...\n");
      inittextures();
      fprintf(stderr, "Done.\n");
      break;
   case 'n':
      NiceFog = !NiceFog;
      printf("NiceFog %d\n", NiceFog);
      break;
   }
   glutPostRedisplay();
}

static void
reshape(int w, int h)
{
   WIDTH = w;
   HEIGHT = h;
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(80.0, w / (float) h, 1.0, 50.0);
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
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glColor4f(0.0, 0.0, 0.0, 0.5);
   glRecti(40, 40, 600, 440);

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
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "m - Toggle strips");
   glRasterPos2i(60, 270);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "b - Toggle Back face culling");
   glRasterPos2i(60, 240);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "Arrow Keys - Rotate");
   glRasterPos2i(60, 210);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "a - Increase velocity");
   glRasterPos2i(60, 180);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "z - Decrease velocity");

   glRasterPos2i(60, 150);
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
#ifdef WIN32
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
draw(void)
{
   static char frbuf[80] = "";
   int i;
   float base, offset;

   if (NiceFog)
      glHint(GL_FOG_HINT, GL_NICEST);
   else
      glHint(GL_FOG_HINT, GL_DONT_CARE);

   dojoy();

   glClear(GL_COLOR_BUFFER_BIT);

   if (usetex)
      glEnable(GL_TEXTURE_2D);
   else
      glDisable(GL_TEXTURE_2D);

   if (fog)
      glEnable(GL_FOG);
   else
      glDisable(GL_FOG);

   glShadeModel(GL_SMOOTH);

   glPushMatrix();
   calcposobs();
   gluLookAt(obs[0], obs[1], obs[2],
	     obs[0] + dir[0], obs[1] + dir[1], obs[2] + dir[2],
	     0.0, 0.0, 1.0);

   if (dir[0] > 0) {
      offset = 8.0;
      base = obs[0] - fmod(obs[0], 8.0);
   }
   else {
      offset = -8.0;
      base = obs[0] + (8.0 - fmod(obs[0], 8.0));
   }

   glPushMatrix();
   glTranslatef(base - offset / 2.0, 0.0, 0.0);
   for (i = 0; i < NUMBLOC; i++) {
      glTranslatef(offset, 0.0, 0.0);
      glBindTexture(GL_TEXTURE_2D, t1id);
      drawobjs(striplength_skin_11, stripdata_skin_11);
      glBindTexture(GL_TEXTURE_2D, t2id);
      drawobjs(striplength_skin_12, stripdata_skin_12);
      drawobjs(striplength_skin_9, stripdata_skin_9);
      drawobjs(striplength_skin_13, stripdata_skin_13);
   }
   glPopMatrix();
   glPopMatrix();

   glDisable(GL_TEXTURE_2D);
   glDisable(GL_FOG);
   glShadeModel(GL_FLAT);

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
	       "Tunnel V1.5 Written by David Bucciarelli (tech.hmw@plus.it)");

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
         GLfloat fps = Frames / seconds;
         sprintf(frbuf, "Frame rate: %f", fps);
         T0 = t;
         Frames = 0;
      }
   }
}

static void
idle(void)
{
   glutPostRedisplay();
}



int
main(int ac, char **av)
{
   float fogcolor[4] = { 0.7, 0.7, 0.7, 1.0 };

   fprintf(stderr,
	   "Tunnel V1.5\nWritten by David Bucciarelli (tech.hmw@plus.it)\n");

   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WIDTH, HEIGHT);
   glutInit(&ac, av);

   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

   if (!(win = glutCreateWindow("Tunnel"))) {
      fprintf(stderr, "Error, couldn't open window\n");
      return -1;
   }

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(80.0, WIDTH / (float) HEIGHT, 1.0, 50.0);

   glMatrixMode(GL_MODELVIEW);

   glShadeModel(GL_SMOOTH);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);

   glEnable(GL_FOG);
   glFogi(GL_FOG_MODE, GL_EXP2);
   glFogfv(GL_FOG_COLOR, fogcolor);

   glFogf(GL_FOG_DENSITY, 0.06);
   glHint(GL_FOG_HINT, GL_NICEST);

   inittextures();

   glClearColor(fogcolor[0], fogcolor[1], fogcolor[2], fogcolor[3]);
   glClear(GL_COLOR_BUFFER_BIT);

   calcposobs();

   glutReshapeFunc(reshape);
   glutDisplayFunc(draw);
   glutKeyboardFunc(key);
   glutSpecialFunc(special);
   glutIdleFunc(idle);

   glEnable(GL_BLEND);
   /*glBlendFunc(GL_SRC_ALPHA_SATURATE,GL_ONE); */
   /*glEnable(GL_POLYGON_SMOOTH); */

   glutMainLoop();

   cleanup();
   return 0;
}
