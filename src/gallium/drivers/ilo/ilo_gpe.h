/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_GPE_H
#define ILO_GPE_H

#include "ilo_common.h"

/**
 * \see brw_context.h
 */
#define ILO_MAX_DRAW_BUFFERS    8
#define ILO_MAX_CONST_BUFFERS   (1 + 12)
#define ILO_MAX_SAMPLER_VIEWS   16
#define ILO_MAX_SAMPLERS        16
#define ILO_MAX_SO_BINDINGS     64
#define ILO_MAX_SO_BUFFERS      4

#define ILO_MAX_VS_SURFACES        (ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_VS_CONST_SURFACE(i)    (i)
#define ILO_VS_TEXTURE_SURFACE(i)  (ILO_MAX_CONST_BUFFERS  + i)

#define ILO_MAX_GS_SURFACES        (ILO_MAX_SO_BINDINGS)
#define ILO_GS_SO_SURFACE(i)       (i)

#define ILO_MAX_WM_SURFACES        (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_WM_DRAW_SURFACE(i)     (i)
#define ILO_WM_CONST_SURFACE(i)    (ILO_MAX_DRAW_BUFFERS + i)
#define ILO_WM_TEXTURE_SURFACE(i)  (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS  + i)

#endif /* ILO_GPE_H */
