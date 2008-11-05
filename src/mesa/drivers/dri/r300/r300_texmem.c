/**************************************************************************

Copyright (C) Tungsten Graphics 2002.  All Rights Reserved.
The Weather Channel, Inc. funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86
license. This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation on the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT. IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR THEIR
SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Gareth Hughes <gareth@valinux.com>
 *
 * \author Kevin E. Martin <martin@valinux.com>
 */

#include <errno.h>

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/colormac.h"
#include "main/macros.h"
#include "main/simple_list.h"
#include "radeon_reg.h"		/* gets definition for usleep */
#include "r300_context.h"
#include "r300_state.h"
#include "r300_cmdbuf.h"
#include "r300_emit.h"
#include "r300_mipmap_tree.h"
#include "radeon_ioctl.h"
#include "r300_tex.h"
#include "r300_ioctl.h"
#include <unistd.h>		/* for usleep() */


