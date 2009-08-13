
/*
 * Test texture wrap modes.
 * Press 'b' to toggle texture image borders.  You should see the same
 * rendering whether or not you're using borders.
 *
 * Brian Paul   March 2001
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif

#ifndef GL_EXT_texture_mirror_clamp
#define GL_MIRROR_CLAMP_EXT               0x8742
#define GL_MIRROR_CLAMP_TO_EDGE_EXT       0x8743
#define GL_MIRROR_CLAMP_TO_BORDER_EXT     0x8912
#endif

#define BORDER_TEXTURE 1
#define NO_BORDER_TEXTURE 2

#define SIZE 8
static GLubyte BorderImage[SIZE+2][SIZE+2][4];
static GLubyte NoBorderImage[SIZE][SIZE][4];
static GLuint Border = 0;

#define TILE_SIZE 110

#define WRAP_MODE(m)        { m , # m, GL_TRUE,  1.0, { NULL, NULL } }
#define WRAP_EXT(m,e1,e2,v) { m , # m, GL_FALSE, v,   { e1,   e2   } }
    
struct wrap_mode {
   GLenum       mode;
   const char * name;
   GLboolean    supported;
   GLfloat      version;
   const char * extension_names[2];
};

static struct wrap_mode modes[] = {
   WRAP_MODE( GL_REPEAT ),
   WRAP_MODE( GL_CLAMP ),
   WRAP_EXT ( GL_CLAMP_TO_EDGE,   "GL_EXT_texture_edge_clamp",
	                          "GL_SGIS_texture_edge_clamp",
	      1.2 ),
   WRAP_EXT ( GL_CLAMP_TO_BORDER, "GL_ARB_texture_border_clamp",
	                          "GL_SGIS_texture_border_clamp",
	      1.3 ),
   WRAP_EXT ( GL_MIRRORED_REPEAT, "GL_ARB_texture_mirrored_repeat",
	                          "GL_IBM_texture_mirrored_repeat",
	      1.4 ),
   WRAP_EXT ( GL_MIRROR_CLAMP_EXT, "GL_ATI_texture_mirror_once",
	                           "GL_EXT_texture_mirror_clamp",
	      999.0 ),
   WRAP_EXT ( GL_MIRROR_CLAMP_TO_BORDER_EXT, "GL_EXT_texture_mirror_clamp",
	                                     NULL,
	      999.0 ),
   WRAP_EXT ( GL_MIRROR_CLAMP_TO_EDGE_EXT, "GL_ATI_texture_mirror_once",
	                                   "GL_EXT_texture_mirror_clamp",
	      999.0 ),
   { 0 }
};

static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void Display( void )
{
   GLenum i, j;
   GLint offset;
   GLfloat version;

   /* Fill in the extensions that are supported.
    */
    
   version = atof( (char *) glGetString( GL_VERSION ) );
   for ( i = 0 ; modes[i].mode != 0 ; i++ ) {
      if ( ((modes[i].extension_names[0] != NULL)
	    && glutExtensionSupported(modes[i].extension_names[0]))
	   || ((modes[i].extension_names[1] != NULL)
	       && glutExtensionSupported(modes[i].extension_names[1])) ) {
	 modes[i].supported = GL_TRUE;
      }
      else if ( !modes[i].supported && (modes[i].version <= version) ) {
	 fprintf( stderr, "WARNING: OpenGL library meets minimum version\n"
		          "         requirement for %s, but the\n"
		          "         extension string is not advertised.\n"
		  	  "         (%s%s%s)\n",
		  modes[i].name,
		  modes[i].extension_names[0],
		  (modes[i].extension_names[1] != NULL) 
		      ? " or " : "",
		  (modes[i].extension_names[1] != NULL)
		      ? modes[i].extension_names[1] : "" );
	 modes[i].supported = GL_TRUE;
      }
   }


   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear( GL_COLOR_BUFFER_BIT );

#if 0
   /* draw texture as image */
   glDisable(GL_TEXTURE_2D);
   glWindowPos2iARB(1, 1);
   glDrawPixels(6, 6, GL_RGBA, GL_UNSIGNED_BYTE, (void *) TexImage);
#endif

   glBindTexture(GL_TEXTURE_2D, Border ? BORDER_TEXTURE : NO_BORDER_TEXTURE);


   /* loop over min/mag filters */
   for (i = 0; i < 2; i++) {
      offset = 0;

      if (i) {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

      /* loop over border modes */
      for (j = 0; modes[j].mode != 0; j++) {
         const GLfloat x0 = 0, y0 = 0, x1 = (TILE_SIZE - 10), y1 = (TILE_SIZE - 10);
         const GLfloat b = 1.2;
         const GLfloat s0 = -b, t0 = -b, s1 = 1.0+b, t1 = 1.0+b;

	 if ( modes[j].supported != GL_TRUE )
	     continue;

         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, modes[j].mode);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, modes[j].mode);

         glPushMatrix();
            glTranslatef(offset * TILE_SIZE + 10, i * TILE_SIZE + 40, 0);
	    offset++;

            glEnable(GL_TEXTURE_2D);
            glColor3f(1, 1, 1);
            glBegin(GL_POLYGON);
            glTexCoord2f(s0, t0);  glVertex2f(x0, y0);
            glTexCoord2f(s1, t0);  glVertex2f(x1, y0);
            glTexCoord2f(s1, t1);  glVertex2f(x1, y1);
            glTexCoord2f(s0, t1);  glVertex2f(x0, y1);
            glEnd();

            /* draw red outline showing bounds of texture at s=0,1 and t=0,1 */
            glDisable(GL_TEXTURE_2D);
            glColor3f(1, 0, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x0 + b * (x1-x0) / (s1-s0), y0 + b * (y1-y0) / (t1-t0));
            glVertex2f(x1 - b * (x1-x0) / (s1-s0), y0 + b * (y1-y0) / (t1-t0));
            glVertex2f(x1 - b * (x1-x0) / (s1-s0), y1 - b * (y1-y0) / (t1-t0));
            glVertex2f(x0 + b * (x1-x0) / (s1-s0), y1 - b * (y1-y0) / (t1-t0));
            glEnd();

         glPopMatrix();
      }
   }

   glDisable(GL_TEXTURE_2D);
   glColor3f(1, 1, 1);
   offset = 0;
   for (i = 0; modes[i].mode != 0; i++) {
      if ( modes[i].supported ) {
         glWindowPos2iARB( offset * TILE_SIZE + 10, 5 + ((offset & 1) * 15) );
	 PrintString(modes[i].name);
	 offset++;
      }
   }

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         Border = !Border;
         printf("Texture Border Size = %d\n", Border);
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   static const GLubyte border[4] = { 0, 255, 0, 255 };
   static const GLfloat borderf[4] = { 0, 1.0, 0, 1.0 };
   GLint i, j;

   for (i = 0; i < SIZE+2; i++) {
      for (j = 0; j < SIZE+2; j++) {
         if (i == 0 || j == 0 || i == SIZE+1 || j == SIZE+1) {
            /* border color */
            BorderImage[i][j][0] = border[0];
            BorderImage[i][j][1] = border[1];
            BorderImage[i][j][2] = border[2];
            BorderImage[i][j][3] = border[3];
         }
         else if ((i + j) & 1) {
            /* white */
            BorderImage[i][j][0] = 255;
            BorderImage[i][j][1] = 255;
            BorderImage[i][j][2] = 255;
            BorderImage[i][j][3] = 255;
         }
         else {
            /* black */
            BorderImage[i][j][0] = 0;
            BorderImage[i][j][1] = 0;
            BorderImage[i][j][2] = 0;
            BorderImage[i][j][3] = 0;
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, BORDER_TEXTURE);
#ifdef TEST_PBO_DLIST
   /* test fetching teximage from PBO in display list */
   {
      GLuint b = 42, l = 10;

      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, b);
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER, sizeof(BorderImage),
                      BorderImage, GL_STREAM_DRAW);

      glNewList(l, GL_COMPILE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE+2, SIZE+2, 1,
                GL_RGBA, GL_UNSIGNED_BYTE, (void *) 0/* BorderImage*/);
      glEndList();
      glCallList(l);
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, 0);
   }
#else
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE+2, SIZE+2, 1,
                GL_RGBA, GL_UNSIGNED_BYTE, (void *) BorderImage);
#endif

   for (i = 0; i < SIZE; i++) {
      for (j = 0; j < SIZE; j++) {
         if ((i + j) & 1) {
            /* white */
            NoBorderImage[i][j][0] = 255;
            NoBorderImage[i][j][1] = 255;
            NoBorderImage[i][j][2] = 255;
            NoBorderImage[i][j][3] = 255;
         }
         else {
            /* black */
            NoBorderImage[i][j][0] = 0;
            NoBorderImage[i][j][1] = 0;
            NoBorderImage[i][j][2] = 0;
            NoBorderImage[i][j][3] = 0;
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, NO_BORDER_TEXTURE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, (void *) NoBorderImage);
   glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderf);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 1000, 270 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
