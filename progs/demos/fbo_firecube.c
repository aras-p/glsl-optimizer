/*
 * Draw the "fire" test program on the six faces of a cube using
 * fbo render-to-texture.
 * 
 * Much of the code comes from David Bucciarelli's "fire" 
 * test program. The rest basically from Brian Paul's "gearbox" and
 * fbotexture programs.
 *
 * By pressing the 'q' key, you can make the driver choose different
 * internal texture RGBA formats by giving it different "format" and "type"
 * arguments to the glTexImage2D function that creates the texture
 * image being rendered to. If the driver doesn't support a texture image
 * format as a render target, it will usually fall back to software when
 * drawing the "fire" image, and frame-rate should drop considerably.
 *
 * Performance: 
 * The rendering speed of this program is usually dictated by fill rate
 * and the fact that software fallbacks for glBitMap makes the driver
 * operate synchronously. Low-end UMA hardware will probably see around
 * 35 fps with the help screen disabled and 32bpp window- and user
 * frame-buffers (2008).
 *
 * This program is released under GPL, following the "fire" licensing.
 *
 * Authors:
 *   David Bucciarelli ("fire")
 *   Brian Paul ("gearbox", "fbotexture")
 *   Thomas Hellstrom (Putting it together)
 *
 */

#define GL_GLEXT_PROTOTYPES
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include "readtex.h"


/*
 * Format of texture we render to.
 */

#define TEXINTFORMAT GL_RGBA

static GLuint texTypes[] = 
   {GL_UNSIGNED_BYTE,
    GL_UNSIGNED_INT_8_8_8_8_REV,
    GL_UNSIGNED_SHORT_1_5_5_5_REV,
    GL_UNSIGNED_SHORT_4_4_4_4_REV,
    GL_UNSIGNED_INT_8_8_8_8,
    GL_UNSIGNED_SHORT_5_5_5_1,
    GL_UNSIGNED_SHORT_4_4_4_4,
    GL_UNSIGNED_INT_8_8_8_8_REV,
    GL_UNSIGNED_SHORT_1_5_5_5_REV,
    GL_UNSIGNED_SHORT_4_4_4_4_REV,
    GL_UNSIGNED_INT_8_8_8_8,
    GL_UNSIGNED_SHORT_5_5_5_1,
    GL_UNSIGNED_SHORT_4_4_4_4,
    GL_UNSIGNED_SHORT_5_6_5,
    GL_UNSIGNED_SHORT_5_6_5_REV,
    GL_UNSIGNED_SHORT_5_6_5,
    GL_UNSIGNED_SHORT_5_6_5_REV};

static GLuint texFormats[] = 
   {GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_BGRA,
    GL_BGRA,
    GL_BGRA,
    GL_BGRA,
    GL_BGRA,
    GL_BGRA,
    GL_RGB,
    GL_RGB,
    GL_BGR,
    GL_BGR};

static const char *texNames[] = 
   {"GL_RGBA GL_UNSIGNED_BYTE",
    "GL_RGBA GL_UNSIGNED_INT_8_8_8_8_REV",
    "GL_RGBA GL_UNSIGNED_SHORT_1_5_5_5_REV",
    "GL_RGBA GL_UNSIGNED_SHORT_4_4_4_4_REV",
    "GL_RGBA GL_UNSIGNED_INT_8_8_8_8",
    "GL_RGBA GL_UNSIGNED_INT_5_5_5_1",
    "GL_RGBA GL_UNSIGNED_INT_4_4_4_4",
    "GL_BGRA GL_UNSIGNED_INT_8_8_8_8_REV",
    "GL_BGRA GL_UNSIGNED_SHORT_1_5_5_5_REV",
    "GL_BGRA GL_UNSIGNED_SHORT_4_4_4_4_REV",
    "GL_BGRA GL_UNSIGNED_INT_8_8_8_8",
    "GL_BGRA GL_UNSIGNED_INT_5_5_5_1",
    "GL_BGRA GL_UNSIGNED_INT_4_4_4_4",
    "GL_RGB GL_UNSIGNED_INT_5_6_5",
    "GL_RGB GL_UNSIGNED_INT_5_6_5_REV",
    "GL_BGR GL_UNSIGNED_INT_5_6_5",
    "GL_BGR GL_UNSIGNED_INT_5_6_5_REV"};




#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define vinit(a,i,j,k) {			\
      (a)[0]=i;					\
      (a)[1]=j;					\
      (a)[2]=k;					\
   }

#define vinit4(a,i,j,k,w) {			\
      (a)[0]=i;					\
      (a)[1]=j;					\
      (a)[2]=k;					\
      (a)[3]=w;					\
   }


#define vadds(a,dt,b) {				\
      (a)[0]+=(dt)*(b)[0];			\
      (a)[1]+=(dt)*(b)[1];			\
      (a)[2]+=(dt)*(b)[2];			\
   }

#define vequ(a,b) {				\
      (a)[0]=(b)[0];				\
      (a)[1]=(b)[1];				\
      (a)[2]=(b)[2];				\
   }

#define vinter(a,dt,b,c) {			\
      (a)[0]=(dt)*(b)[0]+(1.0-dt)*(c)[0];	\
      (a)[1]=(dt)*(b)[1]+(1.0-dt)*(c)[1];	\
      (a)[2]=(dt)*(b)[2]+(1.0-dt)*(c)[2];	\
   }

#define clamp(a)        ((a) < 0.0 ? 0.0 : ((a) < 1.0 ? (a) : 1.0))

#define vclamp(v) {				\
      (v)[0]=clamp((v)[0]);			\
      (v)[1]=clamp((v)[1]);			\
      (v)[2]=clamp((v)[2]);			\
   }

static GLint WinWidth = 800, WinHeight = 800;
static GLint TexWidth, TexHeight;
static GLuint TexObj;
static GLuint MyFB;
static GLuint DepthRB;
static GLboolean WireFrame = GL_FALSE;
static GLint texType = 0;

static GLint T0 = 0;
static GLint Frames = 0;
static GLint Win = 0;

static GLfloat ViewRotX = 20.0, ViewRotY = 30.0, ViewRotZ = 0.0;
static GLfloat CubeRot = 0.0;

static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
      exit(1);
   }
}


static void
cleanup(void)
{
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glDeleteFramebuffersEXT(1, &MyFB);
   glDeleteRenderbuffersEXT(1, &DepthRB);
   glDeleteTextures(1, &TexObj);
   glutDestroyWindow(Win);
}

static GLint NiceFog = 1;

#define DIMP 20.0
#define DIMTP 16.0

#define RIDCOL 0.4

#define NUMTREE 50
#define TREEINR 2.5
#define TREEOUTR 8.0

#define AGRAV -9.8

typedef struct
{
   int age;
   float p[3][3];
   float v[3];
   float c[3][4];
}
   part;

static float treepos[NUMTREE][3];

static float black[3] = { 0.0, 0.0, 0.0 };
static float blu[3] = { 1.0, 0.2, 0.0 };
static float blu2[3] = { 1.0, 1.0, 0.0 };

static float fogcolor[4] = { 1.0, 1.0, 1.0, 1.0 };

static float q[4][3] = {
   {-DIMP, 0.0, -DIMP},
   {DIMP, 0.0, -DIMP},
   {DIMP, 0.0, DIMP},
   {-DIMP, 0.0, DIMP}
};

static float qt[4][2] = {
   {-DIMTP, -DIMTP},
   {DIMTP, -DIMTP},
   {DIMTP, DIMTP},
   {-DIMTP, DIMTP}
};

static int np;
static float eject_r, dt, maxage, eject_vy, eject_vl;
static short shadows;
static float ridtri;
static int fog = 0;
static int help = 1;

static part *p;

static GLuint groundid;
static GLuint treeid;

static float obs[3] = { 2.0, 1.0, 0.0 };
static float dir[3];
static float v = 0.0;
static float alpha = -84.0;
static float beta = 90.0;

static float
vrnd(void)
{
   return (((float) rand()) / RAND_MAX);
}

static void
setnewpart(part * p)
{
   float a, v[3], *c;

   p->age = 0;

   a = vrnd() * 3.14159265359 * 2.0;

   vinit(v, sin(a) * eject_r * vrnd(), 0.15, cos(a) * eject_r * vrnd());
   vinit(p->p[0], v[0] + vrnd() * ridtri, v[1] + vrnd() * ridtri,
	 v[2] + vrnd() * ridtri);
   vinit(p->p[1], v[0] + vrnd() * ridtri, v[1] + vrnd() * ridtri,
	 v[2] + vrnd() * ridtri);
   vinit(p->p[2], v[0] + vrnd() * ridtri, v[1] + vrnd() * ridtri,
	 v[2] + vrnd() * ridtri);

   vinit(p->v, v[0] * eject_vl / (eject_r / 2),
	 vrnd() * eject_vy + eject_vy / 2, v[2] * eject_vl / (eject_r / 2));

   c = blu;

   vinit4(p->c[0], c[0] * ((1.0 - RIDCOL) + vrnd() * RIDCOL),
	  c[1] * ((1.0 - RIDCOL) + vrnd() * RIDCOL),
	  c[2] * ((1.0 - RIDCOL) + vrnd() * RIDCOL), 1.0);
   vinit4(p->c[1], c[0] * ((1.0 - RIDCOL) + vrnd() * RIDCOL),
	  c[1] * ((1.0 - RIDCOL) + vrnd() * RIDCOL),
	  c[2] * ((1.0 - RIDCOL) + vrnd() * RIDCOL), 1.0);
   vinit4(p->c[2], c[0] * ((1.0 - RIDCOL) + vrnd() * RIDCOL),
	  c[1] * ((1.0 - RIDCOL) + vrnd() * RIDCOL),
	  c[2] * ((1.0 - RIDCOL) + vrnd() * RIDCOL), 1.0);
}

static void
setpart(part * p)
{
   float fact;

   if (p->p[0][1] < 0.1) {
      setnewpart(p);
      return;
   }

   p->v[1] += AGRAV * dt;

   vadds(p->p[0], dt, p->v);
   vadds(p->p[1], dt, p->v);
   vadds(p->p[2], dt, p->v);

   p->age++;

   if ((p->age) > maxage) {
      vequ(p->c[0], blu2);
      vequ(p->c[1], blu2);
      vequ(p->c[2], blu2);
   }
   else {
      fact = 1.0 / maxage;
      vadds(p->c[0], fact, blu2);
      vclamp(p->c[0]);
      p->c[0][3] = fact * (maxage - p->age);

      vadds(p->c[1], fact, blu2);
      vclamp(p->c[1]);
      p->c[1][3] = fact * (maxage - p->age);

      vadds(p->c[2], fact, blu2);
      vclamp(p->c[2]);
      p->c[2][3] = fact * (maxage - p->age);
   }
}

static void
drawtree(float x, float y, float z)
{
   glBegin(GL_QUADS);
   glTexCoord2f(0.0, 0.0);
   glVertex3f(x - 1.5, y + 0.0, z);

   glTexCoord2f(1.0, 0.0);
   glVertex3f(x + 1.5, y + 0.0, z);

   glTexCoord2f(1.0, 1.0);
   glVertex3f(x + 1.5, y + 3.0, z);

   glTexCoord2f(0.0, 1.0);
   glVertex3f(x - 1.5, y + 3.0, z);


   glTexCoord2f(0.0, 0.0);
   glVertex3f(x, y + 0.0, z - 1.5);

   glTexCoord2f(1.0, 0.0);
   glVertex3f(x, y + 0.0, z + 1.5);

   glTexCoord2f(1.0, 1.0);
   glVertex3f(x, y + 3.0, z + 1.5);

   glTexCoord2f(0.0, 1.0);
   glVertex3f(x, y + 3.0, z - 1.5);

   glEnd();

}

static void
calcposobs(void)
{
   dir[0] = sin(alpha * M_PI / 180.0);
   dir[2] = cos(alpha * M_PI / 180.0) * sin(beta * M_PI / 180.0);
   dir[1] = cos(beta * M_PI / 180.0);

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
printstring(void *font, const char *string)
{
   int len, i;

   len = (int) strlen(string);
   for (i = 0; i < len; i++)
      glutBitmapCharacter(font, string[i]);
}


static void
printhelp(void)
{
   glColor4f(0.0, 0.0, 0.0, 0.5);
   glRecti(40, 40, 600, 440);

   glColor3f(1.0, 0.0, 0.0);
   glRasterPos2i(300, 420);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "Help");

   glRasterPos2i(60, 390);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "h - Toggle Help");

   glRasterPos2i(60, 360);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "t - Increase particle size");
   glRasterPos2i(60, 330);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "T - Decrease particle size");

   glRasterPos2i(60, 300);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "r - Increase emission radius");
   glRasterPos2i(60, 270);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "R - Decrease emission radius");

   glRasterPos2i(60, 240);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "f - Toggle Fog");
   glRasterPos2i(60, 210);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "s - Toggle shadows");
   glRasterPos2i(60, 180);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "q - Toggle texture format & type");
   glRasterPos2i(60, 150);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "a - Increase velocity");
   glRasterPos2i(60, 120);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24, "z - Decrease velocity");
   glRasterPos2i(60, 90);
   printstring(GLUT_BITMAP_TIMES_ROMAN_24,  "Arrow Keys - Rotate");
}


static void
drawfire(void)
{
   static char frbuf[80] = "";
   int j;
   static double t0 = -1.;
   double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = (t - t0) * 1.0;
   t0 = t;

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);

   glDisable(GL_LIGHTING);

   glShadeModel(GL_FLAT);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glEnable(GL_FOG);
   glFogi(GL_FOG_MODE, GL_EXP);
   glFogfv(GL_FOG_COLOR, fogcolor);
   glFogf(GL_FOG_DENSITY, 0.1);


   glViewport(0, 0, (GLint) TexWidth, (GLint) TexHeight);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(70.0, TexWidth/ (float) TexHeight, 0.1, 30.0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   if (NiceFog)
      glHint(GL_FOG_HINT, GL_NICEST);
   else
      glHint(GL_FOG_HINT, GL_DONT_CARE);

   glEnable(GL_DEPTH_TEST);

   if (fog)
      glEnable(GL_FOG);
   else
      glDisable(GL_FOG);

   glDepthMask(GL_TRUE);
   glClearColor(1.0, 1.0, 1.0, 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


   glPushMatrix();
   calcposobs();
   gluLookAt(obs[0], obs[1], obs[2],
	     obs[0] + dir[0], obs[1] + dir[1], obs[2] + dir[2],
	     0.0, 1.0, 0.0);

   glColor4f(1.0, 1.0, 1.0, 1.0);

   glEnable(GL_TEXTURE_2D);

   glBindTexture(GL_TEXTURE_2D, groundid);
   glBegin(GL_QUADS);
   glTexCoord2fv(qt[0]);
   glVertex3fv(q[0]);
   glTexCoord2fv(qt[1]);
   glVertex3fv(q[1]);
   glTexCoord2fv(qt[2]);
   glVertex3fv(q[2]);
   glTexCoord2fv(qt[3]);
   glVertex3fv(q[3]);
   glEnd();

   glEnable(GL_ALPHA_TEST);
   glAlphaFunc(GL_GEQUAL, 0.9);

   glBindTexture(GL_TEXTURE_2D, treeid);
   for (j = 0; j < NUMTREE; j++)
      drawtree(treepos[j][0], treepos[j][1], treepos[j][2]);

   glDisable(GL_TEXTURE_2D);
   glDepthMask(GL_FALSE);
   glDisable(GL_ALPHA_TEST);

   if (shadows) {
      glBegin(GL_TRIANGLES);
      for (j = 0; j < np; j++) {
	 glColor4f(black[0], black[1], black[2], p[j].c[0][3]);
	 glVertex3f(p[j].p[0][0], 0.1, p[j].p[0][2]);

	 glColor4f(black[0], black[1], black[2], p[j].c[1][3]);
	 glVertex3f(p[j].p[1][0], 0.1, p[j].p[1][2]);

	 glColor4f(black[0], black[1], black[2], p[j].c[2][3]);
	 glVertex3f(p[j].p[2][0], 0.1, p[j].p[2][2]);
      }
      glEnd();
   }

   glBegin(GL_TRIANGLES);
   for (j = 0; j < np; j++) {
      glColor4fv(p[j].c[0]);
      glVertex3fv(p[j].p[0]);

      glColor4fv(p[j].c[1]);
      glVertex3fv(p[j].p[1]);

      glColor4fv(p[j].c[2]);
      glVertex3fv(p[j].p[2]);

      setpart(&p[j]);
   }
   glEnd();


   glDisable(GL_TEXTURE_2D);
   glDisable(GL_ALPHA_TEST);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_FOG);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-0.5, 639.5, -0.5, 479.5
	   , -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(1.0, 0.0, 0.0);
   glRasterPos2i(10, 10);
   printstring(GLUT_BITMAP_HELVETICA_18, frbuf);
   glColor3f(0.0, 0.0, 1.0);
   glRasterPos2i(10, 450);
   printstring(GLUT_BITMAP_HELVETICA_18, texNames[texType]);
   glColor3f(1.0, 0.0, 0.0);
   glRasterPos2i(10, 470);
   printstring(GLUT_BITMAP_HELVETICA_10,
	       "Fire V1.5 Written by David Bucciarelli (tech.hmw@plus.it)");

   if (help)
      printhelp();

   glPopMatrix();
   glDepthMask(GL_TRUE);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);   
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
regen_texImage(void)
{
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexImage2D(GL_TEXTURE_2D, 0, TEXINTFORMAT, TexWidth, TexHeight, 0,
		texFormats[texType], texTypes[texType], NULL);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_2D, TexObj, 0);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

static void
key(unsigned char key, int x, int y)
{
   switch (key) {
   case 27:
      cleanup();
      exit(0);
      break;

   case 'a':
      v += 0.0005;
      break;
   case 'z':
      v -= 0.0005;
      break;
   case 'h':
      help = (!help);
      break;
   case 'f':
      fog = (!fog);
      break;
   case 's':
      shadows = !shadows;
      break;
   case 'R':
      eject_r -= 0.03;
      break;
   case 'r':
      eject_r += 0.03;
      break;
   case 't':
      ridtri += 0.005;
      break;
   case 'T':
      ridtri -= 0.005;
      break;
   case 'v':
      ViewRotZ += 5.0;
      break;
   case 'V':
      ViewRotZ -= 5.0;
      break;
   case 'w':
      WireFrame = !WireFrame;
      break;
   case 'q':
      if (++texType > 16)
	 texType = 0;
      regen_texImage();
      break;
   case 'n':
      NiceFog = !NiceFog;
      printf("NiceFog %d\n", NiceFog);
      break;
   }
   glutPostRedisplay();
}

static void
inittextures(void)
{
   GLenum gluerr;
   GLubyte tex[128][128][4];

   glGenTextures(1, &groundid);
   glBindTexture(GL_TEXTURE_2D, groundid);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   if (!LoadRGBMipmaps("../images/s128.rgb", GL_RGB)) {
      fprintf(stderr, "Error reading a texture.\n");
      exit(-1);
   }

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

   glGenTextures(1, &treeid);
   glBindTexture(GL_TEXTURE_2D, treeid);

   if (1)
      {
	 int w, h;
	 GLenum format;
	 int x, y;
	 GLubyte *image = LoadRGBImage("../images/tree3.rgb", &w, &h, &format);

	 if (!image) {
	    fprintf(stderr, "Error reading a texture.\n");
	    exit(-1);
	 }

	 for (y = 0; y < 128; y++)
	    for (x = 0; x < 128; x++) {
	       tex[x][y][0] = image[(y + x * 128) * 3];
	       tex[x][y][1] = image[(y + x * 128) * 3 + 1];
	       tex[x][y][2] = image[(y + x * 128) * 3 + 2];
	       if ((tex[x][y][0] == tex[x][y][1]) &&
		   (tex[x][y][1] == tex[x][y][2]) && (tex[x][y][2] == 255))
		  tex[x][y][3] = 0;
	       else
		  tex[x][y][3] = 255;
	    }

	 if ((gluerr = gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 128, 128, GL_RGBA,
					 GL_UNSIGNED_BYTE, (GLvoid *) (tex)))) {
	    fprintf(stderr, "GLULib%s\n", (char *) gluErrorString(gluerr));
	    exit(-1);
	 }
      }
   else {
      if (!LoadRGBMipmaps("../images/tree2.rgba", GL_RGBA)) {
	 fprintf(stderr, "Error reading a texture.\n");
	 exit(-1);
      }
   }

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void
inittree(void)
{
   int i;
   float dist;

   for (i = 0; i < NUMTREE; i++)
      do {
	 treepos[i][0] = vrnd() * TREEOUTR * 2.0 - TREEOUTR;
	 treepos[i][1] = 0.0;
	 treepos[i][2] = vrnd() * TREEOUTR * 2.0 - TREEOUTR;
	 dist =
	    sqrt(treepos[i][0] * treepos[i][0] +
		 treepos[i][2] * treepos[i][2]);
      } while ((dist < TREEINR) || (dist > TREEOUTR));
}

static int 
init_fire(int ac, char *av[])
{
   int i;

   np = 800;
   eject_r = -0.65;
   dt = 0.015;
   eject_vy = 4;
   eject_vl = 1;
   shadows = 1;
   ridtri = 0.25;

   maxage = 1.0 / dt;

   if (ac == 2)
      np = atoi(av[1]);


   inittextures();

   p = (part *) malloc(sizeof(part) * np);

   for (i = 0; i < np; i++)
      setnewpart(&p[i]);

   inittree();

   return (0);
}






static void
DrawCube(void)
{
   static const GLfloat texcoords[4][2] = {
      { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
   };
   static const GLfloat vertices[4][2] = {
      { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 }
   };
   static const GLfloat xforms[6][4] = {
      {   0, 0, 1, 0 },
      {  90, 0, 1, 0 },
      { 180, 0, 1, 0 },
      { 270, 0, 1, 0 },
      {  90, 1, 0, 0 },
      { -90, 1, 0, 0 }
   };
   static const GLfloat mat[4] = { 1.0, 1.0, 0.5, 1.0 };
   GLint i, j;

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat);
   glEnable(GL_TEXTURE_2D);

   glPushMatrix();
   glRotatef(ViewRotX, 1.0, 0.0, 0.0);
   glRotatef(15, 1, 0, 0);
   glRotatef(CubeRot, 0, 1, 0);
   glScalef(4, 4, 4);

   for (i = 0; i < 6; i++) {
      glPushMatrix();
      glRotatef(xforms[i][0], xforms[i][1], xforms[i][2], xforms[i][3]);
      glTranslatef(0, 0, 1.1);
      glBegin(GL_POLYGON);
      glNormal3f(0, 0, 1);
      for (j = 0; j < 4; j++) {
	 glTexCoord2fv(texcoords[j]);
	 glVertex2fv(vertices[j]);
      }
      glEnd();
      glPopMatrix();
   }
   glPopMatrix();

   glDisable(GL_TEXTURE_2D);
}


static void
draw(void)
{
   float ar;
   static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};

   drawfire();

   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,
		 GL_SEPARATE_SPECULAR_COLOR);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_NORMALIZE);
   glDisable(GL_BLEND);
   glDisable(GL_FOG);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);

   glClear(GL_DEPTH_BUFFER_BIT);

   /* draw textured cube */

   glViewport(0, 0, WinWidth, WinHeight);
   glClearColor(0.5, 0.5, 0.8, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   ar = (float) (WinWidth) / WinHeight;
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   glBindTexture(GL_TEXTURE_2D, TexObj);

   DrawCube();

   /* finish up */
   glutSwapBuffers();
}


static void
idle(void)
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;

   CubeRot = fmod(CubeRot + 15.0 * dt, 360.0);  /* 15 deg/sec */

   glutPostRedisplay();
}


/* change view angle */
static void
special(int k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
   case GLUT_KEY_UP:
      ViewRotX += 5.0;
      break;
   case GLUT_KEY_DOWN:
      ViewRotX -= 5.0;
      break;
   case GLUT_KEY_LEFT:
      ViewRotY += 5.0;
      break;
   case GLUT_KEY_RIGHT:
      ViewRotY -= 5.0;
      break;
   default:
      return;
   }
   glutPostRedisplay();
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
}


static void 
init_fbotexture()
{
   GLint i;

   /* gen framebuffer id, delete it, do some assertions, just for testing */
   glGenFramebuffersEXT(1, &MyFB);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &i);

   /* Make texture object/image */
   glGenTextures(1, &TexObj);
   glBindTexture(GL_TEXTURE_2D, TexObj);
   /* make one image level. */
   glTexImage2D(GL_TEXTURE_2D, 0, TEXINTFORMAT, TexWidth, TexHeight, 0,
		texFormats[texType], texTypes[texType], NULL);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

   CheckError(__LINE__);

   /* Render color to texture */
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_2D, TexObj, 0);
   CheckError(__LINE__);


   /* make depth renderbuffer */
   glGenRenderbuffersEXT(1, &DepthRB);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthRB);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16,
			    TexWidth, TexHeight);
   CheckError(__LINE__);
   glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT,
				   GL_RENDERBUFFER_DEPTH_SIZE_EXT, &i);
   CheckError(__LINE__);
   printf("Depth renderbuffer size = %d bits\n", i);

   /* attach DepthRB to MyFB */
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
				GL_RENDERBUFFER_EXT, DepthRB);
   CheckError(__LINE__);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

   /*
    * Check for completeness.
    */

}


static void
init(int argc, char *argv[])
{
   GLint i;

   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      fprintf(stderr, "Sorry, GL_EXT_framebuffer_object is required!\n");
      exit(1);
   }

   TexWidth = 512;
   TexHeight = 512;

   init_fbotexture();
   init_fire(argc, argv);


   for ( i=1; i<argc; i++ ) {
      if (strcmp(argv[i], "-info")==0) {
	 printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
	 printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
	 printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
	 printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
      }
   }
}


static void 
visible(int vis)
{
   if (vis == GLUT_VISIBLE)
      glutIdleFunc(idle);
   else
      glutIdleFunc(NULL);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);

   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

   glutInitWindowSize(WinWidth, WinHeight);
   Win = glutCreateWindow("fbo_firecube");
   init(argc, argv);

   glutDisplayFunc(draw);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(key);
   glutSpecialFunc(special);
   glutVisibilityFunc(visible);

   glutMainLoop();
   return 0;             /* ANSI C requires main to return int. */
}
