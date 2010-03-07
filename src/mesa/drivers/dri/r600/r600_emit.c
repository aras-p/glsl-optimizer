/**************************************************************************

Copyright 2008, 2009 Advanced Micro Devices Inc. (AMD)

Copyright (C) Advanced Micro Devices Inc. (AMD)  2009.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/imports.h"
#include "main/macros.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r600_context.h"
#include "r600_emit.h"

void r600EmitCacheFlush(context_t *rmesa)
{
}

GLboolean r600EmitShader(GLcontext * ctx,
                         void ** shaderbo,
			 GLvoid * data,
                         int sizeinDWORD,
                         char * szShaderUsage)
{
	radeonContextPtr radeonctx = RADEON_CONTEXT(ctx);
	struct radeon_bo * pbo;
	uint32_t *out;
shader_again_alloc:
	pbo = radeon_bo_open(radeonctx->radeonScreen->bom,
			0,
			sizeinDWORD * 4,
			256,
			RADEON_GEM_DOMAIN_GTT,
			0);

	radeon_print(RADEON_SHADER, RADEON_NORMAL, "%s %p size %d: %s\n", __func__, pbo, sizeinDWORD, szShaderUsage);

	if (!pbo) {
		radeon_print(RADEON_MEMORY | RADEON_CS, RADEON_IMPORTANT, "No memory for buffer object. Flushing command buffer.\n");
		rcommonFlushCmdBuf(radeonctx, __FUNCTION__);
		goto shader_again_alloc;
	}

	radeon_cs_space_add_persistent_bo(radeonctx->cmdbuf.cs,
			pbo,
			RADEON_GEM_DOMAIN_GTT, 0);

	if (radeon_cs_space_check_with_bo(radeonctx->cmdbuf.cs,
				pbo,
				RADEON_GEM_DOMAIN_GTT, 0)) {
		radeon_error("failure to revalidate BOs - badness\n");
		return GL_FALSE;
	}

	radeon_bo_map(pbo, 1);

	out = (uint32_t*)(pbo->ptr);

	memcpy(out, data, sizeinDWORD * 4);

	radeon_bo_unmap(pbo);

	*shaderbo = (void*)pbo;

	return GL_TRUE;
}

GLboolean r600DeleteShader(GLcontext * ctx,
                           void * shaderbo)
{
    struct radeon_bo * pbo = (struct radeon_bo *)shaderbo;

    radeon_print(RADEON_SHADER, RADEON_NORMAL, "%s: %p\n", __func__, pbo);

    if (pbo) {
	    if (pbo->ptr)
		radeon_bo_unmap(pbo);
	    radeon_bo_unref(pbo); /* when bo->cref <= 0, bo will be bo_free */
    }

    return GL_TRUE;
}
