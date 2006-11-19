/**************************************************************************

Copyright 2006 Stephane Marchesin
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



#ifndef __NOUVEAU_FIFO_H__
#define __NOUVEAU_FIFO_H__

#include "nouveau_context.h"
#include "nouveau_ctrlreg.h"

//#define NOUVEAU_RING_DEBUG

#define NV_READ(reg) *(volatile u_int32_t *)(nmesa->mmio + (reg))

#define NV_FIFO_READ(reg) *(volatile u_int32_t *)(nmesa->fifo.mmio + (reg/4))
#define NV_FIFO_WRITE(reg,value) *(volatile u_int32_t *)(nmesa->fifo.mmio + (reg/4)) = value;
#define NV_FIFO_READ_GET() ((NV_FIFO_READ(NV03_FIFO_REGS_DMAGET) - nmesa->fifo.put_base) >> 2)
#define NV_FIFO_WRITE_PUT(val) NV_FIFO_WRITE(NV03_FIFO_REGS_DMAPUT, ((val)<<2) + nmesa->fifo.put_base)

/* 
 * Ring/fifo interface
 *
 * - Begin a ring section with BEGIN_RING_SIZE (if you know the full size in advance)
 * - Begin a ring section with BEGIN_RING_PRIM otherwise (and then finish with FINISH_RING_PRIM)
 * - Output stuff to the ring with either OUT_RINGp (outputs a raw mem chunk), OUT_RING (1 uint32_t) or OUT_RINGf (1 float)
 * - RING_AVAILABLE returns the available fifo (in uint32_ts)
 * - RING_AHEAD returns how much ahead of the last submission point we are
 * - FIRE_RING fires whatever we have that wasn't fired before
 * - WAIT_RING waits for size (in uint32_ts) to be available in the fifo
 */

/* Enable for ring debugging.  Prints out writes to the ring buffer
 * but does not actually write to it.
 */
#ifdef NOUVEAU_RING_DEBUG

#define OUT_RINGp(ptr,sz) do {                                                  \
int i; printf("OUT_RINGp:\n"); for(i=0;i<sz;i+=4) printf(" 0x%8x\n", ptr+sz);   \
}while(0)

#define OUT_RING(n) do {                                                        \
    printf("OUT_RINGn: 0x%8x\n", n);                                            \
}while(0)

#define OUT_RINGf(n) do {                                                       \
    printf("OUT_RINGf: 0x%8x\n", f);                                            \
}while(0)

#else

#define OUT_RINGp(ptr,sz) do{							\
	memcpy(nmesa->fifo.buffer+nmesa->fifo.current,ptr,sz);			\
	nmesa->fifo.current+=(sz/sizeof(*ptr));					\
}while(0)

#define OUT_RING(n) do {							\
nmesa->fifo.buffer[nmesa->fifo.current++]=n;					\
}while(0)

#define OUT_RINGf(n) do {							\
*((float*)(nmesa->fifo.buffer+nmesa->fifo.current++))=n;			\
}while(0)

#endif

extern void WAIT_RING(nouveauContextPtr nmesa,u_int32_t size);

#define BEGIN_RING_PRIM(subchannel,tag,size) do {					\
	if (nmesa->fifo.free<size)							\
		WAIT_RING(nmesa,(size));						\
	OUT_RING( ((subchannel) << 13) | (tag));					\
}while(0)

#define FINISH_RING_PRIM() do{								\
	nmesa->fifo.buffer[nmesa->fifo.put]|=((nmesa->fifo.current-nmesa->fifo.put) << 18);		\
}while(0)

#define BEGIN_RING_SIZE(subchannel,tag,size) do {					\
	if (nmesa->fifo.free <= (size))							\
		WAIT_RING(nmesa,(size));						\
	OUT_RING( ((size)<<18) | ((subchannel) << 13) | (tag));				\
	nmesa->fifo.free -= ((size) + 1);                                               \
}while(0)

#define RING_AVAILABLE() (nmesa->fifo.free-1)

#define RING_AHEAD() ((nmesa->fifo.put<=nmesa->fifo.current)?(nmesa->fifo.current-nmesa->fifo.put):nmesa->fifo.max-nmesa->fifo.put+nmesa->fifo.current)

#define FIRE_RING() do {                             \
	if (nmesa->fifo.current!=nmesa->fifo.put) {  \
		nmesa->fifo.put=nmesa->fifo.current; \
		NV_FIFO_WRITE_PUT(nmesa->fifo.put);  \
	}                                            \
}while(0)

extern void nouveauWaitForIdle(nouveauContextPtr nmesa);
extern GLboolean nouveauFifoInit(nouveauContextPtr nmesa);

#endif /* __NOUVEAU_FIFO_H__ */


