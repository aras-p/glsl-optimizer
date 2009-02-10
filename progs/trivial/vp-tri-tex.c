/* Test glGenProgramsNV(), glIsProgramNV(), glLoadProgramNV() */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static void Init( void )
{
   GLint errno;
   GLuint prognum;
   
   static const char *prog1 =
      "!!ARBvp1.0\n"
      "MOV  result.texcoord[0], vertex.texcoord[0];\n"
      "MOV  result.position, vertex.position;\n"
      "END\n";


   glGenProgramsARB(1, &prognum);

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		      strlen(prog1), (const GLubyte *) prog1);

   assert(glIsProgramARB(prognum));
   errno = glGetError();
   printf("glGetError = %d\n", errno);
   if (errno != GL_NO_ERROR)
   {
      GLint errorpos;

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("%s\n", (char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   }

#define SIZE 32
   {
      GLubyte tex2d[SIZE][SIZE][3];
      GLint s, t;

      for (s = 0; s < SIZE; s++) {
	 for (t = 0; t < SIZE; t++) {
#if 0
	    tex2d[t][s][0] = (s < SIZE/2) ? 0 : 255;
	    tex2d[t][s][1] = (t < SIZE/2) ? 0 : 255;
	    tex2d[t][s][2] = 0;
#else
	    tex2d[t][s][0] = s*255/(SIZE-1);
	    tex2d[t][s][1] = t*255/(SIZE-1);
	    tex2d[t][s][2] = 0;
#endif
	 }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glTexImage2D(GL_TEXTURE_2D, 0, 3, SIZE, SIZE, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, tex2d);

      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   }

}

static void Display( void )
{
   glClearColor(0.3, 0.3, 0.3, 1);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glEnable(GL_VERTEX_PROGRAM_NV);

   glBegin(GL_TRIANGLES);
   glTexCoord2f(1,-1); 
   glVertex3f( 0.9, -0.9, -0.0);
   glTexCoord2f(1,1); 
   glVertex3f( 0.9,  0.9, -0.0);
   glTexCoord2f(-1,0); 
   glVertex3f(-0.9,  0.0, -0.0);
   glEnd();


   glFlush(); 
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   /*glTranslatef( 0.0, 0.0, -15.0 );*/
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




int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 250, 250 );
   glutInitDisplayMode( GLUT_DEPTH | GLUT_RGB | GLUT_SINGLE );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
