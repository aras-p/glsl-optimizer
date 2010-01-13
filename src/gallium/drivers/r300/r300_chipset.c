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

#include "r300_chipset.h"

#include "util/u_debug.h"

/* r300_chipset: A file all to itself for deducing the various properties of
 * Radeons. */

/* Parse a PCI ID and fill an r300_capabilities struct with information. */
void r300_parse_chipset(struct r300_capabilities* caps)
{
    /* Reasonable defaults */
    caps->num_vert_fpus = 4;
    caps->has_tcl = debug_get_bool_option("RADEON_NO_TCL", FALSE) ? FALSE : TRUE;
    caps->is_r400 = FALSE;
    caps->is_r500 = FALSE;
    caps->high_second_pipe = FALSE;


    /* Note: These are not ordered by PCI ID. I leave that task to GCC,
     * which will perform the ordering while collating jump tables. Instead,
     * I've tried to group them according to capabilities and age. */
    switch (caps->pci_id) {
        case 0x4144:
            caps->family = CHIP_FAMILY_R300;
            caps->high_second_pipe = TRUE;
            break;

        case 0x4145:
        case 0x4146:
        case 0x4147:
        case 0x4E44:
        case 0x4E45:
        case 0x4E46:
        case 0x4E47:
            caps->family = CHIP_FAMILY_R300;
            caps->high_second_pipe = TRUE;
            break;

        case 0x4150:
        case 0x4151:
        case 0x4152:
        case 0x4153:
        case 0x4154:
        case 0x4155:
        case 0x4156:
        case 0x4E50:
        case 0x4E51:
        case 0x4E52:
        case 0x4E53:
        case 0x4E54:
        case 0x4E56:
            caps->family = CHIP_FAMILY_RV350;
            caps->high_second_pipe = TRUE;
            break;

        case 0x4148:
        case 0x4149:
        case 0x414A:
        case 0x414B:
        case 0x4E48:
        case 0x4E49:
        case 0x4E4B:
            caps->family = CHIP_FAMILY_R350;
            caps->high_second_pipe = TRUE;
            break;

        case 0x4E4A:
            caps->family = CHIP_FAMILY_R360;
            caps->high_second_pipe = TRUE;
            break;

        case 0x5460:
        case 0x5462:
        case 0x5464:
        case 0x5B60:
        case 0x5B62:
        case 0x5B63:
        case 0x5B64:
        case 0x5B65:
            caps->family = CHIP_FAMILY_RV370;
            caps->high_second_pipe = TRUE;
            break;

        case 0x3150:
        case 0x3152:
        case 0x3154:
        case 0x3E50:
        case 0x3E54:
            caps->family = CHIP_FAMILY_RV380;
            caps->high_second_pipe = TRUE;
            break;

        case 0x4A48:
        case 0x4A49:
        case 0x4A4A:
        case 0x4A4B:
        case 0x4A4C:
        case 0x4A4D:
        case 0x4A4E:
        case 0x4A4F:
        case 0x4A50:
        case 0x4A54:
            caps->family = CHIP_FAMILY_R420;
            caps->num_vert_fpus = 6;
            caps->is_r400 = TRUE;
            break;

        case 0x5548:
        case 0x5549:
        case 0x554A:
        case 0x554B:
        case 0x5550:
        case 0x5551:
        case 0x5552:
        case 0x5554:
        case 0x5D57:
            caps->family = CHIP_FAMILY_R423;
            caps->num_vert_fpus = 6;
            caps->is_r400 = TRUE;
            break;

        case 0x554C:
        case 0x554D:
        case 0x554E:
        case 0x554F:
        case 0x5D48:
        case 0x5D49:
        case 0x5D4A:
            caps->family = CHIP_FAMILY_R430;
            caps->num_vert_fpus = 6;
            caps->is_r400 = TRUE;
            break;

        case 0x5D4C:
        case 0x5D4D:
        case 0x5D4E:
        case 0x5D4F:
        case 0x5D50:
        case 0x5D52:
            caps->family = CHIP_FAMILY_R480;
            caps->num_vert_fpus = 6;
            caps->is_r400 = TRUE;
            break;

        case 0x4B48:
        case 0x4B49:
        case 0x4B4A:
        case 0x4B4B:
        case 0x4B4C:
            caps->family = CHIP_FAMILY_R481;
            caps->num_vert_fpus = 6;
            caps->is_r400 = TRUE;
            break;

        case 0x5E4C:
        case 0x5E4F:
        case 0x564A:
        case 0x564B:
        case 0x564F:
        case 0x5652:
        case 0x5653:
        case 0x5657:
        case 0x5E48:
        case 0x5E4A:
        case 0x5E4B:
        case 0x5E4D:
            caps->family = CHIP_FAMILY_RV410;
            caps->num_vert_fpus = 6;
            caps->is_r400 = TRUE;
            break;

        case 0x5954:
        case 0x5955:
            caps->family = CHIP_FAMILY_RS480;
            caps->has_tcl = FALSE;
            break;

        case 0x5974:
        case 0x5975:
            caps->family = CHIP_FAMILY_RS482;
            caps->has_tcl = FALSE;
            break;

        case 0x5A41:
        case 0x5A42:
            caps->family = CHIP_FAMILY_RS400;
            caps->has_tcl = FALSE;
            break;

        case 0x5A61:
        case 0x5A62:
            caps->family = CHIP_FAMILY_RC410;
            caps->has_tcl = FALSE;
            break;

        case 0x791E:
        case 0x791F:
            caps->family = CHIP_FAMILY_RS690;
            caps->has_tcl = FALSE;
            caps->is_r400 = TRUE;
            break;

        case 0x793F:
        case 0x7941:
        case 0x7942:
            caps->family = CHIP_FAMILY_RS600;
            caps->has_tcl = FALSE;
            caps->is_r400 = TRUE;
            break;

        case 0x796C:
        case 0x796D:
        case 0x796E:
        case 0x796F:
            caps->family = CHIP_FAMILY_RS740;
            caps->has_tcl = FALSE;
            caps->is_r400 = TRUE;
            break;

        case 0x7100:
        case 0x7101:
        case 0x7102:
        case 0x7103:
        case 0x7104:
        case 0x7105:
        case 0x7106:
        case 0x7108:
        case 0x7109:
        case 0x710A:
        case 0x710B:
        case 0x710C:
        case 0x710E:
        case 0x710F:
            caps->family = CHIP_FAMILY_R520;
            caps->num_vert_fpus = 8;
            caps->is_r500 = TRUE;
            break;

        case 0x7140:
        case 0x7141:
        case 0x7142:
        case 0x7143:
        case 0x7144:
        case 0x7145:
        case 0x7146:
        case 0x7147:
        case 0x7149:
        case 0x714A:
        case 0x714B:
        case 0x714C:
        case 0x714D:
        case 0x714E:
        case 0x714F:
        case 0x7151:
        case 0x7152:
        case 0x7153:
        case 0x715E:
        case 0x715F:
        case 0x7180:
        case 0x7181:
        case 0x7183:
        case 0x7186:
        case 0x7187:
        case 0x7188:
        case 0x718A:
        case 0x718B:
        case 0x718C:
        case 0x718D:
        case 0x718F:
        case 0x7193:
        case 0x7196:
        case 0x719B:
        case 0x719F:
        case 0x7200:
        case 0x7210:
        case 0x7211:
            caps->family = CHIP_FAMILY_RV515;
            caps->num_vert_fpus = 2;
            caps->is_r500 = TRUE;
            break;

        case 0x71C0:
        case 0x71C1:
        case 0x71C2:
        case 0x71C3:
        case 0x71C4:
        case 0x71C5:
        case 0x71C6:
        case 0x71C7:
        case 0x71CD:
        case 0x71CE:
        case 0x71D2:
        case 0x71D4:
        case 0x71D5:
        case 0x71D6:
        case 0x71DA:
        case 0x71DE:
            caps->family = CHIP_FAMILY_RV530;
            caps->num_vert_fpus = 5;
            caps->is_r500 = TRUE;
            break;

        case 0x7240:
        case 0x7243:
        case 0x7244:
        case 0x7245:
        case 0x7246:
        case 0x7247:
        case 0x7248:
        case 0x7249:
        case 0x724A:
        case 0x724B:
        case 0x724C:
        case 0x724D:
        case 0x724E:
        case 0x724F:
        case 0x7284:
            caps->family = CHIP_FAMILY_R580;
            caps->num_vert_fpus = 8;
            caps->is_r500 = TRUE;
            break;

        case 0x7280:
            caps->family = CHIP_FAMILY_RV570;
            caps->num_vert_fpus = 5;
            caps->is_r500 = TRUE;
            break;

        case 0x7281:
        case 0x7283:
        case 0x7287:
        case 0x7288:
        case 0x7289:
        case 0x728B:
        case 0x728C:
        case 0x7290:
        case 0x7291:
        case 0x7293:
        case 0x7297:
            caps->family = CHIP_FAMILY_RV560;
            caps->num_vert_fpus = 5;
            caps->is_r500 = TRUE;
            break;

        default:
            debug_printf("r300: Warning: Unknown chipset 0x%x\n",
                caps->pci_id);
            break;
    }
}
