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
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include "main/glheader.h"
#include "main/state.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/simple_list.h"

#include "drm.h"
#include "radeon_drm.h"

#include "r300_context.h"
#include "r300_reg.h"
#include "r300_cmdbuf.h"
#include "r300_emit.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_mipmap_tree.h"
#include "radeon_queryobj.h"

/** # of dwords reserved for additional instructions that may need to be written
 * during flushing.
 */
#define SPACE_FOR_FLUSHING	4

static unsigned packet0_count(r300ContextPtr r300, uint32_t *pkt)
{
    if (r300->radeon.radeonScreen->kernel_mm) {
        return ((((*pkt) >> 16) & 0x3FFF) + 1);
    } else {
        drm_r300_cmd_header_t *t = (drm_r300_cmd_header_t*)pkt;
        return t->packet0.count;
    }
}

#define vpu_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->vpu.count)
#define r500fp_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->r500fp.count)

static int check_vpu(GLcontext *ctx, struct radeon_state_atom *atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int cnt;
	int extra = 1;
	cnt = vpu_count(atom->cmd);

	if (r300->radeon.radeonScreen->kernel_mm) {
		extra = 3;
	}

	return cnt ? (cnt * 4) + extra : 0;
}

static int check_vpp(GLcontext *ctx, struct radeon_state_atom *atom)
{
    r300ContextPtr r300 = R300_CONTEXT(ctx);
    int cnt;
    int extra = 1;

    if (r300->radeon.radeonScreen->kernel_mm) {
        cnt = r300->selected_vp->code.constants.Count * 4;
        extra = 3;
    } else {
        cnt = vpu_count(atom->cmd);
        extra = 1;
    }

    return cnt ? (cnt * 4) + extra : 0;
}

void r300_emit_vpu(struct r300_context *r300,
                   uint32_t *data,
                   unsigned len,
                   uint32_t addr)
{
    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH_NO_AUTOSTATE(3 + len);
    OUT_BATCH_REGVAL(R300_VAP_PVS_VECTOR_INDX_REG, addr);
    OUT_BATCH(CP_PACKET0(R300_VAP_PVS_UPLOAD_DATA, len-1) | RADEON_ONE_REG_WR);
    OUT_BATCH_TABLE(data, len);
    END_BATCH();
}

static void emit_vpu_state(GLcontext *ctx, struct radeon_state_atom * atom)
{
    r300ContextPtr r300 = R300_CONTEXT(ctx);
    drm_r300_cmd_header_t cmd;
    uint32_t addr;

    cmd.u = atom->cmd[0];
    addr = (cmd.vpu.adrhi << 8) | cmd.vpu.adrlo;

    r300_emit_vpu(r300, &atom->cmd[1], vpu_count(atom->cmd) * 4, addr);
}

static void emit_vpp_state(GLcontext *ctx, struct radeon_state_atom * atom)
{
    r300ContextPtr r300 = R300_CONTEXT(ctx);
    drm_r300_cmd_header_t cmd;
    uint32_t addr;

    cmd.u = atom->cmd[0];
    addr = (cmd.vpu.adrhi << 8) | cmd.vpu.adrlo;

    r300_emit_vpu(r300, &atom->cmd[1], r300->selected_vp->code.constants.Count * 4, addr);
}

void r500_emit_fp(struct r300_context *r300,
                  uint32_t *data,
                  unsigned len,
                  uint32_t addr,
                  unsigned type,
                  unsigned clamp)
{
    BATCH_LOCALS(&r300->radeon);

    addr |= (type << 16);
    addr |= (clamp << 17);

    BEGIN_BATCH_NO_AUTOSTATE(len + 3);
    OUT_BATCH(CP_PACKET0(R500_GA_US_VECTOR_INDEX, 0));
    OUT_BATCH(addr);
    OUT_BATCH(CP_PACKET0(R500_GA_US_VECTOR_DATA, len-1) | RADEON_ONE_REG_WR);
    OUT_BATCH_TABLE(data, len);
    END_BATCH();
}

static void emit_r500fp_atom(GLcontext *ctx, struct radeon_state_atom * atom)
{
    r300ContextPtr r300 = R300_CONTEXT(ctx);
    drm_r300_cmd_header_t cmd;
    uint32_t addr, count;
    int type, clamp;

    cmd.u = atom->cmd[0];
    addr = ((cmd.r500fp.adrhi_flags & 1) << 8) | cmd.r500fp.adrlo;
    type = !!(cmd.r500fp.adrhi_flags & R500FP_CONSTANT_TYPE);
    clamp = !!(cmd.r500fp.adrhi_flags & R500FP_CONSTANT_CLAMP);

    if (type) {
        count = r500fp_count(atom->cmd) * 4;
    } else {
        count = r500fp_count(atom->cmd) * 6;
    }

    r500_emit_fp(r300, &atom->cmd[1], count, addr, type, clamp);
}

static int check_tex_offsets(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int numtmus = packet0_count(r300, r300->hw.tex.offset.cmd);
	int dw = 0, i;
	if (atom->cmd[0] == CP_PACKET2) {
		return dw;
	}
	for(i = 0; i < numtmus; ++i) {
		radeonTexObj *t = r300->hw.textures[i];
		if (!t && !r300->radeon.radeonScreen->kernel_mm) {
			dw += 0;
		} else if (t && t->image_override && !t->bo) {
			if (!r300->radeon.radeonScreen->kernel_mm)
				dw += 2;
		} else
			dw += 4;
	}
	return dw;
}

static void emit_tex_offsets(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	BATCH_LOCALS(&r300->radeon);
	int numtmus = packet0_count(r300, r300->hw.tex.offset.cmd);
	int i;

	for(i = 0; i < numtmus; ++i) {
		radeonTexObj *t = r300->hw.textures[i];
		if (t && !t->image_override) {
			BEGIN_BATCH_NO_AUTOSTATE(4);
			OUT_BATCH_REGSEQ(R300_TX_OFFSET_0 + (i * 4), 1);
			OUT_BATCH_RELOC(t->tile_bits, t->mt->bo, get_base_teximage_offset(t),
					RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
			END_BATCH();
		} else if (!t) {
			/* Texture unit hasn't a texture bound.
			 * We assign the current color buffer as a fakery to make
			 * KIL work on KMS (without it, the CS checker will complain).
			 */
			if (r300->radeon.radeonScreen->kernel_mm) {
				struct radeon_renderbuffer *rrb = radeon_get_colorbuffer(&r300->radeon);
				if (rrb && rrb->bo) {
					BEGIN_BATCH_NO_AUTOSTATE(4);
					OUT_BATCH_REGSEQ(R300_TX_OFFSET_0 + (i * 4), 1);
					OUT_BATCH_RELOC(0, rrb->bo, 0,
							RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
					END_BATCH();
				}
			}
		} else { /* override cases */
			if (t->bo) {
				BEGIN_BATCH_NO_AUTOSTATE(4);
				OUT_BATCH_REGSEQ(R300_TX_OFFSET_0 + (i * 4), 1);
				OUT_BATCH_RELOC(t->tile_bits, t->bo, 0,
						RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
				END_BATCH();
			} else if (!r300->radeon.radeonScreen->kernel_mm) {
				BEGIN_BATCH_NO_AUTOSTATE(2);
				OUT_BATCH_REGSEQ(R300_TX_OFFSET_0 + (i * 4), 1);
				OUT_BATCH(t->override_offset);
				END_BATCH();
			} else {
				/* Texture unit hasn't a texture bound nothings to do */
			}
		}
	}
}

void r300_emit_scissor(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	BATCH_LOCALS(&r300->radeon);
    unsigned x1, y1, x2, y2;
	struct radeon_renderbuffer *rrb;

    if (!r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        return;
    }
	rrb = radeon_get_colorbuffer(&r300->radeon);
	if (!rrb || !rrb->bo) {
		fprintf(stderr, "no rrb\n");
		return;
	}
    if (r300->radeon.state.scissor.enabled) {
        x1 = r300->radeon.state.scissor.rect.x1;
        y1 = r300->radeon.state.scissor.rect.y1;
        x2 = r300->radeon.state.scissor.rect.x2;
        y2 = r300->radeon.state.scissor.rect.y2;
    } else {
        x1 = 0;
        y1 = 0;
        x2 = rrb->base.Width - 1;
        y2 = rrb->base.Height - 1;
    }
    if (r300->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV515) {
        x1 += R300_SCISSORS_OFFSET;
        y1 += R300_SCISSORS_OFFSET;
        x2 += R300_SCISSORS_OFFSET;
        y2 += R300_SCISSORS_OFFSET;
    }
    BEGIN_BATCH_NO_AUTOSTATE(3);
    OUT_BATCH_REGSEQ(R300_SC_SCISSORS_TL, 2);
    OUT_BATCH((x1 << R300_SCISSORS_X_SHIFT)|(y1 << R300_SCISSORS_Y_SHIFT));
    OUT_BATCH((x2 << R300_SCISSORS_X_SHIFT)|(y2 << R300_SCISSORS_Y_SHIFT));
    END_BATCH();
}
static int check_cb_offset(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t dw = 6 + 3 + 16;
	if (r300->radeon.radeonScreen->kernel_mm)
		dw += 2;
	if (!r300->radeon.radeonScreen->driScreen->dri2.enabled) {
		dw -= 3 + 16;
	}
	return dw;
}

static void emit_scissor(struct r300_context *r300,
                         unsigned width,
                         unsigned height)
{
    int i;
    BATCH_LOCALS(&r300->radeon);
    if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
        BEGIN_BATCH_NO_AUTOSTATE(3);
        OUT_BATCH_REGSEQ(R300_SC_SCISSORS_TL, 2);
        OUT_BATCH(0);
        OUT_BATCH(((width - 1) << R300_SCISSORS_X_SHIFT) |
                ((height - 1) << R300_SCISSORS_Y_SHIFT));
        END_BATCH();
        BEGIN_BATCH_NO_AUTOSTATE(16);
        for (i = 0; i < 4; i++) {
            OUT_BATCH_REGSEQ(R300_SC_CLIPRECT_TL_0 + (i * 8), 2);
            OUT_BATCH((0 << R300_CLIPRECT_X_SHIFT) | (0 << R300_CLIPRECT_Y_SHIFT));
            OUT_BATCH(((width - 1) << R300_CLIPRECT_X_SHIFT) | ((height - 1) << R300_CLIPRECT_Y_SHIFT));
        }
        OUT_BATCH_REGSEQ(R300_SC_CLIP_RULE, 1);
        OUT_BATCH(0xAAAA);
        OUT_BATCH_REGSEQ(R300_SC_SCREENDOOR, 1);
        OUT_BATCH(0xffffff);
        END_BATCH();
    } else {
        BEGIN_BATCH_NO_AUTOSTATE(3);
        OUT_BATCH_REGSEQ(R300_SC_SCISSORS_TL, 2);
        OUT_BATCH((R300_SCISSORS_OFFSET << R300_SCISSORS_X_SHIFT) |
                (R300_SCISSORS_OFFSET << R300_SCISSORS_Y_SHIFT));
        OUT_BATCH(((width + R300_SCISSORS_OFFSET - 1) << R300_SCISSORS_X_SHIFT) |
                ((height + R300_SCISSORS_OFFSET - 1) << R300_SCISSORS_Y_SHIFT));
        END_BATCH();
        BEGIN_BATCH_NO_AUTOSTATE(16);
        for (i = 0; i < 4; i++) {
            OUT_BATCH_REGSEQ(R300_SC_CLIPRECT_TL_0 + (i * 8), 2);
            OUT_BATCH((R300_SCISSORS_OFFSET << R300_CLIPRECT_X_SHIFT) | (R300_SCISSORS_OFFSET << R300_CLIPRECT_Y_SHIFT));
            OUT_BATCH(((R300_SCISSORS_OFFSET + width - 1) << R300_CLIPRECT_X_SHIFT) |
                        ((R300_SCISSORS_OFFSET + height - 1) << R300_CLIPRECT_Y_SHIFT));
        }
        OUT_BATCH_REGSEQ(R300_SC_CLIP_RULE, 1);
        OUT_BATCH(0xAAAA);
        OUT_BATCH_REGSEQ(R300_SC_SCREENDOOR, 1);
        OUT_BATCH(0xffffff);
        END_BATCH();
    }
}

void r300_emit_cb_setup(struct r300_context *r300,
                        struct radeon_bo *bo,
                        uint32_t offset,
                        GLuint format,
                        unsigned cpp,
                        unsigned pitch)
{
    BATCH_LOCALS(&r300->radeon);
    uint32_t cbpitch = pitch / cpp;
    uint32_t dw = 6;

    assert(offset % 32 == 0);

    switch (format) {
        case MESA_FORMAT_RGB565:
            assert(_mesa_little_endian());
            cbpitch |= R300_COLOR_FORMAT_RGB565;
            break;
        case MESA_FORMAT_RGB565_REV:
            assert(!_mesa_little_endian());
            cbpitch |= R300_COLOR_FORMAT_RGB565;
            break;
        case MESA_FORMAT_ARGB4444:
            assert(_mesa_little_endian());
            cbpitch |= R300_COLOR_FORMAT_ARGB4444;
            break;
        case MESA_FORMAT_ARGB4444_REV:
            assert(!_mesa_little_endian());
            cbpitch |= R300_COLOR_FORMAT_ARGB4444;
            break;
        case MESA_FORMAT_ARGB1555:
            assert(_mesa_little_endian());
            cbpitch |= R300_COLOR_FORMAT_ARGB1555;
            break;
        case MESA_FORMAT_ARGB1555_REV:
            assert(!_mesa_little_endian());
            cbpitch |= R300_COLOR_FORMAT_ARGB1555;
            break;
        default:
            if (cpp == 4) {
                cbpitch |= R300_COLOR_FORMAT_ARGB8888;
            } else {
                _mesa_problem(r300->radeon.glCtx, "unexpected format in emit_cb_offset()");;
            }
            break;
    }

    if (bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
        cbpitch |= R300_COLOR_TILE_ENABLE;

    if (r300->radeon.radeonScreen->kernel_mm)
        dw += 2;

    BEGIN_BATCH_NO_AUTOSTATE(dw);
    OUT_BATCH_REGSEQ(R300_RB3D_COLOROFFSET0, 1);
    OUT_BATCH_RELOC(offset, bo, offset, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    OUT_BATCH_REGSEQ(R300_RB3D_COLORPITCH0, 1);
    if (!r300->radeon.radeonScreen->kernel_mm)
        OUT_BATCH(cbpitch);
    else
        OUT_BATCH_RELOC(cbpitch, bo, cbpitch, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    END_BATCH();
}

static void emit_cb_offset_atom(GLcontext *ctx, struct radeon_state_atom * atom)
{
    r300ContextPtr r300 = R300_CONTEXT(ctx);
    struct radeon_renderbuffer *rrb;
    uint32_t offset = r300->radeon.state.color.draw_offset;

    rrb = radeon_get_colorbuffer(&r300->radeon);
    if (!rrb || !rrb->bo) {
        fprintf(stderr, "no rrb\n");
        return;
    }

    if (RADEON_DEBUG & RADEON_STATE)
        fprintf(stderr,"rrb is %p %d %dx%d\n", rrb, offset, rrb->base.Width, rrb->base.Height);

    r300_emit_cb_setup(r300, rrb->bo, offset, rrb->base.Format, rrb->cpp, rrb->pitch);

    if (r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        emit_scissor(r300, rrb->base.Width, rrb->base.Height);
    }
}

static int check_zb_offset(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t dw;
	dw = 6;
	if (r300->radeon.radeonScreen->kernel_mm)
		dw += 2;
	return dw;
}

static void emit_zb_offset(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	BATCH_LOCALS(&r300->radeon);
	struct radeon_renderbuffer *rrb;
	uint32_t zbpitch;
	uint32_t dw = atom->check(ctx, atom);

	rrb = radeon_get_depthbuffer(&r300->radeon);
	if (!rrb)
		return;

	zbpitch = (rrb->pitch / rrb->cpp);
	if (!r300->radeon.radeonScreen->kernel_mm) {
	    if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
	        zbpitch |= R300_DEPTHMACROTILE_ENABLE;
	   }
	    if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE){
	        zbpitch |= R300_DEPTHMICROTILE_TILED;
	    }
	}

	BEGIN_BATCH_NO_AUTOSTATE(dw);
	OUT_BATCH_REGSEQ(R300_ZB_DEPTHOFFSET, 1);
	OUT_BATCH_RELOC(0, rrb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	OUT_BATCH_REGSEQ(R300_ZB_DEPTHPITCH, 1);
    	if (!r300->radeon.radeonScreen->kernel_mm)
	    OUT_BATCH(zbpitch);
	else
	    OUT_BATCH_RELOC(cbpitch, rrb->bo, zbpitch, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	END_BATCH();
}

static void emit_zstencil_format(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	BATCH_LOCALS(&r300->radeon);
	struct radeon_renderbuffer *rrb;
	uint32_t format = 0;

	rrb = radeon_get_depthbuffer(&r300->radeon);
	if (!rrb)
	  format = 0;
	else {
	  if (rrb->cpp == 2)
	    format = R300_DEPTHFORMAT_16BIT_INT_Z;
	  else if (rrb->cpp == 4)
	    format = R300_DEPTHFORMAT_24BIT_INT_Z_8BIT_STENCIL;
	}

	BEGIN_BATCH_NO_AUTOSTATE(atom->cmd_size);
	OUT_BATCH(atom->cmd[0]);
	atom->cmd[1] &= ~0xf;
	atom->cmd[1] |= format;
	OUT_BATCH(atom->cmd[1]);
	OUT_BATCH(atom->cmd[2]);
	OUT_BATCH(atom->cmd[3]);
	OUT_BATCH(atom->cmd[4]);
	END_BATCH();
}

static int check_never(GLcontext *ctx, struct radeon_state_atom *atom)
{
   return 0;
}

static int check_always(GLcontext *ctx, struct radeon_state_atom *atom)
{
	return atom->cmd_size;
}

static int check_variable(GLcontext *ctx, struct radeon_state_atom *atom)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int cnt;
	if (atom->cmd[0] == CP_PACKET2) {
		return 0;
	}
	cnt = packet0_count(r300, atom->cmd);
	return cnt ? cnt + 1 : 0;
}

static int check_r500fp(GLcontext *ctx, struct radeon_state_atom *atom)
{
	int cnt;
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int extra = 1;
	cnt = r500fp_count(atom->cmd);
	if (r300->radeon.radeonScreen->kernel_mm)
		extra = 3;

	return cnt ? (cnt * 6) + extra : 0;
}

static int check_r500fp_const(GLcontext *ctx, struct radeon_state_atom *atom)
{
	int cnt;
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int extra = 1;
	cnt = r500fp_count(atom->cmd);
	if (r300->radeon.radeonScreen->kernel_mm)
		extra = 3;

	cnt = r500fp_count(atom->cmd);
	return cnt ? (cnt * 4) + extra : 0;
}

#define ALLOC_STATE( ATOM, CHK, SZ, IDX )				\
   do {									\
      r300->hw.ATOM.cmd_size = (SZ);					\
      r300->hw.ATOM.cmd = (uint32_t*)CALLOC((SZ) * sizeof(uint32_t));	\
      r300->hw.ATOM.name = #ATOM;					\
      r300->hw.ATOM.idx = (IDX);					\
      r300->hw.ATOM.check = check_##CHK;				\
      r300->hw.ATOM.dirty = GL_FALSE;					\
      r300->radeon.hw.max_state_size += (SZ);					\
      insert_at_tail(&r300->radeon.hw.atomlist, &r300->hw.ATOM);		\
   } while (0)
/**
 * Allocate memory for the command buffer and initialize the state atom
 * list. Note that the initial hardware state is set by r300InitState().
 */
void r300InitCmdBuf(r300ContextPtr r300)
{
	int mtu;
	int has_tcl;
	int is_r500 = 0;

	has_tcl = r300->options.hw_tcl_enabled;

	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515)
		is_r500 = 1;

	r300->radeon.hw.max_state_size = 2 + 2;	/* reserve extra space for WAIT_IDLE and tex cache flush */

	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & RADEON_TEXTURE) {
		fprintf(stderr, "Using %d maximum texture units..\n", mtu);
	}

	/* Setup the atom linked list */
	make_empty_list(&r300->radeon.hw.atomlist);
	r300->radeon.hw.atomlist.name = "atom-list";

	/* Initialize state atoms */
	ALLOC_STATE(vpt, always, R300_VPT_CMDSIZE, 0);
	r300->hw.vpt.cmd[R300_VPT_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_SE_VPORT_XSCALE, 6);
	ALLOC_STATE(vap_cntl, always, R300_VAP_CNTL_SIZE, 0);
	r300->hw.vap_cntl.cmd[R300_VAP_CNTL_FLUSH] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_PVS_STATE_FLUSH_REG, 1);
	r300->hw.vap_cntl.cmd[R300_VAP_CNTL_FLUSH_1] = 0;
	r300->hw.vap_cntl.cmd[R300_VAP_CNTL_CMD] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_CNTL, 1);
	if (is_r500 && !r300->radeon.radeonScreen->kernel_mm) {
	    ALLOC_STATE(vap_index_offset, always, 2, 0);
	    r300->hw.vap_index_offset.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R500_VAP_INDEX_OFFSET, 1);
	    r300->hw.vap_index_offset.cmd[1] = 0;
	}
	ALLOC_STATE(vte, always, 3, 0);
	r300->hw.vte.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_SE_VTE_CNTL, 2);
	ALLOC_STATE(vap_vf_max_vtx_indx, always, 3, 0);
	r300->hw.vap_vf_max_vtx_indx.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_VF_MAX_VTX_INDX, 2);
	ALLOC_STATE(vap_cntl_status, always, 2, 0);
	r300->hw.vap_cntl_status.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_CNTL_STATUS, 1);
	ALLOC_STATE(vir[0], variable, R300_VIR_CMDSIZE, 0);
	r300->hw.vir[0].cmd[R300_VIR_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_VAP_PROG_STREAM_CNTL_0, 1);
	ALLOC_STATE(vir[1], variable, R300_VIR_CMDSIZE, 1);
	r300->hw.vir[1].cmd[R300_VIR_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_VAP_PROG_STREAM_CNTL_EXT_0, 1);
	ALLOC_STATE(vic, always, R300_VIC_CMDSIZE, 0);
	r300->hw.vic.cmd[R300_VIC_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_VTX_STATE_CNTL, 2);
	ALLOC_STATE(vap_psc_sgn_norm_cntl, always, 2, 0);
	r300->hw.vap_psc_sgn_norm_cntl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_PSC_SGN_NORM_CNTL, SGN_NORM_ZERO_CLAMP_MINUS_ONE);

	if (has_tcl) {
		ALLOC_STATE(vap_clip_cntl, always, 2, 0);
		r300->hw.vap_clip_cntl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_CLIP_CNTL, 1);
		ALLOC_STATE(vap_clip, always, 5, 0);
		r300->hw.vap_clip.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_GB_VERT_CLIP_ADJ, 4);
		ALLOC_STATE(vap_pvs_vtx_timeout_reg, always, 2, 0);
		r300->hw.vap_pvs_vtx_timeout_reg.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, VAP_PVS_VTX_TIMEOUT_REG, 1);
	}

	ALLOC_STATE(vof, always, R300_VOF_CMDSIZE, 0);
	r300->hw.vof.cmd[R300_VOF_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_VAP_OUTPUT_VTX_FMT_0, 2);

	if (has_tcl) {
		ALLOC_STATE(pvs, always, R300_PVS_CMDSIZE, 0);
		r300->hw.pvs.cmd[R300_PVS_CMD_0] =
		    cmdpacket0(r300->radeon.radeonScreen, R300_VAP_PVS_CODE_CNTL_0, 3);
	}

	ALLOC_STATE(gb_enable, always, 2, 0);
	r300->hw.gb_enable.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GB_ENABLE, 1);
	if (!r300->radeon.radeonScreen->driScreen->dri2.enabled) {
		ALLOC_STATE(gb_misc, always, R300_GB_MISC_CMDSIZE, 0);
	} else {
		ALLOC_STATE(gb_misc, never, R300_GB_MISC_CMDSIZE, 0);
	}
	r300->hw.gb_misc.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GB_MSPOS0, 3);
	ALLOC_STATE(gb_misc2, always, R300_GB_MISC2_CMDSIZE, 0);
	r300->hw.gb_misc2.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, 0x401C, 2);
	ALLOC_STATE(txe, always, R300_TXE_CMDSIZE, 0);
	r300->hw.txe.cmd[R300_TXE_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_TX_ENABLE, 1);
	ALLOC_STATE(ga_point_s0, always, 5, 0);
	r300->hw.ga_point_s0.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_POINT_S0, 4);
	ALLOC_STATE(ga_triangle_stipple, always, 2, 0);
	r300->hw.ga_triangle_stipple.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_TRIANGLE_STIPPLE, 1);
	ALLOC_STATE(ps, always, R300_PS_CMDSIZE, 0);
	r300->hw.ps.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_POINT_SIZE, 1);
	ALLOC_STATE(ga_point_minmax, always, 4, 0);
	r300->hw.ga_point_minmax.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_POINT_MINMAX, 3);
	ALLOC_STATE(lcntl, always, 2, 0);
	r300->hw.lcntl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_LINE_CNTL, 1);
	ALLOC_STATE(ga_line_stipple, always, 4, 0);
	r300->hw.ga_line_stipple.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_LINE_STIPPLE_VALUE, 3);
        if (!r300->radeon.radeonScreen->driScreen->dri2.enabled) {
		ALLOC_STATE(shade, always, 2, 0);
        } else {
		ALLOC_STATE(shade, never, 2, 0);
        }
	r300->hw.shade.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_ENHANCE, 1);
	ALLOC_STATE(shade2, always, 4, 0);
	r300->hw.shade2.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, 0x4278, 3);
	ALLOC_STATE(polygon_mode, always, 4, 0);
	r300->hw.polygon_mode.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_POLY_MODE, 3);
	ALLOC_STATE(fogp, always, 3, 0);
	r300->hw.fogp.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_FOG_SCALE, 2);
	ALLOC_STATE(zbias_cntl, always, 2, 0);
	r300->hw.zbias_cntl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_SU_TEX_WRAP, 1);
	ALLOC_STATE(zbs, always, R300_ZBS_CMDSIZE, 0);
	r300->hw.zbs.cmd[R300_ZBS_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_SU_POLY_OFFSET_FRONT_SCALE, 4);
	ALLOC_STATE(occlusion_cntl, always, 2, 0);
	r300->hw.occlusion_cntl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_SU_POLY_OFFSET_ENABLE, 1);
	ALLOC_STATE(cul, always, R300_CUL_CMDSIZE, 0);
	r300->hw.cul.cmd[R300_CUL_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_SU_CULL_MODE, 1);
	ALLOC_STATE(su_depth_scale, always, 3, 0);
	r300->hw.su_depth_scale.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_SU_DEPTH_SCALE, 2);
	ALLOC_STATE(rc, always, R300_RC_CMDSIZE, 0);
	r300->hw.rc.cmd[R300_RC_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_RS_COUNT, 2);
	if (is_r500) {
		ALLOC_STATE(ri, variable, R500_RI_CMDSIZE, 0);
		r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_RS_IP_0, 16);
		ALLOC_STATE(rr, variable, R300_RR_CMDSIZE, 0);
		r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_RS_INST_0, 1);
	} else {
		ALLOC_STATE(ri, variable, R300_RI_CMDSIZE, 0);
		r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_RS_IP_0, 8);
		ALLOC_STATE(rr, variable, R300_RR_CMDSIZE, 0);
		r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_RS_INST_0, 1);
	}
	ALLOC_STATE(sc_hyperz, always, 3, 0);
	r300->hw.sc_hyperz.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_SC_HYPERZ, 2);
	ALLOC_STATE(sc_screendoor, always, 2, 0);
	r300->hw.sc_screendoor.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_SC_SCREENDOOR, 1);
	ALLOC_STATE(us_out_fmt, always, 6, 0);
	r300->hw.us_out_fmt.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_OUT_FMT, 5);

	if (is_r500) {
		ALLOC_STATE(fp, always, R500_FP_CMDSIZE, 0);
		r300->hw.fp.cmd[R500_FP_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_US_CONFIG, 2);
		r300->hw.fp.cmd[R500_FP_CNTL] = R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO;
		r300->hw.fp.cmd[R500_FP_CMD_1] = cmdpacket0(r300->radeon.radeonScreen, R500_US_CODE_ADDR, 3);
		r300->hw.fp.cmd[R500_FP_CMD_2] = cmdpacket0(r300->radeon.radeonScreen, R500_US_FC_CTRL, 1);
		r300->hw.fp.cmd[R500_FP_FC_CNTL] = 0; /* FIXME when we add flow control */

		ALLOC_STATE(r500fp, r500fp, R500_FPI_CMDSIZE, 0);
		r300->hw.r500fp.cmd[R300_FPI_CMD_0] =
			cmdr500fp(r300->radeon.radeonScreen, 0, 0, 0, 0);
		if (r300->radeon.radeonScreen->kernel_mm)
			r300->hw.r500fp.emit = emit_r500fp_atom;

		ALLOC_STATE(r500fp_const, r500fp_const, R500_FPP_CMDSIZE, 0);
		r300->hw.r500fp_const.cmd[R300_FPI_CMD_0] =
			cmdr500fp(r300->radeon.radeonScreen, 0, 0, 1, 0);
		if (r300->radeon.radeonScreen->kernel_mm)
			r300->hw.r500fp_const.emit = emit_r500fp_atom;
	} else {
		ALLOC_STATE(fp, always, R300_FP_CMDSIZE, 0);
		r300->hw.fp.cmd[R300_FP_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_CONFIG, 3);
		r300->hw.fp.cmd[R300_FP_CMD_1] = cmdpacket0(r300->radeon.radeonScreen, R300_US_CODE_ADDR_0, 4);

		ALLOC_STATE(fpt, variable, R300_FPT_CMDSIZE, 0);
		r300->hw.fpt.cmd[R300_FPT_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_TEX_INST_0, 0);

		ALLOC_STATE(fpi[0], variable, R300_FPI_CMDSIZE, 0);
		r300->hw.fpi[0].cmd[R300_FPI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_ALU_RGB_INST_0, 1);
		ALLOC_STATE(fpi[1], variable, R300_FPI_CMDSIZE, 1);
		r300->hw.fpi[1].cmd[R300_FPI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_ALU_RGB_ADDR_0, 1);
		ALLOC_STATE(fpi[2], variable, R300_FPI_CMDSIZE, 2);
		r300->hw.fpi[2].cmd[R300_FPI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_ALU_ALPHA_INST_0, 1);
		ALLOC_STATE(fpi[3], variable, R300_FPI_CMDSIZE, 3);
		r300->hw.fpi[3].cmd[R300_FPI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_US_ALU_ALPHA_ADDR_0, 1);
		ALLOC_STATE(fpp, variable, R300_FPP_CMDSIZE, 0);
		r300->hw.fpp.cmd[R300_FPP_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_PFS_PARAM_0_X, 0);
	}
	ALLOC_STATE(fogs, always, R300_FOGS_CMDSIZE, 0);
	r300->hw.fogs.cmd[R300_FOGS_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_FG_FOG_BLEND, 1);
	ALLOC_STATE(fogc, always, R300_FOGC_CMDSIZE, 0);
	r300->hw.fogc.cmd[R300_FOGC_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_FG_FOG_COLOR_R, 3);
	ALLOC_STATE(at, always, R300_AT_CMDSIZE, 0);
	r300->hw.at.cmd[R300_AT_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_FG_ALPHA_FUNC, 2);
	ALLOC_STATE(fg_depth_src, always, 2, 0);
	r300->hw.fg_depth_src.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_FG_DEPTH_SRC, 1);
	ALLOC_STATE(rb3d_cctl, always, 2, 0);
	r300->hw.rb3d_cctl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_CCTL, 1);
	ALLOC_STATE(bld, always, R300_BLD_CMDSIZE, 0);
	r300->hw.bld.cmd[R300_BLD_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_CBLEND, 2);
	ALLOC_STATE(cmk, always, R300_CMK_CMDSIZE, 0);
	r300->hw.cmk.cmd[R300_CMK_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, RB3D_COLOR_CHANNEL_MASK, 1);
	if (is_r500) {
		ALLOC_STATE(blend_color, always, 3, 0);
		r300->hw.blend_color.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R500_RB3D_CONSTANT_COLOR_AR, 2);
	} else {
		ALLOC_STATE(blend_color, always, 2, 0);
		r300->hw.blend_color.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_BLEND_COLOR, 1);
	}
	ALLOC_STATE(rop, always, 2, 0);
	r300->hw.rop.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_ROPCNTL, 1);
	ALLOC_STATE(cb, cb_offset, R300_CB_CMDSIZE, 0);
	r300->hw.cb.emit = &emit_cb_offset_atom;
	ALLOC_STATE(rb3d_dither_ctl, always, 10, 0);
	r300->hw.rb3d_dither_ctl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_DITHER_CTL, 9);
	ALLOC_STATE(rb3d_aaresolve_ctl, always, 2, 0);
	r300->hw.rb3d_aaresolve_ctl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_AARESOLVE_CTL, 1);
	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV350) {
		ALLOC_STATE(rb3d_discard_src_pixel_lte_threshold, always, 3, 0);
	} else {
		ALLOC_STATE(rb3d_discard_src_pixel_lte_threshold, never, 3, 0);
	}
	r300->hw.rb3d_discard_src_pixel_lte_threshold.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R500_RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 2);
	ALLOC_STATE(zs, always, R300_ZS_CMDSIZE, 0);
	r300->hw.zs.cmd[R300_ZS_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_ZB_CNTL, 3);
	if (is_r500) {
		if (r300->radeon.radeonScreen->kernel_mm)
			ALLOC_STATE(zsb, always, R300_ZSB_CMDSIZE, 0);
		else
			ALLOC_STATE(zsb, never, R300_ZSB_CMDSIZE, 0);
		r300->hw.zsb.cmd[R300_ZSB_CMD_0] =
			cmdpacket0(r300->radeon.radeonScreen, R500_ZB_STENCILREFMASK_BF, 1);
	}

	ALLOC_STATE(zstencil_format, always, 5, 0);
	r300->hw.zstencil_format.cmd[0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_ZB_FORMAT, 4);
	r300->hw.zstencil_format.emit = emit_zstencil_format;

	ALLOC_STATE(zb, zb_offset, R300_ZB_CMDSIZE, 0);
	r300->hw.zb.emit = emit_zb_offset;
	ALLOC_STATE(zb_depthclearvalue, always, 2, 0);
	r300->hw.zb_depthclearvalue.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_ZB_DEPTHCLEARVALUE, 1);
	ALLOC_STATE(zb_zmask, always, 3, 0);
	r300->hw.zb_zmask.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_ZB_ZMASK_OFFSET, 2);
	ALLOC_STATE(zb_hiz_offset, always, 2, 0);
	r300->hw.zb_hiz_offset.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_ZB_HIZ_OFFSET, 1);
	ALLOC_STATE(zb_hiz_pitch, always, 2, 0);
	r300->hw.zb_hiz_pitch.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_ZB_HIZ_PITCH, 1);

	/* VPU only on TCL */
	if (has_tcl) {
		int i;
		ALLOC_STATE(vpi, vpu, R300_VPI_CMDSIZE, 0);
		r300->hw.vpi.cmd[0] =
			cmdvpu(r300->radeon.radeonScreen, R300_PVS_CODE_START, 0);
		if (r300->radeon.radeonScreen->kernel_mm)
			r300->hw.vpi.emit = emit_vpu_state;

		if (is_r500) {
			ALLOC_STATE(vpp, vpp, R300_VPP_CMDSIZE, 0);
			r300->hw.vpp.cmd[0] =
				cmdvpu(r300->radeon.radeonScreen, R500_PVS_CONST_START, 0);
			if (r300->radeon.radeonScreen->kernel_mm)
				r300->hw.vpp.emit = emit_vpp_state;

			ALLOC_STATE(vps, vpu, R300_VPS_CMDSIZE, 0);
			r300->hw.vps.cmd[0] =
				cmdvpu(r300->radeon.radeonScreen, R500_POINT_VPORT_SCALE_OFFSET, 1);
			if (r300->radeon.radeonScreen->kernel_mm)
				r300->hw.vps.emit = emit_vpu_state;

			for (i = 0; i < 6; i++) {
				ALLOC_STATE(vpucp[i], vpu, R300_VPUCP_CMDSIZE, 0);
				r300->hw.vpucp[i].cmd[0] =
					cmdvpu(r300->radeon.radeonScreen,
							R500_PVS_UCP_START + i, 1);
				if (r300->radeon.radeonScreen->kernel_mm)
					r300->hw.vpucp[i].emit = emit_vpu_state;
			}
		} else {
			ALLOC_STATE(vpp, vpp, R300_VPP_CMDSIZE, 0);
			r300->hw.vpp.cmd[0] =
				cmdvpu(r300->radeon.radeonScreen, R300_PVS_CONST_START, 0);
			if (r300->radeon.radeonScreen->kernel_mm)
				r300->hw.vpp.emit = emit_vpp_state;

			ALLOC_STATE(vps, vpu, R300_VPS_CMDSIZE, 0);
			r300->hw.vps.cmd[0] =
				cmdvpu(r300->radeon.radeonScreen, R300_POINT_VPORT_SCALE_OFFSET, 1);
			if (r300->radeon.radeonScreen->kernel_mm)
				r300->hw.vps.emit = emit_vpu_state;

			for (i = 0; i < 6; i++) {
				ALLOC_STATE(vpucp[i], vpu, R300_VPUCP_CMDSIZE, 0);
				r300->hw.vpucp[i].cmd[0] =
					cmdvpu(r300->radeon.radeonScreen,
							R300_PVS_UCP_START + i, 1);
				if (r300->radeon.radeonScreen->kernel_mm)
					r300->hw.vpucp[i].emit = emit_vpu_state;
			}
		}
	}

	/* Textures */
	ALLOC_STATE(tex.filter, variable, mtu + 1, 0);
	r300->hw.tex.filter.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FILTER0_0, 0);

	ALLOC_STATE(tex.filter_1, variable, mtu + 1, 0);
	r300->hw.tex.filter_1.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FILTER1_0, 0);

	ALLOC_STATE(tex.size, variable, mtu + 1, 0);
	r300->hw.tex.size.cmd[R300_TEX_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_TX_SIZE_0, 0);

	ALLOC_STATE(tex.format, variable, mtu + 1, 0);
	r300->hw.tex.format.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_FORMAT_0, 0);

	ALLOC_STATE(tex.pitch, variable, mtu + 1, 0);
	r300->hw.tex.pitch.cmd[R300_TEX_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_TX_FORMAT2_0, 0);

	ALLOC_STATE(tex.offset, tex_offsets, 1, 0);
	r300->hw.tex.offset.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_OFFSET_0, 0);
	r300->hw.tex.offset.emit = &emit_tex_offsets;

	ALLOC_STATE(tex.chroma_key, variable, mtu + 1, 0);
	r300->hw.tex.chroma_key.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_CHROMA_KEY_0, 0);

	ALLOC_STATE(tex.border_color, variable, mtu + 1, 0);
	r300->hw.tex.border_color.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_BORDER_COLOR_0, 0);

	radeon_init_query_stateobj(&r300->radeon, R300_QUERYOBJ_CMDSIZE);
	if (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV530) {
		r300->radeon.query.queryobj.cmd[R300_QUERYOBJ_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, RV530_FG_ZBREG_DEST, 1);
		r300->radeon.query.queryobj.cmd[R300_QUERYOBJ_DATA_0] = RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL;
	} else {
		r300->radeon.query.queryobj.cmd[R300_QUERYOBJ_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_SU_REG_DEST, 1);
		r300->radeon.query.queryobj.cmd[R300_QUERYOBJ_DATA_0] = R300_RASTER_PIPE_SELECT_ALL;
	}
	r300->radeon.query.queryobj.cmd[R300_QUERYOBJ_CMD_1] = cmdpacket0(r300->radeon.radeonScreen, R300_ZB_ZPASS_DATA, 1);
	r300->radeon.query.queryobj.cmd[R300_QUERYOBJ_DATA_1] = 0;

	r300->radeon.hw.is_dirty = GL_TRUE;
	r300->radeon.hw.all_dirty = GL_TRUE;

	rcommonInitCmdBuf(&r300->radeon);
}
