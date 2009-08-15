/*
 * Copyright © 2008-2009 Maciej Cencora <m.cencora@gmail.com>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Maciej Cencora <m.cencora@gmail.com>
 *
 */

#include "r300_queryobj.h"
#include "r300_emit.h"

#include "main/imports.h"
#include "main/simple_list.h"

#define DDEBUG 0

#define PAGE_SIZE 4096

static void r300QueryGetResult(GLcontext *ctx, struct gl_query_object *q)
{
	struct r300_query_object *query = (struct r300_query_object *)q;
	uint32_t *result;
	int i;

	if (DDEBUG) fprintf(stderr, "%s: query id %d, result %d\n", __FUNCTION__, query->Base.Id, (int) query->Base.Result);

	radeon_bo_map(query->bo, GL_FALSE);

	result = query->bo->ptr;

	query->Base.Result = 0;
	for (i = 0; i < query->curr_offset/sizeof(uint32_t); ++i) {
		query->Base.Result += result[i];
		if (DDEBUG) fprintf(stderr, "result[%d] = %d\n", i, result[i]);
	}

	radeon_bo_unmap(query->bo);
}

static struct gl_query_object * r300NewQueryObject(GLcontext *ctx, GLuint id)
{
	struct r300_query_object *query;

	query = _mesa_calloc(sizeof(struct r300_query_object));

	query->Base.Id = id;
	query->Base.Result = 0;
	query->Base.Active = GL_FALSE;
	query->Base.Ready = GL_TRUE;

	if (DDEBUG) fprintf(stderr, "%s: query id %d\n", __FUNCTION__, query->Base.Id);

	return &query->Base;
}

static void r300DeleteQuery(GLcontext *ctx, struct gl_query_object *q)
{
	struct r300_query_object *query = (struct r300_query_object *)q;

	if (DDEBUG) fprintf(stderr, "%s: query id %d\n", __FUNCTION__, q->Id);

	if (query->bo) {
		radeon_bo_unref(query->bo);
	}

	_mesa_free(query);
}

static void r300BeginQuery(GLcontext *ctx, struct gl_query_object *q)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_query_object *query = (struct r300_query_object *)q;

	if (DDEBUG) fprintf(stderr, "%s: query id %d\n", __FUNCTION__, q->Id);

	assert(r300->query.current == NULL);

	if (!query->bo) {
		query->bo = radeon_bo_open(r300->radeon.radeonScreen->bom, 0, PAGE_SIZE, PAGE_SIZE, RADEON_GEM_DOMAIN_GTT, 0);
	}
	query->curr_offset = 0;

	r300->query.current = query;
	insert_at_tail(&r300->query.not_flushed_head, query);
}

static void r300EndQuery(GLcontext *ctx, struct gl_query_object *q)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (DDEBUG) fprintf(stderr, "%s: query id %d\n", __FUNCTION__, q->Id);

	r300EmitQueryEnd(ctx);

	r300->query.current = NULL;
}

static void r300WaitQuery(GLcontext *ctx, struct gl_query_object *q)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_query_object *tmp, *query = (struct r300_query_object *)q;

	/* If the cmdbuf with packets for this query hasn't been flushed yet, do it now */
	{
		GLboolean found = GL_FALSE;
		foreach(tmp, &r300->query.not_flushed_head) {
			if (tmp == query) {
				found = GL_TRUE;
				break;
			}
		}

		if (found)
			ctx->Driver.Flush(ctx);
	}

	if (DDEBUG) fprintf(stderr, "%s: query id %d, bo %p, offset %d\n", __FUNCTION__, q->Id, query->bo, query->curr_offset);

	r300QueryGetResult(ctx, q);

	query->Base.Ready = GL_TRUE;
}


/**
 * TODO:
 * should check if bo is idle, bo there's no interface to do it
 * just wait for result now
 */
static void r300CheckQuery(GLcontext *ctx, struct gl_query_object *q)
{
	if (DDEBUG) fprintf(stderr, "%s: query id %d\n", __FUNCTION__, q->Id);

	r300WaitQuery(ctx, q);
}

void r300EmitQueryBegin(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_query_object *query = r300->query.current;
	BATCH_LOCALS(&r300->radeon);

	if (!query || query->emitted_begin)
		return;

	if (DDEBUG) fprintf(stderr, "%s: query id %d\n", __FUNCTION__, query->Base.Id);

	if (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV530) {
		BEGIN_BATCH_NO_AUTOSTATE(4);
		OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
		OUT_BATCH_REGVAL(R300_ZB_ZPASS_DATA, 0);
		END_BATCH();
	} else {
		BEGIN_BATCH_NO_AUTOSTATE(4);
		OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_ALL);
		OUT_BATCH_REGVAL(R300_ZB_ZPASS_DATA, 0);
		END_BATCH();
	}

	query->emitted_begin = GL_TRUE;
}

void r300EmitQueryEnd(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_query_object *query = r300->query.current;
	BATCH_LOCALS(&r300->radeon);

	if (!query || !query->emitted_begin)
		return;

	if (DDEBUG) fprintf(stderr, "%s: query id %d, bo %p, offset %d\n", __FUNCTION__, query->Base.Id, query->bo, query->curr_offset);

	radeon_cs_space_check_with_bo(r300->radeon.cmdbuf.cs,
								  query->bo,
								  0, RADEON_GEM_DOMAIN_GTT);

	if (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV530) {
		BEGIN_BATCH_NO_AUTOSTATE(14);
		OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
		OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
		OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
		OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_1);
		OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
		OUT_BATCH_RELOC(0, query->bo, query->curr_offset + sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
		OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
		END_BATCH();
	} else {
		BEGIN_BATCH_NO_AUTOSTATE(3 * 2 *r300->num_z_pipes + 2);
		switch (r300->num_z_pipes) {
			case 4:
				OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_3);
				OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
				OUT_BATCH_RELOC(0, query->bo, query->curr_offset+3*sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
			case 3:
				OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_2);
				OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
				OUT_BATCH_RELOC(0, query->bo, query->curr_offset+2*sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
			case 2:
				if (r300->radeon.radeonScreen->chip_family <= CHIP_FAMILY_RV380) {
					OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_3);
				} else {
					OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_1);
				}
				OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
				OUT_BATCH_RELOC(0, query->bo, query->curr_offset+1*sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
			case 1:
			default:
				OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_0);
				OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
				OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
				break;
		}
		OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_ALL);
		END_BATCH();
	}

	query->curr_offset += r300->num_z_pipes * sizeof(uint32_t);
	assert(query->curr_offset < PAGE_SIZE);
	query->emitted_begin = GL_FALSE;
}

void r300InitQueryObjFunctions(struct dd_function_table *functions)
{
	functions->NewQueryObject = r300NewQueryObject;
	functions->DeleteQuery = r300DeleteQuery;
	functions->BeginQuery = r300BeginQuery;
	functions->EndQuery = r300EndQuery;
	functions->CheckQuery = r300CheckQuery;
	functions->WaitQuery = r300WaitQuery;
}
