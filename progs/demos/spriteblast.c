
/* Copyright (c) Mark J. Kilgard, 1997.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This example demonstrates how to render particle effects
   with OpenGL.  A cloud of pinkish/orange particles explodes with the
   particles bouncing off the ground.  When the EXT_point_parameters
   is present , the particle size is attenuated based on eye distance. */


/* Modified by Brian Paul to test GL_ARB_point_sprite */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#ifdef _WIN32
#include <windows.h>
#endif
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265
#endif

#if 0  /* For debugging. */
#undef GL_EXT_point_parameters
#endif

static GLfloat angle = -150;   /* in degrees */
static int spin = 0;
static int moving, begin;
static float theTime;
static int repeat = 1;
static int blend = 1;
int useMipmaps = 1;
int linearFiltering = 1;

static GLfloat constant[3] = { .2,  0.0,     0.0 };
static GLfloat linear[3]   = { .0,   .1,     0.0 };
static GLfloat theQuad[3]  = { .005, 0.1, 1/600.0 };

#define MAX_POINTS 2000

static int numPoints = 200;

static GLfloat pointList[MAX_POINTS][3];
static GLfloat pointTime[MAX_POINTS];
static GLfloat pointVelocity[MAX_POINTS][2];
static GLfloat pointDirection[MAX_POINTS][2];
static int colorList[MAX_POINTS];
static int animate = 1, motion = 0, org = 0, sprite = 1, smooth = 1;

static GLfloat colorSet[][4] = {
  /* Shades of red. */
  { 0.7, 0.2, 0.4, 0.5 },
  { 0.8, 0.0, 0.7, 0.5 },
  { 1.0, 0.0, 0.0, 0.5 },
  { 0.9, 0.3, 0.6, 0.5 },
  { 1.0, 0.4, 0.0, 0.5 },
  { 1.0, 0.0, 0.5, 0.5 },
};

#define NUM_COLORS (sizeof(colorSet)/sizeof(colorSet[0]))

#define DEAD (NUM_COLORS+1)


/* GL */
static GLint spritePattern[16][16] = {
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0 },
   { 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0 },
   { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};




#if 0  /* drand48 might be better on Unix machines */
#define RANDOM_RANGE(lo, hi) ((lo) + (hi - lo) * drand48())
#else
static float float_rand(void) { return rand() / (float) RAND_MAX; }
#define RANDOM_RANGE(lo, hi) ((lo) + (hi - lo) * float_rand())
#endif

#define MEAN_VELOCITY 3.0
#define GRAVITY 2.0

/* Modeling units of ground extent in each X and Z direction. */
#define EDGE 12

static void
makePointList(void)
{
  float angle, velocity, direction;
  int i;

  motion = 1;
  for (i=0; i<numPoints; i++) {
    pointList[i][0] = 0.0;
    pointList[i][1] = 0.0;
    pointList[i][2] = 0.0;
    pointTime[i] = 0.0;
    angle = (RANDOM_RANGE(60.0, 70.0)) * M_PI/180.0;
    direction = RANDOM_RANGE(0.0, 360.0) * M_PI/180.0;
    pointDirection[i][0] = cos(direction);
    pointDirection[i][1] = sin(direction);
    velocity = MEAN_VELOCITY + RANDOM_RANGE(-0.8, 1.0);
    pointVelocity[i][0] = velocity * cos(angle);
    pointVelocity[i][1] = velocity * sin(angle);
    colorList[i] = rand() % NUM_COLORS;
  }
  theTime = 0.0;
}

static void
updatePointList(void)
{
  float distance;
  int i;

  static double t0 = -1.;
  double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
  if (t0 < 0.0)
    t0 = t;
  dt = t - t0;
  t0 = t;

  motion = 0;
  for (i=0; i<numPoints; i++) {
    distance = pointVelocity[i][0] * theTime;

    /* X and Z */
    pointList[i][0] = pointDirection[i][0] * distance;
    pointList[i][2] = pointDirection[i][1] * distance;

    /* Z */
    pointList[i][1] =
      (pointVelocity[i][1] - 0.5 * GRAVITY * pointTime[i])*pointTime[i];

    /* If we hit the ground, bounce the point upward again. */
    if (pointList[i][1] <= 0.0) {
      if (distance > EDGE) {
        /* Particle has hit ground past the distance duration of
          the particles.  Mark particle as dead. */
       colorList[i] = NUM_COLORS;  /* Not moving. */
       continue;
      }

      pointVelocity[i][1] *= 0.8;  /* 80% of previous up velocity. */
      pointTime[i] = 0.0;  /* Reset the particles sense of up time. */
    }
    motion = 1;
    pointTime[i] += dt;
  }
  theTime += dt;
  if (!motion && !spin) {
    if (repeat) {
      makePointList();
    } else {
      glutIdleFunc(NULL);
    }
  }
}

static void
idle(void)
{
  updatePointList();
  if (spin) {
    angle += 0.3;
  }
  glutPostRedisplay();
}

static void
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (animate && (motion || spin)) {
      glutIdleFunc(idle);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

static void
redraw(void)
{
  int i;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glRotatef(15.0, 1.0, 0.0, 0.0);
  glRotatef(angle, 0.0, 1.0, 0.0);

  glDepthMask(GL_FALSE);

  /* Draw the floor. */
/*  glEnable(GL_TEXTURE_2D);*/
  glColor3f(0.1, 0.5, 1.0);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-EDGE, -0.05, -EDGE);
    glTexCoord2f(20.0, 0.0);
    glVertex3f(EDGE, -0.05, -EDGE);
    glTexCoord2f(20.0, 20.0);
    glVertex3f(EDGE, -0.05, EDGE);
    glTexCoord2f(0.0, 20.0);
    glVertex3f(-EDGE, -0.05, EDGE);
  glEnd();

  /* Allow particles to blend with each other. */
  glDepthMask(GL_TRUE);

  if (blend)
     glEnable(GL_BLEND);

  if (sprite) {
     glEnable(GL_TEXTURE_2D);
#ifdef GL_ARB_point_sprite
     glEnable(GL_POINT_SPRITE_ARB);
#endif
  }

  glColor3f(1,1,1);
  glBegin(GL_POINTS);
    for (i=0; i<numPoints; i++) {
      /* Draw alive particles. */
      if (colorList[i] != DEAD) {
        if (!sprite) glColor4fv(colorSet[colorList[i]]);
        glVertex3fv(pointList[i]);
      }
    }
  glEnd();

  glDisable(GL_TEXTURE_2D);
#ifdef GL_ARB_point_sprite
  glDisable(GL_POINT_SPRITE_ARB);
#endif
  glDisable(GL_BLEND);

  glPopMatrix();

  glutSwapBuffers();
}

/* ARGSUSED2 */
static void
mouse(int button, int state, int x, int y)
{
  /* Scene can be spun around Y axis using left
     mouse button movement. */
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    moving = 1;
    begin = x;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
  }
}

/* ARGSUSED1 */
static void
mouseMotion(int x, int y)
{
  if (moving) {
    angle = angle + (x - begin);
    begin = x;
    glutPostRedisplay();
  }
}

static void
menu(int option)
{
  switch (option) {
  case 0:
    makePointList();
    break;
#ifdef GL_ARB_point_parameters
  case 1:
    glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, constant);
    break;
  case 2:
    glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, linear);
    break;
  case 3:
    glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, theQuad);
    break;
#endif
  case 4:
    blend = 1;
    break;
  case 5:
    blend = 0;
    break;
#ifdef GL_ARB_point_parameters
  case 6:
    glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0);
    break;
  case 7:
    glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 10.0);
    break;
#endif
  case 8:
    glEnable(GL_POINT_SMOOTH);
    smooth = 1;
    break;
  case 9:
    glDisable(GL_POINT_SMOOTH);
    smooth = 0;
    break;
  case 10:
    glPointSize(16.0);
    break;
  case 11:
    glPointSize(32.0);
    break;
  case 12:
    glPointSize(64.0);
    break;
  case 13:
    spin = 1 - spin;
    if (animate && (spin || motion)) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case 14:
    numPoints = 200;
    break;
  case 15:
    numPoints = 500;
    break;
  case 16:
    numPoints = 1000;
    break;
  case 17:
    numPoints = 2000;
    break;
  case 666:
    exit(0);
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
static void
key(unsigned char c, int x, int y)
{
  switch (c) {
  case 13:
    animate = 1 - animate;  /* toggle. */
    if (animate && (motion || spin)) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case ' ':
    animate = 1;
    makePointList();
    glutIdleFunc(idle);
    break;
  case 'o':
  case 'O':
    org ^= 1;
#ifdef GL_VERSION_2_0
#ifdef GL_ARB_point_parameters
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN,
                      org ? GL_LOWER_LEFT : GL_UPPER_LEFT);
#endif
#endif
    glutPostRedisplay();
    break;
  case 't':
  case 'T':
    sprite ^= 1;
    glutPostRedisplay();
    break;
  case 's':
  case 'S':
    (smooth ^= 1) ? glEnable(GL_POINT_SMOOTH) : glDisable(GL_POINT_SMOOTH);
    glutPostRedisplay();
    break;
  case '0':
    glPointSize(1.0);
    glutPostRedisplay();
    break;
  case '1':
    glPointSize(16.0);
    glutPostRedisplay();
    break;
  case '2':
    glPointSize(32.0);
    glutPostRedisplay();
    break;
  case '3':
    glPointSize(64.0);
    glutPostRedisplay();
    break;
  case '4':
    glPointSize(128.0);
    glutPostRedisplay();
    break;
  case 27:
    exit(0);
  }
}



static void
makeSprite(void)
{
   GLubyte texture[16][16][4];
   int i, j;

   if (!glutExtensionSupported("GL_ARB_point_sprite")) {
      printf("Sorry, this demo requires GL_ARB_point_sprite.\n");
      exit(0);
   }
   if (!glutExtensionSupported("GL_ARB_point_parameters")) {
      printf("Sorry, this demo requires GL_ARB_point_parameters.\n");
      exit(0);
   }

   for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
         if (spritePattern[i][j]) {
            texture[i][j][0] = 255;
            texture[i][j][1] = 255;
            texture[i][j][2] = 255;
            texture[i][j][3] = 255;
         }
         else {
            texture[i][j][0] = 255;
            texture[i][j][1] = 0;
            texture[i][j][2] = 0;
            texture[i][j][3] = 0;
         }
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_ARB_point_sprite
   glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
#endif
}


static void
reshape(int width, int height)
{
  GLfloat h = (GLfloat) height / (GLfloat) width;

  glViewport(0, 0, (GLint) width, (GLint) height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -h, h, 2.0, 30.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -10.0);
}

int
main(int argc, char **argv)
{
  int i;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

  for (i=1; i<argc; i++) {
    if(!strcmp("-noms", argv[i])) {
      glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
      printf("forcing no multisampling\n");
    } else if(!strcmp("-nomipmaps", argv[i])) {
      useMipmaps = 0;
    } else if(!strcmp("-nearest", argv[i])) {
      linearFiltering = 0;
    }
  }
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(600,300);
  glutCreateWindow("sprite blast");
  glutReshapeFunc(reshape);
  glutDisplayFunc(redraw);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);
  glutCreateMenu(menu);
  glutAddMenuEntry("Reset time", 0);
  glutAddMenuEntry("Constant", 1);
  glutAddMenuEntry("Linear", 2);
  glutAddMenuEntry("Quadratic", 3);
  glutAddMenuEntry("Blend on", 4);
  glutAddMenuEntry("Blend off", 5);
  glutAddMenuEntry("Threshold 1", 6);
  glutAddMenuEntry("Threshold 10", 7);
  glutAddMenuEntry("Point smooth on", 8);
  glutAddMenuEntry("Point smooth off", 9);
  glutAddMenuEntry("Point size 16", 10);
  glutAddMenuEntry("Point size 32", 11);
  glutAddMenuEntry("Point size 64", 12);
  glutAddMenuEntry("Toggle spin", 13);
  glutAddMenuEntry("200 points ", 14);
  glutAddMenuEntry("500 points ", 15);
  glutAddMenuEntry("1000 points ", 16);
  glutAddMenuEntry("2000 points ", 17);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  makePointList();
  makeSprite();

  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(32.0);
#ifdef GL_ARB_point_parameters
  glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, theQuad);
#endif

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
