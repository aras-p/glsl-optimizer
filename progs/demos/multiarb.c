
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
static GLfloat drift_increment = 0.005;
static GLfloat Xrot = 20.0, Yrot = 30.0, Zrot = 0.0;


static void Idle( void )
{
   if (Animate) {
      GLint i;

      Drift += drift_increment;
      if (Drift >= 1.0)
         Drift = 0.0;

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
            glTranslatef(0.5, 0.5, 0.0);
            glRotatef(180.0 * Drift, 0, 0, 1);
            glScalef(1.0/i, 1.0/i, 1.0/i);
            glTranslatef(-0.5, -0.5, 0.0);
         }
      }
      glMatrixMode(GL_MODELVIEW);

      glutPostRedisplay();
   }
}


static void DrawObject(void)
{
   GLint i;
   GLint j;
   static const GLfloat   tex_coords[] = {  0.0,  0.0,  1.0,  1.0,  0.0 };
   static const GLfloat   vtx_coords[] = { -1.0, -1.0,  1.0,  1.0, -1.0 };

   if (!TexEnabled[0] && !TexEnabled[1])
      glColor3f(0.1, 0.1, 0.1);  /* add onto this */
   else
      glColor3f(1, 1, 1);  /* modulate this */

   glBegin(GL_QUADS);

   /* Toggle between the vector and scalar entry points.  This is done purely
    * to hit multiple paths in the driver.
    */
   if ( Drift > 0.49 ) {
      for (j = 0; j < 4; j++ ) {
	 for (i = 0; i < NumUnits; i++)
	    glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, 
				 tex_coords[j], tex_coords[j+1]);
	 glVertex2f( vtx_coords[j], vtx_coords[j+1] );
      }
   }
   else {
      for (j = 0; j < 4; j++ ) {
	 for (i = 0; i < NumUnits; i++)
	    glMultiTexCoord2fvARB(GL_TEXTURE0_ARB + i, & tex_coords[j]);
	 glVertex2fv( & vtx_coords[j] );
      }
   }

   glEnd();
}



static void Display( void )
{
   static GLint T0 = 0;
   static GLint Frames = 0;
   GLint t;

   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
      glScalef(5.0, 5.0, 5.0);
      DrawObject();
   glPopMatrix();

   glutSwapBuffers();

   Frames++;

   t = glutGet(GLUT_ELAPSED_TIME);
   if (t - T0 >= 250) {
      GLfloat seconds = (t - T0) / 1000.0;
      drift_increment = 2.2 * seconds / Frames;
      T0 = t;
      Frames = 0;
   }
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


static void ModeMenu(int entry)
{
   if (entry >= TEX0 && entry <= TEX7) {
      /* toggle */
      GLint i = entry - TEX0;
      TexEnabled[i] = !TexEnabled[i];
      glActiveTextureARB(GL_TEXTURE0_ARB + i);
      if (TexEnabled[i])
         glEnable(GL_TEXTURE_2D);
      else
         glDisable(GL_TEXTURE_2D);
      printf("Enabled: ");
      for (i = 0; i < NumUnits; i++)
         printf("%d ", (int) TexEnabled[i]);
      printf("\n");
   }
   else if (entry==ANIMATE) {
      Animate = !Animate;
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
   glutIdleFunc( Idle );

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
