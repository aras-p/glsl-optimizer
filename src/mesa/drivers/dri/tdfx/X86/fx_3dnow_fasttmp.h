/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/X86/fx_3dnow_fasttmp.h,v 1.2 2000/09/26 15:56:51 tsi Exp $ */

#if !defined(NASM_ASSEMBLER) && !defined(MASM_ASSEMBLER)
#define TAGLLBL(a) TAG(.L##a)
#else
#define TAGLLBL(a) TAG(a)
#endif

#if !GLIDE3

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              4
#define GR_VERTEX_Z_OFFSET              8
#define GR_VERTEX_R_OFFSET              12
#define GR_VERTEX_G_OFFSET              16
#define GR_VERTEX_B_OFFSET              20
#define GR_VERTEX_OOZ_OFFSET            24
#define GR_VERTEX_A_OFFSET              28
#define GR_VERTEX_OOW_OFFSET            32

#else /* GLIDE3 */

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              4
#define GR_VERTEX_OOZ_OFFSET            8
#define GR_VERTEX_OOW_OFFSET            12
#define GR_VERTEX_R_OFFSET              16
#define GR_VERTEX_G_OFFSET              20
#define GR_VERTEX_B_OFFSET              24
#define GR_VERTEX_A_OFFSET              28
#define GR_VERTEX_Z_OFFSET              32

#endif /* GLIDE3 */

#define GR_VERTEX_SOW_TMU0_OFFSET       36
#define GR_VERTEX_TOW_TMU0_OFFSET       40
#define GR_VERTEX_OOW_TMU0_OFFSET       44
#define GR_VERTEX_SOW_TMU1_OFFSET       48
#define GR_VERTEX_TOW_TMU1_OFFSET       52
#define GR_VERTEX_OOW_TMU1_OFFSET       56




/*#define MAT_SX 0        /*  accessed by REGIND !! */
#define MAT_SY 20
#define MAT_SZ 40
#define MAT_TX 48
#define MAT_TY 52
#define MAT_TZ 56




/* Do viewport map, device scale and perspective projection.
 *
 * void project_verts( GLfloat *first,
 *		       GLfloat *last,
 *		       const GLfloat *m,
 *		       GLuint stride )
 *
 *
 * Rearrange fxVertices to look like grVertices.
 */

GLOBL GLNAME( TAG(fx_3dnow_project_vertices) )
GLNAME( TAG(fx_3dnow_project_vertices) ):

    PUSH_L    ( EBP )

    MOV_L     ( REGOFF(8, ESP), ECX )    /* first_vert */
    MOV_L     ( REGOFF(12, ESP), EDX )     /* last_vert */

    CMP_L     ( ECX, EDX )
    JE        ( TAGLLBL(FXPV_end) )

    FEMMS

    PREFETCH  ( REGIND(ECX) )         /* fetch the first vertex */

    MOV_L     ( REGOFF(16, ESP), EBP )     /* matrix */
    MOV_L     ( REGOFF(20, ESP), EAX )     /* stride */

    MOVD      ( REGOFF(MAT_TX, EBP), MM6 )      /*             | tx           */
    PUNPCKLDQ ( REGOFF(MAT_TY, EBP), MM6 )      /*  ty         | tx           */

#if !defined(FX_V2)
    MOV_L     ( CONST(0x49400000), REGOFF(-8, ESP) )    /*  snapper           */
    MOV_L     ( CONST(0x49400000), REGOFF(-4, ESP) )    /*  snapper           */
#endif

    MOVQ      ( REGOFF(-8, ESP), MM4 )          /*  snapper    | snapper      */
    PFADD     ( MM4, MM6 )                      /*  ty+snapper | tx+snapper   */

    MOVD      ( REGIND(EBP), MM5 )
    PUNPCKLDQ ( REGOFF(MAT_SY, EBP), MM5 )      /*  vsy        | vsx          */

    MOVD      ( REGOFF(MAT_SZ, EBP), MM1 )      /*             | vsz          */


ALIGNTEXT32
TAGLLBL(FXPV_loop_start):

    PREFETCH  ( REGOFF(64, ECX) )               /* fetch the next-ish vertex */


    MOVD      ( REGOFF(12, ECX), MM0 )          /*              | f[3]        */
    PFRCP     ( MM0, MM0 )                      /*  oow = 1/f[3]              */

    MOVD      ( REGOFF(12, ECX), MM7 )          /*              | f[3]        */
    PFRCPIT1  ( MM0, MM7 )
    PFRCPIT2  ( MM0, MM7 )                      /*  oow         | oow         */

    PUNPCKLDQ ( MM7, MM7 )


#if (TYPE & SETUP_RGBA)
    MOVD      ( REGOFF(CLIP_R, ECX ), MM0 )     /*  f[RCOORD] = f[CLIP_R];    */
    MOVD      ( MM0, REGOFF(GR_VERTEX_R_OFFSET, ECX) )
#endif

#if (TYPE & SETUP_TMU1)
    MOVQ      ( REGOFF(CLIP_S1, ECX), MM0 ) /* f[S1COORD] = f[CLIP_S1] * oow  */
    PFMUL     ( MM7, MM0 )                  /* f[T1COORD] = f[CLIP_T1] * oow  */
    MOVQ      ( MM0, REGOFF(GR_VERTEX_SOW_TMU1_OFFSET, ECX) )
#endif


#if (TYPE & SETUP_TMU0)
    MOVQ      ( REGOFF(CLIP_S0, ECX), MM0 ) /* f[S0COORD] = f[CLIP_S0] * oow  */
    PFMUL     ( MM7, MM0 )                  /* f[T0COORD] = f[CLIP_T0] * oow  */
    MOVQ      ( MM0, REGOFF(GR_VERTEX_SOW_TMU0_OFFSET, ECX) )
#endif





/*  DO_SETUP_XYZ */

    MOVQ      ( REGIND(ECX), MM2 )              /*  f[1]        | f[0]        */
    PFMUL     ( MM7, MM2 )                      /*  f[1] * oow  | f[0] * oow  */

    MOVD      ( REGOFF(8, ECX), MM3 )           /*              | f[2]        */
    PFMUL     ( MM7, MM3 )                      /*              | f[2] * oow  */

    MOVD      ( REGOFF(MAT_TZ, EBP), MM0 )      /*              | vtz         */
    PFMUL     ( MM1, MM3 )                      /*              | f[2] *= vsz */

    PFADD     ( MM0, MM3 )                      /*              | f[2] += vtz */
    PFMUL     ( MM5, MM2 )                      /*  f[1] *= vsy | f[0] *= vsx */

    PFADD     ( MM6, MM2 )                      /*  f[1] += vty | f[0] += vtx */

#if !defined(FX_V2)
    PFSUB     ( MM4, MM2 )                      /*  f[0,1] -= snapper         */
#endif

    MOVQ      ( MM2, REGOFF(GR_VERTEX_X_OFFSET, ECX) )
    MOVD      ( MM3, REGOFF(GR_VERTEX_OOZ_OFFSET, ECX) )


/* end of DO_SETUP_XYZ   */

    MOVD      ( MM7, REGOFF(GR_VERTEX_OOW_OFFSET, ECX) ) /* f[OOWCOORD] = oow */
    ADD_L     ( EAX, ECX )        /* f += stride */

    CMP_L     ( ECX, EDX )	/* stall??? */
    JA        ( TAGLLBL(FXPV_loop_start) )

TAGLLBL(FXPV_end):
    FEMMS
    POP_L     ( EBP )
    RET







/* void project_verts( GLfloat *first,
 *		       GLfloat *last,
 *		       const GLfloat *m,
 *		       GLuint stride,
 *                     const GLubyte *mask )
 *
 */

GLOBL GLNAME( TAG(fx_3dnow_project_clipped_vertices) )
GLNAME( TAG(fx_3dnow_project_clipped_vertices) ):

    PUSH_L    ( EBP )

    MOV_L     ( REGOFF(8, ESP), ECX ) /* first FXDRIVER(VB)->verts*/
    MOV_L     ( REGOFF(12, ESP), EDX ) /* last FXDRIVER(VB)->last_vert  */

    FEMMS

    PUSH_L    ( EDI )
    PUSH_L    ( ESI )

    PREFETCH  ( REGIND(ECX) )         /* fetch the first vertex */

    MOV_L     ( REGOFF(24, ESP), EBP ) /* mat ctx->Viewport.WindowMap.M */
    MOV_L     ( REGOFF(28, ESP), EAX )     /* stride */
    MOV_L     ( REGOFF(32, ESP), ESI ) /* VB->ClipMask       */

    MOVD      ( REGOFF(MAT_TX, EBP), MM6 )      /*             | tx           */
    PUNPCKLDQ ( REGOFF(MAT_TY, EBP), MM6 )      /*  ty         | tx           */

#if !defined(FX_V2)
    MOV_L     ( CONST(0x49400000), REGOFF(-8, ESP) )    /*  snapper           */
    MOV_L     ( CONST(0x49400000), REGOFF(-4, ESP) )    /*  snapper           */
#endif

    MOVQ      ( REGOFF(-8, ESP), MM4 )          /*  snapper    | snapper      */
    PFADD     ( MM4, MM6 )                      /*  ty+snapper | tx+snapper   */

    MOVD      ( REGIND(EBP), MM5 )
    PUNPCKLDQ ( REGOFF(MAT_SY, EBP), MM5 )      /*  vsy        | vsx          */

    MOVD      ( REGOFF(MAT_SZ, EBP), MM1 )      /*             | vsz          */



ALIGNTEXT32
TAGLLBL(FXPCV_loop_start):

    PREFETCH  ( REGOFF(64, ECX) )         /* fetch the next-ish vertex */

    CMP_B     ( CONST(0), REGIND(ESI) )
    JNE       ( TAGLLBL(FXPCV_skip) )

    MOVD      ( REGOFF(12, ECX), MM0)           /*              | f[3]        */
    PFRCP     ( MM0, MM0 )                      /*  oow = 1/f[3]              */

    MOVD      ( REGOFF(12, ECX), MM7)           /*              | f[3]        */
    PFRCPIT1  ( MM0, MM7 )
    PFRCPIT2  ( MM0, MM7 )                      /*  oow         | oow         */

    PUNPCKLDQ ( MM7, MM7 )


#if (TYPE & SETUP_RGBA)
    MOVD      ( REGOFF(CLIP_R, ECX ), MM0 )     /*  f[RCOORD] = f[CLIP_R];    */
    MOVD      ( MM0, REGOFF(GR_VERTEX_R_OFFSET, ECX) )
#endif

#if (TYPE & SETUP_TMU1)
    MOVQ      ( REGOFF(CLIP_S1, ECX), MM0 ) /* f[S1COORD] = f[CLIP_S1] * oow  */
    PFMUL     ( MM7, MM0 )                  /* f[T1COORD] = f[CLIP_T1] * oow  */
    MOVQ      ( MM0, REGOFF(GR_VERTEX_SOW_TMU1_OFFSET, ECX) )
#endif


#if (TYPE & SETUP_TMU0)
    MOVQ      ( REGOFF(CLIP_S0, ECX), MM0 ) /* f[S0COORD] = f[CLIP_S0] * oow  */
    PFMUL     ( MM7, MM0 )                  /* f[T0COORD] = f[CLIP_T0] * oow  */
    MOVQ      ( MM0, REGOFF(GR_VERTEX_SOW_TMU0_OFFSET, ECX) )
#endif




/*  DO_SETUP_XYZ */

    MOVQ      ( REGIND(ECX), MM2 )              /*  f[1]        | f[0]        */
    PFMUL     ( MM7, MM2 )                      /*  f[1] * oow  | f[0] * oow  */

    MOVD      ( REGOFF(8, ECX), MM3 )           /*              | f[2]        */
    PFMUL     ( MM7, MM3 )                      /*              | f[2] * oow  */

    MOVD      ( REGOFF(MAT_TZ, EBP), MM0 )      /*              | vtz         */
    PFMUL     ( MM1, MM3 )                      /*              | f[2] *= vsz */

    PFADD     ( MM0, MM3 )                      /*              | f[2] += vtz */
    PFMUL     ( MM5, MM2 )                      /*  f[1] *= vsy | f[0] *= vsx */

    PFADD     ( MM6, MM2 )                      /*  f[1] += vty | f[0] += vtx */

#if !defined(FX_V2)
    PFSUB     ( MM4, MM2 )                      /*  f[0,1] -= snapper         */
#endif

    MOVQ      ( MM2, REGOFF(GR_VERTEX_X_OFFSET, ECX) )
    MOVD      ( MM3, REGOFF(GR_VERTEX_OOZ_OFFSET, ECX) )


/* end of DO_SETUP_XYZ   */

    MOVD      ( MM7, REGOFF(GR_VERTEX_OOW_OFFSET, ECX) ) /* f[OOWCOORD] = oow */

TAGLLBL(FXPCV_skip):
    ADD_L     ( EAX, ECX )    /* f += stride     */

    INC_L     ( ESI )                           /*  next ClipMask             */
    CMP_L     ( ECX, EDX )
    JA        ( TAGLLBL(FXPCV_loop_start) )

    POP_L     ( ESI )
    POP_L     ( EDI )

TAGLLBL(FXPCV_end):
    FEMMS
    POP_L     ( EBP )
    RET



#undef TYPE
#undef TAG
#undef SIZE

