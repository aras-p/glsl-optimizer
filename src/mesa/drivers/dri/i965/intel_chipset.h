 /*
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#pragma once
#include <stdbool.h>

#define PCI_CHIP_IGD_GM			0xA011
#define PCI_CHIP_IGD_G			0xA001

#define IS_IGDGM(devid)	(devid == PCI_CHIP_IGD_GM)
#define IS_IGDG(devid)	(devid == PCI_CHIP_IGD_G)
#define IS_IGD(devid) (IS_IGDG(devid) || IS_IGDGM(devid))

#define PCI_CHIP_I965_G			0x29A2
#define PCI_CHIP_I965_Q			0x2992
#define PCI_CHIP_I965_G_1		0x2982
#define PCI_CHIP_I946_GZ		0x2972
#define PCI_CHIP_I965_GM                0x2A02
#define PCI_CHIP_I965_GME               0x2A12

#define PCI_CHIP_GM45_GM                0x2A42

#define PCI_CHIP_IGD_E_G                0x2E02
#define PCI_CHIP_Q45_G                  0x2E12
#define PCI_CHIP_G45_G                  0x2E22
#define PCI_CHIP_G41_G                  0x2E32
#define PCI_CHIP_B43_G                  0x2E42
#define PCI_CHIP_B43_G1                 0x2E92

#define PCI_CHIP_ILD_G                  0x0042
#define PCI_CHIP_ILM_G                  0x0046

#define PCI_CHIP_SANDYBRIDGE_GT1	0x0102	/* Desktop */
#define PCI_CHIP_SANDYBRIDGE_GT2	0x0112
#define PCI_CHIP_SANDYBRIDGE_GT2_PLUS	0x0122
#define PCI_CHIP_SANDYBRIDGE_M_GT1	0x0106	/* Mobile */
#define PCI_CHIP_SANDYBRIDGE_M_GT2	0x0116
#define PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS	0x0126
#define PCI_CHIP_SANDYBRIDGE_S		0x010A	/* Server */

#define PCI_CHIP_IVYBRIDGE_GT1          0x0152  /* Desktop */
#define PCI_CHIP_IVYBRIDGE_GT2          0x0162
#define PCI_CHIP_IVYBRIDGE_M_GT1        0x0156  /* Mobile */
#define PCI_CHIP_IVYBRIDGE_M_GT2        0x0166
#define PCI_CHIP_IVYBRIDGE_S_GT1        0x015a  /* Server */
#define PCI_CHIP_IVYBRIDGE_S_GT2        0x016a

#define PCI_CHIP_BAYTRAIL_M_1           0x0F31
#define PCI_CHIP_BAYTRAIL_M_2           0x0F32
#define PCI_CHIP_BAYTRAIL_M_3           0x0F33
#define PCI_CHIP_BAYTRAIL_M_4           0x0157
#define PCI_CHIP_BAYTRAIL_D             0x0155

#define PCI_CHIP_HASWELL_GT1            0x0402 /* Desktop */
#define PCI_CHIP_HASWELL_GT2            0x0412
#define PCI_CHIP_HASWELL_GT3            0x0422
#define PCI_CHIP_HASWELL_M_GT1          0x0406 /* Mobile */
#define PCI_CHIP_HASWELL_M_GT2          0x0416
#define PCI_CHIP_HASWELL_M_GT3          0x0426
#define PCI_CHIP_HASWELL_S_GT1          0x040A /* Server */
#define PCI_CHIP_HASWELL_S_GT2          0x041A
#define PCI_CHIP_HASWELL_S_GT3          0x042A
#define PCI_CHIP_HASWELL_B_GT1          0x040B /* Reserved */
#define PCI_CHIP_HASWELL_B_GT2          0x041B
#define PCI_CHIP_HASWELL_B_GT3          0x042B
#define PCI_CHIP_HASWELL_E_GT1          0x040E /* Reserved */
#define PCI_CHIP_HASWELL_E_GT2          0x041E
#define PCI_CHIP_HASWELL_E_GT3          0x042E
#define PCI_CHIP_HASWELL_SDV_GT1        0x0C02 /* Desktop */
#define PCI_CHIP_HASWELL_SDV_GT2        0x0C12
#define PCI_CHIP_HASWELL_SDV_GT3        0x0C22
#define PCI_CHIP_HASWELL_SDV_M_GT1      0x0C06 /* Mobile */
#define PCI_CHIP_HASWELL_SDV_M_GT2      0x0C16
#define PCI_CHIP_HASWELL_SDV_M_GT3      0x0C26
#define PCI_CHIP_HASWELL_SDV_S_GT1      0x0C0A /* Server */
#define PCI_CHIP_HASWELL_SDV_S_GT2      0x0C1A
#define PCI_CHIP_HASWELL_SDV_S_GT3      0x0C2A
#define PCI_CHIP_HASWELL_SDV_B_GT1      0x0C0B /* Reserved */
#define PCI_CHIP_HASWELL_SDV_B_GT2      0x0C1B
#define PCI_CHIP_HASWELL_SDV_B_GT3      0x0C2B
#define PCI_CHIP_HASWELL_SDV_E_GT1      0x0C0E /* Reserved */
#define PCI_CHIP_HASWELL_SDV_E_GT2      0x0C1E
#define PCI_CHIP_HASWELL_SDV_E_GT3      0x0C2E
#define PCI_CHIP_HASWELL_ULT_GT1        0x0A02 /* Desktop */
#define PCI_CHIP_HASWELL_ULT_GT2        0x0A12
#define PCI_CHIP_HASWELL_ULT_GT3        0x0A22
#define PCI_CHIP_HASWELL_ULT_M_GT1      0x0A06 /* Mobile */
#define PCI_CHIP_HASWELL_ULT_M_GT2      0x0A16
#define PCI_CHIP_HASWELL_ULT_M_GT3      0x0A26
#define PCI_CHIP_HASWELL_ULT_S_GT1      0x0A0A /* Server */
#define PCI_CHIP_HASWELL_ULT_S_GT2      0x0A1A
#define PCI_CHIP_HASWELL_ULT_S_GT3      0x0A2A
#define PCI_CHIP_HASWELL_ULT_B_GT1      0x0A0B /* Reserved */
#define PCI_CHIP_HASWELL_ULT_B_GT2      0x0A1B
#define PCI_CHIP_HASWELL_ULT_B_GT3      0x0A2B
#define PCI_CHIP_HASWELL_ULT_E_GT1      0x0A0E /* Reserved */
#define PCI_CHIP_HASWELL_ULT_E_GT2      0x0A1E
#define PCI_CHIP_HASWELL_ULT_E_GT3      0x0A2E
#define PCI_CHIP_HASWELL_CRW_GT1        0x0D02 /* Desktop */
#define PCI_CHIP_HASWELL_CRW_GT2        0x0D12
#define PCI_CHIP_HASWELL_CRW_GT3        0x0D22
#define PCI_CHIP_HASWELL_CRW_M_GT1      0x0D06 /* Mobile */
#define PCI_CHIP_HASWELL_CRW_M_GT2      0x0D16
#define PCI_CHIP_HASWELL_CRW_M_GT3      0x0D26
#define PCI_CHIP_HASWELL_CRW_S_GT1      0x0D0A /* Server */
#define PCI_CHIP_HASWELL_CRW_S_GT2      0x0D1A
#define PCI_CHIP_HASWELL_CRW_S_GT3      0x0D2A
#define PCI_CHIP_HASWELL_CRW_B_GT1      0x0D0B /* Reserved */
#define PCI_CHIP_HASWELL_CRW_B_GT2      0x0D1B
#define PCI_CHIP_HASWELL_CRW_B_GT3      0x0D2B
#define PCI_CHIP_HASWELL_CRW_E_GT1      0x0D0E /* Reserved */
#define PCI_CHIP_HASWELL_CRW_E_GT2      0x0D1E
#define PCI_CHIP_HASWELL_CRW_E_GT3      0x0D2E

#define IS_G45(devid)           (devid == PCI_CHIP_IGD_E_G || \
                                 devid == PCI_CHIP_Q45_G || \
                                 devid == PCI_CHIP_G45_G || \
                                 devid == PCI_CHIP_G41_G || \
                                 devid == PCI_CHIP_B43_G || \
                                 devid == PCI_CHIP_B43_G1)
#define IS_GM45(devid)          (devid == PCI_CHIP_GM45_GM)
#define IS_G4X(devid)		(IS_G45(devid) || IS_GM45(devid))

#define IS_ILD(devid)           (devid == PCI_CHIP_ILD_G)
#define IS_ILM(devid)           (devid == PCI_CHIP_ILM_G)
#define IS_GEN5(devid)          (IS_ILD(devid) || IS_ILM(devid))

#define IS_SNB_GT1(devid)	(devid == PCI_CHIP_SANDYBRIDGE_GT1 || \
				 devid == PCI_CHIP_SANDYBRIDGE_M_GT1 || \
				 devid == PCI_CHIP_SANDYBRIDGE_S)

#define IS_SNB_GT2(devid)	(devid == PCI_CHIP_SANDYBRIDGE_GT2 || \
				 devid == PCI_CHIP_SANDYBRIDGE_GT2_PLUS	|| \
				 devid == PCI_CHIP_SANDYBRIDGE_M_GT2 || \
				 devid == PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS)

#define IS_GEN6(devid)		(IS_SNB_GT1(devid) || IS_SNB_GT2(devid))

#define IS_IVB_GT1(devid)       (devid == PCI_CHIP_IVYBRIDGE_GT1 || \
				 devid == PCI_CHIP_IVYBRIDGE_M_GT1 || \
				 devid == PCI_CHIP_IVYBRIDGE_S_GT1)

#define IS_IVB_GT2(devid)       (devid == PCI_CHIP_IVYBRIDGE_GT2 || \
				 devid == PCI_CHIP_IVYBRIDGE_M_GT2 || \
				 devid == PCI_CHIP_IVYBRIDGE_S_GT2)

#define IS_IVYBRIDGE(devid)     (IS_IVB_GT1(devid) || IS_IVB_GT2(devid))

#define IS_BAYTRAIL(devid)      (devid == PCI_CHIP_BAYTRAIL_M_1 || \
                                 devid == PCI_CHIP_BAYTRAIL_M_2 || \
                                 devid == PCI_CHIP_BAYTRAIL_M_3 || \
                                 devid == PCI_CHIP_BAYTRAIL_M_4 || \
                                 devid == PCI_CHIP_BAYTRAIL_D)

#define IS_GEN7(devid)	        (IS_IVYBRIDGE(devid) || \
				 IS_BAYTRAIL(devid) || \
				 IS_HASWELL(devid))

#define IS_HSW_GT1(devid)	(devid == PCI_CHIP_HASWELL_GT1 || \
				 devid == PCI_CHIP_HASWELL_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_B_GT1 || \
				 devid == PCI_CHIP_HASWELL_E_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_B_GT1 || \
				 devid == PCI_CHIP_HASWELL_SDV_E_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_B_GT1 || \
				 devid == PCI_CHIP_HASWELL_ULT_E_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_M_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_S_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_B_GT1 || \
				 devid == PCI_CHIP_HASWELL_CRW_E_GT1)
#define IS_HSW_GT2(devid)	(devid == PCI_CHIP_HASWELL_GT2 || \
				 devid == PCI_CHIP_HASWELL_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_E_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_SDV_E_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_ULT_E_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_M_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_S_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_B_GT2 || \
				 devid == PCI_CHIP_HASWELL_CRW_E_GT2)
#define IS_HSW_GT3(devid)	(devid == PCI_CHIP_HASWELL_GT3 || \
				 devid == PCI_CHIP_HASWELL_M_GT3 || \
				 devid == PCI_CHIP_HASWELL_S_GT3 || \
				 devid == PCI_CHIP_HASWELL_B_GT3 || \
				 devid == PCI_CHIP_HASWELL_E_GT3 || \
				 devid == PCI_CHIP_HASWELL_SDV_GT3 || \
				 devid == PCI_CHIP_HASWELL_SDV_M_GT3 || \
				 devid == PCI_CHIP_HASWELL_SDV_S_GT3 || \
				 devid == PCI_CHIP_HASWELL_SDV_B_GT3 || \
				 devid == PCI_CHIP_HASWELL_SDV_E_GT3 || \
				 devid == PCI_CHIP_HASWELL_ULT_GT3 || \
				 devid == PCI_CHIP_HASWELL_ULT_M_GT3 || \
				 devid == PCI_CHIP_HASWELL_ULT_S_GT3 || \
				 devid == PCI_CHIP_HASWELL_ULT_B_GT3 || \
				 devid == PCI_CHIP_HASWELL_ULT_E_GT3 || \
				 devid == PCI_CHIP_HASWELL_CRW_GT3 || \
				 devid == PCI_CHIP_HASWELL_CRW_M_GT3 || \
				 devid == PCI_CHIP_HASWELL_CRW_S_GT3 || \
				 devid == PCI_CHIP_HASWELL_CRW_B_GT3 || \
				 devid == PCI_CHIP_HASWELL_CRW_E_GT3)

#define IS_HASWELL(devid)       (IS_HSW_GT1(devid) || \
				 IS_HSW_GT2(devid) || \
				 IS_HSW_GT3(devid))
