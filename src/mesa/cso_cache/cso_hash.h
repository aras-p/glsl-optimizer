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
  *   Zack Rusin <zack@tungstengraphics.com>
  */

#ifndef CSO_HASH_H
#define CSO_HASH_H

struct cso_hash;
struct cso_node;

struct cso_hash_iter {
   struct cso_hash *hash;
   struct cso_node  *node;
};

struct cso_hash *cso_hash_create(void);
void              cso_hash_delete(struct cso_hash *hash);

struct cso_hash_iter cso_hash_insert(struct cso_hash *hash, unsigned key,
                                     void *data);
void  *cso_hash_take(struct cso_hash *hash, unsigned key);

struct cso_hash_iter cso_hash_first_node(struct cso_hash *hash);
struct cso_hash_iter cso_hash_find(struct cso_hash *hash, unsigned key);


int       cso_hash_iter_is_null(struct cso_hash_iter iter);
unsigned  cso_hash_iter_key(struct cso_hash_iter iter);
void     *cso_hash_iter_data(struct cso_hash_iter iter);

struct cso_hash_iter cso_hash_iter_next(struct cso_hash_iter iter);
struct cso_hash_iter cso_hash_iter_prev(struct cso_hash_iter iter);

#endif
