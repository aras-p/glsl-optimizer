/*
 * 'Texture leak' test
 *
 * Allocates and uses an additional texture of the maximum supported size for
 * each frame. This tests the system's ability to cope with using increasing
 * amounts of texture memory.
 *
 * Michel Dänzer  July 2009  This program is in the public domain.
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <GL/glew.h>
#include <GL/glut.h>


GLint size;
GLvoid *image;
static GLuint numTexObj;
static GLuint *texObj;


static void Idle( void )
{
   glutPostRedisplay();
}


static void DrawObject(void)
{
   static const GLfloat tex_coords[] = {  0.0,  0.0,  1.0,  1.0,  0.0 };
   static const GLfloat vtx_coords[] = { -1.0, -1.0,  1.0,  1.0, -1.0 };
   GLint i, j;

   glEnable(GL_TEXTURE_2D);

   for (i = 0; i < numTexObj; i++) {
      glBindTexture(GL_TEXTURE_2D, texObj[i]);
      glBegin(GL_QUADS);
      for (j = 0; j < 4; j++ ) {
         glTexCoord2f(tex_coords[j], tex_coords[j+1]);
         glVertex2f( vtx_coords[j], vtx_coords[j+1] );
      }
      glEnd();
   }
}


static void Display( void )
{
   struct timeval start, end;

   texObj = realloc(texObj, ++numTexObj * sizeof(*texObj));

   /* allocate a texture object */
   glGenTextures(1, texObj + (numTexObj - 1));

   glBindTexture(GL_TEXTURE_2D, texObj[numTexObj - 1]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   memset(image, (16 * numTexObj) & 0xff, 4 * size * size);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);

   gettimeofday(&start, NULL);

   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      glScalef(5.0, 5.0, 5.0);
      DrawObject();
   glPopMatrix();

   glutSwapBuffers();

   glFinish();
   gettimeofday(&end, NULL);
   printf("Rendering frame took %lu ms using %u MB of textures\n",
	  end.tv_sec * 1000 + end.tv_usec / 1000 - start.tv_sec * 1000 -
	  start.tv_usec / 1000, numTexObj * 4 * size / 1024 * size / 1024);

   sleep(1);
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( -6.0, 6.0, -6.0, 6.0, 10.0, 100.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -70.0 );
}


static void Init( int argc, char *argv[] )
{
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
   printf("%d x %d max texture size\n", size, size);

   image = malloc(4 * size * size);
   if (!image) {
      fprintf(stderr, "Failed to allocate %u bytes of memory\n", 4 * size * size);
      exit(1);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);

   Idle();
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 300, 300 );
   glutInitWindowPosition( 0, 0 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0] );
   glewInit();

   Init( argc, argv );

   glutReshapeFunc( Reshape );
   glutDisplayFunc( Display );
   glutIdleFunc(Idle);

   glutMainLoop();
   return 0;
}
