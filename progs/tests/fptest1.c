/* Test GL_NV_fragment_program */

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


static void Init( void )
{
#if 0
   static const char *prog0 =
      "!!FP1.0\n"
      "MUL   o[COLR], R0, f[WPOS]; \n"
      "ADD   o[COLH], H3, f[TEX0]; \n"
      "ADD_SAT o[COLH], H3, f[TEX0]; \n"
      "ADDX o[COLH], H3, f[TEX0]; \n"
      "ADDHC o[COLH], H3, f[TEX0]; \n"
      "ADDXC o[COLH], H3, f[TEX0]; \n"
      "ADDXC_SAT o[COLH], H30, f[TEX0]; \n"
      "MUL   o[COLR].xy, R0.wzyx, f[WPOS]; \n"
      "MUL   o[COLR], H0, f[WPOS]; \n"
      "MUL   o[COLR], -H0, f[WPOS]; \n"
      "MOV   RC, H1; \n"
      "MOV   HC, H2; \n"
      "END \n"
      ;
#endif

   /* masked updates, defines, declarations */
   static const char *prog1 =
      "!!FP1.0\n"
      "DEFINE foo = {1, 2, 3, 4}; \n"
      "DEFINE foo2 = 5; \n"
      "DECLARE foo3 = {5, 6, 7, 8}; \n"
      "DECLARE bar = 3; \n"
      "DECLARE bar2; \n"
      "DECLARE bar3 = bar; \n"
      "#DECLARE bar4 = { a, b, c, d }; \n"
      "MOV o[COLR].xy,   R0; \n"
      "MOV o[COLR] (NE), R0; \n"
      "MOV o[COLR] (NE.wzyx), R0; \n"
      "MOV o[COLR].xy (NE.wzyx), R0; \n"
      "MOV RC.x (EQ), R1.x; \n"
      "KIL NE; \n"
      "KIL EQ.xyxy; \n"
      "END \n"
      ;

   /* texture instructions */
   static const char *prog2 =
      "!!FP1.0\n"
      "TEX R0, f[TEX0], TEX0, 2D; \n"
      "TEX R1, f[TEX1], TEX1, CUBE; \n"
      "TEX R2, f[TEX2], TEX2, 3D; \n"
      "TXP R3, f[TEX3], TEX3, RECT; \n"
      "TXD R3, R2, R1, f[TEX3], TEX3, RECT; \n"
      "MUL o[COLR], R0, f[COL0]; \n"
      "END \n"
      ;

   /* test negation, absolute value */
   static const char *prog3 =
      "!!FP1.0\n"
      "MOV R0, -R1; \n"
      "MOV R0, +R1; \n"
      "MOV R0, |-R1|; \n"
      "MOV R0, |+R1|; \n"
      "MOV R0, -|R1|; \n"
      "MOV R0, +|R1|; \n"
      "MOV R0, -|-R1|; \n"
      "MOV R0, -|+R1|; \n"
      "MOV o[COLR], R0; \n"
      "END \n"
      ;

   /* literal constant sources */
   static const char *prog4 =
      "!!FP1.0\n"
      "DEFINE Pi = 3.14159; \n"
      "MOV R0, {1, -2, +3, 4}; \n"
      "MOV R0, 5; \n"
      "MOV R0, -5; \n"
      "MOV R0, +5; \n"
      "MOV R0, Pi; \n"
      "MOV o[COLR], R0; \n"
      "END \n"
      ;

   /* change the fragment color in a simple way */
   static const char *prog10 =
      "!!FP1.0\n"
      "DEFINE blue = {0, 0, 1, 0};\n"
      "DECLARE color; \n"
      "MOV R0, f[COL0]; \n"
      "#ADD o[COLR], R0, f[COL0]; \n"
      "#ADD o[COLR], blue, f[COL0]; \n"
      "#ADD o[COLR], {1, 0, 0, 0}, f[COL0]; \n"
      "ADD o[COLR], color, f[COL0]; \n"
      "END \n"
      ;

   GLuint progs[20];

   if (!glutExtensionSupported ("GL_NV_fragment_program")) {
	   printf("Sorry, this program requires GL_NV_fragment_program\n");
	   exit(1);
   }

   glGenProgramsNV(20, progs);
   assert(progs[0]);
   assert(progs[1]);
   assert(progs[0] != progs[1]);

#if 0
   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[0],
                   strlen(prog0),
                   (const GLubyte *) prog0);
   assert(glIsProgramNV(progs[0]));
#endif

   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[1],
                   strlen(prog1),
                   (const GLubyte *) prog1);
   assert(glIsProgramNV(progs[1]));

   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[2],
                   strlen(prog2),
                   (const GLubyte *) prog2);
   assert(glIsProgramNV(progs[2]));
   glBindProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[2]);

   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[3],
                   strlen(prog3),
                   (const GLubyte *) prog3);
   assert(glIsProgramNV(progs[3]));
   glBindProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[3]);

   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[4],
                   strlen(prog4),
                   (const GLubyte *) prog4);
   assert(glIsProgramNV(progs[4]));
   glBindProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[4]);


   /* a real program */
   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[10],
                   strlen(prog10),
                   (const GLubyte *) prog10);
   assert(glIsProgramNV(progs[10]));
   glBindProgramNV(GL_FRAGMENT_PROGRAM_NV, progs[10]);

   glProgramNamedParameter4fNV(progs[10],
                               strlen("color"), (const GLubyte *) "color",
                               1, 0, 0, 1);

   glEnable(GL_FRAGMENT_PROGRAM_NV);
   glEnable(GL_ALPHA_TEST);
   glAlphaFunc(GL_ALWAYS, 0.0);

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
