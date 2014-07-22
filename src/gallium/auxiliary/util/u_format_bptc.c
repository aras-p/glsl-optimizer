#include "u_format.h"
#include "u_format_bptc.h"

#define fake(format) \
void \
util_format_##format##_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j) {assert(0);} \
\
void \
util_format_##format##_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height) {assert(0);} \
\
void \
util_format_##format##_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height) {assert(0);} \
\
void \
util_format_##format##_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height) {assert(0);} \
\
void \
util_format_##format##_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height) {assert(0);} \
\
void \
util_format_##format##_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j) {assert(0);}

fake(bptc_rgba_unorm)
fake(bptc_srgba)
fake(bptc_rgb_float)
fake(bptc_rgb_ufloat)
