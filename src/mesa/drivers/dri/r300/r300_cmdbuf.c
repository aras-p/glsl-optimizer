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
#include "swrast/swrast.h"

#include "drm.h"
#include "radeon_drm.h"

#include "radeon_buffer.h"
#include "radeon_ioctl.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "radeon_reg.h"
#include "r300_reg.h"
#include "r300_cmdbuf.h"
#include "r300_emit.h"
#include "r300_mipmap_tree.h"
#include "r300_state.h"
#include "radeon_cs_legacy.h"
#include "radeon_cs_gem.h"
#include "radeon_reg.h"

#define R300_VAP_PVS_UPLOAD_ADDRESS 0x2200
#   define RADEON_ONE_REG_WR        (1 << 15)

// Set this to 1 for extremely verbose debugging of command buffers
#define DEBUG_CMDBUF		0

/** # of dwords reserved for additional instructions that may need to be written
 * during flushing.
 */
#define SPACE_FOR_FLUSHING	4

/**
 * Send the current command buffer via ioctl to the hardware.
 */
int r300FlushCmdBufLocked(r300ContextPtr r300, const char *caller)
{
	int ret = 0;

	if (r300->cmdbuf.flushing) {
		fprintf(stderr, "Recursive call into r300FlushCmdBufLocked!\n");
		exit(-1);
	}
	r300->cmdbuf.flushing = 1;
    if (r300->cmdbuf.cs->cdw) {
        ret = radeon_cs_emit(r300->cmdbuf.cs);
        r300->hw.all_dirty = 1;
    }
    radeon_cs_erase(r300->cmdbuf.cs);
	r300->cmdbuf.flushing = 0;
	return ret;
}

int r300FlushCmdBuf(r300ContextPtr r300, const char *caller)
{
	int ret;

	LOCK_HARDWARE(&r300->radeon);
	ret = r300FlushCmdBufLocked(r300, caller);
	UNLOCK_HARDWARE(&r300->radeon);

	if (ret) {
		fprintf(stderr, "drmRadeonCmdBuffer: %d\n", ret);
		_mesa_exit(ret);
	}

	return ret;
}

/**
 * Make sure that enough space is available in the command buffer
 * by flushing if necessary.
 *
 * \param dwords The number of dwords we need to be free on the command buffer
 */
void r300EnsureCmdBufSpace(r300ContextPtr r300, int dwords, const char *caller)
{
	if ((r300->cmdbuf.cs->cdw + dwords + 128) > r300->cmdbuf.size ||
        radeon_cs_need_flush(r300->cmdbuf.cs)) {
		r300FlushCmdBuf(r300, caller);
    }
}

void r300BeginBatch(r300ContextPtr r300, int n,
		    int dostate,
                    const char *file,
                    const char *function,
                    int line)
{
	r300EnsureCmdBufSpace(r300, n, function);
	if (!r300->cmdbuf.cs->cdw && dostate) {
		if (RADEON_DEBUG & DEBUG_IOCTL)
			fprintf(stderr, "Reemit state after flush (from %s)\n", function);
		r300EmitState(r300);
	}
    radeon_cs_begin(r300->cmdbuf.cs, n, file, function, line);
}

static void r300PrintStateAtom(r300ContextPtr r300,
                               struct r300_state_atom *state)
{
	int i;
	int dwords = (*state->check) (r300, state);

	fprintf(stderr, "  emit %s %d/%d\n", state->name, dwords, state->cmd_size);

	if (RADEON_DEBUG & DEBUG_VERBOSE) {
		for (i = 0; i < dwords; i++) {
			fprintf(stderr, "      %s[%d]: %08x\n",
				state->name, i, state->cmd[i]);
		}
	}
}

/**
 * Emit all atoms with a dirty field equal to dirty.
 *
 * The caller must have ensured that there is enough space in the command
 * buffer.
 */
static INLINE void r300EmitAtoms(r300ContextPtr r300, GLboolean dirty)
{
	BATCH_LOCALS(r300);
	struct r300_state_atom *atom;
	int dwords;

    cp_wait(r300, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	BEGIN_BATCH_NO_AUTOSTATE(2);
	OUT_BATCH(cmdpacket0(r300->radeon.radeonScreen, R300_TX_INVALTAGS, 1));
	OUT_BATCH(R300_TX_FLUSH);
	END_BATCH();
    end_3d(r300);

	/* Emit actual atoms */
	foreach(atom, &r300->hw.atomlist) {
		if ((atom->dirty || r300->hw.all_dirty) == dirty) {
			dwords = (*atom->check) (r300, atom);
			if (dwords) {
				if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
					r300PrintStateAtom(r300, atom);
				}
				if (atom->emit) {
					(*atom->emit)(r300, atom);
				} else {
					BEGIN_BATCH_NO_AUTOSTATE(dwords);
					OUT_BATCH_TABLE(atom->cmd, dwords);
					END_BATCH();
				}
				atom->dirty = GL_FALSE;
			} else {
				if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
					fprintf(stderr, "  skip state %s\n",
						atom->name);
				}
			}
		}
	}

	COMMIT_BATCH();
}

/**
 * Copy dirty hardware state atoms into the command buffer.
 *
 * We also copy out clean state if we're at the start of a buffer. That makes
 * it easy to recover from lost contexts.
 */
void r300EmitState(r300ContextPtr r300)
{
	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_PRIMS))
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300->cmdbuf.cs->cdw && !r300->hw.is_dirty && !r300->hw.all_dirty)
		return;

	/* To avoid going across the entire set of states multiple times, just check
	 * for enough space for the case of emitting all state.
	 */
	r300EnsureCmdBufSpace(r300, r300->hw.max_state_size, __FUNCTION__);

	if (!r300->cmdbuf.cs->cdw) {
		if (RADEON_DEBUG & DEBUG_STATE)
			fprintf(stderr, "Begin reemit state\n");

		r300EmitAtoms(r300, GL_FALSE);
	}

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "Begin dirty state\n");

	r300EmitAtoms(r300, GL_TRUE);
	r300->hw.is_dirty = GL_FALSE;
	r300->hw.all_dirty = GL_FALSE;
}

static unsigned packet0_count(r300ContextPtr r300, uint32_t *pkt)
{
    if (r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        return ((((*pkt) >> 16) & 0x3FFF) + 1);
    } else {
        drm_r300_cmd_header_t *t = (drm_r300_cmd_header_t*)pkt;
        return t->packet0.count;
    }
    return 0;
}

#define vpu_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->vpu.count)
#define r500fp_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->r500fp.count)

void emit_vpu(r300ContextPtr r300, struct r300_state_atom * atom)
{
	BATCH_LOCALS(r300);
	drm_r300_cmd_header_t cmd;
    uint32_t addr, ndw, i;

    if (!r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        uint32_t dwords;
    	dwords = (*atom->check) (r300, atom);
        BEGIN_BATCH_NO_AUTOSTATE(dwords);
        OUT_BATCH_TABLE(atom->cmd, dwords);
        END_BATCH();
        return;
    }

    cmd.u = atom->cmd[0];
    addr = (cmd.vpu.adrhi << 8) | cmd.vpu.adrlo;
	ndw = cmd.vpu.count * 4;
    if (ndw) {
        /* flush processing vertices */
        OUT_BATCH(CP_PACKET0(R300_SC_SCREENDOOR, 0));
        OUT_BATCH(0x0);
        OUT_BATCH(CP_PACKET0(RADEON_WAIT_UNTIL, 0));
        OUT_BATCH((1 << 15) | (1 << 28));
        OUT_BATCH(CP_PACKET0(R300_SC_SCREENDOOR, 0));
        OUT_BATCH(0x00FFFFFF);
        OUT_BATCH(CP_PACKET0(R300_VAP_PVS_STATE_FLUSH_REG, 0));
        OUT_BATCH(1);
        /* write vpu */
        OUT_BATCH(CP_PACKET0(R300_VAP_PVS_UPLOAD_ADDRESS, 0));
        OUT_BATCH(addr);
        OUT_BATCH(CP_PACKET0(R300_VAP_PVS_UPLOAD_DATA, ndw-1) | RADEON_ONE_REG_WR);
        for (i = 0; i < ndw; i++) {
            OUT_BATCH(atom->cmd[i+1]);
        }
    }
}

static void emit_tex_offsets(r300ContextPtr r300, struct r300_state_atom * atom)
{
	BATCH_LOCALS(r300);
	int numtmus = packet0_count(r300, r300->hw.tex.offset.cmd);

	if (numtmus) {
		int i;

		for(i = 0; i < numtmus; ++i) {
		    BEGIN_BATCH(2);
    		OUT_BATCH_REGSEQ(R300_TX_OFFSET_0 + (i * 4), 1);
			r300TexObj *t = r300->hw.textures[i];
			if (t && !t->image_override) {
				OUT_BATCH_RELOC(t->tile_bits, t->mt->bo, 0,
                                RADEON_GEM_DOMAIN_VRAM, 0, 0);
			} else if (!t) {
				OUT_BATCH(r300->radeon.radeonScreen->texOffset[0]);
			} else {
                if (t->bo) {
                    OUT_BATCH_RELOC(t->tile_bits, t->bo, 0,
                    RADEON_GEM_DOMAIN_VRAM, 0, 0);
                } else {
    				OUT_BATCH(t->override_offset);
                }
			}
            END_BATCH();
		}
	}
}

static void emit_cb_offset(r300ContextPtr r300, struct r300_state_atom * atom)
{
	BATCH_LOCALS(r300);
	struct radeon_renderbuffer *rrb;
	uint32_t cbpitch;
	GLframebuffer *fb = r300->radeon.dri.drawable->driverPrivate;

	rrb = r300->radeon.state.color.rrb;
    if (r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        rrb = fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
    }
	if (!rrb || !rrb->bo) {
		fprintf(stderr, "no rrb\n");
		return;
	}

	cbpitch = (rrb->pitch / rrb->cpp);
	if (rrb->cpp == 4)
		cbpitch |= R300_COLOR_FORMAT_ARGB8888;
	else
		cbpitch |= R300_COLOR_FORMAT_RGB565;

	if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
		cbpitch |= R300_COLOR_TILE_ENABLE;

	BEGIN_BATCH(4);
	OUT_BATCH_REGSEQ(R300_RB3D_COLOROFFSET0, 1);
	OUT_BATCH_RELOC(0, rrb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	OUT_BATCH_REGSEQ(R300_RB3D_COLORPITCH0, 1);
	OUT_BATCH(cbpitch);
	END_BATCH();
}

static void emit_zb_offset(r300ContextPtr r300, struct r300_state_atom * atom)
{
	BATCH_LOCALS(r300);
	struct radeon_renderbuffer *rrb;
	uint32_t zbpitch;

	rrb = r300->radeon.state.depth_buffer;
	if (!rrb)
		return;

	zbpitch = (rrb->pitch / rrb->cpp);
    if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
        zbpitch |= R300_DEPTHMACROTILE_ENABLE;
    }
    if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE){
        zbpitch |= R300_DEPTHMICROTILE_TILED;
    }

	BEGIN_BATCH(4);
	OUT_BATCH_REGSEQ(R300_ZB_DEPTHOFFSET, 1);
	OUT_BATCH_RELOC(0, rrb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	OUT_BATCH_REGVAL(R300_ZB_DEPTHPITCH, zbpitch);
	END_BATCH();
}

static int check_always(r300ContextPtr r300, struct r300_state_atom *atom)
{
	return atom->cmd_size;
}

static int check_variable(r300ContextPtr r300, struct r300_state_atom *atom)
{
	int cnt;
    if (atom->cmd[0] == CP_PACKET2) {
        return 0;
    }
	cnt = packet0_count(r300, atom->cmd);
	return cnt ? cnt + 1 : 0;
}

int check_vpu(r300ContextPtr r300, struct r300_state_atom *atom)
{
	int cnt;

	cnt = vpu_count(atom->cmd);
	return cnt ? (cnt * 4) + 1 : 0;
}

static int check_r500fp(r300ContextPtr r300, struct r300_state_atom *atom)
{
	int cnt;

	cnt = r500fp_count(atom->cmd);
	return cnt ? (cnt * 6) + 1 : 0;
}

static int check_r500fp_const(r300ContextPtr r300, struct r300_state_atom *atom)
{
	int cnt;

    cnt = r500fp_count(atom->cmd);
	return cnt ? (cnt * 4) + 1 : 0;
}

#define ALLOC_STATE( ATOM, CHK, SZ, IDX )				\
   do {									\
      r300->hw.ATOM.cmd_size = (SZ);					\
      r300->hw.ATOM.cmd = (uint32_t*)CALLOC((SZ) * sizeof(uint32_t));	\
      r300->hw.ATOM.name = #ATOM;					\
      r300->hw.ATOM.idx = (IDX);					\
      r300->hw.ATOM.check = check_##CHK;				\
      r300->hw.ATOM.dirty = GL_FALSE;					\
      r300->hw.max_state_size += (SZ);					\
      insert_at_tail(&r300->hw.atomlist, &r300->hw.ATOM);		\
   } while (0)
/**
 * Allocate memory for the command buffer and initialize the state atom
 * list. Note that the initial hardware state is set by r300InitState().
 */
void r300InitCmdBuf(r300ContextPtr r300)
{
	int size, mtu;
	int has_tcl = 1;
	int is_r500 = 0;
	int i;

	if (!(r300->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
		has_tcl = 0;

	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515)
		is_r500 = 1;

	r300->hw.max_state_size = 2 + 2;	/* reserve extra space for WAIT_IDLE and tex cache flush */

	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr, "Using %d maximum texture units..\n", mtu);
	}

	/* Setup the atom linked list */
	make_empty_list(&r300->hw.atomlist);
	r300->hw.atomlist.name = "atom-list";

	/* Initialize state atoms */
	ALLOC_STATE(vpt, always, R300_VPT_CMDSIZE, 0);
	r300->hw.vpt.cmd[R300_VPT_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R300_SE_VPORT_XSCALE, 6);
	ALLOC_STATE(vap_cntl, always, R300_VAP_CNTL_SIZE, 0);
	r300->hw.vap_cntl.cmd[R300_VAP_CNTL_FLUSH] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_PVS_STATE_FLUSH_REG, 1);
	r300->hw.vap_cntl.cmd[R300_VAP_CNTL_FLUSH_1] = 0;
	r300->hw.vap_cntl.cmd[R300_VAP_CNTL_CMD] = cmdpacket0(r300->radeon.radeonScreen, R300_VAP_CNTL, 1);
	if (is_r500) {
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
	ALLOC_STATE(gb_misc, always, R300_GB_MISC_CMDSIZE, 0);
	r300->hw.gb_misc.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GB_MSPOS0, 5);
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
	ALLOC_STATE(shade, always, 5, 0);
	r300->hw.shade.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_GA_ENHANCE, 4);
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
		ALLOC_STATE(ri, always, R500_RI_CMDSIZE, 0);
		r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_RS_IP_0, 16);
		for (i = 0; i < 8; i++) {
			r300->hw.ri.cmd[R300_RI_CMD_0 + i +1] =
			  (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_S_SHIFT) |
                          (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_T_SHIFT) |
                          (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_R_SHIFT) |
                          (R500_RS_IP_PTR_K1 << R500_RS_IP_TEX_PTR_Q_SHIFT);
		}
		ALLOC_STATE(rr, variable, R300_RR_CMDSIZE, 0);
		r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(r300->radeon.radeonScreen, R500_RS_INST_0, 1);
	} else {
		ALLOC_STATE(ri, always, R300_RI_CMDSIZE, 0);
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
		ALLOC_STATE(r500fp_const, r500fp_const, R500_FPP_CMDSIZE, 0);
		r300->hw.r500fp_const.cmd[R300_FPI_CMD_0] =
            cmdr500fp(r300->radeon.radeonScreen, 0, 0, 1, 0);
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
	ALLOC_STATE(cb, always, R300_CB_CMDSIZE, 0);
	r300->hw.cb.emit = &emit_cb_offset;
	ALLOC_STATE(rb3d_dither_ctl, always, 10, 0);
	r300->hw.rb3d_dither_ctl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_DITHER_CTL, 9);
	ALLOC_STATE(rb3d_aaresolve_ctl, always, 2, 0);
	r300->hw.rb3d_aaresolve_ctl.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_RB3D_AARESOLVE_CTL, 1);
	ALLOC_STATE(rb3d_discard_src_pixel_lte_threshold, always, 3, 0);
	r300->hw.rb3d_discard_src_pixel_lte_threshold.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R500_RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 2);
	ALLOC_STATE(zs, always, R300_ZS_CMDSIZE, 0);
	r300->hw.zs.cmd[R300_ZS_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_ZB_CNTL, 3);
	ALLOC_STATE(zstencil_format, always, 5, 0);
	r300->hw.zstencil_format.cmd[0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_ZB_FORMAT, 4);
	ALLOC_STATE(zb, always, R300_ZB_CMDSIZE, 0);
	r300->hw.zb.emit = emit_zb_offset;
	ALLOC_STATE(zb_depthclearvalue, always, 2, 0);
	r300->hw.zb_depthclearvalue.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, R300_ZB_DEPTHCLEARVALUE, 1);
	ALLOC_STATE(unk4F30, always, 3, 0);
	r300->hw.unk4F30.cmd[0] = cmdpacket0(r300->radeon.radeonScreen, 0x4F30, 2);
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
    	r300->hw.vpi.emit = emit_vpu;

		if (is_r500) {
		    ALLOC_STATE(vpp, vpu, R300_VPP_CMDSIZE, 0);
		    r300->hw.vpp.cmd[0] =
			cmdvpu(r300->radeon.radeonScreen, R500_PVS_CONST_START, 0);
        	r300->hw.vpp.emit = emit_vpu;

		    ALLOC_STATE(vps, vpu, R300_VPS_CMDSIZE, 0);
		    r300->hw.vps.cmd[0] =
			cmdvpu(r300->radeon.radeonScreen, R500_POINT_VPORT_SCALE_OFFSET, 1);
        	r300->hw.vps.emit = emit_vpu;

			for (i = 0; i < 6; i++) {
				ALLOC_STATE(vpucp[i], vpu, R300_VPUCP_CMDSIZE, 0);
				r300->hw.vpucp[i].cmd[0] =
					cmdvpu(r300->radeon.radeonScreen,
                           R500_PVS_UCP_START + i, 1);
              	r300->hw.vpucp[i].emit = emit_vpu;
			}
		} else {
		    ALLOC_STATE(vpp, vpu, R300_VPP_CMDSIZE, 0);
		    r300->hw.vpp.cmd[0] =
			cmdvpu(r300->radeon.radeonScreen, R300_PVS_CONST_START, 0);
            r300->hw.vpp.emit = emit_vpu;

		    ALLOC_STATE(vps, vpu, R300_VPS_CMDSIZE, 0);
		    r300->hw.vps.cmd[0] =
			cmdvpu(r300->radeon.radeonScreen, R300_POINT_VPORT_SCALE_OFFSET, 1);
            r300->hw.vps.emit = emit_vpu;

			for (i = 0; i < 6; i++) {
				ALLOC_STATE(vpucp[i], vpu, R300_VPUCP_CMDSIZE, 0);
				r300->hw.vpucp[i].cmd[0] =
					cmdvpu(r300->radeon.radeonScreen,
                           R300_PVS_UCP_START + i, 1);
                r300->hw.vpucp[i].emit = emit_vpu;
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

	ALLOC_STATE(tex.offset, variable, 1, 0);
	r300->hw.tex.offset.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_OFFSET_0, 0);
	r300->hw.tex.offset.emit = &emit_tex_offsets;

	ALLOC_STATE(tex.chroma_key, variable, mtu + 1, 0);
	r300->hw.tex.chroma_key.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_CHROMA_KEY_0, 0);

	ALLOC_STATE(tex.border_color, variable, mtu + 1, 0);
	r300->hw.tex.border_color.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(r300->radeon.radeonScreen, R300_TX_BORDER_COLOR_0, 0);

	r300->hw.is_dirty = GL_TRUE;
	r300->hw.all_dirty = GL_TRUE;

	/* Initialize command buffer */
	size =
	    256 * driQueryOptioni(&r300->radeon.optionCache,
				  "command_buffer_size");
	if (size < 2 * r300->hw.max_state_size) {
		size = 2 * r300->hw.max_state_size + 65535;
	}
	if (size > 64 * 256)
		size = 64 * 256;

    size = 64 * 1024 / 4;
	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA)) {
		fprintf(stderr, "sizeof(drm_r300_cmd_header_t)=%zd\n",
			sizeof(drm_r300_cmd_header_t));
		fprintf(stderr, "sizeof(drm_radeon_cmd_buffer_t)=%zd\n",
			sizeof(drm_radeon_cmd_buffer_t));
		fprintf(stderr,
			"Allocating %d bytes command buffer (max state is %d bytes)\n",
			size * 4, r300->hw.max_state_size * 4);
	}

    if (r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        int fd = r300->radeon.radeonScreen->driScreen->fd;
        r300->cmdbuf.csm = radeon_cs_manager_gem_ctor(fd);
    } else {
        r300->cmdbuf.csm = radeon_cs_manager_legacy_ctor(&r300->radeon);
    }
    if (r300->cmdbuf.csm == NULL) {
        /* FIXME: fatal error */
        return;
    }
    r300->cmdbuf.cs = radeon_cs_create(r300->cmdbuf.csm, size);
    assert(r300->cmdbuf.cs != NULL);
	r300->cmdbuf.size = size;
}

/**
 * Destroy the command buffer and state atoms.
 */
void r300DestroyCmdBuf(r300ContextPtr r300)
{
	struct r300_state_atom *atom;

    radeon_cs_destroy(r300->cmdbuf.cs);
	foreach(atom, &r300->hw.atomlist) {
		FREE(atom->cmd);
	}
    if (r300->radeon.radeonScreen->driScreen->dri2.enabled) {
        radeon_cs_manager_gem_dtor(r300->cmdbuf.csm);
    } else {
        radeon_cs_manager_legacy_dtor(r300->cmdbuf.csm);
    }
}
