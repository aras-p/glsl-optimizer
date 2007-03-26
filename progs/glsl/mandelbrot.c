/**
 * "Mandelbrot" shader demo.  Uses the example shaders from
 * chapter 15 (or 18) of the OpenGL Shading Language "orange" book.
 * 15 Jan 2007
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


static char *FragProgFile = "CH18-mandel.frag.txt";
static char *VertProgFile = "CH18-mandel.vert.txt";

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
   /* vert */
   { "LightPosition",        3, -1, { 0.1, 0.1, 9.0, 0} },
   { "SpecularContribution", 1, -1, { 0.5, 0, 0, 0 } },
   { "DiffuseContribution",  1, -1, { 0.5, 0, 0, 0 } },
   { "Shininess",            1, -1, { 20.0, 0, 0, 0 } },
   /* frag */
   { "MaxIterations",        1, -1, { 12, 0, 0, 0 } },
   { "Zoom",                 1, -1, { 0.125, 0, 0, 0 } },
   { "Xcenter",              1, -1, { -1.5, 0, 0, 0 } },
   { "Ycenter",              1, -1, { .005, 0, 0, 0 } },
   { "InnerColor",           3, -1, { 1, 0, 0, 0 } },
   { "OuterColor1",          3, -1, { 0, 1, 0, 0 } },
   { "OuterColor2",          3, -1, { 0, 0, 1, 0 } },
   { NULL, 0, 0, { 0, 0, 0, 0 } }
};

static GLint win = 0;

static GLfloat xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;

static GLint uZoom, uXcenter, uYcenter;
static GLfloat zoom = 1.0, xCenter = -1.5, yCenter = 0.0;


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   /* set interactive uniform parameters */
   glUniform1fv_func(uZoom, 1, &zoom);
   glUniform1fv_func(uXcenter, 1, &xCenter);
   glUniform1fv_func(uYcenter, 1, &yCenter);

   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);

   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);   glVertex2f(-1, -1);
   glTexCoord2f(1, 0);   glVertex2f( 1, -1);
   glTexCoord2f(1, 1);   glVertex2f( 1,  1);
   glTexCoord2f(0, 1);   glVertex2f(-1,  1);
   glEnd();

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
   glTranslatef(0.0f, 0.0f, -6.0f);
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
   case 'z':
      zoom *= 0.9;
      break;
   case 'Z':
      zoom /= 0.9;
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
   const GLfloat step = 0.1 * zoom;

  (void) x;
  (void) y;

   switch(key) {
   case GLUT_KEY_UP:
      yCenter += step;
      break;
   case GLUT_KEY_DOWN:
      yCenter -= step;
      break;
   case GLUT_KEY_LEFT:
      xCenter -= step;
      break;
   case GLUT_KEY_RIGHT:
      xCenter += step;
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

   uZoom = glGetUniformLocation_func(program, "Zoom");
   uXcenter = glGetUniformLocation_func(program, "Xcenter");
   uYcenter = glGetUniformLocation_func(program, "Ycenter");

   assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);

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

