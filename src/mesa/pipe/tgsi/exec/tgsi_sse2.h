#if !defined TGSI_SSE2_H
#define TGSI_SSE2_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

struct tgsi_token;
struct x86_function;

unsigned
tgsi_emit_sse2(
   struct tgsi_token *tokens,
   struct x86_function *function );

unsigned
tgsi_emit_sse2_fs(
   struct tgsi_token *tokens,
   struct x86_function *function );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_SSE2_H

