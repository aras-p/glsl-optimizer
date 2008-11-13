/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_NOTIFIER_H__
#define __NOUVEAU_NOTIFIER_H__

#define NV_NOTIFIER_SIZE                                                      32
#define NV_NOTIFY_TIME_0                                              0x00000000
#define NV_NOTIFY_TIME_1                                              0x00000004
#define NV_NOTIFY_RETURN_VALUE                                        0x00000008
#define NV_NOTIFY_STATE                                               0x0000000C
#define NV_NOTIFY_STATE_STATUS_MASK                                   0xFF000000
#define NV_NOTIFY_STATE_STATUS_SHIFT                                          24
#define NV_NOTIFY_STATE_STATUS_COMPLETED                                    0x00
#define NV_NOTIFY_STATE_STATUS_IN_PROCESS                                   0x01
#define NV_NOTIFY_STATE_ERROR_CODE_MASK                               0x0000FFFF
#define NV_NOTIFY_STATE_ERROR_CODE_SHIFT                                       0

struct nouveau_notifier {
	struct nouveau_channel *channel;
	uint32_t handle;
};

#endif
