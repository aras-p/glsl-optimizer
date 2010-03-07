/**
 * Vertex shader texture sampling test.
 * Brian Paul
 * 2 Dec 2008
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static const char *VertShaderText =
   "uniform sampler2D tex1; \n"
   "void main() \n"
   "{ \n"
   "   vec4 pos = gl_Vertex; \n"
   "   pos.z = texture2D(tex1, gl_MultiTexCoord0.xy).x - 0.5; \n"
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
         glTexCoord2f(s, t);
         glVertex2f(x, y);
         glTexCoord2f(s, t + dt);
         glVertex2f(x, y + dy);
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
   DrawMesh();
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
MakeTexture(void)
{
   const GLuint texWidth = 64, texHeight = 64;
   GLfloat texImage[64][64];
   GLuint i, j;

   /* texture is basically z = f(x, y) */
   for (i = 0; i < texHeight; i++) {
      GLfloat y = 2.0 * (i / (float) (texHeight - 1)) - 1.0;
      for (j = 0; j < texWidth; j++) {
         GLfloat x = 2.0 * (j / (float) (texWidth - 1)) - 1.0;
         GLfloat z = 0.5 + 0.5 * (sin(4.0 * x) * sin(4.0 * y));
         texImage[i][j] = z;
      }
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 42);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, texWidth, texHeight, 0,
                GL_LUMINANCE, GL_FLOAT, texImage);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


static void
Init(void)
{
   GLint m;

   glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &m);
   if (m < 1) {
      printf("Error: no vertex shader texture units supported.\n");
      exit(1);
   }

   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderText(GL_VERTEX_SHADER, VertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, FragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   assert(glGetError() == 0);

   MakeTexture();

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
   glewInit();
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

