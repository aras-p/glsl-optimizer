/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  Mesa transformation pipeline with GLDirect fastpath
*
****************************************************************************/

//#include "../GLDirect.h"

#include "dglcontext.h"
#include "ddlog.h"
#include "gld_dx7.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

//---------------------------------------------------------------------------

extern struct tnl_pipeline_stage _gld_d3d_render_stage;
extern struct tnl_pipeline_stage _gld_mesa_render_stage;

static const struct tnl_pipeline_stage *gld_pipeline[] = {
	&_gld_d3d_render_stage,			// Direct3D TnL
	&_tnl_vertex_transform_stage,
	&_tnl_normal_transform_stage,
	&_tnl_lighting_stage,
	&_tnl_fog_coordinate_stage,	/* TODO: Omit fog stage. ??? */
	&_tnl_texgen_stage,
	&_tnl_texture_transform_stage,
	&_tnl_point_attenuation_stage,
	&_gld_mesa_render_stage,		// Mesa TnL, D3D rendering
	0,
};

//---------------------------------------------------------------------------

void gldInstallPipeline_DX7(
	GLcontext *ctx)
{
	// Remove any existing pipeline	stages,
	// then install GLDirect pipeline stages.

	_tnl_destroy_pipeline(ctx);
	_tnl_install_pipeline(ctx, gld_pipeline);
}

//---------------------------------------------------------------------------
