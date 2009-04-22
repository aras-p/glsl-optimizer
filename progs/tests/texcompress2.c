/*
 * Test texture compression.
 */


#include <assert.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "readtex.c"

#define IMAGE_FILE "../images/arch.rgb"

static int ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLenum CompFormat;
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


static const char *
LookupFormat(GLenum format)
{
   switch (format) {
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
   default:
      return "other";
   }
}


static void
TestSubTex(void)
{
   GLboolean all = 0*GL_TRUE;
   GLubyte *buffer;
   GLint size, fmt;

   glGetTexLevelParameteriv(Target, 0,
                            GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
   glGetTexLevelParameteriv(Target, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);

   buffer = (GLubyte *) malloc(size);
   glGetCompressedTexImageARB(Target, 0, buffer);

   printf("Testing sub-texture replacement\n");
   if (all)
      glCompressedTexImage2DARB(Target, 0,
                                fmt, ImgWidth, ImgHeight, 0,
                                size, buffer);
   else {
      /* bottom half */
      glCompressedTexSubImage2DARB(Target, 0,
                                   0, 0, /* pos */
                                   ImgWidth, ImgHeight / 2,
                                   fmt, size/2, buffer);
      /* top half */
      glCompressedTexSubImage2DARB(Target, 0,
                                   0, ImgHeight / 2, /* pos */
                                   ImgWidth, ImgHeight / 2,
                                   fmt, size/2, buffer + size / 2);
   }

   free(buffer);
}


static void
TestGetTex(void)
{
   GLubyte *buffer;

   buffer = (GLubyte *) malloc(3 * ImgWidth * ImgHeight);

   glGetTexImage(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 buffer);

   free(buffer);
}


static void
LoadCompressedImage(const char *file)
{
   const GLenum filter = GL_LINEAR;
   GLubyte *image;
   GLint p;

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /*
    * Load image and scale if needed.
    */
   image = LoadRGBImage( file, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }
   printf("Image is %d x %d\n", ImgWidth, ImgHeight);

   /* power of two */
   assert(ImgWidth == 128 || ImgWidth == 256 || ImgWidth == 512);
   assert(ImgWidth == 128 || ImgHeight == 256 || ImgHeight == 512);

   if (ImgFormat == GL_RGB)
      CompFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
   else
      CompFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

   if (ImgFormat == GL_RGBA) {
      int i, numAlpha = 0;
      for (i = 0; i < ImgWidth * ImgHeight; i++) {
         if (image[i*4+3] != 0 && image[i*4+3] != 0xff) {
            numAlpha++;
         }
         if (image[i*4+3] == 0)
            image[i*4+3] = 4 * i / ImgWidth;
      }
      printf("Num Alpha !=0,255: %d\n", numAlpha);
   }

   CompFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;


   /*
    * Give image to OpenGL and have it compress it.
    */
   glTexImage2D(Target, 0, CompFormat, ImgWidth, ImgHeight, 0,
                ImgFormat, GL_UNSIGNED_BYTE, image);
   CheckError(__LINE__);

   free(image);

   glGetTexLevelParameteriv(Target, 0, GL_TEXTURE_INTERNAL_FORMAT, &p);
   printf("Compressed Internal Format: %s (0x%x)\n", LookupFormat(p), p);
   assert(p == CompFormat);

   printf("Original size:   %d bytes\n", ImgWidth * ImgHeight * 3);
   glGetTexLevelParameteriv(Target, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &p);
   printf("Compressed size: %d bytes\n", p);

   glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, filter);

   if (0)
      TestSubTex();
   else
      TestGetTex();

}


static void
Init(const char *file)
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

   LoadCompressedImage(file);

   glEnable(GL_TEXTURE_2D);

   if (ImgFormat == GL_RGBA) {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
   }
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

   if (argc > 1)
      Init(argv[1]);
   else
      Init(IMAGE_FILE);

   glutMainLoop();
   return 0;
}
