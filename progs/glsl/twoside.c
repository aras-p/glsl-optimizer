/**
 * Test two-sided lighting with shaders.
 * Both GL_VERTEX_PROGRAM_TWO_SIDE and gl_FrontFacing can be tested
 * (see keys below).
 *
 * Brian Paul
 * 18 Dec 2007
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


static GLint WinWidth = 300, WinHeight = 300;
static char *FragProgFile = NULL;
static char *VertProgFile = NULL;
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;
static GLint win = 0;
static GLboolean anim;
static GLboolean DetermineFacingInFragProg;
static GLfloat Xrot;
static GLint u_fragface;
static GLenum FrontWinding;
static int prevTime = 0;


static const GLfloat Red[4] = {1, 0, 0, 1};
static const GLfloat Green[4] = {0, 1, 0, 0};


static void
SetDefaults(void)
{
   DetermineFacingInFragProg = GL_TRUE;
   FrontWinding = GL_CCW;
   Xrot = 30;
   anim = 0;
   glutIdleFunc(NULL);
}


static void
Redisplay(void)
{
   const int sections = 20;
   int i;
   float radius = 2;

   glFrontFace(FrontWinding);

   if (DetermineFacingInFragProg) {
      glUniform1i_func(u_fragface, 1);
      glDisable(GL_VERTEX_PROGRAM_TWO_SIDE);
   }
   else {
      glUniform1i_func(u_fragface, 0);
      glEnable(GL_VERTEX_PROGRAM_TWO_SIDE);
   }

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);

   /* Draw a tristrip ring */
   glBegin(GL_TRIANGLE_STRIP);
   glColor4fv(Red);
   glSecondaryColor3fv_func(Green);
   for (i = 0; i <= sections; i++) {
      float a = (float) i / (sections) * M_PI * 2.0;
      float x = radius * cos(a);
      float y = radius * sin(a);
      glVertex3f(x, -1, y);
      glVertex3f(x, +1, y);
   }
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void
Idle(void)
{
   int curTime = glutGet(GLUT_ELAPSED_TIME);
   int dt = curTime - prevTime;

   if (prevTime == 0) {
      prevTime = curTime;
      return;
   }
   prevTime = curTime;

   Xrot += dt * 0.1;
   glutPostRedisplay();
}


static void
Reshape(int width, int height)
{
   float ar = (float) width / height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1, 1, 3, 25);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -10);
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
  (void) x;
  (void) y;

   switch(key) {
   case ' ':
   case 'a':
      anim = !anim;
      if (anim) {
         prevTime = glutGet(GLUT_ELAPSED_TIME);
         glutIdleFunc(Idle);
      }
      else
         glutIdleFunc(NULL);
      break;
   case 'f':
      printf("Using frag shader gl_FrontFacing\n");
      DetermineFacingInFragProg = GL_TRUE;
      break;
   case 'v':
      printf("Using vert shader Two-sided lighting\n");
      DetermineFacingInFragProg = GL_FALSE;
      break;
   case 'r':
      /* reset */
      SetDefaults();
      break;
   case 's':
      Xrot += 5;
      break;
   case 'S':
      Xrot -= 5;
      break;
   case 'w':
      if (FrontWinding == GL_CCW) {
         FrontWinding = GL_CW;
         printf("FrontFace = GL_CW\n");
      }
      else {
         FrontWinding = GL_CCW;
         printf("FrontFace = GL_CCW\n");
      }
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
      "uniform bool fragface; \n"
      "void main() { \n"
#if 1
      "   if (!fragface || gl_FrontFacing) { \n"
      "      gl_FragColor = gl_Color; \n"
      "   } \n"
      "   else { \n"
      "      // note: dim green to help debug \n"
      "      gl_FragColor = 0.8 * gl_SecondaryColor; \n"
      "   } \n"
#else
      /* DEBUG CODE */
      "   bool f = gl_FrontFacing; \n"
      "   if (f) { \n"
      "      gl_FragColor = vec4(1.0, 0.0, 0.0, 0.0); \n"
      "   } \n"
      "   else { \n"
      "      gl_FragColor = vec4(0.0, 1.0, 0.0, 0.0); \n"
      "   } \n"
#endif
      "} \n";
   static const char *vertShaderText =
      "uniform bool fragface; \n"
      "void main() { \n"
      "   gl_FrontColor = gl_Color; \n"
      "   if (fragface) { \n"
      "      // front/back chosen in frag prog \n"
      "      gl_FrontSecondaryColor = gl_SecondaryColor; \n"
      "   } \n"
      "   else { \n"
      "      // front/back chosen in prim setup \n"
      "      gl_BackColor = gl_SecondaryColor; \n"
      "   } \n"
      "   gl_Position = ftransform(); \n"
      "} \n";

   if (!ShadersSupported())
      exit(1);

   GetExtensionFuncs();

   vertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram_func(program);

   u_fragface = glGetUniformLocation_func(program, "fragface");
   printf("Uniforms: %d\n", u_fragface);

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));

   glEnable(GL_DEPTH_TEST);

   SetDefaults();
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


static void
Usage(void)
{
   printf("Keys:\n");
   printf("   f - do front/back determination in fragment shader\n");
   printf("   v - do front/back determination in vertex shader\n");
   printf("   r - reset, show front\n");
   printf("   a - toggle animation\n");
   printf("   s - step rotation\n");
   printf("   w - toggle CW, CCW front-face winding\n");
   printf("NOTE: red = front face, green = back face.\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition( 0, 0);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   if (anim)
      glutIdleFunc(Idle);
   ParseOptions(argc, argv);
   Init();
   Usage();
   glutMainLoop();
   return 0;
}
