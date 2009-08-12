/*
 * Test floating point textures.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "extfuncs.h"
#include "readtex.h"
#include "shaderutil.h"


static const char *TexFile = "../images/arch.rgb";

static const char *FragShaderText = 
   "uniform sampler2D tex1; \n"
   "void main() \n"
   "{ \n"
   "   vec4 t = texture2D(tex1, gl_TexCoord[0].xy); \n"
   "   // convert from [-255,0] to [0,1] \n"
   "   gl_FragColor = t * (-1.0 / 255.0); \n"
   "} \n";

static const char *VertShaderText = 
   "void main() \n"
   "{ \n"
   "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
   "   gl_Position = ftransform(); \n"
   "} \n";

static struct uniform_info Uniforms[] = {
   { "tex1",  1, GL_INT, { 0, 0, 0, 0 }, -1 },
   END_OF_UNIFORMS
};


static GLuint Program;



static GLboolean
CheckError( int line )
{
   GLenum error = glGetError();
   if (error) {
      char *err = (char *) gluErrorString( error );
      fprintf( stderr, "GL Error: %s at line %d\n", err, line );
      return GL_TRUE;
   }
   return GL_FALSE;
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();

   glBegin(GL_POLYGON);
   glTexCoord2f( 0.0, 0.0 );   glVertex2f( -1.0, -1.0 );
   glTexCoord2f( 1.0, 0.0 );   glVertex2f(  1.0, -1.0 );
   glTexCoord2f( 1.0, 1.0 );   glVertex2f(  1.0,  1.0 );
   glTexCoord2f( 0.0, 1.0 );   glVertex2f( -1.0,  1.0 );
   glEnd();

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
   glTranslatef(0.0, 0.0, -8.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
InitTexture(void)
{
   GLenum filter = GL_LINEAR;
   GLint imgWidth, imgHeight;
   GLenum imgFormat;
   GLubyte *image = NULL;
   GLfloat *ftex;
   GLint i, t;

   image = LoadRGBImage(TexFile, &imgWidth, &imgHeight, &imgFormat);
   if (!image) {
      printf("Couldn't read %s\n", TexFile);
      exit(0);
   }

   assert(imgFormat == GL_RGB);

   ftex = (float *) malloc(imgWidth * imgHeight * 4 * sizeof(float));
   if (!ftex) {
      printf("out of memory\n");
      exit(0);
   }

   /* convert ubytes to floats, negated */
   for (i = 0; i < imgWidth * imgHeight * 3; i++) {
      ftex[i] = -1.0f * image[i];
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 42);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB,
                imgWidth, imgHeight, 0,
                GL_RGB, GL_FLOAT, ftex);


   CheckError(__LINE__);

   /* sanity checks */
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_TYPE_ARB, &t);
   assert(t == GL_FLOAT);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_TYPE_ARB, &t);
   assert(t == GL_FLOAT);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_TYPE_ARB, &t);
   assert(t == GL_FLOAT);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_TYPE_ARB, &t);
   assert(t == GL_FLOAT);

   free(image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

   if (1) {
      /* read back the texture and make sure values are correct */
      GLfloat *tex2 = (GLfloat *)
         malloc(imgWidth * imgHeight * 4 * sizeof(GLfloat));
      glGetTexImage(GL_TEXTURE_2D, 0, imgFormat, GL_FLOAT, tex2);
      CheckError(__LINE__);
      for (i = 0; i < imgWidth * imgHeight * 4; i++) {
         if (ftex[i] != tex2[i]) {
            printf("tex[%d] %g != tex2[%d] %g\n",
                   i, ftex[i], i, tex2[i]);
         }
      }
   }

   free(ftex);
}


static GLuint
CreateProgram(void)
{
   GLuint fragShader, vertShader, program;

   vertShader = CompileShaderText(GL_VERTEX_SHADER, VertShaderText);
   fragShader = CompileShaderText(GL_FRAGMENT_SHADER, FragShaderText);
   assert(vertShader);
   program = LinkShaders(vertShader, fragShader);

   assert(program);

   glUseProgram_func(program);

   SetUniformValues(program, Uniforms);

   return program;
}


static void
Init(void)
{
   glClearColor(0.25, 0.25, 0.25, 0.0);

   GetExtensionFuncs();

   if (!ShadersSupported()) {
      printf("Sorry, this test requires GLSL\n");
      exit(1);
   }

   if (!glutExtensionSupported("GL_MESAX_texture_float") &&
       !glutExtensionSupported("GL_ARB_texture_float")) {
      printf("Sorry, this test requires GL_MESAX/ARB_texture_float\n");
      exit(1);
   }

   InitTexture();

   Program = CreateProgram();
   glUseProgram_func(Program);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
