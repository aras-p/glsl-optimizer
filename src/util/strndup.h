/*
 * Copyright (c) 2015 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#ifndef STRNDUP_H
#define STRNDUP_H

#if defined(_WIN32)

#include <stdlib.h> // size_t
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

char *
strndup(const char *str, size_t max);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* STRNDUP_H */
