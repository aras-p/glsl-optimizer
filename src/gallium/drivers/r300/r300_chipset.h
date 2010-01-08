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

#ifndef R300_CHIPSET_H
#define R300_CHIPSET_H

#include "pipe/p_compiler.h"

/* Structure containing all the possible information about a specific Radeon
 * in the R3xx, R4xx, and R5xx families. */
struct r300_capabilities {
    /* PCI ID */
    uint32_t pci_id;
    /* Chipset family */
    int family;
    /* The number of vertex floating-point units */
    unsigned num_vert_fpus;
    /* The number of fragment pipes */
    unsigned num_frag_pipes;
    /* The number of z pipes */
    unsigned num_z_pipes;
    /* Whether or not TCL is physically present */
    boolean has_tcl;
    /* Whether or not this is R400. The differences compared to their R3xx
     * cousins are:
     * - Extended fragment shader registers
     * - Blend LTE/GTE thresholds */
    boolean is_r400;
    /* Whether or not this is an RV515 or newer; R500s have many differences
     * that require extra consideration, compared to their R3xx cousins:
     * - Extra bit of width and height on texture sizes
     * - Blend color is split across two registers
     * - Blend LTE/GTE thresholds
     * - Universal Shader (US) block used for fragment shaders
     * - FP16 blending and multisampling */
    boolean is_r500;
    /* Whether or not the second pixel pipe is accessed with the high bit */
    boolean high_second_pipe;
};

/* Enumerations for legibility and telling which card we're running on. */
enum {
    CHIP_FAMILY_R300 = 0,
    CHIP_FAMILY_R350,
    CHIP_FAMILY_R360,
    CHIP_FAMILY_RV350,
    CHIP_FAMILY_RV370,
    CHIP_FAMILY_RV380,
    CHIP_FAMILY_R420,
    CHIP_FAMILY_R423,
    CHIP_FAMILY_R430,
    CHIP_FAMILY_R480,
    CHIP_FAMILY_R481,
    CHIP_FAMILY_RV410,
    CHIP_FAMILY_RS400,
    CHIP_FAMILY_RC410,
    CHIP_FAMILY_RS480,
    CHIP_FAMILY_RS482,
    CHIP_FAMILY_RS600,
    CHIP_FAMILY_RS690,
    CHIP_FAMILY_RS740,
    CHIP_FAMILY_RV515,
    CHIP_FAMILY_R520,
    CHIP_FAMILY_RV530,
    CHIP_FAMILY_R580,
    CHIP_FAMILY_RV560,
    CHIP_FAMILY_RV570
};

void r300_parse_chipset(struct r300_capabilities* caps);

#endif /* R300_CHIPSET_H */
