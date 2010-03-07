/*
 * Use glCopyTexSubImage2D to draw animated gears on the sides of a box.
 *
 * Brian Paul
 * 27 January 2006
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

static GLint WinWidth = 800, WinHeight = 500;
static GLint TexWidth, TexHeight;
static GLuint TexObj = 1;
static GLenum IntFormat = GL_RGB;

static GLboolean WireFrame = GL_FALSE;

static GLint T0 = 0;
static GLint Frames = 0;
static GLint Win = 0;

static GLfloat ViewRotX = 20.0, ViewRotY = 30.0, ViewRotZ = 0.0;
static GLint Gear1, Gear2, Gear3;
static GLfloat GearRot = 0.0;
static GLfloat CubeRot = 0.0;


/**
  Draw a gear wheel.  You'll probably want to call this function when
  building a display list since we do a lot of trig here.
 
  Input:  inner_radius - radius of hole at center
          outer_radius - radius at center of teeth
          width - width of gear
          teeth - number of teeth
          tooth_depth - depth of tooth
 **/
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
  GLint i;
  GLfloat r0, r1, r2;
  GLfloat angle, da;
  GLfloat u, v, len;

  r0 = inner_radius;
  r1 = outer_radius - tooth_depth / 2.0;
  r2 = outer_radius + tooth_depth / 2.0;

  da = 2.0 * M_PI / teeth / 4.0;

  glShadeModel(GL_FLAT);

  glNormal3f(0.0, 0.0, 1.0);

  /* draw front face */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;
    glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
    glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
    if (i < teeth) {
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
    }
  }
  glEnd();

  /* draw front sides of teeth */
  glBegin(GL_QUADS);
  da = 2.0 * M_PI / teeth / 4.0;
  for (i = 0; i < teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;

    glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
  }
  glEnd();

  glNormal3f(0.0, 0.0, -1.0);

  /* draw back face */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;
    glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
    if (i < teeth) {
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
    }
  }
  glEnd();

  /* draw back sides of teeth */
  glBegin(GL_QUADS);
  da = 2.0 * M_PI / teeth / 4.0;
  for (i = 0; i < teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;

    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
    glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
  }
  glEnd();

  /* draw outward faces of teeth */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i < teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;

    glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
    glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    u = r2 * cos(angle + da) - r1 * cos(angle);
    v = r2 * sin(angle + da) - r1 * sin(angle);
    len = sqrt(u * u + v * v);
    u /= len;
    v /= len;
    glNormal3f(v, -u, 0.0);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
    glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
    glNormal3f(cos(angle), sin(angle), 0.0);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
    glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
    u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
    v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
    glNormal3f(v, -u, 0.0);
    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
    glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
    glNormal3f(cos(angle), sin(angle), 0.0);
  }

  glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
  glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

  glEnd();

  glShadeModel(GL_SMOOTH);

  /* draw inside radius cylinder */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= teeth; i++) {
    angle = i * 2.0 * M_PI / teeth;
    glNormal3f(-cos(angle), -sin(angle), 0.0);
    glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
    glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
  }
  glEnd();

}

static void
cleanup(void)
{
   glDeleteTextures(1, &TexObj);
   glDeleteLists(Gear1, 1);
   glDeleteLists(Gear2, 1);
   glDeleteLists(Gear3, 1);
   glutDestroyWindow(Win);
}


static void
DrawGears(void)
{
   if (WireFrame) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }

   glPushMatrix();
      glRotatef(20/*ViewRotX*/, 1.0, 0.0, 0.0);
      glRotatef(ViewRotY, 0.0, 1.0, 0.0);
      glRotatef(ViewRotZ, 0.0, 0.0, 1.0);

      glPushMatrix();
         glTranslatef(-3.0, -2.0, 0.0);
         glRotatef(GearRot, 0.0, 0.0, 1.0);
         glCallList(Gear1);
      glPopMatrix();

      glPushMatrix();
         glTranslatef(3.1, -2.0, 0.0);
         glRotatef(-2.0 * GearRot - 9.0, 0.0, 0.0, 1.0);
         glCallList(Gear2);
      glPopMatrix();

      glPushMatrix();
         glTranslatef(-3.1, 4.2, 0.0);
         glRotatef(-2.0 * GearRot - 25.0, 0.0, 0.0, 1.0);
         glCallList(Gear3);
      glPopMatrix();

  glPopMatrix();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);

   /* clear whole depth buffer */
   glDisable(GL_SCISSOR_TEST);
   glClear(GL_DEPTH_BUFFER_BIT);
   glEnable(GL_SCISSOR_TEST);

   /* clear upper-left corner of color buffer (unused space) */
   glScissor(0, TexHeight, TexWidth, WinHeight - TexHeight);
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* clear lower-left corner of color buffer */
   glViewport(0, 0, TexWidth, TexHeight);
   glScissor(0, 0, TexWidth, TexHeight);
   glClearColor(1, 1, 1, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* draw gears in lower-left corner */
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   DrawGears();

   /* copy color buffer to texture */
   glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, TexWidth, TexHeight);
   
   /* clear right half of color buffer */
   glViewport(TexWidth, 0, WinWidth - TexWidth, WinHeight);
   glScissor(TexWidth, 0, WinWidth - TexWidth, WinHeight);
   glClearColor(0.5, 0.5, 0.8, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* draw textured cube in right half of window */
   ar = (float) (WinWidth - TexWidth) / WinHeight;
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   DrawCube();

   /* finish up */
   glutSwapBuffers();

   Frames++;
   {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      if (t - T0 >= 5000) {
         GLfloat seconds = (t - T0) / 1000.0;
         GLfloat fps = Frames / seconds;
         printf("%d frames in %6.3f seconds = %6.3f FPS\n", Frames, seconds, fps);
         fflush(stdout);
         T0 = t;
         Frames = 0;
      }
   }
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

  /* fmod to prevent overflow */
  GearRot = fmod(GearRot + 70.0 * dt, 360.0);  /* 70 deg/sec */
  CubeRot = fmod(CubeRot + 15.0 * dt, 360.0);  /* 15 deg/sec */

  glutPostRedisplay();
}


/* change view angle, exit upon ESC */
static void
key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
   case 'w':
      WireFrame = !WireFrame;
      break;
   case 'z':
      ViewRotZ += 5.0;
      break;
   case 'Z':
      ViewRotZ -= 5.0;
      break;
   case 27:  /* Escape */
      cleanup();
      exit(0);
      break;
   default:
      return;
   }
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
init(int argc, char *argv[])
{
  static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};
  static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
  static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
  static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
  GLint i;

  glLightfv(GL_LIGHT0, GL_POSITION, pos);
#if 0
  glEnable(GL_CULL_FACE);
#endif
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);

  /* make the gears */
  Gear1 = glGenLists(1);
  glNewList(Gear1, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
  gear(1.0, 4.0, 1.0, 20, 0.7);
  glEndList();

  Gear2 = glGenLists(1);
  glNewList(Gear2, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
  gear(0.5, 2.0, 2.0, 10, 0.7);
  glEndList();

  Gear3 = glGenLists(1);
  glNewList(Gear3, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
  gear(1.3, 2.0, 0.5, 10, 0.7);
  glEndList();

  glEnable(GL_NORMALIZE);

  /* xxx make size dynamic */
  TexWidth = 256;
  TexHeight = 256;

   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexImage2D(GL_TEXTURE_2D, 0, IntFormat, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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
   glutInitWindowSize(WinWidth, WinHeight);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

   Win = glutCreateWindow("gearbox");
   init(argc, argv);

   glutDisplayFunc(draw);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(key);
   glutSpecialFunc(special);
   glutVisibilityFunc(visible);

   glutMainLoop();
   return 0;             /* ANSI C requires main to return int. */
}
