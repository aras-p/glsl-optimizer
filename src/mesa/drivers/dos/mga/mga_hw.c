/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * 
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver v1.3 for Mesa 5.0  --  MGA2064W HW mapping
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <crt0.h>
#include <dpmi.h>
#include <pc.h>
#include <string.h>
#include <sys/nearptr.h>
#include <sys/segments.h>

#include "../internal.h"
#include "mga_reg.h"
#include "mga_hw.h"



/* Hack alert:
 * these should really be externs
 */
/* PCI access routines */
static int pci_find_device (int deviceID, int vendorID, int deviceIndex, int *handle);
static unsigned long pci_read_long (int handle, int index);
static void pci_write_long (int handle, int index, unsigned long value);



/* PCI device identifiers */
#define MATROX_VENDOR_ID 0x102B

typedef enum {
        MATROX_MILL_ID = 0x0519
} MATROX_ID;

static MATROX_ID matrox_id_list[] = {
       MATROX_MILL_ID,
       0
};



/* internal hardware data structures */
#if !MGA_FARPTR
static int dirty;
#endif
static int bus_id;
static unsigned long reg40;
static unsigned long io_mem_base[4], linear_base;
static unsigned long io_mem_size[4], linear_size;
static MATROX_ID matrox_id;



/* interface structures containing hardware pointer data */
MGA_HWPTR mgaptr;



/* Desc: create MMAP
 *
 * In  :
 * Out :
 *
 * Note:
 */
static int _create_mmap (__dpmi_paddr *m, unsigned long base, unsigned long size)
{
#if MGA_FARPTR
 int sel;
 if (_create_selector(&sel, base, size)) {
    return -1;
 }
 m->selector = sel;
 m->offset32 = 0;
#else
 m->selector = _my_ds();
 if (_create_linear_mapping(&m->offset32, base, size)) {
    return -1;
 }
 m->offset32 -= __djgpp_base_address;
#endif
 return 0;
}



/* Desc: destroy MMAP
 *
 * In  :
 * Out :
 *
 * Note:
 */
static void _destroy_mmap (__dpmi_paddr *m)
{
#if MGA_FARPTR
 int sel = m->selector;
 _remove_selector(&sel);
#else
 m->offset32 += __djgpp_base_address;
 _remove_linear_mapping(&m->offset32);
#endif
 m->selector = 0;
 m->offset32 = 0;
}



/* Desc: Counts amount of installed RAM
 *
 * In  :
 * Out :
 *
 * Note:
 */
static int _mga_get_vram (MATROX_ID chip, unsigned long base)
{
 int ProbeSize = 8;
 int SizeFound = 2;
 unsigned char tmp;
 int i;
 __dpmi_paddr fb;

 switch (chip) {
	case MATROX_MILL_ID:
	     ProbeSize = 8;
	     break;
 }

 if (_create_mmap(&fb, base, ProbeSize*1024*1024)) {
    return 0;
 }

 /* turn MGA mode on - enable linear frame buffer (CRTCEXT3) */
 mga_select();
 mga_outb(M_CRTC_EXT_INDEX, 3);
 tmp = mga_inb(M_CRTC_EXT_DATA);
 mga_outb(M_CRTC_EXT_DATA, tmp | M_MGAMODE);

 /* write, read and compare method */
 for (i=ProbeSize; i>2; i-= 2) {
     hwptr_pokeb(fb, i*1024*1024 - 1, 0xAA);
     mga_select();
     mga_outb(M_CRTC_INDEX, 0); /* flush the cache */
     mga_inl(M_STATUS); /* delay */
     mga_inl(M_STATUS); /* delay */
     mga_inl(M_STATUS); /* delay */
     if (hwptr_peekb(fb, i*1024*1024 - 1) == 0xAA) {
	SizeFound = i;
	break;
     }
 }

 /* restore CRTCEXT3 state */
 mga_select();
 mga_outb(M_CRTC_EXT_INDEX, 3);
 mga_outb(M_CRTC_EXT_DATA, tmp);

 _destroy_mmap(&fb);

 return SizeFound*1024*1024;
}



/* Desc: Frees all resources allocated by MGA init code.
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_hw_fini (void)
{
 int i;

 pci_write_long(bus_id, 0x40, reg40);

 for (i=0; i<4; i++) {
     _destroy_mmap(&mgaptr.io_mem_map[i]);
 }

 _destroy_mmap(&mgaptr.linear_map);

#if !MGA_FARPTR
 if (dirty) {
    __djgpp_nearptr_disable();
    dirty = FALSE;
 }
#endif

 matrox_id = 0;
}



/* Desc: Attempts to detect MGA.
 *
 * In  :
 * Out :
 *
 * Note: The first thing ever to be called. This is in charge of filling in
 *       the driver header with all the required information and function
 *       pointers. We do not yet have access to the video memory, so we can't
 *       talk directly to the card.
 */
int mga_hw_init (unsigned long *vram, int *interleave, char *name)
{
 int i;
 unsigned long pci_base[2];

 if (matrox_id) {
    return matrox_id;
 }

#if !MGA_FARPTR
 /* enable nearptr access */
 if (_crt0_startup_flags & _CRT0_FLAG_NEARPTR) {
    dirty = FALSE;
 } else {
    if (__djgpp_nearptr_enable() == 0)
       return NULL;

    dirty = TRUE;
 }
#endif

 /* find PCI device */
 matrox_id = 0;

 for (bus_id=0, i=0; matrox_id_list[i]; i++) {
     if (pci_find_device(matrox_id_list[i], MATROX_VENDOR_ID, 0, &bus_id)) {
	matrox_id = matrox_id_list[i];
	break;
     }
 }

 /* set up the card name */
 switch (matrox_id) {
	case MATROX_MILL_ID:
	     if (name) strcpy(name, "Millennium");
	     break;
        default:
             matrox_id = 0;
             return -1;
 }

 reg40 = pci_read_long(bus_id, 0x40);
#if 0 /* overclock a little :) */
 {
  int rfhcnt = (reg40 >> 16) & 0xF;
  if ((reg40 & 0x200000) && (rfhcnt < 0xC)) {
     pci_write_long(bus_id, 0x40, (reg40 & 0xFFF0FFFF) | 0x000C0000);
  }
 }
#endif

 /* read hardware configuration data */
 for (i=0; i<2; i++)
     pci_base[i] = pci_read_long(bus_id, 16+i*4);

 /* work out the linear framebuffer and MMIO addresses */
 if (matrox_id == MATROX_MILL_ID) {
    if (pci_base[0])
       io_mem_base[0] = pci_base[0] & 0xFFFFC000;

    if (pci_base[1])
       linear_base = pci_base[1] & 0xFF800000;
 }

 if (!linear_base || !io_mem_base[0])
    return NULL;

 /* deal with the memory mapping crap */
 io_mem_size[0] = 0x4000;

 for (i=0; i<4; i++) {
     if (io_mem_base[i]) {
        if (_create_mmap(&mgaptr.io_mem_map[i], io_mem_base[i], io_mem_size[i])) {
	   mga_hw_fini();
	   return NULL;
	}
     }
 }

 *vram = linear_size = _mga_get_vram(matrox_id, linear_base);

 if (_create_mmap(&mgaptr.linear_map, linear_base, linear_size)) {
    mga_hw_fini();
    return NULL;
 }

 /* fill in user data */
 *interleave = linear_size > 2*1024*1024;

 return matrox_id;
}



/* PCI routines added by SET */
#define PCIAddr   0xCF8
#define PCIData   0xCFC
#define PCIEnable 0x80000000



/* FindPCIDevice:
 *  Replacement for the INT 1A - PCI BIOS v2.0c+ - FIND PCI DEVICE, AX = B102h
 *
 *  Note: deviceIndex is because a card can hold more than one PCI chip.
 * 
 *  Searches the board of the vendor supplied in vendorID with 
 *  identification number deviceID and index deviceIndex (normally 0). 
 *  The value returned in handle can be used to access the PCI registers 
 *  of this board.
 *
 *  Return: 1 if found 0 if not found.
 */
static int pci_find_device (int deviceID, int vendorID, int deviceIndex, int *handle)
{
 int model, vendor, card, device;
 unsigned value, full_id, bus, busMax;

 deviceIndex <<= 8;

 /* for each PCI bus */
 for (bus=0, busMax=0x10000; bus<busMax; bus+=0x10000) {

     /* for each hardware device */
     for (device=0, card=0; card<32; card++, device+=0x800) {
	 value = PCIEnable | bus | deviceIndex | device;
	 outportl(PCIAddr, value);
	 full_id = inportl(PCIData);

	 /* get the vendor and model ID */
	 vendor = full_id & 0xFFFF;
	 model = full_id >> 16;

	 if (vendor != 0xFFFF) {
	    /* is this the one we want? */
	    if ((deviceID == model) && (vendorID == vendor)) {
	       *handle = value;
	       return 1;
	    }

	    /* is it a bridge to a secondary bus? */
	    outportl(PCIAddr, value | 8);

	    if (((inportl(PCIData) >> 16) == 0x0600) || (full_id==0x00011011))
	       busMax += 0x10000;
         }
     }
 }

 return 0;
}



/* Desc:
 *
 * In  :
 * Out :
 *
 * Note:
 */
static unsigned long pci_read_long (int handle, int index)
{
 outportl(PCIAddr, PCIEnable | handle | index);
 return inportl(PCIData);
}



/* Desc:
 *
 * In  :
 * Out :
 *
 * Note:
 */
static void pci_write_long (int handle, int index, unsigned long value)
{
 outportl(PCIAddr, PCIEnable | handle | index);
 outportl(PCIData, value);
}
