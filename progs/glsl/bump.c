/**
 * Procedural Bump Mapping demo.  Uses the example shaders from
 * chapter 11 of the OpenGL Shading Language "orange" book.
 * 16 Jan 2007
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "extfuncs.h"


static char *FragProgFile = "CH11-bumpmap.frag.txt";
static char *VertProgFile = "CH11-bumpmap.vert.txt";

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
   { "LightPosition",       3, -1, { 0.57737, 0.57735, 0.57735, 0.0 } },
   { "SurfaceColor",        3, -1, { 0.8, 0.8, 0.2, 0 } },
   { "BumpDensity",         1, -1, { 10.0, 0, 0, 0 } },
   { "BumpSize",            1, -1, { 0.125, 0, 0, 0 } },
   { "SpecularFactor",      1, -1, { 0.5, 0, 0, 0 } },
   { NULL, 0, 0, { 0, 0, 0, 0 } }
};

static GLint win = 0;

static GLfloat xRot = 20.0f, yRot = 0.0f, zRot = 0.0f;

static GLuint tangentAttrib;

static GLboolean Anim = GL_FALSE;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error %s (0x%x) at line %d\n",
             gluErrorString(err), (int) err, line);
   }
}

/*
 * Draw a square, specifying normal and tangent vectors.
 */
static void
Square(GLfloat size)
{
   glNormal3f(0, 0, 1);
   glVertexAttrib3f_func(tangentAttrib, 1, 0, 0);
   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);  glVertex2f(-size, -size);
   glTexCoord2f(1, 0);  glVertex2f( size, -size);
   glTexCoord2f(1, 1);  glVertex2f( size,  size);
   glTexCoord2f(0, 1);  glVertex2f(-size,  size);
   glEnd();
}


static void
Cube(GLfloat size)
{
   /* +X */
   glPushMatrix();
   glRotatef(90, 0, 1, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* -X */
   glPushMatrix();
   glRotatef(-90, 0, 1, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* +Y */
   glPushMatrix();
   glRotatef(90, 1, 0, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* -Y */
   glPushMatrix();
   glRotatef(-90, 1, 0, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();


   /* +Z */
   glPushMatrix();
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* -Z */
   glPushMatrix();
   glRotatef(180, 0, 1, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

}


static void
Idle(void)
{
   GLint t = glutGet(GLUT_ELAPSED_TIME);
   yRot  = t * 0.05;
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

   Cube(1.5);

   glPopMatrix();

   glFinish();
   glFlush();

   CheckError(__LINE__);

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
      glutIdleFunc(Anim ? Idle : NULL);
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

   assert(glGetError() == 0);

   CheckError(__LINE__);

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

   CheckError(__LINE__);

   tangentAttrib = glGetAttribLocation_func(program, "Tangent");
   printf("Tangent Attrib: %d\n", tangentAttrib);

   assert(tangentAttrib >= 0);

   CheckError(__LINE__);

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

