/**
 * Demonstration of doing triangle rasterization with a fragment program.
 * Basic idea:
 *   1. Draw screen-aligned quad / bounding box around the triangle verts.
 *   2. For each pixel in the quad, determine if pixel is inside/outside
 *      the triangle edges.
 *
 * Brian Paul
 * 1 Aug 2007
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
static GLboolean anim = GL_TRUE;
static GLfloat Zrot = 0.0f;
static GLint uv0, uv1, uv2;


static const GLfloat TriVerts[3][2] = {
   { 50,  50 },
   { 250, 50 },
   { 150, 250 }
};


static void
RotateVerts(GLfloat a,
            GLuint n, const GLfloat vertsIn[][2], GLfloat vertsOut[][2])
{
   GLuint i;
   GLfloat cx = WinWidth / 2, cy = WinHeight / 2;
   for (i = 0; i < n; i++) {
      float x = vertsIn[i][0] - cx;
      float y = vertsIn[i][1] - cy;
      
      vertsOut[i][0] =  x * cos(a) + y * sin(a)  + cx;
      vertsOut[i][1] = -x * sin(a) + y * cos(a)  + cy;
   }
}

static void
ComputeBounds(GLuint n, GLfloat vertsIn[][2],
              GLfloat *xmin, GLfloat *ymin,
              GLfloat *xmax, GLfloat *ymax)
{
   GLuint i;
   *xmin = *xmax = vertsIn[0][0];
   *ymin = *ymax = vertsIn[0][1];
   for (i = 1; i < n; i++) {
      if (vertsIn[i][0] < *xmin)
         *xmin = vertsIn[i][0];
      else if (vertsIn[i][0] > *xmax)
         *xmax = vertsIn[i][0];
      if (vertsIn[i][1] < *ymin)
         *ymin = vertsIn[i][1];
      else if (vertsIn[i][1] > *ymax)
         *ymax = vertsIn[i][1];
   }
}


static void
Redisplay(void)
{
   GLfloat v[3][2], xmin, ymin, xmax, ymax;

   RotateVerts(Zrot, 3, TriVerts, v);
   ComputeBounds(3, v, &xmin, &ymin, &xmax, &ymax);

   glUniform2fv(uv0, 1, v[0]);
   glUniform2fv(uv1, 1, v[1]);
   glUniform2fv(uv2, 1, v[2]);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glBegin(GL_POLYGON);
   glVertex2f(xmin, ymin);
   glVertex2f(xmax, ymin);
   glVertex2f(xmax, ymax);
   glVertex2f(xmin, ymax);
   glEnd();
   glPopMatrix();

   glutSwapBuffers();
}


static void
Idle(void)
{
   if (anim) {
      Zrot = glutGet(GLUT_ELAPSED_TIME) * 0.0005;
      glutPostRedisplay();
   }
   else
      abort();
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
   case 'z':
      Zrot = 0;
      break;
   case 's':
      Zrot += 0.05;
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
      "uniform vec2 v0, v1, v2; \n"
      "float crs(const vec2 u, const vec2 v) \n"
      "{ \n"
      "   return u.x * v.y - u.y * v.x; \n"
      "} \n"
      "\n"
      "void main() {\n"
      "   vec2 p = gl_FragCoord.xy; \n"
      "   if (crs(v1 - v0, p - v0) >= 0.0 && \n"
      "       crs(v2 - v1, p - v1) >= 0.0 && \n"
      "       crs(v0 - v2, p - v2) >= 0.0) \n"
      "      gl_FragColor = vec4(1.0); \n"
      "   else \n"
      "      gl_FragColor = vec4(0.5); \n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = ftransform(); \n"
      "}\n";

   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   uv0 = glGetUniformLocation(program, "v0");
   uv1 = glGetUniformLocation(program, "v1");
   uv2 = glGetUniformLocation(program, "v2");
   printf("Uniforms: %d %d %d\n", uv0, uv1, uv2);

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);

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
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   if (anim)
      glutIdleFunc(Idle);
   ParseOptions(argc, argv);
   Init();
   glutMainLoop();
   return 0;
}
