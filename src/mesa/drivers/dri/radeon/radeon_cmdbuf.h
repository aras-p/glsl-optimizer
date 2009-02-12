#ifndef COMMON_CMDBUF_H
#define COMMON_CMDBUF_H

#include "radeon_bocs_wrapper.h"

void rcommonEnsureCmdBufSpace(radeonContextPtr rmesa, int dwords, const char *caller);
int rcommonFlushCmdBuf(radeonContextPtr rmesa, const char *caller);
int rcommonFlushCmdBufLocked(radeonContextPtr rmesa, const char *caller);
void rcommonInitCmdBuf(radeonContextPtr rmesa);
void rcommonDestroyCmdBuf(radeonContextPtr rmesa);

void rcommonBeginBatch(radeonContextPtr rmesa,
		       int n,
		       int dostate,
		       const char *file,
		       const char *function,
		       int line);

#define RADEON_CP_PACKET3_NOP                       0xC0001000
#define RADEON_CP_PACKET3_NEXT_CHAR                 0xC0001900
#define RADEON_CP_PACKET3_PLY_NEXTSCAN              0xC0001D00
#define RADEON_CP_PACKET3_SET_SCISSORS              0xC0001E00
#define RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM     0xC0002300
#define RADEON_CP_PACKET3_LOAD_MICROCODE            0xC0002400
#define RADEON_CP_PACKET3_WAIT_FOR_IDLE             0xC0002600
#define RADEON_CP_PACKET3_3D_DRAW_VBUF              0xC0002800
#define RADEON_CP_PACKET3_3D_DRAW_IMMD              0xC0002900
#define RADEON_CP_PACKET3_3D_DRAW_INDX              0xC0002A00
#define RADEON_CP_PACKET3_LOAD_PALETTE              0xC0002C00
#define RADEON_CP_PACKET3_3D_LOAD_VBPNTR            0xC0002F00
#define RADEON_CP_PACKET3_CNTL_PAINT                0xC0009100
#define RADEON_CP_PACKET3_CNTL_BITBLT               0xC0009200
#define RADEON_CP_PACKET3_CNTL_SMALLTEXT            0xC0009300
#define RADEON_CP_PACKET3_CNTL_HOSTDATA_BLT         0xC0009400
#define RADEON_CP_PACKET3_CNTL_POLYLINE             0xC0009500
#define RADEON_CP_PACKET3_CNTL_POLYSCANLINES        0xC0009800
#define RADEON_CP_PACKET3_CNTL_PAINT_MULTI          0xC0009A00
#define RADEON_CP_PACKET3_CNTL_BITBLT_MULTI         0xC0009B00
#define RADEON_CP_PACKET3_CNTL_TRANS_BITBLT         0xC0009C00

#define CP_PACKET2  (2 << 30)
#define CP_PACKET0(reg, n)	(RADEON_CP_PACKET0 | ((n)<<16) | ((reg)>>2))
#define CP_PACKET0_ONE(reg, n)	(RADEON_CP_PACKET0 | RADEON_CP_PACKET0_ONE_REG_WR | ((n)<<16) | ((reg)>>2))
#define CP_PACKET3( pkt, n )						\
	(RADEON_CP_PACKET3 | (pkt) | ((n) << 16))

/**
 * Every function writing to the command buffer needs to declare this
 * to get the necessary local variables.
 */
#define BATCH_LOCALS(rmesa) \
	const radeonContextPtr b_l_rmesa = rmesa

/**
 * Prepare writing n dwords to the command buffer,
 * including producing any necessary state emits on buffer wraparound.
 */
#define BEGIN_BATCH(n) rcommonBeginBatch(b_l_rmesa, n, 1, __FILE__, __FUNCTION__, __LINE__)

/**
 * Same as BEGIN_BATCH, but do not cause automatic state emits.
 */
#define BEGIN_BATCH_NO_AUTOSTATE(n) rcommonBeginBatch(b_l_rmesa, n, 0, __FILE__, __FUNCTION__, __LINE__)

/**
 * Write one dword to the command buffer.
 */
#define OUT_BATCH(data) \
	do { \
        radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, data);\
	} while(0)

/**
 * Write a relocated dword to the command buffer.
 */
#define OUT_BATCH_RELOC(data, bo, offset, rd, wd, flags) 	\
	do { 							\
        if (0 && offset) {					\
            fprintf(stderr, "(%s:%s:%d) offset : %d\n",		\
            __FILE__, __FUNCTION__, __LINE__, offset);		\
        }							\
        radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, offset);	\
        radeon_cs_write_reloc(b_l_rmesa->cmdbuf.cs, 		\
                              bo, rd, wd, flags);		\
	if (!b_l_rmesa->radeonScreen->kernel_mm) 		\
		b_l_rmesa->cmdbuf.cs->section_cdw += 2;		\
	} while(0)


/**
 * Write n dwords from ptr to the command buffer.
 */
#define OUT_BATCH_TABLE(ptr,n) \
	do { \
		int _i; \
        for (_i=0; _i < n; _i++) {\
            radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, ptr[_i]);\
        }\
	} while(0)

/**
 * Finish writing dwords to the command buffer.
 * The number of (direct or indirect) OUT_BATCH calls between the previous
 * BEGIN_BATCH and END_BATCH must match the number specified at BEGIN_BATCH time.
 */
#define END_BATCH() \
	do { \
        radeon_cs_end(b_l_rmesa->cmdbuf.cs, __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

/**
 * After the last END_BATCH() of rendering, this indicates that flushing
 * the command buffer now is okay.
 */
#define COMMIT_BATCH() \
	do { \
	} while(0)


/** Single register write to command buffer; requires 2 dwords. */
#define OUT_BATCH_REGVAL(reg, val) \
	OUT_BATCH(cmdpacket0(b_l_rmesa->radeonScreen, (reg), 1)); \
	OUT_BATCH((val))

/** Continuous register range write to command buffer; requires 1 dword,
 * expects count dwords afterwards for register contents. */
#define OUT_BATCH_REGSEQ(reg, count) \
	OUT_BATCH(cmdpacket0(b_l_rmesa->radeonScreen, (reg), (count)));

/** Write a 32 bit float to the ring; requires 1 dword. */
#define OUT_BATCH_FLOAT32(f) \
	OUT_BATCH(radeonPackFloat32((f)));


/* Fire the buffered vertices no matter what.
 */
static INLINE void radeon_firevertices(radeonContextPtr radeon)
{
   if (radeon->cmdbuf.cs->cdw || radeon->dma.flush )
      radeonFlush(radeon->glCtx);
}

#endif
