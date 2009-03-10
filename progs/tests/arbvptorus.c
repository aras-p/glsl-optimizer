/*
 * A lit, rotating torus via vertex program
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static float Xrot = 0.0, Yrot = 0.0, Zrot = 0.0;
static GLboolean Anim = GL_TRUE;


static void Idle( void )
{
   Xrot += .3;
   Yrot += .4;
   Zrot += .2;
   glutPostRedisplay();
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
      glRotatef(Xrot, 1, 0, 0);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Zrot, 0, 0, 1);
      glutSolidTorus(0.75, 2.0, 10, 20);
   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -2.0, 2.0, -2.0, 2.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -12.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case ' ':
         Xrot = Yrot = Zrot = 0;
         break;
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
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


static void SpecialKey( int key, int x, int y )
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Xrot -= step;
         break;
      case GLUT_KEY_DOWN:
         Xrot += step;
         break;
      case GLUT_KEY_LEFT:
         Yrot -= step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot += step;
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   GLint errno;
   GLuint prognum;
	
   /* borrowed from an nvidia demo:
    * c[0..3] = modelview matrix
    * c[4..7] = invtrans modelview matrix
    * c[32] = light pos
    * c[35] = diffuse color
    */
   static const char prog[] = 
      "!!ARBvp1.0\n"
      "OPTION ARB_position_invariant ;"
      "TEMP R0, R1; \n"

      "# normal x MV-1T -> lighting normal\n"		
      "DP3   R1.x, state.matrix.modelview.invtrans.row[0], vertex.normal ;\n"
      "DP3   R1.y, state.matrix.modelview.invtrans.row[1], vertex.normal;\n"
      "DP3   R1.z, state.matrix.modelview.invtrans.row[2], vertex.normal;\n"

      "DP3   R0, program.local[32], R1;                  # L.N\n"
#if 0
      "MUL   result.color.xyz, R0, program.local[35] ;   # col = L.N * diffuse\n"
#else
      "MUL   result.color.primary.xyz, R0, program.local[35] ;   # col = L.N * diffuse\n"
#endif
      "MOV   result.texcoord, vertex.texcoord;\n"
      "END";

   if (!glutExtensionSupported("GL_ARB_vertex_program")) {
      printf("Sorry, this program requires GL_ARB_vertex_program");
      exit(1);
   }

   	
   glGenProgramsARB(1, &prognum);

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(prog), (const GLubyte *) prog);

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

   /* Light position */
   glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 32, 2, 2, 4, 1);
   /* Diffuse material color */
   glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 35, 0.25, 0, 0.25, 1);

   glEnable(GL_VERTEX_PROGRAM_ARB);
   glEnable(GL_DEPTH_TEST);
   glClearColor(0.3, 0.3, 0.3, 1);
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
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
