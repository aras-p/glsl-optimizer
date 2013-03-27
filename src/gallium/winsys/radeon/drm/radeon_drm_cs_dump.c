/*
 * Copyright © 2013 Jérôme Glisse
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Jérôme Glisse <jglisse@redhat.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xf86drm.h>
#include "radeon_drm_cs.h"
#include "radeon_drm_bo.h"

#if RADEON_CS_DUMP_ON_LOCKUP
static bool dumped = false;

void radeon_dump_cs_on_lockup(struct radeon_cs_context *csc)
{
    struct drm_radeon_gem_busy args;
    FILE *dump;
    unsigned i, lockup;
    uint32_t *ptr;

    /* only dump the first cs to cause a lockup */
    if (!csc->crelocs || dumped) {
        /* can not determine if there was a lockup if no bo were use by
         * the cs and most likely in such case no lockup occurs
         */
        return;
    }

    memset(&args, 0, sizeof(args));
    args.handle = csc->relocs_bo[0]->handle;
    for (i = 0; i < 10; i++) {
        usleep(5);
        lockup = drmCommandWriteRead(csc->fd, DRM_RADEON_GEM_BUSY, &args, sizeof(args));
        if (!lockup) {
            break;
        }
    }
    if (!lockup || i < 10) {
        return;
    }

    /* ok we are most likely facing a lockup write the standalone replay file */
    dump = fopen("radeon_lockup.c", "w");
    if (dump == NULL) {
        return;
    }
    fprintf(dump, "/* To build this file you will need to copy radeon_ctx.h\n");
    fprintf(dump, " * in same directory. You can find radeon_ctx.h in mesa tree :\n");
    fprintf(dump, " * mesa/src/gallium/winsys/radeon/tools/radeon_ctx.h\n");
    fprintf(dump, " * Build with :\n");
    fprintf(dump, " * gcc -O0 -g radeon_lockup.c -ldrm -o radeon_lockup -I/usr/include/libdrm\n");
    fprintf(dump, " */\n");
    fprintf(dump, "#include <stdio.h>\n");
    fprintf(dump, "#include <stdint.h>\n");
    fprintf(dump, "#include \"radeon_ctx.h\"\n");
    fprintf(dump, "\n");
    fprintf(dump, "#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))\n");
    fprintf(dump, "\n");

    for (i = 0; i < csc->crelocs; i++) {
        unsigned j, ndw = (csc->relocs_bo[i]->base.size + 3) >> 2;

        ptr = radeon_bo_do_map(csc->relocs_bo[i]);
        if (ptr) {
            fprintf(dump, "static uint32_t bo_%04d_data[%d] = {\n   ", i, ndw);
            for (j = 0; j < ndw; j++) {
                if (j && !(j % 8)) {
                    uint32_t offset = (j - 8) << 2;
                    fprintf(dump, "  /* [0x%08x] va[0x%016lx] */\n   ", offset, offset + csc->relocs_bo[i]->va);
                }
                fprintf(dump, " 0x%08x,", ptr[j]);
            }
            fprintf(dump, "};\n\n");
        }
    }

    fprintf(dump, "static uint32_t bo_relocs[%d] = {\n", csc->crelocs * 4);
    for (i = 0; i < csc->crelocs; i++) {
        fprintf(dump, "    0x%08x, 0x%08x, 0x%08x, 0x%08x,\n",
                0, csc->relocs[i].read_domains, csc->relocs[i].write_domain, csc->relocs[i].flags);
    }
    fprintf(dump, "};\n\n");

    fprintf(dump, "static uint32_t cs[] = {\n");
    ptr = csc->buf;
    for (i = 0; i < csc->chunks[0].length_dw; i++) {
        fprintf(dump, "    0x%08x,\n", ptr[i]);
    }
    fprintf(dump, "};\n\n");

    fprintf(dump, "static uint32_t cs_flags[2] = {\n");
    fprintf(dump, "    0x%08x,\n", csc->flags[0]);
    fprintf(dump, "    0x%08x,\n", csc->flags[1]);
    fprintf(dump, "};\n\n");

    fprintf(dump, "int main(int argc, char *argv[])\n");
    fprintf(dump, "{\n");
    fprintf(dump, "    struct bo *bo[%d];\n", csc->crelocs);
    fprintf(dump, "    struct ctx ctx;\n");
    fprintf(dump, "\n");
    fprintf(dump, "    ctx_init(&ctx);\n");
    fprintf(dump, "\n");

    for (i = 0; i < csc->crelocs; i++) {
        unsigned ndw = (csc->relocs_bo[i]->base.size + 3) >> 2;
        uint32_t *ptr;

        ptr = radeon_bo_do_map(csc->relocs_bo[i]);
        if (ptr) {
            fprintf(dump, "    bo[%d] = bo_new(&ctx, %d, bo_%04d_data, 0x%016lx, 0x%08x);\n",
                    i, ndw, i, csc->relocs_bo[i]->va, csc->relocs_bo[i]->base.alignment);
        } else {
            fprintf(dump, "    bo[%d] = bo_new(&ctx, %d, NULL, 0x%016lx, 0x%08x);\n",
                    i, ndw, csc->relocs_bo[i]->va, csc->relocs_bo[i]->base.alignment);
        }
    }
    fprintf(dump, "\n");
    fprintf(dump, "    ctx_cs(&ctx, cs, cs_flags, ARRAY_SIZE(cs), bo, bo_relocs, %d);\n", csc->crelocs);
    fprintf(dump, "\n");
    fprintf(dump, "    fprintf(stderr, \"waiting for cs execution to end ....\\n\");\n");
    fprintf(dump, "    bo_wait(&ctx, bo[0]);\n");
    fprintf(dump, "}\n");
    fclose(dump);
    dumped = true;
}
#endif
