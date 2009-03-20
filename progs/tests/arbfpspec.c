/*
 * To demo that specular color gets lost someplace after vertex
 * program completion and fragment program startup
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
   GLuint prognum, fprognum;
	
   static const char prog[] = 
      "!!ARBvp1.0\n"
      "DP4   result.position.x, state.matrix.mvp.row[0], vertex.position ;\n"
      "DP4   result.position.y, state.matrix.mvp.row[1], vertex.position ;\n"
      "DP4   result.position.z, state.matrix.mvp.row[2], vertex.position ;\n"
      "DP4   result.position.w, state.matrix.mvp.row[3], vertex.position ;\n"
      "MOV   result.color.front.primary,   {.5, .5, .5, 1};\n"		
      "MOV   result.color.front.secondary, {1, 1, 1, 1};\n"
      "END";

    static const char fprog[] = 
      "!!ARBfp1.0\n"			  
      "MOV result.color, fragment.color.secondary;\n"
      "END";

   if (!glutExtensionSupported("GL_ARB_vertex_program")) {
      printf("Sorry, this program requires GL_ARB_vertex_program");
      exit(1);
   }

   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
      printf("Sorry, this program requires GL_ARB_fragment_program");
      exit(1);
   }
   	
   glGenProgramsARB(1, &prognum);
   glGenProgramsARB(1, &fprognum);

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

   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fprognum);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(fprog), (const GLubyte *) fprog);
   errno = glGetError();
   printf("glGetError = %d\n", errno);
   if (errno != GL_NO_ERROR)
   {
      GLint errorpos;

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("%s\n", (char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   }

   glEnable(GL_VERTEX_PROGRAM_ARB);
   glEnable(GL_FRAGMENT_PROGRAM_ARB);
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
