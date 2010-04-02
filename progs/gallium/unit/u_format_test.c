/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include "util/u_half.h"
#include "util/u_format.h"
#include "util/u_format_tests.h"
#include "util/u_format_s3tc.h"


static boolean
compare_float(float x, float y)
{
   float error = y - x;

   if (error < 0.0f)
      error = -error;

   if (error > FLT_EPSILON) {
      return FALSE;
   }

   return TRUE;
}


static void
print_packed(const struct util_format_description *format_desc,
             const char *prefix,
             const uint8_t *packed,
             const char *suffix)
{
   unsigned i;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.bits/8; ++i) {
      printf("%s%02x", sep, packed[i]);
      sep = " ";
   }
   printf("%s", suffix);
}


static void
print_unpacked_doubl(const struct util_format_description *format_desc,
                     const char *prefix,
                     const double unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4],
                     const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s{%f, %f, %f, %f}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         sep = ", ";
      }
      sep = ",\n";
   }
   printf("%s", suffix);
}


static void
print_unpacked_float(const struct util_format_description *format_desc,
                     const char *prefix,
                     const float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4],
                     const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s{%f, %f, %f, %f}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         sep = ", ";
      }
      sep = ",\n";
   }
   printf("%s", suffix);
}


static void
print_unpacked_8unorm(const struct util_format_description *format_desc,
                      const char *prefix,
                      const uint8_t unpacked[][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4],
                      const char *suffix)
{
   unsigned i, j;
   const char *sep = "";

   printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         printf("%s{0x%02x, 0x%02x, 0x%02x, 0x%02x}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         sep = ", ";
      }
   }
   printf("%s", suffix);
}


static boolean
test_format_fetch_float(const struct util_format_description *format_desc,
                        const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
   boolean success;

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         format_desc->fetch_float(unpacked[i][j], test->packed, j, i);
         for (k = 0; k < 4; ++k) {
            if (!compare_float(test->unpacked[i][j][k], unpacked[i][j][k])) {
               success = FALSE;
            }
         }
      }
   }

   if (!success) {
      print_unpacked_float(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_doubl(format_desc, "        ", test->unpacked, " expected\n");
   }

   return success;
}


static boolean
test_format_unpack_float(const struct util_format_description *format_desc,
                         const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
   boolean success;

   format_desc->unpack_float(&unpacked[0][0][0], sizeof unpacked[0], test->packed, 0, format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         for (k = 0; k < 4; ++k) {
            if (!compare_float(test->unpacked[i][j][k], unpacked[i][j][k])) {
               success = FALSE;
            }
         }
      }
   }

   if (!success) {
      print_unpacked_float(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_doubl(format_desc, "        ", test->unpacked, " expected\n");
   }

   return success;
}


static boolean

test_format_pack_float(const struct util_format_description *format_desc,
                       const struct util_format_test_case *test)
{
   float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j, k;
   boolean success;

   if (test->format == PIPE_FORMAT_DXT1_RGBA) {
      /*
       * Skip S3TC as packed representation is not canonical.
       *
       * TODO: Do a round trip conversion.
       */
      return TRUE;
   }

   memset(packed, 0, sizeof packed);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         for (k = 0; k < 4; ++k) {
            unpacked[i][j][k] = (float) test->unpacked[i][j][k];
         }
      }
   }

   format_desc->pack_float(packed, 0, &unpacked[0][0][0], sizeof unpacked[0], format_desc->block.width, format_desc->block.height);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


static boolean
convert_float_to_8unorm(uint8_t *dst, const double *src)
{
   unsigned i;
   boolean accurate = TRUE;

   for (i = 0; i < UTIL_FORMAT_MAX_UNPACKED_HEIGHT*UTIL_FORMAT_MAX_UNPACKED_WIDTH*4; ++i) {
      if (src[i] < 0.0) {
         accurate = FALSE;
         dst[i] = 0;
      }
      else if (src[i] > 1.0) {
         accurate = FALSE;
         dst[i] = 255;
      }
      else {
         dst[i] = src[i] * 255.0;
      }
   }

   return accurate;
}


static boolean
test_format_unpack_8unorm(const struct util_format_description *format_desc,
                          const struct util_format_test_case *test)
{
   uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   uint8_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
   boolean success;

   format_desc->unpack_8unorm(&unpacked[0][0][0], sizeof unpacked[0], test->packed, 0, 1, 1);

   convert_float_to_8unorm(&expected[0][0][0], &test->unpacked[0][0][0]);

   success = TRUE;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
         for (k = 0; k < 4; ++k) {
            if (expected[i][j][k] != unpacked[i][j][k]) {
               success = FALSE;
            }
         }
      }
   }

   if (!success) {
      print_unpacked_8unorm(format_desc, "FAILED: ", unpacked, " obtained\n");
      print_unpacked_8unorm(format_desc, "        ", expected, " expected\n");
   }

   return success;
}


static boolean
test_format_pack_8unorm(const struct util_format_description *format_desc,
                        const struct util_format_test_case *test)
{
   uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i;
   boolean success;

   if (test->format == PIPE_FORMAT_DXT1_RGBA) {
      /*
       * Skip S3TC as packed representation is not canonical.
       *
       * TODO: Do a round trip conversion.
       */
      return TRUE;
   }

   if (!convert_float_to_8unorm(&unpacked[0][0][0], &test->unpacked[0][0][0])) {
      /*
       * Skip test cases which cannot be represented by four unorm bytes.
       */
      return TRUE;
   }

   memset(packed, 0, sizeof packed);

   format_desc->pack_8unorm(packed, 0, &unpacked[0][0][0], sizeof unpacked[0], 1, 1);

   success = TRUE;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
      print_packed(format_desc, "        ", test->packed, " expected\n");
   }

   return success;
}


typedef boolean
(*test_func_t)(const struct util_format_description *format_desc,
               const struct util_format_test_case *test);


static boolean
test_one(test_func_t func, const char *suffix)
{
   enum pipe_format last_format = PIPE_FORMAT_NONE;
   unsigned i;
   bool success = TRUE;

   for (i = 0; i < util_format_nr_test_cases; ++i) {
      const struct util_format_test_case *test = &util_format_test_cases[i];
      const struct util_format_description *format_desc;
      bool skip = FALSE;

      format_desc = util_format_description(test->format);

      if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC &&
          !util_format_s3tc_enabled) {
         skip = TRUE;
      }

      if (test->format != last_format) {
         printf("%s util_format_%s_%s ...\n",
                skip ? "Skipping" : "Testing", format_desc->short_name, suffix);
         last_format = test->format;
      }

      if (!skip) {
         if (!func(format_desc, &util_format_test_cases[i])) {
           success = FALSE;
         }
      }
   }

   return success;
}


static boolean
test_all(void)
{
   bool success = TRUE;

   if (!test_one(&test_format_fetch_float, "fetch_float"))
     success = FALSE;

   if (!test_one(&test_format_pack_float, "pack_float"))
     success = FALSE;

   if (!test_one(&test_format_unpack_float, "unpack_float"))
     success = FALSE;

   if (!test_one(&test_format_pack_8unorm, "pack_8unorm"))
     success = FALSE;

   if (!test_one(&test_format_unpack_8unorm, "unpack_8unorm"))
     success = FALSE;

   return success;
}


int main(int argc, char **argv)
{
   boolean success;

   util_format_s3tc_init();

   success = test_all();

   return success ? 0 : 1;
}
