/**
 * Vertex "skinning" example.
 * The idea is there are multiple modeling matrices applied to every
 * vertex.  Weighting values in [0,1] control the influence of each
 * matrix on each vertex.
 *
 * 4 Nov 2008
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static char *FragProgFile = "skinning.frag";
static char *VertProgFile = "skinning.vert";

/* program/shader objects */
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;


static GLint win = 0;
static GLboolean Anim = GL_TRUE;
static GLboolean WireFrame = GL_TRUE;
static GLfloat xRot = 0.0f, yRot = 90.0f, zRot = 0.0f;

#define NUM_MATS 2

static GLfloat Matrices[NUM_MATS][16];
static GLint uMat0, uMat1;
static GLint WeightAttr;


static void
Idle(void)
{
   yRot = 90 + glutGet(GLUT_ELAPSED_TIME) * 0.005;
   glutPostRedisplay();
}


static void
Cylinder(GLfloat length, GLfloat radius, GLint slices, GLint stacks)
{
   float dw = 1.0 / (stacks - 1);
   float dz = length / stacks;
   int i, j;

   for (j = 0; j < stacks; j++) {
      float w0 = j * dw;
      float z0 = j * dz;

      glBegin(GL_TRIANGLE_STRIP);
      for (i = 0; i < slices; i++) {
         float a = (float) i / (slices - 1) * M_PI * 2.0;
         float x = radius * cos(a);
         float y = radius * sin(a);
         glVertexAttrib1f(WeightAttr, w0);
         glNormal3f(x, y, 0.0);
         glVertex3f(x, y, z0);

         glVertexAttrib1f(WeightAttr, w0 + dw);
         glNormal3f(x, y, 0.0);
         glVertex3f(x, y, z0 + dz);
      }
      glEnd();
   }
}


/**
 * Update/animate the two matrices.  One rotates, the other scales.
 */
static void
UpdateMatrices(void)
{
   GLfloat t = glutGet(GLUT_ELAPSED_TIME) * 0.0025;
   GLfloat scale = 0.5 * (1.1 + sin(0.5 * t));
   GLfloat rot = cos(t) * 90.0;

   glPushMatrix();
   glLoadIdentity();
   glScalef(1.0, scale, 1.0);
   glGetFloatv(GL_MODELVIEW_MATRIX, Matrices[0]);
   glPopMatrix();

   glPushMatrix();
   glLoadIdentity();
   glRotatef(rot, 0, 0, 1);
   glGetFloatv(GL_MODELVIEW_MATRIX, Matrices[1]);
   glPopMatrix();
}


static void
Redisplay(void)
{
   UpdateMatrices();

   glUniformMatrix4fv(uMat0, 1, GL_FALSE, Matrices[0]);
   glUniformMatrix4fv(uMat1, 1, GL_FALSE, Matrices[1]);

   if (WireFrame)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);

   glPushMatrix();
   glTranslatef(0, 0, -2.5);
   Cylinder(5.0, 1.0, 10, 20);
   glPopMatrix();

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
   case 'w':
      WireFrame = !WireFrame;
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

   uMat0 = glGetUniformLocation(program, "mat0");
   uMat1 = glGetUniformLocation(program, "mat1");

   WeightAttr = glGetAttribLocation(program, "weight");

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
   glutInitWindowSize(500, 500);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   if (Anim)
      glutIdleFunc(Idle);
   glutMainLoop();
   return 0;
}

