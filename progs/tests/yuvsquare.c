/*
 * Test the GL_NV_texture_rectangle and GL_MESA_ycrcb_texture extensions.
 *
 * Brian Paul   13 September 2002
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>

#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/tile.rgb"

static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLint ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLushort *ImageYUV = NULL;


static void DrawObject(void)
{
   glBegin(GL_QUADS);

   glTexCoord2f(0, 0);
   glVertex2f(-1.0, -1.0);

   glTexCoord2f(1, 0);
   glVertex2f(1.0, -1.0);

   glTexCoord2f(1, 1);
   glVertex2f(1.0, 1.0);

   glTexCoord2f(0, 1);
   glVertex2f(-1.0, 1.0);

   glEnd();
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
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
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
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


#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )

static void ConvertRGBtoYUV(GLint w, GLint h, GLint texel_bytes,
			    const GLubyte *src,
                            GLushort *dest)
{
   GLint i, j;

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const GLfloat r = (src[0]) / 255.0;
         const GLfloat g = (src[1]) / 255.0;
         const GLfloat b = (src[2]) / 255.0;
         GLfloat y, cr, cb;
         GLint iy, icr, icb;

         y  = r * 65.481 + g * 128.553 + b * 24.966 + 16;
         cb = r * -37.797 + g * -74.203 + b * 112.0 + 128;
         cr = r * 112.0 + g * -93.786 + b * -18.214 + 128;
         /*printf("%f %f %f -> %f %f %f\n", r, g, b, y, cb, cr);*/
         iy  = (GLint) CLAMP(y,  0, 254);
         icb = (GLint) CLAMP(cb, 0, 254);
         icr = (GLint) CLAMP(cr, 0, 254);

         if (j & 1) {
            /* odd */
            *dest = (iy << 8) | icr;
         }
         else {
            /* even */
            *dest = (iy << 8) | icb;
         }
         dest++;
	 src += texel_bytes;
      }
   }
}


/*
 * Load an SGI .rgb file and return a pointer to the image data.
 * Input:  imageFile - name of .rgb to read
 * Output:  width - width of image
 *          height - height of image
 *          format - format of image (GL_RGB or GL_RGBA)
 * Return:  pointer to image data or NULL if error
 */
GLushort *LoadYUVImage( const char *imageFile, GLint *width, GLint *height,
                       GLenum *format )
{
   TK_RGBImageRec *image;
   GLint bytes;
   GLushort *buffer;

   image = tkRGBImageLoad( imageFile );
   if (!image) {
      return NULL;
   }

   if (image->components==3) {
      *format = GL_RGB;
   }
   else if (image->components==4) {
      *format = GL_RGBA;
   }
   else {
      /* not implemented */
      fprintf(stderr,
              "Error in LoadYUVImage %d-component images not implemented\n",
              image->components );
      return NULL;
   }

   *width = image->sizeX;
   *height = image->sizeY;

   buffer = (GLushort *) malloc( image->sizeX * image->sizeY * 2 );

   if (buffer)
      ConvertRGBtoYUV( image->sizeX, 
		       image->sizeY,
		       image->components,
		       image->data, 
		       buffer );


   FreeImage(image);
   return buffer;
}



/* #define LINEAR_FILTER */

static void Init( int argc, char *argv[] )
{
   GLuint texObj = 100;
   const char *file;
   int error;

   if (!glutExtensionSupported("GL_MESA_ycbcr_texture")) {
      printf("Sorry, GL_MESA_ycbcr_texture is required\n");
      exit(0);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glBindTexture(GL_TEXTURE_2D, texObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   if (argc > 1)
      file = argv[1];
   else
      file = TEXTURE_FILE;

   ImageYUV = LoadYUVImage(file, &ImgWidth, &ImgHeight, &ImgFormat);
   if (!ImageYUV) {
      printf("Couldn't read %s\n", TEXTURE_FILE);
      exit(0);
   }

   printf("Image: %dx%d\n", ImgWidth, ImgHeight);

/*    error = gluBuild2DMipmaps( GL_TEXTURE_2D, */
/*                               GL_YCBCR_MESA, */
/*                               ImgWidth, ImgHeight, */
/*                               GL_YCBCR_MESA, */
/* 			      GL_UNSIGNED_SHORT_8_8_MESA, */
/*                               ImageYUV ); */



#if 0
   glTexImage2D(GL_TEXTURE_2D, 0, 
		GL_RGB,  
 		ImgWidth, ImgHeight, 0, 
		GL_RGB,  
 		GL_UNSIGNED_SHORT_5_6_5, ImageYUV); 
#else
   glTexImage2D(GL_TEXTURE_2D, 0,
                GL_YCBCR_MESA, 
		ImgWidth, ImgHeight, 0,
                GL_YCBCR_MESA, 
		GL_UNSIGNED_SHORT_8_8_MESA, ImageYUV);
#endif

   glEnable(GL_TEXTURE_2D);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


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

   glutMainLoop();
   return 0;
}
