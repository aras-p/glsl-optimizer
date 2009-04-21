/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
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

/* Emit vertex data to GART memory
 * Route inputs to the vertex processor
 * This function should never return R600_FALLBACK_TCL when using software tcl.
 */
int r600EmitArrays(GLcontext * ctx)
{
	
	return R600_FALLBACK_NONE;
}

void r600EmitCacheFlush(r600ContextPtr rmesa)
{
	BATCH_LOCALS(&rmesa->radeon);
/*
	BEGIN_BATCH_NO_AUTOSTATE(4);
	OUT_BATCH_REGVAL(R600_RB3D_DSTCACHE_CTLSTAT,
		R600_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
		R600_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
	OUT_BATCH_REGVAL(R600_ZB_ZCACHE_CTLSTAT,
		R600_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
		R600_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);
	END_BATCH();
	COMMIT_BATCH();
*/
}
