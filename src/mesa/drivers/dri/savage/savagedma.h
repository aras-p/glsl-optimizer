/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef SAVAGEDMA
#define SAVAGEDMA

/* Whether use DMA to transfer the 3d commands and data */
/* Need to fix the drm driver first though */
#define SAVAGE_CMD_DMA 0

#define DMA_BUFFER_SIZE (4*1024*1024) /*4M*/
#define MAX_DMA_BUFFER_SIZE (16*1024*1024)
#define DMA_PAGE_SIZE (4*1024)  /* savage4 , twister, prosavage,...*/
#define DMA_TRY_COUNT 4

#define MAX_SHADOWCOUNTER (MAX_DMA_BUFFER_SIZE / DMA_PAGE_SIZE)
typedef struct DMABuffer{
    drm_savage_alloc_cont_mem_t * buf;
    GLuint start, end, allocEnd;
    GLuint usingPage; /*current page */
    unsigned int kickFlag; /* usingPage is kicked off ?*/
} DMABuffer_t, * DMABufferPtr;

void *savageDMAAlloc (savageContextPtr imesa, GLuint size);
void savageDMACommit (savageContextPtr imesa, void *end);
void savageDMAFlush (savageContextPtr imesa);
int savageDMAInit (savageContextPtr imesa);
int savageDMAClose (savageContextPtr);

#endif
