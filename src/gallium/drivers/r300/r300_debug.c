/*
 * Copyright 2009 Nicolai Haehnle <nhaehnle@gmail.com>
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

#include "r300_context.h"


struct debug_option {
    const char * name;
    unsigned flag;
    const char * description;
};

static struct debug_option debug_options[] = {
    { "help", DBG_HELP, "Helpful meta-information about the driver" },
    { "fp", DBG_FP, "Fragment program handling" },
    { "vp", DBG_VP, "Vertex program handling" },
    { "cs", DBG_CS, "Command submissions" },
    { "draw", DBG_DRAW, "Draw and emit" },
    { "tex", DBG_TEX, "Textures" },
    { "fall", DBG_FALL, "Fallbacks" },

    { "all", ~0, "Convenience option that enables all debug flags" },

    /* must be last */
    { 0, 0, 0 }
};

void r300_init_debug(struct r300_screen * screen)
{
    const char * options = debug_get_option("RADEON_DEBUG", 0);
    boolean printhint = FALSE;
    size_t length;
    struct debug_option * opt;

    if (options) {
        while(*options) {
            if (*options == ' ' || *options == ',') {
                options++;
                continue;
            }

            length = strcspn(options, " ,");

            for(opt = debug_options; opt->name; ++opt) {
                if (!strncmp(options, opt->name, length)) {
                    screen->debug |= opt->flag;
                    break;
                }
            }

            if (!opt->name) {
                debug_printf("Unknown debug option: %s\n", options);
                printhint = TRUE;
            }

            options += length;
        }

        if (!screen->debug)
            printhint = TRUE;
    }

    if (printhint || screen->debug & DBG_HELP) {
        debug_printf("You can enable debug output by setting the RADEON_DEBUG environment variable\n"
                     "to a comma-separated list of debug options. Available options are:\n");
        for(opt = debug_options; opt->name; ++opt) {
            debug_printf("    %s: %s\n", opt->name, opt->description);
        }
    }
}
