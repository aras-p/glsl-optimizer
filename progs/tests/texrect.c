
/* GL_NV_texture_rectangle test
 *
 * Brian Paul
 * 14 June 2002
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "readtex.h"

#define TEXTURE_0_FILE "../images/girl.rgb"
#define TEXTURE_1_FILE "../images/reflect.rgb"

#define TEX0 1
#define TEX7 8
#define ANIMATE 10
#define CLAMP 20
#define CLAMP_TO_EDGE 21
#define CLAMP_TO_BORDER 22
#define LINEAR_FILTER 30
#define NEAREST_FILTER 31
#define QUIT 100

static GLboolean Animate = GL_FALSE;
static GLint NumUnits = 2;
static GLboolean TexEnabled[8];
static GLint Width[8], Height[8];  /* image sizes */
static GLenum Format[8];

static GLfloat Xrot = 00.0, Yrot = 00.0, Zrot = 0.0;


static void Idle( void )
{
   Zrot = glutGet(GLUT_ELAPSED_TIME) * 0.01;
   glutPostRedisplay();
}


static void DrawObject(void)
{
   GLint i;
   GLfloat d = 10;  /* so we can see how borders are handled */

   glColor3f(.1, .1, .1);  /* modulate this */

   glPushMatrix();

      glRotatef(Zrot, 0, 0, 1);

      glBegin(GL_QUADS);

      for (i = 0; i < NumUnits; i++)
         glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, -d, -d);
      glVertex2f(-1.0, -1.0);

      for (i = 0; i < NumUnits; i++)
         glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, Width[i]+d, -d);
      glVertex2f(1.0, -1.0);

      for (i = 0; i < NumUnits; i++)
         glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, Width[i]+d, Height[i]+d);
      glVertex2f(1.0, 1.0);

      for (i = 0; i < NumUnits; i++)
         glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, -d, Height[i]+d);
      glVertex2f(-1.0, 1.0);

      glEnd();
   glPopMatrix();
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
      glScalef(5.0, 5.0, 5.0);
      DrawObject();
   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 100.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -35.0 );
}


static void ModeMenu(int entry)
{
   GLint i;
   if (entry >= TEX0 && entry < TEX0 + NumUnits) {
      /* toggle */
      i = entry - TEX0;
      TexEnabled[i] = !TexEnabled[i];
      glActiveTextureARB(GL_TEXTURE0_ARB + i);
      if (TexEnabled[i]) {
         glEnable(GL_TEXTURE_RECTANGLE_NV);
      }
      else {
         glDisable(GL_TEXTURE_RECTANGLE_NV);
      }
      printf("Enabled: ");
      for (i = 0; i < NumUnits; i++)
         printf("%d ", (int) TexEnabled[i]);
      printf("\n");
   }
   else if (entry==ANIMATE) {
      Animate = !Animate;
      if (Animate)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
   }
   else if (entry==CLAMP) {
      for (i = 0; i < NumUnits; i++) {
         glActiveTextureARB(GL_TEXTURE0_ARB + i);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }
   }
   else if (entry==CLAMP_TO_EDGE) {
      for (i = 0; i < NumUnits; i++) {
         glActiveTextureARB(GL_TEXTURE0_ARB + i);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
   }
   else if (entry==CLAMP_TO_BORDER) {
      for (i = 0; i < NumUnits; i++) {
         glActiveTextureARB(GL_TEXTURE0_ARB + i);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      }
   }
   else if (entry==NEAREST_FILTER) {
      for (i = 0; i < NumUnits; i++) {
         glActiveTextureARB(GL_TEXTURE0_ARB + i);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
   }
   else if (entry==LINEAR_FILTER) {
      for (i = 0; i < NumUnits; i++) {
         glActiveTextureARB(GL_TEXTURE0_ARB + i);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
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
      case 'z':
         Zrot -= 1.0;
         break;
      case 'Z':
         Zrot += 1.0;
         break;
      case 'a':
         Animate = !Animate;
         if (Animate)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   float step = 3.0;
   (void) x;
   (void) y;

   switch (key) {
      case GLUT_KEY_UP:
         Xrot += step;
         break;
      case GLUT_KEY_DOWN:
         Xrot -= step;
         break;
      case GLUT_KEY_LEFT:
         Yrot += step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot -= step;
         break;
   }
   glutPostRedisplay();
}


static void Init( int argc, char *argv[] )
{
   const GLenum wrap = GL_CLAMP;
   GLuint texObj[8];
   GLint size, i;

   if (!glutExtensionSupported("GL_ARB_multitexture")) {
      printf("Sorry, GL_ARB_multitexture needed by this program\n");
      exit(1);
   }

   if (!glutExtensionSupported("GL_NV_texture_rectangle")) {
      printf("Sorry, GL_NV_texture_rectangle needed by this program\n");
      exit(1);
   }

   glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &NumUnits);
   printf("%d texture units supported, using 2.\n", NumUnits);
   if (NumUnits > 2)
      NumUnits = 2;

   glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &size);
   printf("%d x %d max texture rectangle size\n", size, size);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   for (i = 0; i < NumUnits; i++) {
      TexEnabled[i] = GL_TRUE;
   }

   /* allocate two texture objects */
   glGenTextures(NumUnits, texObj);

   /* setup the texture objects */
   for (i = 0; i < NumUnits; i++) {

      glActiveTextureARB(GL_TEXTURE0_ARB + i);

      glBindTexture(GL_TEXTURE_RECTANGLE_NV, texObj[i]);
      glTexParameteri(GL_TEXTURE_RECTANGLE_NV,
                      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_NV,
                      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, wrap);
      glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, wrap);

      if (i == 0) {
         GLubyte *img = LoadRGBImage(TEXTURE_0_FILE, &Width[0], &Height[0],
                                     &Format[0]);
         if (!img) {
            printf("Error: couldn't load texture image\n");
            exit(1);
         }
         printf("Texture %d:  %s (%d x %d)\n", i,
                TEXTURE_0_FILE, Width[0], Height[0]);
         glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB,
                      Width[0], Height[0], 0,
                      Format[0], GL_UNSIGNED_BYTE, img);
      }
      else {
         GLubyte *img = LoadRGBImage(TEXTURE_1_FILE, &Width[1], &Height[1],
                                     &Format[1]);
         if (!img) {
            printf("Error: couldn't load texture image\n");
            exit(1);
         }
         printf("Texture %d:  %s (%d x %d)\n", i,
                TEXTURE_1_FILE, Width[1], Height[1]);
         glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB,
                      Width[1], Height[1], 0,
                      Format[1], GL_UNSIGNED_BYTE, img);
      }

      if (i < 1)
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
      else
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

      if (TexEnabled[i])
         glEnable(GL_TEXTURE_RECTANGLE_NV);
   }

   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
}


int main( int argc, char *argv[] )
{
   GLint i;

   glutInit( &argc, argv );
   glutInitWindowSize( 300, 300 );
   glutInitWindowPosition( 0, 0 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0] );
   glewInit();

   Init( argc, argv );

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   if (Animate)
      glutIdleFunc( Idle );

   glutCreateMenu(ModeMenu);

   for (i = 0; i < NumUnits; i++) {
      char s[100];
      sprintf(s, "Toggle Texture %d", i);
      glutAddMenuEntry(s, TEX0 + i);
   }
   glutAddMenuEntry("Toggle Animation", ANIMATE);
   glutAddMenuEntry("GL_CLAMP", CLAMP);
   glutAddMenuEntry("GL_CLAMP_TO_EDGE", CLAMP_TO_EDGE);
   glutAddMenuEntry("GL_CLAMP_TO_BORDER", CLAMP_TO_BORDER);
   glutAddMenuEntry("GL_NEAREST", NEAREST_FILTER);
   glutAddMenuEntry("GL_LINEAR", LINEAR_FILTER);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
