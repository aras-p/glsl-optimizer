/**
 * Test linking of multiple compilation units.
 * Brian Paul
 * 28 March 2009
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static GLfloat diffuse[4] = { 0.5f, 1.0f, 0.5f, 1.0f };
static GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
static GLfloat lightPos[4] = { 0.0f, 10.0f, 20.0f, 0.0f };
static GLfloat delta = 1.0f;

static GLuint VertShader1;
static GLuint VertShader2;
static GLuint FragShader1;
static GLuint FragShader2;
static GLuint Program;

static GLint uDiffuse;
static GLint uSpecular;
static GLint uTexture;

static GLint Win = 0;
static GLboolean anim = GL_TRUE;



static const char *FragShaderSource1 =
   "float compute_dotprod(const vec3 normal) \n"
   "{ \n"
   "   float dotProd = max(dot(gl_LightSource[0].position.xyz, \n"
   "                           normalize(normal)), 0.0); \n"
   "   return dotProd; \n"
   "} \n";

static const char *FragShaderSource2 =
   "uniform vec4 diffuse;\n"
   "uniform vec4 specular;\n"
   "varying vec3 normal;\n"
   "\n"
   "// external function \n"
   "float compute_dotprod(const vec3 normal); \n"
   "\n"
   "void main() \n"
   "{ \n"
   "   float dotProd = compute_dotprod(normal); \n"
   "   gl_FragColor = diffuse * dotProd + specular * pow(dotProd, 20.0); \n"
   "} \n";


static const char *VertShaderSource1 =
   "vec3 compute_normal() \n"
   "{ \n"
   "   return gl_NormalMatrix * gl_Normal; \n"
   "} \n";

static const char *VertShaderSource2 =
   "varying vec3 normal;\n"
   "\n"
   "// external function \n"
   "vec3 compute_normal(); \n"
   "\n"
   "void main() \n"
   "{ \n"
   "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
   "   normal = compute_normal(); \n"
   "} \n";


static void
normalize(GLfloat *dst, const GLfloat *src)
{
   GLfloat len = sqrt(src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);
   dst[0] = src[0] / len;
   dst[1] = src[1] / len;
   dst[2] = src[2] / len;
   dst[3] = src[3];
}


static void
Redisplay(void)
{
   GLfloat vec[4];

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* update light position */
   normalize(vec, lightPos);
   glLightfv(GL_LIGHT0, GL_POSITION, vec);
   
   glutSolidSphere(2.0, 10, 5);

   glutSwapBuffers();
}


static void
Idle(void)
{
   lightPos[0] += delta;
   if (lightPos[0] > 25.0f || lightPos[0] < -25.0f)
      delta = -delta;
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
   glDeleteShader(VertShader1);
   glDeleteShader(VertShader2);
   glDeleteShader(FragShader1);
   glDeleteShader(FragShader2);
   glDeleteProgram(Program);
   glutDestroyWindow(Win);
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
   case 'x':
      lightPos[0] -= 1.0f;
      break;
   case 'X':
      lightPos[0] += 1.0f;
      break;
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
CheckLink(GLuint prog)
{
   GLint stat;
   glGetProgramiv(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog(prog, 1000, &len, log);
      fprintf(stderr, "Linker error:\n%s\n", log);
   }
}


static void
Init(void)
{
   if (!ShadersSupported())
      exit(1);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   VertShader1 = CompileShaderText(GL_VERTEX_SHADER, VertShaderSource1);
   VertShader2 = CompileShaderText(GL_VERTEX_SHADER, VertShaderSource2);
   FragShader1 = CompileShaderText(GL_FRAGMENT_SHADER, FragShaderSource1);
   FragShader2 = CompileShaderText(GL_FRAGMENT_SHADER, FragShaderSource2);

   Program = glCreateProgram();
   glAttachShader(Program, VertShader1);
   glAttachShader(Program, VertShader2);
   glAttachShader(Program, FragShader1);
   glAttachShader(Program, FragShader2);

   glLinkProgram(Program);

   CheckLink(Program);

   glUseProgram(Program);

   uDiffuse = glGetUniformLocation(Program, "diffuse");
   uSpecular = glGetUniformLocation(Program, "specular");
   uTexture = glGetUniformLocation(Program, "texture");
   printf("DiffusePos %d  SpecularPos %d  TexturePos %d\n",
          uDiffuse, uSpecular, uTexture);

   glUniform4fv(uDiffuse, 1, diffuse);
   glUniform4fv(uSpecular, 1, specular);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);

   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0f);

   assert(glIsProgram(Program));
   assert(glIsShader(VertShader1));
   assert(glIsShader(VertShader2));
   assert(glIsShader(FragShader1));
   assert(glIsShader(FragShader2));

   glColor3f(1, 0, 0);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(300, 300);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   if (anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}


