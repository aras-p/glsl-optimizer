/* DO NOT EDIT - This file generated automatically by gl_gen_table.py (from Mesa) script */

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004, 2005
 * (C) Copyright Apple Inc 2011
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "glapi.h"
#include "glapitable.h"

struct _glapi_table *
_glapi_create_table_from_handle(void *handle, const char *symbol_prefix) {
    struct _glapi_table *disp = calloc(1, sizeof(struct _glapi_table));
    char symboln[512];

    if(!disp)
        return NULL;

    if(symbol_prefix == NULL)
        symbol_prefix = "";


    if(!disp->NewList) {
        snprintf(symboln, sizeof(symboln), "%sNewList", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->NewList;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndList) {
        snprintf(symboln, sizeof(symboln), "%sEndList", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndList;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CallList) {
        snprintf(symboln, sizeof(symboln), "%sCallList", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CallList;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CallLists) {
        snprintf(symboln, sizeof(symboln), "%sCallLists", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CallLists;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteLists) {
        snprintf(symboln, sizeof(symboln), "%sDeleteLists", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteLists;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenLists) {
        snprintf(symboln, sizeof(symboln), "%sGenLists", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenLists;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ListBase) {
        snprintf(symboln, sizeof(symboln), "%sListBase", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ListBase;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Begin) {
        snprintf(symboln, sizeof(symboln), "%sBegin", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Begin;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Bitmap) {
        snprintf(symboln, sizeof(symboln), "%sBitmap", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Bitmap;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3b) {
        snprintf(symboln, sizeof(symboln), "%sColor3b", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3b;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3bv) {
        snprintf(symboln, sizeof(symboln), "%sColor3bv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3bv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3d) {
        snprintf(symboln, sizeof(symboln), "%sColor3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3dv) {
        snprintf(symboln, sizeof(symboln), "%sColor3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3f) {
        snprintf(symboln, sizeof(symboln), "%sColor3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3fv) {
        snprintf(symboln, sizeof(symboln), "%sColor3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3i) {
        snprintf(symboln, sizeof(symboln), "%sColor3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3iv) {
        snprintf(symboln, sizeof(symboln), "%sColor3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3s) {
        snprintf(symboln, sizeof(symboln), "%sColor3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3sv) {
        snprintf(symboln, sizeof(symboln), "%sColor3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3ub) {
        snprintf(symboln, sizeof(symboln), "%sColor3ub", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3ub;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3ubv) {
        snprintf(symboln, sizeof(symboln), "%sColor3ubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3ubv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3ui) {
        snprintf(symboln, sizeof(symboln), "%sColor3ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3ui;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3uiv) {
        snprintf(symboln, sizeof(symboln), "%sColor3uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3uiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3us) {
        snprintf(symboln, sizeof(symboln), "%sColor3us", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3us;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color3usv) {
        snprintf(symboln, sizeof(symboln), "%sColor3usv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color3usv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4b) {
        snprintf(symboln, sizeof(symboln), "%sColor4b", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4b;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4bv) {
        snprintf(symboln, sizeof(symboln), "%sColor4bv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4bv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4d) {
        snprintf(symboln, sizeof(symboln), "%sColor4d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4dv) {
        snprintf(symboln, sizeof(symboln), "%sColor4dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4f) {
        snprintf(symboln, sizeof(symboln), "%sColor4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4fv) {
        snprintf(symboln, sizeof(symboln), "%sColor4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4i) {
        snprintf(symboln, sizeof(symboln), "%sColor4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4iv) {
        snprintf(symboln, sizeof(symboln), "%sColor4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4s) {
        snprintf(symboln, sizeof(symboln), "%sColor4s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4sv) {
        snprintf(symboln, sizeof(symboln), "%sColor4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4ub) {
        snprintf(symboln, sizeof(symboln), "%sColor4ub", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4ub;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4ubv) {
        snprintf(symboln, sizeof(symboln), "%sColor4ubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4ubv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4ui) {
        snprintf(symboln, sizeof(symboln), "%sColor4ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4ui;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4uiv) {
        snprintf(symboln, sizeof(symboln), "%sColor4uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4uiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4us) {
        snprintf(symboln, sizeof(symboln), "%sColor4us", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4us;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Color4usv) {
        snprintf(symboln, sizeof(symboln), "%sColor4usv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Color4usv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EdgeFlag) {
        snprintf(symboln, sizeof(symboln), "%sEdgeFlag", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EdgeFlag;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EdgeFlagv) {
        snprintf(symboln, sizeof(symboln), "%sEdgeFlagv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EdgeFlagv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->End) {
        snprintf(symboln, sizeof(symboln), "%sEnd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->End;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexd) {
        snprintf(symboln, sizeof(symboln), "%sIndexd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexd;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexdv) {
        snprintf(symboln, sizeof(symboln), "%sIndexdv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexdv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexf) {
        snprintf(symboln, sizeof(symboln), "%sIndexf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexfv) {
        snprintf(symboln, sizeof(symboln), "%sIndexfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexi) {
        snprintf(symboln, sizeof(symboln), "%sIndexi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexi;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexiv) {
        snprintf(symboln, sizeof(symboln), "%sIndexiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexs) {
        snprintf(symboln, sizeof(symboln), "%sIndexs", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexs;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexsv) {
        snprintf(symboln, sizeof(symboln), "%sIndexsv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexsv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3b) {
        snprintf(symboln, sizeof(symboln), "%sNormal3b", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3b;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3bv) {
        snprintf(symboln, sizeof(symboln), "%sNormal3bv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3bv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3d) {
        snprintf(symboln, sizeof(symboln), "%sNormal3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3dv) {
        snprintf(symboln, sizeof(symboln), "%sNormal3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3f) {
        snprintf(symboln, sizeof(symboln), "%sNormal3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3fv) {
        snprintf(symboln, sizeof(symboln), "%sNormal3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3i) {
        snprintf(symboln, sizeof(symboln), "%sNormal3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3iv) {
        snprintf(symboln, sizeof(symboln), "%sNormal3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3s) {
        snprintf(symboln, sizeof(symboln), "%sNormal3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Normal3sv) {
        snprintf(symboln, sizeof(symboln), "%sNormal3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Normal3sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2d) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2dv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2f) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2fv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2i) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2iv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2s) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos2sv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos2sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos2sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3d) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3dv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3f) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3fv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3i) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3iv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3s) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos3sv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos3sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4d) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4dv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4f) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4fv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4i) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4iv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4s) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RasterPos4sv) {
        snprintf(symboln, sizeof(symboln), "%sRasterPos4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RasterPos4sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rectd) {
        snprintf(symboln, sizeof(symboln), "%sRectd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rectd;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rectdv) {
        snprintf(symboln, sizeof(symboln), "%sRectdv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rectdv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rectf) {
        snprintf(symboln, sizeof(symboln), "%sRectf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rectf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rectfv) {
        snprintf(symboln, sizeof(symboln), "%sRectfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rectfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Recti) {
        snprintf(symboln, sizeof(symboln), "%sRecti", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Recti;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rectiv) {
        snprintf(symboln, sizeof(symboln), "%sRectiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rectiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rects) {
        snprintf(symboln, sizeof(symboln), "%sRects", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rects;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rectsv) {
        snprintf(symboln, sizeof(symboln), "%sRectsv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rectsv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1d) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1dv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1f) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1fv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1i) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1iv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1s) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord1sv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord1sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord1sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2d) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2dv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2f) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2fv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2i) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2iv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2s) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord2sv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord2sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord2sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3d) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3dv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3f) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3fv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3i) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3iv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3s) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord3sv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord3sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4d) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4dv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4f) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4fv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4i) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4iv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4s) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoord4sv) {
        snprintf(symboln, sizeof(symboln), "%sTexCoord4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoord4sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2d) {
        snprintf(symboln, sizeof(symboln), "%sVertex2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2dv) {
        snprintf(symboln, sizeof(symboln), "%sVertex2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2f) {
        snprintf(symboln, sizeof(symboln), "%sVertex2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2fv) {
        snprintf(symboln, sizeof(symboln), "%sVertex2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2i) {
        snprintf(symboln, sizeof(symboln), "%sVertex2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2iv) {
        snprintf(symboln, sizeof(symboln), "%sVertex2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2s) {
        snprintf(symboln, sizeof(symboln), "%sVertex2s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex2sv) {
        snprintf(symboln, sizeof(symboln), "%sVertex2sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex2sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3d) {
        snprintf(symboln, sizeof(symboln), "%sVertex3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3dv) {
        snprintf(symboln, sizeof(symboln), "%sVertex3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3f) {
        snprintf(symboln, sizeof(symboln), "%sVertex3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3fv) {
        snprintf(symboln, sizeof(symboln), "%sVertex3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3i) {
        snprintf(symboln, sizeof(symboln), "%sVertex3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3iv) {
        snprintf(symboln, sizeof(symboln), "%sVertex3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3s) {
        snprintf(symboln, sizeof(symboln), "%sVertex3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex3sv) {
        snprintf(symboln, sizeof(symboln), "%sVertex3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex3sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4d) {
        snprintf(symboln, sizeof(symboln), "%sVertex4d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4dv) {
        snprintf(symboln, sizeof(symboln), "%sVertex4dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4f) {
        snprintf(symboln, sizeof(symboln), "%sVertex4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4fv) {
        snprintf(symboln, sizeof(symboln), "%sVertex4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4i) {
        snprintf(symboln, sizeof(symboln), "%sVertex4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4i;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4iv) {
        snprintf(symboln, sizeof(symboln), "%sVertex4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4iv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4s) {
        snprintf(symboln, sizeof(symboln), "%sVertex4s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4s;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Vertex4sv) {
        snprintf(symboln, sizeof(symboln), "%sVertex4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Vertex4sv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClipPlane) {
        snprintf(symboln, sizeof(symboln), "%sClipPlane", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClipPlane;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorMaterial) {
        snprintf(symboln, sizeof(symboln), "%sColorMaterial", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorMaterial;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CullFace) {
        snprintf(symboln, sizeof(symboln), "%sCullFace", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CullFace;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Fogf) {
        snprintf(symboln, sizeof(symboln), "%sFogf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Fogf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Fogfv) {
        snprintf(symboln, sizeof(symboln), "%sFogfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Fogfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Fogi) {
        snprintf(symboln, sizeof(symboln), "%sFogi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Fogi;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Fogiv) {
        snprintf(symboln, sizeof(symboln), "%sFogiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Fogiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FrontFace) {
        snprintf(symboln, sizeof(symboln), "%sFrontFace", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FrontFace;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Hint) {
        snprintf(symboln, sizeof(symboln), "%sHint", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Hint;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Lightf) {
        snprintf(symboln, sizeof(symboln), "%sLightf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Lightf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Lightfv) {
        snprintf(symboln, sizeof(symboln), "%sLightfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Lightfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Lighti) {
        snprintf(symboln, sizeof(symboln), "%sLighti", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Lighti;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Lightiv) {
        snprintf(symboln, sizeof(symboln), "%sLightiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Lightiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LightModelf) {
        snprintf(symboln, sizeof(symboln), "%sLightModelf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LightModelf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LightModelfv) {
        snprintf(symboln, sizeof(symboln), "%sLightModelfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LightModelfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LightModeli) {
        snprintf(symboln, sizeof(symboln), "%sLightModeli", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LightModeli;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LightModeliv) {
        snprintf(symboln, sizeof(symboln), "%sLightModeliv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LightModeliv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LineStipple) {
        snprintf(symboln, sizeof(symboln), "%sLineStipple", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LineStipple;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LineWidth) {
        snprintf(symboln, sizeof(symboln), "%sLineWidth", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LineWidth;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Materialf) {
        snprintf(symboln, sizeof(symboln), "%sMaterialf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Materialf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Materialfv) {
        snprintf(symboln, sizeof(symboln), "%sMaterialfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Materialfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Materiali) {
        snprintf(symboln, sizeof(symboln), "%sMateriali", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Materiali;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Materialiv) {
        snprintf(symboln, sizeof(symboln), "%sMaterialiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Materialiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointSize) {
        snprintf(symboln, sizeof(symboln), "%sPointSize", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointSize;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PolygonMode) {
        snprintf(symboln, sizeof(symboln), "%sPolygonMode", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PolygonMode;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PolygonStipple) {
        snprintf(symboln, sizeof(symboln), "%sPolygonStipple", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PolygonStipple;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Scissor) {
        snprintf(symboln, sizeof(symboln), "%sScissor", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Scissor;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ShadeModel) {
        snprintf(symboln, sizeof(symboln), "%sShadeModel", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ShadeModel;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameterf) {
        snprintf(symboln, sizeof(symboln), "%sTexParameterf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameterf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sTexParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameteri) {
        snprintf(symboln, sizeof(symboln), "%sTexParameteri", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameteri;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sTexParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexImage1D) {
        snprintf(symboln, sizeof(symboln), "%sTexImage1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexImage2D) {
        snprintf(symboln, sizeof(symboln), "%sTexImage2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexEnvf) {
        snprintf(symboln, sizeof(symboln), "%sTexEnvf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexEnvf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexEnvfv) {
        snprintf(symboln, sizeof(symboln), "%sTexEnvfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexEnvfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexEnvi) {
        snprintf(symboln, sizeof(symboln), "%sTexEnvi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexEnvi;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexEnviv) {
        snprintf(symboln, sizeof(symboln), "%sTexEnviv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexEnviv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexGend) {
        snprintf(symboln, sizeof(symboln), "%sTexGend", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexGend;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexGendv) {
        snprintf(symboln, sizeof(symboln), "%sTexGendv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexGendv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexGenf) {
        snprintf(symboln, sizeof(symboln), "%sTexGenf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexGenf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexGenfv) {
        snprintf(symboln, sizeof(symboln), "%sTexGenfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexGenfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexGeni) {
        snprintf(symboln, sizeof(symboln), "%sTexGeni", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexGeni;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexGeniv) {
        snprintf(symboln, sizeof(symboln), "%sTexGeniv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexGeniv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FeedbackBuffer) {
        snprintf(symboln, sizeof(symboln), "%sFeedbackBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FeedbackBuffer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SelectBuffer) {
        snprintf(symboln, sizeof(symboln), "%sSelectBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SelectBuffer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RenderMode) {
        snprintf(symboln, sizeof(symboln), "%sRenderMode", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RenderMode;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->InitNames) {
        snprintf(symboln, sizeof(symboln), "%sInitNames", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->InitNames;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadName) {
        snprintf(symboln, sizeof(symboln), "%sLoadName", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadName;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PassThrough) {
        snprintf(symboln, sizeof(symboln), "%sPassThrough", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PassThrough;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PopName) {
        snprintf(symboln, sizeof(symboln), "%sPopName", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PopName;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PushName) {
        snprintf(symboln, sizeof(symboln), "%sPushName", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PushName;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawBuffer) {
        snprintf(symboln, sizeof(symboln), "%sDrawBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawBuffer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Clear) {
        snprintf(symboln, sizeof(symboln), "%sClear", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Clear;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearAccum) {
        snprintf(symboln, sizeof(symboln), "%sClearAccum", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearAccum;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearIndex) {
        snprintf(symboln, sizeof(symboln), "%sClearIndex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearIndex;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearColor) {
        snprintf(symboln, sizeof(symboln), "%sClearColor", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearColor;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearStencil) {
        snprintf(symboln, sizeof(symboln), "%sClearStencil", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearStencil;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearDepth) {
        snprintf(symboln, sizeof(symboln), "%sClearDepth", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearDepth;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilMask) {
        snprintf(symboln, sizeof(symboln), "%sStencilMask", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilMask;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorMask) {
        snprintf(symboln, sizeof(symboln), "%sColorMask", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorMask;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DepthMask) {
        snprintf(symboln, sizeof(symboln), "%sDepthMask", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DepthMask;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IndexMask) {
        snprintf(symboln, sizeof(symboln), "%sIndexMask", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IndexMask;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Accum) {
        snprintf(symboln, sizeof(symboln), "%sAccum", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Accum;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Disable) {
        snprintf(symboln, sizeof(symboln), "%sDisable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Disable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Enable) {
        snprintf(symboln, sizeof(symboln), "%sEnable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Enable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Finish) {
        snprintf(symboln, sizeof(symboln), "%sFinish", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Finish;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Flush) {
        snprintf(symboln, sizeof(symboln), "%sFlush", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Flush;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PopAttrib) {
        snprintf(symboln, sizeof(symboln), "%sPopAttrib", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PopAttrib;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PushAttrib) {
        snprintf(symboln, sizeof(symboln), "%sPushAttrib", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PushAttrib;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Map1d) {
        snprintf(symboln, sizeof(symboln), "%sMap1d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Map1d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Map1f) {
        snprintf(symboln, sizeof(symboln), "%sMap1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Map1f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Map2d) {
        snprintf(symboln, sizeof(symboln), "%sMap2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Map2d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Map2f) {
        snprintf(symboln, sizeof(symboln), "%sMap2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Map2f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapGrid1d) {
        snprintf(symboln, sizeof(symboln), "%sMapGrid1d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapGrid1d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapGrid1f) {
        snprintf(symboln, sizeof(symboln), "%sMapGrid1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapGrid1f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapGrid2d) {
        snprintf(symboln, sizeof(symboln), "%sMapGrid2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapGrid2d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapGrid2f) {
        snprintf(symboln, sizeof(symboln), "%sMapGrid2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapGrid2f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1d) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord1d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1dv) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord1dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1f) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord1f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1fv) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord1fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2d) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord2d;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2dv) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord2dv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2f) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord2f;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2fv) {
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalCoord2fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalMesh1) {
        snprintf(symboln, sizeof(symboln), "%sEvalMesh1", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalMesh1;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalPoint1) {
        snprintf(symboln, sizeof(symboln), "%sEvalPoint1", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalPoint1;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalMesh2) {
        snprintf(symboln, sizeof(symboln), "%sEvalMesh2", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalMesh2;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EvalPoint2) {
        snprintf(symboln, sizeof(symboln), "%sEvalPoint2", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EvalPoint2;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AlphaFunc) {
        snprintf(symboln, sizeof(symboln), "%sAlphaFunc", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AlphaFunc;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFunc) {
        snprintf(symboln, sizeof(symboln), "%sBlendFunc", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFunc;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LogicOp) {
        snprintf(symboln, sizeof(symboln), "%sLogicOp", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LogicOp;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilFunc) {
        snprintf(symboln, sizeof(symboln), "%sStencilFunc", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilFunc;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilOp) {
        snprintf(symboln, sizeof(symboln), "%sStencilOp", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilOp;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DepthFunc) {
        snprintf(symboln, sizeof(symboln), "%sDepthFunc", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DepthFunc;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelZoom) {
        snprintf(symboln, sizeof(symboln), "%sPixelZoom", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelZoom;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTransferf) {
        snprintf(symboln, sizeof(symboln), "%sPixelTransferf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTransferf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTransferi) {
        snprintf(symboln, sizeof(symboln), "%sPixelTransferi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTransferi;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelStoref) {
        snprintf(symboln, sizeof(symboln), "%sPixelStoref", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelStoref;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelStorei) {
        snprintf(symboln, sizeof(symboln), "%sPixelStorei", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelStorei;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelMapfv) {
        snprintf(symboln, sizeof(symboln), "%sPixelMapfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelMapfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelMapuiv) {
        snprintf(symboln, sizeof(symboln), "%sPixelMapuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelMapuiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelMapusv) {
        snprintf(symboln, sizeof(symboln), "%sPixelMapusv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelMapusv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ReadBuffer) {
        snprintf(symboln, sizeof(symboln), "%sReadBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ReadBuffer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyPixels) {
        snprintf(symboln, sizeof(symboln), "%sCopyPixels", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyPixels;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ReadPixels) {
        snprintf(symboln, sizeof(symboln), "%sReadPixels", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ReadPixels;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawPixels) {
        snprintf(symboln, sizeof(symboln), "%sDrawPixels", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawPixels;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBooleanv) {
        snprintf(symboln, sizeof(symboln), "%sGetBooleanv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBooleanv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetClipPlane) {
        snprintf(symboln, sizeof(symboln), "%sGetClipPlane", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetClipPlane;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetDoublev) {
        snprintf(symboln, sizeof(symboln), "%sGetDoublev", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetDoublev;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetError) {
        snprintf(symboln, sizeof(symboln), "%sGetError", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetError;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFloatv) {
        snprintf(symboln, sizeof(symboln), "%sGetFloatv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFloatv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetIntegerv) {
        snprintf(symboln, sizeof(symboln), "%sGetIntegerv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetIntegerv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetLightfv) {
        snprintf(symboln, sizeof(symboln), "%sGetLightfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetLightfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetLightiv) {
        snprintf(symboln, sizeof(symboln), "%sGetLightiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetLightiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMapdv) {
        snprintf(symboln, sizeof(symboln), "%sGetMapdv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMapdv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMapfv) {
        snprintf(symboln, sizeof(symboln), "%sGetMapfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMapfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMapiv) {
        snprintf(symboln, sizeof(symboln), "%sGetMapiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMapiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMaterialfv) {
        snprintf(symboln, sizeof(symboln), "%sGetMaterialfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMaterialfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMaterialiv) {
        snprintf(symboln, sizeof(symboln), "%sGetMaterialiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMaterialiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPixelMapfv) {
        snprintf(symboln, sizeof(symboln), "%sGetPixelMapfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPixelMapfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPixelMapuiv) {
        snprintf(symboln, sizeof(symboln), "%sGetPixelMapuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPixelMapuiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPixelMapusv) {
        snprintf(symboln, sizeof(symboln), "%sGetPixelMapusv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPixelMapusv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPolygonStipple) {
        snprintf(symboln, sizeof(symboln), "%sGetPolygonStipple", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPolygonStipple;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetString) {
        snprintf(symboln, sizeof(symboln), "%sGetString", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetString;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexEnvfv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexEnvfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexEnvfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexEnviv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexEnviv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexEnviv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexGendv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexGendv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexGendv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexGenfv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexGenfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexGenfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexGeniv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexGeniv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexGeniv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexImage) {
        snprintf(symboln, sizeof(symboln), "%sGetTexImage", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexImage;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexLevelParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexLevelParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexLevelParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexLevelParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetTexLevelParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexLevelParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsEnabled) {
        snprintf(symboln, sizeof(symboln), "%sIsEnabled", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsEnabled;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsList) {
        snprintf(symboln, sizeof(symboln), "%sIsList", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsList;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DepthRange) {
        snprintf(symboln, sizeof(symboln), "%sDepthRange", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DepthRange;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Frustum) {
        snprintf(symboln, sizeof(symboln), "%sFrustum", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Frustum;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadIdentity) {
        snprintf(symboln, sizeof(symboln), "%sLoadIdentity", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadIdentity;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadMatrixf) {
        snprintf(symboln, sizeof(symboln), "%sLoadMatrixf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadMatrixf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadMatrixd) {
        snprintf(symboln, sizeof(symboln), "%sLoadMatrixd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadMatrixd;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MatrixMode) {
        snprintf(symboln, sizeof(symboln), "%sMatrixMode", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MatrixMode;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultMatrixf) {
        snprintf(symboln, sizeof(symboln), "%sMultMatrixf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultMatrixf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultMatrixd) {
        snprintf(symboln, sizeof(symboln), "%sMultMatrixd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultMatrixd;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Ortho) {
        snprintf(symboln, sizeof(symboln), "%sOrtho", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Ortho;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PopMatrix) {
        snprintf(symboln, sizeof(symboln), "%sPopMatrix", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PopMatrix;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PushMatrix) {
        snprintf(symboln, sizeof(symboln), "%sPushMatrix", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PushMatrix;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rotated) {
        snprintf(symboln, sizeof(symboln), "%sRotated", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rotated;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Rotatef) {
        snprintf(symboln, sizeof(symboln), "%sRotatef", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Rotatef;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Scaled) {
        snprintf(symboln, sizeof(symboln), "%sScaled", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Scaled;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Scalef) {
        snprintf(symboln, sizeof(symboln), "%sScalef", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Scalef;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Translated) {
        snprintf(symboln, sizeof(symboln), "%sTranslated", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Translated;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Translatef) {
        snprintf(symboln, sizeof(symboln), "%sTranslatef", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Translatef;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Viewport) {
        snprintf(symboln, sizeof(symboln), "%sViewport", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Viewport;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ArrayElement) {
        snprintf(symboln, sizeof(symboln), "%sArrayElement", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ArrayElement;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ArrayElement) {
        snprintf(symboln, sizeof(symboln), "%sArrayElementEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ArrayElement;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindTexture) {
        snprintf(symboln, sizeof(symboln), "%sBindTexture", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindTexture;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindTexture) {
        snprintf(symboln, sizeof(symboln), "%sBindTextureEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindTexture;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorPointer) {
        snprintf(symboln, sizeof(symboln), "%sColorPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorPointer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DisableClientState) {
        snprintf(symboln, sizeof(symboln), "%sDisableClientState", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DisableClientState;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawArrays) {
        snprintf(symboln, sizeof(symboln), "%sDrawArrays", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawArrays;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawArrays) {
        snprintf(symboln, sizeof(symboln), "%sDrawArraysEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawArrays;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawElements) {
        snprintf(symboln, sizeof(symboln), "%sDrawElements", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawElements;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EdgeFlagPointer) {
        snprintf(symboln, sizeof(symboln), "%sEdgeFlagPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EdgeFlagPointer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EnableClientState) {
        snprintf(symboln, sizeof(symboln), "%sEnableClientState", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EnableClientState;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IndexPointer) {
        snprintf(symboln, sizeof(symboln), "%sIndexPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IndexPointer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexub) {
        snprintf(symboln, sizeof(symboln), "%sIndexub", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexub;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Indexubv) {
        snprintf(symboln, sizeof(symboln), "%sIndexubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Indexubv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->InterleavedArrays) {
        snprintf(symboln, sizeof(symboln), "%sInterleavedArrays", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->InterleavedArrays;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->NormalPointer) {
        snprintf(symboln, sizeof(symboln), "%sNormalPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->NormalPointer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PolygonOffset) {
        snprintf(symboln, sizeof(symboln), "%sPolygonOffset", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PolygonOffset;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoordPointer) {
        snprintf(symboln, sizeof(symboln), "%sTexCoordPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoordPointer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexPointer) {
        snprintf(symboln, sizeof(symboln), "%sVertexPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexPointer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AreTexturesResident) {
        snprintf(symboln, sizeof(symboln), "%sAreTexturesResident", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AreTexturesResident;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AreTexturesResident) {
        snprintf(symboln, sizeof(symboln), "%sAreTexturesResidentEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AreTexturesResident;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage1D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage1D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage1DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage2D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage2D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage1D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexSubImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage1D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage1DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexSubImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage2D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexSubImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage2D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexSubImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteTextures) {
        snprintf(symboln, sizeof(symboln), "%sDeleteTextures", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteTextures;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteTextures) {
        snprintf(symboln, sizeof(symboln), "%sDeleteTexturesEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteTextures;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenTextures) {
        snprintf(symboln, sizeof(symboln), "%sGenTextures", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenTextures;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenTextures) {
        snprintf(symboln, sizeof(symboln), "%sGenTexturesEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenTextures;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPointerv) {
        snprintf(symboln, sizeof(symboln), "%sGetPointerv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPointerv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPointerv) {
        snprintf(symboln, sizeof(symboln), "%sGetPointervEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPointerv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsTexture) {
        snprintf(symboln, sizeof(symboln), "%sIsTexture", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsTexture;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsTexture) {
        snprintf(symboln, sizeof(symboln), "%sIsTextureEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsTexture;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PrioritizeTextures) {
        snprintf(symboln, sizeof(symboln), "%sPrioritizeTextures", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PrioritizeTextures;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PrioritizeTextures) {
        snprintf(symboln, sizeof(symboln), "%sPrioritizeTexturesEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PrioritizeTextures;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexSubImage1D) {
        snprintf(symboln, sizeof(symboln), "%sTexSubImage1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexSubImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexSubImage1D) {
        snprintf(symboln, sizeof(symboln), "%sTexSubImage1DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexSubImage1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexSubImage2D) {
        snprintf(symboln, sizeof(symboln), "%sTexSubImage2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexSubImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexSubImage2D) {
        snprintf(symboln, sizeof(symboln), "%sTexSubImage2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexSubImage2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PopClientAttrib) {
        snprintf(symboln, sizeof(symboln), "%sPopClientAttrib", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PopClientAttrib;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PushClientAttrib) {
        snprintf(symboln, sizeof(symboln), "%sPushClientAttrib", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PushClientAttrib;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendColor) {
        snprintf(symboln, sizeof(symboln), "%sBlendColor", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendColor;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendColor) {
        snprintf(symboln, sizeof(symboln), "%sBlendColorEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendColor;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquation) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquation", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquation;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquation) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquation;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawRangeElements) {
        snprintf(symboln, sizeof(symboln), "%sDrawRangeElements", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawRangeElements;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawRangeElements) {
        snprintf(symboln, sizeof(symboln), "%sDrawRangeElementsEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawRangeElements;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTable) {
        snprintf(symboln, sizeof(symboln), "%sColorTable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTable) {
        snprintf(symboln, sizeof(symboln), "%sColorTableSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTable) {
        snprintf(symboln, sizeof(symboln), "%sColorTableEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sColorTableParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTableParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sColorTableParameterfvSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTableParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sColorTableParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTableParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sColorTableParameterivSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorTableParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyColorTable) {
        snprintf(symboln, sizeof(symboln), "%sCopyColorTable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyColorTable) {
        snprintf(symboln, sizeof(symboln), "%sCopyColorTableSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTable) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTable) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTable) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTableParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfvSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTableParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTableParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTableParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterivSGI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTableParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetColorTableParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorSubTable) {
        snprintf(symboln, sizeof(symboln), "%sColorSubTable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorSubTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorSubTable) {
        snprintf(symboln, sizeof(symboln), "%sColorSubTableEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorSubTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyColorSubTable) {
        snprintf(symboln, sizeof(symboln), "%sCopyColorSubTable", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyColorSubTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyColorSubTable) {
        snprintf(symboln, sizeof(symboln), "%sCopyColorSubTableEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyColorSubTable;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter1D) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionFilter1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter1D) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter1DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionFilter1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter2D) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionFilter2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter2D) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionFilter2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterf) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameterf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterf) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameterf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteri) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameteri", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameteri;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteri) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameteriEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameteri;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ConvolutionParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter1D) {
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyConvolutionFilter1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter1D) {
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter1DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyConvolutionFilter1D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter2D) {
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyConvolutionFilter2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter2D) {
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyConvolutionFilter2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionFilter) {
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionFilter", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetConvolutionFilter;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionFilter) {
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionFilterEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetConvolutionFilter;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetConvolutionParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetConvolutionParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetConvolutionParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetConvolutionParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSeparableFilter) {
        snprintf(symboln, sizeof(symboln), "%sGetSeparableFilter", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSeparableFilter;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSeparableFilter) {
        snprintf(symboln, sizeof(symboln), "%sGetSeparableFilterEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSeparableFilter;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SeparableFilter2D) {
        snprintf(symboln, sizeof(symboln), "%sSeparableFilter2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SeparableFilter2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SeparableFilter2D) {
        snprintf(symboln, sizeof(symboln), "%sSeparableFilter2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SeparableFilter2D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHistogram) {
        snprintf(symboln, sizeof(symboln), "%sGetHistogram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHistogram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHistogram) {
        snprintf(symboln, sizeof(symboln), "%sGetHistogramEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHistogram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHistogramParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHistogramParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHistogramParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHistogramParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMinmax) {
        snprintf(symboln, sizeof(symboln), "%sGetMinmax", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMinmax;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMinmax) {
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMinmax;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMinmaxParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMinmaxParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMinmaxParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetMinmaxParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Histogram) {
        snprintf(symboln, sizeof(symboln), "%sHistogram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Histogram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Histogram) {
        snprintf(symboln, sizeof(symboln), "%sHistogramEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Histogram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Minmax) {
        snprintf(symboln, sizeof(symboln), "%sMinmax", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Minmax;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Minmax) {
        snprintf(symboln, sizeof(symboln), "%sMinmaxEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Minmax;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ResetHistogram) {
        snprintf(symboln, sizeof(symboln), "%sResetHistogram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ResetHistogram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ResetHistogram) {
        snprintf(symboln, sizeof(symboln), "%sResetHistogramEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ResetHistogram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ResetMinmax) {
        snprintf(symboln, sizeof(symboln), "%sResetMinmax", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ResetMinmax;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ResetMinmax) {
        snprintf(symboln, sizeof(symboln), "%sResetMinmaxEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ResetMinmax;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexImage3D) {
        snprintf(symboln, sizeof(symboln), "%sTexImage3D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexImage3D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexImage3D) {
        snprintf(symboln, sizeof(symboln), "%sTexImage3DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexImage3D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexSubImage3D) {
        snprintf(symboln, sizeof(symboln), "%sTexSubImage3D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexSubImage3D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexSubImage3D) {
        snprintf(symboln, sizeof(symboln), "%sTexSubImage3DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexSubImage3D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage3D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage3D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexSubImage3D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage3D) {
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage3DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyTexSubImage3D;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ActiveTextureARB) {
        snprintf(symboln, sizeof(symboln), "%sActiveTexture", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ActiveTextureARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ActiveTextureARB) {
        snprintf(symboln, sizeof(symboln), "%sActiveTextureARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ActiveTextureARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClientActiveTextureARB) {
        snprintf(symboln, sizeof(symboln), "%sClientActiveTexture", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClientActiveTextureARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClientActiveTextureARB) {
        snprintf(symboln, sizeof(symboln), "%sClientActiveTextureARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClientActiveTextureARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord1svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord2svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord3svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4iARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4ivARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4sARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4svARB) {
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiTexCoord4svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AttachShader) {
        snprintf(symboln, sizeof(symboln), "%sAttachShader", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AttachShader;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CreateProgram) {
        snprintf(symboln, sizeof(symboln), "%sCreateProgram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CreateProgram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CreateShader) {
        snprintf(symboln, sizeof(symboln), "%sCreateShader", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CreateShader;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteProgram) {
        snprintf(symboln, sizeof(symboln), "%sDeleteProgram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteProgram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteShader) {
        snprintf(symboln, sizeof(symboln), "%sDeleteShader", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteShader;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DetachShader) {
        snprintf(symboln, sizeof(symboln), "%sDetachShader", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DetachShader;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetAttachedShaders) {
        snprintf(symboln, sizeof(symboln), "%sGetAttachedShaders", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetAttachedShaders;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramInfoLog) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramInfoLog", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramInfoLog;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramiv) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetShaderInfoLog) {
        snprintf(symboln, sizeof(symboln), "%sGetShaderInfoLog", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetShaderInfoLog;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetShaderiv) {
        snprintf(symboln, sizeof(symboln), "%sGetShaderiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetShaderiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsProgram) {
        snprintf(symboln, sizeof(symboln), "%sIsProgram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsProgram;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsShader) {
        snprintf(symboln, sizeof(symboln), "%sIsShader", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsShader;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilFuncSeparate) {
        snprintf(symboln, sizeof(symboln), "%sStencilFuncSeparate", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilFuncSeparate;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilMaskSeparate) {
        snprintf(symboln, sizeof(symboln), "%sStencilMaskSeparate", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilMaskSeparate;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilOpSeparate) {
        snprintf(symboln, sizeof(symboln), "%sStencilOpSeparate", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilOpSeparate;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilOpSeparate) {
        snprintf(symboln, sizeof(symboln), "%sStencilOpSeparateATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilOpSeparate;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2x3fv) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2x3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix2x3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2x4fv) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2x4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix2x4fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3x2fv) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3x2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix3x2fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3x4fv) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3x4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix3x4fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4x2fv) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4x2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix4x2fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4x3fv) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4x3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix4x3fv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClampColor) {
        snprintf(symboln, sizeof(symboln), "%sClampColor", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClampColor;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearBufferfi) {
        snprintf(symboln, sizeof(symboln), "%sClearBufferfi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearBufferfi;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearBufferfv) {
        snprintf(symboln, sizeof(symboln), "%sClearBufferfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearBufferfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearBufferiv) {
        snprintf(symboln, sizeof(symboln), "%sClearBufferiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearBufferiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearBufferuiv) {
        snprintf(symboln, sizeof(symboln), "%sClearBufferuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearBufferuiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetStringi) {
        snprintf(symboln, sizeof(symboln), "%sGetStringi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetStringi;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexBuffer) {
        snprintf(symboln, sizeof(symboln), "%sTexBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexBuffer;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferParameteri64v) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferParameteri64v", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferParameteri64v;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetInteger64i_v) {
        snprintf(symboln, sizeof(symboln), "%sGetInteger64i_v", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetInteger64i_v;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribDivisor) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribDivisor", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribDivisor;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixdARB) {
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadTransposeMatrixdARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixdARB) {
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixdARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadTransposeMatrixdARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixfARB) {
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadTransposeMatrixfARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixfARB) {
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixfARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadTransposeMatrixfARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixdARB) {
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultTransposeMatrixdARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixdARB) {
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixdARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultTransposeMatrixdARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixfARB) {
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultTransposeMatrixfARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixfARB) {
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixfARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultTransposeMatrixfARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SampleCoverageARB) {
        snprintf(symboln, sizeof(symboln), "%sSampleCoverage", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SampleCoverageARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SampleCoverageARB) {
        snprintf(symboln, sizeof(symboln), "%sSampleCoverageARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SampleCoverageARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage1DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexImage1DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage1DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage1DARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexImage1DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage2DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexImage2DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage2DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage2DARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexImage2DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage3DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage3D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexImage3DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage3DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage3DARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexImage3DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage1DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexSubImage1DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage1DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage1DARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexSubImage1DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage2DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexSubImage2DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage2DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage2DARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexSubImage2DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage3DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage3D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexSubImage3DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage3DARB) {
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage3DARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompressedTexSubImage3DARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetCompressedTexImageARB) {
        snprintf(symboln, sizeof(symboln), "%sGetCompressedTexImage", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetCompressedTexImageARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetCompressedTexImageARB) {
        snprintf(symboln, sizeof(symboln), "%sGetCompressedTexImageARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetCompressedTexImageARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DisableVertexAttribArrayARB) {
        snprintf(symboln, sizeof(symboln), "%sDisableVertexAttribArray", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DisableVertexAttribArrayARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DisableVertexAttribArrayARB) {
        snprintf(symboln, sizeof(symboln), "%sDisableVertexAttribArrayARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DisableVertexAttribArrayARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EnableVertexAttribArrayARB) {
        snprintf(symboln, sizeof(symboln), "%sEnableVertexAttribArray", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EnableVertexAttribArrayARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EnableVertexAttribArrayARB) {
        snprintf(symboln, sizeof(symboln), "%sEnableVertexAttribArrayARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EnableVertexAttribArrayARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramEnvParameterdvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramEnvParameterdvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramEnvParameterdvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramEnvParameterfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramEnvParameterfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramEnvParameterfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramLocalParameterdvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramLocalParameterdvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramLocalParameterdvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramLocalParameterfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramLocalParameterfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramLocalParameterfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramStringARB) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramStringARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramStringARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribdvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribdvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribdvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribdvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4dNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4fNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameter4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4dARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramLocalParameter4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramLocalParameter4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4fARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramLocalParameter4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramLocalParameter4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramStringARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramStringARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramStringARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NbvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nbv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NbvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NbvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NbvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NbvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Niv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NsvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nsv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NsvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NsvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NsvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NsvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nub", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NubARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NubARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NubARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NubvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NubvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NubvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NuivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NuivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NuivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NuivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NuivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NusvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nusv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NusvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NusvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NusvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4NusvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4bvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4bv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4bvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4bvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4bvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4bvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4dARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4dvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4sARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4sARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4svARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4svARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4ubvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4ubvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4uivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4uivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4uivARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4uivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4uivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4usvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4usv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4usvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4usvARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4usvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4usvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribPointerARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribPointerARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribPointerARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribPointerARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribPointerARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sBindBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sBindBufferARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BufferDataARB) {
        snprintf(symboln, sizeof(symboln), "%sBufferData", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BufferDataARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BufferDataARB) {
        snprintf(symboln, sizeof(symboln), "%sBufferDataARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BufferDataARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BufferSubDataARB) {
        snprintf(symboln, sizeof(symboln), "%sBufferSubData", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BufferSubDataARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BufferSubDataARB) {
        snprintf(symboln, sizeof(symboln), "%sBufferSubDataARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BufferSubDataARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sDeleteBuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sDeleteBuffersARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sGenBuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sGenBuffersARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferParameterivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferParameterivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferParameterivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferParameterivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferParameterivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferPointervARB) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferPointerv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferPointervARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferPointervARB) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferPointervARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferPointervARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferSubDataARB) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferSubData", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferSubDataARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBufferSubDataARB) {
        snprintf(symboln, sizeof(symboln), "%sGetBufferSubDataARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBufferSubDataARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sIsBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sIsBufferARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sMapBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sMapBufferARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UnmapBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sUnmapBuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UnmapBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UnmapBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sUnmapBufferARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UnmapBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginQueryARB) {
        snprintf(symboln, sizeof(symboln), "%sBeginQuery", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginQueryARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginQueryARB) {
        snprintf(symboln, sizeof(symboln), "%sBeginQueryARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginQueryARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteQueriesARB) {
        snprintf(symboln, sizeof(symboln), "%sDeleteQueries", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteQueriesARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteQueriesARB) {
        snprintf(symboln, sizeof(symboln), "%sDeleteQueriesARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteQueriesARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndQueryARB) {
        snprintf(symboln, sizeof(symboln), "%sEndQuery", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndQueryARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndQueryARB) {
        snprintf(symboln, sizeof(symboln), "%sEndQueryARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndQueryARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenQueriesARB) {
        snprintf(symboln, sizeof(symboln), "%sGenQueries", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenQueriesARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenQueriesARB) {
        snprintf(symboln, sizeof(symboln), "%sGenQueriesARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenQueriesARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryObjectivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryObjectivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectuivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryObjectuivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectuivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectuivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryObjectuivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsQueryARB) {
        snprintf(symboln, sizeof(symboln), "%sIsQuery", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsQueryARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsQueryARB) {
        snprintf(symboln, sizeof(symboln), "%sIsQueryARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsQueryARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AttachObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sAttachObjectARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AttachObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompileShaderARB) {
        snprintf(symboln, sizeof(symboln), "%sCompileShader", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompileShaderARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CompileShaderARB) {
        snprintf(symboln, sizeof(symboln), "%sCompileShaderARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CompileShaderARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CreateProgramObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sCreateProgramObjectARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CreateProgramObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CreateShaderObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sCreateShaderObjectARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CreateShaderObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sDeleteObjectARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DetachObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sDetachObjectARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DetachObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetActiveUniformARB) {
        snprintf(symboln, sizeof(symboln), "%sGetActiveUniform", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetActiveUniformARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetActiveUniformARB) {
        snprintf(symboln, sizeof(symboln), "%sGetActiveUniformARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetActiveUniformARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetAttachedObjectsARB) {
        snprintf(symboln, sizeof(symboln), "%sGetAttachedObjectsARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetAttachedObjectsARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetHandleARB) {
        snprintf(symboln, sizeof(symboln), "%sGetHandleARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetHandleARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetInfoLogARB) {
        snprintf(symboln, sizeof(symboln), "%sGetInfoLogARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetInfoLogARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetObjectParameterfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetObjectParameterfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetObjectParameterfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetObjectParameterivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetObjectParameterivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetObjectParameterivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetShaderSourceARB) {
        snprintf(symboln, sizeof(symboln), "%sGetShaderSource", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetShaderSourceARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetShaderSourceARB) {
        snprintf(symboln, sizeof(symboln), "%sGetShaderSourceARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetShaderSourceARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformLocationARB) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformLocation", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformLocationARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformLocationARB) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformLocationARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformLocationARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LinkProgramARB) {
        snprintf(symboln, sizeof(symboln), "%sLinkProgram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LinkProgramARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LinkProgramARB) {
        snprintf(symboln, sizeof(symboln), "%sLinkProgramARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LinkProgramARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ShaderSourceARB) {
        snprintf(symboln, sizeof(symboln), "%sShaderSource", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ShaderSourceARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ShaderSourceARB) {
        snprintf(symboln, sizeof(symboln), "%sShaderSourceARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ShaderSourceARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform1ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform2ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform3ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4fARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4fARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4iARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4iARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4ivARB) {
        snprintf(symboln, sizeof(symboln), "%sUniform4ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4ivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix2fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix3fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4fvARB) {
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UniformMatrix4fvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UseProgramObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sUseProgram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UseProgramObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UseProgramObjectARB) {
        snprintf(symboln, sizeof(symboln), "%sUseProgramObjectARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UseProgramObjectARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ValidateProgramARB) {
        snprintf(symboln, sizeof(symboln), "%sValidateProgram", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ValidateProgramARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ValidateProgramARB) {
        snprintf(symboln, sizeof(symboln), "%sValidateProgramARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ValidateProgramARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindAttribLocationARB) {
        snprintf(symboln, sizeof(symboln), "%sBindAttribLocation", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindAttribLocationARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindAttribLocationARB) {
        snprintf(symboln, sizeof(symboln), "%sBindAttribLocationARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindAttribLocationARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetActiveAttribARB) {
        snprintf(symboln, sizeof(symboln), "%sGetActiveAttrib", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetActiveAttribARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetActiveAttribARB) {
        snprintf(symboln, sizeof(symboln), "%sGetActiveAttribARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetActiveAttribARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetAttribLocationARB) {
        snprintf(symboln, sizeof(symboln), "%sGetAttribLocation", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetAttribLocationARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetAttribLocationARB) {
        snprintf(symboln, sizeof(symboln), "%sGetAttribLocationARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetAttribLocationARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawBuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawBuffersARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawBuffersATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawBuffersARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClampColorARB) {
        snprintf(symboln, sizeof(symboln), "%sClampColorARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClampColorARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawArraysInstancedARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawArraysInstancedARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawArraysInstancedARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawArraysInstancedARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawArraysInstancedEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawArraysInstancedARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawArraysInstancedARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawArraysInstanced", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawArraysInstancedARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawElementsInstancedARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawElementsInstancedARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedARB) {
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstanced", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawElementsInstancedARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageMultisample) {
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageMultisample", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RenderbufferStorageMultisample;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageMultisample) {
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageMultisampleEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RenderbufferStorageMultisample;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureARB) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTextureARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureFaceARB) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureFaceARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTextureFaceARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramParameteriARB) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameteriARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramParameteriARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribDivisorARB) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribDivisorARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribDivisorARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FlushMappedBufferRange) {
        snprintf(symboln, sizeof(symboln), "%sFlushMappedBufferRange", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FlushMappedBufferRange;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MapBufferRange) {
        snprintf(symboln, sizeof(symboln), "%sMapBufferRange", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MapBufferRange;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexBufferARB) {
        snprintf(symboln, sizeof(symboln), "%sTexBufferARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexBufferARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindVertexArray) {
        snprintf(symboln, sizeof(symboln), "%sBindVertexArray", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindVertexArray;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenVertexArrays) {
        snprintf(symboln, sizeof(symboln), "%sGenVertexArrays", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenVertexArrays;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CopyBufferSubData) {
        snprintf(symboln, sizeof(symboln), "%sCopyBufferSubData", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CopyBufferSubData;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClientWaitSync) {
        snprintf(symboln, sizeof(symboln), "%sClientWaitSync", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClientWaitSync;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteSync) {
        snprintf(symboln, sizeof(symboln), "%sDeleteSync", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteSync;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FenceSync) {
        snprintf(symboln, sizeof(symboln), "%sFenceSync", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FenceSync;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetInteger64v) {
        snprintf(symboln, sizeof(symboln), "%sGetInteger64v", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetInteger64v;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSynciv) {
        snprintf(symboln, sizeof(symboln), "%sGetSynciv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSynciv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsSync) {
        snprintf(symboln, sizeof(symboln), "%sIsSync", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsSync;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WaitSync) {
        snprintf(symboln, sizeof(symboln), "%sWaitSync", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WaitSync;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawElementsBaseVertex) {
        snprintf(symboln, sizeof(symboln), "%sDrawElementsBaseVertex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawElementsBaseVertex;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedBaseVertex) {
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedBaseVertex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawElementsInstancedBaseVertex;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawRangeElementsBaseVertex) {
        snprintf(symboln, sizeof(symboln), "%sDrawRangeElementsBaseVertex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawRangeElementsBaseVertex;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiDrawElementsBaseVertex) {
        snprintf(symboln, sizeof(symboln), "%sMultiDrawElementsBaseVertex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiDrawElementsBaseVertex;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateiARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateiARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationSeparateiARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateiARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateIndexedAMD", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationSeparateiARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationiARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationiARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationiARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationiARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationIndexedAMD", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationiARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateiARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateiARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFuncSeparateiARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateiARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateIndexedAMD", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFuncSeparateiARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFunciARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendFunciARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFunciARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFunciARB) {
        snprintf(symboln, sizeof(symboln), "%sBlendFuncIndexedAMD", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFunciARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindSampler) {
        snprintf(symboln, sizeof(symboln), "%sBindSampler", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindSampler;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteSamplers) {
        snprintf(symboln, sizeof(symboln), "%sDeleteSamplers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteSamplers;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenSamplers) {
        snprintf(symboln, sizeof(symboln), "%sGenSamplers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenSamplers;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameterIiv) {
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterIiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSamplerParameterIiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameterIuiv) {
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterIuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSamplerParameterIuiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSamplerParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetSamplerParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsSampler) {
        snprintf(symboln, sizeof(symboln), "%sIsSampler", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsSampler;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterIiv) {
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterIiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplerParameterIiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterIuiv) {
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterIuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplerParameterIuiv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterf) {
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplerParameterf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterfv) {
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplerParameterfv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplerParameteri) {
        snprintf(symboln, sizeof(symboln), "%sSamplerParameteri", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplerParameteri;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplerParameteriv) {
        snprintf(symboln, sizeof(symboln), "%sSamplerParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplerParameteriv;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindTransformFeedback) {
        snprintf(symboln, sizeof(symboln), "%sBindTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindTransformFeedback;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteTransformFeedbacks) {
        snprintf(symboln, sizeof(symboln), "%sDeleteTransformFeedbacks", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteTransformFeedbacks;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DrawTransformFeedback) {
        snprintf(symboln, sizeof(symboln), "%sDrawTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DrawTransformFeedback;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenTransformFeedbacks) {
        snprintf(symboln, sizeof(symboln), "%sGenTransformFeedbacks", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenTransformFeedbacks;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsTransformFeedback) {
        snprintf(symboln, sizeof(symboln), "%sIsTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsTransformFeedback;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PauseTransformFeedback) {
        snprintf(symboln, sizeof(symboln), "%sPauseTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PauseTransformFeedback;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ResumeTransformFeedback) {
        snprintf(symboln, sizeof(symboln), "%sResumeTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ResumeTransformFeedback;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearDepthf) {
        snprintf(symboln, sizeof(symboln), "%sClearDepthf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearDepthf;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DepthRangef) {
        snprintf(symboln, sizeof(symboln), "%sDepthRangef", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DepthRangef;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetShaderPrecisionFormat) {
        snprintf(symboln, sizeof(symboln), "%sGetShaderPrecisionFormat", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetShaderPrecisionFormat;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ReleaseShaderCompiler) {
        snprintf(symboln, sizeof(symboln), "%sReleaseShaderCompiler", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ReleaseShaderCompiler;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ShaderBinary) {
        snprintf(symboln, sizeof(symboln), "%sShaderBinary", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ShaderBinary;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetGraphicsResetStatusARB) {
        snprintf(symboln, sizeof(symboln), "%sGetGraphicsResetStatusARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetGraphicsResetStatusARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnColorTableARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnColorTableARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnColorTableARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnCompressedTexImageARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnCompressedTexImageARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnCompressedTexImageARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnConvolutionFilterARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnConvolutionFilterARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnConvolutionFilterARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnHistogramARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnHistogramARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnHistogramARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnMapdvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnMapdvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnMapdvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnMapfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnMapfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnMapfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnMapivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnMapivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnMapivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnMinmaxARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnMinmaxARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnMinmaxARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnPixelMapfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnPixelMapfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnPixelMapfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnPixelMapuivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnPixelMapuivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnPixelMapuivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnPixelMapusvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnPixelMapusvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnPixelMapusvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnPolygonStippleARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnPolygonStippleARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnPolygonStippleARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnSeparableFilterARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnSeparableFilterARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnSeparableFilterARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnTexImageARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnTexImageARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnTexImageARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnUniformdvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnUniformdvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnUniformdvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnUniformfvARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnUniformfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnUniformfvARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnUniformivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnUniformivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnUniformivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetnUniformuivARB) {
        snprintf(symboln, sizeof(symboln), "%sGetnUniformuivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetnUniformuivARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ReadnPixelsARB) {
        snprintf(symboln, sizeof(symboln), "%sReadnPixelsARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ReadnPixelsARB;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PolygonOffsetEXT) {
        snprintf(symboln, sizeof(symboln), "%sPolygonOffsetEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PolygonOffsetEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPixelTexGenParameterfvSGIS) {
        snprintf(symboln, sizeof(symboln), "%sGetPixelTexGenParameterfvSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPixelTexGenParameterfvSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetPixelTexGenParameterivSGIS) {
        snprintf(symboln, sizeof(symboln), "%sGetPixelTexGenParameterivSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetPixelTexGenParameterivSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameterfSGIS) {
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterfSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTexGenParameterfSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameterfvSGIS) {
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterfvSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTexGenParameterfvSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameteriSGIS) {
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameteriSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTexGenParameteriSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameterivSGIS) {
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterivSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTexGenParameterivSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SampleMaskSGIS) {
        snprintf(symboln, sizeof(symboln), "%sSampleMaskSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SampleMaskSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SampleMaskSGIS) {
        snprintf(symboln, sizeof(symboln), "%sSampleMaskEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SampleMaskSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplePatternSGIS) {
        snprintf(symboln, sizeof(symboln), "%sSamplePatternSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplePatternSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SamplePatternSGIS) {
        snprintf(symboln, sizeof(symboln), "%sSamplePatternEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SamplePatternSGIS;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sColorPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EdgeFlagPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sEdgeFlagPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EdgeFlagPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IndexPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sIndexPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IndexPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->NormalPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sNormalPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->NormalPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexCoordPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sTexCoordPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexCoordPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterfvSGIS", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterfvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LockArraysEXT) {
        snprintf(symboln, sizeof(symboln), "%sLockArraysEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LockArraysEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UnlockArraysEXT) {
        snprintf(symboln, sizeof(symboln), "%sUnlockArraysEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UnlockArraysEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3b", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3bEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3bEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3bvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3bvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3dEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3dEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3dvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3dvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3fEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3fEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3fvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3fvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3iEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3iEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3iEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3sEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3sEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3sEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3sEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3sEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3svEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3svEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3svEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3svEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3svEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ub", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3ubEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3ubEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3ubvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3ubvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3us", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3usEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3usEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3usvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usvEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColor3usvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColorPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColorPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColorPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SecondaryColorPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sSecondaryColorPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SecondaryColorPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiDrawArraysEXT) {
        snprintf(symboln, sizeof(symboln), "%sMultiDrawArrays", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiDrawArraysEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiDrawArraysEXT) {
        snprintf(symboln, sizeof(symboln), "%sMultiDrawArraysEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiDrawArraysEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiDrawElementsEXT) {
        snprintf(symboln, sizeof(symboln), "%sMultiDrawElements", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiDrawElementsEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiDrawElementsEXT) {
        snprintf(symboln, sizeof(symboln), "%sMultiDrawElementsEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiDrawElementsEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoordPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoordPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoordPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoordPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoorddEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordd", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoorddEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoorddEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoorddEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoorddEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoorddvEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoorddv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoorddvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoorddvEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoorddvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoorddvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoordfEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordf", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoordfEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoordfEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordfEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoordfEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoordfvEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordfv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoordfvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FogCoordfvEXT) {
        snprintf(symboln, sizeof(symboln), "%sFogCoordfvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FogCoordfvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenSGIX) {
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenSGIX", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PixelTexGenSGIX;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparate", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFuncSeparateEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFuncSeparateEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateINGR", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendFuncSeparateEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FlushVertexArrayRangeNV) {
        snprintf(symboln, sizeof(symboln), "%sFlushVertexArrayRangeNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FlushVertexArrayRangeNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexArrayRangeNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexArrayRangeNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexArrayRangeNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CombinerInputNV) {
        snprintf(symboln, sizeof(symboln), "%sCombinerInputNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CombinerInputNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CombinerOutputNV) {
        snprintf(symboln, sizeof(symboln), "%sCombinerOutputNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CombinerOutputNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CombinerParameterfNV) {
        snprintf(symboln, sizeof(symboln), "%sCombinerParameterfNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CombinerParameterfNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CombinerParameterfvNV) {
        snprintf(symboln, sizeof(symboln), "%sCombinerParameterfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CombinerParameterfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CombinerParameteriNV) {
        snprintf(symboln, sizeof(symboln), "%sCombinerParameteriNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CombinerParameteriNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CombinerParameterivNV) {
        snprintf(symboln, sizeof(symboln), "%sCombinerParameterivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CombinerParameterivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FinalCombinerInputNV) {
        snprintf(symboln, sizeof(symboln), "%sFinalCombinerInputNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FinalCombinerInputNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetCombinerInputParameterfvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetCombinerInputParameterfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetCombinerInputParameterfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetCombinerInputParameterivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetCombinerInputParameterivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetCombinerInputParameterivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetCombinerOutputParameterfvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetCombinerOutputParameterfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetCombinerOutputParameterfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetCombinerOutputParameterivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetCombinerOutputParameterivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetCombinerOutputParameterivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFinalCombinerInputParameterfvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetFinalCombinerInputParameterfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFinalCombinerInputParameterfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFinalCombinerInputParameterivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetFinalCombinerInputParameterivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFinalCombinerInputParameterivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ResizeBuffersMESA) {
        snprintf(symboln, sizeof(symboln), "%sResizeBuffersMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ResizeBuffersMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dvMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fvMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2iMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2ivMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2sMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos2svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos2svMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos2svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3d", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dvMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3f", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fvARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fvMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3iARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3iMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3ivARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3ivMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3s", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3sARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3sMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3svARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos3svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos3svMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos3svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4dMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4dMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4dMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4dvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4dvMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4dvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4fMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4fMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4fMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4fvMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4fvMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4fvMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4iMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4iMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4iMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4ivMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4ivMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4ivMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4sMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4sMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4sMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->WindowPos4svMESA) {
        snprintf(symboln, sizeof(symboln), "%sWindowPos4svMESA", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->WindowPos4svMESA;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiModeDrawArraysIBM) {
        snprintf(symboln, sizeof(symboln), "%sMultiModeDrawArraysIBM", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiModeDrawArraysIBM;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->MultiModeDrawElementsIBM) {
        snprintf(symboln, sizeof(symboln), "%sMultiModeDrawElementsIBM", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->MultiModeDrawElementsIBM;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteFencesNV) {
        snprintf(symboln, sizeof(symboln), "%sDeleteFencesNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteFencesNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FinishFenceNV) {
        snprintf(symboln, sizeof(symboln), "%sFinishFenceNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FinishFenceNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenFencesNV) {
        snprintf(symboln, sizeof(symboln), "%sGenFencesNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenFencesNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFenceivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetFenceivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFenceivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsFenceNV) {
        snprintf(symboln, sizeof(symboln), "%sIsFenceNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsFenceNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SetFenceNV) {
        snprintf(symboln, sizeof(symboln), "%sSetFenceNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SetFenceNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TestFenceNV) {
        snprintf(symboln, sizeof(symboln), "%sTestFenceNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TestFenceNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AreProgramsResidentNV) {
        snprintf(symboln, sizeof(symboln), "%sAreProgramsResidentNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AreProgramsResidentNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindProgramNV) {
        snprintf(symboln, sizeof(symboln), "%sBindProgramARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindProgramNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindProgramNV) {
        snprintf(symboln, sizeof(symboln), "%sBindProgramNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindProgramNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteProgramsNV) {
        snprintf(symboln, sizeof(symboln), "%sDeleteProgramsARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteProgramsNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteProgramsNV) {
        snprintf(symboln, sizeof(symboln), "%sDeleteProgramsNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteProgramsNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ExecuteProgramNV) {
        snprintf(symboln, sizeof(symboln), "%sExecuteProgramNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ExecuteProgramNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenProgramsNV) {
        snprintf(symboln, sizeof(symboln), "%sGenProgramsARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenProgramsNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenProgramsNV) {
        snprintf(symboln, sizeof(symboln), "%sGenProgramsNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenProgramsNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramParameterdvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramParameterdvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramParameterdvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramParameterfvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramParameterfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramParameterfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramStringNV) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramStringNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramStringNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTrackMatrixivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetTrackMatrixivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTrackMatrixivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribPointervNV) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointerv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribPointervNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribPointervNV) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointervARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribPointervNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribPointervNV) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointervNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribPointervNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribdvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribdvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribfvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribivNV) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsProgramNV) {
        snprintf(symboln, sizeof(symboln), "%sIsProgramARB", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsProgramNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsProgramNV) {
        snprintf(symboln, sizeof(symboln), "%sIsProgramNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsProgramNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->LoadProgramNV) {
        snprintf(symboln, sizeof(symboln), "%sLoadProgramNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->LoadProgramNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramParameters4dvNV) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameters4dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramParameters4dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramParameters4fvNV) {
        snprintf(symboln, sizeof(symboln), "%sProgramParameters4fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramParameters4fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RequestResidentProgramsNV) {
        snprintf(symboln, sizeof(symboln), "%sRequestResidentProgramsNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RequestResidentProgramsNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TrackMatrixNV) {
        snprintf(symboln, sizeof(symboln), "%sTrackMatrixNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TrackMatrixNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1dNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1fNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1sNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1sNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib1svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2dNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2fNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2sNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2sNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib2svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3dNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3fNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3sNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3sNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib3svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4dNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4fNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4sNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4sNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4ubNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttrib4ubvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribPointerNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribPointerNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribPointerNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs1dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs1dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs1dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs1fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs1fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs1fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs1svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs1svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs1svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs2dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs2dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs2dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs2fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs2fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs2fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs2svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs2svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs2svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs3dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs3dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs3dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs3fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs3fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs3fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs3svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs3svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs3svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4dvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs4dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4fvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs4fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4svNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4svNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs4svNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4ubvNV) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4ubvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribs4ubvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexBumpParameterfvATI) {
        snprintf(symboln, sizeof(symboln), "%sGetTexBumpParameterfvATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexBumpParameterfvATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexBumpParameterivATI) {
        snprintf(symboln, sizeof(symboln), "%sGetTexBumpParameterivATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexBumpParameterivATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexBumpParameterfvATI) {
        snprintf(symboln, sizeof(symboln), "%sTexBumpParameterfvATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexBumpParameterfvATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexBumpParameterivATI) {
        snprintf(symboln, sizeof(symboln), "%sTexBumpParameterivATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexBumpParameterivATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AlphaFragmentOp1ATI) {
        snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp1ATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AlphaFragmentOp1ATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AlphaFragmentOp2ATI) {
        snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp2ATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AlphaFragmentOp2ATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->AlphaFragmentOp3ATI) {
        snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp3ATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->AlphaFragmentOp3ATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginFragmentShaderATI) {
        snprintf(symboln, sizeof(symboln), "%sBeginFragmentShaderATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginFragmentShaderATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindFragmentShaderATI) {
        snprintf(symboln, sizeof(symboln), "%sBindFragmentShaderATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindFragmentShaderATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorFragmentOp1ATI) {
        snprintf(symboln, sizeof(symboln), "%sColorFragmentOp1ATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorFragmentOp1ATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorFragmentOp2ATI) {
        snprintf(symboln, sizeof(symboln), "%sColorFragmentOp2ATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorFragmentOp2ATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorFragmentOp3ATI) {
        snprintf(symboln, sizeof(symboln), "%sColorFragmentOp3ATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorFragmentOp3ATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteFragmentShaderATI) {
        snprintf(symboln, sizeof(symboln), "%sDeleteFragmentShaderATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteFragmentShaderATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndFragmentShaderATI) {
        snprintf(symboln, sizeof(symboln), "%sEndFragmentShaderATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndFragmentShaderATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenFragmentShadersATI) {
        snprintf(symboln, sizeof(symboln), "%sGenFragmentShadersATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenFragmentShadersATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PassTexCoordATI) {
        snprintf(symboln, sizeof(symboln), "%sPassTexCoordATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PassTexCoordATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SampleMapATI) {
        snprintf(symboln, sizeof(symboln), "%sSampleMapATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SampleMapATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->SetFragmentShaderConstantATI) {
        snprintf(symboln, sizeof(symboln), "%sSetFragmentShaderConstantATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->SetFragmentShaderConstantATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameteriNV) {
        snprintf(symboln, sizeof(symboln), "%sPointParameteri", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameteriNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameteriNV) {
        snprintf(symboln, sizeof(symboln), "%sPointParameteriNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameteriNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterivNV) {
        snprintf(symboln, sizeof(symboln), "%sPointParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PointParameterivNV) {
        snprintf(symboln, sizeof(symboln), "%sPointParameterivNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PointParameterivNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ActiveStencilFaceEXT) {
        snprintf(symboln, sizeof(symboln), "%sActiveStencilFaceEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ActiveStencilFaceEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindVertexArrayAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sBindVertexArrayAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindVertexArrayAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteVertexArraysAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sDeleteVertexArrays", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteVertexArraysAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteVertexArraysAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sDeleteVertexArraysAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteVertexArraysAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenVertexArraysAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sGenVertexArraysAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenVertexArraysAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsVertexArrayAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sIsVertexArray", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsVertexArrayAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsVertexArrayAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sIsVertexArrayAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsVertexArrayAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramNamedParameterdvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramNamedParameterdvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramNamedParameterdvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetProgramNamedParameterfvNV) {
        snprintf(symboln, sizeof(symboln), "%sGetProgramNamedParameterfvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetProgramNamedParameterfvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4dNV) {
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4dNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramNamedParameter4dNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4dvNV) {
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4dvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramNamedParameter4dvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4fNV) {
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4fNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramNamedParameter4fNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4fvNV) {
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4fvNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramNamedParameter4fvNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PrimitiveRestartIndexNV) {
        snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartIndexNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PrimitiveRestartIndexNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PrimitiveRestartIndexNV) {
        snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartIndex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PrimitiveRestartIndexNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->PrimitiveRestartNV) {
        snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->PrimitiveRestartNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DepthBoundsEXT) {
        snprintf(symboln, sizeof(symboln), "%sDepthBoundsEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DepthBoundsEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparate", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationSeparateEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationSeparateEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlendEquationSeparateEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindFramebufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindFramebuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindFramebufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindFramebufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindFramebufferEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindFramebufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindRenderbufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindRenderbuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindRenderbufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindRenderbufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindRenderbufferEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindRenderbufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CheckFramebufferStatusEXT) {
        snprintf(symboln, sizeof(symboln), "%sCheckFramebufferStatus", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CheckFramebufferStatusEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CheckFramebufferStatusEXT) {
        snprintf(symboln, sizeof(symboln), "%sCheckFramebufferStatusEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CheckFramebufferStatusEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteFramebuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sDeleteFramebuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteFramebuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteFramebuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sDeleteFramebuffersEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteFramebuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteRenderbuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sDeleteRenderbuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteRenderbuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DeleteRenderbuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sDeleteRenderbuffersEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DeleteRenderbuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferRenderbufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferRenderbuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferRenderbufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferRenderbufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferRenderbufferEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferRenderbufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture1DEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture1D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture1DEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture1DEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture1DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture1DEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture2DEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture2D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture2DEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture2DEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture2DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture2DEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture3DEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture3D", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture3DEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture3DEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture3DEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTexture3DEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenFramebuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sGenFramebuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenFramebuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenFramebuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sGenFramebuffersEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenFramebuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenRenderbuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sGenRenderbuffers", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenRenderbuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenRenderbuffersEXT) {
        snprintf(symboln, sizeof(symboln), "%sGenRenderbuffersEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenRenderbuffersEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenerateMipmapEXT) {
        snprintf(symboln, sizeof(symboln), "%sGenerateMipmap", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenerateMipmapEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GenerateMipmapEXT) {
        snprintf(symboln, sizeof(symboln), "%sGenerateMipmapEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GenerateMipmapEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFramebufferAttachmentParameterivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetFramebufferAttachmentParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFramebufferAttachmentParameterivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFramebufferAttachmentParameterivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetFramebufferAttachmentParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFramebufferAttachmentParameterivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetRenderbufferParameterivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetRenderbufferParameteriv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetRenderbufferParameterivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetRenderbufferParameterivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetRenderbufferParameterivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetRenderbufferParameterivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsFramebufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sIsFramebuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsFramebufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsFramebufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sIsFramebufferEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsFramebufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsRenderbufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sIsRenderbuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsRenderbufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsRenderbufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sIsRenderbufferEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsRenderbufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageEXT) {
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorage", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RenderbufferStorageEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageEXT) {
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->RenderbufferStorageEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlitFramebufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlitFramebuffer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlitFramebufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BlitFramebufferEXT) {
        snprintf(symboln, sizeof(symboln), "%sBlitFramebufferEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BlitFramebufferEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BufferParameteriAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sBufferParameteriAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BufferParameteriAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FlushMappedBufferRangeAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sFlushMappedBufferRangeAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FlushMappedBufferRangeAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindFragDataLocationEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindFragDataLocationEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindFragDataLocationEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindFragDataLocationEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindFragDataLocation", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindFragDataLocationEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFragDataLocationEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetFragDataLocationEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFragDataLocationEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetFragDataLocationEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetFragDataLocation", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetFragDataLocationEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformuivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetUniformuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetUniformuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetUniformuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribIivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribIivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIuivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribIuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetVertexAttribIuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform1uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform1ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform1uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform1uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform1uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform1uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform2uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform2ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform2uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform2uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform2uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform2uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform3uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform3ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform3uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform3uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform3uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform3uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform4uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform4ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform4uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->Uniform4uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sUniform4uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->Uniform4uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1iEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1ivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI1uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2iEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2ivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI2uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3iEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3ivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI3uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4bvEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4bvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4bvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4bvEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4bv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4bvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4iEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4iEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4i", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4iEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4iv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4ivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4svEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4svEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4svEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4svEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4sv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4svEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ubvEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ubvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4ubvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ubvEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ubv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4ubvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uiEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ui", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4uiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uivEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4uivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4usvEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4usvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4usvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4usvEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4usv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribI4usvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribIPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribIPointerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribIPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->VertexAttribIPointerEXT) {
        snprintf(symboln, sizeof(symboln), "%sVertexAttribIPointer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->VertexAttribIPointerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureLayerEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayer", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTextureLayerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureLayerEXT) {
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayerEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->FramebufferTextureLayerEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorMaskIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sColorMaskIndexedEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorMaskIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ColorMaskIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sColorMaski", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ColorMaskIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DisableIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sDisableIndexedEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DisableIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->DisableIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sDisablei", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->DisableIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EnableIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sEnableIndexedEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EnableIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EnableIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sEnablei", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EnableIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBooleanIndexedvEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetBooleanIndexedvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBooleanIndexedvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetBooleanIndexedvEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetBooleani_v", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetBooleanIndexedvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetIntegerIndexedvEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetIntegerIndexedvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetIntegerIndexedvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetIntegerIndexedvEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetIntegeri_v", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetIntegerIndexedvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsEnabledIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sIsEnabledIndexedEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsEnabledIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->IsEnabledIndexedEXT) {
        snprintf(symboln, sizeof(symboln), "%sIsEnabledi", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->IsEnabledIndexedEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearColorIiEXT) {
        snprintf(symboln, sizeof(symboln), "%sClearColorIiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearColorIiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ClearColorIuiEXT) {
        snprintf(symboln, sizeof(symboln), "%sClearColorIuiEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ClearColorIuiEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameterIivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameterIivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIuivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameterIuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameterIuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameterIivEXT) {
        snprintf(symboln, sizeof(symboln), "%sTexParameterIivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameterIivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameterIivEXT) {
        snprintf(symboln, sizeof(symboln), "%sTexParameterIiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameterIivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameterIuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sTexParameterIuivEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameterIuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TexParameterIuivEXT) {
        snprintf(symboln, sizeof(symboln), "%sTexParameterIuiv", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TexParameterIuivEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginConditionalRenderNV) {
        snprintf(symboln, sizeof(symboln), "%sBeginConditionalRenderNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginConditionalRenderNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginConditionalRenderNV) {
        snprintf(symboln, sizeof(symboln), "%sBeginConditionalRender", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginConditionalRenderNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndConditionalRenderNV) {
        snprintf(symboln, sizeof(symboln), "%sEndConditionalRenderNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndConditionalRenderNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndConditionalRenderNV) {
        snprintf(symboln, sizeof(symboln), "%sEndConditionalRender", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndConditionalRenderNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginTransformFeedbackEXT) {
        snprintf(symboln, sizeof(symboln), "%sBeginTransformFeedbackEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginTransformFeedbackEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BeginTransformFeedbackEXT) {
        snprintf(symboln, sizeof(symboln), "%sBeginTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BeginTransformFeedbackEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferBaseEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindBufferBaseEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferBaseEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferBaseEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindBufferBase", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferBaseEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferOffsetEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindBufferOffsetEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferOffsetEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferRangeEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindBufferRangeEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferRangeEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->BindBufferRangeEXT) {
        snprintf(symboln, sizeof(symboln), "%sBindBufferRange", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->BindBufferRangeEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndTransformFeedbackEXT) {
        snprintf(symboln, sizeof(symboln), "%sEndTransformFeedbackEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndTransformFeedbackEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EndTransformFeedbackEXT) {
        snprintf(symboln, sizeof(symboln), "%sEndTransformFeedback", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EndTransformFeedbackEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTransformFeedbackVaryingEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetTransformFeedbackVaryingEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTransformFeedbackVaryingEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTransformFeedbackVaryingEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetTransformFeedbackVarying", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTransformFeedbackVaryingEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TransformFeedbackVaryingsEXT) {
        snprintf(symboln, sizeof(symboln), "%sTransformFeedbackVaryingsEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TransformFeedbackVaryingsEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TransformFeedbackVaryingsEXT) {
        snprintf(symboln, sizeof(symboln), "%sTransformFeedbackVaryings", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TransformFeedbackVaryingsEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProvokingVertexEXT) {
        snprintf(symboln, sizeof(symboln), "%sProvokingVertexEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProvokingVertexEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProvokingVertexEXT) {
        snprintf(symboln, sizeof(symboln), "%sProvokingVertex", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProvokingVertexEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterPointervAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterPointervAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetTexParameterPointervAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TextureRangeAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sTextureRangeAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TextureRangeAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetObjectParameterivAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sGetObjectParameterivAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetObjectParameterivAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ObjectPurgeableAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sObjectPurgeableAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ObjectPurgeableAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ObjectUnpurgeableAPPLE) {
        snprintf(symboln, sizeof(symboln), "%sObjectUnpurgeableAPPLE", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ObjectUnpurgeableAPPLE;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ActiveProgramEXT) {
        snprintf(symboln, sizeof(symboln), "%sActiveProgramEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ActiveProgramEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->CreateShaderProgramEXT) {
        snprintf(symboln, sizeof(symboln), "%sCreateShaderProgramEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->CreateShaderProgramEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->UseShaderProgramEXT) {
        snprintf(symboln, sizeof(symboln), "%sUseShaderProgramEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->UseShaderProgramEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->TextureBarrierNV) {
        snprintf(symboln, sizeof(symboln), "%sTextureBarrierNV", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->TextureBarrierNV;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->StencilFuncSeparateATI) {
        snprintf(symboln, sizeof(symboln), "%sStencilFuncSeparateATI", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->StencilFuncSeparateATI;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameters4fvEXT) {
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameters4fvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramEnvParameters4fvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameters4fvEXT) {
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameters4fvEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->ProgramLocalParameters4fvEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjecti64vEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjecti64vEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryObjecti64vEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectui64vEXT) {
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectui64vEXT", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->GetQueryObjectui64vEXT;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EGLImageTargetRenderbufferStorageOES) {
        snprintf(symboln, sizeof(symboln), "%sEGLImageTargetRenderbufferStorageOES", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EGLImageTargetRenderbufferStorageOES;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    if(!disp->EGLImageTargetTexture2DOES) {
        snprintf(symboln, sizeof(symboln), "%sEGLImageTargetTexture2DOES", symbol_prefix);
        _glapi_proc *procp = (_glapi_proc *)&disp->EGLImageTargetTexture2DOES;
        *procp = (_glapi_proc) dlsym(handle, symboln);
    }


    return disp;
}

