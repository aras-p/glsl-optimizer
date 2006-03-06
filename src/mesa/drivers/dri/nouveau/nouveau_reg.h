/**************************************************************************

Copyright 2006 Stephane Marchesin, Sylvain Munaut
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/




#define NV03_BOOT_0                                        0x00100000
#    define NV03_BOOT_0_RAM_AMOUNT                         0x00000003
#    define NV03_BOOT_0_RAM_AMOUNT_8MB                     0x00000000
#    define NV03_BOOT_0_RAM_AMOUNT_2MB                     0x00000001
#    define NV03_BOOT_0_RAM_AMOUNT_4MB                     0x00000002
#    define NV03_BOOT_0_RAM_AMOUNT_8MB_SDRAM               0x00000003
#    define NV04_BOOT_0_RAM_AMOUNT_32MB                    0x00000000
#    define NV04_BOOT_0_RAM_AMOUNT_4MB                     0x00000001
#    define NV04_BOOT_0_RAM_AMOUNT_8MB                     0x00000002
#    define NV04_BOOT_0_RAM_AMOUNT_16MB                    0x00000003

#define NV04_FIFO_DATA                                     0x0010020c
#    define NV10_FIFO_DATA_RAM_AMOUNT_MB_MASK              0xfff00000
#    define NV10_FIFO_DATA_RAM_AMOUNT_MB_SHIFT             20

#define NV03_STATUS                                        0x004006b0
#define NV04_STATUS                                        0x00400700

#define NV03_FIFO_SIZE                                     0x8000
// NV10 maybe has 12 fifos
// NV40 probably has 16 fifos
#define NV03_FIFO_NUMBER                                   8
#define NV03_FIFO_REGS_SIZE                                0x10000
#    define NV03_FIFO_REGS_DMAPUT                          0x00000040
#    define NV03_FIFO_REGS_DMAGET                          0x00000044

/* Fifo commands. These are not regs, neither masks */
#define NV03_FIFO_CMD_JUMP                                 0x20000000
#define NV03_FIFO_CMD_JUMP_OFFSET_MASK                     0x1ffffffc
#define NV03_FIFO_CMD_REWIND                               (NV03_FIFO_CMD_JUMP | (0 & NV03_FIFO_CMD_JUMP_OFFSET_MASK))

/* Rendering commands */
#define NV20_PRIMITIVE                                     0x000017fc
#define NV30_PRIMITIVE                                     0x00001808
#define NV20_BEGIN_VERTICES                                0x00001818

/* Vertex attributes */
#define NV20_VERTEX_ATTRIBUTE(i)                           (0x00001760+i*4)
#define NV30_VERTEX_ATTRIBUTES                             0x00001740
#define NV30_UNKNOWN_0                                     0x00001718


