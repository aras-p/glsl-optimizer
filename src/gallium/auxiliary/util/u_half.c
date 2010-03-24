#include "util/u_half.h"

/* see www.fox-toolkit.org/ftp/fasthalffloatconversion.pdf
 * "Fast Half Float Conversions" by Jeroen van der Zijp, Nov 2008
 */

/* Note that using a 64K * 4 table is a terrible idea since it will not fit
 * in the L1 cache and will massively pollute the L2 cache as well
 *
 * These should instead fit in the L1 cache.
 *
 * TODO: we could use a denormal bias table instead of the mantissa/offset
 * tables: this would reduce the L1 cache usage from 8704 to 2304 bytes
 * but would involve more computation
 *
 * Note however that if denormals are never encountered, the L1 cache usage
 * is only about 4608 bytes anyway.
 */
uint32_t util_half_to_float_mantissa_table[2048];
uint32_t util_half_to_float_exponent_table[64];
uint32_t util_half_to_float_offset_table[64];
uint16_t util_float_to_half_base_table[512];
uint8_t util_float_to_half_shift_table[512];

/* called by u_gctors.cpp, which defines the prototype itself */
void util_half_init_tables(void);

void util_half_init_tables(void)
{
	int i;

	/* zero */
	util_half_to_float_mantissa_table[0] = 0;

	/* denormals */
	for(i = 1; i < 1024; ++i) {
		unsigned int m = i << 13;
		unsigned int e = 0;

		/* Normalize number */
		while(!(m & 0x00800000)) {
			e -= 0x00800000;
			m<<=1;
		}
		m &= ~0x00800000;
		e+= 0x38800000;
		util_half_to_float_mantissa_table[i] = m | e;
	}

	/* normals */
	for(i = 1024; i < 2048; ++i)
		util_half_to_float_mantissa_table[i] = ((i-1024)<<13);

	/* positive zero or denormals */
	util_half_to_float_exponent_table[0] = 0;

	/* positive numbers */
	for(i = 1; i <= 30; ++i)
		util_half_to_float_exponent_table[i] = 0x38000000 + (i << 23);

	/* positive infinity/NaN */
	util_half_to_float_exponent_table[31] = 0x7f800000;

	/* negative zero or denormals */
	util_half_to_float_exponent_table[32] = 0x80000000;

	/* negative numbers */
	for(i = 33; i <= 62; ++i)
		util_half_to_float_exponent_table[i] = 0xb8000000 + ((i - 32) << 23);

	/* negative infinity/NaN */
	util_half_to_float_exponent_table[63] = 0xff800000;

	/* positive zero or denormals */
	util_half_to_float_offset_table[0] = 0;

	/* positive normals */
	for(i = 1; i < 32; ++i)
		util_half_to_float_offset_table[i] = 1024;

	/* negative zero or denormals */
	util_half_to_float_offset_table[32] = 0;

	/* negative normals */
	for(i = 33; i < 64; ++i)
		util_half_to_float_offset_table[i] = 1024;



	/* very small numbers mapping to zero */
	for(i = -127; i < -24; ++i) {
		util_float_to_half_base_table[127 + i] = 0;
		util_float_to_half_shift_table[127 + i] = 24;
	}

	/* small numbers mapping to denormals */
	for(i = -24; i < -14; ++i) {
		util_float_to_half_base_table[127 + i] = 0x0400 >> (-14 - i);
		util_float_to_half_shift_table[127 + i] = -i - 1;
	}

	/* normal numbers */
	for(i = -14; i < 16; ++i) {
		util_float_to_half_base_table[127 + i] = (i + 15) << 10;
		util_float_to_half_shift_table[127 + i] = 13;
	}

	/* large numbers mapping to infinity */
	for(i = 16; i < 128; ++i) {
		util_float_to_half_base_table[127 + i] = 0x7c00;
		util_float_to_half_shift_table[127 + i] = 24;
	}

	/* infinity and NaNs */
	util_float_to_half_base_table[255] = 0x7c00;
	util_float_to_half_shift_table[255] = 13;

	/* negative numbers */
	for(i = 0; i < 256; ++i) {
		util_float_to_half_base_table[256 + i] = util_float_to_half_base_table[i] | 0x8000;
		util_float_to_half_shift_table[256 + i] = util_float_to_half_shift_table[i];
	}
}
