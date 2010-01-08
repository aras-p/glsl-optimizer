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

/* Helper utility for uploading user buffers & other data, and
 * coalescing small buffers into larger ones.
 */

#ifndef U_UPLOAD_MGR_H
#define U_UPLOAD_MGR_H

#include "pipe/p_defines.h"

struct pipe_screen;
struct pipe_buffer;
struct u_upload_mgr;


struct u_upload_mgr *u_upload_create( struct pipe_screen *screen,
                                      unsigned default_size,
                                      unsigned alignment,
                                      unsigned usage );

void u_upload_destroy( struct u_upload_mgr *upload );

/* Unmap and release old buffer.
 * 
 * This must usually be called prior to firing the command stream
 * which references the upload buffer, as many memory managers either
 * don't like firing a mapped buffer or cause subsequent maps of a
 * fired buffer to wait.  For now, it's easiest just to grab a new
 * buffer.
 */
void u_upload_flush( struct u_upload_mgr *upload );


enum pipe_error u_upload_data( struct u_upload_mgr *upload,
                               unsigned size,
                               const void *data,
                               unsigned *out_offset,
                               struct pipe_buffer **outbuf );


enum pipe_error u_upload_buffer( struct u_upload_mgr *upload,
                                 unsigned offset,
                                 unsigned size,
                                 struct pipe_buffer *inbuf,
                                 unsigned *out_offset,
                                 struct pipe_buffer **outbuf );



#endif

