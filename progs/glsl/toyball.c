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
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static char *FragProgFile = "CH11-toyball.frag";
static char *VertProgFile = "CH11-toyball.vert";

/* program/shader objects */
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;


static struct uniform_info Uniforms[] = {
   { "LightDir",       4, GL_FLOAT, { 0.57737, 0.57735, 0.57735, 0.0 }, -1 },
   { "HVector",        4, GL_FLOAT, { 0.32506, 0.32506, 0.88808, 0.0 }, -1 },
   { "BallCenter",     4, GL_FLOAT, { 0.0, 0.0, 0.0, 1.0 }, -1 },
   { "SpecularColor",  4, GL_FLOAT, { 0.4, 0.4, 0.4, 60.0 }, -1 },
   { "Red",         4, GL_FLOAT, { 0.6, 0.0, 0.0, 1.0 }, -1 },
   { "Blue",        4, GL_FLOAT, { 0.0, 0.3, 0.6, 1.0 }, -1 },
   { "Yellow",      4, GL_FLOAT, { 0.6, 0.5, 0.0, 1.0 }, -1 },
   { "HalfSpace0",  4, GL_FLOAT, { 1.0, 0.0, 0.0, 0.2 }, -1 },
   { "HalfSpace1",  4, GL_FLOAT, { 0.309016994, 0.951056516, 0.0, 0.2 }, -1 },
   { "HalfSpace2",  4, GL_FLOAT, { -0.809016994, 0.587785252, 0.0, 0.2 }, -1 },
   { "HalfSpace3",  4, GL_FLOAT, { -0.809016994, -0.587785252, 0.0, 0.2 }, -1 },
   { "HalfSpace4",  4, GL_FLOAT, { 0.309116994, -0.951056516, 0.0, 0.2 }, -1 },
   { "InOrOutInit", 1, GL_FLOAT, { -3.0, 0, 0, 0 }, -1 },
   { "StripeWidth", 1, GL_FLOAT, {  0.3, 0, 0, 0 }, -1 },
   { "FWidth",      1, GL_FLOAT, { 0.005, 0, 0, 0 }, -1 },
   END_OF_UNIFORMS
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
   glDeleteShader(fragShader);
   glDeleteShader(vertShader);
   glDeleteProgram(program);
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
Init(void)
{
   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderFile(GL_VERTEX_SHADER, VertProgFile);
   fragShader = CompileShaderFile(GL_FRAGMENT_SHADER, FragProgFile);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   SetUniformValues(program, Uniforms);
   PrintUniforms(Uniforms);

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
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}

