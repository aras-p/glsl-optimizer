#if !defined TGSI_SSE2_H
#define TGSI_SSE2_H

#if defined __cplusplus
extern "C" {
#endif

struct tgsi_token;
struct x86_function;

unsigned
tgsi_emit_sse2(
   struct tgsi_token *tokens,
   struct x86_function *function );

unsigned
tgsi_emit_sse2_fs(
   struct tgsi_token *tokens,
   struct x86_function *function,
   float (*immediates)[4]
 );

#if defined __cplusplus
}
#endif

#endif /* TGSI_SSE2_H */

