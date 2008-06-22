#if !defined TGSI_DUMP_H
#define TGSI_DUMP_H

#if defined __cplusplus
extern "C" {
#endif

#define TGSI_DUMP_VERBOSE       1
#define TGSI_DUMP_NO_IGNORED    2
#define TGSI_DUMP_NO_DEFAULT    4

void
tgsi_dump(
   const struct tgsi_token *tokens,
   unsigned                flags );

struct tgsi_full_immediate;
struct tgsi_full_instruction;
struct tgsi_full_declaration;

void
tgsi_dump_immediate(
   const struct tgsi_full_immediate *imm );

void
tgsi_dump_instruction(
   const struct tgsi_full_instruction  *inst,
   unsigned                      instno );

void
tgsi_dump_declaration(
   const struct tgsi_full_declaration  *decl );


#if defined __cplusplus
}
#endif

#endif /* TGSI_DUMP_H */

