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
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static char *FragProgFile = "CH18-mandel.frag";
static char *VertProgFile = "CH18-mandel.vert";

/* program/shader objects */
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;


static struct uniform_info Uniforms[] = {
   /* vert */
   { "LightPosition",        3, GL_FLOAT, { 0.1, 0.1, 9.0, 0}, -1 },
   { "SpecularContribution", 1, GL_FLOAT, { 0.5, 0, 0, 0 }, -1 },
   { "DiffuseContribution",  1, GL_FLOAT, { 0.5, 0, 0, 0 }, -1 },
   { "Shininess",            1, GL_FLOAT, { 20.0, 0, 0, 0 }, -1 },
   /* frag */
   { "MaxIterations",        1, GL_FLOAT, { 12, 0, 0, 0 }, -1 },
   { "Zoom",                 1, GL_FLOAT, { 0.125, 0, 0, 0 }, -1 },
   { "Xcenter",              1, GL_FLOAT, { -1.5, 0, 0, 0 }, -1 },
   { "Ycenter",              1, GL_FLOAT, { .005, 0, 0, 0 }, -1 },
   { "InnerColor",           3, GL_FLOAT, { 1, 0, 0, 0 }, -1 },
   { "OuterColor1",          3, GL_FLOAT, { 0, 1, 0, 0 }, -1 },
   { "OuterColor2",          3, GL_FLOAT, { 0, 0, 1, 0 }, -1 },
   END_OF_UNIFORMS
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
   glUniform1fv(uZoom, 1, &zoom);
   glUniform1fv(uXcenter, 1, &xCenter);
   glUniform1fv(uYcenter, 1, &yCenter);

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
   glDeleteShader(fragShader);
   glDeleteShader(vertShader);
   glDeleteProgram(program);
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
Init(void)
{
   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderFile(GL_VERTEX_SHADER, VertProgFile);
   fragShader = CompileShaderFile(GL_FRAGMENT_SHADER, FragProgFile);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   InitUniforms(program, Uniforms);

   uZoom = glGetUniformLocation(program, "Zoom");
   uXcenter = glGetUniformLocation(program, "Xcenter");
   uYcenter = glGetUniformLocation(program, "Ycenter");

   assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram(program));
   assert(glIsShader(fragShader));
   assert(glIsShader(vertShader));

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

