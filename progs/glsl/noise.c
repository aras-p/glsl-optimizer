/**
 * Test noise() functions.
 * 28 Jan 2007
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
   "   p.w = 0;\n"
   "   vec4 n = noise4(p * scale);\n"
   "   gl_FragColor = n * Scale + Bias;\n"
   "}\n";


static struct uniform_info Uniforms[] = {
   { "Scale",    1, GL_FLOAT_VEC4, { 0.5, 0.4, 0.0, 0}, -1 },
   { "Bias",     1, GL_FLOAT_VEC4, { 0.5, 0.3, 0.0, 0}, -1 },
   { "Slice",    1, GL_FLOAT, { 0.5, 0, 0, 0}, -1 },
   END_OF_UNIFORMS
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
   
   glUniform1fv(Uniforms[2].location, 1, &Slice);

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
   glDeleteShader(fragShader);
   glDeleteShader(vertShader);
   glDeleteProgram(program);
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
Init(void)
{
   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderText(GL_VERTEX_SHADER, VertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, FragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   SetUniformValues(program, Uniforms);
   PrintUniforms(Uniforms);

   assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram(program));
   assert(glIsShader(fragShader));
   assert(glIsShader(vertShader));

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
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   Init();
   glutMainLoop();
   return 0;
}

