
/*
 * Specular reflection demo.  The specular highlight is modulated by
 * a sphere-mapped texture.  The result is a high-gloss surface.
 * NOTE: you really need hardware acceleration for this.
 * Also note, this technique can't be implemented with multi-texture
 * and separate specular color interpolation because there's no way
 * to indicate that the second texture unit (the reflection map)
 * should modulate the specular color and not the base color.
 * A future multi-texture extension could fix that.
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 *
 * Brian Paul  October 22, 1999  This program is in the public domain.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.h"
#include "trackball.h"


#define SPECULAR_TEXTURE_FILE "../images/reflect.rgb"
#define BASE_TEXTURE_FILE "../images/tile.rgb"

/* Menu items */
#define DO_SPEC_TEXTURE 1
#define OBJECT 2
#define ANIMATE 3
#define QUIT 100

/* for convolution */
#define FILTER_SIZE 7

static GLint Win;
static GLint WinWidth = 500, WinHeight = 500;
static GLuint CylinderObj = 0;
static GLuint TeapotObj = 0;
static GLuint Object = 0;
static GLboolean Animate = GL_TRUE;

static float CurQuat[4] = { 0, 0, 0, 1 };

static GLfloat Black[4] = { 0, 0, 0, 0 };
static GLfloat White[4] = { 1, 1, 1, 1 };
static GLfloat Diffuse[4] = { .3, .3, 1.0, 1.0 };  /* blue */
static GLfloat Shininess = 6;

static GLuint BaseTexture, SpecularTexture;
static GLboolean DoSpecTexture = GL_TRUE;

static GLboolean ButtonDown = GL_FALSE;
static GLint ButtonX, ButtonY;


/* performance info */
static GLint T0 = 0;
static GLint Frames = 0;


static void Idle( void )
{
   static const float yAxis[3] = {0, 1, 0};
   static double t0 = -1.;
   float quat[4];
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;

   axis_to_quat(yAxis, 2.0 * dt, quat);
   add_quats(quat, CurQuat, CurQuat);

   glutPostRedisplay();
}


static void Display( void )
{
   GLfloat rot[4][4];

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   build_rotmatrix(rot, CurQuat);
   glMultMatrixf(&rot[0][0]);

   /* First pass: diffuse lighting with base texture */
   glMaterialfv(GL_FRONT, GL_DIFFUSE, Diffuse);
   glMaterialfv(GL_FRONT, GL_SPECULAR, Black);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, BaseTexture);
   glCallList(Object);

   /* Second pass: specular lighting with reflection texture */
   glEnable(GL_POLYGON_OFFSET_FILL);
   glBlendFunc(GL_ONE, GL_ONE);  /* add */
   glEnable(GL_BLEND);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, Black);
   glMaterialfv(GL_FRONT, GL_SPECULAR, White);
   if (DoSpecTexture) {
      glBindTexture(GL_TEXTURE_2D, SpecularTexture);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }
   glCallList(Object);
   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);
   glDisable(GL_BLEND);
   glDisable(GL_POLYGON_OFFSET_FILL);

   glPopMatrix();

   glutSwapBuffers();

   if (Animate) {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      Frames++;
      if (t - T0 >= 5000) {
         GLfloat seconds = (t - T0) / 1000.0;
         GLfloat fps = Frames / seconds;
         printf("%d frames in %g seconds = %g FPS\n", Frames, seconds, fps);
         fflush(stdout);
         T0 = t;
         Frames = 0;
      }
   }
}


static void Reshape( int width, int height )
{
   GLfloat h = 30.0;
   GLfloat w = h * width / height;
   WinWidth = width;
   WinHeight = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -w, w, -h, h, 150.0, 500.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -380.0 );
}


static void ToggleAnimate(void)
{
   Animate = !Animate;
   if (Animate) {
      glutIdleFunc( Idle );
      T0 = glutGet(GLUT_ELAPSED_TIME);
      Frames = 0;
   }
   else {
      glutIdleFunc( NULL );
   }
}


static void ModeMenu(int entry)
{
   if (entry==ANIMATE) {
      ToggleAnimate();
   }
   else if (entry==DO_SPEC_TEXTURE) {
      DoSpecTexture = !DoSpecTexture;
   }
   else if (entry==OBJECT) {
      if (Object == TeapotObj)
         Object = CylinderObj;
      else
         Object = TeapotObj;
   }
   else if (entry==QUIT) {
      exit(0);
   }
   glutPostRedisplay();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 's':
         Shininess--;
         if (Shininess < 0.0)
            Shininess = 0.0;
         glMaterialf(GL_FRONT, GL_SHININESS, Shininess);
         printf("Shininess = %g\n", Shininess);
         break;
      case 'S':
         Shininess++;
         if (Shininess > 128.0)
            Shininess = 128.0;
         glMaterialf(GL_FRONT, GL_SHININESS, Shininess);
         printf("Shininess = %g\n", Shininess);
         break;
      case 'a':
      case ' ':
         ToggleAnimate();
         break;
      case 'n':
         Idle();
         break;
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
MouseMotion(int x, int y)
{
   if (ButtonDown) {
      float x0 = (2.0 * ButtonX - WinWidth) / WinWidth;
      float y0 = (WinHeight - 2.0 * ButtonY) / WinHeight;
      float x1 = (2.0 * x - WinWidth) / WinWidth;
      float y1 = (WinHeight - 2.0 * y) / WinHeight;
      float q[4];

      trackball(q, x0, y0, x1, y1);
      ButtonX = x;
      ButtonY = y;
      add_quats(q, CurQuat, CurQuat);

      glutPostRedisplay();
   }
}


static void
MouseButton(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
     ButtonDown = GL_TRUE;
     ButtonX = x;
     ButtonY = y;
  }
  else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
     ButtonDown = GL_FALSE;
  }
}


static void Init( int argc, char *argv[] )
{
   GLboolean convolve = GL_FALSE;
   GLboolean fullscreen = GL_FALSE;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-info")==0) {
         printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
         printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
         printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
         printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
      }
      else if (strcmp(argv[i], "-c")==0) {
         convolve = GL_TRUE;
      }
      else if (strcmp(argv[i], "-f")==0) {
         fullscreen = GL_TRUE;
      }
   }

   if (convolve && !glutExtensionSupported("GL_ARB_imaging")) {
      fprintf(stderr,
              "GL_ARB_imaging is not supported, disabling convolution.\n");
      exit(1);
   }


   if (fullscreen)
      glutFullScreen();

   /* Cylinder object */
   {
      static GLfloat height = 100.0;
      static GLfloat radius = 40.0;
      static GLint slices = 24;  /* pie slices around Z axis */
      static GLint stacks = 10;  /* subdivisions along length of cylinder */
      static GLint rings = 4;    /* rings in the end disks */
      GLUquadricObj *q = gluNewQuadric();
      assert(q);
      gluQuadricTexture(q, GL_TRUE);

      CylinderObj = glGenLists(1);
      glNewList(CylinderObj, GL_COMPILE);

      glPushMatrix();
      glTranslatef(0.0, 0.0, -0.5 * height);

      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      /*glScalef(8.0, 4.0, 2.0);*/
      glMatrixMode(GL_MODELVIEW);

      /* cylinder */
      gluQuadricNormals(q, GL_SMOOTH);
      gluQuadricTexture(q, GL_TRUE);
      gluCylinder(q, radius, radius, height, slices, stacks);

      /* end cap */
      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glScalef(3.0, 3.0, 1.0);
      glMatrixMode(GL_MODELVIEW);

      glTranslatef(0.0, 0.0, height);
      gluDisk(q, 0.0, radius, slices, rings);

      /* other end cap */
      glTranslatef(0.0, 0.0, -height);
      gluQuadricOrientation(q, GLU_INSIDE);
      gluDisk(q, 0.0, radius, slices, rings);

      glPopMatrix();

      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);

      glEndList();
      gluDeleteQuadric(q);
   }

   /* Teapot */
   {
      TeapotObj = glGenLists(1);
      glNewList(TeapotObj, GL_COMPILE);

      glFrontFace(GL_CW);
      glutSolidTeapot(40.0);
      glFrontFace(GL_CCW);

      glEndList();
   }

   /* show cylinder by default */
   Object = CylinderObj;


   /* lighting */
   glEnable(GL_LIGHTING);
   {
      GLfloat pos[4] = { 3, 3, 3, 1 };
      glLightfv(GL_LIGHT0, GL_AMBIENT, Black);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, White);
      glLightfv(GL_LIGHT0, GL_SPECULAR, White);
      glLightfv(GL_LIGHT0, GL_POSITION, pos);
      glEnable(GL_LIGHT0);
      glMaterialfv(GL_FRONT, GL_AMBIENT, Black);
      glMaterialf(GL_FRONT, GL_SHININESS, Shininess);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
   }

   /* Base texture */
   glGenTextures(1, &BaseTexture);
   glBindTexture(GL_TEXTURE_2D, BaseTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   if (!LoadRGBMipmaps(BASE_TEXTURE_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image file %s\n", BASE_TEXTURE_FILE);
      exit(1);
   }

   /* Specular texture */
   glGenTextures(1, &SpecularTexture);
   glBindTexture(GL_TEXTURE_2D, SpecularTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   if (convolve) {
      /* use convolution to blur the texture to simulate a dull finish
       * on the object.
       */
      GLubyte *img;
      GLenum format;
      GLint w, h;
      GLfloat filter[FILTER_SIZE][FILTER_SIZE][4];

      for (h = 0; h < FILTER_SIZE; h++) {
         for (w = 0; w < FILTER_SIZE; w++) {
            const GLfloat k = 1.0 / (FILTER_SIZE * FILTER_SIZE);
            filter[h][w][0] = k;
            filter[h][w][1] = k;
            filter[h][w][2] = k;
            filter[h][w][3] = k;
         }
      }

      glEnable(GL_CONVOLUTION_2D);
      glConvolutionParameteri(GL_CONVOLUTION_2D,
                              GL_CONVOLUTION_BORDER_MODE, GL_CONSTANT_BORDER);
      glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGBA,
                            FILTER_SIZE, FILTER_SIZE,
                            GL_RGBA, GL_FLOAT, filter);

      img = LoadRGBImage(SPECULAR_TEXTURE_FILE, &w, &h, &format);
      if (!img) {
         printf("Error: couldn't load texture image file %s\n",
                SPECULAR_TEXTURE_FILE);
         exit(1);
      }

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                   format, GL_UNSIGNED_BYTE, img);
      free(img);
   }
   else {
      /* regular path */
      if (!LoadRGBMipmaps(SPECULAR_TEXTURE_FILE, GL_RGB)) {
         printf("Error: couldn't load texture image file %s\n",
                SPECULAR_TEXTURE_FILE);
         exit(1);
      }
   }

   /* misc */
   glEnable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_NORMALIZE);

   glPolygonOffset( -1, -1 );
}


int main( int argc, char *argv[] )
{
   glutInitWindowSize(WinWidth, WinHeight);
   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Win = glutCreateWindow(argv[0] );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutMotionFunc(MouseMotion);
   glutMouseFunc(MouseButton);
   if (Animate)
      glutIdleFunc( Idle );

   glutCreateMenu(ModeMenu);
   glutAddMenuEntry("Toggle Highlight", DO_SPEC_TEXTURE);
   glutAddMenuEntry("Toggle Object", OBJECT);
   glutAddMenuEntry("Toggle Animate", ANIMATE);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   Init(argc, argv);

   glutMainLoop();
   return 0;
}
