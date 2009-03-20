/* Test GL_ARB_fragment_program */

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

   glColor4f(0, 0.5, 0, 1);
   glColor4f(0, 1, 0, 1);
   glBegin(GL_POLYGON);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 0,  1);
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
   
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
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
   static const char *prog0 =
      "!!ARBfp1.0\n"
      "TEMP R0, RC, HC, H0, H1, H2, H3, H30 ;\n"
      "MUL       result.color, R0, fragment.position; \n"
      "ADD       result.color, H3, fragment.texcoord; \n"
      "ADD_SAT   result.color, H3, fragment.texcoord; \n"
      "MUL       result.color.xy, R0.wzyx, fragment.position; \n"
      "MUL       result.color, H0, fragment.position; \n"
      "MUL       result.color, -H0, fragment.position; \n"
      "MOV       RC, H1; \n"
      "MOV       HC, H2; \n"
      "END \n"
      ;
   /* masked updates, defines, declarations */
   static const char *prog1 =
      "!!ARBfp1.0\n"
      "PARAM foo = {1., 2., 3., 4.}; \n"
      "PARAM foo2 = 5.; \n"
      "PARAM foo3 = {5., 6., 7., 8.}; \n"
      "PARAM bar = 3.; \n"
      "TEMP  R0, R1, RC, EQ, NE, bar2; \n"
      "ALIAS bar3 = bar; \n"
      "MOV result.color.xy,  R0; \n"
      "MOV result.color,     R0; \n"
      "MOV result.color.xyzw, R0; \n"
      "MOV result.color.xy,  R0; \n"
      "MOV RC.x, R1.x; \n"
      "KIL NE; \n"
      "KIL EQ.xyxy; \n"
      "END \n"
      ;

   /* texture instructions */
   static const char *prog2 =
      "!!ARBfp1.0\n"
      "TEMP R0, R1, R2, R3;\n"
      "TEX R0, fragment.texcoord,    texture[0], 2D; \n"
      "TEX R1, fragment.texcoord[1], texture[1], CUBE; \n"
      "TEX R2, fragment.texcoord[2], texture[2], 3D; \n"
      "TXP R3, fragment.texcoord[3], texture[3], RECT; \n"
      "MUL result.color, R0, fragment.color; \n"
      "END \n"
      ;

   /* test negation, absolute value */
   static const char *prog3 =
      "!!ARBfp1.0\n"
      "TEMP R0, R1;\n"
      "MOV R0,  R1; \n"
      "MOV R0, -R1; \n"
      "MOV result.color, R0; \n"
      "END \n"
      ;

   /* literal constant sources */
   static const char *prog4 =
      "!!ARBfp1.0\n"
      "TEMP R0, R1;\n"
      "PARAM Pi = 3.14159; \n"
      "MOV R0, {1., -2., +3., 4.}; \n"
      "MOV R0, 5.; \n"
      "MOV R0, -5.; \n"
      "MOV R0, 5.; \n"
      "MOV R0, Pi; \n"
      "MOV result.color, R0; \n"
      "END \n"
      ;

   /* change the fragment color in a simple way */
   static const char *prog10 =
      "!!ARBfp1.0\n"
      "PARAM blue = {0., 0., 1., 0.};\n"
      "PARAM color = {1., 0., 0., 1.};\n"
      "TEMP R0; \n"
      "MOV R0, fragment.color; \n"
      "#ADD result.color, R0, fragment.color; \n"
      "#ADD result.color, blue, fragment.color; \n"
      "#ADD result.color, {1., 0., 0., 0.}, fragment.color; \n"
      "ADD result.color, color, fragment.color; \n"
      "END \n"
      ;

   GLuint progs[20];

   glGenProgramsARB(20, progs);
   assert(progs[0]);
   assert(progs[1]);
   assert(progs[0] != progs[1]);


   printf("program 0:\n");
	load_program(prog0, progs[0]);
   printf("program 1:\n");
	load_program(prog1, progs[1]);
   printf("program 2:\n");
	load_program(prog2, progs[2]);
   printf("program 3:\n");
	load_program(prog3, progs[3]);
	printf("program 4:\n");
	load_program(prog4, progs[4]);
	printf("program 10:\n");
	load_program(prog10, progs[5]);


   glEnable(GL_FRAGMENT_PROGRAM_ARB);
   glEnable(GL_ALPHA_TEST);
   glAlphaFunc(GL_ALWAYS, 0.0);
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
