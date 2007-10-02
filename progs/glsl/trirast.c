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
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include "extfuncs.h"


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

   glUniform2fv_func(uv0, 1, v[0]);
   glUniform2fv_func(uv1, 1, v[1]);
   glUniform2fv_func(uv2, 1, v[2]);

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
   Zrot = glutGet(GLUT_ELAPSED_TIME) * 0.0005;
   glutPostRedisplay();
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
      if (anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
LoadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;

   glShaderSource_func(shader, 1, (const GLchar **) &text, NULL);

   glCompileShader_func(shader);

   glGetShaderiv_func(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog_func(shader, 1000, &len, log);
      fprintf(stderr, "fslight: problem compiling shader:\n%s\n", log);
      exit(1);
   }
}


/**
 * Read a shader from a file.
 */
static void
ReadShader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "fslight: Unable to open shader file %s\n", filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("fslight: read %d bytes from shader file %s\n", n, filename);
   if (n > 0) {
      buffer[n] = 0;
      LoadAndCompileShader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}


static void
CheckLink(GLuint prog)
{
   GLint stat;
   glGetProgramiv_func(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog_func(prog, 1000, &len, log);
      fprintf(stderr, "Linker error:\n%s\n", log);
   }
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
      "   if (crs(v1 - v0, p - v0) >= 0 && \n"
      "       crs(v2 - v1, p - v1) >= 0 && \n"
      "       crs(v0 - v2, p - v2) >= 0) \n"
      "      gl_FragColor = vec4(1.0); \n"
      "   else \n"
      "      gl_FragColor = vec4(0.5); \n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "}\n";
   const char *version;

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] != '2' || version[1] != '.') {
      printf("This program requires OpenGL 2.x, found %s\n", version);
      exit(1);
   }

   GetExtensionFuncs();

   fragShader = glCreateShader_func(GL_FRAGMENT_SHADER);
   if (FragProgFile)
      ReadShader(fragShader, FragProgFile);
   else
      LoadAndCompileShader(fragShader, fragShaderText);

   vertShader = glCreateShader_func(GL_VERTEX_SHADER);
   if (VertProgFile)
      ReadShader(vertShader, VertProgFile);
   else
      LoadAndCompileShader(vertShader, vertShaderText);

   program = glCreateProgram_func();
   glAttachShader_func(program, fragShader);
   glAttachShader_func(program, vertShader);
   glLinkProgram_func(program);
   CheckLink(program);
   glUseProgram_func(program);

   uv0 = glGetUniformLocation_func(program, "v0");
   uv1 = glGetUniformLocation_func(program, "v1");
   uv2 = glGetUniformLocation_func(program, "v2");
   printf("Uniforms: %d %d %d\n", uv0, uv1, uv2);

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));

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
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
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
