/**
 * Test OpenGL 2.0 dx/dy functions for texcoords.
 * Brian Paul
 * 2 May 2007
 *
 * NOTE: resize the window to observe how the partial derivatives of
 * the texcoords change.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static char *FragProgFile = NULL;
static char *VertProgFile = NULL;
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;
static GLuint SphereList, RectList, CurList;
static GLint win = 0;
static GLboolean anim = GL_TRUE;
static GLfloat xRot = 0.0f, yRot = 0.0f;


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glCallList(CurList);
   glPopMatrix();

   glutSwapBuffers();
}


static void
Idle(void)
{
   yRot = glutGet(GLUT_ELAPSED_TIME) * 0.1;
   glutPostRedisplay();
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
  (void) x;
  (void) y;

   switch(key) {
   case ' ':
   case 'a':
      anim = !anim;
      if (anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'o':
      if (CurList == SphereList)
         CurList = RectList;
      else
         CurList = SphereList;
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
MakeSphere(void)
{
   GLUquadricObj *obj = gluNewQuadric();
   SphereList = glGenLists(1);
   gluQuadricTexture(obj, GL_TRUE);
   glNewList(SphereList, GL_COMPILE);
   gluSphere(obj, 2.0f, 30, 15);
   glEndList();
}


static void
MakeRect(void)
{
   RectList = glGenLists(1);
   glNewList(RectList, GL_COMPILE);
   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);   glVertex2f(-2, -2);
   glTexCoord2f(1, 0);   glVertex2f( 2, -2);
   glTexCoord2f(1, 1);   glVertex2f( 2,  2);
   glTexCoord2f(0, 1);   glVertex2f(-2,  2);
   glEnd();
   glEndList();
}


static void
Init(void)
{
   static const char *fragShaderText =
      "void main() {\n"
      "   gl_FragColor = abs(dFdy(gl_TexCoord[0])) * 50.0;\n"
      "  // gl_FragColor = gl_TexCoord[0];\n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
      "}\n";

   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);

   MakeSphere();
   MakeRect();

   CurList = SphereList;

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
   glutInitWindowSize(200, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   if (anim)
      glutIdleFunc(Idle);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}
