/*
 * Written by José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "matypes.h"

/*
 * make the following approximation to the division (Sree)
 *
 *   rgb*a/255 ~= (rgb*(a+1)) >> 256
 *
 * which is the fastest method that satisfies the following OpenGL criteria
 *
 *   0*0 = 0 and 255*255 = 255
 *
 * note this one should be used alone
 */
#define GMBT_ALPHA_PLUS_ONE	0

/*
 * take the geometric series approximation to the division
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
#define GMBT_GEOMETRIC_SERIES	1

/*
 * when using a geometric series division instead of truncating the result 
 * use roundoff in the approximation (Jim Blinn)
 *
 *   t = rgb*a + 0x80
 *
 * achieving the exact results
 */
#define GMBT_ROUNDOFF		0

/* instead of the roundoff this adds a small correction to satisfy the OpenGL criteria
 *
 *   t/255 ~= (t + (t >> 8) + (t >> 15)) >> 8
 *
 * note that although is faster than rounding off it doesn't give always the exact results
 */
#define GMBT_GEOMETRIC_CORRECTION	1

/*
 * do
 *
 *   s = (q - p)*a + q
 *
 * instead of
 *
 *   s = p*a + q*(1-a)
 *
 * this eliminates a multiply at the expense of
 * complicating the roundoff but is generally worth it
 */
#define GMBT_SIGNED_ARITHMETIC	1

#if GMBT_ROUNDOFF
    SEG_DATA

ALIGNDATA8
const_80:
	D_LONG 0x00800080, 0x00800080
#endif 

   SEG_TEXT

ALIGNTEXT16
GLOBL GLNAME(_mesa_mmx_blend_transparency)

/*
 * void blend_transparency( GLcontext *ctx,
 *                          GLuint n, 
 *                          const GLubyte mask[],
 *                          GLchan rgba[][4], 
 *                          CONST GLchan dest[][4] )
 * 
 * Common transparency blending mode.
 */
GLNAME( _mesa_mmx_blend_transparency ):

    PUSH_L     ( EBP )
    MOV_L      ( ESP, EBP )
    PUSH_L     ( ESI )
    PUSH_L     ( EDI )
    PUSH_L     ( EBX )

    MOV_L      ( REGOFF(12, EBP), ECX )		/* n */
    CMP_L      ( CONST(0), ECX)
    JE         ( LLBL (GMBT_return) )

    MOV_L      ( REGOFF(16, EBP), EBX )		/* mask */
    MOV_L      ( REGOFF(20, EBP), EDI )         /* rgba */
    MOV_L      ( REGOFF(24, EBP), ESI )         /* dest */
    
    TEST_L     ( CONST(4), EDI )		/* align rgba on an 8-byte boundary */
    JZ         ( LLBL (GMBT_align_end) )

    CMP_B      ( CONST(0), REGIND(EBX) )	/* *mask == 0 */
    JE         ( LLBL (GMBT_align_continue) )

    PXOR       ( MM0, MM0 )			/*   0x0000  |   0x0000  |   0x0000  |   0x0000  */

    MOVD       ( REGIND(ESI), MM1 )		/*     |     |     |     | qa1 | qb1 | qg1 | qr1 */
    MOVD       ( REGIND(EDI), MM2 )		/*     |     |     |     | pa1 | pb1 | pg1 | pr1 */

    PUNPCKLBW  ( MM0, MM1 )			/*    qa1    |    qb1    |    qg1    |    qr1    */
    PUNPCKLBW  ( MM0, MM2 )			/*    pa1    |    pb1    |    pg1    |    pr1    */

    MOVQ       ( MM2, MM3 )

    PUNPCKHWD  ( MM3, MM3 )			/*    pa1    |    pa1    |           |           */
    PUNPCKHDQ  ( MM3, MM3 )                     /*    pa1    |    pa1    |    pa1    |    pa1    */

#if GMBT_ALPHA_PLUS_ONE
    PCMPEQW    ( MM4, MM4 )			/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */

    PSUBW      ( MM4, MM3 )                     /*   pa1 + 1 |   pa1 + 1 |   pa1 + 1 |   pa1 + 1 */
#endif

#if GMBT_SIGNED_ARITHMETIC
    PSUBW      ( MM1, MM2 )                     /* pa1 - qa1 | pb1 - qb1 | pg1 - qg1 | pr1 - qr1 */

    PSLLW      ( CONST(8), MM1 )		/*                    q1 << 8                    */

#if GMBT_ROUNDOFF
    MOVQ       ( MM2, MM4 )
#endif

    PMULLW     ( MM3, MM2 )			/*              t1 = (q1 - p1)*pa1               */

#if GMBT_ROUNDOFF
    PSRLW      ( CONST(15), MM4 )		/*                 q1 > p1 ? 1 : 0               */

    PSLLW      ( CONST(8), MM4 )		/*             q1 > p1 ? 0x100 : 0               */

    PSUBW      ( MM4, MM2 )                     /*                  t1 -=? 0x100                 */
#endif

#else
    PCMPEQW    ( MM4, MM4 )			/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */
    PUNPCKLBW  ( MM0, MM4 )			/*   0x00ff  |   0x00ff  |   0x00ff  |   0x00ff  */
    MOVQ       ( MM4, MM0 )
    
    PMULLW     ( MM3, MM2 )			/*                     p1*pa1                    */

    PSUBW      ( MM3, MM0 )			/* 255 - pa1 | 255 - pa1 | 255 - pa1 | 255 - pa1 */

    PMULLW     ( MM0, MM1 )			/*                  q1*(255 - pa1)               */

    PADDW      ( MM1, MM2 )			/*           t1 = p1*pa1 + q1*(255 - pa1)        */
#endif

#if GMBT_ROUNDOFF
    MOVQ       ( CONTENT(const_80), MM4 )

    PADDW      ( MM4, MM2 )                     /*                 t1 += 0x80                    */
#endif

#if GMBT_GEOMETRIC_SERIES
    MOVQ       ( MM2, MM3 )

    PSRLW      ( CONST(8), MM3 )		/*                    t1 >> 8                    */

    PADDW      ( MM3, MM2 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */
#endif

#if GMBT_SIGNED_ARITHMETIC
    PADDW      ( MM1, MM2 )			/*              (t1/255 + q1) << 8               */
#endif

    PSRLW      ( CONST(8), MM2 )		/*    sa1    |    sb1    |    sg1    |    sr1    */
    
    PACKUSWB   ( MM0, MM2 )			/*     |     |     |     | sa1 | sb1 | sg1 | sr1 */
    MOVD       ( MM2, REGIND(EDI) )

LLBL (GMBT_align_continue):

    DEC_L      ( ECX )				/* n -= 1 */
    INC_L      ( EBX )		                /* mask += 1 */
    ADD_L      ( CONST(4), EDI )		/* rgba += 1 */
    ADD_L      ( CONST(4), ESI )		/* dest += 1 */ 

LLBL (GMBT_align_end):

    CMP_L      ( CONST(2), ECX)
    JB         ( LLBL (GMBT_loop_end) )

ALIGNTEXT16
LLBL (GMBT_loop_begin):

    CMP_W      ( CONST(0), REGIND(EBX) )	/* *mask == 0 && *(mask + 1) == 0 */
    JE         ( LLBL (GMBT_loop_continue) )

    /* NOTE: the instruction pairing when multiple pipelines are available must be checked */

    PXOR       ( MM0, MM0 )			/*   0x0000  |   0x0000  |   0x0000  |   0x0000  */

    MOVQ       ( REGIND(ESI), MM7 )		/* qa2 | qb2 | qg2 | qr2 | qa1 | qb1 | qg1 | qr1 */
    MOVQ       ( REGIND(EDI), MM6 )		/* pa2 | pb2 | pg2 | pr2 | pa1 | pb1 | pg1 | pr1 */

    MOVQ       ( MM7, MM1 )
    MOVQ       ( MM6, MM2 )

    PUNPCKLBW  ( MM0, MM1 )			/*    qa1    |    qb1    |    qg1    |    qr1    */
    PUNPCKHBW  ( MM0, MM7 )                     /*    qa2    |    qb2    |    qg2    |    qr2    */
    PUNPCKLBW  ( MM0, MM2 )			/*    pa1    |    pb1    |    pg1    |    pr1    */
    PUNPCKHBW  ( MM0, MM6 )                     /*    pa2    |    pb2    |    pg2    |    pr2    */

    MOVQ       ( MM2, MM3 )
    MOVQ       ( MM6, MM5 )

    PUNPCKHWD  ( MM3, MM3 )			/*    pa1    |    pa1    |           |           */
    PUNPCKHWD  ( MM5, MM5 )			/*    pa2    |    pa2    |           |           */
    PUNPCKHDQ  ( MM3, MM3 )                     /*    pa1    |    pa1    |    pa1    |    pa1    */
    PUNPCKHDQ  ( MM5, MM5 )                     /*    pa2    |    pa2    |    pa2    |    pa2    */

#if GMBT_ALPHA_PLUS_ONE
    PCMPEQW    ( MM4, MM4 )			/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */

    PSUBW      ( MM4, MM3 )                     /*   pa1 + 1 |   pa1 + 1 |   pa1 + 1 |   pa1 + 1 */
    PSUBW      ( MM4, MM5 )                     /*   pa2 + 1 |   pa2 + 1 |   pa2 + 1 |   pa2 + 1 */
#endif

#if GMBT_SIGNED_ARITHMETIC
    PSUBW      ( MM1, MM2 )                     /* pa1 - qa1 | pb1 - qb1 | pg1 - qg1 | pr1 - qr1 */
    PSUBW      ( MM7, MM6 )                     /* pa2 - qa2 | pb2 - qb2 | pg2 - qg2 | pr2 - qr2 */

    PSLLW      ( CONST(8), MM1 )		/*                    q1 << 8                    */
    PSLLW      ( CONST(8), MM7 )		/*                    q2 << 8                    */

#if GMBT_ROUNDOFF
    MOVQ       ( MM2, MM0 )
    MOVQ       ( MM6, MM4 )
#endif

    PMULLW     ( MM3, MM2 )			/*              t1 = (q1 - p1)*pa1               */
    PMULLW     ( MM5, MM6 )			/*              t2 = (q2 - p2)*pa2               */

#if GMBT_ROUNDOFF
    PSRLW      ( CONST(15), MM0 )		/*                 q1 > p1 ? 1 : 0               */
    PSRLW      ( CONST(15), MM4 )		/*                 q2 > q2 ? 1 : 0               */

    PSLLW      ( CONST(8), MM0 )		/*             q1 > p1 ? 0x100 : 0               */
    PSLLW      ( CONST(8), MM4 )		/*             q2 > q2 ? 0x100 : 0               */

    PSUBW      ( MM0, MM2 )                     /*                  t1 -=? 0x100                 */
    PSUBW      ( MM4, MM7 )                     /*                  t2 -=? 0x100                 */ 
#endif

#else
    PCMPEQW    ( MM4, MM4 )			/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */
    PUNPCKLBW  ( MM0, MM4 )			/*   0x00ff  |   0x00ff  |   0x00ff  |   0x00ff  */
    MOVQ       ( MM4, MM0 )
    
    PMULLW     ( MM3, MM2 )			/*                     p1*pa1                    */
    PMULLW     ( MM5, MM6 )			/*                     p2*pa2                    */

    PSUBW      ( MM3, MM0 )			/* 255 - pa1 | 255 - pa1 | 255 - pa1 | 255 - pa1 */
    PSUBW      ( MM5, MM4 )			/* 255 - pa2 | 255 - pa2 | 255 - pa2 | 255 - pa2 */

    PMULLW     ( MM0, MM1 )			/*                  q1*(255 - pa1)               */
    PMULLW     ( MM4, MM7 )			/*                  q2*(255 - pa2)               */

    PADDW      ( MM1, MM2 )			/*           t1 = p1*pa1 + q1*(255 - pa1)        */
    PADDW      ( MM7, MM6 )			/*           t2 = p2*pa2 + q2*(255 - pa2)        */
#endif

#if GMBT_ROUNDOFF
    MOVQ       ( CONTENT(const_80), MM4 )

    PADDW      ( MM4, MM2 )                     /*                 t1 += 0x80                    */
    PADDW      ( MM4, MM6 )                     /*                 t2 += 0x80                    */
#endif

#if GMBT_GEOMETRIC_SERIES
    MOVQ       ( MM2, MM3 )
    MOVQ       ( MM6, MM5 )

    PSRLW      ( CONST(8), MM3 )		/*                    t1 >> 8                    */
    PSRLW      ( CONST(8), MM5 )		/*                    t2 >> 8                    */

    PADDW      ( MM3, MM2 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */
    PADDW      ( MM5, MM6 )			/*        t2 + (t2 >> 8) ~= (t2/255) << 8        */

#if GMBT_GEOMETRIC_CORRECTION 
    PSRLW      ( CONST(7), MM3 )		/*                    t1 >> 15                   */
    PSRLW      ( CONST(7), MM5 )		/*                    t2 >> 15                   */

    PADDW      ( MM3, MM2 )			/*  t1 + (t1 >> 8) + (t1 >>15) ~= (t1/255) << 8  */
    PADDW      ( MM5, MM6 )			/*  t2 + (t2 >> 8) + (t2 >>15) ~= (t2/255) << 8  */
#endif
#endif

#if GMBT_SIGNED_ARITHMETIC
    PADDW      ( MM1, MM2 )			/*              (t1/255 + q1) << 8               */
    PADDW      ( MM7, MM6 )			/*              (t2/255 + q2) << 8               */
#endif

    PSRLW      ( CONST(8), MM2 )		/*    sa1    |    sb1    |    sg1    |    sr1    */
    PSRLW      ( CONST(8), MM6 )		/*    sa2    |    sb2    |    sg2    |    sr2    */
    
    PACKUSWB   ( MM6, MM2 )			/* sa2 | sb2 | sg2 | sr2 | sa1 | sb1 | sg1 | sr1 */
    MOVQ       ( MM2, REGIND(EDI) )

LLBL (GMBT_loop_continue):

    DEC_L      ( ECX )
    DEC_L      ( ECX )				/* n -= 2 */
    ADD_L      ( CONST(2), EBX )		/* mask += 2 */
    ADD_L      ( CONST(8), EDI )		/* rgba += 2 */
    ADD_L      ( CONST(8), ESI )		/* dest += 2 */ 
    CMP_L      ( CONST(2), ECX )
    JAE        ( LLBL (GMBT_loop_begin) )

LLBL (GMBT_loop_end):

    CMP_L      ( CONST(1), ECX )
    JB         ( LLBL (GMBT_done) )

    CMP_B      ( CONST(0), REGIND(EBX) )	/* *mask == 0 */
    JE         ( LLBL (GMBT_done) )

    PXOR       ( MM0, MM0 )			/*   0x0000  |   0x0000  |   0x0000  |   0x0000  */

    MOVD       ( REGIND(ESI), MM1 )		/*     |     |     |     | qa1 | qb1 | qg1 | qr1 */
    MOVD       ( REGIND(EDI), MM2 )		/*     |     |     |     | pa1 | pb1 | pg1 | pr1 */

    PUNPCKLBW  ( MM0, MM1 )			/*    qa1    |    qb1    |    qg1    |    qr1    */
    PUNPCKLBW  ( MM0, MM2 )			/*    pa1    |    pb1    |    pg1    |    pr1    */

    MOVQ       ( MM2, MM3 )

    PUNPCKHWD  ( MM3, MM3 )			/*    pa1    |    pa1    |           |           */
    PUNPCKHDQ  ( MM3, MM3 )                     /*    pa1    |    pa1    |    pa1    |    pa1    */

#if GMBT_ALPHA_PLUS_ONE
    PCMPEQW    ( MM4, MM4 )			/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */

    PSUBW      ( MM4, MM3 )                     /*   pa1 + 1 |   pa1 + 1 |   pa1 + 1 |   pa1 + 1 */
#endif

#if GMBT_SIGNED_ARITHMETIC
    PSUBW      ( MM1, MM2 )                     /* pa1 - qa1 | pb1 - qb1 | pg1 - qg1 | pr1 - qr1 */

    PSLLW      ( CONST(8), MM1 )		/*                    q1 << 8                    */

#if GMBT_ROUNDOFF
    MOVQ       ( MM2, MM4 )
#endif

    PMULLW     ( MM3, MM2 )			/*              t1 = (q1 - p1)*pa1               */

#if GMBT_ROUNDOFF
    PSRLW      ( CONST(15), MM4 )		/*                 q1 > p1 ? 1 : 0               */

    PSLLW      ( CONST(8), MM4 )		/*             q1 > p1 ? 0x100 : 0               */

    PSUBW      ( MM4, MM2 )                     /*                  t1 -=? 0x100                 */
#endif

#else
    PCMPEQW    ( MM4, MM4 )			/*   0xffff  |   0xffff  |   0xffff  |   0xffff  */
    PUNPCKLBW  ( MM0, MM4 )			/*   0x00ff  |   0x00ff  |   0x00ff  |   0x00ff  */
    MOVQ       ( MM4, MM0 )
    
    PMULLW     ( MM3, MM2 )			/*                     p1*pa1                    */

    PSUBW      ( MM3, MM0 )			/* 255 - pa1 | 255 - pa1 | 255 - pa1 | 255 - pa1 */

    PMULLW     ( MM0, MM1 )			/*                  q1*(255 - pa1)               */

    PADDW      ( MM1, MM2 )			/*           t1 = p1*pa1 + q1*(255 - pa1)        */
#endif

#if GMBT_ROUNDOFF
    MOVQ       ( CONTENT(const_80), MM4 )

    PADDW      ( MM4, MM2 )                     /*                 t1 += 0x80                    */
#endif

#if GMBT_GEOMETRIC_SERIES
    MOVQ       ( MM2, MM3 )

    PSRLW      ( CONST(8), MM3 )		/*                    t1 >> 8                    */

    PADDW      ( MM3, MM2 )			/*        t1 + (t1 >> 8) ~= (t1/255) << 8        */
#endif

#if GMBT_SIGNED_ARITHMETIC
    PADDW      ( MM1, MM2 )			/*              (t1/255 + q1) << 8               */
#endif

    PSRLW      ( CONST(8), MM2 )		/*    sa1    |    sb1    |    sg1    |    sr1    */
    
    PACKUSWB   ( MM0, MM2 )			/*     |     |     |     | sa1 | sb1 | sg1 | sr1 */
    MOVD       ( MM2, REGIND(EDI) )

LLBL (GMBT_done):

    EMMS

LLBL (GMBT_return):

    POP_L      ( EBX )
    POP_L      ( EDI )
    POP_L      ( ESI )
    MOV_L      ( EBP, ESP )
    POP_L      ( EBP )
    RET
