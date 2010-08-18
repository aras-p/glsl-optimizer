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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "glcpp.h"

extern int yydebug;

static char *
load_text_file(void *ctx, const char *file_name)
{
	char *text = NULL;
	struct stat st;
	ssize_t total_read = 0;
	int fd = file_name == NULL ? STDIN_FILENO : open(file_name, O_RDONLY);

	if (fd < 0) {
		return NULL;
	}

	if (fstat(fd, & st) == 0) {
		text = (char *) talloc_size(ctx, st.st_size + 1);
		if (text != NULL) {
			do {
				ssize_t bytes = read(fd, text + total_read,
						     st.st_size - total_read);
				if (bytes < 0) {
					text = NULL;
					break;
				}

				if (bytes == 0) {
					break;
				}

				total_read += bytes;
			} while (total_read < st.st_size);

			text[total_read] = '\0';
		}
	}

	close(fd);

	return text;
}

int
main (int argc, char *argv[])
{
	char *filename = NULL;
	void *ctx = talloc(NULL, void*);
	char *info_log = talloc_strdup(ctx, "");
	const char *shader;
	int ret;

	if (argc) {
		filename = argv[1];
	}

	shader = load_text_file(ctx, filename);
	ret = preprocess(ctx, &shader, &info_log, NULL);

	printf("%s", shader);
	fprintf(stderr, "%s", info_log);

	talloc_free(ctx);

	return ret;
}
