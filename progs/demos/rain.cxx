/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (humanware@plus.it)
 *            Humanware s.r.l.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <GL/glut.h>
#ifndef M_PI
#define M_PI 3.14159265
#endif

#include "particles.h"
extern "C" {
#include "readtex.h"
}

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include "particles.cxx"
#include "readtex.c"
#endif

#ifdef XMESA
#include "GL/xmesa.h"
static int fullscreen=1;
#endif

static int WIDTH=640;
static int HEIGHT=480;
static int NUMPART=7500;

#define FRAME 50

static float fogcolor[4]={1.0,1.0,1.0,1.0};

#define DIMP 40.0
#define DIMTP 32.0

static float q[4][3]={
  {-DIMP,0.0,-DIMP},
  {DIMP,0.0,-DIMP},
  {DIMP,0.0,DIMP},
  {-DIMP,0.0,DIMP}
};

static float qt[4][2]={
  {-DIMTP,-DIMTP},
  {DIMTP,-DIMTP},
  {DIMTP,DIMTP},
  {-DIMTP,DIMTP}
};

static int win=0;

static int fog=1;
static int help=1;

static GLuint groundid;

static float obs[3]={2.0,1.0,0.0};
static float dir[3];
static float v=0.0;
static float alpha=-90.0;
static float beta=90.0;

static particleSystem *ps;

static float gettime()
{
  static clock_t told=0;
  clock_t tnew,ris;

  tnew=clock();

  ris=tnew-told;

  told=tnew;

  return(ris/(float)CLOCKS_PER_SEC);
}

static float gettimerain()
{
  static clock_t told=0;
  clock_t tnew,ris;

  tnew=clock();

  ris=tnew-told;

  told=tnew;

  return(ris/(float)CLOCKS_PER_SEC);
}

static void calcposobs(void)
{
  dir[0]=sin(alpha*M_PI/180.0);
  dir[2]=cos(alpha*M_PI/180.0)*sin(beta*M_PI/180.0);
  dir[1]=cos(beta*M_PI/180.0);

  obs[0]+=v*dir[0];
  obs[1]+=v*dir[1];
  obs[2]+=v*dir[2];

  rainParticle::setRainingArea(obs[0]-7.0f,-0.2f,obs[2]-7.0f,obs[0]+7.0f,8.0f,obs[2]+7.0f);
}

static void printstring(void *font, const char *string)
{
  int len,i;

  len=(int)strlen(string);
  for(i=0;i<len;i++)
    glutBitmapCharacter(font,string[i]);
}

static void reshape(int width, int height)
{
  WIDTH=width;
  HEIGHT=height;
  glViewport(0,0,(GLint)width,(GLint)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(70.0,width/(float)height,0.1,30.0);

  glMatrixMode(GL_MODELVIEW);
}

static void printhelp(void)
{
  glEnable(GL_BLEND);
  glColor4f(0.0,0.0,0.0,0.5);
  glRecti(40,40,600,440);
  glDisable(GL_BLEND);

  glColor3f(1.0,0.0,0.0);
  glRasterPos2i(300,420);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"Help");

  glRasterPos2i(60,390);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"h - Toggle Help");

  glRasterPos2i(60,360);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"f - Toggle Fog");
  glRasterPos2i(60,330);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"Arrow Keys - Rotate");
  glRasterPos2i(60,300);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"a - Increase velocity");
  glRasterPos2i(60,270);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"z - Decrease velocity");
  glRasterPos2i(60,240);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"l - Increase rain length");
  glRasterPos2i(60,210);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"k - Increase rain length");
}

static void drawrain(void)
{
  static int count=0;
  static char frbuf[80];
  float fr;

  glEnable(GL_DEPTH_TEST);

  if(fog)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);

  glDepthMask(GL_TRUE);
  glClearColor(1.0,1.0,1.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  calcposobs();
  gluLookAt(obs[0],obs[1],obs[2],
	    obs[0]+dir[0],obs[1]+dir[1],obs[2]+dir[2],
	    0.0,1.0,0.0);

  glColor4f(1.0,1.0,1.0,1.0);

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D,groundid);
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

  // Particle System

  glDisable(GL_TEXTURE_2D);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_BLEND);

  ps->draw();
  ps->addTime(gettimerain());
  
  glShadeModel(GL_FLAT);
 

  if((count % FRAME)==0) {
    fr=gettime();
    sprintf(frbuf,"Frame rate: %f",FRAME/fr);
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor3f(1.0,0.0,0.0);
  glRasterPos2i(10,10);
  printstring(GLUT_BITMAP_HELVETICA_18,frbuf);
  glRasterPos2i(350,470);
  printstring(GLUT_BITMAP_HELVETICA_10,"Rain V1.0 Written by David Bucciarelli (humanware@plus.it)");

  if(help)
    printhelp();

  reshape(WIDTH,HEIGHT);
  glPopMatrix();

  glutSwapBuffers();

  count++;
}


static void special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_LEFT:
    alpha+=2.0;
    break;
  case GLUT_KEY_RIGHT:
    alpha-=2.0;
    break;
  case GLUT_KEY_DOWN:
    beta-=2.0;
    break;
  case GLUT_KEY_UP:
    beta+=2.0;
    break;
  }
}

static void key(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
    break;

  case 'a':
    v+=0.01;
    break;
  case 'z':
    v-=0.01;
    break;

  case 'l':
    rainParticle::setLength(rainParticle::getLength()+0.025f);
    break;
  case 'k':
    rainParticle::setLength(rainParticle::getLength()-0.025f);
    break;

  case 'h':
    help=(!help);
    break;
  case 'f':
    fog=(!fog);
    break;
#ifdef XMESA
  case ' ':
    XMesaSetFXmode(fullscreen ? XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW);
    fullscreen=(!fullscreen);
    break;
#endif
  }
}

static void inittextures(void)
{
  GLubyte *img;
  GLint width,height;
  GLenum format;
  GLenum gluerr;

  glGenTextures(1,&groundid);
  glBindTexture(GL_TEXTURE_2D,groundid);

  if(!(img=LoadRGBImage("../images/s128.rgb",&width,&height,&format))){
  	fprintf(stderr,"Error reading a texture.\n");
  	exit(-1);
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  if((gluerr=(GLenum)gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height,GL_RGB,
			       GL_UNSIGNED_BYTE, (GLvoid *)(img)))) {
    fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
    exit(-1);
  }

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
}

static void initparticle(void)
{
  ps=new particleSystem;

  rainParticle::setRainingArea(-7.0f,-0.2f,-7.0f,7.0f,8.0f,7.0f);

  for(int i=0;i<NUMPART;i++) {
    rainParticle *p=new rainParticle;
    p->randomHeight();

    ps->addParticle((particle *)p);
  }
}

int main(int ac,char **av)
{
  fprintf(stderr,"Rain V1.0\nWritten by David Bucciarelli (humanware@plus.it)\n");

  /* Default settings */

  WIDTH=640;
  HEIGHT=480;

  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  glutInit(&ac,av);

  glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);

  if(!(win=glutCreateWindow("Rain"))) {
    fprintf(stderr,"Error opening a window.\n");
    exit(-1);
  }
  
  reshape(WIDTH,HEIGHT);

  inittextures();

  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE,GL_EXP);
  glFogfv(GL_FOG_COLOR,fogcolor);
  glFogf(GL_FOG_DENSITY,0.1);
#ifdef FX
  glHint(GL_FOG_HINT,GL_NICEST);
#endif

  initparticle();

  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutDisplayFunc(drawrain);
  glutIdleFunc(drawrain);
  glutReshapeFunc(reshape);
  glutMainLoop();

  return(0);
}
