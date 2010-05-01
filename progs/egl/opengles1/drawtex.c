/*
 * Copyright (C) 2008  Tunsgten Graphics,Inc.   All Rights Reserved.
 */

/*
 * Test GL_OES_draw_texture
 * Brian Paul
 * August 2008
 */

#define GL_GLEXT_PROTOTYPES

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include "eglut.h"


static GLfloat view_posx = 10.0, view_posy = 20.0;
static GLfloat width = 200, height = 200;
static GLboolean animate = GL_FALSE;
static int win;


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glDrawTexfOES(view_posx, view_posy, 0.0, width, height);
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

#ifdef GL_VERSION_ES_CM_1_0
   glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
#else
   glFrustum(-ar, ar, -1, 1, 5.0, 60.0);
#endif
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static float
dist(GLuint i, GLuint j, float x, float y)
{
   return sqrt((i-x) * (i-x) + (j-y) * (j-y));
}

static void
make_smile_texture(void)
{
#define SZ 128
   GLenum Filter = GL_LINEAR;
   GLubyte image[SZ][SZ][4];
   GLuint i, j;
   GLint cropRect[4];

   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
         GLfloat d_mouth = dist(i, j, SZ/2, SZ/2);
         GLfloat d_rt_eye = dist(i, j, SZ*3/4, SZ*3/4);
         GLfloat d_lt_eye = dist(i, j, SZ*3/4, SZ*1/4);
         if (d_rt_eye < SZ / 8 || d_lt_eye < SZ / 8) {
            image[i][j][0] = 20;
            image[i][j][1] = 50;
            image[i][j][2] = 255;
            image[i][j][3] = 255;
         }
         else if (i < SZ/2 && d_mouth < SZ/3) {
            image[i][j][0] = 255;
            image[i][j][1] = 20;
            image[i][j][2] = 20;
            image[i][j][3] = 255;
         }
         else {
            image[i][j][0] = 200;
            image[i][j][1] = 200;
            image[i][j][2] = 200;
            image[i][j][3] = 255;
         }
      }
   }

   glActiveTexture(GL_TEXTURE0); /* unit 0 */
   glBindTexture(GL_TEXTURE_2D, 42);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   cropRect[0] = 0;
   cropRect[1] = 0;
   cropRect[2] = SZ;
   cropRect[3] = SZ;
   glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, cropRect);
#undef SZ
}



static void
init(void)
{
   const char *ext = (char *) glGetString(GL_EXTENSIONS);

   if (!strstr(ext, "GL_OES_draw_texture")) {
      fprintf(stderr, "Sorry, this program requires GL_OES_draw_texture");
      exit(1);
   }

   glClearColor(0.4, 0.4, 0.4, 0.0);

   make_smile_texture();
   glEnable(GL_TEXTURE_2D);
}


static void
idle(void)
{
   if (animate) {
      view_posx += 1.0;
      view_posy += 2.0;
      eglutPostRedisplay();
   }
}

static void
key(unsigned char key)
{
   switch (key) {
   case ' ':
      animate = !animate;
      break;
   case 'w':
      width -= 1.0f;
      break;
   case 'W':
      width += 1.0f;
      break;
   case 'h':
      height -= 1.0f;
      break;
   case 'H':
      height += 1.0f;
      break;
   case 27:
      eglutDestroyWindow(win);
      exit(0);
      break;
   default:
      break;
   }
}

static void
special_key(int key)
{
   switch (key) {
   case EGLUT_KEY_LEFT:
      view_posx -= 1.0;
      break;
   case EGLUT_KEY_RIGHT:
      view_posx += 1.0;
      break;
   case EGLUT_KEY_UP:
      view_posy += 1.0;
      break;
   case EGLUT_KEY_DOWN:
      view_posy -= 1.0;
      break;
   default:
      break;
   }
}

int
main(int argc, char *argv[])
{
   eglutInitWindowSize(400, 300);
   eglutInitAPIMask(EGLUT_OPENGL_ES1_BIT);
   eglutInit(argc, argv);

   win = eglutCreateWindow("drawtex");

   eglutIdleFunc(idle);
   eglutReshapeFunc(reshape);
   eglutDisplayFunc(draw);
   eglutKeyboardFunc(key);
   eglutSpecialFunc(special_key);

   init();

   eglutMainLoop();

   return 0;
}
