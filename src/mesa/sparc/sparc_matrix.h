/* $Id: sparc_matrix.h,v 1.1 2001/05/23 14:27:03 brianp Exp $ */

#define M0		%f16
#define M1		%f17
#define M2		%f18
#define M3		%f19
#define M4		%f20
#define M5		%f21
#define M6		%f22
#define M7		%f23
#define M8		%f24
#define M9		%f25
#define M10		%f26
#define M11		%f27
#define M12		%f28
#define M13		%f29
#define M14		%f30
#define M15		%f31

/* Seems to work, disable if unaligned traps begin to appear... -DaveM */
#define USE_LD_DOUBLE

#ifndef USE_LD_DOUBLE

#define LDMATRIX_0_1_2_3_12_13_14_15(BASE)	\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ld	[BASE + ( 3 * 0x4)], M3;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14;	\
	ld	[BASE + (15 * 0x4)], M15

#define LDMATRIX_0_1_12_13(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13

#define LDMATRIX_0_12_13(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13

#define LDMATRIX_0_1_2_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_14(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_2_3_4_5_6_7_12_13_14_15(BASE) \
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ld	[BASE + ( 3 * 0x4)], M3;	\
	ld	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ld	[BASE + ( 7 * 0x4)], M7;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14;	\
	ld	[BASE + (15 * 0x4)], M15

#define LDMATRIX_0_1_4_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13

#define LDMATRIX_0_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13

#define LDMATRIX_0_1_2_3_4_5_6_12_13_14(BASE)	\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ld	[BASE + ( 3 * 0x4)], M3;	\
	ld	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_14(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15(BASE) \
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ld	[BASE + ( 3 * 0x4)], M3;	\
	ld	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ld	[BASE + ( 7 * 0x4)], M7;	\
	ld	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + ( 9 * 0x4)], M9;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ld	[BASE + (11 * 0x4)], M11;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14;	\
	ld	[BASE + (15 * 0x4)], M15

#define LDMATRIX_0_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13

#define LDMATRIX_0_1_2_4_5_6_8_9_10_12_13_14(BASE) \
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 1 * 0x4)], M1;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ld	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ld	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + ( 9 * 0x4)], M9;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_10_12_13_14(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ld	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (13 * 0x4)], M13;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_8_9_10_14(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + ( 9 * 0x4)], M9;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ld	[BASE + (14 * 0x4)], M14

#else /* !(USE_LD_DOUBLE) */

#define LDMATRIX_0_1_2_3_12_13_14_15(BASE)	\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ldd	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_12_13(BASE)		\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_12_13(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_1_2_12_13_14(BASE)		\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_14(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_2_3_4_5_6_7_12_13_14_15(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ldd	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ldd	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_1_2_3_4_5_6_12_13_14(BASE)	\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_14(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ldd	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ldd	[BASE + (10 * 0x4)], M10;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ldd	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_4_5_12_13(BASE) 		\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_1_2_4_5_6_8_9_10_12_13_14(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_10_12_13_14(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_8_9_10_14(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ld	[BASE + (14 * 0x4)], M14

#endif /* USE_LD_DOUBLE */
