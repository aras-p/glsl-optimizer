/*
 * Use GL_ARB_fragment_program and GL_ARB_vertex_program to implement
 * simple per-pixel lighting.
 *
 * Brian Paul
 * 17 April 2003
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

static GLfloat Xrot = 0, Yrot = 0;

static PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB_func;
static PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB_func;
static PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB_func;
static PFNGLGENPROGRAMSARBPROC glGenProgramsARB_func;
static PFNGLPROGRAMSTRINGARBPROC glProgramStringARB_func;
static PFNGLBINDPROGRAMARBPROC glBindProgramARB_func;
static PFNGLISPROGRAMARBPROC glIsProgramARB_func;

/* These must match the indexes used in the fragment program */
#define DIFFUSE 1
#define SPECULAR 2
#define LIGHTPOS 3



static void Redisplay( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   if (PixelLight) {
      glProgramLocalParameter4fvARB_func(GL_FRAGMENT_PROGRAM_ARB,
                                         LIGHTPOS, LightPos);
      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      glEnable(GL_VERTEX_PROGRAM_ARB);
      glDisable(GL_LIGHTING);
   }
   else {
      glLightfv(GL_LIGHT0, GL_POSITION, LightPos);
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
      glDisable(GL_VERTEX_PROGRAM_ARB);
      glEnable(GL_LIGHTING);
   }

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glutSolidSphere(2.0, 10, 5);
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


/* A helper for finding errors in program strings */
static int FindLine( const char *program, int position )
{
   int i, line = 1;
   for (i = 0; i < position; i++) {
      if (program[i] == '\n')
         line++;
   }
   return line;
}


static void Init( void )
{
   GLint errorPos;

   /* Yes, this could be expressed more efficiently */
   static const char *fragProgramText =
      "!!ARBfp1.0\n"
      "PARAM Diffuse = program.local[1]; \n"
      "PARAM Specular = program.local[2]; \n"
      "PARAM LightPos = program.local[3]; \n"
      "TEMP lightDir, normal, len; \n"
      "TEMP dotProd, specAtten; \n"
      "TEMP diffuseColor, specularColor; \n"

      "# Compute normalized light direction \n"
      "DP3 len.x, LightPos, LightPos; \n"
      "RSQ len.y, len.x; \n"
      "MUL lightDir, LightPos, len.y; \n"

      "# Compute normalized normal \n"
      "DP3 len.x, fragment.texcoord[0], fragment.texcoord[0]; \n"
      "RSQ len.y, len.x; \n"
      "MUL normal, fragment.texcoord[0], len.y; \n"

      "# Compute dot product of light direction and normal vector\n"
      "DP3 dotProd, lightDir, normal;"

      "MUL diffuseColor, Diffuse, dotProd;            # diffuse attenuation\n"

      "POW specAtten.x, dotProd.x, {20.0}.x;          # specular exponent\n"

      "MUL specularColor, Specular, specAtten.x;      # specular attenuation\n"

      "ADD result.color, diffuseColor, specularColor; # add colors\n"
      "END \n"
      ;

   static const char *vertProgramText =
      "!!ARBvp1.0\n"
      "ATTRIB pos = vertex.position; \n"
      "ATTRIB norm = vertex.normal; \n"
      "PARAM modelviewProj[4] = { state.matrix.mvp }; \n"
      "PARAM invModelview[4] = { state.matrix.modelview.invtrans }; \n"

      "# typical modelview/projection transform \n"
      "DP4 result.position.x, pos, modelviewProj[0]; \n"
      "DP4 result.position.y, pos, modelviewProj[1]; \n"
      "DP4 result.position.z, pos, modelviewProj[2]; \n"
      "DP4 result.position.w, pos, modelviewProj[3]; \n"

      "# transform normal by inv transpose of modelview, put in tex0 \n"
      "DP4 result.texcoord[0].x, norm, invModelview[0]; \n"
      "DP4 result.texcoord[0].y, norm, invModelview[1]; \n"
      "DP4 result.texcoord[0].z, norm, invModelview[2]; \n"
      "DP4 result.texcoord[0].w, norm, invModelview[3]; \n"

      "END\n";
      ;

   if (!glutExtensionSupported("GL_ARB_vertex_program")) {
      printf("Sorry, this demo requires GL_ARB_vertex_program\n");
      exit(1);
   }
   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
      printf("Sorry, this demo requires GL_ARB_fragment_program\n");
      exit(1);
   }
         
   /*
    * Get extension function pointers.
    */
   glProgramLocalParameter4fvARB_func = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) glutGetProcAddress("glProgramLocalParameter4fvARB");
   assert(glProgramLocalParameter4fvARB_func);

   glProgramLocalParameter4dARB_func = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC) glutGetProcAddress("glProgramLocalParameter4dARB");
   assert(glProgramLocalParameter4dARB_func);

   glGetProgramLocalParameterdvARB_func = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) glutGetProcAddress("glGetProgramLocalParameterdvARB");
   assert(glGetProgramLocalParameterdvARB_func);

   glGenProgramsARB_func = (PFNGLGENPROGRAMSARBPROC) glutGetProcAddress("glGenProgramsARB");
   assert(glGenProgramsARB_func);

   glProgramStringARB_func = (PFNGLPROGRAMSTRINGARBPROC) glutGetProcAddress("glProgramStringARB");
   assert(glProgramStringARB_func);

   glBindProgramARB_func = (PFNGLBINDPROGRAMARBPROC) glutGetProcAddress("glBindProgramARB");
   assert(glBindProgramARB_func);

   glIsProgramARB_func = (PFNGLISPROGRAMARBPROC) glutGetProcAddress("glIsProgramARB");
   assert(glIsProgramARB_func);

   /*
    * Fragment program
    */
   glGenProgramsARB_func(1, &FragProg);
   assert(FragProg > 0);
   glBindProgramARB_func(GL_FRAGMENT_PROGRAM_ARB, FragProg);
   glProgramStringARB_func(GL_FRAGMENT_PROGRAM_ARB,
                           GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(fragProgramText),
                           (const GLubyte *) fragProgramText);
   glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
   if (glGetError() != GL_NO_ERROR || errorPos != -1) {
      int l = FindLine(fragProgramText, errorPos);
      printf("Fragment Program Error (pos=%d line=%d): %s\n", errorPos, l,
             (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
      exit(0);
   }
   assert(glIsProgramARB_func(FragProg));

   glProgramLocalParameter4fvARB_func(GL_FRAGMENT_PROGRAM_ARB, DIFFUSE, Diffuse);
   glProgramLocalParameter4fvARB_func(GL_FRAGMENT_PROGRAM_ARB, SPECULAR, Specular);

   /*
    * Do some sanity tests
    */
   {
      GLdouble v[4];
      glProgramLocalParameter4dARB_func(GL_FRAGMENT_PROGRAM_ARB, 8,
                                   10.0, 20.0, 30.0, 40.0);
      glGetProgramLocalParameterdvARB_func(GL_FRAGMENT_PROGRAM_ARB, 8, v);
      assert(v[0] == 10.0);
      assert(v[1] == 20.0);
      assert(v[2] == 30.0);
      assert(v[3] == 40.0);
      glGetProgramLocalParameterdvARB_func(GL_FRAGMENT_PROGRAM_ARB, DIFFUSE, v);
      assert(v[0] == Diffuse[0]);
      assert(v[1] == Diffuse[1]);
      assert(v[2] == Diffuse[2]);
      assert(v[3] == Diffuse[3]);
   }

   /*
    * Vertex program
    */
   glGenProgramsARB_func(1, &VertProg);
   assert(VertProg > 0);
   glBindProgramARB_func(GL_VERTEX_PROGRAM_ARB, VertProg);
   glProgramStringARB_func(GL_VERTEX_PROGRAM_ARB,
                           GL_PROGRAM_FORMAT_ASCII_ARB,
                           strlen(vertProgramText),
                           (const GLubyte *) vertProgramText);
   glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
   if (glGetError() != GL_NO_ERROR || errorPos != -1) {
      int l = FindLine(fragProgramText, errorPos);
      printf("Vertex Program Error (pos=%d line=%d): %s\n", errorPos, l,
             (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
      exit(0);
   }
   assert(glIsProgramARB_func(VertProg));

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
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Redisplay );
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
