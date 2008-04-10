/**
 * Implement glRasterPos + glBitmap with textures + shaders.
 * Brian Paul
 * 14 May 2007
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
#include "shaderutil.h"


static GLuint FragShader;
static GLuint VertShader;
static GLuint Program;

static GLint Win = 0;
static GLint WinWidth = 500, WinHeight = 500;
static GLboolean Anim = GL_TRUE;
static GLboolean Bitmap = GL_FALSE;
static GLfloat Xrot = 20.0f, Yrot = 70.0f;
static GLint uTex, uScale;
static GLuint Textures[2];

#define TEX_WIDTH 16
#define TEX_HEIGHT 8


static void
BitmapText(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
Redisplay(void)
{
   static const GLfloat px[3] = { 1.2, 0, 0};
   static const GLfloat nx[3] = {-1.2, 0, 0};

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(Xrot, 1.0f, 0.0f, 0.0f);
   glRotatef(Yrot, 0.0f, 1.0f, 0.0f);

   glEnable(GL_LIGHTING);

   glPushMatrix();
   glScalef(0.5, 0.5, 0.5);
   glutSolidDodecahedron();
   glPopMatrix();

   glDisable(GL_LIGHTING);

   glColor3f(0, 1, 0);
   glBegin(GL_LINES);
   glVertex3f(-1, 0, 0);
   glVertex3f( 1, 0, 0);
   glEnd();

   glColor3f(1, 1, 0);

   if (Bitmap) {
      glRasterPos3fv(px);
      BitmapText("+X");
      glRasterPos3fv(nx);
      BitmapText("-X");
   }
   else {
      glUseProgram_func(Program);

      /* vertex positions (deltas) depend on texture size and window size */
      if (uScale != -1) {
         glUniform2f_func(uScale,
                          2.0 * TEX_WIDTH / WinWidth,
                          2.0 * TEX_HEIGHT / WinHeight);
      }

      /* draw +X */
      glBindTexture(GL_TEXTURE_2D, Textures[0]);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);  glVertex3fv(px);
      glTexCoord2f(1, 0);  glVertex3fv(px);
      glTexCoord2f(1, 1);  glVertex3fv(px);
      glTexCoord2f(0, 1);  glVertex3fv(px);
      glEnd();

      /* draw -X */
      glBindTexture(GL_TEXTURE_2D, Textures[1]);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);  glVertex3fv(nx);
      glTexCoord2f(1, 0);  glVertex3fv(nx);
      glTexCoord2f(1, 1);  glVertex3fv(nx);
      glTexCoord2f(0, 1);  glVertex3fv(nx);
      glEnd();

      glUseProgram_func(0);
   }

   glPopMatrix();

   glutSwapBuffers();
}


static void
Idle(void)
{
   Yrot = glutGet(GLUT_ELAPSED_TIME) * 0.01;
   glutPostRedisplay();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0f, 0.0f, -10.0f);
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
      if (Anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'b':
      Bitmap = !Bitmap;
      if (Bitmap)
         printf("Using glBitmap\n");
      else
         printf("Using billboard texture\n");
      break;
   case 27:
      glDeleteShader_func(FragShader);
      glDeleteShader_func(VertShader);
      glDeleteProgram_func(Program);
      glutDestroyWindow(Win);
      exit(0);
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 0.125f;
   switch(key) {
   case GLUT_KEY_UP:
      Xrot -= step;
      break;
   case GLUT_KEY_DOWN:
      Xrot += step;
      break;
   case GLUT_KEY_LEFT:
      Yrot -= step;
      break;
   case GLUT_KEY_RIGHT:
      Yrot += step;
      break;
   }
   /*printf("Xrot: %f  Yrot: %f\n", Xrot, Yrot);*/
   glutPostRedisplay();
}


static void
MakeTexImage(const char *p, GLuint texobj)
{
   GLubyte image[TEX_HEIGHT][TEX_WIDTH];
   GLuint i, j, k;

   for (i = 0; i < TEX_HEIGHT; i++) {
      for (j = 0; j < TEX_WIDTH; j++) {
         k = i * TEX_WIDTH + j;
         if (p[k] == ' ') {
            image[i][j] = 0;
         }
         else {
            image[i][j] = 255;
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, texobj);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, TEX_WIDTH, TEX_HEIGHT, 0,
                GL_RED, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


static void
MakeBitmapTextures(void)
{
   const char *px =
      "        X     X "
      "   X     X   X  "
      "   X      X X   "
      " XXXXX     X    "
      "   X      X X   "
      "   X     X   X  "
      "        X     X "
      "        X     X ";
   const char *nx =
      "        X     X "
      "         X   X  "
      "          X X   "
      " XXXXX     X    "
      "          X X   "
      "         X   X  "
      "        X     X "
      "        X     X ";
   glGenTextures(2, Textures);
   MakeTexImage(px, Textures[0]);
   MakeTexImage(nx, Textures[1]);
}


static void
Init(void)
{
   /* Fragment shader: modulate raster color by texture, discard fragments
    * with alpha < 1.0
    */
   static const char *fragShaderText =
      "uniform sampler2D tex2d; \n"
      "void main() {\n"
      "   vec4 c = texture2D(tex2d, gl_TexCoord[0].xy); \n"
      "   if (c.w < 1.0) \n"
      "      discard; \n"
      "   gl_FragColor = c * gl_Color; \n"
      "}\n";
   /* Vertex shader: compute new vertex position based on incoming vertex pos,
    * texcoords and special scale factor.
    */
   static const char *vertShaderText =
      "uniform vec2 scale; \n"
      "void main() {\n"
      "   vec4 p = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   gl_Position.xy = p.xy + gl_MultiTexCoord0.xy * scale * p.w; \n"
      "   gl_Position.zw = p.zw; \n"
      "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
      "   gl_FrontColor = gl_Color; \n"
      "}\n";

   if (!ShadersSupported())
      exit(1);

   GetExtensionFuncs();

   VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   Program = LinkShaders(VertShader, FragShader);

   glUseProgram_func(Program);

   uScale = glGetUniformLocation_func(Program, "scale");
   uTex = glGetUniformLocation_func(Program, "tex2d");
   if (uTex != -1) {
      glUniform1i_func(uTex, 0);  /* tex unit 0 */
   }

   glUseProgram_func(0);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_NORMALIZE);
   glEnable(GL_LIGHT0);

   MakeBitmapTextures();
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}


