/* Test vertex state program execution */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>



static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   glPushMatrix();
   glutSolidCube(2.0);
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


static void Test1( void )
{
   static const GLfloat p[4] = {9, 8, 7, 6};
   GLfloat q[4];
   /* test addition */
   static const char *prog =
      "!!VSP1.0\n"
      "MOV R0, c[0];\n"
      "MOV R1, c[1];\n"
      "ADD  c[2], R0, R1;\n"
      "END\n";

   glLoadProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 1,
                   strlen(prog),
                   (const GLubyte *) prog);
   assert(glIsProgramNV(1));

   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 0, 1, 2, 3, 4);
   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 1, 10, 20, 30, 40);

   glExecuteProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 1, p);

   glGetProgramParameterfvNV(GL_VERTEX_PROGRAM_NV, 2, GL_PROGRAM_PARAMETER_NV, q);
   printf("Result c[2] = %g %g %g %g  (should be 11 22 33 44)\n",
          q[0], q[1], q[2], q[3]);
}


static void Test2( void )
{
   static const GLfloat p[4] = {9, 8, 7, 6};
   GLfloat q[4];
   /* test swizzling */
   static const char *prog =
      "!!VSP1.0\n"
      "MOV R0, c[0].wzyx;\n"
      "MOV R1, c[1].wzyx;\n"
      "ADD c[2], R0, R1;\n"
      "END\n";

   glLoadProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 1,
                   strlen(prog),
                   (const GLubyte *) prog);
   assert(glIsProgramNV(1));

   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 0, 1, 2, 3, 4);
   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 1, 10, 20, 30, 40);

   glExecuteProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 1, p);

   glGetProgramParameterfvNV(GL_VERTEX_PROGRAM_NV, 2, GL_PROGRAM_PARAMETER_NV, q);
   printf("Result c[2] = %g %g %g %g  (should be 44 33 22 11)\n",
          q[0], q[1], q[2], q[3]);
}


static void Test3( void )
{
   static const GLfloat p[4] = {0, 0, 0, 0};
   GLfloat q[4];
   /* normalize vector */
   static const char *prog =
      "!!VSP1.0\n"
      "# c[0] = (nx,ny,nz)\n"
      "# R0.xyz = normalize(R1)\n"
      "# R0.w   = 1/sqrt(nx*nx + ny*ny + nz*nz)\n"
      "# c[2] = R0\n"
      "DP3 R0.w, c[0], c[0];\n"
      "RSQ R0.w, R0.w;\n"
      "MUL R0.xyz, c[0], R0.w;\n"
      "MOV c[2], R0;\n"
      "END\n";

   glLoadProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 1,
                   strlen(prog),
                   (const GLubyte *) prog);
   assert(glIsProgramNV(1));

   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 0, 0, 10, 0, 0);

   glExecuteProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 1, p);

   glGetProgramParameterfvNV(GL_VERTEX_PROGRAM_NV, 2, GL_PROGRAM_PARAMETER_NV, q);
   printf("Result c[2] = %g %g %g %g  (should be 0, 1, 0, 0.1)\n",
          q[0], q[1], q[2], q[3]);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 50, 50 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );

   if (!glutExtensionSupported("GL_NV_vertex_program")) {
      printf("Sorry, this program requires GL_NV_vertex_program\n");
      exit(1);
   }

   Test1();
   Test2();
   Test3();
   glutMainLoop();
   return 0;
}
