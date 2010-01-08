/**
 * AA lines with texture mapped quads
 *
 * Brian Paul
 * 9 Feb 2008
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.1415926535
#endif

static GLint WinWidth = 300, WinHeight = 300;
static GLint win = 0;
static GLfloat Width = 8.;

/*
 * Quad strip for line from v0 to v1:
 *
 1   3                     5   7
 +---+---------------------+---+
 |                             |
 |   *v0                 v1*   |
 |                             |
 +---+---------------------+---+
 0   2                     4   6
 */
static void
QuadLine(const GLfloat *v0, const GLfloat *v1, GLfloat width)
{
   GLfloat dx = v1[0] - v0[0];
   GLfloat dy = v1[1] - v0[1];
   GLfloat len = sqrt(dx*dx + dy*dy);
   float dx0, dx1, dx2, dx3, dx4, dx5, dx6, dx7;
   float dy0, dy1, dy2, dy3, dy4, dy5, dy6, dy7;

   dx /= len;
   dy /= len;

   width *= 0.5;  /* half width */
   dx = dx * (width + 0.0);
   dy = dy * (width + 0.0);

   dx0 = -dx+dy;  dy0 = -dy-dx;
   dx1 = -dx-dy;  dy1 = -dy+dx;

   dx2 =   0+dy;  dy2 = -dx+0;
   dx3 =   0-dy;  dy3 = +dx+0;

   dx4 =   0+dy;  dy4 = -dx+0;
   dx5 =   0-dy;  dy5 = +dx+0;

   dx6 =  dx+dy;  dy6 =  dy-dx;
   dx7 =  dx-dy;  dy7 =  dy+dx;

   /*
   printf("dx, dy = %g, %g\n", dx, dy);
   printf("  dx0, dy0: %g, %g\n", dx0, dy0);
   printf("  dx1, dy1: %g, %g\n", dx1, dy1);
   printf("  dx2, dy2: %g, %g\n", dx2, dy2);
   printf("  dx3, dy3: %g, %g\n", dx3, dy3);
   */

   glBegin(GL_QUAD_STRIP);
   glTexCoord2f(0, 0);
   glVertex2f(v0[0] + dx0, v0[1] + dy0);
   glTexCoord2f(0, 1);
   glVertex2f(v0[0] + dx1, v0[1] + dy1);

   glTexCoord2f(0.5, 0);
   glVertex2f(v0[0] + dx2, v0[1] + dy2);
   glTexCoord2f(0.5, 1);
   glVertex2f(v0[0] + dx3, v0[1] + dy3);

   glTexCoord2f(0.5, 0);
   glVertex2f(v1[0] + dx2, v1[1] + dy2);
   glTexCoord2f(0.5, 1);
   glVertex2f(v1[0] + dx3, v1[1] + dy3);

   glTexCoord2f(1, 0);
   glVertex2f(v1[0] + dx6, v1[1] + dy6);
   glTexCoord2f(1, 1);
   glVertex2f(v1[0] + dx7, v1[1] + dy7);
   glEnd();
}


static float Cos(float a)
{
   return cos(a * M_PI / 180.);
}

static float Sin(float a)
{
   return sin(a * M_PI / 180.);
}

static void
Redisplay(void)
{
   float cx = 0.5 * WinWidth, cy = 0.5 * WinHeight;
   float len = 0.5 * WinWidth - 20.0;
   int i;

   glClear(GL_COLOR_BUFFER_BIT);

   glColor3f(1, 1, 1);

   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);

   for (i = 0; i < 360; i+=5) {
      float v0[2], v1[2];
      v0[0] = cx + 40 * Cos(i);
      v0[1] = cy + 40 * Sin(i);
      v1[0] = cx + len * Cos(i);
      v1[1] = cy + len * Sin(i);
      QuadLine(v0, v1, Width);
   }

   {
      float v0[2], v1[2], x;
      for (x = 0; x < 1.0; x += 0.2) {
         v0[0] = cx + x;
         v0[1] = cy + x * 40 - 20;
         v1[0] = cx + x + 5.0;
         v1[1] = cy + x * 40 - 20;
         QuadLine(v0, v1, Width);
      }
   }

   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
CleanUp(void)
{
   glutDestroyWindow(win);
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;

   switch(key) {
   case 'w':
      Width -= 0.5;
      break;
   case 'W':
      Width += 0.5;
      break;
   case 27:
      CleanUp();
      exit(0);
      break;
   }
#if 0
   if (Width < 3)
      Width = 3;
#endif
   printf("Width = %g\n", Width);
   glutPostRedisplay();
}


static float
ramp4(GLint i, GLint size)
{
   float d;
   if (i < 4 ) {
      d = i / 4.0;
   }
   else if (i >= size - 5) {
      d = 1.0 - (i - (size - 5)) / 4.0;
   }
   else {
      d = 1.0;
   }
   return d;
}

static float
ramp2(GLint i, GLint size)
{
   float d;
   if (i < 2 ) {
      d = i / 2.0;
   }
   else if (i >= size - 3) {
      d = 1.0 - (i - (size - 3)) / 2.0;
   }
   else {
      d = 1.0;
   }
   return d;
}

static float
ramp1(GLint i, GLint size)
{
   float d;
   if (i == 0 || i == size-1) {
      d = 0.0;
   }
   else {
      d = 1.0;
   }
   return d;
}


/**
 * Make an alpha texture for antialiasing lines.
 * Just a linear fall-off ramp for now.
 * Should have a number of different textures for different line widths.
 * Could try a bell-like-curve....
 */
static void
MakeTexture(void)
{
#define SZ 8
   GLfloat tex[SZ][SZ];  /* alpha tex */
   int i, j;
   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
#if 0
         float k = (SZ-1) / 2.0;
         float dx = fabs(i - k) / k;
         float dy = fabs(j - k) / k;
         float d;

         dx = 1.0 - dx;
         dy = 1.0 - dy;
         d = dx * dy;

#else
         float d = ramp1(i, SZ) * ramp1(j, SZ);
         printf("%d, %d: %g\n", i, j, d);
#endif
         tex[i][j] = d;
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, SZ, SZ, 0, GL_ALPHA, GL_FLOAT, tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#undef SZ
}


static void
MakeMipmap(void)
{
#define SZ 64
   GLfloat tex[SZ][SZ];  /* alpha tex */
   int level;

   glPixelStorei(GL_UNPACK_ROW_LENGTH, SZ);
   for (level = 0; level < 7; level++) {
      int sz = 1 << (6 - level);
      int i, j;
      for (i = 0; i < sz; i++) {
         for (j = 0; j < sz; j++) {
            if (level == 6)
               tex[i][j] = 1.0;
            else if (level == 5)
               tex[i][j] = 0.5;
            else
               tex[i][j] = ramp1(i, sz) * ramp1(j, sz);
         }
      }

      glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA,
                   sz, sz, 0, GL_ALPHA, GL_FLOAT, tex);
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   /* glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4); */
   /* glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5); */

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#undef SZ
}


static void
Init(void)
{
   const char *version;

   (void) MakeTexture;
   (void) ramp4;
   (void) ramp2;

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] != '2' || version[1] != '.') {
      printf("This program requires OpenGL 2.x, found %s\n", version);
      exit(1);
   }

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#elif 0
   MakeTexture();
#else
   MakeMipmap();
#endif
}


static void
ParseOptions(int argc, char *argv[])
{
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}
