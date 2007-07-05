#if !defined TGSI_DUMP_H
#define TGSI_DUMP_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

#define TGSI_DUMP_VERBOSE       0
#define TGSI_DUMP_NO_IGNORED    1
#define TGSI_DUMP_NO_DEFAULT    2

void
tgsi_dump(
   const struct tgsi_token *tokens,
   GLuint flags );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_DUMP_H

