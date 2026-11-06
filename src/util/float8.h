/*
 * Copyright 2025 Valve Corporation
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t _mesa_float_to_e4m3fn(float val);
uint8_t _mesa_float_to_e4m3fn_sat(float val);
float _mesa_e4m3fn_to_float(uint8_t val);

uint8_t _mesa_float_to_e5m2(float val);
uint8_t _mesa_float_to_e5m2_sat(float val);
float _mesa_e5m2_to_float(uint8_t val);

#ifdef __cplusplus
} /* extern C */
#endif
