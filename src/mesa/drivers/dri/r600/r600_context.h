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
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R600_CONTEXT_H__
#define __R600_CONTEXT_H__

#include "tnl/t_vertex.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "texmem.h"
#include "radeon_common.h"

#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"

#include "r700_chip.h"
#include "r600_tex.h"
#include "r700_oglprog.h"
#include "r700_vertprog.h"

struct r600_context;
typedef struct r600_context context_t;

#include "main/mm.h"

/************ DMA BUFFERS **************/

/* The blit width for texture uploads
 */
#define R600_BLIT_WIDTH_BYTES 1024
#define R600_MAX_TEXTURE_UNITS 8

struct r600_texture_state {
	int tc_count;		/* number of incoming texture coordinates from VAP */
};

/* Perhaps more if we store programs in vmem? */
/* drm_r600_cmd_header_t->vpu->count is unsigned char */
#define VSF_MAX_FRAGMENT_LENGTH (255*4)

/* Can be tested with colormat currently. */
#define VSF_MAX_FRAGMENT_TEMPS (14)

#define STATE_R600_WINDOW_DIMENSION (STATE_INTERNAL_DRIVER+0)
#define STATE_R600_TEXRECT_FACTOR (STATE_INTERNAL_DRIVER+1)

extern int hw_tcl_on;

#define COLOR_IS_RGBA
#define TAG(x) r600##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#define R600_FALLBACK_NONE 0
#define R600_FALLBACK_TCL 1
#define R600_FALLBACK_RAST 2

struct r600_hw_state {
	struct radeon_state_atom sq;
	struct radeon_state_atom db;
	struct radeon_state_atom stencil;
	struct radeon_state_atom db_target;
	struct radeon_state_atom sc;
	struct radeon_state_atom scissor;
	struct radeon_state_atom aa;
	struct radeon_state_atom cl;
	struct radeon_state_atom gb;
	struct radeon_state_atom ucp;
	struct radeon_state_atom su;
	struct radeon_state_atom poly;
	struct radeon_state_atom cb;
	struct radeon_state_atom clrcmp;
	struct radeon_state_atom blnd;
	struct radeon_state_atom blnd_clr;
	struct radeon_state_atom cb_target;
	struct radeon_state_atom sx;
	struct radeon_state_atom vgt;
	struct radeon_state_atom spi;
	struct radeon_state_atom vpt;

	struct radeon_state_atom fs;
	struct radeon_state_atom vs;
	struct radeon_state_atom ps;

	struct radeon_state_atom vs_consts;
	struct radeon_state_atom ps_consts;

	struct radeon_state_atom vtx;
	struct radeon_state_atom tx;
	struct radeon_state_atom tx_smplr;
	struct radeon_state_atom tx_brdr_clr;
};

/**
 * \brief R600 context structure.
 */
struct r600_context {
	struct radeon_context radeon;	/* parent class, must be first */

	/* ------ */
	R700_CHIP_CONTEXT hw;

	struct r600_hw_state atoms;

	struct r700_vertex_program *selected_vp;

	/* Vertex buffers
	 */
	GLvector4f dummy_attrib[_TNL_ATTRIB_MAX];
	GLvector4f *temp_attrib[_TNL_ATTRIB_MAX];

};

#define R700_CONTEXT(ctx)		((context_t *)(ctx->DriverCtx))
#define GL_CONTEXT(context)     ((GLcontext *)(context->radeon.glCtx))

extern GLboolean r600CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);

#define R700_CONTEXT_STATES(context) ((R700_CHIP_CONTEXT *)(&context->hw))

#define R600_NEWPRIM( rmesa )			\
do {						\
	if ( rmesa->radeon.dma.flush )			\
		rmesa->radeon.dma.flush( rmesa->radeon.glCtx );	\
} while (0)

#define R600_STATECHANGE(r600, ATOM)			\
do {							\
	R600_NEWPRIM(r600);					\
	r600->atoms.ATOM.dirty = GL_TRUE;					\
	r600->radeon.hw.is_dirty = GL_TRUE;			\
} while(0)

extern GLboolean r700SyncSurf(context_t *context,
			      struct radeon_bo *pbo,
			      uint32_t read_domain,
			      uint32_t write_domain,
			      uint32_t sync_type);

extern void r700SetupStreams(GLcontext * ctx);
extern void r700Start3D(context_t *context);
extern void r600InitAtoms(context_t *context);

#define RADEON_D_CAPTURE 0
#define RADEON_D_PLAYBACK 1
#define RADEON_D_PLAYBACK_RAW 2
#define RADEON_D_T 3

#define r600PackFloat32 radeonPackFloat32
#define r600PackFloat24 radeonPackFloat24

#endif				/* __R600_CONTEXT_H__ */
