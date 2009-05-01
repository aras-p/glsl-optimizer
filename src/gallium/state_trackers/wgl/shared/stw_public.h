/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef STW_PUBLIC_H
#define STW_PUBLIC_H

#include <windows.h>

BOOL stw_copy_context( UINT_PTR hglrcSrc,
                       UINT_PTR hglrcDst,
                       UINT mask );

UINT_PTR stw_create_layer_context( HDC hdc, 
                                   int iLayerPlane );

BOOL stw_share_lists( UINT_PTR hglrc1, UINT_PTR hglrc2 );

BOOL stw_delete_context( UINT_PTR hglrc );

BOOL
stw_release_context( UINT_PTR dhglrc );

UINT_PTR stw_get_current_context( void );

HDC stw_get_current_dc( void );

BOOL stw_make_current( HDC hdc, UINT_PTR hglrc );

BOOL stw_swap_buffers( HDC hdc );

BOOL
stw_swap_layer_buffers( HDC hdc, UINT fuPlanes );

PROC stw_get_proc_address( LPCSTR lpszProc );

int stw_pixelformat_describe( HDC hdc,
                              int iPixelFormat,
                              UINT nBytes,
                              LPPIXELFORMATDESCRIPTOR ppfd );

int stw_pixelformat_get( HDC hdc );

BOOL stw_pixelformat_set( HDC hdc,
                          int iPixelFormat );

int stw_pixelformat_choose( HDC hdc,
                            CONST PIXELFORMATDESCRIPTOR *ppfd );

#endif
