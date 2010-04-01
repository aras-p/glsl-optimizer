#ifndef U_HALF_H
#define U_HALF_H

#include "pipe/p_compiler.h"
#include "u_math.h"

#ifdef __cplusplus
extern "C" {
#endif


extern uint32_t util_half_to_float_mantissa_table[2048];
extern uint32_t util_half_to_float_exponent_table[64];
extern uint32_t util_half_to_float_offset_table[64];
extern uint16_t util_float_to_half_base_table[512];
extern uint8_t util_float_to_half_shift_table[512];

/*
 * Note that if the half float is a signaling NaN, the x87 FPU will turn
 * it into a quiet NaN immediately upon loading into a float.
 *
 * Additionally, denormals may be flushed to zero.
 *
 * To avoid this, use the floatui functions instead of the float ones
 * when just doing conversion rather than computation on the resulting
 * floats.
 */

static INLINE uint32_t
util_half_to_floatui(half h)
{
	unsigned exp = h >> 10;
	return util_half_to_float_mantissa_table[util_half_to_float_offset_table[exp] + (h & 0x3ff)]
		+ util_half_to_float_exponent_table[exp];
}

static INLINE float
util_half_to_float(half h)
{
	union fi r;
	r.ui = util_half_to_floatui(h);
	return r.f;
}

static INLINE half
util_floatui_to_half(uint32_t v)
{
	unsigned signexp = v >> 23;
	return util_float_to_half_base_table[signexp]
		+ ((v & 0x007fffff) >> util_float_to_half_shift_table[signexp]);
}

static INLINE half
util_float_to_half(float f)
{
	union fi i;
	i.f = f;
	return util_floatui_to_half(i.ui);
}

#ifdef __cplusplus
}
#endif

#endif /* U_HALF_H */
