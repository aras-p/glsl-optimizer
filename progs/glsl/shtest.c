/*
 * Simple shader test harness.
 * Brian Paul
 * 13 Aug 2009
 *
 * Usage:
 *   shtest --vs vertShaderFile --fs fragShaderFile
 *
 *   In this case the given vertex/frag shaders are read and compiled.
 *   Random values are assigned to the uniforms.
 *
 * or:
 *   shtest configFile
 *
 *   In this case a config file is read that specifies the file names
 *   of the shaders plus initial values for uniforms.
 *
 * Example config file:
 *
 * vs shader.vert
 * fs shader.frag
 * uniform pi 3.14159
 * uniform v1 1.0 0.5 0.2 0.3
 * texture 0 2D texture0.rgb
 * texture 1 CUBE texture1.rgb
 * texture 2 RECT texture2.rgb
 *
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "shaderutil.h"
#include "readtex.h"


typedef enum
{
   SPHERE,
   CUBE,
   NUM_SHAPES
} shape;


static char *FragShaderFile = NULL;
static char *VertShaderFile = NULL;
static char *ConfigFile = NULL;

/* program/shader objects */
static GLuint fragShader;
static GLuint vertShader;
static GLuint Program;


#define MAX_UNIFORMS 100
static struct uniform_info Uniforms[MAX_UNIFORMS];
static GLuint NumUniforms = 0;


#define MAX_ATTRIBS 100
static struct attrib_info Attribs[MAX_ATTRIBS];
static GLuint NumAttribs = 0;


/**
 * Config file info.
 */
struct config_file
{
   struct name_value
   {
      char name[100];
      float value[4];
      int type;
   } uniforms[100];

   int num_uniforms;
};


static GLint win = 0;
static GLboolean Anim = GL_FALSE;
static GLfloat TexRot = 0.0;
static GLfloat xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;
static shape Object = SPHERE;


static float
RandomFloat(float min, float max)
{
   int k = rand() % 10000;
   float x = min + (max - min) * k / 10000.0;
   return x;
}


/** Set new random values for uniforms */
static void
RandomUniformValues(void)
{
   GLuint i;
   for (i = 0; i < NumUniforms; i++) {
      if (Uniforms[i].type == GL_FLOAT) {
         Uniforms[i].value[0] = RandomFloat(0.0, 1.0);
      }
      else {
         Uniforms[i].value[0] = RandomFloat(-1.0, 2.0);
         Uniforms[i].value[1] = RandomFloat(-1.0, 2.0);
         Uniforms[i].value[2] = RandomFloat(-1.0, 2.0);
         Uniforms[i].value[3] = RandomFloat(-1.0, 2.0);
      }
   }
}


static void
Idle(void)
{
   yRot += 2.0;
   if (yRot > 360.0)
      yRot -= 360.0;
   glutPostRedisplay();
}



static void
SquareVertex(GLfloat s, GLfloat t, GLfloat size)
{
   GLfloat x = -size + s * 2.0 * size;
   GLfloat y = -size + t * 2.0 * size;
   GLuint i;

   glMultiTexCoord2f(GL_TEXTURE0, s, t);
   glMultiTexCoord2f(GL_TEXTURE1, s, t);
   glMultiTexCoord2f(GL_TEXTURE2, s, t);
   glMultiTexCoord2f(GL_TEXTURE3, s, t);

   /* assign (s,t) to the generic attributes */
   for (i = 0; i < NumAttribs; i++) {
      if (Attribs[i].location >= 0) {
         glVertexAttrib2f(Attribs[i].location, s, t);
      }
   }

   glVertex2f(x, y);
}


/*
 * Draw a square, specifying normal and tangent vectors.
 */
static void
Square(GLfloat size)
{
   GLint tangentAttrib = 1;
   glNormal3f(0, 0, 1);
   glVertexAttrib3f(tangentAttrib, 1, 0, 0);
   glBegin(GL_POLYGON);
#if 1
   SquareVertex(0, 0, size);
   SquareVertex(1, 0, size);
   SquareVertex(1, 1, size);
   SquareVertex(0, 1, size);
#else
   glTexCoord2f(0, 0);  glVertex2f(-size, -size);
   glTexCoord2f(1, 0);  glVertex2f( size, -size);
   glTexCoord2f(1, 1);  glVertex2f( size,  size);
   glTexCoord2f(0, 1);  glVertex2f(-size,  size);
#endif
   glEnd();
}


static void
Cube(GLfloat size)
{
   /* +X */
   glPushMatrix();
   glRotatef(90, 0, 1, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* -X */
   glPushMatrix();
   glRotatef(-90, 0, 1, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* +Y */
   glPushMatrix();
   glRotatef(90, 1, 0, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* -Y */
   glPushMatrix();
   glRotatef(-90, 1, 0, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();


   /* +Z */
   glPushMatrix();
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();

   /* -Z */
   glPushMatrix();
   glRotatef(180, 0, 1, 0);
   glTranslatef(0, 0, size);
   Square(size);
   glPopMatrix();
}


static void
Sphere(GLfloat radius, GLint slices, GLint stacks)
{
   static GLUquadricObj *q = NULL;

   if (!q) {
      q = gluNewQuadric();
      gluQuadricDrawStyle(q, GLU_FILL);
      gluQuadricNormals(q, GLU_SMOOTH);
      gluQuadricTexture(q, GL_TRUE);
   }

   gluSphere(q, radius, slices, stacks);
}


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   glPushMatrix();
   glRotatef(xRot, 1.0f, 0.0f, 0.0f);
   glRotatef(yRot, 0.0f, 1.0f, 0.0f);
   glRotatef(zRot, 0.0f, 0.0f, 1.0f);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glRotatef(TexRot, 0.0f, 1.0f, 0.0f);
   glMatrixMode(GL_MODELVIEW);

   if (Object == SPHERE) {
      Sphere(2.5, 20, 10);
   }
   else if (Object == CUBE) {
      Cube(2.0);
   }

   glPopMatrix();

   glutSwapBuffers();
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
   glDeleteProgram(Program);
   glutDestroyWindow(win);
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 2.0;
  (void) x;
  (void) y;

   switch(key) {
   case 'a':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'z':
      zRot += step;
      break;
   case 'Z':
      zRot -= step;
      break;
   case 'o':
      Object = (Object + 1) % NUM_SHAPES;
      break;
   case 'r':
      RandomUniformValues();
      SetUniformValues(Program, Uniforms);
      PrintUniforms(Uniforms);
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
   const GLfloat step = 2.0;

  (void) x;
  (void) y;

   switch(key) {
   case GLUT_KEY_UP:
      xRot += step;
      break;
   case GLUT_KEY_DOWN:
      xRot -= step;
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
InitUniforms(const struct config_file *conf,
             struct uniform_info uniforms[])
{
   int i;

   for (i = 0; i < conf->num_uniforms; i++) {
      int j;
      for (j = 0; uniforms[j].name; j++) {
         if (strcmp(uniforms[j].name, conf->uniforms[i].name) == 0) {
            uniforms[j].type = conf->uniforms[i].type;
            uniforms[j].value[0] = conf->uniforms[i].value[0];
            uniforms[j].value[1] = conf->uniforms[i].value[1];
            uniforms[j].value[2] = conf->uniforms[i].value[2];
            uniforms[j].value[3] = conf->uniforms[i].value[3];
         }
      }
   }
}


static void
LoadTexture(GLint unit, GLenum target, const char *texFileName)
{
   GLint imgWidth, imgHeight;
   GLenum imgFormat;
   GLubyte *image = NULL;
   GLuint tex;
   GLenum filter = GL_LINEAR;
   GLenum objTarget;

   image = LoadRGBImage(texFileName, &imgWidth, &imgHeight, &imgFormat);
   if (!image) {
      printf("Couldn't read %s\n", texFileName);
      exit(1);
   }

   printf("Load Texture: unit %d, target 0x%x: %s %d x %d\n",
          unit, target, texFileName, imgWidth, imgHeight);

   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
      objTarget = GL_TEXTURE_CUBE_MAP;
   }
   else {
      objTarget = target;
   }

   glActiveTexture(GL_TEXTURE0 + unit);
   glGenTextures(1, &tex);
   glBindTexture(objTarget, tex);

   if (target == GL_TEXTURE_3D) {
      /* depth=1 */
      gluBuild3DMipmaps(target, 4, imgWidth, imgHeight, 1,
                        imgFormat, GL_UNSIGNED_BYTE, image);
   }
   else if (target == GL_TEXTURE_1D) {
      gluBuild1DMipmaps(target, 4, imgWidth,
                        imgFormat, GL_UNSIGNED_BYTE, image);
   }
   else {
      gluBuild2DMipmaps(target, 4, imgWidth, imgHeight,
                        imgFormat, GL_UNSIGNED_BYTE, image);
   }

   free(image);
      
   glTexParameteri(objTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(objTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(objTarget, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(objTarget, GL_TEXTURE_MAG_FILTER, filter);
}


static GLenum
TypeFromName(const char *n)
{
   static const struct {
      const char *name;
      GLenum type;
   } types[] = {
      { "GL_FLOAT", GL_FLOAT },
      { "GL_FLOAT_VEC2", GL_FLOAT_VEC2 },
      { "GL_FLOAT_VEC3", GL_FLOAT_VEC3 },
      { "GL_FLOAT_VEC4", GL_FLOAT_VEC4 },
      { "GL_INT", GL_INT },
      { "GL_INT_VEC2", GL_INT_VEC2 },
      { "GL_INT_VEC3", GL_INT_VEC3 },
      { "GL_INT_VEC4", GL_INT_VEC4 },
      { "GL_SAMPLER_1D", GL_SAMPLER_1D },
      { "GL_SAMPLER_2D", GL_SAMPLER_2D },
      { "GL_SAMPLER_3D", GL_SAMPLER_3D },
      { "GL_SAMPLER_CUBE", GL_SAMPLER_CUBE },
      { "GL_SAMPLER_2D_RECT", GL_SAMPLER_2D_RECT_ARB },
      { NULL, 0 }
   };
   GLuint i;

   for (i = 0; types[i].name; i++) {
      if (strcmp(types[i].name, n) == 0)
         return types[i].type;
   }
   abort();
   return GL_NONE;
}



/**
 * Read a config file.
 */
static void
ReadConfigFile(const char *filename, struct config_file *conf)
{
   char line[1000];
   FILE *f;

   f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Unable to open config file %s\n", filename);
      exit(1);
   }

   conf->num_uniforms = 0;

   /* ugly but functional parser */
   while (!feof(f)) {
      fgets(line, sizeof(line), f);
      if (!feof(f) && line[0]) {
         if (strncmp(line, "vs ", 3) == 0) {
            VertShaderFile = strdup(line + 3);
            VertShaderFile[strlen(VertShaderFile) - 1] = 0;
         }
         else if (strncmp(line, "fs ", 3) == 0) {
            FragShaderFile = strdup(line + 3);
            FragShaderFile[strlen(FragShaderFile) - 1] = 0;
         }
         else if (strncmp(line, "texture ", 8) == 0) {
            char target[100], texFileName[100];
            int unit, k;
            k = sscanf(line + 8, "%d %s %s", &unit, target, texFileName);
            assert(k == 3 || k == 8);
            if (strcmp(target, "CUBE") == 0) {
               char texFileNames[6][100];
               k = sscanf(line + 8, "%d %s  %s %s %s %s %s %s",
                          &unit, target,
                          texFileNames[0],
                          texFileNames[1],
                          texFileNames[2],
                          texFileNames[3],
                          texFileNames[4],
                          texFileNames[5]);
               LoadTexture(unit, GL_TEXTURE_CUBE_MAP_POSITIVE_X, texFileNames[0]);
               LoadTexture(unit, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, texFileNames[1]);
               LoadTexture(unit, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, texFileNames[2]);
               LoadTexture(unit, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, texFileNames[3]);
               LoadTexture(unit, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, texFileNames[4]);
               LoadTexture(unit, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, texFileNames[5]);
            }
            else if (!strcmp(target, "2D")) {
               LoadTexture(unit, GL_TEXTURE_2D, texFileName);
            }
            else if (!strcmp(target, "3D")) {
               LoadTexture(unit, GL_TEXTURE_3D, texFileName);
            }
            else if (!strcmp(target, "RECT")) {
               LoadTexture(unit, GL_TEXTURE_RECTANGLE_ARB, texFileName);
            }
            else {
               printf("Bad texture target: %s\n", target);
               exit(1);
            }
         }
         else if (strncmp(line, "uniform ", 8) == 0) {
            char name[1000], typeName[100];
            int k;
            float v1 = 0.0F, v2 = 0.0F, v3 = 0.0F, v4 = 0.0F;
            GLenum type;

            k = sscanf(line + 8, "%s %s %f %f %f %f", name, typeName,
                       &v1, &v2, &v3, &v4);

            type = TypeFromName(typeName);

            strcpy(conf->uniforms[conf->num_uniforms].name, name);
            conf->uniforms[conf->num_uniforms].value[0] = v1;
            conf->uniforms[conf->num_uniforms].value[1] = v2;
            conf->uniforms[conf->num_uniforms].value[2] = v3;
            conf->uniforms[conf->num_uniforms].value[3] = v4;
            conf->uniforms[conf->num_uniforms].type = type;
            conf->num_uniforms++;
         }
         else {
            if (strlen(line) > 1) {
               fprintf(stderr, "syntax error in: %s\n", line);
               break;
            }
         }
      }
   }

   fclose(f);
}


static void
Init(void)
{
   GLdouble vertTime, fragTime, linkTime;
   struct config_file config;

   memset(&config, 0, sizeof(config));

   if (ConfigFile)
      ReadConfigFile(ConfigFile, &config);

   if (!VertShaderFile) {
      fprintf(stderr, "Error: no vertex shader\n");
      exit(1);
   }

   if (!FragShaderFile) {
      fprintf(stderr, "Error: no fragment shader\n");
      exit(1);
   }

   if (!ShadersSupported())
      exit(1);

   vertShader = CompileShaderFile(GL_VERTEX_SHADER, VertShaderFile);
   vertTime = GetShaderCompileTime();
   fragShader = CompileShaderFile(GL_FRAGMENT_SHADER, FragShaderFile);
   fragTime = GetShaderCompileTime();

   Program = LinkShaders(vertShader, fragShader);
   linkTime = GetShaderLinkTime();

   printf("Time to compile vertex shader: %fs\n", vertTime);
   printf("Time to compile fragment shader: %fs\n", fragTime);
   printf("Time to link shaders: %fs\n", linkTime);

   glUseProgram(Program);

   NumUniforms = GetUniforms(Program, Uniforms);
   if (config.num_uniforms) {
      InitUniforms(&config, Uniforms);
   }
   else {
      RandomUniformValues();
   }
   SetUniformValues(Program, Uniforms);
   PrintUniforms(Uniforms);

   NumAttribs = GetAttribs(Program, Attribs);
   PrintAttribs(Attribs);

   //assert(glGetError() == 0);

   glClearColor(0.4f, 0.4f, 0.8f, 0.0f);

   glEnable(GL_DEPTH_TEST);

   glColor3f(1, 0, 0);
}


static void
Keys(void)
{
   printf("Keyboard:\n");
   printf("       a  Animation toggle\n");
   printf("       r  Randomize uniform values\n");
   printf("       o  Change object\n");
   printf("  arrows  Rotate object\n");
   printf("     ESC  Exit\n");
}


static void
Usage(void)
{
   printf("Usage:\n");
   printf("   shtest config.shtest\n");
   printf("       Run w/ given config file.\n");
   printf("   shtest --vs vertShader --fs fragShader\n");
   printf("       Load/compile given shaders.\n");
}


static void
ParseOptions(int argc, char *argv[])
{
   int i;

   if (argc == 1) {
      Usage();
      exit(1);
   }

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--fs") == 0) {
         FragShaderFile = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "--vs") == 0) {
         VertShaderFile = argv[i+1];
         i++;
      }
      else {
         /* assume the arg is a config file */
         ConfigFile = argv[i];
         break;
      }
   }
}


int
main(int argc, char *argv[])
{
   glutInitWindowSize(400, 400);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   ParseOptions(argc, argv);
   Init();
   Keys();
   glutMainLoop();
   return 0;
}

