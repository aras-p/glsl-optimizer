/* Test glGenProgramsARB(), glIsProgramARB(), glLoadProgramARB() */

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
   glVertexAttrib2fARB(0, -1, -1);
   glVertexAttrib2fARB(0, 1, -1);
   glVertexAttrib2fARB(0, 0,  1);
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

static void load_program(const char *prog, GLuint prognum)
{
   int a;	
   GLint errorpos, errno;
   
   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(prog), (const GLubyte *) prog);

   assert(glIsProgramARB(prognum));
   errno = glGetError();
   printf("glGetError = %d\n", errno);
   if (errno != GL_NO_ERROR)
   {
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("%s\n", (char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));

      for (a=-10; a<10; a++)
      {
         if ((errorpos+a < 0) || (errorpos+a >= strlen(prog))) continue;	      
         printf("%c", prog[errorpos+a]);
      }	  
      printf("\n");      
      exit(1);
   }
}

static void Init( void )
{
   GLuint prognum[4];
   
   static const char *prog1 =
      "!!ARBvp1.0\n"
      "TEMP R0;\n"      
      "MUL   result.color.primary.xyz, R0, program.local[35]; \n"
      "END\n";
   static const char *prog2 =
      "!!ARBvp1.0\n"
      "#\n"
      "# c[0-3]  = modelview projection (composite) matrix\n"
      "# c[32]   = normalized light direction in object-space\n"
      "# c[35]   = yellow diffuse material, (1.0, 1.0, 0.0, 1.0)\n"
      "# c[64].x = 0.0\n"
      "# c[64].z = 0.125, a scaling factor\n"
      "TEMP R0, R1;\n"      
      "#\n"
      "# outputs diffuse illumination for color and perturbed position\n"
      "#\n"
      "DP3   R0, program.local[32], vertex.normal;  # light direction DOT normal\n"
      "MUL   result.color.primary.xyz, R0, program.local[35]; \n"
      "MAX   R0, program.local[64].x, R0; \n"
      "MUL   R0, R0, vertex.normal; \n"
      "MUL   R0, R0, program.local[64].z;  \n"
      "ADD   R1, vertex.position, -R0;       # perturb object space position\n"
      "DP4   result.position.x, state.matrix.mvp.row[3], R1; \n"
      "DP4   result.position.y, state.matrix.mvp.row[1], R1; \n"
      "DP4   result.position.z, state.matrix.mvp.row[2], R1; \n"
      "DP4   result.position.w, state.matrix.mvp.row[3], R1; \n"
      "END\n";
   static const char *prog3 = 
      "!!ARBvp1.0\n"
      "TEMP R0, R1, R2, R3;\n"      
      "DP4   result.position.x, state.matrix.mvp.row[0], vertex.position;\n"
      "DP4   result.position.y, state.matrix.mvp.row[1], vertex.position;\n"
      "DP4   result.position.z, state.matrix.mvp.row[2], vertex.position;\n"
      "DP4   result.position.w, state.matrix.mvp.row[3], vertex.position;\n"
      "DP3   R0.x, state.matrix.modelview.inverse.row[0], vertex.normal;\n"
      "DP3   R0.y, state.matrix.modelview.inverse.row[1], vertex.normal;\n"
      "DP3   R0.z, state.matrix.modelview.inverse.row[2], vertex.normal;\n" 
      "DP3   R1.x, program.env[32], R0;               # R1.x = Lpos DOT n'\n"
      "DP3   R1.y, program.env[33], R0;               # R1.y = hHat DOT n'\n"
      "MOV   R1.w, program.local[38].x;               # R1.w = specular power\n"
      "LIT   R2, R1;                                  # Compute lighting values\n"
      "MAD   R3, program.env[35].x, R2.y, program.env[35].y;       # diffuse + emissive\n"
      "MAD   result.color.primary.xyz, program.env[36], R2.z, R3;  # + specular\n"
      "END\n";
   static const char *prog4 = 
      "!!ARBvp1.0\n"
      "TEMP R2, R3;\n"
      "PARAM foo = {0., 0., 0., 1.};\n"
      "PARAM blah[] = { program.local[0..8] };\n"
      "ADDRESS A0;\n"		
      "ARL   A0.x, foo.x;\n"   
      "DP4   R2, R3, blah[A0.x].x;\n"
      "DP4   R2, R3, blah[A0.x + 5];\n"
      "DP4   result.position, R3, blah[A0.x - 4];\n"
      "END\n";

   glGenProgramsARB(4, prognum);

   load_program(prog1, prognum[0]);   
   load_program(prog2, prognum[1]);   
   load_program(prog3, prognum[2]);   
   load_program(prog4, prognum[3]);   
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
