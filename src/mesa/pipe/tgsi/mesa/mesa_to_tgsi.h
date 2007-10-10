#if !defined MESA_TO_TGSI_H
#define MESA_TO_TGSI_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

struct tgsi_token;

GLboolean
tgsi_translate_mesa_program(
   uint procType,
   const struct gl_program *program,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   struct tgsi_token *tokens,
   GLuint maxTokens );


#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined MESA_TO_TGSI_H

