/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include <spu_mfcio.h>
#include <transpose_matrix4x4.h>

#include "pipe/p_util.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "spu_exec.h"
#include "spu_vertex_shader.h"
#include "spu_main.h"

#define CACHE_NAME            attribute
#define CACHED_TYPE           qword
#define CACHE_TYPE            CACHE_TYPE_RO
#define CACHE_SET_TAGID(set)  TAG_VERTEX_BUFFER
#define CACHE_LOG2NNWAY       2
#define CACHE_LOG2NSETS       6
#include <cache-api.h>

/* Yes folks, this is ugly.
 */
#undef CACHE_NWAY
#undef CACHE_NSETS
#define CACHE_NAME            attribute
#define CACHE_NWAY            4
#define CACHE_NSETS           (1U << 6)


#define DRAW_DBG 0


static const vec_float4 defaults = { 0.0, 0.0, 0.0, 1.0 };

/**
 * Fetch between 1 and 32 bytes from an unaligned address
 */
static INLINE void
fetch_unaligned(qword *dst, unsigned ea, unsigned size)
{
   qword tmp[4];
   const int shift = ea & 0x0f;
   const unsigned aligned_start_ea = ea & ~0x0f;
   const unsigned aligned_end_ea = (ea + size) & ~0x0f;
   const unsigned num_entries = ((aligned_end_ea - aligned_start_ea) / 16) + 1;
   unsigned i;


   if (shift == 0) {
      /* Data is already aligned.  Fetch directly into the destination buffer.
       */
      for (i = 0; i < num_entries; i++) {
	 dst[i] = cache_rd(attribute, (ea & ~0x0f) + (i * 16));
      }
   } else {
      /* Fetch data from the cache to the local buffer.
       */
      for (i = 0; i < num_entries; i++) {
	 tmp[i] = cache_rd(attribute, (ea & ~0x0f) + (i * 16));
      }


      /* Fix the alignment of the data and write to the destination buffer.
       */
      for (i = 0; i < ((size + 15) / 16); i++) {
	 dst[i] = si_or((qword) spu_slqwbyte(tmp[i], shift),
			(qword) spu_rlmaskqwbyte(tmp[i + 1], shift - 16));
      }
   }
}


#define CVT_32_FLOAT(q)    (*(q))

static INLINE qword
CVT_64_FLOAT(const qword *qw)
{
   qword shuf_first = (qword) {
      0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13,
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   };

   qword a = si_frds(qw[0]);
   qword b = si_frds(si_rotqbyi(qw[0], 8));
   qword c = si_frds(qw[1]);
   qword d = si_frds(si_rotqbyi(qw[1], 8));

   qword ab = si_shufb(a, b, shuf_first);
   qword cd = si_shufb(c, d, si_rotqbyi(shuf_first, 8));
   
   return si_or(ab, cd);
}


static INLINE qword
CVT_8_USCALED(const qword *qw)
{
   qword shuffle = (qword) {
      0x00, 0x80, 0x80, 0x80, 0x01, 0x80, 0x80, 0x80,
      0x02, 0x80, 0x80, 0x80, 0x03, 0x80, 0x80, 0x80,
   };

   return si_cuflt(si_shufb(*qw, *qw, shuffle), 0);
}


static INLINE qword
CVT_16_USCALED(const qword *qw)
{
   qword shuffle = (qword) {
      0x00, 0x01, 0x80, 0x80, 0x02, 0x03, 0x80, 0x80,
      0x04, 0x05, 0x80, 0x80, 0x06, 0x07, 0x80, 0x80,
   };

   return si_cuflt(si_shufb(*qw, *qw, shuffle), 0);
}


static INLINE qword
CVT_32_USCALED(const qword *qw)
{
   return si_cuflt(*qw, 0);
}

static INLINE qword
CVT_8_SSCALED(const qword *qw)
{
   qword shuffle = (qword) {
      0x00, 0x80, 0x80, 0x80, 0x01, 0x80, 0x80, 0x80,
      0x02, 0x80, 0x80, 0x80, 0x03, 0x80, 0x80, 0x80,
   };

   return si_csflt(si_shufb(*qw, *qw, shuffle), 0);
}


static INLINE qword
CVT_16_SSCALED(const qword *qw)
{
   qword shuffle = (qword) {
      0x00, 0x01, 0x80, 0x80, 0x02, 0x03, 0x80, 0x80,
      0x04, 0x05, 0x80, 0x80, 0x06, 0x07, 0x80, 0x80,
   };

   return si_csflt(si_shufb(*qw, *qw, shuffle), 0);
}


static INLINE qword
CVT_32_SSCALED(const qword *qw)
{
   return si_csflt(*qw, 0);
}


static INLINE qword
CVT_8_UNORM(const qword *qw)
{
   const qword scale = (qword) spu_splats(1.0f / 255.0f);
   return si_fm(CVT_8_USCALED(qw), scale);
}


static INLINE qword
CVT_16_UNORM(const qword *qw)
{
   const qword scale = (qword) spu_splats(1.0f / 65535.0f);
   return si_fm(CVT_16_USCALED(qw), scale);
}


static INLINE qword
CVT_32_UNORM(const qword *qw)
{
   const qword scale = (qword) spu_splats(1.0f / 4294967295.0f);
   return si_fm(CVT_32_USCALED(qw), scale);
}


static INLINE qword
CVT_8_SNORM(const qword *qw)
{
   const qword scale = (qword) spu_splats(1.0f / 127.0f);
   return si_fm(CVT_8_SSCALED(qw), scale);
}


static INLINE qword
CVT_16_SNORM(const qword *qw)
{
   const qword scale = (qword) spu_splats(1.0f / 32767.0f);
   return si_fm(CVT_16_SSCALED(qw), scale);
}


static INLINE qword
CVT_32_SNORM(const qword *qw)
{
   const qword scale = (qword) spu_splats(1.0f / 2147483647.0f);
   return si_fm(CVT_32_SSCALED(qw), scale);
}

#define SZ_4 si_il(0U)
#define SZ_3 si_rotqmbyi(si_il(~0), -12)
#define SZ_2 si_rotqmbyi(si_il(~0), -8)
#define SZ_1 si_rotqmbyi(si_il(~0), -4)

/**
 * Fetch a float[4] vertex attribute from memory, doing format/type
 * conversion as needed.
 *
 * This is probably needed/dupliocated elsewhere, eg format
 * conversion, texture sampling etc.
 */
#define FETCH_ATTRIB( NAME, SZ, CVT, N )			\
static void							\
fetch_##NAME(qword *out, const qword *in)			\
{								\
   qword tmp[4];						\
								\
   tmp[0] = si_selb(CVT(in + (0 * N)), (qword) defaults, SZ);	\
   tmp[1] = si_selb(CVT(in + (1 * N)), (qword) defaults, SZ);	\
   tmp[2] = si_selb(CVT(in + (2 * N)), (qword) defaults, SZ);	\
   tmp[3] = si_selb(CVT(in + (3 * N)), (qword) defaults, SZ);	\
   _transpose_matrix4x4((vec_float4 *) out, (vec_float4 *) tmp);	\
}

FETCH_ATTRIB( R64G64B64A64_FLOAT,   SZ_4, CVT_64_FLOAT, 2 )
FETCH_ATTRIB( R64G64B64_FLOAT,      SZ_3, CVT_64_FLOAT, 2 )
FETCH_ATTRIB( R64G64_FLOAT,         SZ_2, CVT_64_FLOAT, 2 )
FETCH_ATTRIB( R64_FLOAT,            SZ_1, CVT_64_FLOAT, 2 )

FETCH_ATTRIB( R32G32B32A32_FLOAT,   SZ_4, CVT_32_FLOAT, 1 )
FETCH_ATTRIB( R32G32B32_FLOAT,      SZ_3, CVT_32_FLOAT, 1 )
FETCH_ATTRIB( R32G32_FLOAT,         SZ_2, CVT_32_FLOAT, 1 )
FETCH_ATTRIB( R32_FLOAT,            SZ_1, CVT_32_FLOAT, 1 )

FETCH_ATTRIB( R32G32B32A32_USCALED, SZ_4, CVT_32_USCALED, 1 )
FETCH_ATTRIB( R32G32B32_USCALED,    SZ_3, CVT_32_USCALED, 1 )
FETCH_ATTRIB( R32G32_USCALED,       SZ_2, CVT_32_USCALED, 1 )
FETCH_ATTRIB( R32_USCALED,          SZ_1, CVT_32_USCALED, 1 )

FETCH_ATTRIB( R32G32B32A32_SSCALED, SZ_4, CVT_32_SSCALED, 1 )
FETCH_ATTRIB( R32G32B32_SSCALED,    SZ_3, CVT_32_SSCALED, 1 )
FETCH_ATTRIB( R32G32_SSCALED,       SZ_2, CVT_32_SSCALED, 1 )
FETCH_ATTRIB( R32_SSCALED,          SZ_1, CVT_32_SSCALED, 1 )

FETCH_ATTRIB( R32G32B32A32_UNORM, SZ_4, CVT_32_UNORM, 1 )
FETCH_ATTRIB( R32G32B32_UNORM,    SZ_3, CVT_32_UNORM, 1 )
FETCH_ATTRIB( R32G32_UNORM,       SZ_2, CVT_32_UNORM, 1 )
FETCH_ATTRIB( R32_UNORM,          SZ_1, CVT_32_UNORM, 1 )

FETCH_ATTRIB( R32G32B32A32_SNORM, SZ_4, CVT_32_SNORM, 1 )
FETCH_ATTRIB( R32G32B32_SNORM,    SZ_3, CVT_32_SNORM, 1 )
FETCH_ATTRIB( R32G32_SNORM,       SZ_2, CVT_32_SNORM, 1 )
FETCH_ATTRIB( R32_SNORM,          SZ_1, CVT_32_SNORM, 1 )

FETCH_ATTRIB( R16G16B16A16_USCALED, SZ_4, CVT_16_USCALED, 1 )
FETCH_ATTRIB( R16G16B16_USCALED,    SZ_3, CVT_16_USCALED, 1 )
FETCH_ATTRIB( R16G16_USCALED,       SZ_2, CVT_16_USCALED, 1 )
FETCH_ATTRIB( R16_USCALED,          SZ_1, CVT_16_USCALED, 1 )

FETCH_ATTRIB( R16G16B16A16_SSCALED, SZ_4, CVT_16_SSCALED, 1 )
FETCH_ATTRIB( R16G16B16_SSCALED,    SZ_3, CVT_16_SSCALED, 1 )
FETCH_ATTRIB( R16G16_SSCALED,       SZ_2, CVT_16_SSCALED, 1 )
FETCH_ATTRIB( R16_SSCALED,          SZ_1, CVT_16_SSCALED, 1 )

FETCH_ATTRIB( R16G16B16A16_UNORM, SZ_4, CVT_16_UNORM, 1 )
FETCH_ATTRIB( R16G16B16_UNORM,    SZ_3, CVT_16_UNORM, 1 )
FETCH_ATTRIB( R16G16_UNORM,       SZ_2, CVT_16_UNORM, 1 )
FETCH_ATTRIB( R16_UNORM,          SZ_1, CVT_16_UNORM, 1 )

FETCH_ATTRIB( R16G16B16A16_SNORM, SZ_4, CVT_16_SNORM, 1 )
FETCH_ATTRIB( R16G16B16_SNORM,    SZ_3, CVT_16_SNORM, 1 )
FETCH_ATTRIB( R16G16_SNORM,       SZ_2, CVT_16_SNORM, 1 )
FETCH_ATTRIB( R16_SNORM,          SZ_1, CVT_16_SNORM, 1 )

FETCH_ATTRIB( R8G8B8A8_USCALED,   SZ_4, CVT_8_USCALED, 1 )
FETCH_ATTRIB( R8G8B8_USCALED,     SZ_3, CVT_8_USCALED, 1 )
FETCH_ATTRIB( R8G8_USCALED,       SZ_2, CVT_8_USCALED, 1 )
FETCH_ATTRIB( R8_USCALED,         SZ_1, CVT_8_USCALED, 1 )

FETCH_ATTRIB( R8G8B8A8_SSCALED,  SZ_4, CVT_8_SSCALED, 1 )
FETCH_ATTRIB( R8G8B8_SSCALED,    SZ_3, CVT_8_SSCALED, 1 )
FETCH_ATTRIB( R8G8_SSCALED,      SZ_2, CVT_8_SSCALED, 1 )
FETCH_ATTRIB( R8_SSCALED,        SZ_1, CVT_8_SSCALED, 1 )

FETCH_ATTRIB( R8G8B8A8_UNORM,  SZ_4, CVT_8_UNORM, 1 )
FETCH_ATTRIB( R8G8B8_UNORM,    SZ_3, CVT_8_UNORM, 1 )
FETCH_ATTRIB( R8G8_UNORM,      SZ_2, CVT_8_UNORM, 1 )
FETCH_ATTRIB( R8_UNORM,        SZ_1, CVT_8_UNORM, 1 )

FETCH_ATTRIB( R8G8B8A8_SNORM,  SZ_4, CVT_8_SNORM, 1 )
FETCH_ATTRIB( R8G8B8_SNORM,    SZ_3, CVT_8_SNORM, 1 )
FETCH_ATTRIB( R8G8_SNORM,      SZ_2, CVT_8_SNORM, 1 )
FETCH_ATTRIB( R8_SNORM,        SZ_1, CVT_8_SNORM, 1 )

FETCH_ATTRIB( A8R8G8B8_UNORM,       SZ_4, CVT_8_UNORM, 1 )



static spu_fetch_func get_fetch_func( enum pipe_format format )
{
   switch (format) {
   case PIPE_FORMAT_R64_FLOAT:
      return fetch_R64_FLOAT;
   case PIPE_FORMAT_R64G64_FLOAT:
      return fetch_R64G64_FLOAT;
   case PIPE_FORMAT_R64G64B64_FLOAT:
      return fetch_R64G64B64_FLOAT;
   case PIPE_FORMAT_R64G64B64A64_FLOAT:
      return fetch_R64G64B64A64_FLOAT;

   case PIPE_FORMAT_R32_FLOAT:
      return fetch_R32_FLOAT;
   case PIPE_FORMAT_R32G32_FLOAT:
      return fetch_R32G32_FLOAT;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      return fetch_R32G32B32_FLOAT;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return fetch_R32G32B32A32_FLOAT;

   case PIPE_FORMAT_R32_UNORM:
      return fetch_R32_UNORM;
   case PIPE_FORMAT_R32G32_UNORM:
      return fetch_R32G32_UNORM;
   case PIPE_FORMAT_R32G32B32_UNORM:
      return fetch_R32G32B32_UNORM;
   case PIPE_FORMAT_R32G32B32A32_UNORM:
      return fetch_R32G32B32A32_UNORM;

   case PIPE_FORMAT_R32_USCALED:
      return fetch_R32_USCALED;
   case PIPE_FORMAT_R32G32_USCALED:
      return fetch_R32G32_USCALED;
   case PIPE_FORMAT_R32G32B32_USCALED:
      return fetch_R32G32B32_USCALED;
   case PIPE_FORMAT_R32G32B32A32_USCALED:
      return fetch_R32G32B32A32_USCALED;

   case PIPE_FORMAT_R32_SNORM:
      return fetch_R32_SNORM;
   case PIPE_FORMAT_R32G32_SNORM:
      return fetch_R32G32_SNORM;
   case PIPE_FORMAT_R32G32B32_SNORM:
      return fetch_R32G32B32_SNORM;
   case PIPE_FORMAT_R32G32B32A32_SNORM:
      return fetch_R32G32B32A32_SNORM;

   case PIPE_FORMAT_R32_SSCALED:
      return fetch_R32_SSCALED;
   case PIPE_FORMAT_R32G32_SSCALED:
      return fetch_R32G32_SSCALED;
   case PIPE_FORMAT_R32G32B32_SSCALED:
      return fetch_R32G32B32_SSCALED;
   case PIPE_FORMAT_R32G32B32A32_SSCALED:
      return fetch_R32G32B32A32_SSCALED;

   case PIPE_FORMAT_R16_UNORM:
      return fetch_R16_UNORM;
   case PIPE_FORMAT_R16G16_UNORM:
      return fetch_R16G16_UNORM;
   case PIPE_FORMAT_R16G16B16_UNORM:
      return fetch_R16G16B16_UNORM;
   case PIPE_FORMAT_R16G16B16A16_UNORM:
      return fetch_R16G16B16A16_UNORM;

   case PIPE_FORMAT_R16_USCALED:
      return fetch_R16_USCALED;
   case PIPE_FORMAT_R16G16_USCALED:
      return fetch_R16G16_USCALED;
   case PIPE_FORMAT_R16G16B16_USCALED:
      return fetch_R16G16B16_USCALED;
   case PIPE_FORMAT_R16G16B16A16_USCALED:
      return fetch_R16G16B16A16_USCALED;

   case PIPE_FORMAT_R16_SNORM:
      return fetch_R16_SNORM;
   case PIPE_FORMAT_R16G16_SNORM:
      return fetch_R16G16_SNORM;
   case PIPE_FORMAT_R16G16B16_SNORM:
      return fetch_R16G16B16_SNORM;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      return fetch_R16G16B16A16_SNORM;

   case PIPE_FORMAT_R16_SSCALED:
      return fetch_R16_SSCALED;
   case PIPE_FORMAT_R16G16_SSCALED:
      return fetch_R16G16_SSCALED;
   case PIPE_FORMAT_R16G16B16_SSCALED:
      return fetch_R16G16B16_SSCALED;
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
      return fetch_R16G16B16A16_SSCALED;

   case PIPE_FORMAT_R8_UNORM:
      return fetch_R8_UNORM;
   case PIPE_FORMAT_R8G8_UNORM:
      return fetch_R8G8_UNORM;
   case PIPE_FORMAT_R8G8B8_UNORM:
      return fetch_R8G8B8_UNORM;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return fetch_R8G8B8A8_UNORM;

   case PIPE_FORMAT_R8_USCALED:
      return fetch_R8_USCALED;
   case PIPE_FORMAT_R8G8_USCALED:
      return fetch_R8G8_USCALED;
   case PIPE_FORMAT_R8G8B8_USCALED:
      return fetch_R8G8B8_USCALED;
   case PIPE_FORMAT_R8G8B8A8_USCALED:
      return fetch_R8G8B8A8_USCALED;

   case PIPE_FORMAT_R8_SNORM:
      return fetch_R8_SNORM;
   case PIPE_FORMAT_R8G8_SNORM:
      return fetch_R8G8_SNORM;
   case PIPE_FORMAT_R8G8B8_SNORM:
      return fetch_R8G8B8_SNORM;
   case PIPE_FORMAT_R8G8B8A8_SNORM:
      return fetch_R8G8B8A8_SNORM;

   case PIPE_FORMAT_R8_SSCALED:
      return fetch_R8_SSCALED;
   case PIPE_FORMAT_R8G8_SSCALED:
      return fetch_R8G8_SSCALED;
   case PIPE_FORMAT_R8G8B8_SSCALED:
      return fetch_R8G8B8_SSCALED;
   case PIPE_FORMAT_R8G8B8A8_SSCALED:
      return fetch_R8G8B8A8_SSCALED;

   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return fetch_A8R8G8B8_UNORM;

   case 0:
      return NULL;		/* not sure why this is needed */

   default:
      assert(0);
      return NULL;
   }
}


static unsigned get_vertex_size( enum pipe_format format )
{
   switch (format) {
   case PIPE_FORMAT_R64_FLOAT:
      return 8;
   case PIPE_FORMAT_R64G64_FLOAT:
      return 2 * 8;
   case PIPE_FORMAT_R64G64B64_FLOAT:
      return 3 * 8;
   case PIPE_FORMAT_R64G64B64A64_FLOAT:
      return 4 * 8;

   case PIPE_FORMAT_R32_SSCALED:
   case PIPE_FORMAT_R32_SNORM:
   case PIPE_FORMAT_R32_USCALED:
   case PIPE_FORMAT_R32_UNORM:
   case PIPE_FORMAT_R32_FLOAT:
      return 4;
   case PIPE_FORMAT_R32G32_SSCALED:
   case PIPE_FORMAT_R32G32_SNORM:
   case PIPE_FORMAT_R32G32_USCALED:
   case PIPE_FORMAT_R32G32_UNORM:
   case PIPE_FORMAT_R32G32_FLOAT:
      return 2 * 4;
   case PIPE_FORMAT_R32G32B32_SSCALED:
   case PIPE_FORMAT_R32G32B32_SNORM:
   case PIPE_FORMAT_R32G32B32_USCALED:
   case PIPE_FORMAT_R32G32B32_UNORM:
   case PIPE_FORMAT_R32G32B32_FLOAT:
      return 3 * 4;
   case PIPE_FORMAT_R32G32B32A32_SSCALED:
   case PIPE_FORMAT_R32G32B32A32_SNORM:
   case PIPE_FORMAT_R32G32B32A32_USCALED:
   case PIPE_FORMAT_R32G32B32A32_UNORM:
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return 4 * 4;

   case PIPE_FORMAT_R16_SSCALED:
   case PIPE_FORMAT_R16_SNORM:
   case PIPE_FORMAT_R16_UNORM:
   case PIPE_FORMAT_R16_USCALED:
      return 2;
   case PIPE_FORMAT_R16G16_SSCALED:
   case PIPE_FORMAT_R16G16_SNORM:
   case PIPE_FORMAT_R16G16_USCALED:
   case PIPE_FORMAT_R16G16_UNORM:
      return 2 * 2;
   case PIPE_FORMAT_R16G16B16_SSCALED:
   case PIPE_FORMAT_R16G16B16_SNORM:
   case PIPE_FORMAT_R16G16B16_USCALED:
   case PIPE_FORMAT_R16G16B16_UNORM:
      return 3 * 2;
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
   case PIPE_FORMAT_R16G16B16A16_SNORM:
   case PIPE_FORMAT_R16G16B16A16_USCALED:
   case PIPE_FORMAT_R16G16B16A16_UNORM:
      return 4 * 2;

   case PIPE_FORMAT_R8_SSCALED:
   case PIPE_FORMAT_R8_SNORM:
   case PIPE_FORMAT_R8_USCALED:
   case PIPE_FORMAT_R8_UNORM:
      return 1;
   case PIPE_FORMAT_R8G8_SSCALED:
   case PIPE_FORMAT_R8G8_SNORM:
   case PIPE_FORMAT_R8G8_USCALED:
   case PIPE_FORMAT_R8G8_UNORM:
      return 2 * 1;
   case PIPE_FORMAT_R8G8B8_SSCALED:
   case PIPE_FORMAT_R8G8B8_SNORM:
   case PIPE_FORMAT_R8G8B8_USCALED:
   case PIPE_FORMAT_R8G8B8_UNORM:
      return 3 * 1;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_R8G8B8A8_SSCALED:
   case PIPE_FORMAT_R8G8B8A8_SNORM:
   case PIPE_FORMAT_R8G8B8A8_USCALED:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return 4 * 1;

   case 0:
      return 0;		/* not sure why this is needed */

   default:
      assert(0);
      return 0;
   }
}


/**
 * Fetch vertex attributes for 'count' vertices.
 */
static void generic_vertex_fetch(struct spu_vs_context *draw,
                                 struct spu_exec_machine *machine,
                                 const unsigned *elts,
                                 unsigned count)
{
   unsigned nr_attrs = draw->vertex_fetch.nr_attrs;
   unsigned attr;

   assert(count <= 4);

#if DRAW_DBG
   printf("SPU: %s count = %u, nr_attrs = %u\n", 
          __FUNCTION__, count, nr_attrs);
#endif

   /* loop over vertex attributes (vertex shader inputs)
    */
   for (attr = 0; attr < nr_attrs; attr++) {
      const unsigned pitch = draw->vertex_fetch.pitch[attr];
      const uint64_t src = draw->vertex_fetch.src_ptr[attr];
      const spu_fetch_func fetch = draw->vertex_fetch.fetch[attr];
      unsigned i;
      unsigned idx;
      const unsigned bytes_per_entry = draw->vertex_fetch.size[attr];
      const unsigned quads_per_entry = (bytes_per_entry + 15) / 16;
      qword in[2 * 4];


      /* Fetch four attributes for four vertices.  
       */
      idx = 0;
      for (i = 0; i < count; i++) {
         const uint64_t addr = src + (elts[i] * pitch);

#if DRAW_DBG
         printf("SPU: fetching = 0x%llx\n", addr);
#endif

	 fetch_unaligned(& in[idx], addr, bytes_per_entry);
	 idx += quads_per_entry;
      }

      /* Be nice and zero out any missing vertices.
       */
      (void) memset(& in[idx], 0, (8 - idx) * sizeof(qword));


      /* Convert all 4 vertices to vectors of float.
       */
      (*fetch)(&machine->Inputs[attr].xyzw[0].q, in);
   }
}


void spu_update_vertex_fetch( struct spu_vs_context *draw )
{
   unsigned i;

   
   /* Invalidate the vertex cache.
    */
   for (i = 0; i < (CACHE_NWAY * CACHE_NSETS); i++) {
      CACHELINE_CLEARVALID(i);
   }


   for (i = 0; i < draw->vertex_fetch.nr_attrs; i++) {
      draw->vertex_fetch.fetch[i] =
          get_fetch_func(draw->vertex_fetch.format[i]);
      draw->vertex_fetch.size[i] =
          get_vertex_size(draw->vertex_fetch.format[i]);
   }

   draw->vertex_fetch.fetch_func = generic_vertex_fetch;
}
