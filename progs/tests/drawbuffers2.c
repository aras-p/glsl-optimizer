/*
 * Test GL_ARB_draw_buffers2, GL_ARB_draw_buffers, GL_EXT_framebuffer_object
 * and GLSL's gl_FragData[].
 *
 * We draw to two color buffers and show the left half of the first
 * color buffer on the left side of the window, and show the right
 * half of the second color buffer on the right side of the window.
 *
 * Different color masks are used for the two color buffers.
 * Blending is enabled for the second buffer only.
 *
 * Brian Paul
 * 31 Dec 2009
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "extfuncs.h"

static int Win;
static int Width = 400, Height = 400;
static GLuint FBobject, RBobjects[3];
static GLfloat Xrot = 0.0, Yrot = 0.0;
static GLuint Program;
static GLboolean Anim = GL_TRUE;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void
Display(void)
{
   GLubyte *buffer = malloc(Width * Height * 4);
   static const GLenum buffers[2] = {
      GL_COLOR_ATTACHMENT0_EXT,
      GL_COLOR_ATTACHMENT1_EXT
   };

   glUseProgram_func(Program);

   glEnable(GL_DEPTH_TEST);

   /* draw to user framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FBobject);

   /* Clear color buffer 0 (blue) */
   glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
   glClear(GL_COLOR_BUFFER_BIT);

   /* Clear color buffer 1 (1 - blue) */
   glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glClear(GL_COLOR_BUFFER_BIT);

   glClear(GL_DEPTH_BUFFER_BIT);

   /* draw to two buffers w/ fragment shader */
   glDrawBuffersARB(2, buffers);

   /* different color masks for each buffer */
   if (1) {
   glColorMaskIndexedEXT(0, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
   glColorMaskIndexedEXT(1, GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
   }

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glPushMatrix();
   glTranslatef(1, 0, 0);
   glutSolidTorus(1.0, 2.0, 10, 20);
   glPopMatrix();
   glPushMatrix();
   glTranslatef(-1, 0, 0);
   glRotatef(90, 1, 0, 0);
   glutSolidTorus(1.0, 2.0, 10, 20);
   glPopMatrix();
   glPopMatrix();

   /* restore default color masks */
   glColorMaskIndexedEXT(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glColorMaskIndexedEXT(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   /* read from user framebuffer */
   /* left half = colorbuffer 0 */
   glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
   glPixelStorei(GL_PACK_ROW_LENGTH, Width);
   glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
   glReadPixels(0, 0, Width / 2, Height, GL_RGBA, GL_UNSIGNED_BYTE,
                buffer);

   /* right half = colorbuffer 1 */
   glReadBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glPixelStorei(GL_PACK_SKIP_PIXELS, Width / 2);
   glReadPixels(Width / 2, 0, Width - Width / 2, Height,
                GL_RGBA, GL_UNSIGNED_BYTE,
                buffer);

   /* draw to window */
   glUseProgram_func(0);
   glDisable(GL_DEPTH_TEST);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glWindowPos2iARB(0, 0);
   glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

   free(buffer);
   glutSwapBuffers();
   CheckError(__LINE__);
}


static void
Idle(void)
{
   Xrot = glutGet(GLUT_ELAPSED_TIME) * 0.05;
   glutPostRedisplay();
}


static void
Reshape(int width, int height)
{
   float ar = (float) width / (float) height;

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 35.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -20.0);

   Width = width;
   Height = height;

   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBobjects[0]);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBobjects[1]);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBobjects[2]);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                            Width, Height);
}


static void
CleanUp(void)
{
   glDeleteFramebuffersEXT(1, &FBobject);
   glDeleteRenderbuffersEXT(3, RBobjects);
   glutDestroyWindow(Win);
   exit(0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case ' ':
      Anim = !Anim;
      glutIdleFunc(Anim ? Idle : NULL);
      break;
   case 'x':
      Xrot += 5.0;
      break;
   case 'X':
      Xrot -= 5.0;
      break;
   case 'y':
      Yrot += 5.0;
      break;
   case 'Y':
      Yrot -= 5.0;
      break;
   case 27:
      CleanUp();
      break;
   }
   glutPostRedisplay();
}


static void
CheckExtensions(void)
{
   const char *req[] = {
      "GL_EXT_framebuffer_object",
      "GL_ARB_draw_buffers",
      "GL_EXT_draw_buffers2"
   };

   const char *version = (const char *) glGetString(GL_VERSION);
   GLint numBuf;
   GLint i;

   for (i = 0; i < 3; i++) {
      if (!glutExtensionSupported(req[i])) {
         printf("Sorry, %s extension is required!\n", req[i]);
         exit(1);
      }
   }
   if (version[0] != '2') {
      printf("Sorry, OpenGL 2.0 is required!\n");
      exit(1);
   }

   glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &numBuf);
   printf("GL_MAX_DRAW_BUFFERS_ARB = %d\n", numBuf);
   if (numBuf < 2) {
      printf("Sorry, GL_MAX_DRAW_BUFFERS_ARB needs to be >= 2\n");
      exit(1);
   }

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
}


static void
SetupRenderbuffers(void)
{
   glGenFramebuffersEXT(1, &FBobject);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FBobject);

   glGenRenderbuffersEXT(3, RBobjects);

   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBobjects[0]);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);

   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBobjects[1]);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);

   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBobjects[2]);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                            Width, Height);

   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                GL_RENDERBUFFER_EXT, RBobjects[0]);
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                                GL_RENDERBUFFER_EXT, RBobjects[1]);
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                GL_RENDERBUFFER_EXT, RBobjects[2]);

   CheckError(__LINE__);
}


static GLuint
LoadAndCompileShader(GLenum target, const char *text)
{
   GLint stat;
   GLuint shader = glCreateShader_func(target);
   glShaderSource_func(shader, 1, (const GLchar **) &text, NULL);
   glCompileShader_func(shader);
   glGetShaderiv_func(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog_func(shader, 1000, &len, log);
      fprintf(stderr, "drawbuffers: problem compiling shader:\n%s\n", log);
      exit(1);
   }
   return shader;
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
      fprintf(stderr, "drawbuffers: shader link error:\n%s\n", log);
   }
}


static void
SetupShaders(void)
{
   /* emit same color to both draw buffers */
   static const char *fragShaderText =
      "void main() {\n"
      "   gl_FragData[0] = gl_Color; \n"
      "   gl_FragData[1] = gl_Color; \n"
      "}\n";

   GLuint fragShader;

   fragShader = LoadAndCompileShader(GL_FRAGMENT_SHADER, fragShaderText);
   Program = glCreateProgram_func();

   glAttachShader_func(Program, fragShader);
   glLinkProgram_func(Program);
   CheckLink(Program);
   glUseProgram_func(Program);
}


static void
SetupLighting(void)
{
   static const GLfloat ambient[4] = { 0.0, 0.0, 0.0, 0.0 };
   static const GLfloat diffuse[4] = { 1.0, 1.0, 1.0, 0.75 };

   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);

   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHTING);
}


static void
Init(void)
{
   CheckExtensions();
   GetExtensionFuncs();
   SetupRenderbuffers();
   SetupShaders();
   SetupLighting();
   glEnable(GL_DEPTH_TEST);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnableIndexedEXT(GL_BLEND, 1);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutIdleFunc(Anim ? Idle : NULL);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   Init();
   glutMainLoop();
   return 0;
}
