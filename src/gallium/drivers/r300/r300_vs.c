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
#include "r300_fs.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_tgsi_to_rc.h"
#include "r300_reg.h"

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "radeon_compiler.h"

#include "util/u_math.h"

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
                assert(index < ATTR_COLOR_COUNT);
                vs_outputs->color[index] = i;
                break;

            case TGSI_SEMANTIC_BCOLOR:
                assert(index < ATTR_COLOR_COUNT);
                vs_outputs->bcolor[index] = i;
                break;

            case TGSI_SEMANTIC_GENERIC:
                assert(index < ATTR_GENERIC_COUNT);
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

static void r300_shader_vap_output_fmt(struct r300_vertex_shader* vs)
{
    struct r300_shader_semantics* vs_outputs = &vs->outputs;
    uint32_t* hwfmt = vs->hwfmt;
    int i, gen_count;
    boolean any_bcolor_used = vs_outputs->bcolor[0] != ATTR_UNUSED ||
                              vs_outputs->bcolor[1] != ATTR_UNUSED;

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
        if (vs_outputs->color[i] != ATTR_UNUSED || any_bcolor_used ||
            vs_outputs->color[1] != ATTR_UNUSED) {
            hwfmt[1] |= R300_INPUT_CNTL_COLOR;
            hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i;
        }
    }

    /* Back-face colors. */
    if (any_bcolor_used) {
        for (i = 0; i < ATTR_COLOR_COUNT; i++) {
            hwfmt[1] |= R300_INPUT_CNTL_COLOR;
            hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << (2+i);
        }
    }

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

    /* WPOS. */
    vs->wpos_tex_output = gen_count;
}

/* Sets up stream mapping to equivalent VS outputs if TCL is bypassed
 * or isn't present. */
static void r300_stream_locations_notcl(
    struct r300_shader_semantics* vs_outputs,
    int* stream_loc)
{
    int i, tabi = 0, gen_count;
    boolean any_bcolor_used = vs_outputs->bcolor[0] != ATTR_UNUSED ||
                              vs_outputs->bcolor[1] != ATTR_UNUSED;

    /* Position. */
    stream_loc[tabi++] = 0;

    /* Point size. */
    if (vs_outputs->psize != ATTR_UNUSED) {
        stream_loc[tabi++] = 1;
    }

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->color[i] != ATTR_UNUSED || any_bcolor_used ||
            vs_outputs->color[1] != ATTR_UNUSED) {
            stream_loc[tabi++] = 2 + i;
        }
    }

    /* Back-face colors. */
    if (any_bcolor_used) {
        for (i = 0; i < ATTR_COLOR_COUNT; i++) {
            stream_loc[tabi++] = 4 + i;
        }
    }

    /* Texture coordinates. */
    gen_count = 0;
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (vs_outputs->generic[i] != ATTR_UNUSED) {
            assert(tabi < 16);
            stream_loc[tabi++] = 6 + gen_count;
            gen_count++;
        }
    }

    /* Fog coordinates. */
    if (vs_outputs->fog != ATTR_UNUSED) {
        assert(tabi < 16);
        stream_loc[tabi++] = 6 + gen_count;
        gen_count++;
    }

    /* WPOS. */
    if (vs_outputs->wpos != ATTR_UNUSED) {
        assert(tabi < 16);
        stream_loc[tabi++] = 6 + gen_count;
        gen_count++;
    }

    for (; tabi < 16;) {
        stream_loc[tabi++] = -1;
    }
}

static void set_vertex_inputs_outputs(struct r300_vertex_program_compiler * c)
{
    struct r300_vertex_shader * vs = c->UserData;
    struct r300_shader_semantics* outputs = &vs->outputs;
    struct tgsi_shader_info* info = &vs->info;
    int i, reg = 0;
    boolean any_bcolor_used = outputs->bcolor[0] != ATTR_UNUSED ||
                              outputs->bcolor[1] != ATTR_UNUSED;

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

    /* If we're writing back facing colors we need to send
     * four colors to make front/back face colors selection work.
     * If the vertex program doesn't write all 4 colors, lets
     * pretend it does by skipping output index reg so the colors
     * get written into appropriate output vectors.
     */

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (outputs->color[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->color[i]] = reg++;
        } else if (any_bcolor_used ||
                   outputs->color[1] != ATTR_UNUSED) {
            reg++;
        }
    }

    /* Back-face colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (outputs->bcolor[i] != ATTR_UNUSED) {
            c->code->outputs[outputs->bcolor[i]] = reg++;
        } else if (any_bcolor_used) {
            reg++;
        }
    }

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

    /* WPOS. */
    if (outputs->wpos != ATTR_UNUSED) {
        c->code->outputs[outputs->wpos] = reg++;
    }
}

static void r300_insert_wpos(struct r300_vertex_program_compiler* c,
                             struct r300_shader_semantics* outputs)
{
    int i, lastOutput = 0;

    /* Find the max output index. */
    lastOutput = MAX2(lastOutput, outputs->psize);
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        lastOutput = MAX2(lastOutput, outputs->color[i]);
        lastOutput = MAX2(lastOutput, outputs->bcolor[i]);
    }
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        lastOutput = MAX2(lastOutput, outputs->generic[i]);
    }
    lastOutput = MAX2(lastOutput, outputs->fog);

    /* Set WPOS after the last output. */
    lastOutput++;
    rc_copy_output(&c->Base, 0, lastOutput); /* out[lastOutput] = out[0]; */
    outputs->wpos = lastOutput;
}

void r300_translate_vertex_shader(struct r300_context* r300,
                                  struct r300_vertex_shader* vs)
{
    struct r300_vertex_program_compiler compiler;
    struct tgsi_to_rc ttr;

    /* Initialize. */
    r300_shader_read_vs_outputs(&vs->info, &vs->outputs);

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
    ttr.use_half_swizzles = FALSE;

    r300_tgsi_to_rc(&ttr, vs->state.tokens);

    compiler.RequiredOutputs = ~(~0 << (vs->info.num_outputs+1));
    compiler.SetHwInputOutput = &set_vertex_inputs_outputs;

    /* Insert the WPOS output. */
    r300_insert_wpos(&compiler, &vs->outputs);

    r300_shader_vap_output_fmt(vs);
    r300_stream_locations_notcl(&vs->outputs, vs->stream_loc_notcl);

    /* Invoke the compiler */
    r3xx_compile_vertex_program(&compiler);
    if (compiler.Base.Error) {
        /* XXX We should fallback using Draw. */
        fprintf(stderr, "r300 VP: Compiler error\n");
        abort();
    }

    /* And, finally... */
    rc_destroy(&compiler.Base);
    vs->translated = TRUE;
}

boolean r300_vertex_shader_setup_wpos(struct r300_context* r300)
{
    struct r300_vertex_shader* vs = r300->vs_state.state;
    int tex_output = vs->wpos_tex_output;
    uint32_t tex_fmt = R300_INPUT_CNTL_TC0 << tex_output;
    uint32_t* hwfmt = vs->hwfmt;

    if (r300->fs->inputs.wpos != ATTR_UNUSED) {
        /* Enable WPOS in VAP. */
        if (!(hwfmt[1] & tex_fmt)) {
            hwfmt[1] |= tex_fmt;
            hwfmt[3] |= (4 << (3 * tex_output));

            assert(tex_output < 8);
            return TRUE;
        }
    } else {
        /* Disable WPOS in VAP. */
        if (hwfmt[1] & tex_fmt) {
            hwfmt[1] &= ~tex_fmt;
            hwfmt[3] &= ~(4 << (3 * tex_output));
            return TRUE;
        }
    }
    return FALSE;
}
