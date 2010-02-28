/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
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

#include "util/u_math.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_dump.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_fs.h"
#include "r300_tgsi_to_rc.h"

#include "radeon_code.h"
#include "radeon_compiler.h"

/* Convert info about FS input semantics to r300_shader_semantics. */
void r300_shader_read_fs_inputs(struct tgsi_shader_info* info,
                                struct r300_shader_semantics* fs_inputs)
{
    int i;
    unsigned index;

    r300_shader_semantics_reset(fs_inputs);

    for (i = 0; i < info->num_inputs; i++) {
        index = info->input_semantic_index[i];

        switch (info->input_semantic_name[i]) {
            case TGSI_SEMANTIC_COLOR:
                assert(index < ATTR_COLOR_COUNT);
                fs_inputs->color[index] = i;
                break;

            case TGSI_SEMANTIC_GENERIC:
                assert(index < ATTR_GENERIC_COUNT);
                fs_inputs->generic[index] = i;
                break;

            case TGSI_SEMANTIC_FOG:
                assert(index == 0);
                fs_inputs->fog = i;
                break;

            case TGSI_SEMANTIC_POSITION:
                assert(index == 0);
                fs_inputs->wpos = i;
                break;

            default:
                assert(0);
        }
    }
}

static void find_output_registers(struct r300_fragment_program_compiler * compiler,
                                  struct r300_fragment_shader * fs)
{
    unsigned i, colorbuf_count = 0;

    /* Mark the outputs as not present initially */
    compiler->OutputColor[0] = fs->info.num_outputs;
    compiler->OutputColor[1] = fs->info.num_outputs;
    compiler->OutputColor[2] = fs->info.num_outputs;
    compiler->OutputColor[3] = fs->info.num_outputs;
    compiler->OutputDepth = fs->info.num_outputs;

    /* Now see where they really are. */
    for(i = 0; i < fs->info.num_outputs; ++i) {
        switch(fs->info.output_semantic_name[i]) {
            case TGSI_SEMANTIC_COLOR:
                compiler->OutputColor[colorbuf_count] = i;
                colorbuf_count++;
                break;
            case TGSI_SEMANTIC_POSITION:
                compiler->OutputDepth = i;
                break;
        }
    }
}

static void allocate_hardware_inputs(
    struct r300_fragment_program_compiler * c,
    void (*allocate)(void * data, unsigned input, unsigned hwreg),
    void * mydata)
{
    struct r300_shader_semantics* inputs =
        (struct r300_shader_semantics*)c->UserData;
    int i, reg = 0;

    /* Allocate input registers. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (inputs->color[i] != ATTR_UNUSED) {
            allocate(mydata, inputs->color[i], reg++);
        }
    }
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (inputs->generic[i] != ATTR_UNUSED) {
            allocate(mydata, inputs->generic[i], reg++);
        }
    }
    if (inputs->fog != ATTR_UNUSED) {
        allocate(mydata, inputs->fog, reg++);
    }
    if (inputs->wpos != ATTR_UNUSED) {
        allocate(mydata, inputs->wpos, reg++);
    }
}

static void get_compare_state(
    struct r300_context* r300,
    struct r300_fragment_program_external_state* state,
    unsigned shadow_samplers)
{
    struct r300_textures_state *texstate =
        (struct r300_textures_state*)r300->textures_state.state;

    memset(state, 0, sizeof(*state));

    for (int i = 0; i < texstate->sampler_count; i++) {
        struct r300_sampler_state* s = texstate->sampler_states[i];

        if (s && s->state.compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
            /* XXX Gallium doesn't provide us with any information regarding
             * this mode, so we are screwed. I'm setting 0 = LUMINANCE. */
            state->unit[i].depth_texture_mode = 0;

            /* Fortunately, no need to translate this. */
            state->unit[i].texture_compare_func = s->state.compare_func;
        }
    }
}

static void r300_translate_fragment_shader(
    struct r300_context* r300,
    struct r300_fragment_shader_code* shader)
{
    struct r300_fragment_shader* fs = r300->fs;
    struct r300_fragment_program_compiler compiler;
    struct tgsi_to_rc ttr;
    int wpos = fs->inputs.wpos;

    /* Setup the compiler. */
    memset(&compiler, 0, sizeof(compiler));
    rc_init(&compiler.Base);
    compiler.Base.Debug = DBG_ON(r300, DBG_FP);

    compiler.code = &shader->code;
    compiler.state = shader->compare_state;
    compiler.is_r500 = r300_screen(r300->context.screen)->caps->is_r500;
    compiler.AllocateHwInputs = &allocate_hardware_inputs;
    compiler.UserData = &fs->inputs;

    find_output_registers(&compiler, fs);

    if (compiler.Base.Debug) {
        debug_printf("r300: Initial fragment program\n");
        tgsi_dump(fs->state.tokens, 0);
    }

    /* Translate TGSI to our internal representation */
    ttr.compiler = &compiler.Base;
    ttr.info = &fs->info;
    ttr.use_half_swizzles = TRUE;

    r300_tgsi_to_rc(&ttr, fs->state.tokens);

    fs->shadow_samplers = compiler.Base.Program.ShadowSamplers;

    /**
     * Transform the program to support WPOS.
     *
     * Introduce a small fragment at the start of the program that will be
     * the only code that directly reads the WPOS input.
     * All other code pieces that reference that input will be rewritten
     * to read from a newly allocated temporary. */
    if (wpos != ATTR_UNUSED) {
        /* Moving the input to some other reg is not really necessary. */
        rc_transform_fragment_wpos(&compiler.Base, wpos, wpos, TRUE);
    }

    /* Invoke the compiler */
    r3xx_compile_fragment_program(&compiler);
    if (compiler.Base.Error) {
        /* XXX failover maybe? */
        DBG(r300, DBG_FP, "r300: Error compiling fragment program: %s\n",
            compiler.Base.ErrorMsg);
        assert(0);
    }

    /* And, finally... */
    rc_destroy(&compiler.Base);
}

boolean r300_pick_fragment_shader(struct r300_context* r300)
{
    struct r300_fragment_shader* fs = r300->fs;
    struct r300_fragment_program_external_state state;
    struct r300_fragment_shader_code* ptr;

    if (!fs->first) {
        /* Build the fragment shader for the first time. */
        fs->first = fs->shader = CALLOC_STRUCT(r300_fragment_shader_code);

        /* BTW shadow samplers will be known after the first translation,
         * therefore we set ~0, which means it should look at all sampler
         * states. This choice doesn't have any impact on the correctness. */
        get_compare_state(r300, &fs->shader->compare_state, ~0);
        r300_translate_fragment_shader(r300, fs->shader);
        return TRUE;

    } else if (fs->shadow_samplers) {
        get_compare_state(r300, &state, fs->shadow_samplers);

        /* Check if the currently-bound shader has been compiled
         * with the texture-compare state we need. */
        if (memcmp(&fs->shader->compare_state, &state, sizeof(state)) != 0) {
            /* Search for the right shader. */
            ptr = fs->first;
            while (ptr) {
                if (memcmp(&ptr->compare_state, &state, sizeof(state)) == 0) {
                    fs->shader = ptr;
                    return TRUE;
                }
                ptr = ptr->next;
            }

            /* Not found, gotta compile a new one. */
            ptr = CALLOC_STRUCT(r300_fragment_shader_code);
            ptr->next = fs->first;
            fs->first = fs->shader = ptr;

            ptr->compare_state = state;
            r300_translate_fragment_shader(r300, ptr);
            return TRUE;
        }
    }

    return FALSE;
}
