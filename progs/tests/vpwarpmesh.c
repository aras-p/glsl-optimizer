/*
 * Warp a triangle mesh with a vertex program.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static float Xrot = -60.0, Yrot = 0.0, Zrot = 0.0;
static GLboolean Anim = GL_TRUE;
static GLfloat Phi = 0.0;


static void Idle( void )
{
   Phi += 0.01;
   glutPostRedisplay();
}


static void DrawMesh( int rows, int cols )
{
   static const GLfloat colorA[3] = { 0, 1, 0 };
   static const GLfloat colorB[3] = { 0, 0, 1 };
   const float dx = 2.0 / (cols - 1);
   const float dy = 2.0 / (rows - 1);
   float x, y;
   int i, j;

#if 1
#define COLOR3FV(c)     glVertexAttrib3fvNV(3, c)
#define VERTEX2F(x, y)  glVertexAttrib2fNV(0, x, y)
#else
#define COLOR3FV(c)     glColor3fv(c)
#define VERTEX2F(x, y)  glVertex2f(x, y)
#endif

   y = -1.0;
   for (i = 0; i < rows - 1; i++) {
      glBegin(GL_QUAD_STRIP);
      x = -1.0;
      for (j = 0; j < cols; j++) {
         if ((i + j) & 1)
            COLOR3FV(colorA);
         else
            COLOR3FV(colorB);
         VERTEX2F(x, y);
         VERTEX2F(x, y + dy);
         x += dx;
      }
      glEnd();
      y += dy;
   }
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
      glRotatef(Xrot, 1, 0, 0);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Zrot, 0, 0, 1);

      /* Position the gravity source */
      {
         GLfloat x, y, z, r = 0.5;
         x = r * cos(Phi);
         y = r * sin(Phi);
         z = 1.0;
         glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 30, x, y, z, 1);
         glDisable(GL_VERTEX_PROGRAM_NV);
         glBegin(GL_POINTS);
         glColor3f(1,1,1);
         glVertex3f(x, y, z);
         glEnd();
      }

      glEnable(GL_VERTEX_PROGRAM_NV);
      DrawMesh(8, 8);
   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   float ar = (float) width / (float) height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0 * ar, 1.0 * ar, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -12.0 );
   glScalef(2, 2, 2);
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'p':
         Phi += 0.2;
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
   /*
    * c[0..3] = modelview matrix
    * c[4..7] = inverse modelview matrix
    * c[30] = gravity source location
    * c[31] = gravity source strength
    * c[32] = light pos
    * c[35] = diffuse color
    */
   static const char prog[] = 
      "!!VP1.0\n"

      "# Compute distance from vertex to gravity source\n"
      "ADD   R1, c[30], -v[OPOS]; # vector from vertex to gravity\n"
      "DP3   R2, R1, R1;          # dot product\n"
      "RSQ   R2, R2.x;            # square root = distance\n"
      "MUL   R2, R2, c[31].xxxx;  # scale by the gravity factor\n"

      "# Displace vertex by gravity factor along R1 vector\n"
      "MAD   R3, R1, R2, v[OPOS];\n"

      "# Continue with typical modelview/projection\n"
      "DP4   o[HPOS].x, c[0], R3 ;	# object x MVP -> clip\n"
      "DP4   o[HPOS].y, c[1], R3 ;\n"
      "DP4   o[HPOS].z, c[2], R3 ;\n"
      "DP4   o[HPOS].w, c[3], R3 ;\n"

      "MOV   o[COL0], v[COL0];\n       # copy input color to output color\n"

      "END";

   if (!glutExtensionSupported("GL_NV_vertex_program")) {
      printf("Sorry, this program requires GL_NV_vertex_program\n");
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

   /* Gravity strength */
   glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 31, .5, 0, 0, 0);

   glEnable(GL_DEPTH_TEST);
   glClearColor(0.3, 0.3, 0.3, 1);
   glShadeModel(GL_FLAT);
   glPointSize(3);
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
