/* Test glGenProgramsNV(), glIsProgramNV(), glLoadProgramNV() */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static float Zrot = 0.0;


static void Display( void )
{
   glClearColor(0.3, 0.3, 0.3, 1);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glEnable(GL_VERTEX_PROGRAM_NV);

   glLoadIdentity();
   glRotatef(Zrot, 0, 0, 1);

   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW, GL_IDENTITY_NV);
   glPushMatrix();

   glVertexAttrib3fNV(3, 1, 0.5, 0.25);
   glBegin(GL_TRIANGLES);
#if 1
   glVertexAttrib3fNV(3, 1.0, 0.0, 0.0);
   glVertexAttrib2fNV(0, -0.5, -0.5);
   glVertexAttrib3fNV(3, 0.0, 1.0, 0.0);
   glVertexAttrib2fNV(0, 0.5, -0.5);
   glVertexAttrib3fNV(3, 0.0, 0.0, 1.0);
   glVertexAttrib2fNV(0, 0,  0.5);
#else
   glVertex2f( -1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 0,  1);
#endif
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   /*   glFrustum( -2.0, 2.0, -2.0, 2.0, 5.0, 25.0 );*/
   glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   /*glTranslatef( 0.0, 0.0, -15.0 );*/
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'z':
         Zrot -= 5.0;
         break;
      case 'Z':
         Zrot += 5.0;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   static const char *prog1 =
      "!!VP1.0\n"
      "MOV  o[COL0], v[COL0];\n"
#if 0
      "MOV   o[HPOS], v[OPOS];\n"
#else
      "DP4  o[HPOS].x, v[OPOS], c[0];\n"
      "DP4  o[HPOS].y, v[OPOS], c[1];\n"
      "DP4  o[HPOS].z, v[OPOS], c[2];\n"
      "DP4  o[HPOS].w, v[OPOS], c[3];\n"
#endif
      "END\n";

   if (!glutExtensionSupported("GL_NV_vertex_program")) {
      printf("Sorry, this program requires GL_NV_vertex_program\n");
      exit(1);
   }

   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 1,
                   strlen(prog1),
                   (const GLubyte *) prog1);
   assert(glIsProgramNV(1));

   glBindProgramNV(GL_VERTEX_PROGRAM_NV, 1);

   printf("glGetError = %d\n", (int) glGetError());
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
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
