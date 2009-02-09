/*
 * (C) Copyright IBM Corporation 2008
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cell/common.h"
#include "spu_main.h"
#include "spu_dcache.h"

#define CACHELINE_LOG2SIZE    7
#define LINE_SIZE             (1U << 7)
#define ALIGN_MASK            (~(LINE_SIZE - 1))

#define CACHE_NAME            data
#define CACHED_TYPE           qword
#define CACHE_TYPE            CACHE_TYPE_RO
#define CACHE_SET_TAGID(set)  (((set) & 0x03) + TAG_DCACHE0)
#define CACHE_LOG2NNWAY       2
#define CACHE_LOG2NSETS       6
#ifdef DEBUG
#define CACHE_STATS           1
#endif
#include <cache-api.h>

/* Yes folks, this is ugly.
 */
#undef CACHE_NWAY
#undef CACHE_NSETS
#define CACHE_NAME            data
#define CACHE_NWAY            4
#define CACHE_NSETS           (1U << 6)


/**
 * Fetch between arbitrary number of bytes from an unaligned address
 *
 * \param dst   Destination data buffer
 * \param ea    Main memory effective address of source data
 * \param size  Number of bytes to read
 *
 * \warning
 * As is hinted by the type of the \c dst pointer, this function writes
 * multiples of 16-bytes.
 */
void
spu_dcache_fetch_unaligned(qword *dst, unsigned ea, unsigned size)
{
   const int shift = ea & 0x0f;
   const unsigned read_size = ROUNDUP16(size + shift);
   const unsigned last_read = ROUNDUP16(ea + size);
   const qword *const last_write = dst + (ROUNDUP16(size) / 16);
   unsigned i;


   if (shift == 0) {
      /* Data is already aligned.  Fetch directly into the destination buffer.
       */
      for (i = 0; i < size; i += 16) {
         *(dst++) = cache_rd(data, ea + i);
      }
   } else {
      qword hi;


      /* Please exercise extreme caution when modifying this code.  This code
       * must not read past the end of the page containing the source data,
       * and it must not write more than ((size + 15) / 16) qwords to the
       * destination buffer.
       */
      ea &= ~0x0f;
      hi = cache_rd(data, ea);
      for (i = 16; i < read_size; i += 16) {
         qword lo = cache_rd(data, ea + i);

         *(dst++) = si_or((qword) spu_slqwbyte(hi, shift),
                          (qword) spu_rlmaskqwbyte(lo, shift - 16));
         hi = lo;
      }

      if (dst != last_write) {
         *(dst++) = si_or((qword) spu_slqwbyte(hi, shift), si_il(0));
      }
   }
   
   ASSERT((ea + i) == last_read);
   ASSERT(dst == last_write);
}


/**
 * Notify the cache that a range of main memory may have been modified
 */
void
spu_dcache_mark_dirty(unsigned ea, unsigned size)
{
   unsigned i;
   const unsigned aligned_start = (ea & ALIGN_MASK);
   const unsigned aligned_end = (ea + size + (LINE_SIZE - 1)) 
       & ALIGN_MASK;


   for (i = 0; i < (CACHE_NWAY * CACHE_NSETS); i++) {
      const unsigned entry = __cache_dir[i];
      const unsigned addr = entry & ~0x0f;

      __cache_dir[i] = ((addr >= aligned_start) && (addr < aligned_end))
          ? (entry & ~CACHELINE_VALID) : entry;
   }
}


/**
 * Print cache utilization report
 */
void
spu_dcache_report(void)
{
#ifdef CACHE_STATS
   if (spu.init.id == 0) {
      printf("SPU 0: Texture cache report:\n");
      cache_pr_stats(data);
   }
#endif
}


