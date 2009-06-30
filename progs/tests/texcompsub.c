/*
 * Test texture compression.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "texcomp_image.h"

static int ImgWidth = 512;
static int ImgHeight = 512;
static GLenum CompFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
static GLfloat EyeDist = 5.0;
static GLfloat Rot = 0.0;
const GLenum Target = GL_TEXTURE_2D;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error %d at line %d\n", (int) err, line);
   }
}

static void
LoadCompressedImage(void)
{
   const GLenum filter = GL_LINEAR;
   glTexImage2D(Target, 0, CompFormat, ImgWidth, ImgHeight, 0,
                GL_RGB, GL_UNSIGNED_BYTE, NULL);

   /* bottom half */
   glCompressedTexSubImage2DARB(Target, 0,
                                0, 0, /* pos */
                                ImgWidth, ImgHeight / 2,
                                CompFormat, ImgSize / 2, ImgData + ImgSize / 2);
   /* top half */
   glCompressedTexSubImage2DARB(Target, 0,
                                0, ImgHeight / 2, /* pos */
                                ImgWidth, ImgHeight / 2,
                                CompFormat, ImgSize / 2, ImgData);

   glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, filter);
}

static void
Init()
{
   GLint numFormats, formats[100];
   GLint p;

   if (!glutExtensionSupported("GL_ARB_texture_compression")) {
      printf("Sorry, GL_ARB_texture_compression is required.\n");
      exit(1);
   }
   if (!glutExtensionSupported("GL_EXT_texture_compression_s3tc")) {
      printf("Sorry, GL_EXT_texture_compression_s3tc is required.\n");
      exit(1);
   }

   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, &numFormats);
   glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS_ARB, formats);
   printf("%d supported compression formats: ", numFormats);
   for (p = 0; p < numFormats; p++)
      printf("0x%x ", formats[p]);
   printf("\n");

   glEnable(GL_TEXTURE_2D);

   LoadCompressedImage();
}


static void
Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum(-1, 1, -1, 1, 4, 100);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void
Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'd':
         EyeDist -= 1.0;
         if (EyeDist < 4.0)
            EyeDist = 4.0;
         break;
      case 'D':
         EyeDist += 1.0;
         break;
      case 'z':
         Rot += 5.0;
         break;
      case 'Z':
         Rot -= 5.0;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Draw( void )
{
   glClearColor(0.3, 0.3, .8, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   CheckError(__LINE__);
   glPushMatrix();
   glTranslatef(0, 0, -(EyeDist+0.01));
   glRotatef(Rot, 0, 0, 1);
   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);  glVertex2f(-1, -1);
   glTexCoord2f(1, 0);  glVertex2f( 1, -1);
   glTexCoord2f(1, 1);  glVertex2f( 1,  1);
   glTexCoord2f(0, 1);  glVertex2f(-1,  1);
   glEnd();
   glPopMatrix();

   glutSwapBuffers();
}


int
main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 600, 600 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE);

   glutCreateWindow(argv[0]);
   glewInit();

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Draw );

   Init();

   glutMainLoop();
   return 0;
}
