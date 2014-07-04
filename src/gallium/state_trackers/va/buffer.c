/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
 * Copyright 2014 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_memory.h"
#include "util/u_handle_table.h"

#include "va_private.h"

VAStatus
vlVaCreateBuffer(VADriverContextP ctx, VAContextID context, VABufferType type,
                 unsigned int size, unsigned int num_elements, void *data,
                 VABufferID *buf_id)
{
   vlVaBuffer *buf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   buf = CALLOC(1, sizeof(vlVaBuffer));
   if (!buf)
      return VA_STATUS_ERROR_ALLOCATION_FAILED;

   buf->type = type;
   buf->size = size;
   buf->num_elements = num_elements;
   buf->data = MALLOC(size * num_elements);

   if (!buf->data) {
      FREE(buf);
      return VA_STATUS_ERROR_ALLOCATION_FAILED;
   }

   if (data)
      memcpy(buf->data, data, size * num_elements);

   *buf_id = handle_table_add(VL_VA_DRIVER(ctx)->htab, buf);

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaBufferSetNumElements(VADriverContextP ctx, VABufferID buf_id,
                         unsigned int num_elements)
{
   vlVaBuffer *buf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, buf_id);
   buf->data = REALLOC(buf->data, buf->size * buf->num_elements,
                       buf->size * num_elements);
   buf->num_elements = num_elements;

   if (!buf->data)
      return VA_STATUS_ERROR_ALLOCATION_FAILED;

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaMapBuffer(VADriverContextP ctx, VABufferID buf_id, void **pbuff)
{
   vlVaBuffer *buf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, buf_id);
   if (!buf)
      return VA_STATUS_ERROR_INVALID_BUFFER;

   *pbuff = buf->data;

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaUnmapBuffer(VADriverContextP ctx, VABufferID buf_id)
{
   vlVaBuffer *buf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, buf_id);
   if (!buf)
      return VA_STATUS_ERROR_INVALID_BUFFER;

   /* Nothing to do here */

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaDestroyBuffer(VADriverContextP ctx, VABufferID buf_id)
{
   vlVaBuffer *buf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, buf_id);
   if (!buf)
      return VA_STATUS_ERROR_INVALID_BUFFER;

   FREE(buf->data);
   FREE(buf);

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaBufferInfo(VADriverContextP ctx, VABufferID buf_id, VABufferType *type,
               unsigned int *size, unsigned int *num_elements)
{
   vlVaBuffer *buf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, buf_id);
   if (!buf)
      return VA_STATUS_ERROR_INVALID_BUFFER;

   *type = buf->type;
   *size = buf->size;
   *num_elements = buf->num_elements;

   return VA_STATUS_SUCCESS;
}
