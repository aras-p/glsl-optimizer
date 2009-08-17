/**
 * Test GLSL 1.20 gl_PointCoord fragment program attribute.
 * Brian Paul
 * 11 Aug 2007
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static GLint WinWidth = 300, WinHeight = 300;
static char *FragProgFile = NULL;
static char *VertProgFile = NULL;
static GLuint fragShader;
static GLuint vertShader;
static GLuint program;
static GLint win = 0;
static GLint tex0;
static GLenum Filter = GL_NEAREST;


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   /* draw one point/sprite */
   glPushMatrix();
   glPointSize(60);
   glBegin(GL_POINTS);
   glVertex2f(WinWidth / 2.0f, WinHeight / 2.0f);
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
   glOrtho(0, width, 0, height, -1, 1);

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
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}



static void
MakeTexture(void)
{
#define SZ 16
   GLubyte image[SZ][SZ][4];
   GLuint i, j;

   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
         if ((i + j) & 1) {
            image[i][j][0] = 0;
            image[i][j][1] = 0;
            image[i][j][2] = 0;
            image[i][j][3] = 255;
         }
         else {
            image[i][j][0] = j * 255 / (SZ-1);
            image[i][j][1] = i * 255 / (SZ-1);
            image[i][j][2] = 0;
            image[i][j][3] = 255;
         }
      }
   }

   glActiveTexture(GL_TEXTURE0); /* unit 0 */
   glBindTexture(GL_TEXTURE_2D, 42);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#undef SZ
}


static void
Init(void)
{
   static const char *fragShaderText =
      "#version 120 \n"
      "uniform sampler2D tex0; \n"
      "void main() { \n"
      "   gl_FragColor = texture2D(tex0, gl_PointCoord.xy, 0.0); \n"
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

   tex0 = glGetUniformLocation(program, "tex0");
   printf("Uniforms: tex0: %d\n", tex0);

   glUniform1i(tex0, 0); /* tex unit 0 */

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram(program));
   assert(glIsShader(fragShader));
   assert(glIsShader(vertShader));

   MakeTexture();

   glEnable(GL_POINT_SPRITE);

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
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}
