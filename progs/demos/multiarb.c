/*
 * GL_ARB_multitexture demo
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 *
 * Brian Paul  November 1998  This program is in the public domain.
 * Modified on 12 Feb 2002 for > 2 texture units.
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#include "readtex.h"

#define TEXTURE_1_FILE "../images/girl.rgb"
#define TEXTURE_2_FILE "../images/reflect.rgb"

#define TEX0 1
#define TEX7 8
#define ANIMATE 10
#define QUIT 100

static GLboolean Animate = GL_TRUE;
static GLint NumUnits = 1;
static GLboolean TexEnabled[8];

static GLfloat Drift = 0.0;
static GLfloat Xrot = 20.0, Yrot = 30.0, Zrot = 0.0;


static void Idle( void )
{
   if (Animate) {
      GLint i;

      Drift = glutGet(GLUT_ELAPSED_TIME) * 0.001;

      for (i = 0; i < NumUnits; i++) {
         glActiveTextureARB(GL_TEXTURE0_ARB + i);
         glMatrixMode(GL_TEXTURE);
         glLoadIdentity();
         if (i == 0) {
            glTranslatef(Drift, 0.0, 0.0);
            glScalef(2, 2, 1);
         }
         else if (i == 1) {
            glTranslatef(0.0, Drift, 0.0);
         }
         else {
            float tx = 0.5, ty = 0.5;
            glTranslatef(tx, ty, 0.0);
            glRotatef(180.0 * Drift, 0, 0, 1);
            glScalef(1.0/i, 1.0/i, 1.0/i);
            glTranslatef(-tx, -ty + i * 0.1, 0.0);
         }
      }
      glMatrixMode(GL_MODELVIEW);

      glutPostRedisplay();
   }
}


static void DrawObject(void)
{
   static const GLfloat tex_coords[] = {  0.0,  0.0,  1.0,  1.0,  0.0 };
   static const GLfloat vtx_coords[] = { -1.0, -1.0,  1.0,  1.0, -1.0 };
   GLint i, j;

   if (!TexEnabled[0] && !TexEnabled[1])
      glColor3f(0.1, 0.1, 0.1);  /* add onto this */
   else
      glColor3f(1, 1, 1);  /* modulate this */

   glBegin(GL_QUADS);
   for (j = 0; j < 4; j++ ) {
      for (i = 0; i < NumUnits; i++) {
         if (TexEnabled[i])
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, 
                                 tex_coords[j], tex_coords[j+1]);
      }
      glVertex2f( vtx_coords[j], vtx_coords[j+1] );
   }
   glEnd();
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
   glFrustum( -1.0, 1.0, -1.0, 1.0, 10.0, 100.0 );
   /*glOrtho( -6.0, 6.0, -6.0, 6.0, 10.0, 100.0 );*/
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -70.0 );
}


static void ToggleUnit(int unit)
{
   TexEnabled[unit] = !TexEnabled[unit];
   glActiveTextureARB(GL_TEXTURE0_ARB + unit);
   if (TexEnabled[unit])
      glEnable(GL_TEXTURE_2D);
   else
      glDisable(GL_TEXTURE_2D);
   printf("Enabled: ");
   for (unit = 0; unit < NumUnits; unit++)
      printf("%d ", (int) TexEnabled[unit]);
   printf("\n");
}


static void ModeMenu(int entry)
{
   if (entry >= TEX0 && entry <= TEX7) {
      /* toggle */
      GLint i = entry - TEX0;
      ToggleUnit(i);
   }
   else if (entry==ANIMATE) {
      Animate = !Animate;
      if (Animate)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
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
   case 'a':
      Animate = !Animate;
      break;
   case '0':
      ToggleUnit(0);
      break;
   case '1':
      ToggleUnit(1);
      break;
   case '2':
      ToggleUnit(2);
      break;
   case '3':
      ToggleUnit(3);
      break;
   case '4':
      ToggleUnit(4);
      break;
   case '5':
      ToggleUnit(5);
      break;
   case '6':
      ToggleUnit(6);
      break;
   case '7':
      ToggleUnit(7);
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
   GLuint texObj[8];
   GLint size, i;

   const char *exten = (const char *) glGetString(GL_EXTENSIONS);
   if (!strstr(exten, "GL_ARB_multitexture")) {
      printf("Sorry, GL_ARB_multitexture not supported by this renderer.\n");
      exit(1);
   }

   glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &NumUnits);
   printf("%d texture units supported\n", NumUnits);
   if (NumUnits > 8)
      NumUnits = 8;

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
   printf("%d x %d max texture size\n", size, size);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   for (i = 0; i < NumUnits; i++) {
      if (i < 2)
         TexEnabled[i] = GL_TRUE;
      else
         TexEnabled[i] = GL_FALSE;
   }

   /* allocate two texture objects */
   glGenTextures(NumUnits, texObj);

   /* setup the texture objects */
   for (i = 0; i < NumUnits; i++) {

      glActiveTextureARB(GL_TEXTURE0_ARB + i);
      glBindTexture(GL_TEXTURE_2D, texObj[i]);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      if (i == 0) {
         if (!LoadRGBMipmaps(TEXTURE_1_FILE, GL_RGB)) {
            printf("Error: couldn't load texture image\n");
            exit(1);
         }
      }
      else if (i == 1) {
         if (!LoadRGBMipmaps(TEXTURE_2_FILE, GL_RGB)) {
            printf("Error: couldn't load texture image\n");
            exit(1);
         }
      }
      else {
         /* checker */
         GLubyte image[8][8][3];
         GLint i, j;
         for (i = 0; i < 8; i++) {
            for (j = 0; j < 8; j++) {
               if ((i + j) & 1) {
                  image[i][j][0] = 50;
                  image[i][j][1] = 50;
                  image[i][j][2] = 50;
               }
               else {
                  image[i][j][0] = 25;
                  image[i][j][1] = 25;
                  image[i][j][2] = 25;
               }
            }
         }
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0,
                      GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *) image);
      }

      /* Bind texObj[i] to ith texture unit */
      if (i < 2)
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      else
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

      if (TexEnabled[i])
         glEnable(GL_TEXTURE_2D);
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

   Init( argc, argv );

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   if (Animate)
      glutIdleFunc(Idle);

   glutCreateMenu(ModeMenu);

   for (i = 0; i < NumUnits; i++) {
      char s[100];
      sprintf(s, "Toggle Texture %d", i);
      glutAddMenuEntry(s, TEX0 + i);
   }
   glutAddMenuEntry("Toggle Animation", ANIMATE);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
