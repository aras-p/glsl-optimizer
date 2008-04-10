#ifndef SHADER_UTIL_H
#define SHADER_UTIL_H



struct uniform_info
{
   const char *name;
   GLuint size;
   GLenum type;  /**< GL_FLOAT or GL_INT */
   GLfloat value[4];
   GLint location;  /**< filled in by InitUniforms() */
};

#define END_OF_UNIFORMS   { NULL, 0, GL_NONE, { 0, 0, 0, 0 }, -1 }


extern GLboolean
ShadersSupported(void);

extern GLuint
CompileShaderText(GLenum shaderType, const char *text);

extern GLuint
CompileShaderFile(GLenum shaderType, const char *filename);

extern GLuint
LinkShaders(GLuint vertShader, GLuint fragShader);

extern void
InitUniforms(GLuint program, struct uniform_info uniforms[]);


#endif /* SHADER_UTIL_H */
