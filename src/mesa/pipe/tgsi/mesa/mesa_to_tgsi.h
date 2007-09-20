#if !defined MESA_TO_TGSI_H
#define MESA_TO_TGSI_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

struct tgsi_token;

GLboolean
tgsi_mesa_compile_fp_program(
   const struct gl_fragment_program *program,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemantic[],
   const GLuint interpMode[],
   const GLuint outputMapping[],
   struct tgsi_token *tokens,
   GLuint maxTokens );

GLboolean
tgsi_mesa_compile_vp_program(
   const struct gl_vertex_program *program,
   const GLuint inputMapping[],
   const GLuint outputMapping[],
   struct tgsi_token *tokens,
   GLuint maxTokens );

uint
tgsi_mesa_translate_vertex_input(GLuint attrib);

uint
tgsi_mesa_translate_vertex_output(GLuint attrib);

uint
tgsi_mesa_translate_fragment_input(GLuint attrib);

uint
tgsi_mesa_translate_fragment_output(GLuint attrib);

uint
tgsi_mesa_translate_vertex_input_mask(GLbitfield mask);

uint
tgsi_mesa_translate_vertex_output_mask(GLbitfield mask);

uint
tgsi_mesa_translate_fragment_input_mask(GLbitfield mask);

uint
tgsi_mesa_translate_fragment_output_mask(GLbitfield mask);


#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined MESA_TO_TGSI_H

