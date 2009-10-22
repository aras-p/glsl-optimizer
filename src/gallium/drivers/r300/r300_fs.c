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

#include "tgsi/tgsi_dump.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_fs.h"
#include "r300_tgsi_to_rc.h"

#include "radeon_code.h"
#include "radeon_compiler.h"

static void find_output_registers(struct r300_fragment_program_compiler * compiler,
                                  struct r300_fragment_shader * fs)
{
    unsigned i;

    /* Mark the outputs as not present initially */
    compiler->OutputColor = fs->info.num_outputs;
    compiler->OutputDepth = fs->info.num_outputs;

    /* Now see where they really are. */
    for(i = 0; i < fs->info.num_outputs; ++i) {
        switch(fs->info.output_semantic_name[i]) {
            case TGSI_SEMANTIC_COLOR:
                compiler->OutputColor = i;
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
    struct tgsi_shader_info* info = &((struct r300_fragment_shader*)c->UserData)->info;
    int total_colors = 0;
    int colors = 0;
    int total_generic = 0;
    int generic = 0;
    int i;

    for (i = 0; i < info->num_inputs; i++) {
        switch (info->input_semantic_name[i]) {
            case TGSI_SEMANTIC_COLOR:
                total_colors++;
                break;
            case TGSI_SEMANTIC_FOG:
            case TGSI_SEMANTIC_GENERIC:
                total_generic++;
                break;
        }
    }

    for(i = 0; i < info->num_inputs; i++) {
        switch (info->input_semantic_name[i]) {
            case TGSI_SEMANTIC_COLOR:
                allocate(mydata, i, colors);
                colors++;
                break;
            case TGSI_SEMANTIC_FOG:
            case TGSI_SEMANTIC_GENERIC:
                allocate(mydata, i, total_colors + generic);
                generic++;
                break;
        }
    }
}

void r300_translate_fragment_shader(struct r300_context* r300,
                                    struct r300_fragment_shader* fs)
{
    struct r300_fragment_program_compiler compiler;
    struct tgsi_to_rc ttr;

    memset(&compiler, 0, sizeof(compiler));
    rc_init(&compiler.Base);
    compiler.Base.Debug = DBG_ON(r300, DBG_FP);

    compiler.code = &fs->code;
    compiler.is_r500 = r300_screen(r300->context.screen)->caps->is_r500;
    compiler.AllocateHwInputs = &allocate_hardware_inputs;
    compiler.UserData = fs;

    /* TODO: Program compilation depends on texture compare modes,
     * which are sampler state. Therefore, programs need to be recompiled
     * depending on this state as in the classic Mesa driver.
     *
     * This is not yet handled correctly.
     */

    find_output_registers(&compiler, fs);

    if (compiler.Base.Debug) {
        debug_printf("r300: Initial fragment program\n");
        tgsi_dump(fs->state.tokens, 0);
    }

    /* Translate TGSI to our internal representation */
    ttr.compiler = &compiler.Base;
    ttr.info = &fs->info;

    r300_tgsi_to_rc(&ttr, fs->state.tokens);

    /* Invoke the compiler */
    r3xx_compile_fragment_program(&compiler);
    if (compiler.Base.Error) {
        /* XXX failover maybe? */
        DBG(r300, DBG_FP, "r300: Error compiling fragment program: %s\n",
            compiler.Base.ErrorMsg);
    }

    /* And, finally... */
    rc_destroy(&compiler.Base);
    fs->translated = TRUE;
}
