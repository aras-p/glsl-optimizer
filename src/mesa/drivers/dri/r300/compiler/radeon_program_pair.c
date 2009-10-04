/*
 * Copyright (C) 2008-2009 Nicolai Haehnle.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "radeon_program_pair.h"


/**
 * Return the source slot where we installed the given register access,
 * or -1 if no slot was free anymore.
 */
int rc_pair_alloc_source(struct rc_pair_instruction *pair,
	unsigned int rgb, unsigned int alpha,
	rc_register_file file, unsigned int index)
{
	int candidate = -1;
	int candidate_quality = -1;
	int i;

	if ((!rgb && !alpha) || file == RC_FILE_NONE)
		return 0;

	for(i = 0; i < 3; ++i) {
		int q = 0;
		if (rgb) {
			if (pair->RGB.Src[i].Used) {
				if (pair->RGB.Src[i].File != file ||
				    pair->RGB.Src[i].Index != index)
					continue;
				q++;
			}
		}
		if (alpha) {
			if (pair->Alpha.Src[i].Used) {
				if (pair->Alpha.Src[i].File != file ||
				    pair->Alpha.Src[i].Index != index)
					continue;
				q++;
			}
		}
		if (q > candidate_quality) {
			candidate_quality = q;
			candidate = i;
		}
	}

	if (candidate >= 0) {
		if (rgb) {
			pair->RGB.Src[candidate].Used = 1;
			pair->RGB.Src[candidate].File = file;
			pair->RGB.Src[candidate].Index = index;
		}
		if (alpha) {
			pair->Alpha.Src[candidate].Used = 1;
			pair->Alpha.Src[candidate].File = file;
			pair->Alpha.Src[candidate].Index = index;
		}
	}

	return candidate;
}
