/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_fs.h"

void r300_translate_fragment_shader(struct r300_context* r300,
                                    struct r300_fragment_shader* fs)
{
    struct tgsi_parse_context parser;
    int i;
    boolean is_r500 = r300_screen(r300->context.screen)->caps->is_r500;
    struct r300_constant_buffer* consts =
        &r300->shader_constants[PIPE_SHADER_FRAGMENT];

    struct r300_fs_asm* assembler = CALLOC_STRUCT(r300_fs_asm);
    if (assembler == NULL) {
        return;
    }
    /* Setup starting offset for immediates. */
    assembler->imm_offset = consts->user_count;
    /* Enable depth writes, if needed. */
    assembler->writes_depth = fs->info.writes_z;

    /* Make sure we start at the beginning of the shader. */
    if (is_r500) {
        ((struct r5xx_fragment_shader*)fs)->instruction_count = 0;
    }

    tgsi_parse_init(&parser, fs->state.tokens);

    while (!tgsi_parse_end_of_tokens(&parser)) {
        tgsi_parse_token(&parser);

        /* This is seriously the lamest way to create fragment programs ever.
         * I blame TGSI. */
        switch (parser.FullToken.Token.Type) {
            case TGSI_TOKEN_TYPE_DECLARATION:
                /* Allocated registers sitting at the beginning
                 * of the program. */
                r300_fs_declare(assembler, &parser.FullToken.FullDeclaration);
                break;
            case TGSI_TOKEN_TYPE_IMMEDIATE:
                debug_printf("r300: Emitting immediate to constant buffer, "
                        "position %d\n",
                        assembler->imm_offset + assembler->imm_count);
                /* I am not amused by the length of these. */
                for (i = 0; i < 4; i++) {
                    consts->constants[assembler->imm_offset +
                        assembler->imm_count][i] =
                        parser.FullToken.FullImmediate.u.ImmediateFloat32[i]
                        .Float;
                }
                assembler->imm_count++;
                break;
            case TGSI_TOKEN_TYPE_INSTRUCTION:
                if (is_r500) {
                    r5xx_fs_instruction((struct r5xx_fragment_shader*)fs,
                            assembler, &parser.FullToken.FullInstruction);
                } else {
                    r3xx_fs_instruction((struct r3xx_fragment_shader*)fs,
                            assembler, &parser.FullToken.FullInstruction);
                }
                break;
        }
    }

    debug_printf("r300: fs: %d texs and %d colors, first free reg is %d\n",
            assembler->tex_count, assembler->color_count,
            assembler->tex_count + assembler->color_count);

    consts->count = consts->user_count + assembler->imm_count;
    fs->uses_imms = assembler->imm_count;
    debug_printf("r300: fs: %d total constants, "
            "%d from user and %d from immediates\n", consts->count,
            consts->user_count, assembler->imm_count);
    r3xx_fs_finalize(fs, assembler);
    if (is_r500) {
        r5xx_fs_finalize((struct r5xx_fragment_shader*)fs, assembler);
    }

    tgsi_dump(fs->state.tokens, 0);
    /* XXX finish r300 dumper too */
    if (is_r500) {
        r5xx_fs_dump((struct r5xx_fragment_shader*)fs);
    }

    tgsi_parse_free(&parser);
    FREE(assembler);
}
