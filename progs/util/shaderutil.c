/**
 * Utilities for OpenGL shading language
 *
 * Brian Paul
 * 9 April 2008
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "extfuncs.h"
#include "shaderutil.h"


static void
Init(void)
{
   static GLboolean firstCall = GL_TRUE;
   if (firstCall) {
      GetExtensionFuncs();
      firstCall = GL_FALSE;
   }
}


GLboolean
ShadersSupported(void)
{
   const char *version = (const char *) glGetString(GL_VERSION);
   if (version[0] == '2' && version[1] == '.') {
      return GL_TRUE;
   }
   else if (glutExtensionSupported("GL_ARB_vertex_shader")
            && glutExtensionSupported("GL_ARB_fragment_shader")
            && glutExtensionSupported("GL_ARB_shader_objects")) {
      fprintf(stderr, "Warning: Trying ARB GLSL instead of OpenGL 2.x.  This may not work.\n");
      return GL_TRUE;
   }
   return GL_TRUE;
}


GLuint
CompileShaderText(GLenum shaderType, const char *text)
{
   GLuint shader;
   GLint stat;

   Init();

   shader = glCreateShader_func(shaderType);
   glShaderSource_func(shader, 1, (const GLchar **) &text, NULL);
   glCompileShader_func(shader);
   glGetShaderiv_func(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog_func(shader, 1000, &len, log);
      fprintf(stderr, "Error: problem compiling shader: %s\n", log);
      exit(1);
   }
   else {
      /*printf("Shader compiled OK\n");*/
   }
   return shader;
}


/**
 * Read a shader from a file.
 */
GLuint
CompileShaderFile(GLenum shaderType, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   GLuint shader;

   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Unable to open shader file %s\n", filename);
      return 0;
   }

   n = fread(buffer, 1, max, f);
   /*printf("read %d bytes from shader file %s\n", n, filename);*/
   if (n > 0) {
      buffer[n] = 0;
      shader = CompileShaderText(shaderType, buffer);
   }
   else {
      return 0;
   }

   fclose(f);
   free(buffer);

   return shader;
}


GLuint
LinkShaders(GLuint vertShader, GLuint fragShader)
{
   GLuint program = glCreateProgram_func();

   assert(vertShader || fragShader);

   if (fragShader)
      glAttachShader_func(program, fragShader);
   if (vertShader)
      glAttachShader_func(program, vertShader);
   glLinkProgram_func(program);

   /* check link */
   {
      GLint stat;
      glGetProgramiv_func(program, GL_LINK_STATUS, &stat);
      if (!stat) {
         GLchar log[1000];
         GLsizei len;
         glGetProgramInfoLog_func(program, 1000, &len, log);
         fprintf(stderr, "Shader link error:\n%s\n", log);
         return 0;
      }
   }

   return program;
}


void
InitUniforms(GLuint program, struct uniform_info uniforms[])
{
   GLuint i;

   for (i = 0; uniforms[i].name; i++) {
      uniforms[i].location
         = glGetUniformLocation_func(program, uniforms[i].name);

      printf("Uniform %s location: %d\n", uniforms[i].name,
             uniforms[i].location);

      switch (uniforms[i].size) {
      case 1:
         if (uniforms[i].type == GL_INT)
            glUniform1i_func(uniforms[i].location,
                             (GLint) uniforms[i].value[0]);
         else
            glUniform1fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 2:
         glUniform2fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 3:
         glUniform3fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 4:
         glUniform4fv_func(uniforms[i].location, 1, uniforms[i].value);
         break;
      default:
         abort();
      }
   }
}
