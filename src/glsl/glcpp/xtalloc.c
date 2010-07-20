/*
 * Copyright © 2010 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "glcpp.h"

void *
xtalloc_named_const (const void *context, size_t size, const char *name)
{
	void *ret;

	ret = talloc_named_const (context, size, name);
	if (ret == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	return ret;
}

char *
xtalloc_strdup (const void *t, const char *p)
{
	char *ret;

	ret = talloc_strdup (t, p);
	if (ret == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	return ret;
}

char *
xtalloc_strndup (const void *t, const char *p, size_t n)
{
	char *ret;

	ret = talloc_strndup (t, p, n);
	if (ret == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	return ret;
}

char *
xtalloc_asprintf (const void *t, const char *fmt, ...)
{
	va_list ap;
	char *ret;

	va_start(ap, fmt);

	ret = talloc_vasprintf(t, fmt, ap);
	if (ret == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	va_end(ap);
	return ret;
}

void *
_xtalloc_reference_loc (const void *context,
			const void *ptr, const char *location)
{
	void *ret;

	ret = _talloc_reference_loc (context, ptr, location);
	if (ret == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	return ret;
}
