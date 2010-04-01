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

#include "util/u_format.h"
#include "util/u_format_tests.h"


static boolean
test_format_fetch_float(const struct util_format_description *format_desc,
                        const struct util_format_test_case *test)
{
   float unpacked[4];
   unsigned i;
   boolean success;

   /*
    * TODO: test block formats too.
    */
   if (format_desc->block.width != 1 && format_desc->block.height != 1) {
     return TRUE;
   }

   format_desc->fetch_float(unpacked, test->packed, 0, 0);

   success = TRUE;
   for (i = 0; i < 4; ++i)
      if (test->unpacked[i] != unpacked[i])
         success = FALSE;

   if (!success) {
      printf("FAILED: (%f %f %f %f) obtained\n", unpacked[0], unpacked[1], unpacked[2], unpacked[3]);
      printf("        (%f %f %f %f) expected\n", test->unpacked[0], test->unpacked[1], test->unpacked[2], test->unpacked[3]);
   }

   return success;
}


static boolean
test_format_unpack_float(const struct util_format_description *format_desc,
                         const struct util_format_test_case *test)
{
   float unpacked[4];
   unsigned i;
   boolean success;

   format_desc->unpack_float(unpacked, 0, test->packed, 0, 1, 1);

   success = TRUE;
   for (i = 0; i < 4; ++i)
      if (test->unpacked[i] != unpacked[i])
         success = FALSE;

   if (!success) {
      printf("FAILED: (%f %f %f %f) obtained\n", unpacked[0], unpacked[1], unpacked[2], unpacked[3]);
      printf("        (%f %f %f %f) expected\n", test->unpacked[0], test->unpacked[1], test->unpacked[2], test->unpacked[3]);
   }

   return success;
}


static boolean
test_format_pack_float(const struct util_format_description *format_desc,
                       const struct util_format_test_case *test)
{
   float unpacked[4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i;
   boolean success;

   memset(packed, 0, sizeof packed);

   for (i = 0; i < 4; ++i)
      unpacked[i] = (float) test->unpacked[i];

   format_desc->pack_float(packed, 0, unpacked, 0, 1, 1);

   success = TRUE;
   for (i = 0; i < UTIL_FORMAT_MAX_PACKED_BYTES; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      /* TODO: print more than 4 bytes */
      printf("FAILED: (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x) obtained\n",
             packed[0], packed[1], packed[2], packed[3],
             packed[4], packed[5], packed[6], packed[7],
             packed[8], packed[9], packed[10], packed[11],
             packed[12], packed[13], packed[14], packed[15]);
      printf("        (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x) expected\n",
             test->packed[0], test->packed[1], test->packed[2], test->packed[3],
             test->packed[4], test->packed[5], test->packed[6], test->packed[7],
             test->packed[8], test->packed[9], test->packed[10], test->packed[11],
             test->packed[12], test->packed[13], test->packed[14], test->packed[15]);
   }

   return success;
}


static boolean
convert_float_to_8unorm(uint8_t *dst, const double *src)
{
   unsigned i;
   boolean accurate = TRUE;

   for (i = 0; i < 4; ++i) {
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
   uint8_t unpacked[4];
   uint8_t expected[4];
   unsigned i;
   boolean success;

   format_desc->unpack_8unorm(unpacked, 0, test->packed, 0, 1, 1);

   convert_float_to_8unorm(expected, test->unpacked);

   success = TRUE;
   for (i = 0; i < 4; ++i)
      if (expected[i] != unpacked[i])
         success = FALSE;

   if (!success) {
      printf("FAILED: (0x%02x 0x%02x 0x%02x 0x%02x) obtained\n", unpacked[0], unpacked[1], unpacked[2], unpacked[3]);
      printf("        (0x%02x 0x%02x 0x%02x 0x%02x) expected\n", expected[0], expected[1], expected[2], expected[3]);
   }

   return success;
}


static boolean
test_format_pack_8unorm(const struct util_format_description *format_desc,
                        const struct util_format_test_case *test)
{
   uint8_t unpacked[4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i;
   boolean success;

   if (!convert_float_to_8unorm(unpacked, test->unpacked)) {
      /*
       * Skip test cases which cannot be represented by four unorm bytes.
       */
      return TRUE;
   }

   memset(packed, 0, sizeof packed);

   format_desc->pack_8unorm(packed, 0, unpacked, 0, 1, 1);

   success = TRUE;
   for (i = 0; i < UTIL_FORMAT_MAX_PACKED_BYTES; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         success = FALSE;

   if (!success) {
      /* TODO: print more than 4 bytes */
      printf("FAILED: (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x) obtained\n",
             packed[0], packed[1], packed[2], packed[3],
             packed[4], packed[5], packed[6], packed[7],
             packed[8], packed[9], packed[10], packed[11],
             packed[12], packed[13], packed[14], packed[15]);
      printf("        (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x) expected\n",
             test->packed[0], test->packed[1], test->packed[2], test->packed[3],
             test->packed[4], test->packed[5], test->packed[6], test->packed[7],
             test->packed[8], test->packed[9], test->packed[10], test->packed[11],
             test->packed[12], test->packed[13], test->packed[14], test->packed[15]);
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
      format_desc = util_format_description(test->format);

      if (test->format != last_format) {
         printf("Testing util_format_%s_%s ...\n", format_desc->short_name, suffix);
         last_format = test->format;
      }

      if (!func(format_desc, &util_format_test_cases[i]))
        success = FALSE;
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

   success = test_all();

   return success ? 0 : 1;
}
