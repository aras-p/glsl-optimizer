/**
 * "Toy Ball" shader demo.  Uses the example shaders from
 * chapter 11 of the OpenGL Shading Language "orange" book.
 * 16 Jan 2007
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


static char *FragProgFile = "CH11-toyball.frag.txt";
static char *VertProgFile = "CH11-toyball.vert.txt";

/* program/shader objects */
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;


struct uniform_info {
   const char *name;
   GLuint size;
   GLint location;
   GLfloat value[4];
};

static struct uniform_info Uniforms[] = {
   { "LightDir",       4, -1, { 0.57737, 0.57735, 0.57735, 0.0 } },
   { "HVector",        4, -1, { 0.32506, 0.32506, 0.88808, 0.0 } },
   { "BallCenter",     4, -1, { 0.0, 0.0, 0.0, 1.0 } },
   { "SpecularColor",  4, -1, { 0.4, 0.4, 0.4, 60.0 } },
   { "Red",            4, -1, { 0.6, 0.0, 0.0, 1.0 } },
   { "Blue",           4, -1, { 0.0, 0.3, 0.6, 1.0 } },
   { "Yellow",         4, -1, { 0.6, 0.5, 0.0, 1.0 } },
   { "HalfSpace0",     4, -1, { 1.0, 0.0, 0.0, 0.2 } },
   { "HalfSpace1",     4, -1, { 0.309016994, 0.951056516, 0.0, 0.2 } },
   { "HalfSpace2",     4, -1, { -0.809016994, 0.587785252, 0.0, 0.2 } },
   { "HalfSpace3",     4, -1, { -0.809016994, -0.587785252, 0.0, 0.2 } },
   { "HalfSpace4",     4, -1, { 0.309116994, -0.951056516, 0.0, 0.2 } },
   { "InOrOutInit",    1, -1, { -3.0, 0, 0, 0 } },
   { "StripeWidth",    1, -1, {  0.3, 0, 0, 0 } },
   { "FWidth",         1, -1, { 0.005, 0, 0, 0 } },
   { NULL, 0, 0, { 0, 0, 0, 0 } }
};

static GLint win = 0;
static GLboolean Anim = GL_FALSE;
static GLfloat TexRot = 0.0;
static GLfloat xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;


static void
Idle(void)
{
   TexRot += 2.0;
   if (TexRot > 360.0)
      TexRot -= 360.0;
   glutPostRedisplay();
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glRotatef(TexRot, 0.0f, 1.0f, 0.0f);
   glMatrixMode(GL_MODELVIEW);

   glutSolidSphere(2.0, 20, 10);

   glPopMatrix();

   glFinish();
   glFlush();
   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0f, 0.0f, -15.0f);
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
   const GLfloat step = 2.0;
  (void) x;
  (void) y;

   switch(key) {
   case 'a':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'z':
      zRot += step;
      break;
   case 'Z':
      zRot -= step;
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
   const GLfloat step = 2.0;

  (void) x;
  (void) y;

   switch(key) {
   case GLUT_KEY_UP:
      xRot += step;
      break;
   case GLUT_KEY_DOWN:
      xRot -= step;
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
      fprintf(stderr, "brick: problem compiling shader: %s\n", log);
      exit(1);
   }
   else {
      printf("Shader compiled OK\n");
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
      fprintf(stderr, "brick: Unable to open shader file %s\n", filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("brick: read %d bytes from shader file %s\n", n, filename);
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
   else {
      fprintf(stderr, "Link success!\n");
   }
}


static void
Init(void)
{
   const char *version;
   GLint i;

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] != '2' || version[1] != '.') {
      printf("Warning: this program expects OpenGL 2.0\n");
      /*exit(1);*/
   }
   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   GetExtensionFuncs();

   vertShader = glCreateShader_func(GL_VERTEX_SHADER);
   ReadShader(vertShader, VertProgFile);

   fragShader = glCreateShader_func(GL_FRAGMENT_SHADER);
   ReadShader(fragShader, FragProgFile);

   program = glCreateProgram_func();
   glAttachShader_func(program, fragShader);
   glAttachShader_func(program, vertShader);
   glLinkProgram_func(program);
   CheckLink(program);
   glUseProgram_func(program);

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));


   for (i = 0; Uniforms[i].name; i++) {
      Uniforms[i].location
         = glGetUniformLocation_func(program, Uniforms[i].name);
      printf("Uniform %s location: %d\n", Uniforms[i].name,
             Uniforms[i].location);
      switch (Uniforms[i].size) {
      case 1:
         glUniform1fv_func(Uniforms[i].location, 1, Uniforms[i].value);
         break;
      case 2:
         glUniform2fv_func(Uniforms[i].location, 1, Uniforms[i].value);
         break;
      case 3:
         glUniform3fv_func(Uniforms[i].location, 1, Uniforms[i].value);
         break;
      case 4:
         glUniform4fv_func(Uniforms[i].location, 1, Uniforms[i].value);
         break;
      default:
         abort();
      }
   }

   assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);

   glEnable(GL_DEPTH_TEST);

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
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}

