/*
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef GLMAIN_H
#define GLMAIN_H


#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <stdlib.h>
#include <math.h>


/** Test programs can use these vars/functions */

extern int WinWidth, WinHeight;

extern double
PerfGetTime(void);

extern void
PerfSwapBuffers(void);

extern GLuint
PerfCheckerTexture(GLsizei width, GLsizei height);

extern GLuint
PerfShaderProgram(const char *vertShader, const char *fragShader);

extern int
PerfReshapeWindow( unsigned w, unsigned h );

extern GLboolean
PerfExtensionSupported(const char *ext);


/** Test programs must implement these functions **/

extern void
PerfInit(void);

extern void
PerfNextRound(void);

extern void
PerfDraw(void);


#endif /* GLMAIN_H */
