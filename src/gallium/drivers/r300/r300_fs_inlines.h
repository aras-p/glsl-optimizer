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

#ifndef R300_FS_INLINES_H
#define R300_FS_INLINES_H

#include "tgsi/tgsi_parse.h"

#include "r300_context.h"
#include "r300_debug.h"
#include "r300_reg.h"
#include "r300_screen.h"
#include "r300_shader_inlines.h"

/* Temporary struct used to hold assembly state while putting together
 * fragment programs. */
struct r300_fs_asm {
    /* Pipe context. */
    struct r300_context* r300;
    /* Number of colors. */
    unsigned color_count;
    /* Number of texcoords. */
    unsigned tex_count;
    /* Offset for temporary registers. Inputs and temporaries have no
     * distinguishing markings, so inputs start at 0 and the first usable
     * temporary register is after all inputs. */
    unsigned temp_offset;
    /* Number of requested temporary registers. */
    unsigned temp_count;
    /* Offset for immediate constants. Neither R300 nor R500 can do four
     * inline constants per source, so instead we copy immediates into the
     * constant buffer. */
    unsigned imm_offset;
    /* Number of immediate constants. */
    unsigned imm_count;
    /* Are depth writes enabled? */
    boolean writes_depth;
    /* Depth write offset. This is the TGSI output that corresponds to
     * depth writes. */
    unsigned depth_output;
};

static INLINE void r300_fs_declare(struct r300_fs_asm* assembler,
                            struct tgsi_full_declaration* decl)
{
    switch (decl->Declaration.File) {
        case TGSI_FILE_INPUT:
            switch (decl->Semantic.SemanticName) {
                case TGSI_SEMANTIC_COLOR:
                    assembler->color_count++;
                    break;
                case TGSI_SEMANTIC_FOG:
                case TGSI_SEMANTIC_GENERIC:
                    assembler->tex_count++;
                    break;
                default:
                    debug_printf("r300: fs: Bad semantic declaration %d\n",
                        decl->Semantic.SemanticName);
                    break;
            }
            break;
        case TGSI_FILE_OUTPUT:
            /* Depth write. Mark the position of the output so we can
             * identify it later. */
            if (decl->Semantic.SemanticName == TGSI_SEMANTIC_POSITION) {
                assembler->depth_output = decl->DeclarationRange.First;
            }
            break;
        case TGSI_FILE_CONSTANT:
            break;
        case TGSI_FILE_TEMPORARY:
            assembler->temp_count++;
            break;
        default:
            debug_printf("r300: fs: Bad file %d\n", decl->Declaration.File);
            break;
    }

    assembler->temp_offset = assembler->color_count + assembler->tex_count;
}

static INLINE unsigned r300_fs_src(struct r300_fs_asm* assembler,
                                   struct tgsi_src_register* src)
{
    switch (src->File) {
        case TGSI_FILE_NULL:
            return 0;
        case TGSI_FILE_INPUT:
            /* XXX may be wrong */
            return src->Index;
            break;
        case TGSI_FILE_TEMPORARY:
            return src->Index + assembler->temp_offset;
            break;
        case TGSI_FILE_IMMEDIATE:
            return (src->Index + assembler->imm_offset) | (1 << 8);
            break;
        case TGSI_FILE_CONSTANT:
            /* XXX magic */
            return src->Index | (1 << 8);
            break;
        default:
            debug_printf("r300: fs: Unimplemented src %d\n", src->File);
            break;
    }
    return 0;
}

static INLINE unsigned r300_fs_dst(struct r300_fs_asm* assembler,
                                   struct tgsi_dst_register* dst)
{
    switch (dst->File) {
        case TGSI_FILE_NULL:
            /* This happens during KIL instructions. */
            return 0;
            break;
        case TGSI_FILE_OUTPUT:
            return 0;
            break;
        case TGSI_FILE_TEMPORARY:
            return dst->Index + assembler->temp_offset;
            break;
        default:
            debug_printf("r300: fs: Unimplemented dst %d\n", dst->File);
            break;
    }
    return 0;
}

static INLINE boolean r300_fs_is_depr(struct r300_fs_asm* assembler,
                                      struct tgsi_dst_register* dst)
{
    return (assembler->writes_depth &&
            (dst->File == TGSI_FILE_OUTPUT) &&
            (dst->Index == assembler->depth_output));
}

#endif /* R300_FS_INLINES_H */
