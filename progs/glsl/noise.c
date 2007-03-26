/**
 * Test noise() functions.
 * 28 Jan 2007
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


static const char *VertShaderText =
   "void main() {\n"
   "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
   "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
   "}\n";

static const char *FragShaderText =
   "uniform vec4 Scale, Bias;\n"
   "uniform float Slice;\n"
   "void main()\n"
   "{\n"
   "   vec4 scale = vec4(5.0);\n"
   "   vec4 p;\n"
   "   p.xy = gl_TexCoord[0].xy;\n"
   "   p.z = Slice;\n"
   "   vec4 n = noise4(p * scale);\n"
   "   gl_FragColor = n * Scale + Bias;\n"
   "}\n";


struct uniform_info {
   const char *name;
   GLuint size;
   GLint location;
   GLfloat value[4];
};

static struct uniform_info Uniforms[] = {
   { "Scale",          4, -1, { 0.5, 0.4, 0.0, 0} },
   { "Bias",           4, -1, { 0.5, 0.3, 0.0, 0} },
   { "Slice",          1, -1, { 0.5, 0, 0, 0} },
   { NULL, 0, 0, { 0, 0, 0, 0 } }
};

/* program/shader objects */
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;

static GLint win = 0;
static GLfloat xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;
static GLfloat Slice = 0.0;
static GLboolean Anim = GL_FALSE;


static void
Idle(void)
{
   Slice += 0.01;
   glutPostRedisplay();
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   glUniform1fv_func(Uniforms[2].location, 1, &Slice);

   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);

   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);   glVertex2f(-2, -2);
   glTexCoord2f(1, 0);   glVertex2f( 2, -2);
   glTexCoord2f(1, 1);   glVertex2f( 2,  2);
   glTexCoord2f(0, 1);   glVertex2f(-2,  2);
   glEnd();

   glPopMatrix();

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
   const GLfloat step = 0.01;
  (void) x;
  (void) y;

   switch(key) {
   case 'a':
      Anim = !Anim;
      glutIdleFunc(Anim ? Idle : NULL);
   case 's':
      Slice -= step;
      break;
   case 'S':
      Slice += step;
      break;
   case 'z':
      zRot -= 1.0;
      break;
   case 'Z':
      zRot += 1.0;
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
      fprintf(stderr, "brick: problem compiling shader: %s\n", log);
      exit(1);
   }
   else {
      printf("Shader compiled OK\n");
   }
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
   LoadAndCompileShader(vertShader, VertShaderText);

   fragShader = glCreateShader_func(GL_FRAGMENT_SHADER);
   LoadAndCompileShader(fragShader, FragShaderText);

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

   assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));

   glColor3f(1, 0, 0);
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
   Init();
   glutMainLoop();
   return 0;
}

