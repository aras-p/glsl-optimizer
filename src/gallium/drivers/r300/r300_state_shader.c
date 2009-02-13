/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_state_shader.h"

static void r300_copy_passthrough_shader(struct r300_fragment_shader* fs)
{
    struct r300_fragment_shader* pt = &r300_passthrough_fragment_shader;
    fs->shader.stack_size = pt->shader.stack_size;
    fs->alu_instruction_count = pt->alu_instruction_count;
    fs->tex_instruction_count = pt->tex_instruction_count;
    fs->indirections = pt->indirections;
    fs->instructions[0] = pt->instructions[0];
}

static void r500_copy_passthrough_shader(struct r500_fragment_shader* fs)
{
    struct r500_fragment_shader* pt = &r500_passthrough_fragment_shader;
    fs->shader.stack_size = pt->shader.stack_size;
    fs->instruction_count = pt->instruction_count;
    fs->instructions[0] = pt->instructions[0];
}

void r300_translate_shader(struct r300_context* r300,
                           struct r300_fragment_shader* fs)
{
    r300_copy_passthrough_shader(fs);
}

void r500_translate_shader(struct r300_context* r300,
                           struct r500_fragment_shader* fs)
{
    r500_copy_passthrough_shader(fs);
}
