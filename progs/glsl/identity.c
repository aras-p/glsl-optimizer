/**
 * Test very basic glsl functionality (identity vertex and fragment shaders).
 * Brian Paul
 * 2 May 2007
 *
 * NOTE: resize the window to observe how the partial derivatives of
 * the texcoords change.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include "extfuncs.h"


static char *FragProgFile = NULL;
static char *VertProgFile = NULL;
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;
static GLint win = 0;
static GLboolean anim = GL_TRUE;
static GLfloat xRot = 0.0f, yRot = 0.0f;
static int w,h;

static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBegin(GL_TRIANGLES);
   glColor3f(.8,0,0); 
   glVertex3f(-0.9, -0.9, 0.0);
   glColor3f(0,.9,0); 
   glVertex3f( 0.9, -0.9, 0.0);
   glColor3f(0,0,.7); 
   glVertex3f( 0.0,  0.9, 0.0);
   glEnd();

   glutSwapBuffers();
}


static void
Idle(void)
{
   yRot = glutGet(GLUT_ELAPSED_TIME) * 0.1;
   glutPostRedisplay();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   w = width;
   h = height;
}


static void
CleanUp(void)
{
   glDeleteShader_func(fragShader);
   glDeleteShader_func(vertShader);
   glDeleteProgram_func(program);
   glutDestroyWindow(win);
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;

   switch(key) {
   case ' ':
   case 'a':
      anim = !anim;
      if (anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0f;

  (void) x;
  (void) y;

   switch(key) {
   case GLUT_KEY_UP:
      xRot -= step;
      break;
   case GLUT_KEY_DOWN:
      xRot += step;
      break;
   case GLUT_KEY_LEFT:
      yRot -= step;
      break;
   case GLUT_KEY_RIGHT:
      yRot += step;
      break;
   }
   glutPostRedisplay();
}




static void
LoadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;

   glShaderSource_func(shader, 1, (const GLchar **) &text, NULL);

   glCompileShader_func(shader);

   glGetShaderiv_func(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog_func(shader, 1000, &len, log);
      fprintf(stderr, "fslight: problem compiling shader:\n%s\n", log);
      exit(1);
   }
}


/**
 * Read a shader from a file.
 */
static void
ReadShader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "fslight: Unable to open shader file %s\n", filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("fslight: read %d bytes from shader file %s\n", n, filename);
   if (n > 0) {
      buffer[n] = 0;
      LoadAndCompileShader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}


static void
CheckLink(GLuint prog)
{
   GLint stat;
   glGetProgramiv_func(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog_func(prog, 1000, &len, log);
      fprintf(stderr, "Linker error:\n%s\n", log);
   }
}


static void
Init(void)
{
   static const char *fragShaderText =
      "void main() {\n"
      "   gl_FragColor = vec4(1.0,0.0,0.0,1.0);\n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = gl_Vertex;\n"
      "}\n";
   const char *version;

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] != '2' || version[1] != '.') {
      printf("This program requires OpenGL 2.x, found %s\n", version);
      exit(1);
   }

   GetExtensionFuncs();

   fragShader = glCreateShader_func(GL_FRAGMENT_SHADER);
   if (FragProgFile)
      ReadShader(fragShader, FragProgFile);
   else
      LoadAndCompileShader(fragShader, fragShaderText);

   vertShader = glCreateShader_func(GL_VERTEX_SHADER);
   if (VertProgFile)
      ReadShader(vertShader, VertProgFile);
   else
      LoadAndCompileShader(vertShader, vertShaderText);

   program = glCreateProgram_func();
   glAttachShader_func(program, fragShader);
   glAttachShader_func(program, vertShader);
   glLinkProgram_func(program);
   CheckLink(program);
   glUseProgram_func(program);

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));

   glColor3f(1, 0, 0);
}


static void
ParseOptions(int argc, char *argv[])
{
   int i;
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-fs") == 0) {
         FragProgFile = argv[i+1];
      }
      else if (strcmp(argv[i], "-vs") == 0) {
         VertProgFile = argv[i+1];
      }
   }
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition( 0, 0);
   glutInitWindowSize(200, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   if (anim)
      glutIdleFunc(Idle);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}
