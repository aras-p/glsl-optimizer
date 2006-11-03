/**
 * Test OpenGL 2.0 vertex/fragment shaders.
 * Brian Paul
 * 1 November 2006
 *
 * Based on ARB version by:
 * Michal Krol
 * 20 February 2006
 *
 * Based on the original demo by:
 * Brian Paul
 * 17 April 2003
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>

static GLfloat diffuse[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
static GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
static GLfloat lightPos[4] = { 0.0f, 10.0f, 20.0f, 1.0f };
static GLfloat delta = 1.0f;

static GLuint fragShader;
static GLuint vertShader;
static GLuint program;

static GLint uLightPos;
static GLint uDiffuse;
static GLint uSpecular;

static GLint win = 0;
static GLboolean anim = GL_TRUE;
static GLboolean wire = GL_FALSE;
static GLboolean pixelLight = GL_TRUE;

static GLint t0 = 0;
static GLint frames = 0;

static GLfloat xRot = 0.0f, yRot = 0.0f;

static PFNGLCREATESHADERPROC glCreateShader_func = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource_func = NULL;
static PFNGLGETSHADERSOURCEPROC glGetShaderSource_func = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader_func = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram_func = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram_func = NULL;
static PFNGLDELETESHADERPROC glDeleteShader_func = NULL;
static PFNGLATTACHSHADERPROC glAttachShader_func = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram_func = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram_func = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_func = NULL;
static PFNGLISPROGRAMPROC glIsProgram_func = NULL;
static PFNGLISSHADERPROC glIsShader_func = NULL;
static PFNGLUNIFORM3FVPROC glUniform3fv_func = NULL;
static PFNGLUNIFORM3FVPROC glUniform4fv_func = NULL;



static void
normalize(GLfloat *dst, const GLfloat *src)
{
   GLfloat len = sqrtf(src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);
   dst[0] = src[0] / len;
   dst[1] = src[1] / len;
   dst[2] = src[2] / len;
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   if (pixelLight) {
      GLfloat vec[3];
      glUseProgram_func(program);
      normalize(vec, lightPos);
      glUniform3fv_func(uLightPos, 1, vec);
      glDisable(GL_LIGHTING);
   }
   else {
      glUseProgram_func(0);
      glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
      glEnable(GL_LIGHTING);
   }

   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glutSolidSphere(2.0, 10, 5);
   glPopMatrix();

   glutSwapBuffers();
   frames++;

   if (anim) {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      if (t - t0 >= 5000) {
         GLfloat seconds =(GLfloat)(t - t0) / 1000.0f;
         GLfloat fps = frames / seconds;
         printf("%d frames in %6.3f seconds = %6.3f FPS\n",
                frames, seconds, fps);
         t0 = t;
         frames = 0;
      }
   }
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
   case 'x':
      lightPos[0] -= 1.0f;
      break;
   case 'X':
      lightPos[0] += 1.0f;
      break;
   case 'w':
      wire = !wire;
      if (wire)
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
         glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
   case 'p':
      pixelLight = !pixelLight;
      if (pixelLight)
         printf("Per-pixel lighting\n");
      else
         printf("Conventional lighting\n");
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
   static const char *fragShaderText =
      "uniform vec3 lightPos;\n"
      "uniform vec4 diffuse;\n"
      "uniform vec4 specular;\n"
      "varying vec3 normal;\n"
      "void main() {\n"
      "   // Compute dot product of light direction and normal vector\n"
      "   float dotProd = max(dot(lightPos, normalize(normal)), 0.0);\n"
      "   // Compute diffuse and specular contributions\n"
      "   gl_FragColor = diffuse * dotProd + specular * pow(dotProd, 20.0);\n"
      "}\n";
   static const char *vertShaderText =
      "varying vec3 normal;\n"
      "void main() {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   normal = gl_NormalMatrix * gl_Normal;\n"
      "}\n";


   const char *version;

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] != '2' || version[1] != '.') {
      printf("Warning: this program expects OpenGL 2.0\n");
      /*exit(1);*/
   }


   glCreateShader_func = (PFNGLCREATESHADERPROC) glutGetProcAddress("glCreateShader");
   glDeleteShader_func = (PFNGLDELETESHADERPROC) glutGetProcAddress("glDeleteShader");
   glDeleteProgram_func = (PFNGLDELETEPROGRAMPROC) glutGetProcAddress("glDeleteProgram");
   glShaderSource_func = (PFNGLSHADERSOURCEPROC) glutGetProcAddress("glShaderSource");
   glGetShaderSource_func = (PFNGLGETSHADERSOURCEPROC) glutGetProcAddress("glGetShaderSource");
   glCompileShader_func = (PFNGLCOMPILESHADERPROC) glutGetProcAddress("glCompileShader");
   glCreateProgram_func = (PFNGLCREATEPROGRAMPROC) glutGetProcAddress("glCreateProgram");
   glAttachShader_func = (PFNGLATTACHSHADERPROC) glutGetProcAddress("glAttachShader");
   glLinkProgram_func = (PFNGLLINKPROGRAMPROC) glutGetProcAddress("glLinkProgram");
   glUseProgram_func = (PFNGLUSEPROGRAMPROC) glutGetProcAddress("glUseProgram");
   glGetUniformLocation_func = (PFNGLGETUNIFORMLOCATIONPROC) glutGetProcAddress("glGetUniformLocation");
   glIsProgram_func = (PFNGLISPROGRAMPROC) glutGetProcAddress("glIsProgram");
   glIsShader_func = (PFNGLISSHADERPROC) glutGetProcAddress("glIsShader");
   glUniform3fv_func = (PFNGLUNIFORM3FVPROC) glutGetProcAddress("glUniform3fv");
   glUniform4fv_func = (PFNGLUNIFORM3FVPROC) glutGetProcAddress("glUniform4fv");

   fragShader = glCreateShader_func(GL_FRAGMENT_SHADER);
   glShaderSource_func(fragShader, 1, &fragShaderText, NULL);
   glCompileShader_func(fragShader);

   vertShader = glCreateShader_func(GL_VERTEX_SHADER);
   glShaderSource_func(vertShader, 1, &vertShaderText, NULL);
   glCompileShader_func(vertShader);

   program = glCreateProgram_func();
   glAttachShader_func(program, fragShader);
   glAttachShader_func(program, vertShader);
   glLinkProgram_func(program);
   glUseProgram_func(program);

   uLightPos = glGetUniformLocation_func(program, "lightPos");
   uDiffuse = glGetUniformLocation_func(program, "diffuse");
   uSpecular = glGetUniformLocation_func(program, "specular");

   glUniform4fv_func(uDiffuse, 1, diffuse);
   glUniform4fv_func(uSpecular, 1, specular);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHTING);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));
   printf("Press p to toggle between per-pixel and per-vertex lighting\n");

   /* test glGetShaderSource() */
   {
      GLsizei len = strlen(fragShaderText) + 1;
      GLsizei lenOut;
      GLchar *src =(GLchar *) malloc(len * sizeof(GLchar));
      glGetShaderSource_func(fragShader, 0, NULL, src);
      glGetShaderSource_func(fragShader, len, &lenOut, src);
      assert(len == lenOut + 1);
      assert(strcmp(src, fragShaderText) == 0);
      free(src);
   }

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition( 0, 0);
   glutInitWindowSize(200, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   if (anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}

