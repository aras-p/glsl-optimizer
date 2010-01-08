/*
 * Compressed texture demo.  Written by Daniel Borca.
 * This program is in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.c" /* I know, this is a hack. */
#define TEXTURE_FILE "../images/tree2.rgba"


static float Rot = 0.0;
static GLboolean Anim = 1;

typedef struct {
        GLubyte *data;
        GLuint size;
        GLenum format;
        GLuint w, h;

        GLenum TC;

        GLubyte *cData;
        GLuint cSize;
        GLenum cFormat;
} TEXTURE;

static TEXTURE *Tx, t1, t2, t3;
static GLboolean fxt1, dxtc, s3tc;


static const char *TextureName (GLenum TC)
{
 switch (TC) {
        case GL_RGB:
             return "RGB";
        case GL_RGBA:
             return "RGBA";
        case GL_COMPRESSED_RGB:
             return "COMPRESSED_RGB";
        case GL_COMPRESSED_RGBA:
             return "COMPRESSED_RGBA";
        case GL_COMPRESSED_RGB_FXT1_3DFX:
             return "GL_COMPRESSED_RGB_FXT1_3DFX";
        case GL_COMPRESSED_RGBA_FXT1_3DFX:
             return "GL_COMPRESSED_RGBA_FXT1_3DFX";
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
             return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
             return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
             return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
             return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
        case GL_RGB_S3TC:
             return "GL_RGB_S3TC";
        case GL_RGB4_S3TC:
             return "GL_RGB4_S3TC";
        case GL_RGBA_S3TC:
             return "GL_RGBA_S3TC";
        case GL_RGBA4_S3TC:
             return "GL_RGBA4_S3TC";
        case 0:
             return "Invalid format";
        default:
             return "Unknown format";
 }
}


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void Idle( void )
{
   float t = glutGet(GLUT_ELAPSED_TIME) * 0.001;  /* in seconds */
   Rot = t * 360 / 4;  /* 1 rotation per 4 seconds */
   glutPostRedisplay();
}


static void Display( void )
{
   /* draw background gradient */
   glDisable(GL_TEXTURE_2D);
   glBegin(GL_POLYGON);
   glColor3f(1.0, 0.0, 0.2); glVertex2f(-1.5, -1.0);
   glColor3f(1.0, 0.0, 0.2); glVertex2f( 1.5, -1.0);
   glColor3f(0.0, 0.0, 1.0); glVertex2f( 1.5,  1.0);
   glColor3f(0.0, 0.0, 1.0); glVertex2f(-1.5,  1.0);
   glEnd();

   glPushMatrix();
   glRotatef(Rot, 0, 0, 1);

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);

   glBegin(GL_POLYGON);
   glTexCoord2f(0, 1);  glVertex2f(-1, -0.5);
   glTexCoord2f(1, 1);  glVertex2f( 1, -0.5);
   glTexCoord2f(1, 0);  glVertex2f( 1,  0.5);
   glTexCoord2f(0, 0);  glVertex2f(-1,  0.5);
   glEnd();

   glPopMatrix();

   glDisable(GL_TEXTURE_2D);

   /* info */
   glDisable(GL_BLEND);
   glColor4f(1, 1, 1, 1);

   glRasterPos3f(-1.2, -0.7, 0);
   PrintString("Selected: ");
   PrintString(TextureName(Tx->TC));
   if (Tx->cData) {
      char tmp[64];
      glRasterPos3f(-1.2, -0.8, 0);
      PrintString("Internal: ");
      PrintString(TextureName(Tx->cFormat));
      glRasterPos3f(-1.2, -0.9, 0);
      PrintString("Size    : ");
      sprintf(tmp, "%d (%d%% of %d)", Tx->cSize, Tx->cSize * 100 / Tx->size, Tx->size);
      PrintString(tmp);
   }

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( -1.5, 1.5, -1.0, 1.0, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void ReInit( GLenum TC, TEXTURE *Tx )
{
   GLint rv, v;

   if ((Tx->TC == TC) && (Tx->cData != NULL)) {
      glCompressedTexImage2DARB(GL_TEXTURE_2D, /* target */
	                        0,             /* level */
	                        Tx->cFormat,   /* real format */
	                        Tx->w,         /* original width */
	                        Tx->h,         /* original height */
	                        0,             /* border */
	                        Tx->cSize,     /* compressed size*/
	                        Tx->cData);    /* compressed data*/
   } else {
      glTexImage2D(GL_TEXTURE_2D,    /* target */
                   0,                /* level */
                   TC,               /* internal format */
                   Tx->w, Tx->h,     /* width, height */
                   0,                /* border */
                   Tx->format,       /* texture format */
                   GL_UNSIGNED_BYTE, /* texture type */
                   Tx->data);        /* the texture */


      v = 0;
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                               GL_TEXTURE_INTERNAL_FORMAT, &v);
      printf("Requested internal format = 0x%x, actual = 0x%x\n", TC, v);

      if (0) {
         GLint r, g, b, a, l, i;
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &r);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &g);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &b);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &a);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_LUMINANCE_SIZE, &l);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTENSITY_SIZE, &i);
         printf("Compressed Bits per R: %d  G: %d  B: %d  A: %d  L: %d  I: %d\n",
                r, g, b, a, l, i);
      }

      /* okay, now cache the compressed texture */
      Tx->TC = TC;
      if (Tx->cData != NULL) {
         free(Tx->cData);
         Tx->cData = NULL;
      }
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &rv);
      if (rv) {
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint *)&Tx->cFormat);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, (GLint *)&Tx->cSize);
         if ((Tx->cData = malloc(Tx->cSize)) != NULL) {
            glGetCompressedTexImageARB(GL_TEXTURE_2D, 0, Tx->cData);
         }
      }
   }
}


static void Init( void )
{
   /* HEIGHT * WIDTH + 1 (for trailing '\0') */
   static char pattern[8 * 32 + 1] = {"\
                                \
    MMM    EEEE   SSS    AAA    \
   M M M  E      S   S  A   A   \
   M M M  EEEE    SS    A   A   \
   M M M  E         SS  AAAAA   \
   M   M  E      S   S  A   A   \
   M   M   EEEE   SSS   A   A   \
                                "
      };

   GLuint i, j;

   GLubyte (*texture1)[8 * 32][4];
   GLubyte (*texture2)[256][256][4];

   t1.w = 32;
   t1.h = 8;
   t1.size = t1.w * t1.h * 4;
   t1.data = malloc(t1.size);
   t1.format = GL_RGBA;
   t1.TC = GL_RGBA;

   texture1 = (GLubyte (*)[8 * 32][4])t1.data;
   for (i = 0; i < sizeof(pattern) - 1; i++) {
       switch (pattern[i]) {
              default:
              case ' ':
                   (*texture1)[i][0] = 255;
                   (*texture1)[i][1] = 255;
                   (*texture1)[i][2] = 255;
                   (*texture1)[i][3] = 64;
                   break;
              case 'M':
                   (*texture1)[i][0] = 255;
                   (*texture1)[i][1] = 0;
                   (*texture1)[i][2] = 0;
                   (*texture1)[i][3] = 255;
                   break;
              case 'E':
                   (*texture1)[i][0] = 0;
                   (*texture1)[i][1] = 255;
                   (*texture1)[i][2] = 0;
                   (*texture1)[i][3] = 255;
                   break;
              case 'S':
                   (*texture1)[i][0] = 0;
                   (*texture1)[i][1] = 0;
                   (*texture1)[i][2] = 255;
                   (*texture1)[i][3] = 255;
                   break;
              case 'A':
                   (*texture1)[i][0] = 255;
                   (*texture1)[i][1] = 255;
                   (*texture1)[i][2] = 0;
                   (*texture1)[i][3] = 255;
                   break;
       }
   }

   t2.w = 256;
   t2.h = 256;
   t2.size = t2.w * t2.h * 4;
   t2.data = malloc(t2.size);
   t2.format = GL_RGBA;
   t2.TC = GL_RGBA;

   texture2 = (GLubyte (*)[256][256][4])t2.data;
   for (j = 0; j < t2.h; j++) {
      for (i = 0; i < t2.w; i++) {
         (*texture2)[j][i][0] = sqrt(i * j * 255 * 255 / (t2.w * t2.h));
         (*texture2)[j][i][1] = 0;
         (*texture2)[j][i][2] = 0;
         (*texture2)[j][i][3] = 255;
      }
   }

   t3.data = LoadRGBImage(TEXTURE_FILE, (GLint *)&t3.w, (GLint *)&t3.h, &t3.format);
   t3.size = t3.w * t3.h * ((t3.format == GL_RGB) ? 3 : 4);
   t3.TC = GL_RGBA;

   ReInit(GL_RGBA, Tx = &t1);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glEnable(GL_TEXTURE_2D);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
      case ' ':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc( Idle );
         else
            glutIdleFunc( NULL );
         break;
      case 't':
         if (Tx == &t1) {
            Tx = &t2;
         } else if (Tx == &t2) {
            Tx = &t3;
         } else {
            Tx = &t1;
         }
         ReInit(Tx->TC, Tx);
         break;
      case '9':
         ReInit(GL_RGB, Tx);
         break;
      case '0':
         ReInit(GL_RGBA, Tx);
         break;
      case '1':
         ReInit(GL_COMPRESSED_RGB, Tx);
         break;
      case '2':
         ReInit(GL_COMPRESSED_RGBA, Tx);
         break;
      case '3':
         if (fxt1) ReInit(GL_COMPRESSED_RGB_FXT1_3DFX, Tx);
         break;
      case '4':
         if (fxt1) ReInit(GL_COMPRESSED_RGBA_FXT1_3DFX, Tx);
         break;
      case '5':
         if (dxtc) ReInit(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, Tx);
         break;
      case '6':
         if (dxtc) ReInit(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, Tx);
         break;
      case '7':
         if (dxtc) ReInit(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, Tx);
         break;
      case '8':
         if (dxtc) ReInit(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, Tx);
         break;
      case 'a':
         if (s3tc) ReInit(GL_RGB_S3TC, Tx);
         break;
      case 's':
         if (s3tc) ReInit(GL_RGB4_S3TC, Tx);
         break;
      case 'd':
         if (s3tc) ReInit(GL_RGBA_S3TC, Tx);
         break;
      case 'f':
         if (s3tc) ReInit(GL_RGBA4_S3TC, Tx);
         break;
   }
   glutPostRedisplay();
}


int main( int argc, char *argv[] )
{
   float gl_version;
   GLint num_formats;
   GLint i;
   GLint formats[64];


   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 300 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   if (glutCreateWindow(argv[0]) <= 0) {
      printf("Couldn't create window\n");
      exit(0);
   }

   glewInit();
   gl_version = atof( (const char *) glGetString( GL_VERSION ) );
   if ( (gl_version < 1.3) 
	&& !glutExtensionSupported("GL_ARB_texture_compression") ) {
      printf("Sorry, GL_ARB_texture_compression not supported\n");
      exit(0);
   }
   if (glutExtensionSupported("GL_3DFX_texture_compression_FXT1")) {
      fxt1 = GL_TRUE;
   }
   if (glutExtensionSupported("GL_EXT_texture_compression_s3tc")) {
      dxtc = GL_TRUE;
   }
   if (glutExtensionSupported("GL_S3_s3tc")) {
      s3tc = GL_TRUE;
   }

   glGetIntegerv( GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, & num_formats );
   
   (void) memset( formats, 0, sizeof( formats ) );
   glGetIntegerv( GL_COMPRESSED_TEXTURE_FORMATS_ARB, formats );

   printf( "The following texture formats are supported:\n" );
   for ( i = 0 ; i < num_formats ; i++ ) {
      printf( "\t%s\n", TextureName( formats[i] ) );
   }
	
   Init();

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
