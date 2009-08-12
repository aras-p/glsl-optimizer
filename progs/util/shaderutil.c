/**
 * Utilities for OpenGL shading language
 *
 * Brian Paul
 * 9 April 2008
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"


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


/** Get list of uniforms used in the program */
GLuint
GetUniforms(GLuint program, struct uniform_info uniforms[])
{
   GLint n, max, i;

   glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
   glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);

   for (i = 0; i < n; i++) {
      GLint size, len;
      GLenum type;
      char name[100];

      glGetActiveUniform(program, i, 100, &len, &size, &type, name);

      uniforms[i].name = strdup(name);
      switch (type) {
      case GL_FLOAT:
         size = 1;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_VEC2:
         size = 2;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_VEC3:
         size = 3;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_VEC4:
         size = 4;
         type = GL_FLOAT;
         break;
      case GL_INT:
         size = 1;
         type = GL_INT;
         break;
      case GL_INT_VEC2:
         size = 2;
         type = GL_INT;
         break;
      case GL_INT_VEC3:
         size = 3;
         type = GL_INT;
         break;
      case GL_INT_VEC4:
         size = 4;
         type = GL_INT;
         break;
      case GL_FLOAT_MAT3:
         /* XXX fix me */
         size = 3;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_MAT4:
         /* XXX fix me */
         size = 4;
         type = GL_FLOAT;
         break;
      default:
         abort();
      }
      uniforms[i].size = size;
      uniforms[i].type = type;
      uniforms[i].location = glGetUniformLocation(program, name);
   }

   uniforms[i].name = NULL; /* end of list */

   return n;
}


void
PrintUniforms(const struct uniform_info uniforms[])
{
   GLint i;

   printf("Uniforms:\n");

   for (i = 0; uniforms[i].name; i++) {
      printf("  %d: %s size=%d type=0x%x loc=%d value=%g, %g, %g, %g\n",
             i,
             uniforms[i].name,
             uniforms[i].size,
             uniforms[i].type,
             uniforms[i].location,
             uniforms[i].value[0],
             uniforms[i].value[1],
             uniforms[i].value[2],
             uniforms[i].value[3]);
   }
}


/** Get list of attribs used in the program */
GLuint
GetAttribs(GLuint program, struct attrib_info attribs[])
{
   GLint n, max, i;

   glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
   glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max);

   for (i = 0; i < n; i++) {
      GLint size, len;
      GLenum type;
      char name[100];

      glGetActiveAttrib(program, i, 100, &len, &size, &type, name);

      attribs[i].name = strdup(name);
      switch (type) {
      case GL_FLOAT:
         size = 1;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_VEC2:
         size = 2;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_VEC3:
         size = 3;
         type = GL_FLOAT;
         break;
      case GL_FLOAT_VEC4:
         size = 4;
         type = GL_FLOAT;
         break;
      case GL_INT:
         size = 1;
         type = GL_INT;
         break;
      case GL_INT_VEC2:
         size = 2;
         type = GL_INT;
         break;
      case GL_INT_VEC3:
         size = 3;
         type = GL_INT;
         break;
      case GL_INT_VEC4:
         size = 4;
         type = GL_INT;
         break;
      default:
         abort();
      }
      attribs[i].size = size;
      attribs[i].type = type;
      attribs[i].location = glGetAttribLocation(program, name);
   }

   attribs[i].name = NULL; /* end of list */

   return n;
}


void
PrintAttribs(const struct attrib_info attribs[])
{
   GLint i;

   printf("Attribs:\n");

   for (i = 0; attribs[i].name; i++) {
      printf("  %d: %s size=%d type=0x%x loc=%d\n",
             i,
             attribs[i].name,
             attribs[i].size,
             attribs[i].type,
             attribs[i].location);
   }
}
