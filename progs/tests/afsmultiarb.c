/*
 * GL_ATI_fragment_shader test
 * Roland Scheidegger
 *
 * Command line options:
 *    -info      print GL implementation information
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.h"

#define TEXTURE_1_FILE "../images/girl.rgb"
#define TEXTURE_2_FILE "../images/reflect.rgb"

#define TEX0 1
#define TEX7 8
#define ANIMATE 10
#define SHADER 20
#define QUIT 100

static GLboolean Animate = GL_TRUE;
static GLint NumUnits = 6;
static GLboolean TexEnabled[8];
static GLuint boringshaderID = 0;
static GLuint boring2passID = 0;
static GLboolean Shader = GL_FALSE;

static GLfloat Drift = 0.0;
static GLfloat drift_increment = 0.005;
static GLfloat Xrot = 20.0, Yrot = 30.0, Zrot = 0.0;
static GLfloat shaderconstant[4] = {0.5, 0.0, 0.0, 0.0};

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
   if (t - T0 >= 2500) {
      GLfloat seconds = (t - T0) / 1000.0;
      GLfloat fps = Frames / seconds;
      drift_increment = 2.2 * seconds / Frames;
      printf("%d frames in %6.3f seconds = %6.3f FPS\n", Frames, seconds, fps);
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
   else if (entry==SHADER) {
      Shader = !Shader;
      if (Shader) {
	 fprintf(stderr, "using 2-pass shader\n");
	 glBindFragmentShaderATI(boring2passID);
      }
      else {
	 fprintf(stderr, "using 1-pass shader\n");
         glBindFragmentShaderATI(boringshaderID);
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
   if (!strstr(exten, "GL_ATI_fragment_shader")) {
      printf("Sorry, GL_ATI_fragment_shader not supported by this renderer.\n");
      exit(1);
   }


   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
   printf("%d x %d max texture size\n", size, size);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   for (i = 0; i < NumUnits; i++) {
      if (i < 6)
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
/*      if (i < 2)
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      else
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);*/

      if (TexEnabled[i])
         glEnable(GL_TEXTURE_2D);
   }

   boringshaderID = glGenFragmentShadersATI(1);
   boring2passID = glGenFragmentShadersATI(1);
   if (boring2passID == 0)
   {
      fprintf(stderr, "couldn't get frag shader id\n");
      exit(1);
   }
   glBindFragmentShaderATI(boringshaderID);
/* maybe not the most creative shader but at least I know how it should look like! */
   glBeginFragmentShaderATI();
   glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_1_ATI, GL_TEXTURE1_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_2_ATI, GL_TEXTURE2_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_3_ATI, GL_TEXTURE3_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_4_ATI, GL_TEXTURE4_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_5_ATI, GL_TEXTURE5_ARB, GL_SWIZZLE_STR_ATI);
   glColorFragmentOp2ATI(GL_MUL_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_PRIMARY_COLOR, GL_NONE, GL_NONE);
   glAlphaFragmentOp1ATI(GL_MOV_ATI,
			 GL_REG_0_ATI, GL_NONE,
			 GL_PRIMARY_COLOR, GL_NONE, GL_NONE);
   glColorFragmentOp3ATI(GL_MAD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_1_ATI, GL_NONE, GL_NONE,
			 GL_REG_2_ATI, GL_NONE, GL_NONE);
   glColorFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_3_ATI, GL_NONE, GL_NONE);
   glColorFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_4_ATI, GL_NONE, GL_NONE);
   glColorFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_5_ATI, GL_NONE, GL_NONE);
   glEndFragmentShaderATI();

/* mathematically equivalent to first shader but using 2 passes together with
   some tex coord rerouting */
   glBindFragmentShaderATI(boring2passID);
   glBeginFragmentShaderATI();
   glPassTexCoordATI(GL_REG_1_ATI, GL_TEXTURE0_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_2_ATI, GL_TEXTURE2_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_3_ATI, GL_TEXTURE3_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_4_ATI, GL_TEXTURE4_ARB, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_5_ATI, GL_TEXTURE5_ARB, GL_SWIZZLE_STR_ATI);
   glColorFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_2_ATI, GL_NONE, GL_NONE,
			 GL_REG_3_ATI, GL_NONE, GL_NONE);
   glColorFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_4_ATI, GL_NONE, GL_NONE);
   glColorFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_5_ATI, GL_NONE, GL_NONE);
   /* not really a dependant read */
   glSampleMapATI(GL_REG_0_ATI, GL_REG_1_ATI, GL_SWIZZLE_STR_ATI);
   glSampleMapATI(GL_REG_1_ATI, GL_TEXTURE1_ARB, GL_SWIZZLE_STR_ATI);
   glPassTexCoordATI(GL_REG_5_ATI, GL_REG_0_ATI, GL_SWIZZLE_STR_ATI);
   glColorFragmentOp2ATI(GL_MUL_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_PRIMARY_COLOR, GL_NONE, GL_NONE);
   glAlphaFragmentOp1ATI(GL_MOV_ATI,
			 GL_REG_0_ATI, GL_NONE,
			 GL_PRIMARY_COLOR, GL_NONE, GL_NONE);
   glColorFragmentOp3ATI(GL_MAD_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_1_ATI, GL_NONE, GL_NONE,
			 GL_REG_5_ATI, GL_NONE, GL_NONE);
   /* in principle we're finished here, but to test a bit more
      we do some fun with dot ops, replication et al. */
   glSetFragmentShaderConstantATI(GL_CON_3_ATI, shaderconstant);
   glColorFragmentOp2ATI(GL_DOT4_ATI,
			 GL_REG_3_ATI, GL_GREEN_BIT_ATI, GL_EIGHTH_BIT_ATI,
			 GL_ZERO, GL_NONE, GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI,
			 GL_CON_3_ATI, GL_RED, GL_2X_BIT_ATI);
   /* those args must get ignored, except dstReg */
   glAlphaFragmentOp2ATI(GL_DOT4_ATI,
			 GL_REG_4_ATI, GL_NONE,
			 GL_ZERO, GL_NONE, GL_NONE,
			 GL_ZERO, GL_NONE, GL_NONE);
   /* -> reg3 g = reg4 alpha = -0.5 */
   glAlphaFragmentOp2ATI(GL_ADD_ATI,
			 GL_REG_5_ATI, GL_NONE,
			 GL_REG_3_ATI, GL_GREEN, GL_NONE,
			 GL_REG_4_ATI, GL_NONE, GL_NONE);
   /* -> reg5 a = -1 */
   glColorFragmentOp3ATI(GL_DOT2_ADD_ATI,
			 GL_REG_4_ATI, GL_BLUE_BIT_ATI, GL_HALF_BIT_ATI,
			 GL_REG_5_ATI, GL_ALPHA, GL_NEGATE_BIT_ATI,
			 GL_ONE, GL_NONE, GL_BIAS_BIT_ATI,
			 GL_ONE, GL_ALPHA, GL_2X_BIT_ATI | GL_NEGATE_BIT_ATI);
   /* -> reg 4 b = -0.5 */
   glColorFragmentOp2ATI(GL_MUL_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE,
			 GL_REG_4_ATI, GL_BLUE, GL_NEGATE_BIT_ATI | GL_2X_BIT_ATI,
			 GL_REG_0_ATI, GL_NONE, GL_NONE);
   glEndFragmentShaderATI();

   glBindFragmentShaderATI(boringshaderID);
   glEnable(GL_FRAGMENT_SHADER_ATI);

   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
   printf("output should be identical with both shaders to multiarb demo when 6 textures are enabled\n");
}


int main( int argc, char *argv[] )
{
/*   GLint i;*/

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
   glutIdleFunc( Idle );

   glutCreateMenu(ModeMenu);

/*   for (i = 0; i < NumUnits; i++) {
      char s[100];
      sprintf(s, "Toggle Texture %d", i);
      glutAddMenuEntry(s, TEX0 + i);
   }*/
   glutAddMenuEntry("Toggle 1/2 Pass Shader", SHADER);
   glutAddMenuEntry("Toggle Animation", ANIMATE);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
