/**
 * Just compile ARB vert/frag program from named file(s).
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static GLuint FragProg;
static GLuint VertProg;
static GLint Win;

static PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB_func;
static PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB_func;
static PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB_func;
static PFNGLGENPROGRAMSARBPROC glGenProgramsARB_func;
static PFNGLPROGRAMSTRINGARBPROC glProgramStringARB_func;
static PFNGLBINDPROGRAMARBPROC glBindProgramARB_func;
static PFNGLISPROGRAMARBPROC glIsProgramARB_func;
static PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB_func;


static void Redisplay( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   glutSwapBuffers();
   exit(0);
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
         glDeleteProgramsARB_func(1, &VertProg);
         glDeleteProgramsARB_func(1, &FragProg);
         glutDestroyWindow(Win);
         exit(0);
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


static void Init( const char *vertProgFile,
                  const char *fragProgFile )
{
   GLint errorPos;
   char buf[10*1000];

   if (!glutExtensionSupported("GL_ARB_vertex_program")) {
      printf("Sorry, this demo requires GL_ARB_vertex_program\n");
      exit(1);
   }
   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
      printf("Sorry, this demo requires GL_ARB_fragment_program\n");
      exit(1);
   }
         
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

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

   glDeleteProgramsARB_func = (PFNGLDELETEPROGRAMSARBPROC) glutGetProcAddress("glDeleteProgramsARB");
   assert(glDeleteProgramsARB_func);

   /*
    * Vertex program
    */
   if (vertProgFile) {
      FILE *f;
      int len;

      glGenProgramsARB_func(1, &VertProg);
      assert(VertProg > 0);
      glBindProgramARB_func(GL_VERTEX_PROGRAM_ARB, VertProg);

      f = fopen(vertProgFile, "r");
      if (!f) {
         printf("Unable to open %s\n", fragProgFile);
         exit(1);
      }

      len = fread(buf, 1, 10*1000,f);
      glProgramStringARB_func(GL_VERTEX_PROGRAM_ARB,
                              GL_PROGRAM_FORMAT_ASCII_ARB,
                              len,
                              (const GLubyte *) buf);

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      if (glGetError() != GL_NO_ERROR || errorPos != -1) {
         int l = FindLine(buf, errorPos);
         printf("Vertex Program Error (pos=%d line=%d): %s\n", errorPos, l,
                (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
         exit(0);
      }
      else {
         glEnable(GL_VERTEX_PROGRAM_ARB);
         printf("Vertex Program OK\n");
      }
   }

   /*
    * Fragment program
    */
   if (fragProgFile) {
      FILE *f;
      int len;

      glGenProgramsARB_func(1, &FragProg);
      assert(FragProg > 0);
      glBindProgramARB_func(GL_FRAGMENT_PROGRAM_ARB, FragProg);

      f = fopen(fragProgFile, "r");
      if (!f) {
         printf("Unable to open %s\n", fragProgFile);
         exit(1);
      }

      len = fread(buf, 1, 10*1000,f);
      glProgramStringARB_func(GL_FRAGMENT_PROGRAM_ARB,
                              GL_PROGRAM_FORMAT_ASCII_ARB,
                              len,
                              (const GLubyte *) buf);

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
      if (glGetError() != GL_NO_ERROR || errorPos != -1) {
         int l = FindLine(buf, errorPos);
         printf("Fragment Program Error (pos=%d line=%d): %s\n", errorPos, l,
                (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
         exit(0);
      }
      else {
         glEnable(GL_FRAGMENT_PROGRAM_ARB);
         printf("Fragment Program OK\n");
      }
   }
}


int main( int argc, char *argv[] )
{
   const char *vertProgFile = NULL, *fragProgFile = NULL;
   int i;

   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 200, 200 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Redisplay );

   if (argc == 1) {
      printf("arbgpuprog:\n");
      printf("  Compile GL_ARB_vertex/fragment_programs, report any errors.\n");
      printf("Usage:\n");
      printf("  arbgpuprog [--vp vertprogfile] [--fp fragprogfile]\n");
      exit(1);
   }

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--vp") == 0) {
         vertProgFile = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "--fp") == 0) {
         fragProgFile = argv[i+1];
         i++;
      }
   }

   Init(vertProgFile, fragProgFile);

   glutMainLoop();
   return 0;
}
