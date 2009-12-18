/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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

#include "r300_vs.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_tgsi_to_rc.h"
#include "r300_reg.h"

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "radeon_compiler.h"

/* Convert info about VS output semantics into r300_shader_semantics. */
static void r300_shader_read_vs_outputs(
    struct tgsi_shader_info* info,
    struct r300_shader_semantics* vs_outputs)
{
    int i;
    unsigned index;

    r300_shader_semantics_reset(vs_outputs);

    for (i = 0; i < info->num_outputs; i++) {
        index = info->output_semantic_index[i];

        switch (info->output_semantic_name[i]) {
            case TGSI_SEMANTIC_POSITION:
                assert(index == 0);
                vs_outputs->pos = i;
                break;

            case TGSI_SEMANTIC_PSIZE:
                assert(index == 0);
                vs_outputs->psize = i;
                break;

            case TGSI_SEMANTIC_COLOR:
                assert(index <= ATTR_COLOR_COUNT);
                vs_outputs->color[index] = i;
                break;

            case TGSI_SEMANTIC_BCOLOR:
                assert(index <= ATTR_COLOR_COUNT);
                vs_outputs->bcolor[index] = i;
                break;

            case TGSI_SEMANTIC_GENERIC:
                assert(index <= ATTR_GENERIC_COUNT);
                vs_outputs->generic[index] = i;
                break;

            case TGSI_SEMANTIC_FOG:
                assert(index == 0);
                vs_outputs->fog = i;
                break;

            case TGSI_SEMANTIC_EDGEFLAG:
                assert(index == 0);
                fprintf(stderr, "r300 VP: cannot handle edgeflag output\n");
                assert(0);
                break;
            default:
                assert(0);
        }
    }
}

static void r300_shader_vap_output_fmt(
    struct r300_shader_semantics* vs_outputs,
    uint* hwfmt)
{
    int i, gen_count;

    /* Do the actual vertex_info setup.
     *
     * vertex_info has four uints of hardware-specific data in it.
     * vinfo.hwfmt[0] is R300_VAP_VTX_STATE_CNTL
     * vinfo.hwfmt[1] is R300_VAP_VSM_VTX_ASSM
     * vinfo.hwfmt[2] is R300_VAP_OUTPUT_VTX_FMT_0
     * vinfo.hwfmt[3] is R300_VAP_OUTPUT_VTX_FMT_1 */

    hwfmt[0] = 0x5555; /* XXX this is classic Mesa bonghits */

    /* Position. */
    if (vs_outputs->pos != ATTR_UNUSED) {
        hwfmt[1] |= R300_INPUT_CNTL_POS;
        hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;
    } else {
        assert(0);
    }

    /* Point size. */
    if (vs_outputs->psize != ATTR_UNUSED) {
        hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
    }

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->color[i] != ATTR_UNUSED) {
            hwfmt[1] |= R300_INPUT_CNTL_COLOR;
            hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i;
        }
    }

    /* XXX Back-face colors. */

    /* Texture coordinates. */
    gen_count = 0;
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (vs_outputs->generic[i] != ATTR_UNUSED) {
            hwfmt[1] |= (R300_INPUT_CNTL_TC0 << gen_count);
            hwfmt[3] |= (4 << (3 * gen_count));
            gen_count++;
        }
    }

    /* Fog coordinates. */
    if (vs_outputs->fog != ATTR_UNUSED) {
        hwfmt[1] |= (R300_INPUT_CNTL_TC0 << gen_count);
        hwfmt[3] |= (4 << (3 * gen_count));
        gen_count++;
    }

    /* XXX magic */
    assert(gen_count <= 8);
}

/* Set VS output stream locations for SWTCL. */
static void r300_stream_locations_swtcl(
    struct r300_shader_semantics* vs_outputs,
    int* output_stream_loc)
{
    int i, tabi = 0, gen_count;

    /* XXX Check whether the numbers (0, 1, 2+i, etc.) are correct.
     * These should go to VAP_PROG_STREAM_CNTL/DST_VEC_LOC. */

    /* Position. */
    output_stream_loc[tabi++] = 0;

    /* Point size. */
    if (vs_outputs->psize != ATTR_UNUSED) {
        output_stream_loc[tabi++] = 1;
    }

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->color[i] != ATTR_UNUSED) {
            output_stream_loc[tabi++] = 2 + i;
        }
    }

    /* Back-face colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->bcolor[i] != ATTR_UNUSED) {
            output_stream_loc[tabi++] = 4 + i;
        }
    }

    /* Texture coordinates. */
    gen_count = 0;
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (vs_outputs->bcolor[i] != ATTR_UNUSED) {
            assert(tabi < 16);
            output_stream_loc[tabi++] = 6 + gen_count;
            gen_count++;
        }
    }

    /* Fog coordinates. */
    if (vs_outputs->fog != ATTR_UNUSED) {
        assert(tabi < 16);
        output_stream_loc[tabi++] = 6 + gen_count;
        gen_count++;
    }

    /* XXX magic */
    assert(gen_count <= 8);

    for (; tabi < 16;) {
        output_stream_loc[tabi++] = -1;
    }
}

static void set_vertex_inputs_outputs(struct r300_vertex_program_compiler * c)
{
    struct r300_vertex_shader * vs = c->UserData;
    struct r300_shader_semantics* outputs = &vs->outputs;
    struct tgsi_shader_info* info = &vs->info;
    int i, reg = 0;

    /* Fill in the input mapping */
    for (i = 0; i < info->num_inputs; i++)
        c->code->inputs[i] = i;

    /* Position. */
    if (outputs->pos != ATTR_UNUSED) {
        c->code->outputs[outputs->pos] = reg++;
    } else {
        assert(0);
    }

    /* Point size. */
    if (outputs->psize != ATTR_UNUSED) {
        c->code->outputs[outputs->psize] = reg++;
    }

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (outputs->color[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->color[i]] = reg++;
        }
    }

    /* XXX Back-face colors. */

    /* Texture coordinates. */
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (outputs->generic[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->generic[i]] = reg++;
        }
    }

    /* Fog coordinates. */
    if (outputs->fog != ATTR_UNUSED) {
        c->code->outputs[outputs->fog] = reg++;
    }
}

void r300_translate_vertex_shader(struct r300_context* r300,
                                  struct r300_vertex_shader* vs)
{
    struct r300_vertex_program_compiler compiler;
    struct tgsi_to_rc ttr;

    /* Initialize. */
    r300_shader_read_vs_outputs(&vs->info, &vs->outputs);
    r300_shader_vap_output_fmt(&vs->outputs, vs->hwfmt);

    if (!r300_screen(r300->context.screen)->caps->has_tcl) {
        r300_stream_locations_swtcl(&vs->outputs, vs->output_stream_loc_swtcl);
    }

    /* Setup the compiler */
    rc_init(&compiler.Base);

    compiler.Base.Debug = DBG_ON(r300, DBG_VP);
    compiler.code = &vs->code;
    compiler.UserData = vs;

    if (compiler.Base.Debug) {
        debug_printf("r300: Initial vertex program\n");
        tgsi_dump(vs->state.tokens, 0);
    }

    /* Translate TGSI to our internal representation */
    ttr.compiler = &compiler.Base;
    ttr.info = &vs->info;

    r300_tgsi_to_rc(&ttr, vs->state.tokens);

    compiler.RequiredOutputs = ~(~0 << vs->info.num_outputs);
    compiler.SetHwInputOutput = &set_vertex_inputs_outputs;

    /* Invoke the compiler */
    r3xx_compile_vertex_program(&compiler);
    if (compiler.Base.Error) {
        /* XXX Fail gracefully */
        fprintf(stderr, "r300 VP: Compiler error\n");
        abort();
    }

    /* And, finally... */
    rc_destroy(&compiler.Base);
    vs->translated = TRUE;
}
