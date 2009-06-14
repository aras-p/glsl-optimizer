/**
 * Draw two quads, one using only a vertex shader, the other only with a
 * fragment shader.  They should appear the same.
 * 17 Dec 2008
 * Brian Paul
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include "extfuncs.h"
#include "shaderutil.h"


static char *FragProgFile = NULL;
static char *VertProgFile = NULL;
static GLuint FragShader;
static GLuint VertShader;
static GLuint VertProgram;  /* w/out vertex shader */
static GLuint FragProgram;  /* w/out fragment shader */
static GLint Win = 0;


static void
DrawQuadColor(void)
{
   glBegin(GL_QUADS);
   glColor3f(1, 0, 0);    glVertex2f(-1, -1);
   glColor3f(0, 1, 0);    glVertex2f( 1, -1);
   glColor3f(0, 0, 1);    glVertex2f( 1,  1);
   glColor3f(1, 0, 1);    glVertex2f(-1,  1);
   glEnd();
}


/** as above, but specify color via texcoords */
static void
DrawQuadTex(void)
{
   glBegin(GL_QUADS);
   glTexCoord3f(1, 0, 0);    glVertex2f(-1, -1);
   glTexCoord3f(0, 1, 0);    glVertex2f( 1, -1);
   glTexCoord3f(0, 0, 1);    glVertex2f( 1,  1);
   glTexCoord3f(1, 0, 1);    glVertex2f(-1,  1);
   glEnd();
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* render with vertex shader only */
   glUseProgram_func(VertProgram);
   glPushMatrix();
   glTranslatef(-1.5, 0, 0);
   DrawQuadTex();
   glPopMatrix();

   /* render with fragment shader only */
   glUseProgram_func(FragProgram);
   glPushMatrix();
   glTranslatef(+1.5, 0, 0);
   DrawQuadColor();
   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-4, 4, -2, 2, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
CleanUp(void)
{
   glDeleteShader_func(FragShader);
   glDeleteShader_func(VertShader);
   glDeleteProgram_func(VertProgram);
   glDeleteProgram_func(FragProgram);
   glutDestroyWindow(Win);
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;
   switch(key) {
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   static const char *fragShaderText =
      "void main() {\n"
      "   gl_FragColor = gl_Color;\n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = ftransform();\n"
      "   gl_FrontColor = gl_MultiTexCoord0;\n" /* see DrawQuadTex() */
      "}\n";

   if (!ShadersSupported())
      exit(1);

   GetExtensionFuncs();

   if (FragProgFile)
      FragShader = CompileShaderFile(GL_FRAGMENT_SHADER, FragProgFile);
   else
      FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);

   if (VertProgFile)
      VertShader = CompileShaderFile(GL_VERTEX_SHADER, VertProgFile);
   else
      VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);

   VertProgram = LinkShaders(VertShader, 0);
   FragProgram = LinkShaders(0, FragShader);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram_func(VertProgram));
   assert(glIsProgram_func(FragProgram));
   assert(glIsShader_func(FragShader));
   assert(glIsShader_func(VertShader));

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
   glutInitWindowSize(400, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}
