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

struct r600_context;
typedef struct r600_context context_t;

GLboolean r700SendPSState(context_t *context);
GLboolean r700SendVSState(context_t *context);
GLboolean r700SendSQConfig(context_t *context);

#include "main/mm.h"

/* From http://gcc. gnu.org/onlinedocs/gcc-3.2.3/gcc/Variadic-Macros.html .
   I suppose we could inline this and use macro to fetch out __LINE__ and stuff in case we run into trouble
   with other compilers ... GLUE!
*/
#define WARN_ONCE(a, ...)	{ \
	static int warn##__LINE__=1; \
	if(warn##__LINE__){ \
		fprintf(stderr, "*********************************WARN_ONCE*********************************\n"); \
		fprintf(stderr, "File %s function %s line %d\n", \
			__FILE__, __FUNCTION__, __LINE__); \
		fprintf(stderr,  a, ## __VA_ARGS__);\
		fprintf(stderr, "***************************************************************************\n"); \
		warn##__LINE__=0;\
		} \
	}

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

#define PFS_MAX_ALU_INST	64
#define PFS_MAX_TEX_INST	64
#define PFS_MAX_TEX_INDIRECT 4
#define PFS_NUM_TEMP_REGS	32
#define PFS_NUM_CONST_REGS	16

#define R600_MAX_AOS_ARRAYS		16

#define REG_COORDS	0
#define REG_COLOR0	1
#define REG_TEX0	2

#define R600_FALLBACK_NONE 0
#define R600_FALLBACK_TCL 1
#define R600_FALLBACK_RAST 2

enum 
{
    NO_SHIFT    = 0,
    LEFT_SHIFT  = 1,
    RIGHT_SHIFT = 2,
};

typedef struct offset_modifiers
{
    GLuint shift;
    GLuint shiftbits;
    GLuint mask;
} offset_modifiers;

/**
 * \brief R600 context structure.
 */
struct r600_context {
	struct radeon_context radeon;	/* parent class, must be first */

	/* ------ */
	R700_CHIP_CONTEXT hw;

	/* Vertex buffers
	 */
	GLvector4f dummy_attrib[_TNL_ATTRIB_MAX];
	GLvector4f *temp_attrib[_TNL_ATTRIB_MAX];

};

#define R700_CONTEXT(ctx)		((context_t *)(ctx->DriverCtx))
#define GL_CONTEXT(context)     ((GLcontext *)(context->radeon.glCtx))

extern void r600DestroyContext(__DRIcontextPrivate * driContextPriv);
extern GLboolean r600CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);

#define R700_CONTEXT_STATES(context) ((R700_CHIP_CONTEXT *)(&context->hw))

extern GLboolean r700InitChipObject(context_t *context);
extern GLboolean r700SendContextStates(context_t *context);
extern GLboolean r700SendViewportState(context_t *context, int id);
extern GLboolean r700SendRenderTargetState(context_t *context, int id);

extern int       r700SetupStreams(GLcontext * ctx);
extern void      r700SetupVTXConstants(GLcontext  * ctx, 
				       unsigned int nStreamID,
				       void *       pAos,
				       unsigned int size,      /* number of elements in vector */
				       unsigned int stride,
				       unsigned int Count);    /* number of vectors in stream */

#define RADEON_D_CAPTURE 0
#define RADEON_D_PLAYBACK 1
#define RADEON_D_PLAYBACK_RAW 2
#define RADEON_D_T 3

#define r600PackFloat32 radeonPackFloat32
#define r600PackFloat24 radeonPackFloat24

#endif				/* __R600_CONTEXT_H__ */
