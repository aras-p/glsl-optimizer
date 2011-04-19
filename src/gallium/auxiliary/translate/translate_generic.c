/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "pipe/p_state.h"
#include "translate.h"


#define DRAW_DBG 0

typedef void (*fetch_func)(float *dst,
                           const uint8_t *src,
                           unsigned i, unsigned j);
typedef void (*emit_func)(const float *attrib, void *ptr);



struct translate_generic {
   struct translate translate;

   struct {
      enum translate_element_type type;

      fetch_func fetch;
      unsigned buffer;
      unsigned input_offset;
      unsigned instance_divisor;

      emit_func emit;
      unsigned output_offset;
      
      const uint8_t *input_ptr;
      unsigned input_stride;
      unsigned max_index;

      /* this value is set to -1 if this is a normal element with output_format != input_format:
       * in this case, u_format is used to do a full conversion
       *
       * this value is set to the format size in bytes if output_format == input_format or for 32-bit instance ids:
       * in this case, memcpy is used to copy this amount of bytes
       */
      int copy_size;

   } attrib[PIPE_MAX_ATTRIBS];

   unsigned nr_attrib;
};


static struct translate_generic *translate_generic( struct translate *translate )
{
   return (struct translate_generic *)translate;
}

/**
 * Fetch a float[4] vertex attribute from memory, doing format/type
 * conversion as needed.
 *
 * This is probably needed/dupliocated elsewhere, eg format
 * conversion, texture sampling etc.
 */
#define ATTRIB( NAME, SZ, TYPE, TO )	    	        \
static void						\
emit_##NAME(const float *attrib, void *ptr)		\
{  \
   unsigned i;						\
   TYPE *out = (TYPE *)ptr;				\
							\
   for (i = 0; i < SZ; i++) {				\
      out[i] = TO(attrib[i]);				\
   }							\
}


#define TO_64_FLOAT(x)   ((double) x)
#define TO_32_FLOAT(x)   (x)

#define TO_8_USCALED(x)  ((unsigned char) x)
#define TO_16_USCALED(x) ((unsigned short) x)
#define TO_32_USCALED(x) ((unsigned int) x)

#define TO_8_SSCALED(x)  ((char) x)
#define TO_16_SSCALED(x) ((short) x)
#define TO_32_SSCALED(x) ((int) x)

#define TO_8_UNORM(x)    ((unsigned char) (x * 255.0f))
#define TO_16_UNORM(x)   ((unsigned short) (x * 65535.0f))
#define TO_32_UNORM(x)   ((unsigned int) (x * 4294967295.0f))

#define TO_8_SNORM(x)    ((char) (x * 127.0f))
#define TO_16_SNORM(x)   ((short) (x * 32767.0f))
#define TO_32_SNORM(x)   ((int) (x * 2147483647.0f))

#define TO_32_FIXED(x)   ((int) (x * 65536.0f))


ATTRIB( R64G64B64A64_FLOAT,   4, double, TO_64_FLOAT )
ATTRIB( R64G64B64_FLOAT,      3, double, TO_64_FLOAT )
ATTRIB( R64G64_FLOAT,         2, double, TO_64_FLOAT )
ATTRIB( R64_FLOAT,            1, double, TO_64_FLOAT )

ATTRIB( R32G32B32A32_FLOAT,   4, float, TO_32_FLOAT )
ATTRIB( R32G32B32_FLOAT,      3, float, TO_32_FLOAT )
ATTRIB( R32G32_FLOAT,         2, float, TO_32_FLOAT )
ATTRIB( R32_FLOAT,            1, float, TO_32_FLOAT )

ATTRIB( R32G32B32A32_USCALED, 4, unsigned, TO_32_USCALED )
ATTRIB( R32G32B32_USCALED,    3, unsigned, TO_32_USCALED )
ATTRIB( R32G32_USCALED,       2, unsigned, TO_32_USCALED )
ATTRIB( R32_USCALED,          1, unsigned, TO_32_USCALED )

ATTRIB( R32G32B32A32_SSCALED, 4, int, TO_32_SSCALED )
ATTRIB( R32G32B32_SSCALED,    3, int, TO_32_SSCALED )
ATTRIB( R32G32_SSCALED,       2, int, TO_32_SSCALED )
ATTRIB( R32_SSCALED,          1, int, TO_32_SSCALED )

ATTRIB( R32G32B32A32_UNORM, 4, unsigned, TO_32_UNORM )
ATTRIB( R32G32B32_UNORM,    3, unsigned, TO_32_UNORM )
ATTRIB( R32G32_UNORM,       2, unsigned, TO_32_UNORM )
ATTRIB( R32_UNORM,          1, unsigned, TO_32_UNORM )

ATTRIB( R32G32B32A32_SNORM, 4, int, TO_32_SNORM )
ATTRIB( R32G32B32_SNORM,    3, int, TO_32_SNORM )
ATTRIB( R32G32_SNORM,       2, int, TO_32_SNORM )
ATTRIB( R32_SNORM,          1, int, TO_32_SNORM )

ATTRIB( R16G16B16A16_USCALED, 4, ushort, TO_16_USCALED )
ATTRIB( R16G16B16_USCALED,    3, ushort, TO_16_USCALED )
ATTRIB( R16G16_USCALED,       2, ushort, TO_16_USCALED )
ATTRIB( R16_USCALED,          1, ushort, TO_16_USCALED )

ATTRIB( R16G16B16A16_SSCALED, 4, short, TO_16_SSCALED )
ATTRIB( R16G16B16_SSCALED,    3, short, TO_16_SSCALED )
ATTRIB( R16G16_SSCALED,       2, short, TO_16_SSCALED )
ATTRIB( R16_SSCALED,          1, short, TO_16_SSCALED )

ATTRIB( R16G16B16A16_UNORM, 4, ushort, TO_16_UNORM )
ATTRIB( R16G16B16_UNORM,    3, ushort, TO_16_UNORM )
ATTRIB( R16G16_UNORM,       2, ushort, TO_16_UNORM )
ATTRIB( R16_UNORM,          1, ushort, TO_16_UNORM )

ATTRIB( R16G16B16A16_SNORM, 4, short, TO_16_SNORM )
ATTRIB( R16G16B16_SNORM,    3, short, TO_16_SNORM )
ATTRIB( R16G16_SNORM,       2, short, TO_16_SNORM )
ATTRIB( R16_SNORM,          1, short, TO_16_SNORM )

ATTRIB( R8G8B8A8_USCALED,   4, ubyte, TO_8_USCALED )
ATTRIB( R8G8B8_USCALED,     3, ubyte, TO_8_USCALED )
ATTRIB( R8G8_USCALED,       2, ubyte, TO_8_USCALED )
ATTRIB( R8_USCALED,         1, ubyte, TO_8_USCALED )

ATTRIB( R8G8B8A8_SSCALED,  4, char, TO_8_SSCALED )
ATTRIB( R8G8B8_SSCALED,    3, char, TO_8_SSCALED )
ATTRIB( R8G8_SSCALED,      2, char, TO_8_SSCALED )
ATTRIB( R8_SSCALED,        1, char, TO_8_SSCALED )

ATTRIB( R8G8B8A8_UNORM,  4, ubyte, TO_8_UNORM )
ATTRIB( R8G8B8_UNORM,    3, ubyte, TO_8_UNORM )
ATTRIB( R8G8_UNORM,      2, ubyte, TO_8_UNORM )
ATTRIB( R8_UNORM,        1, ubyte, TO_8_UNORM )

ATTRIB( R8G8B8A8_SNORM,  4, char, TO_8_SNORM )
ATTRIB( R8G8B8_SNORM,    3, char, TO_8_SNORM )
ATTRIB( R8G8_SNORM,      2, char, TO_8_SNORM )
ATTRIB( R8_SNORM,        1, char, TO_8_SNORM )

static void
emit_A8R8G8B8_UNORM( const float *attrib, void *ptr)
{
   ubyte *out = (ubyte *)ptr;
   out[0] = TO_8_UNORM(attrib[3]);
   out[1] = TO_8_UNORM(attrib[0]);
   out[2] = TO_8_UNORM(attrib[1]);
   out[3] = TO_8_UNORM(attrib[2]);
}

static void
emit_B8G8R8A8_UNORM( const float *attrib, void *ptr)
{
   ubyte *out = (ubyte *)ptr;
   out[2] = TO_8_UNORM(attrib[0]);
   out[1] = TO_8_UNORM(attrib[1]);
   out[0] = TO_8_UNORM(attrib[2]);
   out[3] = TO_8_UNORM(attrib[3]);
}

static void 
emit_NULL( const float *attrib, void *ptr )
{
   /* do nothing is the only sensible option */
}

static emit_func get_emit_func( enum pipe_format format )
{
   switch (format) {
   case PIPE_FORMAT_R64_FLOAT:
      return &emit_R64_FLOAT;
   case PIPE_FORMAT_R64G64_FLOAT:
      return &emit_R64G64_FLOAT;
   case PIPE_FORMAT_R64G64B64_FLOAT:
      return &emit_R64G64B64_FLOAT;
   case PIPE_FORMAT_R64G64B64A64_FLOAT:
      return &emit_R64G64B64A64_FLOAT;

   case PIPE_FORMAT_R32_FLOAT:
      return &emit_R32_FLOAT;
   case PIPE_FORMAT_R32G32_FLOAT:
      return &emit_R32G32_FLOAT;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      return &emit_R32G32B32_FLOAT;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return &emit_R32G32B32A32_FLOAT;

   case PIPE_FORMAT_R32_UNORM:
      return &emit_R32_UNORM;
   case PIPE_FORMAT_R32G32_UNORM:
      return &emit_R32G32_UNORM;
   case PIPE_FORMAT_R32G32B32_UNORM:
      return &emit_R32G32B32_UNORM;
   case PIPE_FORMAT_R32G32B32A32_UNORM:
      return &emit_R32G32B32A32_UNORM;

   case PIPE_FORMAT_R32_USCALED:
      return &emit_R32_USCALED;
   case PIPE_FORMAT_R32G32_USCALED:
      return &emit_R32G32_USCALED;
   case PIPE_FORMAT_R32G32B32_USCALED:
      return &emit_R32G32B32_USCALED;
   case PIPE_FORMAT_R32G32B32A32_USCALED:
      return &emit_R32G32B32A32_USCALED;

   case PIPE_FORMAT_R32_SNORM:
      return &emit_R32_SNORM;
   case PIPE_FORMAT_R32G32_SNORM:
      return &emit_R32G32_SNORM;
   case PIPE_FORMAT_R32G32B32_SNORM:
      return &emit_R32G32B32_SNORM;
   case PIPE_FORMAT_R32G32B32A32_SNORM:
      return &emit_R32G32B32A32_SNORM;

   case PIPE_FORMAT_R32_SSCALED:
      return &emit_R32_SSCALED;
   case PIPE_FORMAT_R32G32_SSCALED:
      return &emit_R32G32_SSCALED;
   case PIPE_FORMAT_R32G32B32_SSCALED:
      return &emit_R32G32B32_SSCALED;
   case PIPE_FORMAT_R32G32B32A32_SSCALED:
      return &emit_R32G32B32A32_SSCALED;

   case PIPE_FORMAT_R16_UNORM:
      return &emit_R16_UNORM;
   case PIPE_FORMAT_R16G16_UNORM:
      return &emit_R16G16_UNORM;
   case PIPE_FORMAT_R16G16B16_UNORM:
      return &emit_R16G16B16_UNORM;
   case PIPE_FORMAT_R16G16B16A16_UNORM:
      return &emit_R16G16B16A16_UNORM;

   case PIPE_FORMAT_R16_USCALED:
      return &emit_R16_USCALED;
   case PIPE_FORMAT_R16G16_USCALED:
      return &emit_R16G16_USCALED;
   case PIPE_FORMAT_R16G16B16_USCALED:
      return &emit_R16G16B16_USCALED;
   case PIPE_FORMAT_R16G16B16A16_USCALED:
      return &emit_R16G16B16A16_USCALED;

   case PIPE_FORMAT_R16_SNORM:
      return &emit_R16_SNORM;
   case PIPE_FORMAT_R16G16_SNORM:
      return &emit_R16G16_SNORM;
   case PIPE_FORMAT_R16G16B16_SNORM:
      return &emit_R16G16B16_SNORM;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      return &emit_R16G16B16A16_SNORM;

   case PIPE_FORMAT_R16_SSCALED:
      return &emit_R16_SSCALED;
   case PIPE_FORMAT_R16G16_SSCALED:
      return &emit_R16G16_SSCALED;
   case PIPE_FORMAT_R16G16B16_SSCALED:
      return &emit_R16G16B16_SSCALED;
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
      return &emit_R16G16B16A16_SSCALED;

   case PIPE_FORMAT_R8_UNORM:
      return &emit_R8_UNORM;
   case PIPE_FORMAT_R8G8_UNORM:
      return &emit_R8G8_UNORM;
   case PIPE_FORMAT_R8G8B8_UNORM:
      return &emit_R8G8B8_UNORM;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return &emit_R8G8B8A8_UNORM;

   case PIPE_FORMAT_R8_USCALED:
      return &emit_R8_USCALED;
   case PIPE_FORMAT_R8G8_USCALED:
      return &emit_R8G8_USCALED;
   case PIPE_FORMAT_R8G8B8_USCALED:
      return &emit_R8G8B8_USCALED;
   case PIPE_FORMAT_R8G8B8A8_USCALED:
      return &emit_R8G8B8A8_USCALED;

   case PIPE_FORMAT_R8_SNORM:
      return &emit_R8_SNORM;
   case PIPE_FORMAT_R8G8_SNORM:
      return &emit_R8G8_SNORM;
   case PIPE_FORMAT_R8G8B8_SNORM:
      return &emit_R8G8B8_SNORM;
   case PIPE_FORMAT_R8G8B8A8_SNORM:
      return &emit_R8G8B8A8_SNORM;

   case PIPE_FORMAT_R8_SSCALED:
      return &emit_R8_SSCALED;
   case PIPE_FORMAT_R8G8_SSCALED:
      return &emit_R8G8_SSCALED;
   case PIPE_FORMAT_R8G8B8_SSCALED:
      return &emit_R8G8B8_SSCALED;
   case PIPE_FORMAT_R8G8B8A8_SSCALED:
      return &emit_R8G8B8A8_SSCALED;

   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return &emit_B8G8R8A8_UNORM;

   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return &emit_A8R8G8B8_UNORM;

   default:
      assert(0); 
      return &emit_NULL;
   }
}

static ALWAYS_INLINE void PIPE_CDECL generic_run_one( struct translate_generic *tg,
                                         unsigned elt,
                                         unsigned instance_id,
                                         void *vert )
{
   unsigned nr_attrs = tg->nr_attrib;
   unsigned attr;

   for (attr = 0; attr < nr_attrs; attr++) {
      float data[4];
      uint8_t *dst = (uint8_t *)vert + tg->attrib[attr].output_offset;

      if (tg->attrib[attr].type == TRANSLATE_ELEMENT_NORMAL) {
         const uint8_t *src;
         unsigned index;
         int copy_size;

         if (tg->attrib[attr].instance_divisor) {
            index = instance_id / tg->attrib[attr].instance_divisor;
            /* XXX we need to clamp the index here too, but to a
             * per-array max value, not the draw->pt.max_index value
             * that's being given to us via translate->set_buffer().
             */
         }
         else {
            index = elt;
            /* clamp to avoid going out of bounds */
            index = MIN2(index, tg->attrib[attr].max_index);
         }

         src = tg->attrib[attr].input_ptr +
               tg->attrib[attr].input_stride * index;

         copy_size = tg->attrib[attr].copy_size;
         if(likely(copy_size >= 0))
            memcpy(dst, src, copy_size);
         else
         {
            tg->attrib[attr].fetch( data, src, 0, 0 );

            if (0)
               debug_printf("Fetch linear attr %d  from %p  stride %d  index %d: "
                         " %f, %f, %f, %f \n",
                         attr,
                         tg->attrib[attr].input_ptr,
                         tg->attrib[attr].input_stride,
                         index,
                         data[0], data[1],data[2], data[3]);

            tg->attrib[attr].emit( data, dst );
         }
      } else {
         if(likely(tg->attrib[attr].copy_size >= 0))
            memcpy(data, &instance_id, 4);
         else
         {
            data[0] = (float)instance_id;
            tg->attrib[attr].emit( data, dst );
         }
      }
   }
}

/**
 * Fetch vertex attributes for 'count' vertices.
 */
static void PIPE_CDECL generic_run_elts( struct translate *translate,
                                         const unsigned *elts,
                                         unsigned count,
                                         unsigned instance_id,
                                         void *output_buffer )
{
   struct translate_generic *tg = translate_generic(translate);
   char *vert = output_buffer;
   unsigned i;

   for (i = 0; i < count; i++) {
      generic_run_one(tg, *elts++, instance_id, vert);
      vert += tg->translate.key.output_stride;
   }
}

static void PIPE_CDECL generic_run_elts16( struct translate *translate,
                                         const uint16_t *elts,
                                         unsigned count,
                                         unsigned instance_id,
                                         void *output_buffer )
{
   struct translate_generic *tg = translate_generic(translate);
   char *vert = output_buffer;
   unsigned i;

   for (i = 0; i < count; i++) {
      generic_run_one(tg, *elts++, instance_id, vert);
      vert += tg->translate.key.output_stride;
   }
}

static void PIPE_CDECL generic_run_elts8( struct translate *translate,
                                         const uint8_t *elts,
                                         unsigned count,
                                         unsigned instance_id,
                                         void *output_buffer )
{
   struct translate_generic *tg = translate_generic(translate);
   char *vert = output_buffer;
   unsigned i;

   for (i = 0; i < count; i++) {
      generic_run_one(tg, *elts++, instance_id, vert);
      vert += tg->translate.key.output_stride;
   }
}

static void PIPE_CDECL generic_run( struct translate *translate,
                                    unsigned start,
                                    unsigned count,
                                    unsigned instance_id,
                                    void *output_buffer )
{
   struct translate_generic *tg = translate_generic(translate);
   char *vert = output_buffer;
   unsigned i;

   for (i = 0; i < count; i++) {
      generic_run_one(tg, start + i, instance_id, vert);
      vert += tg->translate.key.output_stride;
   }
}


			       
static void generic_set_buffer( struct translate *translate,
				unsigned buf,
				const void *ptr,
				unsigned stride,
				unsigned max_index )
{
   struct translate_generic *tg = translate_generic(translate);
   unsigned i;

   for (i = 0; i < tg->nr_attrib; i++) {
      if (tg->attrib[i].buffer == buf) {
	 tg->attrib[i].input_ptr = ((const uint8_t *)ptr +
				    tg->attrib[i].input_offset);
	 tg->attrib[i].input_stride = stride;
         tg->attrib[i].max_index = max_index;
      }
   }
}


static void generic_release( struct translate *translate )
{
   /* Refcount?
    */
   FREE(translate);
}

struct translate *translate_generic_create( const struct translate_key *key )
{
   struct translate_generic *tg = CALLOC_STRUCT(translate_generic);
   unsigned i;

   if (tg == NULL)
      return NULL;

   tg->translate.key = *key;
   tg->translate.release = generic_release;
   tg->translate.set_buffer = generic_set_buffer;
   tg->translate.run_elts = generic_run_elts;
   tg->translate.run_elts16 = generic_run_elts16;
   tg->translate.run_elts8 = generic_run_elts8;
   tg->translate.run = generic_run;

   for (i = 0; i < key->nr_elements; i++) {
      const struct util_format_description *format_desc =
            util_format_description(key->element[i].input_format);

      assert(format_desc);
      assert(format_desc->fetch_rgba_float);

      tg->attrib[i].type = key->element[i].type;

      tg->attrib[i].fetch = format_desc->fetch_rgba_float;
      tg->attrib[i].buffer = key->element[i].input_buffer;
      tg->attrib[i].input_offset = key->element[i].input_offset;
      tg->attrib[i].instance_divisor = key->element[i].instance_divisor;

      tg->attrib[i].output_offset = key->element[i].output_offset;

      tg->attrib[i].copy_size = -1;
      if (tg->attrib[i].type == TRANSLATE_ELEMENT_INSTANCE_ID)
      {
            if(key->element[i].output_format == PIPE_FORMAT_R32_USCALED
                  || key->element[i].output_format == PIPE_FORMAT_R32_SSCALED)
               tg->attrib[i].copy_size = 4;
      }
      else
      {
         if(key->element[i].input_format == key->element[i].output_format
               && format_desc->block.width == 1
               && format_desc->block.height == 1
               && !(format_desc->block.bits & 7))
            tg->attrib[i].copy_size = format_desc->block.bits >> 3;
      }

      if(tg->attrib[i].copy_size < 0)
	      tg->attrib[i].emit = get_emit_func(key->element[i].output_format);
      else
	      tg->attrib[i].emit  = NULL;
   }

   tg->nr_attrib = key->nr_elements;


   return &tg->translate;
}

boolean translate_generic_is_output_format_supported(enum pipe_format format)
{
   switch(format)
   {
   case PIPE_FORMAT_R64G64B64A64_FLOAT: return TRUE;
   case PIPE_FORMAT_R64G64B64_FLOAT: return TRUE;
   case PIPE_FORMAT_R64G64_FLOAT: return TRUE;
   case PIPE_FORMAT_R64_FLOAT: return TRUE;

   case PIPE_FORMAT_R32G32B32A32_FLOAT: return TRUE;
   case PIPE_FORMAT_R32G32B32_FLOAT: return TRUE;
   case PIPE_FORMAT_R32G32_FLOAT: return TRUE;
   case PIPE_FORMAT_R32_FLOAT: return TRUE;

   case PIPE_FORMAT_R32G32B32A32_USCALED: return TRUE;
   case PIPE_FORMAT_R32G32B32_USCALED: return TRUE;
   case PIPE_FORMAT_R32G32_USCALED: return TRUE;
   case PIPE_FORMAT_R32_USCALED: return TRUE;

   case PIPE_FORMAT_R32G32B32A32_SSCALED: return TRUE;
   case PIPE_FORMAT_R32G32B32_SSCALED: return TRUE;
   case PIPE_FORMAT_R32G32_SSCALED: return TRUE;
   case PIPE_FORMAT_R32_SSCALED: return TRUE;

   case PIPE_FORMAT_R32G32B32A32_UNORM: return TRUE;
   case PIPE_FORMAT_R32G32B32_UNORM: return TRUE;
   case PIPE_FORMAT_R32G32_UNORM: return TRUE;
   case PIPE_FORMAT_R32_UNORM: return TRUE;

   case PIPE_FORMAT_R32G32B32A32_SNORM: return TRUE;
   case PIPE_FORMAT_R32G32B32_SNORM: return TRUE;
   case PIPE_FORMAT_R32G32_SNORM: return TRUE;
   case PIPE_FORMAT_R32_SNORM: return TRUE;

   case PIPE_FORMAT_R16G16B16A16_USCALED: return TRUE;
   case PIPE_FORMAT_R16G16B16_USCALED: return TRUE;
   case PIPE_FORMAT_R16G16_USCALED: return TRUE;
   case PIPE_FORMAT_R16_USCALED: return TRUE;

   case PIPE_FORMAT_R16G16B16A16_SSCALED: return TRUE;
   case PIPE_FORMAT_R16G16B16_SSCALED: return TRUE;
   case PIPE_FORMAT_R16G16_SSCALED: return TRUE;
   case PIPE_FORMAT_R16_SSCALED: return TRUE;

   case PIPE_FORMAT_R16G16B16A16_UNORM: return TRUE;
   case PIPE_FORMAT_R16G16B16_UNORM: return TRUE;
   case PIPE_FORMAT_R16G16_UNORM: return TRUE;
   case PIPE_FORMAT_R16_UNORM: return TRUE;

   case PIPE_FORMAT_R16G16B16A16_SNORM: return TRUE;
   case PIPE_FORMAT_R16G16B16_SNORM: return TRUE;
   case PIPE_FORMAT_R16G16_SNORM: return TRUE;
   case PIPE_FORMAT_R16_SNORM: return TRUE;

   case PIPE_FORMAT_R8G8B8A8_USCALED: return TRUE;
   case PIPE_FORMAT_R8G8B8_USCALED: return TRUE;
   case PIPE_FORMAT_R8G8_USCALED: return TRUE;
   case PIPE_FORMAT_R8_USCALED: return TRUE;

   case PIPE_FORMAT_R8G8B8A8_SSCALED: return TRUE;
   case PIPE_FORMAT_R8G8B8_SSCALED: return TRUE;
   case PIPE_FORMAT_R8G8_SSCALED: return TRUE;
   case PIPE_FORMAT_R8_SSCALED: return TRUE;

   case PIPE_FORMAT_R8G8B8A8_UNORM: return TRUE;
   case PIPE_FORMAT_R8G8B8_UNORM: return TRUE;
   case PIPE_FORMAT_R8G8_UNORM: return TRUE;
   case PIPE_FORMAT_R8_UNORM: return TRUE;

   case PIPE_FORMAT_R8G8B8A8_SNORM: return TRUE;
   case PIPE_FORMAT_R8G8B8_SNORM: return TRUE;
   case PIPE_FORMAT_R8G8_SNORM: return TRUE;
   case PIPE_FORMAT_R8_SNORM: return TRUE;

   case PIPE_FORMAT_A8R8G8B8_UNORM: return TRUE;
   case PIPE_FORMAT_B8G8R8A8_UNORM: return TRUE;
   default: return FALSE;
   }
}
