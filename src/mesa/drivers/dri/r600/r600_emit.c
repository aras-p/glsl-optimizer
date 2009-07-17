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
#include "main/image.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r600_context.h"
#include "r600_emit.h"

void r600EmitCacheFlush(context_t *rmesa)
{
	BATCH_LOCALS(&rmesa->radeon);
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
#ifdef RADEON_DEBUG_BO
	pbo = radeon_bo_open(radeonctx->radeonScreen->bom,
					     0, 
                         sizeinDWORD * 4, 
                         256, 
                         RADEON_GEM_DOMAIN_GTT,
					     0,
                         szShaderUsage);
#else
    pbo = radeon_bo_open(radeonctx->radeonScreen->bom,
					     0, 
                         sizeinDWORD * 4, 
                         256, 
                         RADEON_GEM_DOMAIN_GTT,
					     0);
#endif /* RADEON_DEBUG_BO */

	if (!pbo) 
    {
		rcommonFlushCmdBuf(radeonctx, __FUNCTION__);
		goto shader_again_alloc;
	}

        if (radeon_cs_space_check_with_bo(radeonctx->cmdbuf.cs,
                                          pbo,
                                          RADEON_GEM_DOMAIN_GTT, 0))
                fprintf(stderr,"failure to revalidate BOs - badness\n");


	radeon_bo_map(pbo, 1);

    radeon_bo_ref(pbo);

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

    if (pbo) {
	    radeon_bo_unmap(pbo);
	    radeon_bo_unref(pbo); /* when bo->cref <= 0, bo will be bo_free */
    }

    return GL_TRUE;
}
