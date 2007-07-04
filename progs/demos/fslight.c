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
#include "extfuncs.h"


#define TEXTURE 0

static GLint CoordAttrib = 0;

static char *FragProgFile = NULL;
static char *VertProgFile = NULL;

static GLfloat diffuse[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
static GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
static GLfloat lightPos[4] = { 0.0f, 10.0f, 20.0f, 0.0f };
static GLfloat delta = 1.0f;

static GLuint fragShader;
static GLuint vertShader;
static GLuint program;

static GLint uDiffuse;
static GLint uSpecular;
static GLint uTexture;

static GLuint SphereList, RectList, CurList;
static GLint win = 0;
static GLboolean anim = GL_TRUE;
static GLboolean wire = GL_FALSE;
static GLboolean pixelLight = GL_TRUE;

static GLint t0 = 0;
static GLint frames = 0;

static GLfloat xRot = 90.0f, yRot = 0.0f;


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
   
   if (pixelLight) {
      glUseProgram_func(program);
      glDisable(GL_LIGHTING);
   }
   else {
      glUseProgram_func(0);
      glEnable(GL_LIGHTING);
   }

   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   /*
   glutSolidSphere(2.0, 10, 5);
   */
   glCallList(CurList);
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
   case 'o':
      if (CurList == SphereList)
         CurList = RectList;
      else
         CurList = SphereList;
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
TestFunctions(void)
{
   printf("Error 0x%x at line %d\n", glGetError(), __LINE__);
   {
      GLfloat pos[3];
      printf("Error 0x%x at line %d\n", glGetError(), __LINE__);
      printf("Light pos %g %g %g\n", pos[0], pos[1], pos[2]);
   }


   {
      GLfloat m[16], result[16];
      GLint mPos;
      int i;

      for (i = 0; i < 16; i++)
         m[i] = (float) i;

      mPos = glGetUniformLocation_func(program, "m");
      printf("Error 0x%x at line %d\n", glGetError(), __LINE__);
      glUniformMatrix4fv_func(mPos, 1, GL_FALSE, m);
      printf("Error 0x%x at line %d\n", glGetError(), __LINE__);

      glGetUniformfv_func(program, mPos, result);
      printf("Error 0x%x at line %d\n", glGetError(), __LINE__);

      for (i = 0; i < 16; i++) {
         printf("%8g %8g\n", m[i], result[i]);
      }
   }

   assert(glIsProgram_func(program));
   assert(glIsShader_func(fragShader));
   assert(glIsShader_func(vertShader));

   /* attached shaders */
   {
      GLuint shaders[20];
      GLsizei count;
      int i;
      glGetAttachedShaders_func(program, 20, &count, shaders);
      for (i = 0; i < count; i++) {
         printf("Attached: %u\n", shaders[i]);
         assert(shaders[i] == fragShader ||
                shaders[i] == vertShader);
      }
   }

   {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog_func(vertShader, 1000, &len, log);
      printf("Vert Shader Info Log: %s\n", log);
      glGetShaderInfoLog_func(fragShader, 1000, &len, log);
      printf("Frag Shader Info Log: %s\n", log);
      glGetProgramInfoLog_func(program, 1000, &len, log);
      printf("Program Info Log: %s\n", log);
   }
}


#if TEXTURE
static void
MakeTexture(void)
{
#define SZ0 64
#define SZ1 32
   GLubyte image0[SZ0][SZ0][SZ0][4];
   GLubyte image1[SZ1][SZ1][SZ1][4];
   GLuint i, j, k;

   /* level 0: two-tone gray checkboard */
   for (i = 0; i < SZ0; i++) {
      for (j = 0; j < SZ0; j++) {
         for (k = 0; k < SZ0; k++) {
            if ((i/8 + j/8 + k/8) & 1) {
               image0[i][j][k][0] = 
               image0[i][j][k][1] = 
               image0[i][j][k][2] = 200;
            }
            else {
               image0[i][j][k][0] = 
               image0[i][j][k][1] = 
               image0[i][j][k][2] = 100;
            }
            image0[i][j][k][3] = 255;
         }
      }
   }

   /* level 1: two-tone green checkboard */
   for (i = 0; i < SZ1; i++) {
      for (j = 0; j < SZ1; j++) {
         for (k = 0; k < SZ1; k++) {
            if ((i/8 + j/8 + k/8) & 1) {
               image1[i][j][k][0] = 0;
               image1[i][j][k][1] = 250;
               image1[i][j][k][2] = 0;
            }
            else {
               image1[i][j][k][0] = 0;
               image1[i][j][k][1] = 200;
               image1[i][j][k][2] = 0;
            }
            image1[i][j][k][3] = 255;
         }
      }
   }

   glActiveTexture(GL_TEXTURE2); /* unit 2 */
   glBindTexture(GL_TEXTURE_2D, 42);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ0, SZ0, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image0);
   glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, SZ1, SZ1, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image1);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glActiveTexture(GL_TEXTURE4); /* unit 4 */
   glBindTexture(GL_TEXTURE_3D, 43);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, SZ0, SZ0, SZ0, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image0);
   glTexImage3D(GL_TEXTURE_3D, 1, GL_RGBA, SZ1, SZ1, SZ1, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image1);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 1);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
#endif


static void
MakeSphere(void)
{
   GLUquadricObj *obj = gluNewQuadric();
   SphereList = glGenLists(1);
   gluQuadricTexture(obj, GL_TRUE);
   glNewList(SphereList, GL_COMPILE);
   gluSphere(obj, 2.0f, 10, 5);
   glEndList();
}

static void
VertAttrib(GLint index, float x, float y)
{
#if 1
   glVertexAttrib2f_func(index, x, y);
#else
   glTexCoord2f(x, y);
#endif
}

static void
MakeRect(void)
{
   RectList = glGenLists(1);
   glNewList(RectList, GL_COMPILE);
   glNormal3f(0, 0, 1);
   glBegin(GL_POLYGON);
   VertAttrib(CoordAttrib, 0, 0);   glVertex2f(-2, -2);
   VertAttrib(CoordAttrib, 1, 0);   glVertex2f( 2, -2);
   VertAttrib(CoordAttrib, 1, 1);   glVertex2f( 2,  2);
   VertAttrib(CoordAttrib, 0, 1);   glVertex2f(-2,  2);
   glEnd();    /* XXX omit this and crash! */
   glEndList();
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
      "uniform vec4 diffuse;\n"
      "uniform vec4 specular;\n"
      "varying vec3 normal;\n"
      "void main() {\n"
      "   // Compute dot product of light direction and normal vector\n"
      "   float dotProd = max(dot(gl_LightSource[0].position.xyz, \n"
      "                           normalize(normal)), 0.0);\n"
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

   uDiffuse = glGetUniformLocation_func(program, "diffuse");
   uSpecular = glGetUniformLocation_func(program, "specular");
   uTexture = glGetUniformLocation_func(program, "texture");
   printf("DiffusePos %d  SpecularPos %d  TexturePos %d\n",
          uDiffuse, uSpecular, uTexture);

   glUniform4fv_func(uDiffuse, 1, diffuse);
   glUniform4fv_func(uSpecular, 1, specular);
   /*   assert(glGetError() == 0);*/
   glUniform1i_func(uTexture, 2);  /* use texture unit 2 */
   /*assert(glGetError() == 0);*/

   if (CoordAttrib) {
      int i;
      glBindAttribLocation_func(program, CoordAttrib, "coord");
      i = glGetAttribLocation_func(program, "coord");
      assert(i >= 0);
      if (i != CoordAttrib) {
         printf("Hmmm, NVIDIA bug?\n");
         CoordAttrib = i;
      }
      else {
         printf("Mesa bind attrib: coord = %d\n", i);
      }
   }

   /*assert(glGetError() == 0);*/

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHTING);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);

   MakeSphere();
   MakeRect();

   CurList = SphereList;

#if TEXTURE
   MakeTexture();
#endif

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));
   printf("Press p to toggle between per-pixel and per-vertex lighting\n");

   /* test glGetShaderSource() */
   if (0) {
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

   glColor3f(1, 0, 0);

   /* for testing state vars */
   {
      static GLfloat fc[4] = { 1, 1, 0, 0 };
      static GLfloat amb[4] = { 1, 0, 1, 0 };
      glFogfv(GL_FOG_COLOR, fc);
      glLightfv(GL_LIGHT1, GL_AMBIENT, amb);
   }

#if 0
   TestFunctions();
#else
   (void) TestFunctions;
#endif
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
   glutInitWindowSize(200, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
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


