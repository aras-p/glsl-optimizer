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

#include "r600_context.h"
#include "r600_ioctl.h"
#include "radeon_reg.h"
#include "r600_reg.h"
#include "r600_cmdbuf.h"
#include "r600_emit.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_mipmap_tree.h"
#include "r600_state.h"
#include "radeon_reg.h"

#define R600_VAP_PVS_UPLOAD_ADDRESS 0x2200
#   define RADEON_ONE_REG_WR        (1 << 15)

/** # of dwords reserved for additional instructions that may need to be written
 * during flushing.
 */
#define SPACE_FOR_FLUSHING	4

static unsigned packet0_count(r600ContextPtr r600, uint32_t *pkt)
{
    if (r600->radeon.radeonScreen->kernel_mm) {
        return ((((*pkt) >> 16) & 0x3FFF) + 1);
    } else {
        drm_r300_cmd_header_t *t = (drm_r300_cmd_header_t*)pkt;
        return t->packet0.count;
    }
    return 0;
}

#define vpu_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->vpu.count)

void emit_vpu(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	BATCH_LOCALS(&r600->radeon);
	drm_r300_cmd_header_t cmd;
	uint32_t addr, ndw, i;
	
	if (!r600->radeon.radeonScreen->kernel_mm) {
		uint32_t dwords;
		dwords = (*atom->check) (ctx, atom);
		BEGIN_BATCH_NO_AUTOSTATE(dwords);
		OUT_BATCH_TABLE(atom->cmd, dwords);
		END_BATCH();
		return;
	}
	
	cmd.u = atom->cmd[0];
	addr = (cmd.vpu.adrhi << 8) | cmd.vpu.adrlo;
	ndw = cmd.vpu.count * 4;
	if (ndw) {

		if (r600->vap_flush_needed) {
			BEGIN_BATCH_NO_AUTOSTATE(15 + ndw);

			/* flush processing vertices */
			OUT_BATCH_REGVAL(R600_SC_SCREENDOOR, 0);
			OUT_BATCH_REGVAL(R600_RB3D_DSTCACHE_CTLSTAT, R600_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
			OUT_BATCH_REGVAL(RADEON_WAIT_UNTIL, RADEON_WAIT_3D_IDLECLEAN);
			OUT_BATCH_REGVAL(R600_SC_SCREENDOOR, 0xffffff);
			OUT_BATCH_REGVAL(R600_VAP_PVS_STATE_FLUSH_REG, 0);
			r600->vap_flush_needed = GL_FALSE;
		} else {
			BEGIN_BATCH_NO_AUTOSTATE(5 + ndw);
		}
		OUT_BATCH_REGVAL(R600_VAP_PVS_UPLOAD_ADDRESS, addr);
		OUT_BATCH(CP_PACKET0(R600_VAP_PVS_UPLOAD_DATA, ndw-1) | RADEON_ONE_REG_WR);
		for (i = 0; i < ndw; i++) {
			OUT_BATCH(atom->cmd[i+1]);
		}
		OUT_BATCH_REGVAL(R600_VAP_PVS_STATE_FLUSH_REG, 0);
		END_BATCH();
	}
}

static void emit_tex_offsets(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	BATCH_LOCALS(&r600->radeon);
	int numtmus = packet0_count(r600, r600->hw.tex.offset.cmd);
	int notexture = 0;

	if (numtmus) {
		int i;

		for(i = 0; i < numtmus; ++i) {
		    radeonTexObj *t = r600->hw.textures[i];
		
		    if (!t)
			notexture = 1;
		}

		if (r600->radeon.radeonScreen->kernel_mm && notexture) {
			return;
		}
		BEGIN_BATCH_NO_AUTOSTATE(4 * numtmus);
		for(i = 0; i < numtmus; ++i) {
		    radeonTexObj *t = r600->hw.textures[i];
		    OUT_BATCH_REGSEQ(R600_TX_OFFSET_0 + (i * 4), 1);
		    if (t && !t->image_override) {
			    OUT_BATCH_RELOC(t->tile_bits, t->mt->bo, 0,
					    RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
		    } else if (!t) {
			    OUT_BATCH(r600->radeon.radeonScreen->texOffset[0]);
		    } else { /* override cases */
			    if (t->bo) {
				    OUT_BATCH_RELOC(t->tile_bits, t->bo, 0,
						    RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
			    } else if (!r600->radeon.radeonScreen->kernel_mm) {
				    OUT_BATCH(t->override_offset);
			    }
			    else
			    	OUT_BATCH(r600->radeon.radeonScreen->texOffset[0]);
		    }
		}
		END_BATCH();
	}
}

static void emit_cb_offset(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	BATCH_LOCALS(&r600->radeon);
	struct radeon_renderbuffer *rrb;
	uint32_t cbpitch;
	uint32_t offset = r600->radeon.state.color.draw_offset;

	rrb = radeon_get_colorbuffer(&r600->radeon);
	if (!rrb || !rrb->bo) {
		fprintf(stderr, "no rrb\n");
		return;
	}

	cbpitch = (rrb->pitch / rrb->cpp);
	if (rrb->cpp == 4)
		cbpitch |= R600_COLOR_FORMAT_ARGB8888;
	else
		cbpitch |= R600_COLOR_FORMAT_RGB565;

	if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
		cbpitch |= R600_COLOR_TILE_ENABLE;

	BEGIN_BATCH_NO_AUTOSTATE(8);
	OUT_BATCH_REGSEQ(R600_RB3D_COLOROFFSET0, 1);
	OUT_BATCH_RELOC(offset, rrb->bo, offset, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	OUT_BATCH_REGSEQ(R600_RB3D_COLORPITCH0, 1);
	OUT_BATCH_RELOC(cbpitch, rrb->bo, cbpitch, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	END_BATCH();
    if (r600->radeon.radeonScreen->driScreen->dri2.enabled) {
        if (r600->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
            BEGIN_BATCH_NO_AUTOSTATE(3);
            OUT_BATCH_REGSEQ(R600_SC_SCISSORS_TL, 2);
            OUT_BATCH(0);
            OUT_BATCH((rrb->width << R600_SCISSORS_X_SHIFT) |
                    (rrb->height << R600_SCISSORS_Y_SHIFT));
            END_BATCH();
        } else {
            BEGIN_BATCH_NO_AUTOSTATE(3);
            OUT_BATCH_REGSEQ(R600_SC_SCISSORS_TL, 2);
            OUT_BATCH((R600_SCISSORS_OFFSET << R600_SCISSORS_X_SHIFT) |
                    (R600_SCISSORS_OFFSET << R600_SCISSORS_Y_SHIFT));
            OUT_BATCH(((rrb->width + R600_SCISSORS_OFFSET) << R600_SCISSORS_X_SHIFT) |
                    ((rrb->height + R600_SCISSORS_OFFSET) << R600_SCISSORS_Y_SHIFT));
            END_BATCH();
        }
    }
}

static void emit_zb_offset(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	BATCH_LOCALS(&r600->radeon);
	struct radeon_renderbuffer *rrb;
	uint32_t zbpitch;

	rrb = radeon_get_depthbuffer(&r600->radeon);
	if (!rrb)
		return;

	zbpitch = (rrb->pitch / rrb->cpp);
	if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
		zbpitch |= R600_DEPTHMACROTILE_ENABLE;
	}
	if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE){
		zbpitch |= R600_DEPTHMICROTILE_TILED;
	}
	
	BEGIN_BATCH_NO_AUTOSTATE(6);
	OUT_BATCH_REGSEQ(R600_ZB_DEPTHOFFSET, 1);
	OUT_BATCH_RELOC(0, rrb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
	OUT_BATCH_REGVAL(R600_ZB_DEPTHPITCH, zbpitch);
	END_BATCH();
}

static void emit_zstencil_format(GLcontext *ctx, struct radeon_state_atom * atom)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	BATCH_LOCALS(&r600->radeon);
	struct radeon_renderbuffer *rrb;
	uint32_t format = 0;

	rrb = radeon_get_depthbuffer(&r600->radeon);
	if (!rrb)
	  format = 0;
	else {
	  if (rrb->cpp == 2)
	    format = R600_DEPTHFORMAT_16BIT_INT_Z;
	  else if (rrb->cpp == 4)
	    format = R600_DEPTHFORMAT_24BIT_INT_Z_8BIT_STENCIL;
	}

	OUT_BATCH(atom->cmd[0]);
	atom->cmd[1] &= ~0xf;
	atom->cmd[1] |= format;
	OUT_BATCH(atom->cmd[1]);
	OUT_BATCH(atom->cmd[2]);
	OUT_BATCH(atom->cmd[3]);
	OUT_BATCH(atom->cmd[4]);
}

static int check_always(GLcontext *ctx, struct radeon_state_atom *atom)
{
	return atom->cmd_size;
}

static int check_variable(GLcontext *ctx, struct radeon_state_atom *atom)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	int cnt;
	if (atom->cmd[0] == CP_PACKET2) {
		return 0;
	}
	cnt = packet0_count(r600, atom->cmd);
	return cnt ? cnt + 1 : 0;
}

int check_vpu(GLcontext *ctx, struct radeon_state_atom *atom)
{
	int cnt;

	cnt = vpu_count(atom->cmd);
	return cnt ? (cnt * 4) + 1 : 0;
}

#define ALLOC_STATE( ATOM, CHK, SZ, IDX )				\
   do {									\
      r600->hw.ATOM.cmd_size = (SZ);					\
      r600->hw.ATOM.cmd = (uint32_t*)CALLOC((SZ) * sizeof(uint32_t));	\
      r600->hw.ATOM.name = #ATOM;					\
      r600->hw.ATOM.idx = (IDX);					\
      r600->hw.ATOM.check = check_##CHK;				\
      r600->hw.ATOM.dirty = GL_FALSE;					\
      r600->radeon.hw.max_state_size += (SZ);					\
      insert_at_tail(&r600->radeon.hw.atomlist, &r600->hw.ATOM);		\
   } while (0)
/**
 * Allocate memory for the command buffer and initialize the state atom
 * list. Note that the initial hardware state is set by r600InitState().
 */
void r600InitCmdBuf(r600ContextPtr r600)
{
	int mtu;
	int i;

	r600->radeon.hw.max_state_size = 2 + 2;	/* reserve extra space for WAIT_IDLE and tex cache flush */

	mtu = r600->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr, "Using %d maximum texture units..\n", mtu);
	}

	/* Setup the atom linked list */
	make_empty_list(&r600->radeon.hw.atomlist);
	r600->radeon.hw.atomlist.name = "atom-list";

	/* Initialize state atoms */
	ALLOC_STATE(vpt, always, R600_VPT_CMDSIZE, 0);
	r600->hw.vpt.cmd[R600_VPT_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_SE_VPORT_XSCALE, 6);
	ALLOC_STATE(vap_cntl, always, R600_VAP_CNTL_SIZE, 0);
	r600->hw.vap_cntl.cmd[R600_VAP_CNTL_FLUSH] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_PVS_STATE_FLUSH_REG, 1);
	r600->hw.vap_cntl.cmd[R600_VAP_CNTL_FLUSH_1] = 0;
	r600->hw.vap_cntl.cmd[R600_VAP_CNTL_CMD] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_CNTL, 1);

	ALLOC_STATE(vte, always, 3, 0);
	r600->hw.vte.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_SE_VTE_CNTL, 2);
	ALLOC_STATE(vap_vf_max_vtx_indx, always, 3, 0);
	r600->hw.vap_vf_max_vtx_indx.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_VF_MAX_VTX_INDX, 2);
	ALLOC_STATE(vap_cntl_status, always, 2, 0);
	r600->hw.vap_cntl_status.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_CNTL_STATUS, 1);
	ALLOC_STATE(vir[0], variable, R600_VIR_CMDSIZE, 0);
	r600->hw.vir[0].cmd[R600_VIR_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_VAP_PROG_STREAM_CNTL_0, 1);
	ALLOC_STATE(vir[1], variable, R600_VIR_CMDSIZE, 1);
	r600->hw.vir[1].cmd[R600_VIR_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_VAP_PROG_STREAM_CNTL_EXT_0, 1);
	ALLOC_STATE(vic, always, R600_VIC_CMDSIZE, 0);
	r600->hw.vic.cmd[R600_VIC_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_VTX_STATE_CNTL, 2);
	ALLOC_STATE(vap_psc_sgn_norm_cntl, always, 2, 0);
	r600->hw.vap_psc_sgn_norm_cntl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_PSC_SGN_NORM_CNTL, SGN_NORM_ZERO_CLAMP_MINUS_ONE);

	ALLOC_STATE(vap_clip_cntl, always, 2, 0);
	r600->hw.vap_clip_cntl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_CLIP_CNTL, 1);
	ALLOC_STATE(vap_clip, always, 5, 0);
	r600->hw.vap_clip.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_VAP_GB_VERT_CLIP_ADJ, 4);
	ALLOC_STATE(vap_pvs_vtx_timeout_reg, always, 2, 0);
	r600->hw.vap_pvs_vtx_timeout_reg.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, VAP_PVS_VTX_TIMEOUT_REG, 1);

	ALLOC_STATE(vof, always, R600_VOF_CMDSIZE, 0);
	r600->hw.vof.cmd[R600_VOF_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_VAP_OUTPUT_VTX_FMT_0, 2);

	ALLOC_STATE(pvs, always, R600_PVS_CMDSIZE, 0);
	r600->hw.pvs.cmd[R600_PVS_CMD_0] =
		cmdpacket0(r600->radeon.radeonScreen, R600_VAP_PVS_CODE_CNTL_0, 3);

	ALLOC_STATE(gb_enable, always, 2, 0);
	r600->hw.gb_enable.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GB_ENABLE, 1);
	ALLOC_STATE(gb_misc, always, R600_GB_MISC_CMDSIZE, 0);
	r600->hw.gb_misc.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GB_MSPOS0, 5);
	ALLOC_STATE(txe, always, R600_TXE_CMDSIZE, 0);
	r600->hw.txe.cmd[R600_TXE_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_TX_ENABLE, 1);
	ALLOC_STATE(ga_point_s0, always, 5, 0);
	r600->hw.ga_point_s0.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_POINT_S0, 4);
	ALLOC_STATE(ga_triangle_stipple, always, 2, 0);
	r600->hw.ga_triangle_stipple.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_TRIANGLE_STIPPLE, 1);
	ALLOC_STATE(ps, always, R600_PS_CMDSIZE, 0);
	r600->hw.ps.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_POINT_SIZE, 1);
	ALLOC_STATE(ga_point_minmax, always, 4, 0);
	r600->hw.ga_point_minmax.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_POINT_MINMAX, 3);
	ALLOC_STATE(lcntl, always, 2, 0);
	r600->hw.lcntl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_LINE_CNTL, 1);
	ALLOC_STATE(ga_line_stipple, always, 4, 0);
	r600->hw.ga_line_stipple.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_LINE_STIPPLE_VALUE, 3);
	ALLOC_STATE(shade, always, 5, 0);
	r600->hw.shade.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_ENHANCE, 4);
	ALLOC_STATE(polygon_mode, always, 4, 0);
	r600->hw.polygon_mode.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_POLY_MODE, 3);
	ALLOC_STATE(fogp, always, 3, 0);
	r600->hw.fogp.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_GA_FOG_SCALE, 2);
	ALLOC_STATE(zbias_cntl, always, 2, 0);
	r600->hw.zbias_cntl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_SU_TEX_WRAP, 1);
	ALLOC_STATE(zbs, always, R600_ZBS_CMDSIZE, 0);
	r600->hw.zbs.cmd[R600_ZBS_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_SU_POLY_OFFSET_FRONT_SCALE, 4);
	ALLOC_STATE(occlusion_cntl, always, 2, 0);
	r600->hw.occlusion_cntl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_SU_POLY_OFFSET_ENABLE, 1);
	ALLOC_STATE(cul, always, R600_CUL_CMDSIZE, 0);
	r600->hw.cul.cmd[R600_CUL_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_SU_CULL_MODE, 1);
	ALLOC_STATE(su_depth_scale, always, 3, 0);
	r600->hw.su_depth_scale.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_SU_DEPTH_SCALE, 2);
	ALLOC_STATE(rc, always, R600_RC_CMDSIZE, 0);
	r600->hw.rc.cmd[R600_RC_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_RS_COUNT, 2);

	ALLOC_STATE(ri, always, R600_RI_CMDSIZE, 0);
	r600->hw.ri.cmd[R600_RI_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_RS_IP_0, 8);
	ALLOC_STATE(rr, variable, R600_RR_CMDSIZE, 0);
	r600->hw.rr.cmd[R600_RR_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_RS_INST_0, 1);

	ALLOC_STATE(sc_hyperz, always, 3, 0);
	r600->hw.sc_hyperz.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_SC_HYPERZ, 2);
	ALLOC_STATE(sc_screendoor, always, 2, 0);
	r600->hw.sc_screendoor.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_SC_SCREENDOOR, 1);
	ALLOC_STATE(us_out_fmt, always, 6, 0);
	r600->hw.us_out_fmt.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_OUT_FMT, 5);

	ALLOC_STATE(fp, always, R600_FP_CMDSIZE, 0);
	r600->hw.fp.cmd[R600_FP_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_CONFIG, 3);
	r600->hw.fp.cmd[R600_FP_CMD_1] = cmdpacket0(r600->radeon.radeonScreen, R600_US_CODE_ADDR_0, 4);

	ALLOC_STATE(fpt, variable, R600_FPT_CMDSIZE, 0);
	r600->hw.fpt.cmd[R600_FPT_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_TEX_INST_0, 0);

	ALLOC_STATE(fpi[0], variable, R600_FPI_CMDSIZE, 0);
	r600->hw.fpi[0].cmd[R600_FPI_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_ALU_RGB_INST_0, 1);
	ALLOC_STATE(fpi[1], variable, R600_FPI_CMDSIZE, 1);
	r600->hw.fpi[1].cmd[R600_FPI_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_ALU_RGB_ADDR_0, 1);
	ALLOC_STATE(fpi[2], variable, R600_FPI_CMDSIZE, 2);
	r600->hw.fpi[2].cmd[R600_FPI_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_ALU_ALPHA_INST_0, 1);
	ALLOC_STATE(fpi[3], variable, R600_FPI_CMDSIZE, 3);
	r600->hw.fpi[3].cmd[R600_FPI_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_US_ALU_ALPHA_ADDR_0, 1);
	ALLOC_STATE(fpp, variable, R600_FPP_CMDSIZE, 0);
	r600->hw.fpp.cmd[R600_FPP_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_PFS_PARAM_0_X, 0);

	ALLOC_STATE(fogs, always, R600_FOGS_CMDSIZE, 0);
	r600->hw.fogs.cmd[R600_FOGS_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_FG_FOG_BLEND, 1);
	ALLOC_STATE(fogc, always, R600_FOGC_CMDSIZE, 0);
	r600->hw.fogc.cmd[R600_FOGC_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_FG_FOG_COLOR_R, 3);
	ALLOC_STATE(at, always, R600_AT_CMDSIZE, 0);
	r600->hw.at.cmd[R600_AT_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_FG_ALPHA_FUNC, 2);
	ALLOC_STATE(fg_depth_src, always, 2, 0);
	r600->hw.fg_depth_src.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_FG_DEPTH_SRC, 1);
	ALLOC_STATE(rb3d_cctl, always, 2, 0);
	r600->hw.rb3d_cctl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_RB3D_CCTL, 1);
	ALLOC_STATE(bld, always, R600_BLD_CMDSIZE, 0);
	r600->hw.bld.cmd[R600_BLD_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_RB3D_CBLEND, 2);
	ALLOC_STATE(cmk, always, R600_CMK_CMDSIZE, 0);
	r600->hw.cmk.cmd[R600_CMK_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, RB3D_COLOR_CHANNEL_MASK, 1);

	ALLOC_STATE(blend_color, always, 2, 0);
	r600->hw.blend_color.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_RB3D_BLEND_COLOR, 1);

	ALLOC_STATE(rop, always, 2, 0);
	r600->hw.rop.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_RB3D_ROPCNTL, 1);
	ALLOC_STATE(cb, always, R600_CB_CMDSIZE, 0);
	r600->hw.cb.emit = &emit_cb_offset;
	ALLOC_STATE(rb3d_dither_ctl, always, 10, 0);
	r600->hw.rb3d_dither_ctl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_RB3D_DITHER_CTL, 9);
	ALLOC_STATE(rb3d_aaresolve_ctl, always, 2, 0);
	r600->hw.rb3d_aaresolve_ctl.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_RB3D_AARESOLVE_CTL, 1);

	ALLOC_STATE(zs, always, R600_ZS_CMDSIZE, 0);
	r600->hw.zs.cmd[R600_ZS_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_ZB_CNTL, 3);

	ALLOC_STATE(zstencil_format, always, 5, 0);
	r600->hw.zstencil_format.cmd[0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_ZB_FORMAT, 4);
	r600->hw.zstencil_format.emit = emit_zstencil_format;

	ALLOC_STATE(zb, always, R600_ZB_CMDSIZE, 0);
	r600->hw.zb.emit = emit_zb_offset;
	ALLOC_STATE(zb_depthclearvalue, always, 2, 0);
	r600->hw.zb_depthclearvalue.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_ZB_DEPTHCLEARVALUE, 1);
	ALLOC_STATE(unk4F30, always, 3, 0);
	r600->hw.unk4F30.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, 0x4F30, 2);
	ALLOC_STATE(zb_hiz_offset, always, 2, 0);
	r600->hw.zb_hiz_offset.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_ZB_HIZ_OFFSET, 1);
	ALLOC_STATE(zb_hiz_pitch, always, 2, 0);
	r600->hw.zb_hiz_pitch.cmd[0] = cmdpacket0(r600->radeon.radeonScreen, R600_ZB_HIZ_PITCH, 1);

	ALLOC_STATE(vpi, vpu, R600_VPI_CMDSIZE, 0);
	r600->hw.vpi.cmd[0] =
		cmdvpu(r600->radeon.radeonScreen, R600_PVS_CODE_START, 0);
	r600->hw.vpi.emit = emit_vpu;

	ALLOC_STATE(vpp, vpu, R600_VPP_CMDSIZE, 0);
	r600->hw.vpp.cmd[0] =
		cmdvpu(r600->radeon.radeonScreen, R600_PVS_CONST_START, 0);
	r600->hw.vpp.emit = emit_vpu;

	ALLOC_STATE(vps, vpu, R600_VPS_CMDSIZE, 0);
	r600->hw.vps.cmd[0] =
		cmdvpu(r600->radeon.radeonScreen, R600_POINT_VPORT_SCALE_OFFSET, 1);
	r600->hw.vps.emit = emit_vpu;

	for (i = 0; i < 6; i++) {
		ALLOC_STATE(vpucp[i], vpu, R600_VPUCP_CMDSIZE, 0);
		r600->hw.vpucp[i].cmd[0] =
			cmdvpu(r600->radeon.radeonScreen,
			       R600_PVS_UCP_START + i, 1);
		r600->hw.vpucp[i].emit = emit_vpu;
	}

	/* Textures */
	ALLOC_STATE(tex.filter, variable, mtu + 1, 0);
	r600->hw.tex.filter.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FILTER0_0, 0);

	ALLOC_STATE(tex.filter_1, variable, mtu + 1, 0);
	r600->hw.tex.filter_1.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FILTER1_0, 0);

	ALLOC_STATE(tex.size, variable, mtu + 1, 0);
	r600->hw.tex.size.cmd[R600_TEX_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_TX_SIZE_0, 0);

	ALLOC_STATE(tex.format, variable, mtu + 1, 0);
	r600->hw.tex.format.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_FORMAT_0, 0);

	ALLOC_STATE(tex.pitch, variable, mtu + 1, 0);
	r600->hw.tex.pitch.cmd[R600_TEX_CMD_0] = cmdpacket0(r600->radeon.radeonScreen, R600_TX_FORMAT2_0, 0);

	ALLOC_STATE(tex.offset, variable, 1, 0);
	r600->hw.tex.offset.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_OFFSET_0, 0);
	r600->hw.tex.offset.emit = &emit_tex_offsets;

	ALLOC_STATE(tex.chroma_key, variable, mtu + 1, 0);
	r600->hw.tex.chroma_key.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_CHROMA_KEY_0, 0);

	ALLOC_STATE(tex.border_color, variable, mtu + 1, 0);
	r600->hw.tex.border_color.cmd[R600_TEX_CMD_0] =
	    cmdpacket0(r600->radeon.radeonScreen, R600_TX_BORDER_COLOR_0, 0);

	r600->radeon.hw.is_dirty = GL_TRUE;
	r600->radeon.hw.all_dirty = GL_TRUE;

	rcommonInitCmdBuf(&r600->radeon);
}
