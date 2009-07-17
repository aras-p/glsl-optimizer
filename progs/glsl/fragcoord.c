/**
 * Test GLSL gl_FragCoord fragment program attribute.
 * Color the quad's fragments according to their window position.
 *
 * Brian Paul
 * 20 Nov 2008
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static GLint WinWidth = 200, WinHeight = 200;
static char *FragProgFile = NULL;
static char *VertProgFile = NULL;
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;
static GLint win = 0;
static GLboolean Anim = GL_TRUE;
static GLfloat PosX = 0.0, PosY = 0.0;


static void
Idle(void)
{
   float r = (WinWidth < WinHeight) ? WinWidth : WinHeight;
   float a = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   r *= 0.25;
   PosX = WinWidth / 2 + r * cos(a);
   PosY = WinHeight / 2 + r * sin(a);

   glutPostRedisplay();
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glPushMatrix();
   glTranslatef(PosX, PosY, 0.0);
#if 0
   glBegin(GL_POLYGON);
   glVertex2f(-50, -50);
   glVertex2f( 50, -50);
   glVertex2f( 50,  50);
   glVertex2f(-50,  50);
   glEnd();
#else
   glutSolidSphere(50, 20, 10);
#endif
   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, 0, height, -55, 55);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   WinWidth = width;
   WinHeight = height;
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
      Anim = !Anim;
      glutIdleFunc(Anim ? Idle : NULL);
      break;
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
      "void main() { \n"
      "   vec4 scale = vec4(.005, 0.005, 0.5, 1.0);\n"
      "   gl_FragColor = gl_FragCoord * scale; \n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "}\n";

   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

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
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   glutIdleFunc(Anim ? Idle : NULL);
   glutMainLoop();
   return 0;
}
