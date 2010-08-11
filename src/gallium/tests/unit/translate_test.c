/**************************************************************************
 *
 * Copyright © 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <stdio.h>
#include <translate/translate.h>
#include <util/u_memory.h>
#include <util/u_format.h>
#include <util/u_cpu_detect.h>
#include <rtasm/rtasm_cpu.h>

int main(int argc, char** argv)
{
   struct translate *(*create_fn)(const struct translate_key *key) = 0;

   struct translate_key key;
   unsigned output_format;
   unsigned input_format;
   unsigned char* buffer[5];
   unsigned count = 4;
   unsigned i, j, k;
   unsigned passed = 0;
   unsigned total = 0;
   const float error = 0.03125;

   create_fn = 0;

   util_cpu_detect();

   if(argc <= 1)
   {}
   else if (!strcmp(argv[1], "generic"))
      create_fn = translate_generic_create;
   else if (!strcmp(argv[1], "x86"))
      create_fn = translate_sse2_create;
   else if (!strcmp(argv[1], "nosse"))
   {
      util_cpu_caps.has_sse = 0;
      util_cpu_caps.has_sse2 = 0;
      util_cpu_caps.has_sse3 = 0;
      util_cpu_caps.has_sse4_1 = 0;
      create_fn = translate_sse2_create;
   }
   else if (!strcmp(argv[1], "sse"))
   {
      if(!util_cpu_caps.has_sse || !rtasm_cpu_has_sse())
      {
         printf("Error: CPU doesn't support SSE (test with qemu)\n");
         return 2;
      }
      util_cpu_caps.has_sse2 = 0;
      util_cpu_caps.has_sse3 = 0;
      util_cpu_caps.has_sse4_1 = 0;
      create_fn = translate_sse2_create;
   }
   else if (!strcmp(argv[1], "sse2"))
   {
      if(!util_cpu_caps.has_sse2 || !rtasm_cpu_has_sse())
      {
         printf("Error: CPU doesn't support SSE2 (test with qemu)\n");
         return 2;
      }
      util_cpu_caps.has_sse3 = 0;
      util_cpu_caps.has_sse4_1 = 0;
      create_fn = translate_sse2_create;
   }
   else if (!strcmp(argv[1], "sse3"))
   {
      if(!util_cpu_caps.has_sse3 || !rtasm_cpu_has_sse())
      {
         printf("Error: CPU doesn't support SSE3 (test with qemu)\n");
         return 2;
      }
      util_cpu_caps.has_sse4_1 = 0;
      create_fn = translate_sse2_create;
   }
   else if (!strcmp(argv[1], "sse4.1"))
   {
      if(!util_cpu_caps.has_sse4_1 || !rtasm_cpu_has_sse())
      {
         printf("Error: CPU doesn't support SSE4.1 (test with qemu)\n");
         return 2;
      }
      create_fn = translate_sse2_create;
   }

   if (!create_fn)
   {
      printf("Usage: ./translate_test [generic|x86|nosse|sse|sse2|sse3|sse4.1]\n");
      return 2;
   }

   for (i = 0; i < Elements(buffer); ++i)
      buffer[i] = align_malloc(4096, 4096);

   key.nr_elements = 1;
   key.element[0].input_buffer = 0;
   key.element[0].input_offset = 0;
   key.element[0].output_offset = 0;
   key.element[0].type = TRANSLATE_ELEMENT_NORMAL;
   key.element[0].instance_divisor = 0;

   srand(4359025);
   for (i = 0; i < 4096; ++i)
      buffer[0][i] = rand() & 0x7f; /* avoid negative values that work badly when converted to unsigned format*/

   for (output_format = 1; output_format < PIPE_FORMAT_COUNT; ++output_format)
   {
      const struct util_format_description* output_format_desc = util_format_description(output_format);
      unsigned output_format_size;
      if (!output_format_desc
            || !output_format_desc->fetch_rgba_float
            || !output_format_desc->pack_rgba_float
            || output_format_desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB
            || output_format_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN
            || !translate_is_output_format_supported(output_format))
         continue;

      output_format_size = util_format_get_stride(output_format, 1);

      for (input_format = 1; input_format < PIPE_FORMAT_COUNT; ++input_format)
      {
         const struct util_format_description* input_format_desc = util_format_description(input_format);
         unsigned input_format_size;
         struct translate* translate[2];
         unsigned fail = 0;
         unsigned used_generic = 0;

         if (!input_format_desc
               || !input_format_desc->fetch_rgba_float
               || !input_format_desc->pack_rgba_float
               || input_format_desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB
               || input_format_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN
               || !translate_is_output_format_supported(input_format))
            continue;

         input_format_size = util_format_get_stride(input_format, 1);

         key.element[0].input_format = input_format;
         key.element[0].output_format = output_format;
         key.output_stride = output_format_size;
         translate[0] = create_fn(&key);
         if (!translate[0])
            continue;

         key.element[0].input_format = output_format;
         key.element[0].output_format = input_format;
         key.output_stride = input_format_size;
         translate[1] = create_fn(&key);
         if(!translate[1])
         {
            used_generic = 1;
            translate[1] = translate_generic_create(&key);
            if(!translate[1])
               continue;
         }

         translate[0]->set_buffer(translate[0], 0, buffer[0], input_format_size, ~0);
         translate[0]->run(translate[0], 0, count, 0, buffer[1]);
         translate[1]->set_buffer(translate[1], 0, buffer[1], output_format_size, ~0);
         translate[1]->run(translate[1], 0, count, 0, buffer[2]);
         translate[0]->set_buffer(translate[0], 0, buffer[2], input_format_size, ~0);
         translate[0]->run(translate[0], 0, count, 0, buffer[3]);
         translate[1]->set_buffer(translate[1], 0, buffer[3], output_format_size, ~0);
         translate[1]->run(translate[1], 0, count, 0, buffer[4]);

         for (i = 0; i < count; ++i)
         {
            float a[4];
            float b[4];
            input_format_desc->fetch_rgba_float(a, buffer[2] + i * input_format_size, 0, 0);
            input_format_desc->fetch_rgba_float(b, buffer[4] + i * input_format_size, 0, 0);

            for (j = 0; j < count; ++j)
            {
               float d = a[j] - b[j];
               if (d > error || d < -error)
               {
                  fail = 1;
                  break;
               }
            }
         }

         printf("%s%s: %s -> %s -> %s -> %s -> %s\n",
               fail ? "FAIL" : "PASS",
               used_generic ? "[GENERIC]" : "",
               input_format_desc->name, output_format_desc->name, input_format_desc->name, output_format_desc->name, input_format_desc->name);

         if (fail)
         {
            for (i = 0; i < Elements(buffer); ++i)
            {
               unsigned format_size = (i & 1) ? output_format_size : input_format_size;
               printf("%c ", (i == 2 || i == 4) ? '*' : ' ');
               for (j = 0; j < count; ++j)
               {
                  for (k = 0; k < format_size; ++k)
                  {
                     printf("%02x", buffer[i][j * format_size + k]);
                  }
                  printf(" ");
               }
               printf("\n");
            }
         }

         if (!fail)
            ++passed;
         ++total;

         if(translate[1])
            translate[1]->release(translate[1]);
         translate[0]->release(translate[0]);
      }
   }

   printf("%u/%u tests passed for translate_%s\n", passed, total, argv[1]);
   return passed != total;
}
