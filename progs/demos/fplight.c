/*
 * Use GL_NV_fragment_program to implement per-pixel lighting.
 *
 * Brian Paul
 * 7 April 2003
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>


static GLfloat Diffuse[4] = { 0.5, 0.5, 1.0, 1.0 };
static GLfloat Specular[4] = { 0.8, 0.8, 0.8, 1.0 };
static GLfloat LightPos[4] = { 0.0, 10.0, 20.0, 1.0 };
static GLfloat Delta = 1.0;

static GLuint FragProg;
static GLuint VertProg;
static GLboolean Anim = GL_TRUE;
static GLboolean Wire = GL_FALSE;
static GLboolean PixelLight = GL_TRUE;
static GLint Win;
static GLfloat Xrot = 0, Yrot = 0;


#define NAMED_PARAMETER4FV(prog, name, v)        \
  glProgramNamedParameter4fvNV(prog, strlen(name), (const GLubyte *) name, v)


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   if (PixelLight) {
#if defined(GL_NV_fragment_program)
      NAMED_PARAMETER4FV(FragProg, "LightPos", LightPos);
      glEnable(GL_FRAGMENT_PROGRAM_NV);
      glEnable(GL_VERTEX_PROGRAM_NV);
#endif
      glDisable(GL_LIGHTING);
   }
   else {
      glLightfv(GL_LIGHT0, GL_POSITION, LightPos);
#if defined(GL_NV_fragment_program)
      glDisable(GL_FRAGMENT_PROGRAM_NV);
      glDisable(GL_VERTEX_PROGRAM_NV);
#endif
      glEnable(GL_LIGHTING);
   }

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);

#if 1
   glutSolidSphere(2.0, 10, 5);
#else
   {
      GLUquadricObj *q = gluNewQuadric();
      gluQuadricNormals(q, GL_SMOOTH);
      gluQuadricTexture(q, GL_TRUE);
      glRotatef(90, 1, 0, 0);
      glTranslatef(0, 0, -1);
      gluCylinder(q, 1.0, 1.0, 2.0, 24, 1);
      gluDeleteQuadric(q);
   }
#endif

   glPopMatrix();

   glutSwapBuffers();
}


static void Idle(void)
{
   LightPos[0] += Delta;
   if (LightPos[0] > 25.0)
      Delta = -1.0;
   else if (LightPos[0] <- 25.0)
      Delta = 1.0;
   glutPostRedisplay();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   /*glOrtho( -2.0, 2.0, -2.0, 2.0, 5.0, 25.0 );*/
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
     case ' ':
        Anim = !Anim;
        if (Anim)
           glutIdleFunc(Idle);
        else
           glutIdleFunc(NULL);
        break;
      case 'x':
         LightPos[0] -= 1.0;
         break;
      case 'X':
         LightPos[0] += 1.0;
         break;
      case 'w':
         Wire = !Wire;
         if (Wire)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
         else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
         break;
      case 'p':
         PixelLight = !PixelLight;
         if (PixelLight) {
            printf("Per-pixel lighting\n");
         }
         else {
            printf("Conventional lighting\n");
         }
         break;
      case 27:
         glutDestroyWindow(Win);
         exit(0);
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
   static const char *fragProgramText =
      "!!FP1.0\n"
      "DECLARE Diffuse; \n"
      "DECLARE Specular; \n"
      "DECLARE LightPos; \n"

      "# Compute normalized LightPos, put it in R0\n"
      "DP3 R0.x, LightPos, LightPos;\n"
      "RSQ R0.y, R0.x;\n"
      "MUL R0, LightPos, R0.y;\n"

      "# Compute normalized normal, put it in R1\n"
      "DP3 R1, f[TEX0], f[TEX0]; \n"
      "RSQ R1.y, R1.x;\n"
      "MUL R1, f[TEX0], R1.y;\n"

      "# Compute dot product of light direction and normal vector\n"
      "DP3_SAT R2, R0, R1;"

      "MUL R3, Diffuse, R2;    # diffuse attenuation\n"

      "POW R4, R2.x, {20.0}.x; # specular exponent\n"

      "MUL R5, Specular, R4;   # specular attenuation\n"

      "ADD o[COLR], R3, R5;    # add diffuse and specular colors\n"
      "END \n"
      ;

   static const char *vertProgramText =
      "!!VP1.0\n"
      "# typical modelview/projection transform\n"
      "DP4   o[HPOS].x, c[0], v[OPOS] ;\n"
      "DP4   o[HPOS].y, c[1], v[OPOS] ;\n"
      "DP4   o[HPOS].z, c[2], v[OPOS] ;\n"
      "DP4   o[HPOS].w, c[3], v[OPOS] ;\n"
      "# transform normal by inv transpose of modelview, put in tex0\n"
      "DP3   o[TEX0].x, c[4], v[NRML] ;\n"
      "DP3   o[TEX0].y, c[5], v[NRML] ;\n"
      "DP3   o[TEX0].z, c[6], v[NRML] ;\n"
      "DP3   o[TEX0].w, c[7], v[NRML] ;\n"
      "END\n";
   ;

   if (!glutExtensionSupported("GL_NV_vertex_program")) {
      printf("Sorry, this demo requires GL_NV_vertex_program\n");
      exit(1);
   }
   if (!glutExtensionSupported("GL_NV_fragment_program")) {
      printf("Sorry, this demo requires GL_NV_fragment_program\n");
      exit(1);
   }
         
#if defined(GL_NV_fragment_program)
   glGenProgramsNV(1, &FragProg);
   assert(FragProg > 0);
   glGenProgramsNV(1, &VertProg);
   assert(VertProg > 0);

   /*
    * Fragment program
    */
   glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV, FragProg,
                   strlen(fragProgramText),
                   (const GLubyte *) fragProgramText);
   assert(glIsProgramNV(FragProg));
   glBindProgramNV(GL_FRAGMENT_PROGRAM_NV, FragProg);

   NAMED_PARAMETER4FV(FragProg, "Diffuse", Diffuse);
   NAMED_PARAMETER4FV(FragProg, "Specular", Specular);

   /*
    * Vertex program
    */
   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, VertProg,
                   strlen(vertProgramText),
                   (const GLubyte *) vertProgramText);
   assert(glIsProgramNV(VertProg));
   glBindProgramNV(GL_VERTEX_PROGRAM_NV, VertProg);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4, GL_MODELVIEW, GL_INVERSE_TRANSPOSE_NV);
#endif

   /*
    * Misc init
    */
   glClearColor(0.3, 0.3, 0.3, 0.0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHTING);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Diffuse);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Specular);
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0);

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("Press p to toggle between per-pixel and per-vertex lighting\n");
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 200, 200 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Win = glutCreateWindow(argv[0]);
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
