/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"

#include "via_context.h"
#include "via_vb.h"
#include "via_ioctl.h"
#include "via_tris.h"
#include "via_state.h"

static struct {
    void                (*emit)(GLcontext *, GLuint, GLuint, void *, GLuint);
    tnl_interp_func      interp;
    tnl_copy_pv_func     copy_pv;
    GLboolean           (*check_tex_sizes)(GLcontext *ctx);
    GLuint               vertex_size;
    GLuint               vertex_format;
} setup_tab[VIA_MAX_SETUP];

#define TINY_VERTEX_FORMAT 	1
#define NOTEX_VERTEX_FORMAT 	2
#define TEX0_VERTEX_FORMAT 	3
#define TEX1_VERTEX_FORMAT 	4
			    
#define PROJ_TEX1_VERTEX_FORMAT 0
#define TEX2_VERTEX_FORMAT      0
#define TEX3_VERTEX_FORMAT      0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & VIA_XYZW_BIT)
#define DO_RGBA (IND & VIA_RGBA_BIT)
#define DO_SPEC (IND & VIA_SPEC_BIT)
#define DO_FOG  (IND & VIA_FOG_BIT)
#define DO_TEX0 (IND & VIA_TEX0_BIT)
#define DO_TEX1 (IND & VIA_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & VIA_PTEX_BIT)

#define VERTEX viaVertex
#define VERTEX_COLOR via_color_t
#define GET_VIEWPORT_MAT() VIA_CONTEXT(ctx)->ViewportMatrix.m
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() VIA_CONTEXT(ctx)->vertexFormat
#define GET_VERTEX_SIZE() VIA_CONTEXT(ctx)->vertexSize * sizeof(GLuint)
#define GET_VERTEX_STORE() VIA_CONTEXT(ctx)->verts
#define INVALIDATE_STORED_VERTICES()

#define HAVE_HW_VIEWPORT    0
#define HAVE_HW_DIVIDE      0
#define HAVE_RGBA_COLOR     0
#define HAVE_TINY_VERTICES  1
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  0

#define UNVIEWPORT_VARS  GLfloat h = VIA_CONTEXT(ctx)->driDrawable->h, \
                                 depth_max = VIA_CONTEXT(ctx)->depth_max;
#define UNVIEWPORT_X(x)  x - SUBPIXEL_X
#define UNVIEWPORT_Y(y)  - y + h + SUBPIXEL_Y
#define UNVIEWPORT_Z(z)  z * (float)depth_max

#define PTEX_FALLBACK() FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_TEXTURE, 1)

#define INTERP_VERTEX setup_tab[VIA_CONTEXT(ctx)->setupIndex].interp
#define COPY_PV_VERTEX setup_tab[VIA_CONTEXT(ctx)->setupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) via_##x
#include "tnl_dd/t_dd_vb.c"

/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/
#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_TEX0_BIT | VIA_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_TEX0_BIT | VIA_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_TEX0_BIT | VIA_TEX1_BIT |\
             VIA_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_SPEC_BIT | VIA_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_SPEC_BIT | VIA_TEX0_BIT |\
             VIA_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_SPEC_BIT | VIA_TEX0_BIT |\
             VIA_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_SPEC_BIT | VIA_TEX0_BIT |\
             VIA_TEX1_BIT | VIA_PTEX_BIT)	     
#define TAG(x) x##_wgspt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_TEX0_BIT |\
             VIA_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_TEX0_BIT |\
             VIA_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_TEX0_BIT |\
             VIA_TEX1_BIT | VIA_PTEX_BIT)
#define TAG(x) x##_wgfpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_SPEC_BIT |\
             VIA_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_SPEC_BIT |\
             VIA_TEX0_BIT | VIA_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_SPEC_BIT |\
             VIA_TEX0_BIT | VIA_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (VIA_XYZW_BIT | VIA_RGBA_BIT | VIA_FOG_BIT | VIA_SPEC_BIT |\
             VIA_TEX0_BIT | VIA_TEX1_BIT | VIA_PTEX_BIT)
#define TAG(x) x##_wgfspt0t1
#include "tnl_dd/t_dd_vbtmp.h"

static void init_setup_tab(void) {

    init_wg();
    init_wgs();
    init_wgt0();
    init_wgt0t1();
    init_wgpt0();
    init_wgpt0t1();    
    init_wgst0();
    init_wgst0t1();
    init_wgspt0();
    init_wgspt0t1();
    init_wgf();
    init_wgfs();
    init_wgft0();
    init_wgft0t1();
    init_wgfpt0();
    init_wgfpt0t1();
    init_wgfst0();
    init_wgfst0t1();
    init_wgfspt0();
    init_wgfspt0t1();
}

void viaPrintSetupFlags(char *msg, GLuint flags) {
    if (VIA_DEBUG) fprintf(stderr, "%s(%x): %s%s%s%s%s%s\n",
            msg,
            (int)flags,
            (flags & VIA_XYZW_BIT)     ? " xyzw," : "",
            (flags & VIA_RGBA_BIT)     ? " rgba," : "",
            (flags & VIA_SPEC_BIT)     ? " spec," : "",
            (flags & VIA_FOG_BIT)      ? " fog," : "",
            (flags & VIA_TEX0_BIT)     ? " tex-0," : "",
            (flags & VIA_TEX1_BIT)     ? " tex-1," : "");
}

void viaCheckTexSizes(GLcontext *ctx) 
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (!setup_tab[vmesa->setupIndex].check_tex_sizes(ctx)) {
        /* Invalidate stored verts
         */
        vmesa->setupNewInputs = ~0;
        vmesa->setupIndex |= VIA_PTEX_BIT;

        if (!vmesa->Fallback &&
            !(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
            tnl->Driver.Render.Interp = setup_tab[vmesa->setupIndex].interp;
            tnl->Driver.Render.CopyPV = setup_tab[vmesa->setupIndex].copy_pv;
        }

	if (vmesa->Fallback)
	   tnl->Driver.Render.Start(ctx);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

void viaBuildVertices(GLcontext *ctx,
                      GLuint start,
                      GLuint count,
                      GLuint newinputs) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint stride = vmesa->vertexSize * sizeof(GLuint);
    GLubyte *v = (GLubyte *)vmesa->verts + start * stride;

    newinputs |= vmesa->setupNewInputs;
    vmesa->setupNewInputs = 0;

    if (!newinputs)
        return;

    if (newinputs & VERT_BIT_POS) {
        setup_tab[vmesa->setupIndex].emit(ctx, start, count, v, stride);
    }
    else {
        GLuint ind = 0;

        if (newinputs & VERT_BIT_COLOR0)
            ind |= VIA_RGBA_BIT;

        if (newinputs & VERT_BIT_COLOR1)
            ind |= VIA_SPEC_BIT;

        if (newinputs & VERT_BIT_TEX0)
            ind |= VIA_TEX0_BIT;

        if (newinputs & VERT_BIT_TEX1)
            ind |= VIA_TEX1_BIT;

        if (newinputs & VERT_BIT_FOG)
            ind |= VIA_FOG_BIT;

        if (vmesa->setupIndex & VIA_PTEX_BIT)
            ind = ~0;
	
        ind &= vmesa->setupIndex;
	ind |= VIA_XYZW_BIT;
        
	if (ind) {
            setup_tab[ind].emit(ctx, start, count, v, stride);
        }
    }
}

void viaChooseVertexState(GLcontext *ctx) {
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint ind = VIA_XYZW_BIT | VIA_RGBA_BIT;

    if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
        ind |= VIA_SPEC_BIT;

    if (ctx->Fog.Enabled)
        ind |= VIA_FOG_BIT;

    if (ctx->Texture._EnabledUnits > 1)
        ind |= VIA_TEX1_BIT | VIA_TEX0_BIT;
    else if (ctx->Texture._EnabledUnits == 1)
        ind |= VIA_TEX0_BIT;

    vmesa->setupIndex = ind;
    viaPrintSetupFlags(__FUNCTION__, ind);

    if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
        tnl->Driver.Render.Interp = via_interp_extras;
        tnl->Driver.Render.CopyPV = via_copy_pv_extras;
    }
    else {
        tnl->Driver.Render.Interp = setup_tab[ind].interp;
        tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
    }

    vmesa->vertexSize = setup_tab[ind].vertex_size;
    vmesa->vertexFormat = setup_tab[ind].vertex_format;

    if (VIA_DEBUG) fprintf(stderr, "%s, size %d\n", __FUNCTION__, vmesa->vertexSize);
}

void *via_emit_contiguous_verts(GLcontext *ctx,
				GLuint start,
				GLuint count,
				void *dest) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint stride = vmesa->vertexSize * 4;    
    setup_tab[vmesa->setupIndex].emit(ctx, start, count, dest, stride);
    return (void *)((char *)dest + (count - start) * stride); 
}


void viaInitVB(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint size = TNL_CONTEXT(ctx)->vb.Size;

    vmesa->verts = ALIGN_MALLOC(size * 4 * 16, 32);

    {
        static int firsttime = 1;
        if (firsttime) {
            init_setup_tab();
            firsttime = 0;
        }
    }
}

void viaFreeVB(GLcontext *ctx) {
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (vmesa->verts) {
        ALIGN_FREE(vmesa->verts);
        vmesa->verts = 0;
    }
}
