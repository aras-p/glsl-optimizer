#ifndef _RADEON_CHIPSET_H
#define _RADEON_CHIPSET_H
/* Including xf86PciInfo.h introduces a bunch of errors...
 */

/* General chip classes:
 * r100 includes R100, RV100, RV200, RS100, RS200, RS250.
 * r200 includes R200, RV250, RV280, RS300.
 * r300 includes R300, RV350, RV370.
 * (RS* denotes IGP)
 */
#define PCI_CHIP_RS100_4136		0x4136
#define PCI_CHIP_RS200_4137		0x4137
#define PCI_CHIP_R300_AD		0x4144
#define PCI_CHIP_R300_AE		0x4145
#define PCI_CHIP_R300_AF		0x4146
#define PCI_CHIP_R300_AG		0x4147
#define PCI_CHIP_R350_AH                0x4148
#define PCI_CHIP_R350_AI                0x4149
#define PCI_CHIP_R350_AJ                0x414A
#define PCI_CHIP_R350_AK                0x414B
#define PCI_CHIP_RV350_AP               0x4150
#define PCI_CHIP_RV350_AQ               0x4151
#define PCI_CHIP_RV350_AR               0x4152
#define PCI_CHIP_RV350_AS               0x4153
#define PCI_CHIP_RV350_AT               0x4154
#define PCI_CHIP_RV350_AU             0x4155
#define PCI_CHIP_RV350_AV               0x4156
#define PCI_CHIP_RS250_4237		0x4237
#define PCI_CHIP_R200_BB		0x4242
#define PCI_CHIP_R200_BC		0x4243
#define PCI_CHIP_RS100_4336		0x4336
#define PCI_CHIP_RS200_4337		0x4337
#define PCI_CHIP_RS250_4437		0x4437
#define PCI_CHIP_RV250_Id		0x4964
#define PCI_CHIP_RV250_Ie		0x4965
#define PCI_CHIP_RV250_If		0x4966
#define PCI_CHIP_RV250_Ig		0x4967
#define PCI_CHIP_RV410_5E4B             0x5E4B
#define PCI_CHIP_R420_JH                0x4A48
#define PCI_CHIP_R420_JI                0x4A49
#define PCI_CHIP_R420_JJ                0x4A4A
#define PCI_CHIP_R420_JK                0x4A4B
#define PCI_CHIP_R420_JL                0x4A4C
#define PCI_CHIP_R420_JM                0x4A4D
#define PCI_CHIP_R420_JN                0x4A4E
#define PCI_CHIP_R420_JO		0x4A4F
#define PCI_CHIP_R420_JP                0x4A50
#define PCI_CHIP_RADEON_LW		0x4C57
#define PCI_CHIP_RADEON_LX		0x4C58
#define PCI_CHIP_RADEON_LY		0x4C59
#define PCI_CHIP_RADEON_LZ		0x4C5A
#define PCI_CHIP_RV250_Ld		0x4C64
#define PCI_CHIP_RV250_Le		0x4C65
#define PCI_CHIP_RV250_Lf		0x4C66
#define PCI_CHIP_RV250_Lg		0x4C67
#define PCI_CHIP_RV250_Ln		0x4C6E
#define PCI_CHIP_R300_ND		0x4E44
#define PCI_CHIP_R300_NE		0x4E45
#define PCI_CHIP_R300_NF		0x4E46
#define PCI_CHIP_R300_NG		0x4E47
#define PCI_CHIP_R350_NH                0x4E48
#define PCI_CHIP_R350_NI                0x4E49  
#define PCI_CHIP_R360_NJ                0x4E4A  
#define PCI_CHIP_R350_NK                0x4E4B  
#define PCI_CHIP_RV350_NP               0x4E50
#define PCI_CHIP_RV350_NQ               0x4E51
#define PCI_CHIP_RV350_NR               0x4E52
#define PCI_CHIP_RV350_NS               0x4E53
#define PCI_CHIP_RV350_NT               0x4E54
#define PCI_CHIP_RV350_NV               0x4E56
#define PCI_CHIP_RADEON_QD		0x5144
#define PCI_CHIP_RADEON_QE		0x5145
#define PCI_CHIP_RADEON_QF		0x5146
#define PCI_CHIP_RADEON_QG		0x5147
#define PCI_CHIP_RADEON_QY		0x5159
#define PCI_CHIP_RADEON_QZ		0x515A
#define PCI_CHIP_RN50_515E		0x515E
#define PCI_CHIP_R200_QH		0x5148
#define PCI_CHIP_R200_QI		0x5149
#define PCI_CHIP_R200_QJ		0x514A
#define PCI_CHIP_R200_QK		0x514B
#define PCI_CHIP_R200_QL		0x514C
#define PCI_CHIP_R200_QM		0x514D
#define PCI_CHIP_R200_QN		0x514E
#define PCI_CHIP_R200_QO		0x514F
#define PCI_CHIP_RV200_QW		0x5157
#define PCI_CHIP_RV200_QX		0x5158
#define PCI_CHIP_RV370_5460		0x5460
#define PCI_CHIP_RV370_5464             0x5464
#define PCI_CHIP_RS300_5834		0x5834
#define PCI_CHIP_RS300_5835		0x5835
#define PCI_CHIP_RS300_5836		0x5836
#define PCI_CHIP_RS300_5837		0x5837
#define PCI_CHIP_RV280_5960		0x5960
#define PCI_CHIP_RV280_5961		0x5961
#define PCI_CHIP_RV280_5962		0x5962
#define PCI_CHIP_RV280_5964		0x5964
#define PCI_CHIP_RV280_5965 		0x5965
#define PCI_CHIP_RN50_5969		0x5969
#define PCI_CHIP_RV370_5B60             0x5B60
#define PCI_CHIP_RV370_5B62             0x5B62
#define PCI_CHIP_RV370_5B64             0x5B64
#define PCI_CHIP_RV370_5B65             0x5B65
#define PCI_CHIP_RV280_5C61		0x5C61
#define PCI_CHIP_RV280_5C63		0x5C63

enum {
   CHIP_FAMILY_R100,
   CHIP_FAMILY_RV100,
   CHIP_FAMILY_RS100,
   CHIP_FAMILY_RV200,
   CHIP_FAMILY_RS200,
   CHIP_FAMILY_R200,
   CHIP_FAMILY_RV250,
   CHIP_FAMILY_RS300,
   CHIP_FAMILY_RV280,
   CHIP_FAMILY_R300,
   CHIP_FAMILY_R350,
   CHIP_FAMILY_RV350,
   CHIP_FAMILY_RV380,
   CHIP_FAMILY_R420,
   CHIP_FAMILY_RV410,
   CHIP_FAMILY_LAST
};

/* General classes of Radeons, as described above the device ID section */
#define RADEON_CLASS_R100		(0 << 0)
#define RADEON_CLASS_R200		(1 << 0)
#define RADEON_CLASS_R300		(2 << 0)
#define RADEON_CLASS_MASK		(3 << 0)

#define RADEON_CHIPSET_TCL		(1 << 2)	/* tcl support - any radeon */
#define RADEON_CHIPSET_BROKEN_STENCIL	(1 << 3)	/* r100 stencil bug */
#define R200_CHIPSET_YCBCR_BROKEN	(1 << 4)	/* r200 ycbcr bug */

#endif /* _RADEON_CHIPSET_H */
