/**
 * Test variable array indexing in a vertex shader.
 * Brian Paul
 * 17 April 2009
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
#include "shaderutil.h"


/**
 * The vertex position.z is used as a (variable) index into an
 * array which returns a new Z value.
 */
static const char *VertShaderText =
   "uniform sampler2D tex1; \n"
   "uniform float HeightArray[20]; \n"
   "void main() \n"
   "{ \n"
   "   vec4 pos = gl_Vertex; \n"
   "   int i = int(pos.z * 9.5); \n"
   "   pos.z = HeightArray[i]; \n"
   "   gl_Position = gl_ModelViewProjectionMatrix * pos; \n"
   "   gl_FrontColor = pos; \n"
   "} \n";

static const char *FragShaderText =
   "void main() \n"
   "{ \n"
   "   gl_FragColor = gl_Color; \n"
   "} \n";


static GLuint fragShader;
static GLuint vertShader;
static GLuint program;

static GLint win = 0;
static GLboolean Anim = GL_TRUE;
static GLboolean WireFrame = GL_TRUE;
static GLfloat xRot = -70.0f, yRot = 0.0f, zRot = 0.0f;


static void
Idle(void)
{
   zRot = 90 + glutGet(GLUT_ELAPSED_TIME) * 0.05;
   glutPostRedisplay();
}


/** z=f(x,y) */
static float
fz(float x, float y)
{
   return fabs(cos(1.5*x) + cos(1.5*y));
}


static void
DrawMesh(void)
{
   GLfloat xmin = -2.0, xmax = 2.0;
   GLfloat ymin = -2.0, ymax = 2.0;
   GLuint xdivs = 20, ydivs = 20;
   GLfloat dx = (xmax - xmin) / xdivs;
   GLfloat dy = (ymax - ymin) / ydivs;
   GLfloat ds = 1.0 / xdivs, dt = 1.0 / ydivs;
   GLfloat x, y, s, t;
   GLuint i, j;

   y = ymin;
   t = 0.0;
   for (i = 0; i < ydivs; i++) {
      x = xmin;
      s = 0.0;
      glBegin(GL_QUAD_STRIP);
      for (j = 0; j < xdivs; j++) {
         float z0 = fz(x, y), z1 = fz(x, y + dy);

         glTexCoord2f(s, t);
         glVertex3f(x, y, z0);

         glTexCoord2f(s, t + dt);
         glVertex3f(x, y + dy, z1);
         x += dx;
         s += ds;
      }
      glEnd();
      y += dy;
      t += dt;
   }
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   if (WireFrame)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);

   glPushMatrix();
   DrawMesh();
   glPopMatrix();

   glPopMatrix();

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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
   GLfloat HeightArray[20];
   GLint u, i;

   if (!ShadersSupported())
      exit(1);

   GetExtensionFuncs();

   vertShader = CompileShaderText(GL_VERTEX_SHADER, VertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, FragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram_func(program);

   /* Setup the HeightArray[] uniform */
   for (i = 0; i < 20; i++)
      HeightArray[i] = i / 20.0;
   u = glGetUniformLocation_func(program, "HeightArray");
   glUniform1fv_func(u, 20, HeightArray);

   assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);
   glEnable(GL_DEPTH_TEST);
   glColor3f(1, 1, 1);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(500, 500);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   Init();
   if (Anim)
      glutIdleFunc(Idle);
   glutMainLoop();
   return 0;
}

