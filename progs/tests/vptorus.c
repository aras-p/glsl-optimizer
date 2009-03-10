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
   /* borrowed from an nvidia demo:
    * c[0..3] = modelview matrix
    * c[4..7] = inverse modelview matrix
    * c[32] = light pos
    * c[35] = diffuse color
    */
   static const char prog[] = 
      "!!VP1.0\n"
      "#Simple transform and diffuse lighting\n"
      "\n"
      "DP4   o[HPOS].x, c[0], v[OPOS] ;	# object x MVP -> clip\n"
      "DP4   o[HPOS].y, c[1], v[OPOS] ;\n"
      "DP4   o[HPOS].z, c[2], v[OPOS] ;\n"
      "DP4   o[HPOS].w, c[3], v[OPOS] ;\n"

      "DP3   R1.x, c[4], v[NRML] ;	# normal x MV-1T -> lighting normal\n"
      "DP3   R1.y, c[5], v[NRML] ;\n"
      "DP3   R1.z, c[6], v[NRML] ;\n"

      "DP3   R0, c[32], R1 ;  		# L.N\n"
      "MUL   o[COL0].xyz, R0, c[35] ;   # col = L.N * diffuse\n"
      "MOV   o[TEX0], v[TEX0];\n"
      "END";

   if (!glutExtensionSupported("GL_NV_vertex_program")) {
      printf("Sorry, this program requires GL_NV_vertex_program");
      exit(1);
   }

   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 1,
                   strlen(prog), (const GLubyte *) prog);
   assert(glIsProgramNV(1));
   glBindProgramNV(GL_VERTEX_PROGRAM_NV, 1);

   /* Load the program registers */
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4, GL_MODELVIEW, GL_INVERSE_TRANSPOSE_NV);

   /* Light position */
   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 32, 2, 2, 4, 1);
   /* Diffuse material color */
   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 35, 0.25, 0, 0.25, 1);

   glEnable(GL_VERTEX_PROGRAM_NV);
   glEnable(GL_DEPTH_TEST);
   glClearColor(0.3, 0.3, 0.3, 1);

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
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
