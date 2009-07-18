/**
 * Utilities for OpenGL shading language
 *
 * Brian Paul
 * 9 April 2008
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


static void
Init(void)
{
   static GLboolean firstCall = GL_TRUE;
   if (firstCall) {
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

   shader = glCreateShader(shaderType);
   glShaderSource(shader, 1, (const GLchar **) &text, NULL);
   glCompileShader(shader);
   glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog(shader, 1000, &len, log);
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
   FILE *f;

   Init();


   f = fopen(filename, "r");
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
   GLuint program = glCreateProgram();

   assert(vertShader || fragShader);

   if (fragShader)
      glAttachShader(program, fragShader);
   if (vertShader)
      glAttachShader(program, vertShader);
   glLinkProgram(program);

   /* check link */
   {
      GLint stat;
      glGetProgramiv(program, GL_LINK_STATUS, &stat);
      if (!stat) {
         GLchar log[1000];
         GLsizei len;
         glGetProgramInfoLog(program, 1000, &len, log);
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
         = glGetUniformLocation(program, uniforms[i].name);

      printf("Uniform %s location: %d\n", uniforms[i].name,
             uniforms[i].location);

      switch (uniforms[i].size) {
      case 1:
         if (uniforms[i].type == GL_INT)
            glUniform1i(uniforms[i].location,
                             (GLint) uniforms[i].value[0]);
         else
            glUniform1fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 2:
         glUniform2fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 3:
         glUniform3fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case 4:
         glUniform4fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      default:
         abort();
      }
   }
}
