/* Test glGenProgramsNV(), glIsProgramNV(), glLoadProgramNV() */

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

   glBegin(GL_POLYGON);
   glVertexAttrib2fNV(0, -1, -1);
   glVertexAttrib2fNV(0, 1, -1);
   glVertexAttrib2fNV(0, 0,  1);
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


static void Init( void )
{
   static const char *prog1 =
      "!!VP1.0\n"
      "MUL   o[COL0].xyz, R0, c[35]; \n"
      "END\n";
   static const char *prog2 =
      "!!VP1.0\n"
      "#\n"
      "# c[0-3]  = modelview projection (composite) matrix\n"
      "# c[32]   = normalized light direction in object-space\n"
      "# c[35]   = yellow diffuse material, (1.0, 1.0, 0.0, 1.0)\n"
      "# c[64].x = 0.0\n"
      "# c[64].z = 0.125, a scaling factor\n"
      "#\n"
      "# outputs diffuse illumination for color and perturbed position\n"
      "#\n"
      "DP3   R0, c[32], v[NRML];     # light direction DOT normal\n"
      "MUL   o[COL0].xyz, R0, c[35]; \n"
      "MAX   R0, c[64].x, R0; \n"
      "MUL   R0, R0, v[NRML]; \n"
      "MUL   R0, R0, c[64].z;  \n"
      "ADD   R1, v[OPOS], -R0;       # perturb object space position\n"
      "DP4   o[HPOS].x, c[0], R1; \n"
      "DP4   o[HPOS].y, c[1], R1; \n"
      "DP4   o[HPOS].z, c[2], R1; \n"
      "DP4   o[HPOS].w, c[3], R1; \n"
      "END\n";
   static const char *prog3 = 
      "!!VP1.0\n"
      "DP4   o[HPOS].x, c[0], v[OPOS];\n"
      "DP4   o[HPOS].y, c[1], v[OPOS];\n"
      "DP4   o[HPOS].z, c[2], v[OPOS];\n"
      "DP4   o[HPOS].w, c[3], v[OPOS];\n"
      "DP3   R0.x, c[4], v[NRML];\n"
      "DP3   R0.y, c[5], v[NRML]; \n"
      "DP3   R0.z, c[6], v[NRML];           # R0 = n' = transformed normal\n"
      "DP3   R1.x, c[32], R0;               # R1.x = Lpos DOT n'\n"
      "DP3   R1.y, c[33], R0;               # R1.y = hHat DOT n'\n"
      "MOV   R1.w, c[38].x;                 # R1.w = specular power\n"
      "LIT   R2, R1;                        # Compute lighting values\n"
      "MAD   R3, c[35].x, R2.y, c[35].y;    # diffuse + emissive\n"
      "MAD   o[COL0].xyz, c[36], R2.z, R3;  # + specular\n"
      "END\n";
   static const char *prog4 = 
      "!!VP1.0\n"
      "DP4   R2, R3, c[A0.x];\n"
      "DP4   R2, R3, c[A0.x + 5];\n"
      "DP4   o[HPOS], R3, c[A0.x - 4];\n"
      "END\n";
   static const char *prog5 = 
      "!!VSP1.0\n"
      "DP4   R2, R3, c[A0.x];\n"
      "DP4   R2, R3, v[0];\n"
      "DP4   c[3], R3, R2;\n"
      "END\n";


   GLuint progs[5];

   glGenProgramsNV(2, progs);
   assert(progs[0]);
   assert(progs[1]);
   assert(progs[0] != progs[1]);

   glGenProgramsNV(3, progs + 2);
   assert(progs[2]);
   assert(progs[3]);
   assert(progs[2] != progs[3]);
   assert(progs[0] != progs[2]);


   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 1,
                   strlen(prog1),
                   (const GLubyte *) prog1);
   assert(glIsProgramNV(1));

   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 2,
                   strlen(prog2),
                   (const GLubyte *) prog2);
   assert(glIsProgramNV(2));

   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 3,
                   strlen(prog3),
                   (const GLubyte *) prog3);
   assert(glIsProgramNV(3));

   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 4,
                   strlen(prog4),
                   (const GLubyte *) prog4);
   assert(glIsProgramNV(4));

   glLoadProgramNV(GL_VERTEX_STATE_PROGRAM_NV, 5,
                   strlen(prog5),
                   (const GLubyte *) prog5);
   assert(glIsProgramNV(5));

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
