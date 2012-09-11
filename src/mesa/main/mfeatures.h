/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file mfeatures.h
 * Flags to enable/disable specific parts of the API.
 */

#ifndef FEATURES_H
#define FEATURES_H

#ifndef _HAVE_FULL_GL
#define _HAVE_FULL_GL 1
#endif

/* assert that a feature is disabled and should never be used */
#define ASSERT_NO_FEATURE() ASSERT(0)

#ifndef FEATURE_ES1
#define FEATURE_ES1 0
#endif
#ifndef FEATURE_ES2
#define FEATURE_ES2 0
#endif

#define FEATURE_ES (FEATURE_ES1 || FEATURE_ES2)

#ifndef FEATURE_GL
#define FEATURE_GL !FEATURE_ES
#endif

#if defined(IN_DRI_DRIVER) || (FEATURE_GL + FEATURE_ES1 + FEATURE_ES2 > 1)
#define FEATURE_remap_table               1
#else
#define FEATURE_remap_table               0
#endif

#endif /* FEATURES_H */
