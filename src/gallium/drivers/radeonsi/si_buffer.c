/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 */

#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

#include "si.h"
#include "si_pipe.h"

void si_upload_const_buffer(struct si_context *sctx, struct r600_resource **rbuffer,
			const uint8_t *ptr, unsigned size,
			uint32_t *const_offset)
{
	if (SI_BIG_ENDIAN) {
		uint32_t *tmpPtr;
		unsigned i;

		if (!(tmpPtr = malloc(size))) {
			R600_ERR("Failed to allocate BE swap buffer.\n");
			return;
		}

		for (i = 0; i < size / 4; ++i) {
			tmpPtr[i] = util_bswap32(((uint32_t *)ptr)[i]);
		}

		u_upload_data(sctx->b.uploader, 0, size, tmpPtr, const_offset,
				(struct pipe_resource**)rbuffer);

		free(tmpPtr);
	} else {
		u_upload_data(sctx->b.uploader, 0, size, ptr, const_offset,
					(struct pipe_resource**)rbuffer);
	}
}
