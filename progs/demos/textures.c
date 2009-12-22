/*
 * Simple test of multiple textures
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "readtex.h"

#define TEST_CLAMP 0
#define TEST_MIPMAPS 0

#define MAX_TEXTURES 8


static int Win;
static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLboolean Anim = GL_TRUE;
static GLboolean Blend = GL_FALSE;
static GLuint Filter = 0;
static GLboolean Clamp = GL_FALSE;

static GLuint NumTextures;
static GLuint Textures[MAX_TEXTURES];
static float TexRot[MAX_TEXTURES][3];
static float TexPos[MAX_TEXTURES][3];
static float TexAspect[MAX_TEXTURES];

static const char *DefaultFiles[] = {
   "../images/arch.rgb",
   "../images/reflect.rgb",
   "../images/tree2.rgba",
   "../images/tile.rgb"
};


#define NUM_FILTERS 5
static
struct filter {
   GLenum min, mag;
   const char *name;
} FilterModes[NUM_FILTERS] = {
   { GL_NEAREST, GL_NEAREST, "Nearest,Nearest" },
   { GL_LINEAR, GL_LINEAR, "Linear,Linear" },
   { GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, "NearestMipmapNearest,Nearest" },
   { GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, "LinearMipmapNearest,Linear" },
   { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, "LinearMipmapLinear,Linear" }
};




static void
Idle(void)
{
   Xrot = glutGet(GLUT_ELAPSED_TIME) * 0.02;
   Yrot = glutGet(GLUT_ELAPSED_TIME) * 0.04;
   /* Zrot += 2.0; */
   glutPostRedisplay();
}


static void
DrawTextures(void)
{
   GLuint i;

   for (i = 0; i < NumTextures; i++) {
      GLfloat ar = TexAspect[i];

      glPushMatrix();
      glTranslatef(TexPos[i][0], TexPos[i][1], TexPos[i][2]);
      glRotatef(TexRot[i][0], 1, 0, 0);
      glRotatef(TexRot[i][1], 0, 1, 0);
      glRotatef(TexRot[i][2], 0, 0, 1);

      glBindTexture(GL_TEXTURE_2D, Textures[i]);
      glBegin(GL_POLYGON);
#if TEST_CLAMP
      glTexCoord2f( -0.5, -0.5 );   glVertex2f( -ar, -1.0 );
      glTexCoord2f(  1.5, -0.5 );   glVertex2f(  ar, -1.0 );
      glTexCoord2f(  1.5,  1.5 );   glVertex2f(  ar,  1.0 );
      glTexCoord2f( -0.5,  1.5 );   glVertex2f( -ar,  1.0 );
#else
      glTexCoord2f( 0.0, 0.0 );   glVertex2f( -ar, -1.0 );
      glTexCoord2f( 1.0, 0.0 );   glVertex2f(  ar, -1.0 );
      glTexCoord2f( 1.0, 1.0 );   glVertex2f(  ar,  1.0 );
      glTexCoord2f( 0.0, 1.0 );   glVertex2f( -ar,  1.0 );
#endif
      glEnd();

      glPopMatrix();
   }
}

static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (Blend) {
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
   }
   else {
      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
   }

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   DrawTextures();

   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 50.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -10.0);
}


static GLfloat
RandFloat(float min, float max)
{
   float x = (float) (rand() % 1000) * 0.001;
   x = x * (max - min) + min;
   return x;
}


static void
Randomize(void)
{
   GLfloat k = 1.0;
   GLuint i;

   srand(glutGet(GLUT_ELAPSED_TIME));

   for (i = 0; i < NumTextures; i++) {
      TexRot[i][0] = RandFloat(0.0, 360);
      TexRot[i][1] = RandFloat(0.0, 360);
      TexRot[i][2] = RandFloat(0.0, 360);
      TexPos[i][0] = RandFloat(-k, k);
      TexPos[i][1] = RandFloat(-k, k);
      TexPos[i][2] = RandFloat(-k, k);
   }
}


static void
SetTexParams(void)
{
   GLuint i;
   for (i = 0; i < NumTextures; i++) {
      glBindTexture(GL_TEXTURE_2D, Textures[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      FilterModes[Filter].min);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                      FilterModes[Filter].mag);

      if (Clamp) {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
      else {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      }
   }
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
   case 'a':
   case ' ':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'b':
      Blend = !Blend;
      break;
   case 'f':
      Filter = (Filter + 1) % NUM_FILTERS;
      SetTexParams();
      break;      
   case 'r':
      Randomize();
      break;
#if TEST_CLAMP
   case 'c':
      Clamp = !Clamp;
      SetTexParams();
      break;
#endif
   case 'z':
      Zrot -= step;
      break;
   case 'Z':
      Zrot += step;
      break;
   case 27:
      glutDestroyWindow(Win);
      exit(0);
      break;
   }

   printf("Blend=%s Filter=%s\n",
          Blend ? "Y" : "n",
          FilterModes[Filter].name);

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
LoadTextures(GLuint n, const char *files[])
{
   GLuint i;

   NumTextures = n < MAX_TEXTURES ? n : MAX_TEXTURES;

   glGenTextures(n, Textures);

   SetTexParams();

   for (i = 0; i < n; i++) {
      GLint w, h;
      glBindTexture(GL_TEXTURE_2D, Textures[i]);
#if TEST_MIPMAPS
      {
         static const GLubyte color[9][4] = {
            {255, 0, 0},
            {0, 255, 0},
            {0, 0, 255},
            {0, 255, 255},
            {255, 0, 255},
            {255, 255, 0},
            {255, 128, 255},
            {128, 128, 128},
            {64, 64, 64}
         };

         GLubyte image[256*256*4];
         int i, level;
         w = h = 256;
         for (level = 0; level <= 8; level++) {
            for (i = 0; i < w * h; i++) {
               image[i*4+0] = color[level][0];
               image[i*4+1] = color[level][1];
               image[i*4+2] = color[level][2];
               image[i*4+3] = color[level][3];
            }
            printf("Load level %d: %d x %d\n", level, w>>level, h>>level);
            glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w>>level, h>>level, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, image);
         }
      }
#else
      if (!LoadRGBMipmaps2(files[i], GL_TEXTURE_2D, GL_RGB, &w, &h)) {
         printf("Error: couldn't load %s\n", files[i]);
         exit(1);
      }
#endif
      TexAspect[i] = (float) w / (float) h;
      printf("Loaded %s\n", files[i]);
   }
}


static void
Init(int argc, const char *argv[])
{
   if (argc == 1)
      LoadTextures(4, DefaultFiles);
   else
      LoadTextures(argc - 1, argv + 1);

   Randomize();

   glEnable(GL_TEXTURE_2D);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glColor4f(1, 1, 1, 0.5);

#if 0
   /* setup lighting, etc */
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
#endif
}


static void
Usage(void)
{
   printf("Usage:\n");
   printf("  textures [file.rgb] ...\n");
   printf("Keys:\n");
   printf("  a - toggle animation\n");
   printf("  b - toggle blending\n");
   printf("  f - change texture filter mode\n");
   printf("  r - randomize\n");
   printf("  ESC - exit\n");
}


int
main(int argc, char *argv[])
{
   glutInitWindowSize(700, 700);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init(argc, (const char **) argv);
   Usage();
   glutMainLoop();
   return 0;
}
