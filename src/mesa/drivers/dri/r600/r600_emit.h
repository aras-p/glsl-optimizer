/**************************************************************************

Copyright 2008, 2009 Advanced Micro Devices Inc. (AMD)

Copyright (C) Advanced Micro Devices Inc. (AMD)  2009.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */


#ifndef __R600_EMIT_H__
#define __R600_EMIT_H__

#include "main/glheader.h"
#include "r600_context.h"
#include "r600_cmdbuf.h"
#include "radeon_reg.h"

void r600EmitCacheFlush(context_t *rmesa);

extern GLboolean r600EmitShader(GLcontext * ctx, 
                                void ** shaderbo,
			                    GLvoid * data, 
                                int sizeinDWORD,
                                char * szShaderUsage); 

extern GLboolean r600DeleteShader(GLcontext * ctx, 
                                 void * shaderbo);

#endif
