/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2025 Yonggang Luo
 * SPDX-License-Identifier: MIT
 */

#include "strndup.h"

#if defined(_WIN32)
char *
strndup(const char *str, size_t max)
{
   size_t n;
   char *ptr;

   if (!str)
      return NULL;

   n = strnlen(str, max);
   ptr = (char *) calloc(n + 1, sizeof(char));
   if (!ptr)
      return NULL;

   memcpy(ptr, str, n);
   return ptr;
}
#endif
