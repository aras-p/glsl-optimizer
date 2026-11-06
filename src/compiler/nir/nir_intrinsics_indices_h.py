
template = """\
/* Copyright (C) 2018 Red Hat
 * Copyright (C) 2020 Valve Corporation
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
 */

#ifndef _NIR_INTRINSICS_INDICES_
#define _NIR_INTRINSICS_INDICES_

% for index in INTR_INDICES:
<%
data_type = index.c_data_type
name = index.name
enum = "NIR_INTRINSIC_" + name.upper()
%>

static inline ${data_type}
nir_intrinsic_${name}(const nir_intrinsic_instr *instr)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   assert(info->index_map[${enum}] > 0);
% if "struct" in data_type:
   ${data_type} res;
   STATIC_ASSERT(sizeof(instr->const_index[0]) * ${index.size} == sizeof(res));
   memcpy(&res, &instr->const_index[info->index_map[${enum}] - 1], sizeof(res));
   return res;
% else:
   return (${data_type})instr->const_index[info->index_map[${enum}] - 1];
% endif
}

static inline void
nir_intrinsic_set_${name}(nir_intrinsic_instr *instr, ${data_type} val)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   assert(info->index_map[${enum}] > 0);
% if "struct" in data_type:
   STATIC_ASSERT(sizeof(instr->const_index[0]) * ${index.size} == sizeof(val));
   /* NOTE: gcc has a a false positive here, silenced with the pragmas */
   PRAGMA_DIAGNOSTIC_PUSH
   PRAGMA_DIAGNOSTIC_IGNORED_GCC(-Wstringop-overflow)
   memcpy(&instr->const_index[info->index_map[${enum}] - 1], &val, sizeof(val));
   PRAGMA_DIAGNOSTIC_POP
% else:
   instr->const_index[info->index_map[${enum}] - 1] = val;
% endif
}

static inline bool
nir_intrinsic_has_${name}(const nir_intrinsic_instr *instr)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   return info->index_map[${enum}] > 0;
}
% endfor

static inline unsigned
nir_intrinsic_index_size(nir_intrinsic_index_flag index)
{
   switch (index) {
% for index in INTR_INDICES:
% if index.size != 1:
   case ${"NIR_INTRINSIC_" + index.name.upper()}: return ${index.size};
% endif
% endfor
   default: return 1;
   }
}

#endif /* _NIR_INTRINSICS_INDICES_ */
"""

from nir_intrinsics import INTR_INDICES
from mako.template import Template
import argparse
import os


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True,
                        help='Output H file')

    args = parser.parse_args()

    with open(args.out, 'w', encoding='utf-8') as f:
        f.write(Template(template).render(INTR_INDICES=INTR_INDICES))

if __name__ == '__main__':
    main()

