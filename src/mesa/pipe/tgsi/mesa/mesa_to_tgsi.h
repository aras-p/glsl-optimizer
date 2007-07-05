#if !defined MESA_TO_TGSI_H
#define MESA_TO_TGSI_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

GLboolean
tgsi_mesa_compile_fp_program(
   const struct gl_fragment_program *program,
   struct tgsi_token *tokens,
   GLuint maxTokens );

GLboolean
tgsi_mesa_compile_vp_program(
   const struct gl_vertex_program *program,
   struct tgsi_token *tokens,
   GLuint maxTokens );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined MESA_TO_TGSI_H

