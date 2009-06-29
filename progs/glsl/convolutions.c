/**
 * Convolution with GLSL.
 * Note: uses GL_ARB_shader_objects, GL_ARB_vertex_shader, GL_ARB_fragment_shader,
 * not the OpenGL 2.0 shader API.
 * Author: Zack Rusin
 */

#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include "readtex.h"

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

enum Filter {
   GAUSSIAN_BLUR,
   SHARPEN,
   MEAN_REMOVAL,
   EMBOSS,
   EDGE_DETECT,
   NO_FILTER,
   LAST
};
#define QUIT LAST

struct BoundingBox {
   float minx, miny, minz;
   float maxx, maxy, maxz;
};
struct Texture {
   GLuint id;
   GLfloat x;
   GLfloat y;
   GLint width;
   GLint height;
   GLenum format;
};

static const char *textureLocation = "../images/girl2.rgb";

static GLfloat viewRotx = 0.0, viewRoty = 0.0, viewRotz = 0.0;
static struct BoundingBox box;
static struct Texture texture;
static GLuint program;
static GLint menuId;
static enum Filter filter = GAUSSIAN_BLUR;


static void checkError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error %s (0x%x) at line %d\n",
             gluErrorString(err), (int) err, line);
   }
}

static void loadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;

   glShaderSource(shader, 1, (const GLchar **) &text, NULL);

   glCompileShader(shader);

   glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "Problem compiling shader: %s\n", log);
      exit(1);
   }
   else {
      printf("Shader compiled OK\n");
   }
}

static void readShader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Unable to open shader file %s\n", filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("Read %d bytes from shader file %s\n", n, filename);
   if (n > 0) {
      buffer[n] = 0;
      loadAndCompileShader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}


static void
checkLink(GLuint prog)
{
   GLint stat;
   glGetProgramiv(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog(prog, 1000, &len, log);
      fprintf(stderr, "Linker error:\n%s\n", log);
   }
   else {
      fprintf(stderr, "Link success!\n");
   }
}

static void fillConvolution(GLint *k,
                            GLfloat *scale,
                            GLfloat *color)
{
   switch(filter) {
   case GAUSSIAN_BLUR:
      k[0] = 1; k[1] = 2; k[2] = 1;
      k[3] = 2; k[4] = 4; k[5] = 2;
      k[6] = 1; k[7] = 2; k[8] = 1;

      *scale = 1./16.;
      break;
   case SHARPEN:
      k[0] =  0; k[1] = -2; k[2] =  0;
      k[3] = -2; k[4] = 11; k[5] = -2;
      k[6] =  0; k[7] = -2; k[8] =  0;

      *scale = 1./3.;
      break;
   case MEAN_REMOVAL:
      k[0] = -1; k[1] = -1; k[2] = -1;
      k[3] = -1; k[4] =  9; k[5] = -1;
      k[6] = -1; k[7] = -1; k[8] = -1;

      *scale = 1./1.;
      break;
   case EMBOSS:
      k[0] = -1; k[1] =  0; k[2] = -1;
      k[3] =  0; k[4] =  4; k[5] =  0;
      k[6] = -1; k[7] =  0; k[8] = -1;

      *scale = 1./1.;
      color[0] = 0.5;
      color[1] = 0.5;
      color[2] = 0.5;
      color[3] = 0.5;
      break;
   case EDGE_DETECT:
      k[0] =  1; k[1] =  1; k[2] =  1;
      k[3] =  0; k[4] =  0; k[5] =  0;
      k[6] = -1; k[7] = -1; k[8] = -1;

      *scale = 1.;
      color[0] = 0.5;
      color[1] = 0.5;
      color[2] = 0.5;
      color[3] = 0.5;
      break;
   case NO_FILTER:
      k[0] =  0; k[1] =  0; k[2] =  0;
      k[3] =  0; k[4] =  1; k[5] =  0;
      k[6] =  0; k[7] =  0; k[8] =  0;

      *scale = 1.;
      break;
   default:
      assert(!"Unhandled switch value");
   }
}

static void setupConvolution()
{
   GLint *kernel = (GLint*)malloc(sizeof(GLint) * 9);
   GLfloat scale;
   GLfloat *vecKer = (GLfloat*)malloc(sizeof(GLfloat) * 9 * 4);
   GLuint loc;
   GLuint i;
   GLfloat baseColor[4];
   baseColor[0] = 0;
   baseColor[1] = 0;
   baseColor[2] = 0;
   baseColor[3] = 0;

   fillConvolution(kernel, &scale, baseColor);
   /*vector of 4*/
   for (i = 0; i < 9; ++i) {
      vecKer[i*4 + 0] = kernel[i];
      vecKer[i*4 + 1] = kernel[i];
      vecKer[i*4 + 2] = kernel[i];
      vecKer[i*4 + 3] = kernel[i];
   }

   loc = glGetUniformLocationARB(program, "KernelValue");
   glUniform4fv(loc, 9, vecKer);
   loc = glGetUniformLocationARB(program, "ScaleFactor");
   glUniform4f(loc, scale, scale, scale, scale);
   loc = glGetUniformLocationARB(program, "BaseColor");
   glUniform4f(loc, baseColor[0], baseColor[1],
               baseColor[2], baseColor[3]);

   free(vecKer);
   free(kernel);
}

static void createProgram(const char *vertProgFile,
                          const char *fragProgFile)
{
   GLuint fragShader = 0, vertShader = 0;

   program = glCreateProgram();
   if (vertProgFile) {
      vertShader = glCreateShader(GL_VERTEX_SHADER);
      readShader(vertShader, vertProgFile);
      glAttachShader(program, vertShader);
   }

   if (fragProgFile) {
      fragShader = glCreateShader(GL_FRAGMENT_SHADER);
      readShader(fragShader, fragProgFile);
      glAttachShader(program, fragShader);
   }

   glLinkProgram(program);
   checkLink(program);

   glUseProgram(program);

   /*
   assert(glIsProgram(program));
   assert(glIsShader(fragShader));
   assert(glIsShader(vertShader));
   */

   checkError(__LINE__);
   {/*texture*/
      GLuint texLoc = glGetUniformLocationARB(program, "srcTex");
      glUniform1iARB(texLoc, 0);
   }
   {/*setup offsets */
      float offsets[] = { 1.0 / texture.width,  1.0 / texture.height,
                          0.0                ,  1.0 / texture.height,
                          -1.0 / texture.width,  1.0 / texture.height,
                          1.0 / texture.width,  0.0,
                          0.0                ,  0.0,
                          -1.0 / texture.width,  0.0,
                          1.0 / texture.width, -1.0 / texture.height,
                          0.0                , -1.0 / texture.height,
                          -1.0 / texture.width, -1.0 / texture.height };
      GLuint offsetLoc = glGetUniformLocationARB(program, "Offset");
      glUniform2fv(offsetLoc, 9, offsets);
   }
   setupConvolution();

   checkError(__LINE__);
}


static void readTexture(const char *filename)
{
   GLubyte *data;

   texture.x = 0;
   texture.y = 0;

   glGenTextures(1, &texture.id);
   glBindTexture(GL_TEXTURE_2D, texture.id);
   glTexParameteri(GL_TEXTURE_2D,
                   GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,
                   GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   data = LoadRGBImage(filename, &texture.width, &texture.height,
                       &texture.format);
   if (!data) {
      printf("Error: couldn't load texture image '%s'\n", filename);
      exit(1);
   }
   printf("Texture %s (%d x %d)\n",
          filename, texture.width, texture.height);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                texture.width, texture.height, 0, texture.format,
                GL_UNSIGNED_BYTE, data);
}

static void menuSelected(int entry)
{
   switch (entry) {
   case QUIT:
      exit(0);
      break;
   default:
      filter = (enum Filter)entry;
   }
   setupConvolution();

   glutPostRedisplay();
}

static void menuInit()
{
   menuId = glutCreateMenu(menuSelected);

   glutAddMenuEntry("Gaussian blur", GAUSSIAN_BLUR);
   glutAddMenuEntry("Sharpen", SHARPEN);
   glutAddMenuEntry("Mean removal", MEAN_REMOVAL);
   glutAddMenuEntry("Emboss", EMBOSS);
   glutAddMenuEntry("Edge detect", EDGE_DETECT);
   glutAddMenuEntry("None", NO_FILTER);

   glutAddMenuEntry("Quit", QUIT);

   glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void init()
{
   if (!glutExtensionSupported("GL_ARB_shader_objects") ||
       !glutExtensionSupported("GL_ARB_vertex_shader") ||
       !glutExtensionSupported("GL_ARB_fragment_shader")) {
      fprintf(stderr, "Sorry, this program requires GL_ARB_shader_objects, GL_ARB_vertex_shader, and GL_ARB_fragment_shader\n");
      exit(1);
   }

   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));

   menuInit();
   readTexture(textureLocation);
   createProgram("convolution.vert", "convolution.frag");

   glEnable(GL_TEXTURE_2D);
   glClearColor(1.0, 1.0, 1.0, 1.0);
   /*glShadeModel(GL_SMOOTH);*/
   glShadeModel(GL_FLAT);
}

static void reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   box.minx = 0;
   box.maxx = width;
   box.miny = 0;
   box.maxy = height;
   box.minz = 0;
   box.maxz = 1;
   glOrtho(box.minx, box.maxx, box.miny, box.maxy, -999999, 999999);
   glMatrixMode(GL_MODELVIEW);
}

static void keyPress(unsigned char key, int x, int y)
{
   switch(key) {
   case 27:
      exit(0);
   default:
      return;
   }
   glutPostRedisplay();
}

static void
special(int k, int x, int y)
{
   switch (k) {
   case GLUT_KEY_UP:
      viewRotx += 2.0;
      break;
   case GLUT_KEY_DOWN:
      viewRotx -= 2.0;
      break;
   case GLUT_KEY_LEFT:
      viewRoty += 2.0;
      break;
   case GLUT_KEY_RIGHT:
      viewRoty -= 2.0;
      break;
   default:
      return;
   }
   glutPostRedisplay();
}


static void draw()
{
   GLfloat center[2];
   GLfloat anchor[2];

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glLoadIdentity();
   glPushMatrix();

   center[0] = box.maxx/2;
   center[1] = box.maxy/2;
   anchor[0] = center[0] - texture.width/2;
   anchor[1] = center[1] - texture.height/2;

   glTranslatef(center[0], center[1], 0);
   glRotatef(viewRotx, 1.0, 0.0, 0.0);
   glRotatef(viewRoty, 0.0, 1.0, 0.0);
   glRotatef(viewRotz, 0.0, 0.0, 1.0);
   glTranslatef(-center[0], -center[1], 0);

   glTranslatef(anchor[0], anchor[1], 0);
   glBegin(GL_TRIANGLE_STRIP);
   {
      glColor3f(1., 0., 0.);
      glTexCoord2f(0, 0);
      glVertex3f(0, 0, 0);

      glColor3f(0., 1., 0.);
      glTexCoord2f(0, 1.0);
      glVertex3f(0, texture.height, 0);

      glColor3f(1., 0., 0.);
      glTexCoord2f(1.0, 0);
      glVertex3f(texture.width, 0, 0);

      glColor3f(0., 1., 0.);
      glTexCoord2f(1, 1);
      glVertex3f(texture.width, texture.height, 0);
   }
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}

int main(int argc, char **argv)
{
   glutInit(&argc, argv);

   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE);

   if (!glutCreateWindow("Image Convolutions")) {
      fprintf(stderr, "Couldn't create window!\n");
      exit(1);
   }

   glewInit();
   init();

   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyPress);
   glutSpecialFunc(special);
   glutDisplayFunc(draw);


   glutMainLoop();
   return 0;
}
