/**************************************************************************
 *
 * Copyright 2011 Christian König.
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

/**
 * This file is based uppon slice_xvmc.c and vlc.h from the xine project,
 * which in turn is based on mpeg2dec. The following is the original copyright:
 *
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef vl_vlc_h
#define vl_vlc_h

struct vl_vlc
{
   uint32_t buf; /* current 32 bit working set of buffer */
   int bits;     /* used bits in working set */
   const uint8_t *ptr; /* buffer with stream data */
   const uint8_t *max; /* ptr+len of buffer */
};

static inline void
vl_vlc_restart(struct vl_vlc *vlc)
{
   vlc->buf = (vlc->ptr[0] << 24) | (vlc->ptr[1] << 16) | (vlc->ptr[2] << 8) | vlc->ptr[3];
   vlc->bits = -16;
   vlc->ptr += 4;
}

static inline void
vl_vlc_init(struct vl_vlc *vlc, const uint8_t *data, unsigned len)
{
   vlc->ptr = data;
   vlc->max = data + len;
   vl_vlc_restart(vlc);
}

static inline bool
vl_vlc_getbyte(struct vl_vlc *vlc)
{
   vlc->buf <<= 8;
   vlc->buf |= vlc->ptr[0];
   vlc->ptr++;
   return vlc->ptr < vlc->max;
}

#define vl_vlc_getword(vlc, shift)                                      \
do {                                                                    \
   (vlc)->buf |= (((vlc)->ptr[0] << 8) | (vlc)->ptr[1]) << (shift);     \
   (vlc)->ptr += 2;                                                     \
} while (0)

/* make sure that there are at least 16 valid bits in bit_buf */
#define vl_vlc_needbits(vlc)                    \
do {                                            \
    if ((vlc)->bits >= 0) {                      \
	vl_vlc_getword(vlc, (vlc)->bits);       \
	(vlc)->bits -= 16;                      \
    }                                           \
} while (0)

/* make sure that the full 32 bit of the buffer are valid */
static inline void
vl_vlc_need32bits(struct vl_vlc *vlc)
{
   vl_vlc_needbits(vlc);
   if (vlc->bits > -8) {
      unsigned n = -vlc->bits;
      vlc->buf <<= n;
      vlc->buf |= *vlc->ptr << 8;
      vlc->bits = -8;
      vlc->ptr++;
   }
   if (vlc->bits > -16) {
      unsigned n = -vlc->bits - 8;
      vlc->buf <<= n;
      vlc->buf |= *vlc->ptr;
      vlc->bits = -16;
      vlc->ptr++;
   }
}

/* remove num valid bits from bit_buf */
#define vl_vlc_dumpbits(vlc, num)       \
do {					\
    (vlc)->buf <<= (num);		\
    (vlc)->bits += (num);		\
} while (0)

/* take num bits from the high part of bit_buf and zero extend them */
#define vl_vlc_ubits(vlc, num) (((uint32_t)((vlc)->buf)) >> (32 - (num)))

/* take num bits from the high part of bit_buf and sign extend them */
#define vl_vlc_sbits(vlc, num) (((int32_t)((vlc)->buf)) >> (32 - (num)))

#endif /* vl_vlc_h */
