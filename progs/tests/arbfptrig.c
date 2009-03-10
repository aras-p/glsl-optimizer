/* GL_ARB_fragment_program texture test */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.c"


#define TEXTURE_FILE "../images/girl.rgb"

static GLfloat Xrot = 0.0, Yrot = 0.0, Zrot = 0.0;

#define PI 3.141592

static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1.0, 0.0, 0.0);
   glRotatef(Yrot, 0.0, 1.0, 0.0);
   glRotatef(Zrot, 0.0, 0.0, 1.0);

   glBegin(GL_POLYGON);
   glTexCoord2f(-PI, 0);   glVertex2f(-1, -1);
   glTexCoord2f(PI, 0);   glVertex2f( 1, -1);
   glTexCoord2f(PI, 1);   glVertex2f( 1,  1);
   glTexCoord2f(-PI, 1);   glVertex2f(-1,  1);
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -8.0 );
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


static void Init( void )
{
   static const char *modulate2D =
      "!!ARBfp1.0\n"
      "TEMP R0;\n"
      "MOV R0, {0,0,0,1};\n"
      "SCS R0, fragment.texcoord[0].x; \n"
      "ADD R0, R0, {1.0}.x;\n"
      "MUL R0, R0, {0.5}.x;\n"
      "MOV result.color, R0; \n"
      "END"
      ;
   GLuint modulateProg;
   GLuint Texture;

   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
      printf("Error: GL_ARB_fragment_program not supported!\n");
      exit(1);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* Setup the fragment program */
   glGenProgramsARB(1, &modulateProg);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, modulateProg);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(modulate2D), (const GLubyte *)modulate2D);

   printf("glGetError = 0x%x\n", (int) glGetError());
   printf("glError(GL_PROGRAM_ERROR_STRING_ARB) = %s\n",
          (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   assert(glIsProgramARB(modulateProg));

   glEnable(GL_FRAGMENT_PROGRAM_ARB);

   /* Load texture */
   glGenTextures(1, &Texture);
   glBindTexture(GL_TEXTURE_2D, Texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image file %s\n", TEXTURE_FILE);
      exit(1);
   }
   /* XXX this enable shouldn't really be needed!!! */
   glEnable(GL_TEXTURE_2D);

   glClearColor(.3, .3, .3, 0);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 250, 250 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
