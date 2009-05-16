/*
 * Copyright 2009 Jakob Bornecrantz <jakob@vmware.com>
 *                Corbin Simpson <MostAwesomeDude@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef TRACE_DRM_H
#define TRACE_DRM_H

#include "state_tracker/drm_api.h"

#include "trace/tr_buffer.h"
#include "trace/tr_context.h"
#include "trace/tr_screen.h"
#include "trace/tr_texture.h"

struct drm_api hooks;

static struct pipe_screen *
trace_drm_create_screen(int fd, struct drm_create_screen_arg *arg)
{
	struct pipe_screen *screen;

	if (arg && arg->mode != DRM_CREATE_NORMAL)
		return NULL;

	screen = hooks.create_screen(fd, arg);

	return trace_screen_create(screen);
};

static struct pipe_context *
trace_drm_create_context(struct pipe_screen *_screen)
{
	struct pipe_screen *screen;
	struct pipe_context *pipe;

	if (trace_enabled())
		screen = trace_screen(_screen)->screen;
	else
		screen = _screen;

	pipe = hooks.create_context(screen);

	if (trace_enabled())
		pipe = trace_context_create(_screen, pipe);

	return pipe;
};

static boolean
trace_drm_buffer_from_texture(struct pipe_texture *_texture,
                              struct pipe_buffer **_buffer,
                              unsigned *stride)
{
	struct pipe_texture *texture;
	struct pipe_buffer *buffer = NULL;
	boolean result;

	if (trace_enabled())
		texture = trace_texture(_texture)->texture;
	else
		texture = _texture;

	result = hooks.buffer_from_texture(texture, &buffer, stride);

	if (result && _buffer)
		buffer = trace_buffer_create(trace_screen(texture->screen), buffer);

	if (_buffer)
		*_buffer = buffer;
	else
		pipe_buffer_reference(&buffer, NULL);

	return result;
}

static struct pipe_buffer *
trace_drm_buffer_from_handle(struct pipe_screen *_screen,
                             const char *name,
                             unsigned handle)
{
	struct pipe_screen *screen;
	struct pipe_buffer *result;

	if (trace_enabled())
		screen = trace_screen(_screen)->screen;
	else
		screen = _screen;

	result = hooks.buffer_from_handle(screen, name, handle);

	if (trace_enabled())
		result = trace_buffer_create(trace_screen(_screen), result);

	return result;
}

static boolean
trace_drm_handle_from_buffer(struct pipe_screen *_screen,
                             struct pipe_buffer *_buffer,
                             unsigned *handle)
{
	struct pipe_screen *screen;
	struct pipe_buffer *buffer;

	if (trace_enabled()) {
		screen = trace_screen(_screen)->screen;
		buffer = trace_buffer(_buffer)->buffer;
	} else {
		screen = _screen;
		buffer = _buffer;
	}

	return hooks.handle_from_buffer(screen, buffer, handle);
}

static boolean
trace_drm_global_handle_from_buffer(struct pipe_screen *_screen,
                                    struct pipe_buffer *_buffer,
                                    unsigned *handle)
{
	struct pipe_screen *screen;
	struct pipe_buffer *buffer;

	if (trace_enabled()) {
		screen = trace_screen(_screen)->screen;
		buffer = trace_buffer(_buffer)->buffer;
	} else {
		screen = _screen;
		buffer = _buffer;
	}

	return hooks.global_handle_from_buffer(screen, buffer, handle);
}

struct drm_api drm_api_hooks =
{
	.create_screen = trace_drm_create_screen,
	.create_context = trace_drm_create_context,

	.buffer_from_texture = trace_drm_buffer_from_texture,
	.buffer_from_handle = trace_drm_buffer_from_handle,
	.handle_from_buffer = trace_drm_handle_from_buffer,
	.global_handle_from_buffer = trace_drm_global_handle_from_buffer,
};

#endif /* TRACE_DRM_H */
