/*
 * Written by José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "matypes.h"


/* integer multiplication - alpha plus one
 *
 * makes the following approximation to the division (Sree)
 *
 *   rgb*a/255 ~= (rgb*(a+1)) >> 256
 *
 * which is the fastest method that satisfies the following OpenGL criteria
 *
 *   0*0 = 0 and 255*255 = 255
 *
 * note that MX1 is a register with 0xffffffffffffffff constant which can be easily obtained making
 *
 *   PCMPEQW    ( MX1, MX1 )
 */
#define GMB_MULT_AP1( MP1, MA1, MP2, MA2, MX1 ) \
    PSUBW      ( MX1, MA1 )			/*   a1 + 1  |   a1 + 1  |   a1 + 1  |   a1 + 1  */	;\
TWO(PSUBW      ( MX1, MA2 ))			/*   a2 + 1  |   a2 + 1  |   a2 + 1  |   a2 + 1  */	;\
													;\
    PMULLW     ( MP1, MA1 )			/*                  t1 = p1*a1                   */	;\
TWO(PMULLW     ( MP2, MA2 ))			/*                  t2 = p2*a2                   */	;\
													;\
    PSRLW      ( CONST(8), MA1 )		/*               t1 >> 8 ~= t1/255               */	;\
TWO(PSRLW      ( CONST(8), MA2 ))		/*               t2 >> 8 ~= t2/255               */	


/* integer multiplication - geometric series
 *
 * takes the geometric series approximation to the division
 *
 *   t/255 = (t >> 8) + (t >> 16) + (t >> 24) ..
 *
 * in this case just the first two terms to fit in 16bit arithmetic
 *
 *   t/255 ~= (t + (t >> 8)) >> 8
 *
 * note that just by itself it doesn't satisfies the OpenGL criteria, as 255*255 = 254, 
 * so the special case a = 255 must be accounted or roundoff must be used
 */
#define GMB_MULT_GS( MP1, MA1, MP2, MA2 ) \
    PMULLW     ( MP1, MA1 )			/*                  t1 = p1*a1                   */	;\
TWO(PMULLW     ( MP2, MA2 ))			/*                  t2 = p2*a2                   */	;\
													;\
    MOVQ       ( MA1, MP1 )										;\
TWO(MOVQ       ( MA2, MP2 ))										;\
													;\
    PSRLW      ( CONST(8), MP1 )		/*                    t1 >> 8                    */	;\
TWO(PSRLW      ( CONST(8), MP2 ))		/*                    t2 >> 8                    */	;\
													;\
    PADDW      ( MP1, MA1 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */	;\
TWO(PADDW      ( MP2, MA2 ))			/*        t2 + (t2 >> 8) ~= (t2/255) << 8        */	;\
													;\
    PSRLW      ( CONST(8), MA1 )		/*    sa1    |    sb1    |    sg1    |    sr1    */	;\
TWO(PSRLW      ( CONST(8), MA2 ))		/*    sa2    |    sb2    |    sg2    |    sr2    */


/* integer multiplication - geometric series plus rounding
 *
 * when using a geometric series division instead of truncating the result 
 * use roundoff in the approximation (Jim Blinn)
 *
 *   t = rgb*a + 0x80
 *
 * achieving the exact results
 *
 * note that M80 is register with the 0x0080008000800080 constant
 */
#define GMB_MULT_GSR( MP1, MA1, MP2, MA2, M80 ) \
    PMULLW     ( MP1, MA1 )			/*                  t1 = p1*a1                   */	;\
TWO(PMULLW     ( MP2, MA2 ))			/*                  t2 = p2*a2                   */	;\
													;\
    PADDW      ( M80, MA1 )			/*                 t1 += 0x80                    */	;\
TWO(PADDW      ( M80, MA2 ))			/*                 t2 += 0x80                    */	;\
													;\
    MOVQ       ( MA1, MP1 )										;\
TWO(MOVQ       ( MA2, MP2 ))										;\
													;\
    PSRLW      ( CONST(8), MP1 )		/*                    t1 >> 8                    */	;\
TWO(PSRLW      ( CONST(8), MP2 ))		/*                    t2 >> 8                    */	;\
													;\
    PADDW      ( MP1, MA1 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */	;\
TWO(PADDW      ( MP2, MA2 ))			/*        t2 + (t2 >> 8) ~= (t2/255) << 8        */	;\
													;\
    PSRLW      ( CONST(8), MA1 )		/*    sa1    |    sb1    |    sg1    |    sr1    */	;\
TWO(PSRLW      ( CONST(8), MA2 ))		/*    sa2    |    sb2    |    sg2    |    sr2    */


/* linear interpolation - geometric series 
 */
#define GMB_LERP_GS( MP1, MQ1, MA1, MP2, MQ2, MA2) \
    PSUBW      ( MQ1, MP1 )                     /* pa1 - qa1 | pb1 - qb1 | pg1 - qg1 | pr1 - qr1 */	;\
TWO(PSUBW      ( MQ2, MP2 ))                    /* pa2 - qa2 | pb2 - qb2 | pg2 - qg2 | pr2 - qr2 */	;\
													;\
    PSLLW      ( CONST(8), MQ1 )		/*                    q1 << 8                    */	;\
TWO(PSLLW      ( CONST(8), MQ2 ))		/*                    q2 << 8                    */	;\
													;\
    PMULLW     ( MP1, MA1 )			/*              t1 = (q1 - p1)*pa1               */	;\
TWO(PMULLW     ( MP2, MA2 ))			/*              t2 = (q2 - p2)*pa2               */	;\
													;\
    MOVQ       ( MA1, MP1 )										;\
TWO(MOVQ       ( MA2, MP2 ))										;\
													;\
    PSRLW      ( CONST(8), MP1 )		/*                    t1 >> 8                    */	;\
TWO(PSRLW      ( CONST(8), MP2 ))		/*                    t2 >> 8                    */	;\
													;\
    PADDW      ( MP1, MA1 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */	;\
TWO(PADDW      ( MP2, MA2 ))			/*        t2 + (t2 >> 8) ~= (t2/255) << 8        */	;\
													;\
    PADDW      ( MQ1, MA1 )			/*              (t1/255 + q1) << 8               */	;\
TWO(PADDW      ( MQ2, MA2 ))			/*              (t2/255 + q2) << 8               */	;\
													;\
    PSRLW      ( CONST(8), MA1 )		/*    sa1    |    sb1    |    sg1    |    sr1    */	;\
TWO(PSRLW      ( CONST(8), MA2 ))		/*    sa2    |    sb2    |    sg2    |    sr2    */


/* linear interpolation - geometric series with roundoff
 *
 * this is a generalization of Blinn's formula to signed arithmetic
 *
 * note that M80 is a register with the 0x0080008000800080 constant
 */
#define GMB_LERP_GSR( MP1, MQ1, MA1, MP2, MQ2, MA2, M80) \
    PSUBW      ( MQ1, MP1 )                     /* pa1 - qa1 | pb1 - qb1 | pg1 - qg1 | pr1 - qr1 */	;\
TWO(PSUBW      ( MQ2, MP2 ))                    /* pa2 - qa2 | pb2 - qb2 | pg2 - qg2 | pr2 - qr2 */	;\
													;\
    PSLLW      ( CONST(8), MQ1 )		/*                    q1 << 8                    */	;\
TWO(PSLLW      ( CONST(8), MQ2 ))		/*                    q2 << 8                    */	;\
													;\
    PMULLW     ( MP1, MA1 )			/*              t1 = (q1 - p1)*pa1               */	;\
TWO(PMULLW     ( MP2, MA2 ))			/*              t2 = (q2 - p2)*pa2               */	;\
													;\
    PSRLW      ( CONST(15), MP1 )		/*                 q1 > p1 ? 1 : 0               */	;\
TWO(PSRLW      ( CONST(15), MP2 ))		/*                 q2 > q2 ? 1 : 0               */	;\
													;\
    PSLLW      ( CONST(8), MP1 )		/*             q1 > p1 ? 0x100 : 0               */	;\
TWO(PSLLW      ( CONST(8), MP2 ))		/*             q2 > q2 ? 0x100 : 0               */	;\
													;\
    PSUBW      ( MP1, MA1 )			/*                  t1 -=? 0x100                 */	;\
TWO(PSUBW      ( MP2, MA2 ))			/*                  t2 -=? 0x100                 */	;\
 													;\
    PADDW      ( M80, MA1 )			/*                 t1 += 0x80                    */	;\
TWO(PADDW      ( M80, MA2 ))			/*                 t2 += 0x80                    */	;\
													;\
    MOVQ       ( MA1, MP1 )										;\
TWO(MOVQ       ( MA2, MP2 ))										;\
													;\
    PSRLW      ( CONST(8), MP1 )		/*                    t1 >> 8                    */	;\
TWO(PSRLW      ( CONST(8), MP2 ))		/*                    t2 >> 8                    */	;\
													;\
    PADDW      ( MP1, MA1 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */	;\
TWO(PADDW      ( MP2, MA2 ))			/*        t2 + (t2 >> 8) ~= (t2/255) << 8        */	;\
													;\
    PADDW      ( MQ1, MA1 )			/*              (t1/255 + q1) << 8               */	;\
TWO(PADDW      ( MQ2, MA2 ))			/*              (t2/255 + q2) << 8               */	;\
													;\
    PSRLW      ( CONST(8), MA1 )		/*    sa1    |    sb1    |    sg1    |    sr1    */	;\
TWO(PSRLW      ( CONST(8), MA2 ))		/*    sa2    |    sb2    |    sg2    |    sr2    */


/* linear interpolation - geometric series with correction
 *
 * instead of the roundoff this adds a small correction to satisfy the OpenGL criteria
 *
 *   t/255 ~= (t + (t >> 8) + (t >> 15)) >> 8
 *
 * note that although is faster than rounding off it doesn't give always the exact results
 */
#define GMB_LERP_GSC( MP1, MQ1, MA1, MP2, MQ2, MA2) \
    PSUBW      ( MQ1, MP1 )                     /* pa1 - qa1 | pb1 - qb1 | pg1 - qg1 | pr1 - qr1 */	;\
TWO(PSUBW      ( MQ2, MP2 ))                    /* pa2 - qa2 | pb2 - qb2 | pg2 - qg2 | pr2 - qr2 */	;\
													;\
    PSLLW      ( CONST(8), MQ1 )		/*                    q1 << 8                    */	;\
TWO(PSLLW      ( CONST(8), MQ2 ))		/*                    q2 << 8                    */	;\
													;\
    PMULLW     ( MP1, MA1 )			/*              t1 = (q1 - p1)*pa1               */	;\
TWO(PMULLW     ( MP2, MA2 ))			/*              t2 = (q2 - p2)*pa2               */	;\
													;\
    MOVQ       ( MA1, MP1 )										;\
TWO(MOVQ       ( MA2, MP2 ))										;\
													;\
    PSRLW      ( CONST(8), MP1 )		/*                    t1 >> 8                    */	;\
TWO(PSRLW      ( CONST(8), MP2 ))		/*                    t2 >> 8                    */	;\
													;\
    PADDW      ( MP1, MA1 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */	;\
TWO(PADDW      ( MP2, MA2 ))			/*        t2 + (t2 >> 8) ~= (t2/255) << 8        */	;\
													;\
    PSRLW      ( CONST(7), MP1 )		/*                    t1 >> 15                   */	;\
TWO(PSRLW      ( CONST(7), MP2 ))		/*                    t2 >> 15                   */	;\
													;\
    PADDW      ( MP1, MA1 )			/*  t1 + (t1 >> 8) + (t1 >>15) ~= (t1/255) << 8  */	;\
TWO(PADDW      ( MP2, MA2 ))			/*  t2 + (t2 >> 8) + (t2 >>15) ~= (t2/255) << 8  */	;\
													;\
    PADDW      ( MQ1, MA1 )			/*              (t1/255 + q1) << 8               */	;\
TWO(PADDW      ( MQ2, MA2 ))			/*              (t2/255 + q2) << 8               */	;\
													;\
    PSRLW      ( CONST(8), MA1 )		/*    sa1    |    sb1    |    sg1    |    sr1    */	;\
TWO(PSRLW      ( CONST(8), MA2 ))		/*    sa2    |    sb2    |    sg2    |    sr2    */


/* common blending initialization code
 */
#if 0	/* rounding not used */
    SEG_DATA

ALIGNDATA8
const_80:
	D_LONG 0x00800080, 0x00800080

#define GMB_INIT( M00, M80 ) \
    PXOR       ( M00, M00 )			/*   0x0000  |   0x0000  |   0x0000  |   0x0000  */
    MOVQ       ( CONTENT(const_80), M80 )	/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */

#else

#define GMB_INIT( M00 ) \
    PXOR       ( M00, M00 )			/*   0x0000  |   0x0000  |   0x0000  |   0x0000  */

#endif

/* common blending loading code
 *
 * note that M00 is a register with 0x0000000000000000 constant which can be easily obtained making
 *
 *   PXOR      ( M00, M00 )
 */
#define GMB_LOAD(rgba, dest, MP1, MQ1, MA1, MP2, MQ2, MA2, M00) \
ONE(MOVD       ( REGIND(rgba), MP1 ))		/*     |     |     |     | qa1 | qb1 | qg1 | qr1 */	;\
ONE(MOVD       ( REGIND(dest), MQ1 ))		/*     |     |     |     | pa1 | pb1 | pg1 | pr1 */	;\
													;\
TWO(MOVQ       ( REGIND(rgba), MP1 ))		/* qa2 | qb2 | qg2 | qr2 | qa1 | qb1 | qg1 | qr1 */	;\
TWO(MOVQ       ( REGIND(dest), MQ1 ))		/* pa2 | pb2 | pg2 | pr2 | pa1 | pb1 | pg1 | pr1 */	;\
													;\
TWO(MOVQ       ( MP1, MP2 ))										;\
TWO(MOVQ       ( MQ1, MQ2 ))										;\
													;\
    PUNPCKLBW  ( M00, MQ1 )			/*    qa1    |    qb1    |    qg1    |    qr1    */	;\
TWO(PUNPCKHBW  ( M00, MQ2 ))                    /*    qa2    |    qb2    |    qg2    |    qr2    */	;\
    PUNPCKLBW  ( M00, MP1 )			/*    pa1    |    pb1    |    pg1    |    pr1    */	;\
TWO(PUNPCKHBW  ( M00, MP2 ))                    /*    pa2    |    pb2    |    pg2    |    pr2    */	;\
													;\
    MOVQ       ( MP1, MA1 )										;\
TWO(MOVQ       ( MP2, MA2 ))										;\
													;\
    PUNPCKHWD  ( MA1, MA1 )			/*    pa1    |    pa1    |           |           */	;\
TWO(PUNPCKHWD  ( MA2, MA2 ))			/*    pa2    |    pa2    |           |           */	;\
    PUNPCKHDQ  ( MA1, MA1 )                     /*    pa1    |    pa1    |    pa1    |    pa1    */	;\
TWO(PUNPCKHDQ  ( MA2, MA2 ))                    /*    pa2    |    pa2    |    pa2    |    pa2    */

 
/* common blending storing code
 */
#define GMB_STORE(rgba, MA1, MA2) \
    PACKUSWB   ( MA2, MA1 )			/* sa2 | sb2 | sg2 | sr2 | sa1 | sb1 | sg1 | sr1 */	;\
													;\
ONE(MOVD       ( MA1, REGIND(rgba) ))									;\
TWO(MOVQ       ( MA1, REGIND(rgba) ))


   SEG_TEXT


/* common transparency blending mode
 */

#define TAG(x) x##_transparency

#define INIT \
	GMB_INIT( MM0 )

#define MAIN \
	GMB_LOAD( EDI, ESI, MM1, MM2, MM3, MM4, MM5, MM6, MM0)						;\
	GMB_LERP_GSC( MM1, MM2, MM3, MM4, MM5, MM6 )							;\
	GMB_STORE( EDI, MM3, MM6 )

#include "mmx_blendtmp.h"

