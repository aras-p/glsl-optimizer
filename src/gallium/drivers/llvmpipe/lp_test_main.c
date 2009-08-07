/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


/**
 * @file
 * Shared testing code.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "lp_bld_const.h"
#include "lp_test.h"


void
dump_type(FILE *fp,
          union lp_type type)
{
   fprintf(fp, "%s%u%sx%u",
           type.floating ? "f" : (type.fixed ? "h" : (type.sign ? "s" : "u")),
           type.width,
           type.norm ? "n" : "",
           type.length);
}


double
read_elem(union lp_type type, const void *src, unsigned index)
{
   double scale = lp_const_scale(type);
   double value;
   assert(index < type.length);
   if (type.floating) {
      switch(type.width) {
      case 32:
         value = *((const float *)src + index);
         break;
      case 64:
         value =  *((const double *)src + index);
         break;
      default:
         assert(0);
         return 0.0;
      }
   }
   else {
      if(type.sign) {
         switch(type.width) {
         case 8:
            value = *((const int8_t *)src + index);
            break;
         case 16:
            value = *((const int16_t *)src + index);
            break;
         case 32:
            value = *((const int32_t *)src + index);
            break;
         case 64:
            value = *((const int64_t *)src + index);
            break;
         default:
            assert(0);
            return 0.0;
         }
      }
      else {
         switch(type.width) {
         case 8:
            value = *((const uint8_t *)src + index);
            break;
         case 16:
            value = *((const uint16_t *)src + index);
            break;
         case 32:
            value = *((const uint32_t *)src + index);
            break;
         case 64:
            value = *((const uint64_t *)src + index);
            break;
         default:
            assert(0);
            return 0.0;
         }
      }
   }
   return value/scale;
}


void
write_elem(union lp_type type, void *dst, unsigned index, double src)
{
   double scale = lp_const_scale(type);
   double value = scale*src;
   assert(index < type.length);
   if (type.floating) {
      switch(type.width) {
      case 32:
         *((float *)dst + index) = (float)(value);
         break;
      case 64:
          *((double *)dst + index) = value;
         break;
      default:
         assert(0);
      }
   }
   else {
      if(type.sign) {
         switch(type.width) {
         case 8:
            *((int8_t *)dst + index) = (int8_t)round(value);
            break;
         case 16:
            *((int16_t *)dst + index) = (int16_t)round(value);
            break;
         case 32:
            *((int32_t *)dst + index) = (int32_t)round(value);
            break;
         case 64:
            *((int64_t *)dst + index) = (int32_t)round(value);
            break;
         default:
            assert(0);
         }
      }
      else {
         switch(type.width) {
         case 8:
            *((uint8_t *)dst + index) = (uint8_t)round(value);
            break;
         case 16:
            *((uint16_t *)dst + index) = (uint16_t)round(value);
            break;
         case 32:
            *((uint32_t *)dst + index) = (uint32_t)round(value);
            break;
         case 64:
            *((uint64_t *)dst + index) = (uint64_t)round(value);
            break;
         default:
            assert(0);
         }
      }
   }
}


void
random_elem(union lp_type type, void *dst, unsigned index)
{
   assert(index < type.length);
   if (type.floating) {
      double value = (double)random()/(double)RAND_MAX;
      if(!type.norm) {
         value += (double)random();
         if(random() & 1)
            value = -value;
      }
      switch(type.width) {
      case 32:
         *((float *)dst + index) = (float)value;
         break;
      case 64:
          *((double *)dst + index) = value;
         break;
      default:
         assert(0);
      }
   }
   else {
      switch(type.width) {
      case 8:
         *((uint8_t *)dst + index) = (uint8_t)random();
         break;
      case 16:
         *((uint16_t *)dst + index) = (uint16_t)random();
         break;
      case 32:
         *((uint32_t *)dst + index) = (uint32_t)random();
         break;
      case 64:
         *((uint64_t *)dst + index) = (uint64_t)random();
         break;
      default:
         assert(0);
      }
   }
}


void
read_vec(union lp_type type, const void *src, double *dst)
{
   unsigned i;
   for (i = 0; i < type.length; ++i)
      dst[i] = read_elem(type, src, i);
}


void
write_vec(union lp_type type, void *dst, const double *src)
{
   unsigned i;
   for (i = 0; i < type.length; ++i)
      write_elem(type, dst, i, src[i]);
}


float
random_float(void)
{
    return (float)((double)random()/(double)RAND_MAX);
}


void
random_vec(union lp_type type, void *dst)
{
   unsigned i;
   for (i = 0; i < type.length; ++i)
      random_elem(type, dst, i);
}


boolean
compare_vec(union lp_type type, const void *res, const void *ref)
{
   double eps;
   unsigned i;

   if (type.floating) {
      switch(type.width) {
      case 32:
         eps = FLT_EPSILON;
         break;
      case 64:
         eps = DBL_EPSILON;
         break;
      default:
         assert(0);
         eps = 0.0;
         break;
      }
   }
   else {
      double scale = lp_const_scale(type);
      eps = 1.0/scale;
   }

   for (i = 0; i < type.length; ++i) {
      double res_elem = read_elem(type, res, i);
      double ref_elem = read_elem(type, ref, i);
      double delta = fabs(res_elem - ref_elem);
      if(delta >= 2.0*eps)
         return FALSE;
   }

   return TRUE;
}


void
dump_vec(FILE *fp, union lp_type type, const void *src)
{
   unsigned i;
   for (i = 0; i < type.length; ++i) {
      if(i)
         fprintf(fp, " ");
      if (type.floating) {
         double value;
         switch(type.width) {
         case 32:
            value = *((const float *)src + i);
            break;
         case 64:
            value = *((const double *)src + i);
            break;
         default:
            assert(0);
            value = 0.0;
         }
         fprintf(fp, "%f", value);
      }
      else {
         if(type.sign) {
            long long value;
            const char *format;
            switch(type.width) {
            case 8:
               value = *((const int8_t *)src + i);
               format = "%3lli";
               break;
            case 16:
               value = *((const int16_t *)src + i);
               format = "%5lli";
               break;
            case 32:
               value = *((const int32_t *)src + i);
               format = "%10lli";
               break;
            case 64:
               value = *((const int64_t *)src + i);
               format = "%20lli";
               break;
            default:
               assert(0);
               value = 0.0;
               format = "?";
            }
            fprintf(fp, format, value);
         }
         else {
            unsigned long long value;
            const char *format;
            switch(type.width) {
            case 8:
               value = *((const uint8_t *)src + i);
               format = "%4llu";
               break;
            case 16:
               value = *((const uint16_t *)src + i);
               format = "%6llu";
               break;
            case 32:
               value = *((const uint32_t *)src + i);
               format = "%11llu";
               break;
            case 64:
               value = *((const uint64_t *)src + i);
               format = "%21llu";
               break;
            default:
               assert(0);
               value = 0.0;
               format = "?";
            }
            fprintf(fp, format, value);
         }
      }
   }
}


int main(int argc, char **argv)
{
   unsigned verbose = 0;
   FILE *fp = NULL;
   unsigned long n = 1000;
   unsigned i;
   boolean success;

   for(i = 1; i < argc; ++i) {
      if(strcmp(argv[i], "-v") == 0)
         ++verbose;
      else if(strcmp(argv[i], "-o") == 0)
         fp = fopen(argv[++i], "wt");
      else
         n = atoi(argv[i]);
   }

   if(fp) {
      /* Warm up the caches */
      test_some(0, NULL, 100);

      write_tsv_header(fp);
   }
      
   if(n)
      success = test_some(verbose, fp, n);
   else
      success = test_all(verbose, fp);

   if(fp)
      fclose(fp);

   return success ? 0 : 1;
}
