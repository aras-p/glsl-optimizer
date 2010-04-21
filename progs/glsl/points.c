/**
 * Implement smooth (AA) points with shaders.
 * A simple variation could be used for sprite points.
 * Brian Paul
 * 29 July 2007
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static GLuint FragShader;
static GLuint VertShader;
static GLuint Program;

static GLint Win = 0;
static GLint WinWidth = 500, WinHeight = 200;
static GLfloat Xpos = 0.0f, Ypos = 0.0f;
static GLint uViewportInv;
static GLboolean Smooth = GL_TRUE, Blend = GL_TRUE;


/**
 * Issue vertices for a "shader point".
 * The position is duplicated, only texcoords (or other vertex attrib) change.
 * The vertex program will compute the "real" quad corners.
 */
static void
PointVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
   glTexCoord2f(-1, -1);
   glVertex3f(x, y, z);

   glTexCoord2f( 1, -1);
   glVertex3f(x, y, z);

   glTexCoord2f( 1,  1);
   glVertex3f(x, y, z);

   glTexCoord2f(-1,  1);
   glVertex3f(x, y, z);
}


static void
DrawPoints(GLboolean shaderPoints)
{
   int i;
   for (i = 0; i < 9; i++) {
      GLfloat x = i - 4, y = 0, z = 0;
      /* note: can't call glPointSize inside Begin/End :( */
      glPointSize( 2 + i * 5 );
      if (shaderPoints) {
         glBegin(GL_QUADS);
         PointVertex3f(x, y, z);
         glEnd();
      }
      else {
         glBegin(GL_POINTS);
         glVertex3f(x, y, z);
         glEnd();
      }
   }
}


/**
 * Top row of points is rendered conventionally with GL_POINT_SMOOTH.
 * Bottom row is rendered with special vertex/fragment shaders (see Init()).
 */
static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (Smooth)
      glEnable(GL_POINT_SMOOTH);
   else
      glDisable(GL_POINT_SMOOTH);

   if (Blend)
      glEnable(GL_BLEND);
   else
      glDisable(GL_BLEND);

   glPushMatrix();
   glTranslatef(Xpos, Ypos, 0);

   /*
    * regular points
    */
   glPushMatrix();
   glTranslatef(0, 1.2, 0);
   glUseProgram(0);
   DrawPoints(GL_FALSE);
   glPopMatrix();

   /*
    * shader points
    */
   glPushMatrix();
   glTranslatef(0, -1.2, 0);
   glUseProgram(Program);
   if (uViewportInv != -1) {
      glUniform2f(uViewportInv, 1.0 / WinWidth, 1.0 / WinHeight);
   }
   DrawPoints(GL_TRUE);
   glPopMatrix();

   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 4.0, 30.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0f, 0.0f, -20.0f);
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;

   switch(key) {
   case 'b':
      Blend = !Blend;
      break;
   case 's':
      Smooth = !Smooth;
      break;
   case 27:
      glDeleteShader(FragShader);
      glDeleteShader(VertShader);
      glDeleteProgram(Program);
      glutDestroyWindow(Win);
      exit(0);
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 1/100.0;
   switch(key) {
   case GLUT_KEY_UP:
      Ypos += step;
      break;
   case GLUT_KEY_DOWN:
      Ypos -= step;
      break;
   case GLUT_KEY_LEFT:
      Xpos -= step;
      break;
   case GLUT_KEY_RIGHT:
      Xpos += step;
      break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   /* Fragment shader: compute distance of fragment from center of point
    * (we're using texcoords but another varying could be used).
    *   if dist > 1, discard (coverage==0)
    *   if dist < k, coverage = 1
    *   else, coverage = func(dist)
    * Note: length() uses sqrt() and may be expensive.  The distance could
    * be squared instead (with adjustments to the threshold (k) test)
    */
   static const char *fragShaderText =
      "void main() {\n"
      "   float cover; \n"
      "   float k = 2.0 / gl_Point.size; \n"
      "   float d = length(gl_TexCoord[0].xy); \n"
      "   if (d >= 1.0) \n"
      "      discard; \n"
      "   if (d < 1.0 - k) \n"
      "      cover = 1.0; \n"
      "   else \n"
      "      cover = (1.0 - d) * 0.5 * gl_Point.size; \n"
      "   gl_FragColor.rgb = gl_Color.rgb; \n"
      "   gl_FragColor.a = cover; \n"
      "}\n";
   /* Vertex shader: compute new vertex position based on incoming vertex pos,
    * texcoords, point size, and inverse viewport scale factor.
    * Note: should compute point size attenuation here too.
    */
   static const char *vertShaderText =
      "uniform vec2 viewportInv; \n"
      "void main() {\n"
      "   vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   gl_Position.xy = pos.xy + gl_MultiTexCoord0.xy * viewportInv \n"
      "                    * gl_Point.size * pos.w; \n"
      "   gl_Position.zw = pos.zw; \n"
      "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
      "   gl_FrontColor = gl_Color; \n"
      "}\n";

   if (!ShadersSupported())
      exit(1);

   VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   Program = LinkShaders(VertShader, FragShader);

   glUseProgram(Program);

   uViewportInv = glGetUniformLocation(Program, "viewportInv");

   glUseProgram(0);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   Init();
   glutMainLoop();
   return 0;
}


