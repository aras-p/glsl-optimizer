//
// Copyright 2020 Serge Martin
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Extract from Serge's printf clover code by airlied.

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "hash_table.h"
#include "macros.h"
#include "ralloc.h"
#include "simple_mtx.h"
#include "strndup.h"
#include "u_math.h"
#include "u_printf.h"

#include "util/half_float.h"

#include "util/hash_table.h" /* glsl-optimizer change: no xxhash.h */

/* Some versions of MinGW are missing _vscprintf's declaration, although they
 * still provide the symbol in the import library. */
#ifdef __MINGW32__
_CRTIMP int _vscprintf(const char *format, va_list argptr);
#endif

const char*
util_printf_prev_tok(const char *str)
{
   while (*str != '%')
      str--;
   return str;
}

size_t util_printf_next_spec_pos(const char *str, size_t pos)
{
   if (str == NULL)
      return -1;

   const char *str_found = str + pos;
   do {
      str_found = strchr(str_found, '%');
      if (str_found == NULL)
         return -1;

      ++str_found;
      if (*str_found == '%') {
         ++str_found;
         continue;
      }

      const char *spec_pos = strpbrk(str_found, "cdieEfFgGaAosuxXp%");
      if (spec_pos == NULL) {
         return -1;
      } else if (*spec_pos == '%') {
         str_found = spec_pos;
      } else {
         return spec_pos - str;
      }
   } while (1);
}

size_t u_printf_length(const char *fmt, va_list untouched_args)
{
   int size;
   char junk;

   /* Make a copy of the va_list so the original caller can still use it */
   va_list args;
   va_copy(args, untouched_args);

#ifdef _WIN32
   /* We need to use _vcsprintf to calculate the size as vsnprintf returns -1
    * if the number of characters to write is greater than count.
    */
   size = _vscprintf(fmt, args);
   (void)junk;
#else
   size = vsnprintf(&junk, 1, fmt, args);
#endif
   assert(size >= 0);

   va_end(args);

   return size;
}

/**
 * Used to print plain format strings without arguments as some post-processing
 * will be required:
 *  - %% needs to be printed as %
 */
static void
u_printf_plain_sized(FILE *out, const char* format, size_t len)
{
   bool found = false;
   size_t last = 0;

   for (size_t i = 0; i < len; i++) {
      if (!found && format[i] == '%') {
         found = true;
      } else if (found && format[i] == '%') {
         /* print one character less so we only print a single % */
         fwrite(format + last, i - last - 1, 1, out);

         last = i;
         found = false;
      } else {
         /* We should never end up here with an actual format token */
         assert(!found);
         found = false;
      }
   }

   fwrite(format + last, len - last, 1, out);
}

static void
u_printf_plain(FILE *out, const char* format)
{
   u_printf_plain_sized(out, format, strlen(format));
}

static void
u_printf_impl(FILE *out, const char *buffer, size_t buffer_size,
              const u_printf_info *info,
              const u_printf_info **info_ptr,
              unsigned info_size)
{
   bool use_singleton = info == NULL && info_ptr == NULL;
   for (size_t buf_pos = 0; buf_pos < buffer_size;) {
      uint32_t fmt_idx = *(uint32_t*)&buffer[buf_pos];

      /* Don't die on invalid printf buffers due to aborted shaders. */
      if (fmt_idx == 0)
         break;

      /* the idx is 1 based, and hashes are nonzero */
      assert(fmt_idx > 0);

      const u_printf_info *fmt;
      if (use_singleton) {
         fmt = u_printf_singleton_search(fmt_idx /* hash */);
         if (!fmt)
            return;
      } else {
         fmt_idx -= 1;

         if (fmt_idx >= info_size)
            return;

         fmt = info != NULL ?  &info[fmt_idx] : info_ptr[fmt_idx];
      }

      const char *format = fmt->strings;
      buf_pos += sizeof(fmt_idx);

      if (!fmt->num_args) {
         u_printf_plain(out, format);
         continue;
      }

      for (int i = 0; i < fmt->num_args; i++) {
         int arg_size = fmt->arg_sizes[i];
         size_t spec_pos = util_printf_next_spec_pos(format, 0);

         /* If we hit an unused argument we skip all remaining ones */
         if (spec_pos == -1)
            break;

         const char *token = util_printf_prev_tok(&format[spec_pos]);
         const char *next_format = &format[spec_pos + 1];

         /* print the part before the format token */
         if (token != format)
            u_printf_plain_sized(out, format, token - format);

         char *print_str = strndup(token, next_format - token);
         /* rebase spec_pos so we can use it with print_str */
         spec_pos += format - token;

         /* print the formatted part */
         if (print_str[spec_pos] == 's') {
            uint64_t idx;
            memcpy(&idx, &buffer[buf_pos], 8);
            fprintf(out, print_str, &fmt->strings[idx]);

         /* Never pass a 'n' spec to the host printf */
         } else if (print_str[spec_pos] != 'n') {
            char *vec_pos = strchr(print_str, 'v');
            char *mod_pos = strpbrk(print_str, "hl");

            int component_count = 1;
            if (vec_pos != NULL) {
               /* non vector part of the format */
               size_t base = mod_pos ? mod_pos - print_str : spec_pos;
               size_t l = base - (vec_pos - print_str) - 1;
               char *vec = strndup(&vec_pos[1], l);
               component_count = atoi(vec);
               free(vec);

               /* remove the vector and precision stuff */
               memmove(&print_str[vec_pos - print_str], &print_str[spec_pos], 2);
            }

            /* in fact vec3 are vec4 */
            int men_components = component_count == 3 ? 4 : component_count;
            size_t elmt_size = arg_size / men_components;
            bool is_float = strpbrk(print_str, "fFeEgGaA") != NULL;

            for (int i = 0; i < component_count; i++) {
               size_t elmt_buf_pos = buf_pos + i * elmt_size;
               switch (elmt_size) {
               case 1: {
                  uint8_t v;
                  memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
                  fprintf(out, print_str, v);
                  break;
               }
               case 2: {
                  uint16_t v;
                  memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
                  if (is_float) {
                     fprintf(out, print_str, _mesa_half_to_float(v));
                  } else {
                     fprintf(out, print_str, v);
                  }
                  break;
               }
               case 4: {
                  if (is_float) {
                     float v;
                     memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
                     fprintf(out, print_str, v);
                  } else {
                     uint32_t v;
                     memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
                     fprintf(out, print_str, v);
                  }
                  break;
               }
               case 8: {
                  if (is_float) {
                     double v;
                     memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
                     fprintf(out, print_str, v);
                  } else {
                     uint64_t v;
                     memcpy(&v, &buffer[elmt_buf_pos], elmt_size);
                     fprintf(out, print_str, v);
                  }
                  break;
               }
               default:
                  assert(false);
                  break;
               }

               if (i < component_count - 1)
                  fprintf(out, ",");
            }
         }

         /* rebase format */
         format = next_format;
         free(print_str);

         buf_pos += arg_size;
         buf_pos = align_uintptr(buf_pos, 4);
      }

      /* print remaining */
      u_printf_plain(out, format);
   }
}

void u_printf(FILE *out, const char *buffer, size_t buffer_size,
              const u_printf_info *info, unsigned info_size)
{
   u_printf_impl(out, buffer, buffer_size, info, NULL, info_size);
}

void u_printf_ptr(FILE *out, const char *buffer, size_t buffer_size,
                  const u_printf_info **info, unsigned info_size)
{
   u_printf_impl(out, buffer, buffer_size, NULL, info, info_size);
}

void
u_printf_serialize_info(struct blob *blob,
                        const u_printf_info *printf_info,
                        unsigned printf_info_count)
{
   blob_write_uint32(blob, printf_info_count);
   for (int i = 0; i < printf_info_count; i++) {
      const u_printf_info *info = &printf_info[i];
      blob_write_uint32(blob, info->num_args);
      blob_write_uint32(blob, info->string_size);
      blob_write_bytes(blob, info->arg_sizes,
                       info->num_args * sizeof(info->arg_sizes[0]));
      /* we can't use blob_write_string, because it contains multiple NULL
       * terminated strings */
      blob_write_bytes(blob, info->strings, info->string_size);
   }
}

u_printf_info *
u_printf_deserialize_info(void *mem_ctx,
                          struct blob_reader *blob,
                          unsigned *printf_info_count)
{
   *printf_info_count = blob_read_uint32(blob);

   u_printf_info *printf_info =
      ralloc_array(mem_ctx, u_printf_info, *printf_info_count);

   for (int i = 0; i < *printf_info_count; i++) {
      u_printf_info *info = &printf_info[i];
      info->num_args = blob_read_uint32(blob);
      info->string_size = blob_read_uint32(blob);
      info->arg_sizes = ralloc_array(printf_info, unsigned, info->num_args);
      blob_copy_bytes(blob, info->arg_sizes,
                      info->num_args * sizeof(info->arg_sizes[0]));
      info->strings = ralloc_array(printf_info, char, info->string_size);
      blob_copy_bytes(blob, info->strings, info->string_size);
   }

   return printf_info;
}

/*
 * Hash the format string, allowing the driver to pool format strings.
 *
 * Post-condition: hash is nonzero. This is convenient.
 */
uint32_t
u_printf_hash(const u_printf_info *info)
{
   struct blob blob;
   blob_init(&blob);
   u_printf_serialize_info(&blob, info, 1);
   /* glsl-optimizer change: hash_table.h hash instead of xxHash */
   uint32_t hash = _mesa_hash_data(blob.data, blob.size);
   blob_finish(&blob);

   /* Force things away from zero. This weakens the hash only slightly, as
    * there's only a 2^-31 probability of hashing to either hash=0 or hash=1.
    */
   if (hash == 0) {
      hash = 1;
   }

   assert(hash != 0);
   return hash;
}

static struct {
   uint32_t users;
   struct hash_table_u64 *ht;
} u_printf_cache = {0};

static simple_mtx_t u_printf_lock = SIMPLE_MTX_INITIALIZER;

void
u_printf_singleton_init_or_ref(void)
{
   simple_mtx_lock(&u_printf_lock);

   if ((u_printf_cache.users++) == 0) {
      u_printf_cache.ht = _mesa_hash_table_u64_create(NULL);
   }

   simple_mtx_unlock(&u_printf_lock);
}

void
u_printf_singleton_decref()
{
   simple_mtx_lock(&u_printf_lock);
   assert(u_printf_cache.users > 0);

   if ((--u_printf_cache.users) == 0) {
      ralloc_free(u_printf_cache.ht);
      memset(&u_printf_cache, 0, sizeof(u_printf_cache));
   }

   simple_mtx_unlock(&u_printf_lock);
}

static void
assert_singleton_exists_and_is_locked()
{
   simple_mtx_assert_locked(&u_printf_lock);
   assert(u_printf_cache.users > 0);
}

static const u_printf_info *
u_printf_singleton_search_locked(uint32_t hash)
{
   assert_singleton_exists_and_is_locked();

   return _mesa_hash_table_u64_search(u_printf_cache.ht, hash);
}

static void
u_printf_singleton_add_locked(const u_printf_info *info)
{
   assert_singleton_exists_and_is_locked();

   /* If the format string is already known, do nothing. */
   uint32_t hash = u_printf_hash(info);
   const u_printf_info *cached = u_printf_singleton_search_locked(hash);
   if (cached != NULL) {
      assert(u_printf_hash(cached) == hash && "hash table invariant");
      assert(!strcmp(cached->strings, info->strings) && "assume no collisions");
      return;
   }

   /* Otherwise, we need to add the string to the table. Doing so requires
    * a deep-clone, so the singleton will probably outlive our parameter.
    */
   u_printf_info *clone = rzalloc(u_printf_cache.ht, u_printf_info);
   clone->num_args = info->num_args;
   clone->string_size = info->string_size;
   clone->arg_sizes = ralloc_memdup(u_printf_cache.ht, info->arg_sizes,
                                    sizeof(info->arg_sizes[0]) * info->num_args);
   clone->strings = ralloc_memdup(u_printf_cache.ht, info->strings,
                                  info->string_size);

   assert(_mesa_hash_table_u64_search(u_printf_cache.ht, hash) == NULL &&
          "no duplicates at this point");

   _mesa_hash_table_u64_insert(u_printf_cache.ht, hash, clone);
}

const u_printf_info *
u_printf_singleton_search(uint32_t hash)
{
   simple_mtx_lock(&u_printf_lock);
   const u_printf_info *info = u_printf_singleton_search_locked(hash);
   simple_mtx_unlock(&u_printf_lock);
   return info;
}

void
u_printf_singleton_add(const u_printf_info *info, unsigned count)
{
   simple_mtx_lock(&u_printf_lock);
   for (unsigned i = 0; i < count; ++i) {
      u_printf_singleton_add_locked(&info[i]);
   }
   simple_mtx_unlock(&u_printf_lock);
}

void
u_printf_singleton_add_serialized(const void *data, size_t data_size)
{
   struct blob_reader blob;
   blob_reader_init(&blob, data, data_size);

   unsigned count = 0;
   u_printf_info *info = u_printf_deserialize_info(NULL, &blob, &count);
   u_printf_singleton_add(info, count);
   ralloc_free(info);
}
