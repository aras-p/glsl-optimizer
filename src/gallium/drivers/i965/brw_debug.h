#ifndef BRW_DEBUG_H
#define BRW_DEBUG_H

/* ================================================================
 * Debugging:
 */

#define DEBUG_TEXTURE	        0x1
#define DEBUG_STATE	        0x2
#define DEBUG_IOCTL	        0x4
#define DEBUG_BLIT	        0x8
#define DEBUG_CURBE             0x10
#define DEBUG_FALLBACKS	        0x20
#define DEBUG_VERBOSE	        0x40
#define DEBUG_BATCH             0x80
#define DEBUG_PIXEL             0x100
#define DEBUG_WINSYS            0x200
#define DEBUG_MIN_URB           0x400
#define DEBUG_DISASSEM           0x800
#define DEBUG_unused3           0x1000
#define DEBUG_SYNC	        0x2000
#define DEBUG_PRIMS	        0x4000
#define DEBUG_VERTS	        0x8000
#define DEBUG_unused4           0x10000
#define DEBUG_DMA               0x20000
#define DEBUG_SANITY            0x40000
#define DEBUG_SLEEP             0x80000
#define DEBUG_STATS             0x100000
#define DEBUG_unused5           0x200000
#define DEBUG_SINGLE_THREAD     0x400000
#define DEBUG_WM                0x800000
#define DEBUG_URB               0x1000000
#define DEBUG_VS                0x2000000

#ifdef DEBUG
extern int BRW_DEBUG;
#else
#define BRW_DEBUG 0
#endif



#endif
