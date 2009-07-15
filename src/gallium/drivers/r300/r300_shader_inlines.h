/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#ifndef R300_SHADER_INLINES_H
#define R300_SHADER_INLINES_H

/* TGSI constants. TGSI is like XML: If it can't solve your problems, you're
 * not using enough of it. */
static const struct tgsi_full_src_register r300_constant_zero = {
    .SrcRegister.Extended = TRUE,
    .SrcRegister.File = TGSI_FILE_NULL,
    .SrcRegisterExtSwz.ExtSwizzleX = TGSI_EXTSWIZZLE_ZERO,
    .SrcRegisterExtSwz.ExtSwizzleY = TGSI_EXTSWIZZLE_ZERO,
    .SrcRegisterExtSwz.ExtSwizzleZ = TGSI_EXTSWIZZLE_ZERO,
    .SrcRegisterExtSwz.ExtSwizzleW = TGSI_EXTSWIZZLE_ZERO,
};

static const struct tgsi_full_src_register r300_constant_one = {
    .SrcRegister.Extended = TRUE,
    .SrcRegister.File = TGSI_FILE_NULL,
    .SrcRegisterExtSwz.ExtSwizzleX = TGSI_EXTSWIZZLE_ONE,
    .SrcRegisterExtSwz.ExtSwizzleY = TGSI_EXTSWIZZLE_ONE,
    .SrcRegisterExtSwz.ExtSwizzleZ = TGSI_EXTSWIZZLE_ONE,
    .SrcRegisterExtSwz.ExtSwizzleW = TGSI_EXTSWIZZLE_ONE,
};

#endif /* R300_SHADER_INLINES_H */
