/* $Id: multipal.c,v 1.2 2002/01/16 01:03:25 kschultz Exp $ */

/*
 * GL_ARB_multitexture demo
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 *
 * Brian Paul  November 1998  This program is in the public domain.
 */

/*
 * $Log: multipal.c,v $
 * Revision 1.2  2002/01/16 01:03:25  kschultz
 * get tests working on windows (Robert Bergkvist)
 *
 * Revision 1.1  2000/11/18 17:12:33  brianp
 * test texture palettes with multitexture
 *
 * Revision 1.7  2000/11/01 16:02:01  brianp
 * print number of texture units
 *
 * Revision 1.6  2000/05/23 23:21:00  brianp
 * set default window pos
 *
 * Revision 1.5  2000/02/02 17:31:45  brianp
 * changed > to >=
 *
 * Revision 1.4  2000/02/02 01:07:21  brianp
 * limit Drift to [0, 1]
 *
 * Revision 1.3  1999/10/21 16:40:32  brianp
 * added -info command line option
 *
 * Revision 1.2  1999/10/13 12:02:13  brianp
 * use texture objects now
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 1.3  1999/03/28 18:20:49  brianp
 * minor clean-up
 *
 * Revision 1.2  1998/11/05 04:34:04  brianp
 * moved image files to ../images/ directory
 *
 * Revision 1.1  1998/11/03 01:36:33  brianp
 * Initial revision
 *
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define GL_GLEXT_LEGACY
#include <GL/glut.h>

#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_1_FILE "../images/tile.rgb"
#define TEXTURE_2_FILE "../images/reflect.rgb"

#define TEX0 1
#define TEX1 2
#define TEXBOTH 3
#define ANIMATE 10
#define QUIT 100

static GLboolean Animate = GL_TRUE;

static GLfloat Drift = 0.0;
static GLfloat Xrot = 20.0, Yrot = 30.0, Zrot = 0.0;



static void Idle( void )
{
   if (Animate) {
      Drift += 0.05;
      if (Drift >= 1.0)
         Drift = 0.0;

#ifdef GL_ARB_multitexture
      glActiveTextureARB(GL_TEXTURE0_ARB);
#endif
      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glTranslatef(Drift, 0.0, 0.0);
      glMatrixMode(GL_MODELVIEW);

#ifdef GL_ARB_multitexture
      glActiveTextureARB(GL_TEXTURE1_ARB);
#endif
      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glTranslatef(0.0, Drift, 0.0);
      glMatrixMode(GL_MODELVIEW);

      glutPostRedisplay();
   }
}


static void DrawObject(void)
{
   glBegin(GL_QUADS);

#ifdef GL_ARB_multitexture
   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 0.0);
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 0.0);
   glVertex2f(-1.0, -1.0);

   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 2.0, 0.0);
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, 0.0);
   glVertex2f(1.0, -1.0);

   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 2.0, 2.0);
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, 1.0);
   glVertex2f(1.0, 1.0);

   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 2.0);
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 1.0);
   glVertex2f(-1.0, 1.0);
#else
   glTexCoord2f(0.0, 0.0);
   glVertex2f(-1.0, -1.0);

   glTexCoord2f(1.0, 0.0);
   glVertex2f(1.0, -1.0);

   glTexCoord2f(1.0, 1.0);
   glVertex2f(1.0, 1.0);

   glTexCoord2f(0.0, 1.0);
   glVertex2f(-1.0, 1.0);
#endif

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


static void ModeMenu(int entry)
{
   GLboolean enable0 = GL_FALSE, enable1 = GL_FALSE;
   if (entry==TEX0) {
      enable0 = GL_TRUE;
   }
   else if (entry==TEX1) {
      enable1 = GL_TRUE;
   }
   else if (entry==TEXBOTH) {
      enable0 = GL_TRUE;
      enable1 = GL_TRUE;
   }
   else if (entry==ANIMATE) {
      Animate = !Animate;
   }
   else if (entry==QUIT) {
      exit(0);
   }

   if (entry != ANIMATE) {
#ifdef GL_ARB_multitexture
      glActiveTextureARB(GL_TEXTURE0_ARB);
#endif
      if (enable0) {
         glEnable(GL_TEXTURE_2D);
      }
      else
         glDisable(GL_TEXTURE_2D);

#ifdef GL_ARB_multitexture
      glActiveTextureARB(GL_TEXTURE1_ARB);
#endif
      if (enable1) {
         glEnable(GL_TEXTURE_2D);
      }
      else
         glDisable(GL_TEXTURE_2D);
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


static void load_tex(const char *fname, int channel)
{
   GLubyte *image;
   GLenum format, type;
   GLint w, h;
   GLubyte *grayImage;
   int i;
   GLubyte table[256][4];

   image = LoadRGBImage(fname, &w, &h, &format);
   if (!image)
      exit(1);

   printf("%s %d x %d\n", fname, w, h);
   grayImage = malloc(w * h * 1);
   assert(grayImage);
   for (i = 0; i < w * h; i++) {
      int g = (image[i*3+0] + image[i*3+1] + image[i*3+2]) / 3;
      assert(g < 256);
      grayImage[i] = g;
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX, w, h, 0, GL_COLOR_INDEX,
                GL_UNSIGNED_BYTE, grayImage);

   for (i = 0; i < 256; i++) {
      table[i][0] = channel ? i : 0;
      table[i][1] = i;
      table[i][2] = channel ? 0 : i;
      table[i][3] = 255;
   }

   glColorTableEXT(GL_TEXTURE_2D,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */

   free(grayImage);
   free(image);
}



static void Init( int argc, char *argv[] )
{
   GLuint texObj[2];
   GLint units;

   const char *exten = (const char *) glGetString(GL_EXTENSIONS);
   if (!strstr(exten, "GL_ARB_multitexture")) {
      printf("Sorry, GL_ARB_multitexture not supported by this renderer.\n");
      exit(1);
   }

   glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &units);
   printf("%d texture units supported\n", units);

   /* allocate two texture objects */
   glGenTextures(2, texObj);

   /* setup texture obj 0 */
   glBindTexture(GL_TEXTURE_2D, texObj[0]);
#ifdef LINEAR_FILTER
   /* linear filtering looks much nicer but is much slower for Mesa */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
foo
#else
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   load_tex(TEXTURE_1_FILE, 0);
#if 0
   if (!LoadRGBMipmaps(TEXTURE_1_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }
#endif

   /* setup texture obj 1 */
   glBindTexture(GL_TEXTURE_2D, texObj[1]);
#ifdef LINEAR_FILTER
   /* linear filtering looks much nicer but is much slower for Mesa */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
foo
#else
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   load_tex(TEXTURE_2_FILE, 1);
#if 0
   if (!LoadRGBMipmaps(TEXTURE_2_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }
#endif

   /* now bind the texture objects to the respective texture units */
#ifdef GL_ARB_multitexture
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glBindTexture(GL_TEXTURE_2D, texObj[0]);
   glActiveTextureARB(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, texObj[1]);
#endif

   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);

   ModeMenu(TEXBOTH);

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
}


int main( int argc, char *argv[] )
{
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
   glutAddMenuEntry("Texture 0", TEX0);
   glutAddMenuEntry("Texture 1", TEX1);
   glutAddMenuEntry("Multi-texture", TEXBOTH);
   glutAddMenuEntry("Toggle Animation", ANIMATE);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
