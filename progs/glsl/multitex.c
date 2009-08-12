/**
 * Test multi-texturing with GL shading language.
 *
 * Copyright (C) 2008  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include "GL/glut.h"
#include "readtex.h"
#include "shaderutil.h"

static const char *Demo = "multitex";

static const char *VertFile = "multitex.vert";
static const char *FragFile = "multitex.frag";

static const char *TexFiles[2] = 
   {
      "../images/tile.rgb",
      "../images/tree2.rgba"
   };


static GLuint Program;

static GLfloat Xrot = 0.0, Yrot = .0, Zrot = 0.0;
static GLfloat EyeDist = 10;
static GLboolean Anim = GL_TRUE;
static GLboolean UseArrays = GL_TRUE;
static GLboolean UseVBO = GL_TRUE;
static GLuint VBO = 0;

static GLint VertCoord_attr = -1, TexCoord0_attr = -1, TexCoord1_attr = -1;


/* value[0] = tex unit */
static struct uniform_info Uniforms[] = {
   { "tex1",  1, GL_INT, { 0, 0, 0, 0 }, -1 },
   { "tex2",  1, GL_INT, { 1, 0, 0, 0 }, -1 },
   END_OF_UNIFORMS
};


static const GLfloat Tex0Coords[4][2] = {
   { 0.0, 0.0 }, { 2.0, 0.0 }, { 2.0, 2.0 }, { 0.0, 2.0 }
};

static const GLfloat Tex1Coords[4][2] = {
   { 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 }, { 0.0, 1.0 }
};

static const GLfloat VertCoords[4][2] = {
   { -3.0, -3.0 }, { 3.0, -3.0 }, { 3.0, 3.0 }, { -3.0, 3.0 }
};



static void
SetupVertexBuffer(void)
{
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);

   glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                        sizeof(VertCoords) +
                        sizeof(Tex0Coords) +
                        sizeof(Tex1Coords),
                        NULL,
                        GL_STATIC_DRAW_ARB);

   /* non-interleaved vertex arrays */

   glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,
                           0,                   /* offset */
                           sizeof(VertCoords),  /* size */
                           VertCoords);         /* data */

   glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,
                           sizeof(VertCoords),  /* offset */
                           sizeof(Tex0Coords),  /* size */
                           Tex0Coords);         /* data */

   glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,
                           sizeof(VertCoords) +
                           sizeof(Tex0Coords),  /* offset */
                           sizeof(Tex1Coords),  /* size */
                           Tex1Coords);         /* data */

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}


static void
DrawPolygonArray(void)
{
   void *vertPtr, *tex0Ptr, *tex1Ptr;

   if (UseVBO) {
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
      vertPtr = (void *) 0;
      tex0Ptr = (void *) sizeof(VertCoords);
      tex1Ptr = (void *) (sizeof(VertCoords) + sizeof(Tex0Coords));
   }
   else {
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
      vertPtr = VertCoords;
      tex0Ptr = Tex0Coords;
      tex1Ptr = Tex1Coords;
   }

   if (VertCoord_attr >= 0) {
      glVertexAttribPointer(VertCoord_attr, 2, GL_FLOAT, GL_FALSE,
                                 0, vertPtr);
      glEnableVertexAttribArray(VertCoord_attr);
   }
   else {
      glVertexPointer(2, GL_FLOAT, 0, vertPtr);
      glEnableClientState(GL_VERTEX_ARRAY);
   }

   glVertexAttribPointer(TexCoord0_attr, 2, GL_FLOAT, GL_FALSE,
                              0, tex0Ptr);
   glEnableVertexAttribArray(TexCoord0_attr);

   glVertexAttribPointer(TexCoord1_attr, 2, GL_FLOAT, GL_FALSE,
                              0, tex1Ptr);
   glEnableVertexAttribArray(TexCoord1_attr);

   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}


static void
DrawPolygonVert(void)
{
   GLuint i;

   glBegin(GL_TRIANGLE_FAN);

   for (i = 0; i < 4; i++) {
      glVertexAttrib2fv(TexCoord0_attr, Tex0Coords[i]);
      glVertexAttrib2fv(TexCoord1_attr, Tex1Coords[i]);

      if (VertCoord_attr >= 0)
         glVertexAttrib2fv(VertCoord_attr, VertCoords[i]);
      else
         glVertex2fv(VertCoords[i]);
   }

   glEnd();
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix(); /* modelview matrix */
      glTranslatef(0.0, 0.0, -EyeDist);
      glRotatef(Zrot, 0, 0, 1);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Xrot, 1, 0, 0);

      if (UseArrays)
         DrawPolygonArray();
      else
         DrawPolygonVert();

   glPopMatrix();

   glutSwapBuffers();
}


static void
idle(void)
{
   GLfloat t = 0.05 * glutGet(GLUT_ELAPSED_TIME);
   Yrot = t;
   glutPostRedisplay();
}


static void
key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
   case 'a':
      UseArrays = !UseArrays;
      printf("Arrays: %d\n", UseArrays);
      break;
   case 'v':
      UseVBO = !UseVBO;
      printf("Use VBO: %d\n", UseVBO);
      break;
   case ' ':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'z':
      EyeDist -= 0.5;
      if (EyeDist < 3.0)
         EyeDist = 3.0;
      break;
   case 'Z':
      EyeDist += 0.5;
      if (EyeDist > 90.0)
         EyeDist = 90;
      break;
   case 27:
      exit(0);
   }
   glutPostRedisplay();
}


static void
specialkey(int key, int x, int y)
{
   GLfloat step = 2.0;
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      Xrot += step;
      break;
   case GLUT_KEY_DOWN:
      Xrot -= step;
      break;
   case GLUT_KEY_LEFT:
      Yrot -= step;
      break;
   case GLUT_KEY_RIGHT:
      Yrot += step;
      break;
   }
   glutPostRedisplay();
}


/* new window size or exposure */
static void
Reshape(int width, int height)
{
   GLfloat ar = (float) width / (float) height;
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-2.0*ar, 2.0*ar, -2.0, 2.0, 4.0, 100.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
InitTextures(void)
{
   GLenum filter = GL_LINEAR;
   int i;

   for (i = 0; i < 2; i++) {
      GLint imgWidth, imgHeight;
      GLenum imgFormat;
      GLubyte *image = NULL;

      image = LoadRGBImage(TexFiles[i], &imgWidth, &imgHeight, &imgFormat);
      if (!image) {
         printf("Couldn't read %s\n", TexFiles[i]);
         exit(0);
      }

      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, 42 + i);
      gluBuild2DMipmaps(GL_TEXTURE_2D, 4, imgWidth, imgHeight,
                        imgFormat, GL_UNSIGNED_BYTE, image);
      free(image);
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
   }
}


static GLuint
CreateProgram(const char *vertProgFile, const char *fragProgFile,
              struct uniform_info *uniforms)
{
   GLuint fragShader, vertShader, program;

   vertShader = CompileShaderFile(GL_VERTEX_SHADER, vertProgFile);
   fragShader = CompileShaderFile(GL_FRAGMENT_SHADER, fragProgFile);
   assert(vertShader);
   program = LinkShaders(vertShader, fragShader);

   glUseProgram(program);

   SetUniformValues(program, uniforms);
   PrintUniforms(Uniforms);

   VertCoord_attr = glGetAttribLocation(program, "VertCoord");
   if (VertCoord_attr > 0) {
      /* We want the VertCoord attrib to have position zero so that
       * the call to glVertexAttrib(0, xyz) triggers vertex processing.
       * Otherwise, if TexCoord0 or TexCoord1 gets position 0 we'd have
       * to set that attribute last (which is a PITA to manage).
       */
      glBindAttribLocation(program, 0, "VertCoord");
      /* re-link */
      glLinkProgram(program);
      /* VertCoord_attr should be zero now */
      VertCoord_attr = glGetAttribLocation(program, "VertCoord");
      assert(VertCoord_attr == 0);
   }

   TexCoord0_attr = glGetAttribLocation(program, "TexCoord0");
   TexCoord1_attr = glGetAttribLocation(program, "TexCoord1");

   printf("TexCoord0_attr = %d\n", TexCoord0_attr);
   printf("TexCoord1_attr = %d\n", TexCoord1_attr);
   printf("VertCoord_attr = %d\n", VertCoord_attr);

   return program;
}


static void
InitPrograms(void)
{
   Program = CreateProgram(VertFile, FragFile, Uniforms);
}


static void
InitGL(void)
{
   const char *version = (const char *) glGetString(GL_VERSION);

   if (version[0] != '2' || version[1] != '.') {
      printf("Warning: this program expects OpenGL 2.0\n");
      /*exit(1);*/
   }
   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));
   printf("Usage:\n");
   printf("  a     - toggle arrays vs. immediate mode rendering\n");
   printf("  v     - toggle VBO usage for array rendering\n");
   printf("  z/Z   - change viewing distance\n");
   printf("  SPACE - toggle animation\n");
   printf("  Esc   - exit\n");

   InitTextures();
   InitPrograms();

   SetupVertexBuffer();

   glEnable(GL_DEPTH_TEST);

   glClearColor(.6, .6, .9, 0);
   glColor3f(1.0, 1.0, 1.0);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(500, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutCreateWindow(Demo);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(key);
   glutSpecialFunc(specialkey);
   glutDisplayFunc(draw);
   if (Anim)
      glutIdleFunc(idle);
   InitGL();
   glutMainLoop();
   return 0;
}
