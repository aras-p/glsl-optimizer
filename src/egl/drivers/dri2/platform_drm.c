/*
 * Copyright © 2011 Intel Corporation
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <xf86drm.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIBUDEV
#include <libudev.h>
#endif

#include "egl_dri2.h"


#ifdef HAVE_LIBUDEV

struct dri2_driver_map {
   int vendor_id;
   const char *driver;
   const int *chip_ids;
   int num_chips_ids;
};

const int i915_chip_ids[] = {
   0x3577, /* PCI_CHIP_I830_M */
   0x2562, /* PCI_CHIP_845_G */
   0x3582, /* PCI_CHIP_I855_GM */
   0x2572, /* PCI_CHIP_I865_G */
   0x2582, /* PCI_CHIP_I915_G */
   0x258a, /* PCI_CHIP_E7221_G */
   0x2592, /* PCI_CHIP_I915_GM */
   0x2772, /* PCI_CHIP_I945_G */
   0x27a2, /* PCI_CHIP_I945_GM */
   0x27ae, /* PCI_CHIP_I945_GME */
   0x29b2, /* PCI_CHIP_Q35_G */
   0x29c2, /* PCI_CHIP_G33_G */
   0x29d2, /* PCI_CHIP_Q33_G */
   0xa001, /* PCI_CHIP_IGD_G */
   0xa011, /* Pineview */
};

const int i965_chip_ids[] = {
   0x0042, /* PCI_CHIP_ILD_G */
   0x0046, /* PCI_CHIP_ILM_G */
   0x0102, /* PCI_CHIP_SANDYBRIDGE_GT1 */
   0x0106, /* PCI_CHIP_SANDYBRIDGE_M_GT1 */
   0x010a, /* PCI_CHIP_SANDYBRIDGE_S */
   0x0112, /* PCI_CHIP_SANDYBRIDGE_GT2 */
   0x0116, /* PCI_CHIP_SANDYBRIDGE_M_GT2 */
   0x0122, /* PCI_CHIP_SANDYBRIDGE_GT2_PLUS */
   0x0126, /* PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS */
   0x0152, /* PCI_CHIP_IVYBRIDGE_GT1 */
   0x0162, /* PCI_CHIP_IVYBRIDGE_GT2 */
   0x0156, /* PCI_CHIP_IVYBRIDGE_M_GT1 */
   0x0166, /* PCI_CHIP_IVYBRIDGE_M_GT2 */
   0x015a, /* PCI_CHIP_IVYBRIDGE_S_GT1 */
   0x29a2, /* PCI_CHIP_I965_G */
   0x2992, /* PCI_CHIP_I965_Q */
   0x2982, /* PCI_CHIP_I965_G_1 */
   0x2972, /* PCI_CHIP_I946_GZ */
   0x2a02, /* PCI_CHIP_I965_GM */
   0x2a12, /* PCI_CHIP_I965_GME */
   0x2a42, /* PCI_CHIP_GM45_GM */
   0x2e02, /* PCI_CHIP_IGD_E_G */
   0x2e12, /* PCI_CHIP_Q45_G */
   0x2e22, /* PCI_CHIP_G45_G */
   0x2e32, /* PCI_CHIP_G41_G */
   0x2e42, /* PCI_CHIP_B43_G */
   0x2e92, /* PCI_CHIP_B43_G1 */
};

const int r100_chip_ids[] = {
   0x4C57, /* PCI_CHIP_RADEON_LW */
   0x4C58, /* PCI_CHIP_RADEON_LX */
   0x4C59, /* PCI_CHIP_RADEON_LY */
   0x4C5A, /* PCI_CHIP_RADEON_LZ */
   0x5144, /* PCI_CHIP_RADEON_QD */
   0x5145, /* PCI_CHIP_RADEON_QE */
   0x5146, /* PCI_CHIP_RADEON_QF */
   0x5147, /* PCI_CHIP_RADEON_QG */
   0x5159, /* PCI_CHIP_RADEON_QY */
   0x515A, /* PCI_CHIP_RADEON_QZ */
   0x5157, /* PCI_CHIP_RV200_QW */
   0x5158, /* PCI_CHIP_RV200_QX */
   0x515E, /* PCI_CHIP_RN50_515E */
   0x5969, /* PCI_CHIP_RN50_5969 */
   0x4136, /* PCI_CHIP_RS100_4136 */
   0x4336, /* PCI_CHIP_RS100_4336 */
   0x4137, /* PCI_CHIP_RS200_4137 */
   0x4337, /* PCI_CHIP_RS200_4337 */
   0x4237, /* PCI_CHIP_RS250_4237 */
   0x4437, /* PCI_CHIP_RS250_4437 */
};

const int r200_chip_ids[] = {
   0x5148, /* PCI_CHIP_R200_QH */
   0x514C, /* PCI_CHIP_R200_QL */
   0x514D, /* PCI_CHIP_R200_QM */
   0x4242, /* PCI_CHIP_R200_BB */
   0x4966, /* PCI_CHIP_RV250_If */
   0x4967, /* PCI_CHIP_RV250_Ig */
   0x4C64, /* PCI_CHIP_RV250_Ld */
   0x4C66, /* PCI_CHIP_RV250_Lf */
   0x4C67, /* PCI_CHIP_RV250_Lg */
   0x5960, /* PCI_CHIP_RV280_5960 */
   0x5961, /* PCI_CHIP_RV280_5961 */
   0x5962, /* PCI_CHIP_RV280_5962 */
   0x5964, /* PCI_CHIP_RV280_5964 */
   0x5965, /* PCI_CHIP_RV280_5965 */
   0x5C61, /* PCI_CHIP_RV280_5C61 */
   0x5C63, /* PCI_CHIP_RV280_5C63 */
   0x5834, /* PCI_CHIP_RS300_5834 */
   0x5835, /* PCI_CHIP_RS300_5835 */
   0x7834, /* PCI_CHIP_RS350_7834 */
   0x7835, /* PCI_CHIP_RS350_7835 */
};

const int r300_chip_ids[] = {
   0x4144, /* PCI_CHIP_R300_AD */
   0x4145, /* PCI_CHIP_R300_AE */
   0x4146, /* PCI_CHIP_R300_AF */
   0x4147, /* PCI_CHIP_R300_AG */
   0x4E44, /* PCI_CHIP_R300_ND */
   0x4E45, /* PCI_CHIP_R300_NE */
   0x4E46, /* PCI_CHIP_R300_NF */
   0x4E47, /* PCI_CHIP_R300_NG */
   0x4E48, /* PCI_CHIP_R350_NH */
   0x4E49, /* PCI_CHIP_R350_NI */
   0x4E4B, /* PCI_CHIP_R350_NK */
   0x4148, /* PCI_CHIP_R350_AH */
   0x4149, /* PCI_CHIP_R350_AI */
   0x414A, /* PCI_CHIP_R350_AJ */
   0x414B, /* PCI_CHIP_R350_AK */
   0x4E4A, /* PCI_CHIP_R360_NJ */
   0x4150, /* PCI_CHIP_RV350_AP */
   0x4151, /* PCI_CHIP_RV350_AQ */
   0x4152, /* PCI_CHIP_RV350_AR */
   0x4153, /* PCI_CHIP_RV350_AS */
   0x4154, /* PCI_CHIP_RV350_AT */
   0x4155, /* PCI_CHIP_RV350_AU */
   0x4156, /* PCI_CHIP_RV350_AV */
   0x4E50, /* PCI_CHIP_RV350_NP */
   0x4E51, /* PCI_CHIP_RV350_NQ */
   0x4E52, /* PCI_CHIP_RV350_NR */
   0x4E53, /* PCI_CHIP_RV350_NS */
   0x4E54, /* PCI_CHIP_RV350_NT */
   0x4E56, /* PCI_CHIP_RV350_NV */
   0x5460, /* PCI_CHIP_RV370_5460 */
   0x5462, /* PCI_CHIP_RV370_5462 */
   0x5464, /* PCI_CHIP_RV370_5464 */
   0x5B60, /* PCI_CHIP_RV370_5B60 */
   0x5B62, /* PCI_CHIP_RV370_5B62 */
   0x5B63, /* PCI_CHIP_RV370_5B63 */
   0x5B64, /* PCI_CHIP_RV370_5B64 */
   0x5B65, /* PCI_CHIP_RV370_5B65 */
   0x3150, /* PCI_CHIP_RV380_3150 */
   0x3152, /* PCI_CHIP_RV380_3152 */
   0x3154, /* PCI_CHIP_RV380_3154 */
   0x3155, /* PCI_CHIP_RV380_3155 */
   0x3E50, /* PCI_CHIP_RV380_3E50 */
   0x3E54, /* PCI_CHIP_RV380_3E54 */
   0x4A48, /* PCI_CHIP_R420_JH */
   0x4A49, /* PCI_CHIP_R420_JI */
   0x4A4A, /* PCI_CHIP_R420_JJ */
   0x4A4B, /* PCI_CHIP_R420_JK */
   0x4A4C, /* PCI_CHIP_R420_JL */
   0x4A4D, /* PCI_CHIP_R420_JM */
   0x4A4E, /* PCI_CHIP_R420_JN */
   0x4A4F, /* PCI_CHIP_R420_JO */
   0x4A50, /* PCI_CHIP_R420_JP */
   0x4A54, /* PCI_CHIP_R420_JT */
   0x5548, /* PCI_CHIP_R423_UH */
   0x5549, /* PCI_CHIP_R423_UI */
   0x554A, /* PCI_CHIP_R423_UJ */
   0x554B, /* PCI_CHIP_R423_UK */
   0x5550, /* PCI_CHIP_R423_5550 */
   0x5551, /* PCI_CHIP_R423_UQ */
   0x5552, /* PCI_CHIP_R423_UR */
   0x5554, /* PCI_CHIP_R423_UT */
   0x5D57, /* PCI_CHIP_R423_5D57 */
   0x554C, /* PCI_CHIP_R430_554C */
   0x554D, /* PCI_CHIP_R430_554D */
   0x554E, /* PCI_CHIP_R430_554E */
   0x554F, /* PCI_CHIP_R430_554F */
   0x5D48, /* PCI_CHIP_R430_5D48 */
   0x5D49, /* PCI_CHIP_R430_5D49 */
   0x5D4A, /* PCI_CHIP_R430_5D4A */
   0x5D4C, /* PCI_CHIP_R480_5D4C */
   0x5D4D, /* PCI_CHIP_R480_5D4D */
   0x5D4E, /* PCI_CHIP_R480_5D4E */
   0x5D4F, /* PCI_CHIP_R480_5D4F */
   0x5D50, /* PCI_CHIP_R480_5D50 */
   0x5D52, /* PCI_CHIP_R480_5D52 */
   0x4B49, /* PCI_CHIP_R481_4B49 */
   0x4B4A, /* PCI_CHIP_R481_4B4A */
   0x4B4B, /* PCI_CHIP_R481_4B4B */
   0x4B4C, /* PCI_CHIP_R481_4B4C */
   0x564A, /* PCI_CHIP_RV410_564A */
   0x564B, /* PCI_CHIP_RV410_564B */
   0x564F, /* PCI_CHIP_RV410_564F */
   0x5652, /* PCI_CHIP_RV410_5652 */
   0x5653, /* PCI_CHIP_RV410_5653 */
   0x5657, /* PCI_CHIP_RV410_5657 */
   0x5E48, /* PCI_CHIP_RV410_5E48 */
   0x5E4A, /* PCI_CHIP_RV410_5E4A */
   0x5E4B, /* PCI_CHIP_RV410_5E4B */
   0x5E4C, /* PCI_CHIP_RV410_5E4C */
   0x5E4D, /* PCI_CHIP_RV410_5E4D */
   0x5E4F, /* PCI_CHIP_RV410_5E4F */
   0x5A41, /* PCI_CHIP_RS400_5A41 */
   0x5A42, /* PCI_CHIP_RS400_5A42 */
   0x5A61, /* PCI_CHIP_RC410_5A61 */
   0x5A62, /* PCI_CHIP_RC410_5A62 */
   0x5954, /* PCI_CHIP_RS480_5954 */
   0x5955, /* PCI_CHIP_RS480_5955 */
   0x5974, /* PCI_CHIP_RS482_5974 */
   0x5975, /* PCI_CHIP_RS482_5975 */
   0x7100, /* PCI_CHIP_R520_7100 */
   0x7101, /* PCI_CHIP_R520_7101 */
   0x7102, /* PCI_CHIP_R520_7102 */
   0x7103, /* PCI_CHIP_R520_7103 */
   0x7104, /* PCI_CHIP_R520_7104 */
   0x7105, /* PCI_CHIP_R520_7105 */
   0x7106, /* PCI_CHIP_R520_7106 */
   0x7108, /* PCI_CHIP_R520_7108 */
   0x7109, /* PCI_CHIP_R520_7109 */
   0x710A, /* PCI_CHIP_R520_710A */
   0x710B, /* PCI_CHIP_R520_710B */
   0x710C, /* PCI_CHIP_R520_710C */
   0x710E, /* PCI_CHIP_R520_710E */
   0x710F, /* PCI_CHIP_R520_710F */
   0x7140, /* PCI_CHIP_RV515_7140 */
   0x7141, /* PCI_CHIP_RV515_7141 */
   0x7142, /* PCI_CHIP_RV515_7142 */
   0x7143, /* PCI_CHIP_RV515_7143 */
   0x7144, /* PCI_CHIP_RV515_7144 */
   0x7145, /* PCI_CHIP_RV515_7145 */
   0x7146, /* PCI_CHIP_RV515_7146 */
   0x7147, /* PCI_CHIP_RV515_7147 */
   0x7149, /* PCI_CHIP_RV515_7149 */
   0x714A, /* PCI_CHIP_RV515_714A */
   0x714B, /* PCI_CHIP_RV515_714B */
   0x714C, /* PCI_CHIP_RV515_714C */
   0x714D, /* PCI_CHIP_RV515_714D */
   0x714E, /* PCI_CHIP_RV515_714E */
   0x714F, /* PCI_CHIP_RV515_714F */
   0x7151, /* PCI_CHIP_RV515_7151 */
   0x7152, /* PCI_CHIP_RV515_7152 */
   0x7153, /* PCI_CHIP_RV515_7153 */
   0x715E, /* PCI_CHIP_RV515_715E */
   0x715F, /* PCI_CHIP_RV515_715F */
   0x7180, /* PCI_CHIP_RV515_7180 */
   0x7181, /* PCI_CHIP_RV515_7181 */
   0x7183, /* PCI_CHIP_RV515_7183 */
   0x7186, /* PCI_CHIP_RV515_7186 */
   0x7187, /* PCI_CHIP_RV515_7187 */
   0x7188, /* PCI_CHIP_RV515_7188 */
   0x718A, /* PCI_CHIP_RV515_718A */
   0x718B, /* PCI_CHIP_RV515_718B */
   0x718C, /* PCI_CHIP_RV515_718C */
   0x718D, /* PCI_CHIP_RV515_718D */
   0x718F, /* PCI_CHIP_RV515_718F */
   0x7193, /* PCI_CHIP_RV515_7193 */
   0x7196, /* PCI_CHIP_RV515_7196 */
   0x719B, /* PCI_CHIP_RV515_719B */
   0x719F, /* PCI_CHIP_RV515_719F */
   0x7200, /* PCI_CHIP_RV515_7200 */
   0x7210, /* PCI_CHIP_RV515_7210 */
   0x7211, /* PCI_CHIP_RV515_7211 */
   0x71C0, /* PCI_CHIP_RV530_71C0 */
   0x71C1, /* PCI_CHIP_RV530_71C1 */
   0x71C2, /* PCI_CHIP_RV530_71C2 */
   0x71C3, /* PCI_CHIP_RV530_71C3 */
   0x71C4, /* PCI_CHIP_RV530_71C4 */
   0x71C5, /* PCI_CHIP_RV530_71C5 */
   0x71C6, /* PCI_CHIP_RV530_71C6 */
   0x71C7, /* PCI_CHIP_RV530_71C7 */
   0x71CD, /* PCI_CHIP_RV530_71CD */
   0x71CE, /* PCI_CHIP_RV530_71CE */
   0x71D2, /* PCI_CHIP_RV530_71D2 */
   0x71D4, /* PCI_CHIP_RV530_71D4 */
   0x71D5, /* PCI_CHIP_RV530_71D5 */
   0x71D6, /* PCI_CHIP_RV530_71D6 */
   0x71DA, /* PCI_CHIP_RV530_71DA */
   0x71DE, /* PCI_CHIP_RV530_71DE */
   0x7281, /* PCI_CHIP_RV560_7281 */
   0x7283, /* PCI_CHIP_RV560_7283 */
   0x7287, /* PCI_CHIP_RV560_7287 */
   0x7290, /* PCI_CHIP_RV560_7290 */
   0x7291, /* PCI_CHIP_RV560_7291 */
   0x7293, /* PCI_CHIP_RV560_7293 */
   0x7297, /* PCI_CHIP_RV560_7297 */
   0x7280, /* PCI_CHIP_RV570_7280 */
   0x7288, /* PCI_CHIP_RV570_7288 */
   0x7289, /* PCI_CHIP_RV570_7289 */
   0x728B, /* PCI_CHIP_RV570_728B */
   0x728C, /* PCI_CHIP_RV570_728C */
   0x7240, /* PCI_CHIP_R580_7240 */
   0x7243, /* PCI_CHIP_R580_7243 */
   0x7244, /* PCI_CHIP_R580_7244 */
   0x7245, /* PCI_CHIP_R580_7245 */
   0x7246, /* PCI_CHIP_R580_7246 */
   0x7247, /* PCI_CHIP_R580_7247 */
   0x7248, /* PCI_CHIP_R580_7248 */
   0x7249, /* PCI_CHIP_R580_7249 */
   0x724A, /* PCI_CHIP_R580_724A */
   0x724B, /* PCI_CHIP_R580_724B */
   0x724C, /* PCI_CHIP_R580_724C */
   0x724D, /* PCI_CHIP_R580_724D */
   0x724E, /* PCI_CHIP_R580_724E */
   0x724F, /* PCI_CHIP_R580_724F */
   0x7284, /* PCI_CHIP_R580_7284 */
   0x793F, /* PCI_CHIP_RS600_793F */
   0x7941, /* PCI_CHIP_RS600_7941 */
   0x7942, /* PCI_CHIP_RS600_7942 */
   0x791E, /* PCI_CHIP_RS690_791E */
   0x791F, /* PCI_CHIP_RS690_791F */
   0x796C, /* PCI_CHIP_RS740_796C */
   0x796D, /* PCI_CHIP_RS740_796D */
   0x796E, /* PCI_CHIP_RS740_796E */
   0x796F, /* PCI_CHIP_RS740_796F */
};

const int r600_chip_ids[] = {
   0x9400, /* PCI_CHIP_R600_9400 */
   0x9401, /* PCI_CHIP_R600_9401 */
   0x9402, /* PCI_CHIP_R600_9402 */
   0x9403, /* PCI_CHIP_R600_9403 */
   0x9405, /* PCI_CHIP_R600_9405 */
   0x940A, /* PCI_CHIP_R600_940A */
   0x940B, /* PCI_CHIP_R600_940B */
   0x940F, /* PCI_CHIP_R600_940F */
   0x94C0, /* PCI_CHIP_RV610_94C0 */
   0x94C1, /* PCI_CHIP_RV610_94C1 */
   0x94C3, /* PCI_CHIP_RV610_94C3 */
   0x94C4, /* PCI_CHIP_RV610_94C4 */
   0x94C5, /* PCI_CHIP_RV610_94C5 */
   0x94C6, /* PCI_CHIP_RV610_94C6 */
   0x94C7, /* PCI_CHIP_RV610_94C7 */
   0x94C8, /* PCI_CHIP_RV610_94C8 */
   0x94C9, /* PCI_CHIP_RV610_94C9 */
   0x94CB, /* PCI_CHIP_RV610_94CB */
   0x94CC, /* PCI_CHIP_RV610_94CC */
   0x94CD, /* PCI_CHIP_RV610_94CD */
   0x9580, /* PCI_CHIP_RV630_9580 */
   0x9581, /* PCI_CHIP_RV630_9581 */
   0x9583, /* PCI_CHIP_RV630_9583 */
   0x9586, /* PCI_CHIP_RV630_9586 */
   0x9587, /* PCI_CHIP_RV630_9587 */
   0x9588, /* PCI_CHIP_RV630_9588 */
   0x9589, /* PCI_CHIP_RV630_9589 */
   0x958A, /* PCI_CHIP_RV630_958A */
   0x958B, /* PCI_CHIP_RV630_958B */
   0x958C, /* PCI_CHIP_RV630_958C */
   0x958D, /* PCI_CHIP_RV630_958D */
   0x958E, /* PCI_CHIP_RV630_958E */
   0x958F, /* PCI_CHIP_RV630_958F */
   0x9500, /* PCI_CHIP_RV670_9500 */
   0x9501, /* PCI_CHIP_RV670_9501 */
   0x9504, /* PCI_CHIP_RV670_9504 */
   0x9505, /* PCI_CHIP_RV670_9505 */
   0x9506, /* PCI_CHIP_RV670_9506 */
   0x9507, /* PCI_CHIP_RV670_9507 */
   0x9508, /* PCI_CHIP_RV670_9508 */
   0x9509, /* PCI_CHIP_RV670_9509 */
   0x950F, /* PCI_CHIP_RV670_950F */
   0x9511, /* PCI_CHIP_RV670_9511 */
   0x9515, /* PCI_CHIP_RV670_9515 */
   0x9517, /* PCI_CHIP_RV670_9517 */
   0x9519, /* PCI_CHIP_RV670_9519 */
   0x95C0, /* PCI_CHIP_RV620_95C0 */
   0x95C2, /* PCI_CHIP_RV620_95C2 */
   0x95C4, /* PCI_CHIP_RV620_95C4 */
   0x95C5, /* PCI_CHIP_RV620_95C5 */
   0x95C6, /* PCI_CHIP_RV620_95C6 */
   0x95C7, /* PCI_CHIP_RV620_95C7 */
   0x95C9, /* PCI_CHIP_RV620_95C9 */
   0x95CC, /* PCI_CHIP_RV620_95CC */
   0x95CD, /* PCI_CHIP_RV620_95CD */
   0x95CE, /* PCI_CHIP_RV620_95CE */
   0x95CF, /* PCI_CHIP_RV620_95CF */
   0x9590, /* PCI_CHIP_RV635_9590 */
   0x9591, /* PCI_CHIP_RV635_9591 */
   0x9593, /* PCI_CHIP_RV635_9593 */
   0x9595, /* PCI_CHIP_RV635_9595 */
   0x9596, /* PCI_CHIP_RV635_9596 */
   0x9597, /* PCI_CHIP_RV635_9597 */
   0x9598, /* PCI_CHIP_RV635_9598 */
   0x9599, /* PCI_CHIP_RV635_9599 */
   0x959B, /* PCI_CHIP_RV635_959B */
   0x9610, /* PCI_CHIP_RS780_9610 */
   0x9611, /* PCI_CHIP_RS780_9611 */
   0x9612, /* PCI_CHIP_RS780_9612 */
   0x9613, /* PCI_CHIP_RS780_9613 */
   0x9614, /* PCI_CHIP_RS780_9614 */
   0x9615, /* PCI_CHIP_RS780_9615 */
   0x9616, /* PCI_CHIP_RS780_9616 */
   0x9710, /* PCI_CHIP_RS880_9710 */
   0x9711, /* PCI_CHIP_RS880_9711 */
   0x9712, /* PCI_CHIP_RS880_9712 */
   0x9713, /* PCI_CHIP_RS880_9713 */
   0x9714, /* PCI_CHIP_RS880_9714 */
   0x9715, /* PCI_CHIP_RS880_9715 */
   0x9440, /* PCI_CHIP_RV770_9440 */
   0x9441, /* PCI_CHIP_RV770_9441 */
   0x9442, /* PCI_CHIP_RV770_9442 */
   0x9443, /* PCI_CHIP_RV770_9443 */
   0x9444, /* PCI_CHIP_RV770_9444 */
   0x9446, /* PCI_CHIP_RV770_9446 */
   0x944A, /* PCI_CHIP_RV770_944A */
   0x944B, /* PCI_CHIP_RV770_944B */
   0x944C, /* PCI_CHIP_RV770_944C */
   0x944E, /* PCI_CHIP_RV770_944E */
   0x9450, /* PCI_CHIP_RV770_9450 */
   0x9452, /* PCI_CHIP_RV770_9452 */
   0x9456, /* PCI_CHIP_RV770_9456 */
   0x945A, /* PCI_CHIP_RV770_945A */
   0x945B, /* PCI_CHIP_RV770_945B */
   0x945E, /* PCI_CHIP_RV770_945E */
   0x9460, /* PCI_CHIP_RV790_9460 */
   0x9462, /* PCI_CHIP_RV790_9462 */
   0x946A, /* PCI_CHIP_RV770_946A */
   0x946B, /* PCI_CHIP_RV770_946B */
   0x947A, /* PCI_CHIP_RV770_947A */
   0x947B, /* PCI_CHIP_RV770_947B */
   0x9480, /* PCI_CHIP_RV730_9480 */
   0x9487, /* PCI_CHIP_RV730_9487 */
   0x9488, /* PCI_CHIP_RV730_9488 */
   0x9489, /* PCI_CHIP_RV730_9489 */
   0x948A, /* PCI_CHIP_RV730_948A */
   0x948F, /* PCI_CHIP_RV730_948F */
   0x9490, /* PCI_CHIP_RV730_9490 */
   0x9491, /* PCI_CHIP_RV730_9491 */
   0x9495, /* PCI_CHIP_RV730_9495 */
   0x9498, /* PCI_CHIP_RV730_9498 */
   0x949C, /* PCI_CHIP_RV730_949C */
   0x949E, /* PCI_CHIP_RV730_949E */
   0x949F, /* PCI_CHIP_RV730_949F */
   0x9540, /* PCI_CHIP_RV710_9540 */
   0x9541, /* PCI_CHIP_RV710_9541 */
   0x9542, /* PCI_CHIP_RV710_9542 */
   0x954E, /* PCI_CHIP_RV710_954E */
   0x954F, /* PCI_CHIP_RV710_954F */
   0x9552, /* PCI_CHIP_RV710_9552 */
   0x9553, /* PCI_CHIP_RV710_9553 */
   0x9555, /* PCI_CHIP_RV710_9555 */
   0x9557, /* PCI_CHIP_RV710_9557 */
   0x955F, /* PCI_CHIP_RV710_955F */
   0x94A0, /* PCI_CHIP_RV740_94A0 */
   0x94A1, /* PCI_CHIP_RV740_94A1 */
   0x94A3, /* PCI_CHIP_RV740_94A3 */
   0x94B1, /* PCI_CHIP_RV740_94B1 */
   0x94B3, /* PCI_CHIP_RV740_94B3 */
   0x94B4, /* PCI_CHIP_RV740_94B4 */
   0x94B5, /* PCI_CHIP_RV740_94B5 */
   0x94B9, /* PCI_CHIP_RV740_94B9 */
   0x68E0, /* PCI_CHIP_CEDAR_68E0 */
   0x68E1, /* PCI_CHIP_CEDAR_68E1 */
   0x68E4, /* PCI_CHIP_CEDAR_68E4 */
   0x68E5, /* PCI_CHIP_CEDAR_68E5 */
   0x68E8, /* PCI_CHIP_CEDAR_68E8 */
   0x68E9, /* PCI_CHIP_CEDAR_68E9 */
   0x68F1, /* PCI_CHIP_CEDAR_68F1 */
   0x68F2, /* PCI_CHIP_CEDAR_68F2 */
   0x68F8, /* PCI_CHIP_CEDAR_68F8 */
   0x68F9, /* PCI_CHIP_CEDAR_68F9 */
   0x68FE, /* PCI_CHIP_CEDAR_68FE */
   0x68C0, /* PCI_CHIP_REDWOOD_68C0 */
   0x68C1, /* PCI_CHIP_REDWOOD_68C1 */
   0x68C8, /* PCI_CHIP_REDWOOD_68C8 */
   0x68C9, /* PCI_CHIP_REDWOOD_68C9 */
   0x68D8, /* PCI_CHIP_REDWOOD_68D8 */
   0x68D9, /* PCI_CHIP_REDWOOD_68D9 */
   0x68DA, /* PCI_CHIP_REDWOOD_68DA */
   0x68DE, /* PCI_CHIP_REDWOOD_68DE */
   0x68A0, /* PCI_CHIP_JUNIPER_68A0 */
   0x68A1, /* PCI_CHIP_JUNIPER_68A1 */
   0x68A8, /* PCI_CHIP_JUNIPER_68A8 */
   0x68A9, /* PCI_CHIP_JUNIPER_68A9 */
   0x68B0, /* PCI_CHIP_JUNIPER_68B0 */
   0x68B8, /* PCI_CHIP_JUNIPER_68B8 */
   0x68B9, /* PCI_CHIP_JUNIPER_68B9 */
   0x68BA, /* PCI_CHIP_JUNIPER_68BA */
   0x68BE, /* PCI_CHIP_JUNIPER_68BE */
   0x68BF, /* PCI_CHIP_JUNIPER_68BF */
   0x6880, /* PCI_CHIP_CYPRESS_6880 */
   0x6888, /* PCI_CHIP_CYPRESS_6888 */
   0x6889, /* PCI_CHIP_CYPRESS_6889 */
   0x688A, /* PCI_CHIP_CYPRESS_688A */
   0x6898, /* PCI_CHIP_CYPRESS_6898 */
   0x6899, /* PCI_CHIP_CYPRESS_6899 */
   0x689B, /* PCI_CHIP_CYPRESS_689B */
   0x689E, /* PCI_CHIP_CYPRESS_689E */
   0x689C, /* PCI_CHIP_HEMLOCK_689C */
   0x689D, /* PCI_CHIP_HEMLOCK_689D */
   0x9802, /* PCI_CHIP_PALM_9802 */
   0x9803, /* PCI_CHIP_PALM_9803 */
   0x9804, /* PCI_CHIP_PALM_9804 */
   0x9805, /* PCI_CHIP_PALM_9805 */
   0x9806, /* PCI_CHIP_PALM_9806 */
   0x9807, /* PCI_CHIP_PALM_9807 */
   0x6700, /* PCI_CHIP_CAYMAN_6700 */
   0x6701, /* PCI_CHIP_CAYMAN_6701 */
   0x6702, /* PCI_CHIP_CAYMAN_6702 */
   0x6703, /* PCI_CHIP_CAYMAN_6703 */
   0x6704, /* PCI_CHIP_CAYMAN_6704 */
   0x6705, /* PCI_CHIP_CAYMAN_6705 */
   0x6706, /* PCI_CHIP_CAYMAN_6706 */
   0x6707, /* PCI_CHIP_CAYMAN_6707 */
   0x6708, /* PCI_CHIP_CAYMAN_6708 */
   0x6709, /* PCI_CHIP_CAYMAN_6709 */
   0x6718, /* PCI_CHIP_CAYMAN_6718 */
   0x6719, /* PCI_CHIP_CAYMAN_6719 */
   0x671C, /* PCI_CHIP_CAYMAN_671C */
   0x671D, /* PCI_CHIP_CAYMAN_671D */
   0x671F, /* PCI_CHIP_CAYMAN_671F */
   0x6720, /* PCI_CHIP_BARTS_6720 */
   0x6721, /* PCI_CHIP_BARTS_6721 */
   0x6722, /* PCI_CHIP_BARTS_6722 */
   0x6723, /* PCI_CHIP_BARTS_6723 */
   0x6724, /* PCI_CHIP_BARTS_6724 */
   0x6725, /* PCI_CHIP_BARTS_6725 */
   0x6726, /* PCI_CHIP_BARTS_6726 */
   0x6727, /* PCI_CHIP_BARTS_6727 */
   0x6728, /* PCI_CHIP_BARTS_6728 */
   0x6729, /* PCI_CHIP_BARTS_6729 */
   0x6738, /* PCI_CHIP_BARTS_6738 */
   0x6739, /* PCI_CHIP_BARTS_6738 */
   0x673E, /* PCI_CHIP_BARTS_673E */
   0x6740, /* PCI_CHIP_TURKS_6740 */
   0x6741, /* PCI_CHIP_TURKS_6741 */
   0x6742, /* PCI_CHIP_TURKS_6742 */
   0x6743, /* PCI_CHIP_TURKS_6743 */
   0x6744, /* PCI_CHIP_TURKS_6744 */
   0x6745, /* PCI_CHIP_TURKS_6745 */
   0x6746, /* PCI_CHIP_TURKS_6746 */
   0x6747, /* PCI_CHIP_TURKS_6747 */
   0x6748, /* PCI_CHIP_TURKS_6748 */
   0x6749, /* PCI_CHIP_TURKS_6749 */
   0x6750, /* PCI_CHIP_TURKS_6750 */
   0x6758, /* PCI_CHIP_TURKS_6758 */
   0x6759, /* PCI_CHIP_TURKS_6759 */
   0x6760, /* PCI_CHIP_CAICOS_6760 */
   0x6761, /* PCI_CHIP_CAICOS_6761 */
   0x6762, /* PCI_CHIP_CAICOS_6762 */
   0x6763, /* PCI_CHIP_CAICOS_6763 */
   0x6764, /* PCI_CHIP_CAICOS_6764 */
   0x6765, /* PCI_CHIP_CAICOS_6765 */
   0x6766, /* PCI_CHIP_CAICOS_6766 */
   0x6767, /* PCI_CHIP_CAICOS_6767 */
   0x6768, /* PCI_CHIP_CAICOS_6768 */
   0x6770, /* PCI_CHIP_CAICOS_6770 */
   0x6779, /* PCI_CHIP_CAICOS_6779 */
};

const struct dri2_driver_map driver_map[] = {
   { 0x8086, "i915", i915_chip_ids, ARRAY_SIZE(i915_chip_ids) },
   { 0x8086, "i965", i965_chip_ids, ARRAY_SIZE(i965_chip_ids) },
   { 0x1002, "radeon", r100_chip_ids, ARRAY_SIZE(r100_chip_ids) },
   { 0x1002, "r200", r200_chip_ids, ARRAY_SIZE(r200_chip_ids) },
   { 0x1002, "r300", r300_chip_ids, ARRAY_SIZE(r300_chip_ids) },
   { 0x1002, "r600", r600_chip_ids, ARRAY_SIZE(r600_chip_ids) },
   { 0x10de, "nouveau", NULL, -1 },
};

static char *
dri2_get_device_name(int fd)
{
   struct udev *udev;
   struct udev_device *device;
   struct stat buf;
   const char *const_device_name;
   char *device_name = NULL;

   udev = udev_new();
   if (fstat(fd, &buf) < 0) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: failed to stat fd %d", fd);
      goto out;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      _eglLog(_EGL_WARNING,
	      "EGL-DRI2: could not create udev device for fd %d", fd);
      goto out;
   }

   const_device_name = udev_device_get_devnode(device);
   if (!const_device_name) {
      goto out;
   }
   device_name = strdup(const_device_name);

 out:
   udev_device_unref(device);
   udev_unref(udev);

   return device_name;
}

char *
dri2_get_driver_for_fd(int fd)
{
   struct udev *udev;
   struct udev_device *device, *parent;
   struct stat buf;
   const char *pci_id;
   char *driver = NULL;
   int vendor_id, chip_id, i, j;

   udev = udev_new();
   if (fstat(fd, &buf) < 0) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: failed to stat fd %d", fd);
      goto out;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      _eglLog(_EGL_WARNING,
	      "EGL-DRI2: could not create udev device for fd %d", fd);
      goto out;
   }

   parent = udev_device_get_parent(device);
   if (parent == NULL) {
      _eglLog(_EGL_WARNING, "DRI2: could not get parent device");
      goto out;
   }

   pci_id = udev_device_get_property_value(parent, "PCI_ID");
   if (pci_id == NULL || sscanf(pci_id, "%x:%x", &vendor_id, &chip_id) != 2) {
      _eglLog(_EGL_WARNING, "EGL-DRI2: malformed or no PCI ID");
      goto out;
   }

   for (i = 0; i < ARRAY_SIZE(driver_map); i++) {
      if (vendor_id != driver_map[i].vendor_id)
	 continue;
      if (driver_map[i].num_chips_ids == -1) {
	    driver = strdup(driver_map[i].driver);
	    _eglLog(_EGL_DEBUG, "pci id for %d: %04x:%04x, driver %s",
		    fd, vendor_id, chip_id, driver);
	    goto out;
      }

      for (j = 0; j < driver_map[i].num_chips_ids; j++)
	 if (driver_map[i].chip_ids[j] == chip_id) {
	    driver = strdup(driver_map[i].driver);
	    _eglLog(_EGL_DEBUG, "pci id for %d: %04x:%04x, driver %s",
		    fd, vendor_id, chip_id, driver);
	    goto out;
	 }
   }

 out:
   udev_device_unref(device);
   udev_unref(udev);

   return driver;
}

static int
dri2_drm_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   return drmAuthMagic(dri2_dpy->fd, id);
}

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   int i;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");
   
   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;
   dri2_dpy->fd = (int) (intptr_t) disp->PlatformDisplay;

   dri2_dpy->driver_name = dri2_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL)
      return _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");

   dri2_dpy->device_name = dri2_get_device_name(dri2_dpy->fd);
   if (dri2_dpy->device_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get device name");
      goto cleanup_driver_name;
   }

   if (!dri2_load_driver(disp))
      goto cleanup_device_name;

   dri2_dpy->extensions[0] = &image_lookup_extension.base;
   dri2_dpy->extensions[1] = &use_invalidate.base;
   dri2_dpy->extensions[2] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_driver;

   for (i = 0; dri2_dpy->driver_configs[i]; i++)
      dri2_add_config(disp, dri2_dpy->driver_configs[i], i + 1, 0, 0, NULL);

#ifdef HAVE_WAYLAND_PLATFORM
   disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
#endif
   dri2_dpy->authenticate = dri2_drm_authenticate;
   
   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_device_name:
   free(dri2_dpy->device_name);
 cleanup_driver_name:
   free(dri2_dpy->driver_name);

   return EGL_FALSE;
}

#endif
