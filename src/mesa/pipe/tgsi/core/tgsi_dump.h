#if !defined TGSI_DUMP_H
#define TGSI_DUMP_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

#define TGSI_DUMP_VERBOSE       1
#define TGSI_DUMP_NO_IGNORED    2
#define TGSI_DUMP_NO_DEFAULT    4

void
tgsi_dump(
   const struct tgsi_token *tokens,
   unsigned flags );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_DUMP_H

