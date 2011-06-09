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
#include "main/dispatch.h"

struct _glapi_table *
_glapi_create_table_from_handle(void *handle, const char *symbol_prefix) {
   struct _glapi_table *disp = calloc(1, sizeof(struct _glapi_table));
   char symboln[512];

   if(!disp)
       return NULL;


    if(!disp->NewList) {
         snprintf(symboln, sizeof(symboln), "%sNewList", symbol_prefix);
         SET_NewList(disp, dlsym(handle, symboln));
    }


    if(!disp->EndList) {
         snprintf(symboln, sizeof(symboln), "%sEndList", symbol_prefix);
         SET_EndList(disp, dlsym(handle, symboln));
    }


    if(!disp->CallList) {
         snprintf(symboln, sizeof(symboln), "%sCallList", symbol_prefix);
         SET_CallList(disp, dlsym(handle, symboln));
    }


    if(!disp->CallLists) {
         snprintf(symboln, sizeof(symboln), "%sCallLists", symbol_prefix);
         SET_CallLists(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteLists) {
         snprintf(symboln, sizeof(symboln), "%sDeleteLists", symbol_prefix);
         SET_DeleteLists(disp, dlsym(handle, symboln));
    }


    if(!disp->GenLists) {
         snprintf(symboln, sizeof(symboln), "%sGenLists", symbol_prefix);
         SET_GenLists(disp, dlsym(handle, symboln));
    }


    if(!disp->ListBase) {
         snprintf(symboln, sizeof(symboln), "%sListBase", symbol_prefix);
         SET_ListBase(disp, dlsym(handle, symboln));
    }


    if(!disp->Begin) {
         snprintf(symboln, sizeof(symboln), "%sBegin", symbol_prefix);
         SET_Begin(disp, dlsym(handle, symboln));
    }


    if(!disp->Bitmap) {
         snprintf(symboln, sizeof(symboln), "%sBitmap", symbol_prefix);
         SET_Bitmap(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3b) {
         snprintf(symboln, sizeof(symboln), "%sColor3b", symbol_prefix);
         SET_Color3b(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3bv) {
         snprintf(symboln, sizeof(symboln), "%sColor3bv", symbol_prefix);
         SET_Color3bv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3d) {
         snprintf(symboln, sizeof(symboln), "%sColor3d", symbol_prefix);
         SET_Color3d(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3dv) {
         snprintf(symboln, sizeof(symboln), "%sColor3dv", symbol_prefix);
         SET_Color3dv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3f) {
         snprintf(symboln, sizeof(symboln), "%sColor3f", symbol_prefix);
         SET_Color3f(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3fv) {
         snprintf(symboln, sizeof(symboln), "%sColor3fv", symbol_prefix);
         SET_Color3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3i) {
         snprintf(symboln, sizeof(symboln), "%sColor3i", symbol_prefix);
         SET_Color3i(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3iv) {
         snprintf(symboln, sizeof(symboln), "%sColor3iv", symbol_prefix);
         SET_Color3iv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3s) {
         snprintf(symboln, sizeof(symboln), "%sColor3s", symbol_prefix);
         SET_Color3s(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3sv) {
         snprintf(symboln, sizeof(symboln), "%sColor3sv", symbol_prefix);
         SET_Color3sv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3ub) {
         snprintf(symboln, sizeof(symboln), "%sColor3ub", symbol_prefix);
         SET_Color3ub(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3ubv) {
         snprintf(symboln, sizeof(symboln), "%sColor3ubv", symbol_prefix);
         SET_Color3ubv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3ui) {
         snprintf(symboln, sizeof(symboln), "%sColor3ui", symbol_prefix);
         SET_Color3ui(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3uiv) {
         snprintf(symboln, sizeof(symboln), "%sColor3uiv", symbol_prefix);
         SET_Color3uiv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3us) {
         snprintf(symboln, sizeof(symboln), "%sColor3us", symbol_prefix);
         SET_Color3us(disp, dlsym(handle, symboln));
    }


    if(!disp->Color3usv) {
         snprintf(symboln, sizeof(symboln), "%sColor3usv", symbol_prefix);
         SET_Color3usv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4b) {
         snprintf(symboln, sizeof(symboln), "%sColor4b", symbol_prefix);
         SET_Color4b(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4bv) {
         snprintf(symboln, sizeof(symboln), "%sColor4bv", symbol_prefix);
         SET_Color4bv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4d) {
         snprintf(symboln, sizeof(symboln), "%sColor4d", symbol_prefix);
         SET_Color4d(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4dv) {
         snprintf(symboln, sizeof(symboln), "%sColor4dv", symbol_prefix);
         SET_Color4dv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4f) {
         snprintf(symboln, sizeof(symboln), "%sColor4f", symbol_prefix);
         SET_Color4f(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4fv) {
         snprintf(symboln, sizeof(symboln), "%sColor4fv", symbol_prefix);
         SET_Color4fv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4i) {
         snprintf(symboln, sizeof(symboln), "%sColor4i", symbol_prefix);
         SET_Color4i(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4iv) {
         snprintf(symboln, sizeof(symboln), "%sColor4iv", symbol_prefix);
         SET_Color4iv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4s) {
         snprintf(symboln, sizeof(symboln), "%sColor4s", symbol_prefix);
         SET_Color4s(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4sv) {
         snprintf(symboln, sizeof(symboln), "%sColor4sv", symbol_prefix);
         SET_Color4sv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4ub) {
         snprintf(symboln, sizeof(symboln), "%sColor4ub", symbol_prefix);
         SET_Color4ub(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4ubv) {
         snprintf(symboln, sizeof(symboln), "%sColor4ubv", symbol_prefix);
         SET_Color4ubv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4ui) {
         snprintf(symboln, sizeof(symboln), "%sColor4ui", symbol_prefix);
         SET_Color4ui(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4uiv) {
         snprintf(symboln, sizeof(symboln), "%sColor4uiv", symbol_prefix);
         SET_Color4uiv(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4us) {
         snprintf(symboln, sizeof(symboln), "%sColor4us", symbol_prefix);
         SET_Color4us(disp, dlsym(handle, symboln));
    }


    if(!disp->Color4usv) {
         snprintf(symboln, sizeof(symboln), "%sColor4usv", symbol_prefix);
         SET_Color4usv(disp, dlsym(handle, symboln));
    }


    if(!disp->EdgeFlag) {
         snprintf(symboln, sizeof(symboln), "%sEdgeFlag", symbol_prefix);
         SET_EdgeFlag(disp, dlsym(handle, symboln));
    }


    if(!disp->EdgeFlagv) {
         snprintf(symboln, sizeof(symboln), "%sEdgeFlagv", symbol_prefix);
         SET_EdgeFlagv(disp, dlsym(handle, symboln));
    }


    if(!disp->End) {
         snprintf(symboln, sizeof(symboln), "%sEnd", symbol_prefix);
         SET_End(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexd) {
         snprintf(symboln, sizeof(symboln), "%sIndexd", symbol_prefix);
         SET_Indexd(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexdv) {
         snprintf(symboln, sizeof(symboln), "%sIndexdv", symbol_prefix);
         SET_Indexdv(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexf) {
         snprintf(symboln, sizeof(symboln), "%sIndexf", symbol_prefix);
         SET_Indexf(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexfv) {
         snprintf(symboln, sizeof(symboln), "%sIndexfv", symbol_prefix);
         SET_Indexfv(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexi) {
         snprintf(symboln, sizeof(symboln), "%sIndexi", symbol_prefix);
         SET_Indexi(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexiv) {
         snprintf(symboln, sizeof(symboln), "%sIndexiv", symbol_prefix);
         SET_Indexiv(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexs) {
         snprintf(symboln, sizeof(symboln), "%sIndexs", symbol_prefix);
         SET_Indexs(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexsv) {
         snprintf(symboln, sizeof(symboln), "%sIndexsv", symbol_prefix);
         SET_Indexsv(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3b) {
         snprintf(symboln, sizeof(symboln), "%sNormal3b", symbol_prefix);
         SET_Normal3b(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3bv) {
         snprintf(symboln, sizeof(symboln), "%sNormal3bv", symbol_prefix);
         SET_Normal3bv(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3d) {
         snprintf(symboln, sizeof(symboln), "%sNormal3d", symbol_prefix);
         SET_Normal3d(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3dv) {
         snprintf(symboln, sizeof(symboln), "%sNormal3dv", symbol_prefix);
         SET_Normal3dv(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3f) {
         snprintf(symboln, sizeof(symboln), "%sNormal3f", symbol_prefix);
         SET_Normal3f(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3fv) {
         snprintf(symboln, sizeof(symboln), "%sNormal3fv", symbol_prefix);
         SET_Normal3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3i) {
         snprintf(symboln, sizeof(symboln), "%sNormal3i", symbol_prefix);
         SET_Normal3i(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3iv) {
         snprintf(symboln, sizeof(symboln), "%sNormal3iv", symbol_prefix);
         SET_Normal3iv(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3s) {
         snprintf(symboln, sizeof(symboln), "%sNormal3s", symbol_prefix);
         SET_Normal3s(disp, dlsym(handle, symboln));
    }


    if(!disp->Normal3sv) {
         snprintf(symboln, sizeof(symboln), "%sNormal3sv", symbol_prefix);
         SET_Normal3sv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2d) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2d", symbol_prefix);
         SET_RasterPos2d(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2dv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2dv", symbol_prefix);
         SET_RasterPos2dv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2f) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2f", symbol_prefix);
         SET_RasterPos2f(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2fv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2fv", symbol_prefix);
         SET_RasterPos2fv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2i) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2i", symbol_prefix);
         SET_RasterPos2i(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2iv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2iv", symbol_prefix);
         SET_RasterPos2iv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2s) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2s", symbol_prefix);
         SET_RasterPos2s(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos2sv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos2sv", symbol_prefix);
         SET_RasterPos2sv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3d) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3d", symbol_prefix);
         SET_RasterPos3d(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3dv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3dv", symbol_prefix);
         SET_RasterPos3dv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3f) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3f", symbol_prefix);
         SET_RasterPos3f(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3fv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3fv", symbol_prefix);
         SET_RasterPos3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3i) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3i", symbol_prefix);
         SET_RasterPos3i(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3iv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3iv", symbol_prefix);
         SET_RasterPos3iv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3s) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3s", symbol_prefix);
         SET_RasterPos3s(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos3sv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos3sv", symbol_prefix);
         SET_RasterPos3sv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4d) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4d", symbol_prefix);
         SET_RasterPos4d(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4dv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4dv", symbol_prefix);
         SET_RasterPos4dv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4f) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4f", symbol_prefix);
         SET_RasterPos4f(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4fv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4fv", symbol_prefix);
         SET_RasterPos4fv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4i) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4i", symbol_prefix);
         SET_RasterPos4i(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4iv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4iv", symbol_prefix);
         SET_RasterPos4iv(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4s) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4s", symbol_prefix);
         SET_RasterPos4s(disp, dlsym(handle, symboln));
    }


    if(!disp->RasterPos4sv) {
         snprintf(symboln, sizeof(symboln), "%sRasterPos4sv", symbol_prefix);
         SET_RasterPos4sv(disp, dlsym(handle, symboln));
    }


    if(!disp->Rectd) {
         snprintf(symboln, sizeof(symboln), "%sRectd", symbol_prefix);
         SET_Rectd(disp, dlsym(handle, symboln));
    }


    if(!disp->Rectdv) {
         snprintf(symboln, sizeof(symboln), "%sRectdv", symbol_prefix);
         SET_Rectdv(disp, dlsym(handle, symboln));
    }


    if(!disp->Rectf) {
         snprintf(symboln, sizeof(symboln), "%sRectf", symbol_prefix);
         SET_Rectf(disp, dlsym(handle, symboln));
    }


    if(!disp->Rectfv) {
         snprintf(symboln, sizeof(symboln), "%sRectfv", symbol_prefix);
         SET_Rectfv(disp, dlsym(handle, symboln));
    }


    if(!disp->Recti) {
         snprintf(symboln, sizeof(symboln), "%sRecti", symbol_prefix);
         SET_Recti(disp, dlsym(handle, symboln));
    }


    if(!disp->Rectiv) {
         snprintf(symboln, sizeof(symboln), "%sRectiv", symbol_prefix);
         SET_Rectiv(disp, dlsym(handle, symboln));
    }


    if(!disp->Rects) {
         snprintf(symboln, sizeof(symboln), "%sRects", symbol_prefix);
         SET_Rects(disp, dlsym(handle, symboln));
    }


    if(!disp->Rectsv) {
         snprintf(symboln, sizeof(symboln), "%sRectsv", symbol_prefix);
         SET_Rectsv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1d) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1d", symbol_prefix);
         SET_TexCoord1d(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1dv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1dv", symbol_prefix);
         SET_TexCoord1dv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1f) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1f", symbol_prefix);
         SET_TexCoord1f(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1fv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1fv", symbol_prefix);
         SET_TexCoord1fv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1i) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1i", symbol_prefix);
         SET_TexCoord1i(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1iv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1iv", symbol_prefix);
         SET_TexCoord1iv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1s) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1s", symbol_prefix);
         SET_TexCoord1s(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord1sv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord1sv", symbol_prefix);
         SET_TexCoord1sv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2d) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2d", symbol_prefix);
         SET_TexCoord2d(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2dv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2dv", symbol_prefix);
         SET_TexCoord2dv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2f) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2f", symbol_prefix);
         SET_TexCoord2f(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2fv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2fv", symbol_prefix);
         SET_TexCoord2fv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2i) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2i", symbol_prefix);
         SET_TexCoord2i(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2iv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2iv", symbol_prefix);
         SET_TexCoord2iv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2s) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2s", symbol_prefix);
         SET_TexCoord2s(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord2sv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord2sv", symbol_prefix);
         SET_TexCoord2sv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3d) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3d", symbol_prefix);
         SET_TexCoord3d(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3dv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3dv", symbol_prefix);
         SET_TexCoord3dv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3f) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3f", symbol_prefix);
         SET_TexCoord3f(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3fv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3fv", symbol_prefix);
         SET_TexCoord3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3i) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3i", symbol_prefix);
         SET_TexCoord3i(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3iv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3iv", symbol_prefix);
         SET_TexCoord3iv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3s) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3s", symbol_prefix);
         SET_TexCoord3s(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord3sv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord3sv", symbol_prefix);
         SET_TexCoord3sv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4d) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4d", symbol_prefix);
         SET_TexCoord4d(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4dv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4dv", symbol_prefix);
         SET_TexCoord4dv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4f) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4f", symbol_prefix);
         SET_TexCoord4f(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4fv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4fv", symbol_prefix);
         SET_TexCoord4fv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4i) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4i", symbol_prefix);
         SET_TexCoord4i(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4iv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4iv", symbol_prefix);
         SET_TexCoord4iv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4s) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4s", symbol_prefix);
         SET_TexCoord4s(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoord4sv) {
         snprintf(symboln, sizeof(symboln), "%sTexCoord4sv", symbol_prefix);
         SET_TexCoord4sv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2d) {
         snprintf(symboln, sizeof(symboln), "%sVertex2d", symbol_prefix);
         SET_Vertex2d(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2dv) {
         snprintf(symboln, sizeof(symboln), "%sVertex2dv", symbol_prefix);
         SET_Vertex2dv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2f) {
         snprintf(symboln, sizeof(symboln), "%sVertex2f", symbol_prefix);
         SET_Vertex2f(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2fv) {
         snprintf(symboln, sizeof(symboln), "%sVertex2fv", symbol_prefix);
         SET_Vertex2fv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2i) {
         snprintf(symboln, sizeof(symboln), "%sVertex2i", symbol_prefix);
         SET_Vertex2i(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2iv) {
         snprintf(symboln, sizeof(symboln), "%sVertex2iv", symbol_prefix);
         SET_Vertex2iv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2s) {
         snprintf(symboln, sizeof(symboln), "%sVertex2s", symbol_prefix);
         SET_Vertex2s(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex2sv) {
         snprintf(symboln, sizeof(symboln), "%sVertex2sv", symbol_prefix);
         SET_Vertex2sv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3d) {
         snprintf(symboln, sizeof(symboln), "%sVertex3d", symbol_prefix);
         SET_Vertex3d(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3dv) {
         snprintf(symboln, sizeof(symboln), "%sVertex3dv", symbol_prefix);
         SET_Vertex3dv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3f) {
         snprintf(symboln, sizeof(symboln), "%sVertex3f", symbol_prefix);
         SET_Vertex3f(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3fv) {
         snprintf(symboln, sizeof(symboln), "%sVertex3fv", symbol_prefix);
         SET_Vertex3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3i) {
         snprintf(symboln, sizeof(symboln), "%sVertex3i", symbol_prefix);
         SET_Vertex3i(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3iv) {
         snprintf(symboln, sizeof(symboln), "%sVertex3iv", symbol_prefix);
         SET_Vertex3iv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3s) {
         snprintf(symboln, sizeof(symboln), "%sVertex3s", symbol_prefix);
         SET_Vertex3s(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex3sv) {
         snprintf(symboln, sizeof(symboln), "%sVertex3sv", symbol_prefix);
         SET_Vertex3sv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4d) {
         snprintf(symboln, sizeof(symboln), "%sVertex4d", symbol_prefix);
         SET_Vertex4d(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4dv) {
         snprintf(symboln, sizeof(symboln), "%sVertex4dv", symbol_prefix);
         SET_Vertex4dv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4f) {
         snprintf(symboln, sizeof(symboln), "%sVertex4f", symbol_prefix);
         SET_Vertex4f(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4fv) {
         snprintf(symboln, sizeof(symboln), "%sVertex4fv", symbol_prefix);
         SET_Vertex4fv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4i) {
         snprintf(symboln, sizeof(symboln), "%sVertex4i", symbol_prefix);
         SET_Vertex4i(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4iv) {
         snprintf(symboln, sizeof(symboln), "%sVertex4iv", symbol_prefix);
         SET_Vertex4iv(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4s) {
         snprintf(symboln, sizeof(symboln), "%sVertex4s", symbol_prefix);
         SET_Vertex4s(disp, dlsym(handle, symboln));
    }


    if(!disp->Vertex4sv) {
         snprintf(symboln, sizeof(symboln), "%sVertex4sv", symbol_prefix);
         SET_Vertex4sv(disp, dlsym(handle, symboln));
    }


    if(!disp->ClipPlane) {
         snprintf(symboln, sizeof(symboln), "%sClipPlane", symbol_prefix);
         SET_ClipPlane(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorMaterial) {
         snprintf(symboln, sizeof(symboln), "%sColorMaterial", symbol_prefix);
         SET_ColorMaterial(disp, dlsym(handle, symboln));
    }


    if(!disp->CullFace) {
         snprintf(symboln, sizeof(symboln), "%sCullFace", symbol_prefix);
         SET_CullFace(disp, dlsym(handle, symboln));
    }


    if(!disp->Fogf) {
         snprintf(symboln, sizeof(symboln), "%sFogf", symbol_prefix);
         SET_Fogf(disp, dlsym(handle, symboln));
    }


    if(!disp->Fogfv) {
         snprintf(symboln, sizeof(symboln), "%sFogfv", symbol_prefix);
         SET_Fogfv(disp, dlsym(handle, symboln));
    }


    if(!disp->Fogi) {
         snprintf(symboln, sizeof(symboln), "%sFogi", symbol_prefix);
         SET_Fogi(disp, dlsym(handle, symboln));
    }


    if(!disp->Fogiv) {
         snprintf(symboln, sizeof(symboln), "%sFogiv", symbol_prefix);
         SET_Fogiv(disp, dlsym(handle, symboln));
    }


    if(!disp->FrontFace) {
         snprintf(symboln, sizeof(symboln), "%sFrontFace", symbol_prefix);
         SET_FrontFace(disp, dlsym(handle, symboln));
    }


    if(!disp->Hint) {
         snprintf(symboln, sizeof(symboln), "%sHint", symbol_prefix);
         SET_Hint(disp, dlsym(handle, symboln));
    }


    if(!disp->Lightf) {
         snprintf(symboln, sizeof(symboln), "%sLightf", symbol_prefix);
         SET_Lightf(disp, dlsym(handle, symboln));
    }


    if(!disp->Lightfv) {
         snprintf(symboln, sizeof(symboln), "%sLightfv", symbol_prefix);
         SET_Lightfv(disp, dlsym(handle, symboln));
    }


    if(!disp->Lighti) {
         snprintf(symboln, sizeof(symboln), "%sLighti", symbol_prefix);
         SET_Lighti(disp, dlsym(handle, symboln));
    }


    if(!disp->Lightiv) {
         snprintf(symboln, sizeof(symboln), "%sLightiv", symbol_prefix);
         SET_Lightiv(disp, dlsym(handle, symboln));
    }


    if(!disp->LightModelf) {
         snprintf(symboln, sizeof(symboln), "%sLightModelf", symbol_prefix);
         SET_LightModelf(disp, dlsym(handle, symboln));
    }


    if(!disp->LightModelfv) {
         snprintf(symboln, sizeof(symboln), "%sLightModelfv", symbol_prefix);
         SET_LightModelfv(disp, dlsym(handle, symboln));
    }


    if(!disp->LightModeli) {
         snprintf(symboln, sizeof(symboln), "%sLightModeli", symbol_prefix);
         SET_LightModeli(disp, dlsym(handle, symboln));
    }


    if(!disp->LightModeliv) {
         snprintf(symboln, sizeof(symboln), "%sLightModeliv", symbol_prefix);
         SET_LightModeliv(disp, dlsym(handle, symboln));
    }


    if(!disp->LineStipple) {
         snprintf(symboln, sizeof(symboln), "%sLineStipple", symbol_prefix);
         SET_LineStipple(disp, dlsym(handle, symboln));
    }


    if(!disp->LineWidth) {
         snprintf(symboln, sizeof(symboln), "%sLineWidth", symbol_prefix);
         SET_LineWidth(disp, dlsym(handle, symboln));
    }


    if(!disp->Materialf) {
         snprintf(symboln, sizeof(symboln), "%sMaterialf", symbol_prefix);
         SET_Materialf(disp, dlsym(handle, symboln));
    }


    if(!disp->Materialfv) {
         snprintf(symboln, sizeof(symboln), "%sMaterialfv", symbol_prefix);
         SET_Materialfv(disp, dlsym(handle, symboln));
    }


    if(!disp->Materiali) {
         snprintf(symboln, sizeof(symboln), "%sMateriali", symbol_prefix);
         SET_Materiali(disp, dlsym(handle, symboln));
    }


    if(!disp->Materialiv) {
         snprintf(symboln, sizeof(symboln), "%sMaterialiv", symbol_prefix);
         SET_Materialiv(disp, dlsym(handle, symboln));
    }


    if(!disp->PointSize) {
         snprintf(symboln, sizeof(symboln), "%sPointSize", symbol_prefix);
         SET_PointSize(disp, dlsym(handle, symboln));
    }


    if(!disp->PolygonMode) {
         snprintf(symboln, sizeof(symboln), "%sPolygonMode", symbol_prefix);
         SET_PolygonMode(disp, dlsym(handle, symboln));
    }


    if(!disp->PolygonStipple) {
         snprintf(symboln, sizeof(symboln), "%sPolygonStipple", symbol_prefix);
         SET_PolygonStipple(disp, dlsym(handle, symboln));
    }


    if(!disp->Scissor) {
         snprintf(symboln, sizeof(symboln), "%sScissor", symbol_prefix);
         SET_Scissor(disp, dlsym(handle, symboln));
    }


    if(!disp->ShadeModel) {
         snprintf(symboln, sizeof(symboln), "%sShadeModel", symbol_prefix);
         SET_ShadeModel(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameterf) {
         snprintf(symboln, sizeof(symboln), "%sTexParameterf", symbol_prefix);
         SET_TexParameterf(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sTexParameterfv", symbol_prefix);
         SET_TexParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameteri) {
         snprintf(symboln, sizeof(symboln), "%sTexParameteri", symbol_prefix);
         SET_TexParameteri(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sTexParameteriv", symbol_prefix);
         SET_TexParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexImage1D) {
         snprintf(symboln, sizeof(symboln), "%sTexImage1D", symbol_prefix);
         SET_TexImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexImage2D) {
         snprintf(symboln, sizeof(symboln), "%sTexImage2D", symbol_prefix);
         SET_TexImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexEnvf) {
         snprintf(symboln, sizeof(symboln), "%sTexEnvf", symbol_prefix);
         SET_TexEnvf(disp, dlsym(handle, symboln));
    }


    if(!disp->TexEnvfv) {
         snprintf(symboln, sizeof(symboln), "%sTexEnvfv", symbol_prefix);
         SET_TexEnvfv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexEnvi) {
         snprintf(symboln, sizeof(symboln), "%sTexEnvi", symbol_prefix);
         SET_TexEnvi(disp, dlsym(handle, symboln));
    }


    if(!disp->TexEnviv) {
         snprintf(symboln, sizeof(symboln), "%sTexEnviv", symbol_prefix);
         SET_TexEnviv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexGend) {
         snprintf(symboln, sizeof(symboln), "%sTexGend", symbol_prefix);
         SET_TexGend(disp, dlsym(handle, symboln));
    }


    if(!disp->TexGendv) {
         snprintf(symboln, sizeof(symboln), "%sTexGendv", symbol_prefix);
         SET_TexGendv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexGenf) {
         snprintf(symboln, sizeof(symboln), "%sTexGenf", symbol_prefix);
         SET_TexGenf(disp, dlsym(handle, symboln));
    }


    if(!disp->TexGenfv) {
         snprintf(symboln, sizeof(symboln), "%sTexGenfv", symbol_prefix);
         SET_TexGenfv(disp, dlsym(handle, symboln));
    }


    if(!disp->TexGeni) {
         snprintf(symboln, sizeof(symboln), "%sTexGeni", symbol_prefix);
         SET_TexGeni(disp, dlsym(handle, symboln));
    }


    if(!disp->TexGeniv) {
         snprintf(symboln, sizeof(symboln), "%sTexGeniv", symbol_prefix);
         SET_TexGeniv(disp, dlsym(handle, symboln));
    }


    if(!disp->FeedbackBuffer) {
         snprintf(symboln, sizeof(symboln), "%sFeedbackBuffer", symbol_prefix);
         SET_FeedbackBuffer(disp, dlsym(handle, symboln));
    }


    if(!disp->SelectBuffer) {
         snprintf(symboln, sizeof(symboln), "%sSelectBuffer", symbol_prefix);
         SET_SelectBuffer(disp, dlsym(handle, symboln));
    }


    if(!disp->RenderMode) {
         snprintf(symboln, sizeof(symboln), "%sRenderMode", symbol_prefix);
         SET_RenderMode(disp, dlsym(handle, symboln));
    }


    if(!disp->InitNames) {
         snprintf(symboln, sizeof(symboln), "%sInitNames", symbol_prefix);
         SET_InitNames(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadName) {
         snprintf(symboln, sizeof(symboln), "%sLoadName", symbol_prefix);
         SET_LoadName(disp, dlsym(handle, symboln));
    }


    if(!disp->PassThrough) {
         snprintf(symboln, sizeof(symboln), "%sPassThrough", symbol_prefix);
         SET_PassThrough(disp, dlsym(handle, symboln));
    }


    if(!disp->PopName) {
         snprintf(symboln, sizeof(symboln), "%sPopName", symbol_prefix);
         SET_PopName(disp, dlsym(handle, symboln));
    }


    if(!disp->PushName) {
         snprintf(symboln, sizeof(symboln), "%sPushName", symbol_prefix);
         SET_PushName(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawBuffer) {
         snprintf(symboln, sizeof(symboln), "%sDrawBuffer", symbol_prefix);
         SET_DrawBuffer(disp, dlsym(handle, symboln));
    }


    if(!disp->Clear) {
         snprintf(symboln, sizeof(symboln), "%sClear", symbol_prefix);
         SET_Clear(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearAccum) {
         snprintf(symboln, sizeof(symboln), "%sClearAccum", symbol_prefix);
         SET_ClearAccum(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearIndex) {
         snprintf(symboln, sizeof(symboln), "%sClearIndex", symbol_prefix);
         SET_ClearIndex(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearColor) {
         snprintf(symboln, sizeof(symboln), "%sClearColor", symbol_prefix);
         SET_ClearColor(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearStencil) {
         snprintf(symboln, sizeof(symboln), "%sClearStencil", symbol_prefix);
         SET_ClearStencil(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearDepth) {
         snprintf(symboln, sizeof(symboln), "%sClearDepth", symbol_prefix);
         SET_ClearDepth(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilMask) {
         snprintf(symboln, sizeof(symboln), "%sStencilMask", symbol_prefix);
         SET_StencilMask(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorMask) {
         snprintf(symboln, sizeof(symboln), "%sColorMask", symbol_prefix);
         SET_ColorMask(disp, dlsym(handle, symboln));
    }


    if(!disp->DepthMask) {
         snprintf(symboln, sizeof(symboln), "%sDepthMask", symbol_prefix);
         SET_DepthMask(disp, dlsym(handle, symboln));
    }


    if(!disp->IndexMask) {
         snprintf(symboln, sizeof(symboln), "%sIndexMask", symbol_prefix);
         SET_IndexMask(disp, dlsym(handle, symboln));
    }


    if(!disp->Accum) {
         snprintf(symboln, sizeof(symboln), "%sAccum", symbol_prefix);
         SET_Accum(disp, dlsym(handle, symboln));
    }


    if(!disp->Disable) {
         snprintf(symboln, sizeof(symboln), "%sDisable", symbol_prefix);
         SET_Disable(disp, dlsym(handle, symboln));
    }


    if(!disp->Enable) {
         snprintf(symboln, sizeof(symboln), "%sEnable", symbol_prefix);
         SET_Enable(disp, dlsym(handle, symboln));
    }


    if(!disp->Finish) {
         snprintf(symboln, sizeof(symboln), "%sFinish", symbol_prefix);
         SET_Finish(disp, dlsym(handle, symboln));
    }


    if(!disp->Flush) {
         snprintf(symboln, sizeof(symboln), "%sFlush", symbol_prefix);
         SET_Flush(disp, dlsym(handle, symboln));
    }


    if(!disp->PopAttrib) {
         snprintf(symboln, sizeof(symboln), "%sPopAttrib", symbol_prefix);
         SET_PopAttrib(disp, dlsym(handle, symboln));
    }


    if(!disp->PushAttrib) {
         snprintf(symboln, sizeof(symboln), "%sPushAttrib", symbol_prefix);
         SET_PushAttrib(disp, dlsym(handle, symboln));
    }


    if(!disp->Map1d) {
         snprintf(symboln, sizeof(symboln), "%sMap1d", symbol_prefix);
         SET_Map1d(disp, dlsym(handle, symboln));
    }


    if(!disp->Map1f) {
         snprintf(symboln, sizeof(symboln), "%sMap1f", symbol_prefix);
         SET_Map1f(disp, dlsym(handle, symboln));
    }


    if(!disp->Map2d) {
         snprintf(symboln, sizeof(symboln), "%sMap2d", symbol_prefix);
         SET_Map2d(disp, dlsym(handle, symboln));
    }


    if(!disp->Map2f) {
         snprintf(symboln, sizeof(symboln), "%sMap2f", symbol_prefix);
         SET_Map2f(disp, dlsym(handle, symboln));
    }


    if(!disp->MapGrid1d) {
         snprintf(symboln, sizeof(symboln), "%sMapGrid1d", symbol_prefix);
         SET_MapGrid1d(disp, dlsym(handle, symboln));
    }


    if(!disp->MapGrid1f) {
         snprintf(symboln, sizeof(symboln), "%sMapGrid1f", symbol_prefix);
         SET_MapGrid1f(disp, dlsym(handle, symboln));
    }


    if(!disp->MapGrid2d) {
         snprintf(symboln, sizeof(symboln), "%sMapGrid2d", symbol_prefix);
         SET_MapGrid2d(disp, dlsym(handle, symboln));
    }


    if(!disp->MapGrid2f) {
         snprintf(symboln, sizeof(symboln), "%sMapGrid2f", symbol_prefix);
         SET_MapGrid2f(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord1d) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord1d", symbol_prefix);
         SET_EvalCoord1d(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord1dv) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord1dv", symbol_prefix);
         SET_EvalCoord1dv(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord1f) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord1f", symbol_prefix);
         SET_EvalCoord1f(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord1fv) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord1fv", symbol_prefix);
         SET_EvalCoord1fv(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord2d) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord2d", symbol_prefix);
         SET_EvalCoord2d(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord2dv) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord2dv", symbol_prefix);
         SET_EvalCoord2dv(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord2f) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord2f", symbol_prefix);
         SET_EvalCoord2f(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalCoord2fv) {
         snprintf(symboln, sizeof(symboln), "%sEvalCoord2fv", symbol_prefix);
         SET_EvalCoord2fv(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalMesh1) {
         snprintf(symboln, sizeof(symboln), "%sEvalMesh1", symbol_prefix);
         SET_EvalMesh1(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalPoint1) {
         snprintf(symboln, sizeof(symboln), "%sEvalPoint1", symbol_prefix);
         SET_EvalPoint1(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalMesh2) {
         snprintf(symboln, sizeof(symboln), "%sEvalMesh2", symbol_prefix);
         SET_EvalMesh2(disp, dlsym(handle, symboln));
    }


    if(!disp->EvalPoint2) {
         snprintf(symboln, sizeof(symboln), "%sEvalPoint2", symbol_prefix);
         SET_EvalPoint2(disp, dlsym(handle, symboln));
    }


    if(!disp->AlphaFunc) {
         snprintf(symboln, sizeof(symboln), "%sAlphaFunc", symbol_prefix);
         SET_AlphaFunc(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFunc) {
         snprintf(symboln, sizeof(symboln), "%sBlendFunc", symbol_prefix);
         SET_BlendFunc(disp, dlsym(handle, symboln));
    }


    if(!disp->LogicOp) {
         snprintf(symboln, sizeof(symboln), "%sLogicOp", symbol_prefix);
         SET_LogicOp(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilFunc) {
         snprintf(symboln, sizeof(symboln), "%sStencilFunc", symbol_prefix);
         SET_StencilFunc(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilOp) {
         snprintf(symboln, sizeof(symboln), "%sStencilOp", symbol_prefix);
         SET_StencilOp(disp, dlsym(handle, symboln));
    }


    if(!disp->DepthFunc) {
         snprintf(symboln, sizeof(symboln), "%sDepthFunc", symbol_prefix);
         SET_DepthFunc(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelZoom) {
         snprintf(symboln, sizeof(symboln), "%sPixelZoom", symbol_prefix);
         SET_PixelZoom(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTransferf) {
         snprintf(symboln, sizeof(symboln), "%sPixelTransferf", symbol_prefix);
         SET_PixelTransferf(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTransferi) {
         snprintf(symboln, sizeof(symboln), "%sPixelTransferi", symbol_prefix);
         SET_PixelTransferi(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelStoref) {
         snprintf(symboln, sizeof(symboln), "%sPixelStoref", symbol_prefix);
         SET_PixelStoref(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelStorei) {
         snprintf(symboln, sizeof(symboln), "%sPixelStorei", symbol_prefix);
         SET_PixelStorei(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelMapfv) {
         snprintf(symboln, sizeof(symboln), "%sPixelMapfv", symbol_prefix);
         SET_PixelMapfv(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelMapuiv) {
         snprintf(symboln, sizeof(symboln), "%sPixelMapuiv", symbol_prefix);
         SET_PixelMapuiv(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelMapusv) {
         snprintf(symboln, sizeof(symboln), "%sPixelMapusv", symbol_prefix);
         SET_PixelMapusv(disp, dlsym(handle, symboln));
    }


    if(!disp->ReadBuffer) {
         snprintf(symboln, sizeof(symboln), "%sReadBuffer", symbol_prefix);
         SET_ReadBuffer(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyPixels) {
         snprintf(symboln, sizeof(symboln), "%sCopyPixels", symbol_prefix);
         SET_CopyPixels(disp, dlsym(handle, symboln));
    }


    if(!disp->ReadPixels) {
         snprintf(symboln, sizeof(symboln), "%sReadPixels", symbol_prefix);
         SET_ReadPixels(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawPixels) {
         snprintf(symboln, sizeof(symboln), "%sDrawPixels", symbol_prefix);
         SET_DrawPixels(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBooleanv) {
         snprintf(symboln, sizeof(symboln), "%sGetBooleanv", symbol_prefix);
         SET_GetBooleanv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetClipPlane) {
         snprintf(symboln, sizeof(symboln), "%sGetClipPlane", symbol_prefix);
         SET_GetClipPlane(disp, dlsym(handle, symboln));
    }


    if(!disp->GetDoublev) {
         snprintf(symboln, sizeof(symboln), "%sGetDoublev", symbol_prefix);
         SET_GetDoublev(disp, dlsym(handle, symboln));
    }


    if(!disp->GetError) {
         snprintf(symboln, sizeof(symboln), "%sGetError", symbol_prefix);
         SET_GetError(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFloatv) {
         snprintf(symboln, sizeof(symboln), "%sGetFloatv", symbol_prefix);
         SET_GetFloatv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetIntegerv) {
         snprintf(symboln, sizeof(symboln), "%sGetIntegerv", symbol_prefix);
         SET_GetIntegerv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetLightfv) {
         snprintf(symboln, sizeof(symboln), "%sGetLightfv", symbol_prefix);
         SET_GetLightfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetLightiv) {
         snprintf(symboln, sizeof(symboln), "%sGetLightiv", symbol_prefix);
         SET_GetLightiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMapdv) {
         snprintf(symboln, sizeof(symboln), "%sGetMapdv", symbol_prefix);
         SET_GetMapdv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMapfv) {
         snprintf(symboln, sizeof(symboln), "%sGetMapfv", symbol_prefix);
         SET_GetMapfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMapiv) {
         snprintf(symboln, sizeof(symboln), "%sGetMapiv", symbol_prefix);
         SET_GetMapiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMaterialfv) {
         snprintf(symboln, sizeof(symboln), "%sGetMaterialfv", symbol_prefix);
         SET_GetMaterialfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMaterialiv) {
         snprintf(symboln, sizeof(symboln), "%sGetMaterialiv", symbol_prefix);
         SET_GetMaterialiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPixelMapfv) {
         snprintf(symboln, sizeof(symboln), "%sGetPixelMapfv", symbol_prefix);
         SET_GetPixelMapfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPixelMapuiv) {
         snprintf(symboln, sizeof(symboln), "%sGetPixelMapuiv", symbol_prefix);
         SET_GetPixelMapuiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPixelMapusv) {
         snprintf(symboln, sizeof(symboln), "%sGetPixelMapusv", symbol_prefix);
         SET_GetPixelMapusv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPolygonStipple) {
         snprintf(symboln, sizeof(symboln), "%sGetPolygonStipple", symbol_prefix);
         SET_GetPolygonStipple(disp, dlsym(handle, symboln));
    }


    if(!disp->GetString) {
         snprintf(symboln, sizeof(symboln), "%sGetString", symbol_prefix);
         SET_GetString(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexEnvfv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexEnvfv", symbol_prefix);
         SET_GetTexEnvfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexEnviv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexEnviv", symbol_prefix);
         SET_GetTexEnviv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexGendv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexGendv", symbol_prefix);
         SET_GetTexGendv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexGenfv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexGenfv", symbol_prefix);
         SET_GetTexGenfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexGeniv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexGeniv", symbol_prefix);
         SET_GetTexGeniv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexImage) {
         snprintf(symboln, sizeof(symboln), "%sGetTexImage", symbol_prefix);
         SET_GetTexImage(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameterfv", symbol_prefix);
         SET_GetTexParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameteriv", symbol_prefix);
         SET_GetTexParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexLevelParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexLevelParameterfv", symbol_prefix);
         SET_GetTexLevelParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexLevelParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetTexLevelParameteriv", symbol_prefix);
         SET_GetTexLevelParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->IsEnabled) {
         snprintf(symboln, sizeof(symboln), "%sIsEnabled", symbol_prefix);
         SET_IsEnabled(disp, dlsym(handle, symboln));
    }


    if(!disp->IsList) {
         snprintf(symboln, sizeof(symboln), "%sIsList", symbol_prefix);
         SET_IsList(disp, dlsym(handle, symboln));
    }


    if(!disp->DepthRange) {
         snprintf(symboln, sizeof(symboln), "%sDepthRange", symbol_prefix);
         SET_DepthRange(disp, dlsym(handle, symboln));
    }


    if(!disp->Frustum) {
         snprintf(symboln, sizeof(symboln), "%sFrustum", symbol_prefix);
         SET_Frustum(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadIdentity) {
         snprintf(symboln, sizeof(symboln), "%sLoadIdentity", symbol_prefix);
         SET_LoadIdentity(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadMatrixf) {
         snprintf(symboln, sizeof(symboln), "%sLoadMatrixf", symbol_prefix);
         SET_LoadMatrixf(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadMatrixd) {
         snprintf(symboln, sizeof(symboln), "%sLoadMatrixd", symbol_prefix);
         SET_LoadMatrixd(disp, dlsym(handle, symboln));
    }


    if(!disp->MatrixMode) {
         snprintf(symboln, sizeof(symboln), "%sMatrixMode", symbol_prefix);
         SET_MatrixMode(disp, dlsym(handle, symboln));
    }


    if(!disp->MultMatrixf) {
         snprintf(symboln, sizeof(symboln), "%sMultMatrixf", symbol_prefix);
         SET_MultMatrixf(disp, dlsym(handle, symboln));
    }


    if(!disp->MultMatrixd) {
         snprintf(symboln, sizeof(symboln), "%sMultMatrixd", symbol_prefix);
         SET_MultMatrixd(disp, dlsym(handle, symboln));
    }


    if(!disp->Ortho) {
         snprintf(symboln, sizeof(symboln), "%sOrtho", symbol_prefix);
         SET_Ortho(disp, dlsym(handle, symboln));
    }


    if(!disp->PopMatrix) {
         snprintf(symboln, sizeof(symboln), "%sPopMatrix", symbol_prefix);
         SET_PopMatrix(disp, dlsym(handle, symboln));
    }


    if(!disp->PushMatrix) {
         snprintf(symboln, sizeof(symboln), "%sPushMatrix", symbol_prefix);
         SET_PushMatrix(disp, dlsym(handle, symboln));
    }


    if(!disp->Rotated) {
         snprintf(symboln, sizeof(symboln), "%sRotated", symbol_prefix);
         SET_Rotated(disp, dlsym(handle, symboln));
    }


    if(!disp->Rotatef) {
         snprintf(symboln, sizeof(symboln), "%sRotatef", symbol_prefix);
         SET_Rotatef(disp, dlsym(handle, symboln));
    }


    if(!disp->Scaled) {
         snprintf(symboln, sizeof(symboln), "%sScaled", symbol_prefix);
         SET_Scaled(disp, dlsym(handle, symboln));
    }


    if(!disp->Scalef) {
         snprintf(symboln, sizeof(symboln), "%sScalef", symbol_prefix);
         SET_Scalef(disp, dlsym(handle, symboln));
    }


    if(!disp->Translated) {
         snprintf(symboln, sizeof(symboln), "%sTranslated", symbol_prefix);
         SET_Translated(disp, dlsym(handle, symboln));
    }


    if(!disp->Translatef) {
         snprintf(symboln, sizeof(symboln), "%sTranslatef", symbol_prefix);
         SET_Translatef(disp, dlsym(handle, symboln));
    }


    if(!disp->Viewport) {
         snprintf(symboln, sizeof(symboln), "%sViewport", symbol_prefix);
         SET_Viewport(disp, dlsym(handle, symboln));
    }


    if(!disp->ArrayElement) {
         snprintf(symboln, sizeof(symboln), "%sArrayElement", symbol_prefix);
         SET_ArrayElement(disp, dlsym(handle, symboln));
    }


    if(!disp->ArrayElement) {
         snprintf(symboln, sizeof(symboln), "%sArrayElementEXT", symbol_prefix);
         SET_ArrayElement(disp, dlsym(handle, symboln));
    }


    if(!disp->BindTexture) {
         snprintf(symboln, sizeof(symboln), "%sBindTexture", symbol_prefix);
         SET_BindTexture(disp, dlsym(handle, symboln));
    }


    if(!disp->BindTexture) {
         snprintf(symboln, sizeof(symboln), "%sBindTextureEXT", symbol_prefix);
         SET_BindTexture(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorPointer) {
         snprintf(symboln, sizeof(symboln), "%sColorPointer", symbol_prefix);
         SET_ColorPointer(disp, dlsym(handle, symboln));
    }


    if(!disp->DisableClientState) {
         snprintf(symboln, sizeof(symboln), "%sDisableClientState", symbol_prefix);
         SET_DisableClientState(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawArrays) {
         snprintf(symboln, sizeof(symboln), "%sDrawArrays", symbol_prefix);
         SET_DrawArrays(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawArrays) {
         snprintf(symboln, sizeof(symboln), "%sDrawArraysEXT", symbol_prefix);
         SET_DrawArrays(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawElements) {
         snprintf(symboln, sizeof(symboln), "%sDrawElements", symbol_prefix);
         SET_DrawElements(disp, dlsym(handle, symboln));
    }


    if(!disp->EdgeFlagPointer) {
         snprintf(symboln, sizeof(symboln), "%sEdgeFlagPointer", symbol_prefix);
         SET_EdgeFlagPointer(disp, dlsym(handle, symboln));
    }


    if(!disp->EnableClientState) {
         snprintf(symboln, sizeof(symboln), "%sEnableClientState", symbol_prefix);
         SET_EnableClientState(disp, dlsym(handle, symboln));
    }


    if(!disp->IndexPointer) {
         snprintf(symboln, sizeof(symboln), "%sIndexPointer", symbol_prefix);
         SET_IndexPointer(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexub) {
         snprintf(symboln, sizeof(symboln), "%sIndexub", symbol_prefix);
         SET_Indexub(disp, dlsym(handle, symboln));
    }


    if(!disp->Indexubv) {
         snprintf(symboln, sizeof(symboln), "%sIndexubv", symbol_prefix);
         SET_Indexubv(disp, dlsym(handle, symboln));
    }


    if(!disp->InterleavedArrays) {
         snprintf(symboln, sizeof(symboln), "%sInterleavedArrays", symbol_prefix);
         SET_InterleavedArrays(disp, dlsym(handle, symboln));
    }


    if(!disp->NormalPointer) {
         snprintf(symboln, sizeof(symboln), "%sNormalPointer", symbol_prefix);
         SET_NormalPointer(disp, dlsym(handle, symboln));
    }


    if(!disp->PolygonOffset) {
         snprintf(symboln, sizeof(symboln), "%sPolygonOffset", symbol_prefix);
         SET_PolygonOffset(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoordPointer) {
         snprintf(symboln, sizeof(symboln), "%sTexCoordPointer", symbol_prefix);
         SET_TexCoordPointer(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexPointer) {
         snprintf(symboln, sizeof(symboln), "%sVertexPointer", symbol_prefix);
         SET_VertexPointer(disp, dlsym(handle, symboln));
    }


    if(!disp->AreTexturesResident) {
         snprintf(symboln, sizeof(symboln), "%sAreTexturesResident", symbol_prefix);
         SET_AreTexturesResident(disp, dlsym(handle, symboln));
    }


    if(!disp->AreTexturesResident) {
         snprintf(symboln, sizeof(symboln), "%sAreTexturesResidentEXT", symbol_prefix);
         SET_AreTexturesResident(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexImage1D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexImage1D", symbol_prefix);
         SET_CopyTexImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexImage1D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexImage1DEXT", symbol_prefix);
         SET_CopyTexImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexImage2D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexImage2D", symbol_prefix);
         SET_CopyTexImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexImage2D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexImage2DEXT", symbol_prefix);
         SET_CopyTexImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexSubImage1D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage1D", symbol_prefix);
         SET_CopyTexSubImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexSubImage1D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage1DEXT", symbol_prefix);
         SET_CopyTexSubImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexSubImage2D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage2D", symbol_prefix);
         SET_CopyTexSubImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexSubImage2D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage2DEXT", symbol_prefix);
         SET_CopyTexSubImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteTextures) {
         snprintf(symboln, sizeof(symboln), "%sDeleteTextures", symbol_prefix);
         SET_DeleteTextures(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteTextures) {
         snprintf(symboln, sizeof(symboln), "%sDeleteTexturesEXT", symbol_prefix);
         SET_DeleteTextures(disp, dlsym(handle, symboln));
    }


    if(!disp->GenTextures) {
         snprintf(symboln, sizeof(symboln), "%sGenTextures", symbol_prefix);
         SET_GenTextures(disp, dlsym(handle, symboln));
    }


    if(!disp->GenTextures) {
         snprintf(symboln, sizeof(symboln), "%sGenTexturesEXT", symbol_prefix);
         SET_GenTextures(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPointerv) {
         snprintf(symboln, sizeof(symboln), "%sGetPointerv", symbol_prefix);
         SET_GetPointerv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPointerv) {
         snprintf(symboln, sizeof(symboln), "%sGetPointervEXT", symbol_prefix);
         SET_GetPointerv(disp, dlsym(handle, symboln));
    }


    if(!disp->IsTexture) {
         snprintf(symboln, sizeof(symboln), "%sIsTexture", symbol_prefix);
         SET_IsTexture(disp, dlsym(handle, symboln));
    }


    if(!disp->IsTexture) {
         snprintf(symboln, sizeof(symboln), "%sIsTextureEXT", symbol_prefix);
         SET_IsTexture(disp, dlsym(handle, symboln));
    }


    if(!disp->PrioritizeTextures) {
         snprintf(symboln, sizeof(symboln), "%sPrioritizeTextures", symbol_prefix);
         SET_PrioritizeTextures(disp, dlsym(handle, symboln));
    }


    if(!disp->PrioritizeTextures) {
         snprintf(symboln, sizeof(symboln), "%sPrioritizeTexturesEXT", symbol_prefix);
         SET_PrioritizeTextures(disp, dlsym(handle, symboln));
    }


    if(!disp->TexSubImage1D) {
         snprintf(symboln, sizeof(symboln), "%sTexSubImage1D", symbol_prefix);
         SET_TexSubImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexSubImage1D) {
         snprintf(symboln, sizeof(symboln), "%sTexSubImage1DEXT", symbol_prefix);
         SET_TexSubImage1D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexSubImage2D) {
         snprintf(symboln, sizeof(symboln), "%sTexSubImage2D", symbol_prefix);
         SET_TexSubImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexSubImage2D) {
         snprintf(symboln, sizeof(symboln), "%sTexSubImage2DEXT", symbol_prefix);
         SET_TexSubImage2D(disp, dlsym(handle, symboln));
    }


    if(!disp->PopClientAttrib) {
         snprintf(symboln, sizeof(symboln), "%sPopClientAttrib", symbol_prefix);
         SET_PopClientAttrib(disp, dlsym(handle, symboln));
    }


    if(!disp->PushClientAttrib) {
         snprintf(symboln, sizeof(symboln), "%sPushClientAttrib", symbol_prefix);
         SET_PushClientAttrib(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendColor) {
         snprintf(symboln, sizeof(symboln), "%sBlendColor", symbol_prefix);
         SET_BlendColor(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendColor) {
         snprintf(symboln, sizeof(symboln), "%sBlendColorEXT", symbol_prefix);
         SET_BlendColor(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquation) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquation", symbol_prefix);
         SET_BlendEquation(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquation) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationEXT", symbol_prefix);
         SET_BlendEquation(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawRangeElements) {
         snprintf(symboln, sizeof(symboln), "%sDrawRangeElements", symbol_prefix);
         SET_DrawRangeElements(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawRangeElements) {
         snprintf(symboln, sizeof(symboln), "%sDrawRangeElementsEXT", symbol_prefix);
         SET_DrawRangeElements(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTable) {
         snprintf(symboln, sizeof(symboln), "%sColorTable", symbol_prefix);
         SET_ColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTable) {
         snprintf(symboln, sizeof(symboln), "%sColorTableSGI", symbol_prefix);
         SET_ColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTable) {
         snprintf(symboln, sizeof(symboln), "%sColorTableEXT", symbol_prefix);
         SET_ColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTableParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sColorTableParameterfv", symbol_prefix);
         SET_ColorTableParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTableParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sColorTableParameterfvSGI", symbol_prefix);
         SET_ColorTableParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTableParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sColorTableParameteriv", symbol_prefix);
         SET_ColorTableParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorTableParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sColorTableParameterivSGI", symbol_prefix);
         SET_ColorTableParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyColorTable) {
         snprintf(symboln, sizeof(symboln), "%sCopyColorTable", symbol_prefix);
         SET_CopyColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyColorTable) {
         snprintf(symboln, sizeof(symboln), "%sCopyColorTableSGI", symbol_prefix);
         SET_CopyColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTable) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTable", symbol_prefix);
         SET_GetColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTable) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableSGI", symbol_prefix);
         SET_GetColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTable) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableEXT", symbol_prefix);
         SET_GetColorTable(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTableParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfv", symbol_prefix);
         SET_GetColorTableParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTableParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfvSGI", symbol_prefix);
         SET_GetColorTableParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTableParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfvEXT", symbol_prefix);
         SET_GetColorTableParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTableParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableParameteriv", symbol_prefix);
         SET_GetColorTableParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTableParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterivSGI", symbol_prefix);
         SET_GetColorTableParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetColorTableParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterivEXT", symbol_prefix);
         SET_GetColorTableParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorSubTable) {
         snprintf(symboln, sizeof(symboln), "%sColorSubTable", symbol_prefix);
         SET_ColorSubTable(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorSubTable) {
         snprintf(symboln, sizeof(symboln), "%sColorSubTableEXT", symbol_prefix);
         SET_ColorSubTable(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyColorSubTable) {
         snprintf(symboln, sizeof(symboln), "%sCopyColorSubTable", symbol_prefix);
         SET_CopyColorSubTable(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyColorSubTable) {
         snprintf(symboln, sizeof(symboln), "%sCopyColorSubTableEXT", symbol_prefix);
         SET_CopyColorSubTable(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionFilter1D) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionFilter1D", symbol_prefix);
         SET_ConvolutionFilter1D(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionFilter1D) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionFilter1DEXT", symbol_prefix);
         SET_ConvolutionFilter1D(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionFilter2D) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionFilter2D", symbol_prefix);
         SET_ConvolutionFilter2D(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionFilter2D) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionFilter2DEXT", symbol_prefix);
         SET_ConvolutionFilter2D(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameterf) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameterf", symbol_prefix);
         SET_ConvolutionParameterf(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameterf) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfEXT", symbol_prefix);
         SET_ConvolutionParameterf(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfv", symbol_prefix);
         SET_ConvolutionParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfvEXT", symbol_prefix);
         SET_ConvolutionParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameteri) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameteri", symbol_prefix);
         SET_ConvolutionParameteri(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameteri) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameteriEXT", symbol_prefix);
         SET_ConvolutionParameteri(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameteriv", symbol_prefix);
         SET_ConvolutionParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->ConvolutionParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sConvolutionParameterivEXT", symbol_prefix);
         SET_ConvolutionParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyConvolutionFilter1D) {
         snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter1D", symbol_prefix);
         SET_CopyConvolutionFilter1D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyConvolutionFilter1D) {
         snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter1DEXT", symbol_prefix);
         SET_CopyConvolutionFilter1D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyConvolutionFilter2D) {
         snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter2D", symbol_prefix);
         SET_CopyConvolutionFilter2D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyConvolutionFilter2D) {
         snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter2DEXT", symbol_prefix);
         SET_CopyConvolutionFilter2D(disp, dlsym(handle, symboln));
    }


    if(!disp->GetConvolutionFilter) {
         snprintf(symboln, sizeof(symboln), "%sGetConvolutionFilter", symbol_prefix);
         SET_GetConvolutionFilter(disp, dlsym(handle, symboln));
    }


    if(!disp->GetConvolutionFilter) {
         snprintf(symboln, sizeof(symboln), "%sGetConvolutionFilterEXT", symbol_prefix);
         SET_GetConvolutionFilter(disp, dlsym(handle, symboln));
    }


    if(!disp->GetConvolutionParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterfv", symbol_prefix);
         SET_GetConvolutionParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetConvolutionParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterfvEXT", symbol_prefix);
         SET_GetConvolutionParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetConvolutionParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameteriv", symbol_prefix);
         SET_GetConvolutionParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetConvolutionParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterivEXT", symbol_prefix);
         SET_GetConvolutionParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSeparableFilter) {
         snprintf(symboln, sizeof(symboln), "%sGetSeparableFilter", symbol_prefix);
         SET_GetSeparableFilter(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSeparableFilter) {
         snprintf(symboln, sizeof(symboln), "%sGetSeparableFilterEXT", symbol_prefix);
         SET_GetSeparableFilter(disp, dlsym(handle, symboln));
    }


    if(!disp->SeparableFilter2D) {
         snprintf(symboln, sizeof(symboln), "%sSeparableFilter2D", symbol_prefix);
         SET_SeparableFilter2D(disp, dlsym(handle, symboln));
    }


    if(!disp->SeparableFilter2D) {
         snprintf(symboln, sizeof(symboln), "%sSeparableFilter2DEXT", symbol_prefix);
         SET_SeparableFilter2D(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHistogram) {
         snprintf(symboln, sizeof(symboln), "%sGetHistogram", symbol_prefix);
         SET_GetHistogram(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHistogram) {
         snprintf(symboln, sizeof(symboln), "%sGetHistogramEXT", symbol_prefix);
         SET_GetHistogram(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHistogramParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterfv", symbol_prefix);
         SET_GetHistogramParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHistogramParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterfvEXT", symbol_prefix);
         SET_GetHistogramParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHistogramParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetHistogramParameteriv", symbol_prefix);
         SET_GetHistogramParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHistogramParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterivEXT", symbol_prefix);
         SET_GetHistogramParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMinmax) {
         snprintf(symboln, sizeof(symboln), "%sGetMinmax", symbol_prefix);
         SET_GetMinmax(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMinmax) {
         snprintf(symboln, sizeof(symboln), "%sGetMinmaxEXT", symbol_prefix);
         SET_GetMinmax(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMinmaxParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterfv", symbol_prefix);
         SET_GetMinmaxParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMinmaxParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterfvEXT", symbol_prefix);
         SET_GetMinmaxParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMinmaxParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameteriv", symbol_prefix);
         SET_GetMinmaxParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetMinmaxParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterivEXT", symbol_prefix);
         SET_GetMinmaxParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->Histogram) {
         snprintf(symboln, sizeof(symboln), "%sHistogram", symbol_prefix);
         SET_Histogram(disp, dlsym(handle, symboln));
    }


    if(!disp->Histogram) {
         snprintf(symboln, sizeof(symboln), "%sHistogramEXT", symbol_prefix);
         SET_Histogram(disp, dlsym(handle, symboln));
    }


    if(!disp->Minmax) {
         snprintf(symboln, sizeof(symboln), "%sMinmax", symbol_prefix);
         SET_Minmax(disp, dlsym(handle, symboln));
    }


    if(!disp->Minmax) {
         snprintf(symboln, sizeof(symboln), "%sMinmaxEXT", symbol_prefix);
         SET_Minmax(disp, dlsym(handle, symboln));
    }


    if(!disp->ResetHistogram) {
         snprintf(symboln, sizeof(symboln), "%sResetHistogram", symbol_prefix);
         SET_ResetHistogram(disp, dlsym(handle, symboln));
    }


    if(!disp->ResetHistogram) {
         snprintf(symboln, sizeof(symboln), "%sResetHistogramEXT", symbol_prefix);
         SET_ResetHistogram(disp, dlsym(handle, symboln));
    }


    if(!disp->ResetMinmax) {
         snprintf(symboln, sizeof(symboln), "%sResetMinmax", symbol_prefix);
         SET_ResetMinmax(disp, dlsym(handle, symboln));
    }


    if(!disp->ResetMinmax) {
         snprintf(symboln, sizeof(symboln), "%sResetMinmaxEXT", symbol_prefix);
         SET_ResetMinmax(disp, dlsym(handle, symboln));
    }


    if(!disp->TexImage3D) {
         snprintf(symboln, sizeof(symboln), "%sTexImage3D", symbol_prefix);
         SET_TexImage3D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexImage3D) {
         snprintf(symboln, sizeof(symboln), "%sTexImage3DEXT", symbol_prefix);
         SET_TexImage3D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexSubImage3D) {
         snprintf(symboln, sizeof(symboln), "%sTexSubImage3D", symbol_prefix);
         SET_TexSubImage3D(disp, dlsym(handle, symboln));
    }


    if(!disp->TexSubImage3D) {
         snprintf(symboln, sizeof(symboln), "%sTexSubImage3DEXT", symbol_prefix);
         SET_TexSubImage3D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexSubImage3D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage3D", symbol_prefix);
         SET_CopyTexSubImage3D(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyTexSubImage3D) {
         snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage3DEXT", symbol_prefix);
         SET_CopyTexSubImage3D(disp, dlsym(handle, symboln));
    }


    if(!disp->ActiveTextureARB) {
         snprintf(symboln, sizeof(symboln), "%sActiveTexture", symbol_prefix);
         SET_ActiveTextureARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ActiveTextureARB) {
         snprintf(symboln, sizeof(symboln), "%sActiveTextureARB", symbol_prefix);
         SET_ActiveTextureARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ClientActiveTextureARB) {
         snprintf(symboln, sizeof(symboln), "%sClientActiveTexture", symbol_prefix);
         SET_ClientActiveTextureARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ClientActiveTextureARB) {
         snprintf(symboln, sizeof(symboln), "%sClientActiveTextureARB", symbol_prefix);
         SET_ClientActiveTextureARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1d", symbol_prefix);
         SET_MultiTexCoord1dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dARB", symbol_prefix);
         SET_MultiTexCoord1dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dv", symbol_prefix);
         SET_MultiTexCoord1dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dvARB", symbol_prefix);
         SET_MultiTexCoord1dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1f", symbol_prefix);
         SET_MultiTexCoord1fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fARB", symbol_prefix);
         SET_MultiTexCoord1fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fv", symbol_prefix);
         SET_MultiTexCoord1fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fvARB", symbol_prefix);
         SET_MultiTexCoord1fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1i", symbol_prefix);
         SET_MultiTexCoord1iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1iARB", symbol_prefix);
         SET_MultiTexCoord1iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1iv", symbol_prefix);
         SET_MultiTexCoord1ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1ivARB", symbol_prefix);
         SET_MultiTexCoord1ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1s", symbol_prefix);
         SET_MultiTexCoord1sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1sARB", symbol_prefix);
         SET_MultiTexCoord1sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1sv", symbol_prefix);
         SET_MultiTexCoord1svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord1svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1svARB", symbol_prefix);
         SET_MultiTexCoord1svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2d", symbol_prefix);
         SET_MultiTexCoord2dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dARB", symbol_prefix);
         SET_MultiTexCoord2dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dv", symbol_prefix);
         SET_MultiTexCoord2dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dvARB", symbol_prefix);
         SET_MultiTexCoord2dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2f", symbol_prefix);
         SET_MultiTexCoord2fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fARB", symbol_prefix);
         SET_MultiTexCoord2fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fv", symbol_prefix);
         SET_MultiTexCoord2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fvARB", symbol_prefix);
         SET_MultiTexCoord2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2i", symbol_prefix);
         SET_MultiTexCoord2iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2iARB", symbol_prefix);
         SET_MultiTexCoord2iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2iv", symbol_prefix);
         SET_MultiTexCoord2ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2ivARB", symbol_prefix);
         SET_MultiTexCoord2ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2s", symbol_prefix);
         SET_MultiTexCoord2sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2sARB", symbol_prefix);
         SET_MultiTexCoord2sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2sv", symbol_prefix);
         SET_MultiTexCoord2svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord2svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2svARB", symbol_prefix);
         SET_MultiTexCoord2svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3d", symbol_prefix);
         SET_MultiTexCoord3dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dARB", symbol_prefix);
         SET_MultiTexCoord3dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dv", symbol_prefix);
         SET_MultiTexCoord3dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dvARB", symbol_prefix);
         SET_MultiTexCoord3dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3f", symbol_prefix);
         SET_MultiTexCoord3fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fARB", symbol_prefix);
         SET_MultiTexCoord3fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fv", symbol_prefix);
         SET_MultiTexCoord3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fvARB", symbol_prefix);
         SET_MultiTexCoord3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3i", symbol_prefix);
         SET_MultiTexCoord3iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3iARB", symbol_prefix);
         SET_MultiTexCoord3iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3iv", symbol_prefix);
         SET_MultiTexCoord3ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3ivARB", symbol_prefix);
         SET_MultiTexCoord3ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3s", symbol_prefix);
         SET_MultiTexCoord3sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3sARB", symbol_prefix);
         SET_MultiTexCoord3sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3sv", symbol_prefix);
         SET_MultiTexCoord3svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord3svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3svARB", symbol_prefix);
         SET_MultiTexCoord3svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4d", symbol_prefix);
         SET_MultiTexCoord4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4dARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dARB", symbol_prefix);
         SET_MultiTexCoord4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dv", symbol_prefix);
         SET_MultiTexCoord4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dvARB", symbol_prefix);
         SET_MultiTexCoord4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4f", symbol_prefix);
         SET_MultiTexCoord4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4fARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fARB", symbol_prefix);
         SET_MultiTexCoord4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fv", symbol_prefix);
         SET_MultiTexCoord4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fvARB", symbol_prefix);
         SET_MultiTexCoord4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4i", symbol_prefix);
         SET_MultiTexCoord4iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4iARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4iARB", symbol_prefix);
         SET_MultiTexCoord4iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4iv", symbol_prefix);
         SET_MultiTexCoord4ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4ivARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4ivARB", symbol_prefix);
         SET_MultiTexCoord4ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4s", symbol_prefix);
         SET_MultiTexCoord4sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4sARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4sARB", symbol_prefix);
         SET_MultiTexCoord4sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4sv", symbol_prefix);
         SET_MultiTexCoord4svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiTexCoord4svARB) {
         snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4svARB", symbol_prefix);
         SET_MultiTexCoord4svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->AttachShader) {
         snprintf(symboln, sizeof(symboln), "%sAttachShader", symbol_prefix);
         SET_AttachShader(disp, dlsym(handle, symboln));
    }


    if(!disp->CreateProgram) {
         snprintf(symboln, sizeof(symboln), "%sCreateProgram", symbol_prefix);
         SET_CreateProgram(disp, dlsym(handle, symboln));
    }


    if(!disp->CreateShader) {
         snprintf(symboln, sizeof(symboln), "%sCreateShader", symbol_prefix);
         SET_CreateShader(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteProgram) {
         snprintf(symboln, sizeof(symboln), "%sDeleteProgram", symbol_prefix);
         SET_DeleteProgram(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteShader) {
         snprintf(symboln, sizeof(symboln), "%sDeleteShader", symbol_prefix);
         SET_DeleteShader(disp, dlsym(handle, symboln));
    }


    if(!disp->DetachShader) {
         snprintf(symboln, sizeof(symboln), "%sDetachShader", symbol_prefix);
         SET_DetachShader(disp, dlsym(handle, symboln));
    }


    if(!disp->GetAttachedShaders) {
         snprintf(symboln, sizeof(symboln), "%sGetAttachedShaders", symbol_prefix);
         SET_GetAttachedShaders(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramInfoLog) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramInfoLog", symbol_prefix);
         SET_GetProgramInfoLog(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramiv) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramiv", symbol_prefix);
         SET_GetProgramiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetShaderInfoLog) {
         snprintf(symboln, sizeof(symboln), "%sGetShaderInfoLog", symbol_prefix);
         SET_GetShaderInfoLog(disp, dlsym(handle, symboln));
    }


    if(!disp->GetShaderiv) {
         snprintf(symboln, sizeof(symboln), "%sGetShaderiv", symbol_prefix);
         SET_GetShaderiv(disp, dlsym(handle, symboln));
    }


    if(!disp->IsProgram) {
         snprintf(symboln, sizeof(symboln), "%sIsProgram", symbol_prefix);
         SET_IsProgram(disp, dlsym(handle, symboln));
    }


    if(!disp->IsShader) {
         snprintf(symboln, sizeof(symboln), "%sIsShader", symbol_prefix);
         SET_IsShader(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilFuncSeparate) {
         snprintf(symboln, sizeof(symboln), "%sStencilFuncSeparate", symbol_prefix);
         SET_StencilFuncSeparate(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilMaskSeparate) {
         snprintf(symboln, sizeof(symboln), "%sStencilMaskSeparate", symbol_prefix);
         SET_StencilMaskSeparate(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilOpSeparate) {
         snprintf(symboln, sizeof(symboln), "%sStencilOpSeparate", symbol_prefix);
         SET_StencilOpSeparate(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilOpSeparate) {
         snprintf(symboln, sizeof(symboln), "%sStencilOpSeparateATI", symbol_prefix);
         SET_StencilOpSeparate(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix2x3fv) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix2x3fv", symbol_prefix);
         SET_UniformMatrix2x3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix2x4fv) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix2x4fv", symbol_prefix);
         SET_UniformMatrix2x4fv(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix3x2fv) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix3x2fv", symbol_prefix);
         SET_UniformMatrix3x2fv(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix3x4fv) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix3x4fv", symbol_prefix);
         SET_UniformMatrix3x4fv(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix4x2fv) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix4x2fv", symbol_prefix);
         SET_UniformMatrix4x2fv(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix4x3fv) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix4x3fv", symbol_prefix);
         SET_UniformMatrix4x3fv(disp, dlsym(handle, symboln));
    }


    if(!disp->ClampColor) {
         snprintf(symboln, sizeof(symboln), "%sClampColor", symbol_prefix);
         SET_ClampColor(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearBufferfi) {
         snprintf(symboln, sizeof(symboln), "%sClearBufferfi", symbol_prefix);
         SET_ClearBufferfi(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearBufferfv) {
         snprintf(symboln, sizeof(symboln), "%sClearBufferfv", symbol_prefix);
         SET_ClearBufferfv(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearBufferiv) {
         snprintf(symboln, sizeof(symboln), "%sClearBufferiv", symbol_prefix);
         SET_ClearBufferiv(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearBufferuiv) {
         snprintf(symboln, sizeof(symboln), "%sClearBufferuiv", symbol_prefix);
         SET_ClearBufferuiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetStringi) {
         snprintf(symboln, sizeof(symboln), "%sGetStringi", symbol_prefix);
         SET_GetStringi(disp, dlsym(handle, symboln));
    }


    if(!disp->TexBuffer) {
         snprintf(symboln, sizeof(symboln), "%sTexBuffer", symbol_prefix);
         SET_TexBuffer(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture", symbol_prefix);
         SET_FramebufferTexture(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferParameteri64v) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferParameteri64v", symbol_prefix);
         SET_GetBufferParameteri64v(disp, dlsym(handle, symboln));
    }


    if(!disp->GetInteger64i_v) {
         snprintf(symboln, sizeof(symboln), "%sGetInteger64i_v", symbol_prefix);
         SET_GetInteger64i_v(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribDivisor) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribDivisor", symbol_prefix);
         SET_VertexAttribDivisor(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadTransposeMatrixdARB) {
         snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixd", symbol_prefix);
         SET_LoadTransposeMatrixdARB(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadTransposeMatrixdARB) {
         snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixdARB", symbol_prefix);
         SET_LoadTransposeMatrixdARB(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadTransposeMatrixfARB) {
         snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixf", symbol_prefix);
         SET_LoadTransposeMatrixfARB(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadTransposeMatrixfARB) {
         snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixfARB", symbol_prefix);
         SET_LoadTransposeMatrixfARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultTransposeMatrixdARB) {
         snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixd", symbol_prefix);
         SET_MultTransposeMatrixdARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultTransposeMatrixdARB) {
         snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixdARB", symbol_prefix);
         SET_MultTransposeMatrixdARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultTransposeMatrixfARB) {
         snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixf", symbol_prefix);
         SET_MultTransposeMatrixfARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MultTransposeMatrixfARB) {
         snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixfARB", symbol_prefix);
         SET_MultTransposeMatrixfARB(disp, dlsym(handle, symboln));
    }


    if(!disp->SampleCoverageARB) {
         snprintf(symboln, sizeof(symboln), "%sSampleCoverage", symbol_prefix);
         SET_SampleCoverageARB(disp, dlsym(handle, symboln));
    }


    if(!disp->SampleCoverageARB) {
         snprintf(symboln, sizeof(symboln), "%sSampleCoverageARB", symbol_prefix);
         SET_SampleCoverageARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexImage1DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexImage1D", symbol_prefix);
         SET_CompressedTexImage1DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexImage1DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexImage1DARB", symbol_prefix);
         SET_CompressedTexImage1DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexImage2DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexImage2D", symbol_prefix);
         SET_CompressedTexImage2DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexImage2DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexImage2DARB", symbol_prefix);
         SET_CompressedTexImage2DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexImage3DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexImage3D", symbol_prefix);
         SET_CompressedTexImage3DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexImage3DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexImage3DARB", symbol_prefix);
         SET_CompressedTexImage3DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexSubImage1DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage1D", symbol_prefix);
         SET_CompressedTexSubImage1DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexSubImage1DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage1DARB", symbol_prefix);
         SET_CompressedTexSubImage1DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexSubImage2DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage2D", symbol_prefix);
         SET_CompressedTexSubImage2DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexSubImage2DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage2DARB", symbol_prefix);
         SET_CompressedTexSubImage2DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexSubImage3DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage3D", symbol_prefix);
         SET_CompressedTexSubImage3DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompressedTexSubImage3DARB) {
         snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage3DARB", symbol_prefix);
         SET_CompressedTexSubImage3DARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetCompressedTexImageARB) {
         snprintf(symboln, sizeof(symboln), "%sGetCompressedTexImage", symbol_prefix);
         SET_GetCompressedTexImageARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetCompressedTexImageARB) {
         snprintf(symboln, sizeof(symboln), "%sGetCompressedTexImageARB", symbol_prefix);
         SET_GetCompressedTexImageARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DisableVertexAttribArrayARB) {
         snprintf(symboln, sizeof(symboln), "%sDisableVertexAttribArray", symbol_prefix);
         SET_DisableVertexAttribArrayARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DisableVertexAttribArrayARB) {
         snprintf(symboln, sizeof(symboln), "%sDisableVertexAttribArrayARB", symbol_prefix);
         SET_DisableVertexAttribArrayARB(disp, dlsym(handle, symboln));
    }


    if(!disp->EnableVertexAttribArrayARB) {
         snprintf(symboln, sizeof(symboln), "%sEnableVertexAttribArray", symbol_prefix);
         SET_EnableVertexAttribArrayARB(disp, dlsym(handle, symboln));
    }


    if(!disp->EnableVertexAttribArrayARB) {
         snprintf(symboln, sizeof(symboln), "%sEnableVertexAttribArrayARB", symbol_prefix);
         SET_EnableVertexAttribArrayARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramEnvParameterdvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramEnvParameterdvARB", symbol_prefix);
         SET_GetProgramEnvParameterdvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramEnvParameterfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramEnvParameterfvARB", symbol_prefix);
         SET_GetProgramEnvParameterfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramLocalParameterdvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramLocalParameterdvARB", symbol_prefix);
         SET_GetProgramLocalParameterdvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramLocalParameterfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramLocalParameterfvARB", symbol_prefix);
         SET_GetProgramLocalParameterfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramStringARB) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramStringARB", symbol_prefix);
         SET_GetProgramStringARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramivARB", symbol_prefix);
         SET_GetProgramivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribdvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdv", symbol_prefix);
         SET_GetVertexAttribdvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribdvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdvARB", symbol_prefix);
         SET_GetVertexAttribdvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfv", symbol_prefix);
         SET_GetVertexAttribfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfvARB", symbol_prefix);
         SET_GetVertexAttribfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribiv", symbol_prefix);
         SET_GetVertexAttribivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribivARB", symbol_prefix);
         SET_GetVertexAttribivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4dARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4dARB", symbol_prefix);
         SET_ProgramEnvParameter4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4dARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameter4dNV", symbol_prefix);
         SET_ProgramEnvParameter4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4dvARB", symbol_prefix);
         SET_ProgramEnvParameter4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameter4dvNV", symbol_prefix);
         SET_ProgramEnvParameter4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4fARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4fARB", symbol_prefix);
         SET_ProgramEnvParameter4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4fARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameter4fNV", symbol_prefix);
         SET_ProgramEnvParameter4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4fvARB", symbol_prefix);
         SET_ProgramEnvParameter4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameter4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameter4fvNV", symbol_prefix);
         SET_ProgramEnvParameter4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramLocalParameter4dARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4dARB", symbol_prefix);
         SET_ProgramLocalParameter4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramLocalParameter4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4dvARB", symbol_prefix);
         SET_ProgramLocalParameter4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramLocalParameter4fARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4fARB", symbol_prefix);
         SET_ProgramLocalParameter4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramLocalParameter4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4fvARB", symbol_prefix);
         SET_ProgramLocalParameter4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramStringARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramStringARB", symbol_prefix);
         SET_ProgramStringARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1d", symbol_prefix);
         SET_VertexAttrib1dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dARB", symbol_prefix);
         SET_VertexAttrib1dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dv", symbol_prefix);
         SET_VertexAttrib1dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dvARB", symbol_prefix);
         SET_VertexAttrib1dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1f", symbol_prefix);
         SET_VertexAttrib1fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fARB", symbol_prefix);
         SET_VertexAttrib1fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fv", symbol_prefix);
         SET_VertexAttrib1fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fvARB", symbol_prefix);
         SET_VertexAttrib1fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1s", symbol_prefix);
         SET_VertexAttrib1sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sARB", symbol_prefix);
         SET_VertexAttrib1sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sv", symbol_prefix);
         SET_VertexAttrib1svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1svARB", symbol_prefix);
         SET_VertexAttrib1svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2d", symbol_prefix);
         SET_VertexAttrib2dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dARB", symbol_prefix);
         SET_VertexAttrib2dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dv", symbol_prefix);
         SET_VertexAttrib2dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dvARB", symbol_prefix);
         SET_VertexAttrib2dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2f", symbol_prefix);
         SET_VertexAttrib2fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fARB", symbol_prefix);
         SET_VertexAttrib2fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fv", symbol_prefix);
         SET_VertexAttrib2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fvARB", symbol_prefix);
         SET_VertexAttrib2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2s", symbol_prefix);
         SET_VertexAttrib2sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sARB", symbol_prefix);
         SET_VertexAttrib2sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sv", symbol_prefix);
         SET_VertexAttrib2svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2svARB", symbol_prefix);
         SET_VertexAttrib2svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3d", symbol_prefix);
         SET_VertexAttrib3dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dARB", symbol_prefix);
         SET_VertexAttrib3dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dv", symbol_prefix);
         SET_VertexAttrib3dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dvARB", symbol_prefix);
         SET_VertexAttrib3dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3f", symbol_prefix);
         SET_VertexAttrib3fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fARB", symbol_prefix);
         SET_VertexAttrib3fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fv", symbol_prefix);
         SET_VertexAttrib3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fvARB", symbol_prefix);
         SET_VertexAttrib3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3s", symbol_prefix);
         SET_VertexAttrib3sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sARB", symbol_prefix);
         SET_VertexAttrib3sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sv", symbol_prefix);
         SET_VertexAttrib3svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3svARB", symbol_prefix);
         SET_VertexAttrib3svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NbvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nbv", symbol_prefix);
         SET_VertexAttrib4NbvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NbvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NbvARB", symbol_prefix);
         SET_VertexAttrib4NbvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Niv", symbol_prefix);
         SET_VertexAttrib4NivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NivARB", symbol_prefix);
         SET_VertexAttrib4NivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NsvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nsv", symbol_prefix);
         SET_VertexAttrib4NsvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NsvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NsvARB", symbol_prefix);
         SET_VertexAttrib4NsvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NubARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nub", symbol_prefix);
         SET_VertexAttrib4NubARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NubARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NubARB", symbol_prefix);
         SET_VertexAttrib4NubARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NubvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nubv", symbol_prefix);
         SET_VertexAttrib4NubvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NubvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NubvARB", symbol_prefix);
         SET_VertexAttrib4NubvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NuivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nuiv", symbol_prefix);
         SET_VertexAttrib4NuivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NuivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NuivARB", symbol_prefix);
         SET_VertexAttrib4NuivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NusvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nusv", symbol_prefix);
         SET_VertexAttrib4NusvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4NusvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NusvARB", symbol_prefix);
         SET_VertexAttrib4NusvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4bvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4bv", symbol_prefix);
         SET_VertexAttrib4bvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4bvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4bvARB", symbol_prefix);
         SET_VertexAttrib4bvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4d", symbol_prefix);
         SET_VertexAttrib4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4dARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dARB", symbol_prefix);
         SET_VertexAttrib4dARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dv", symbol_prefix);
         SET_VertexAttrib4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4dvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dvARB", symbol_prefix);
         SET_VertexAttrib4dvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4f", symbol_prefix);
         SET_VertexAttrib4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4fARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fARB", symbol_prefix);
         SET_VertexAttrib4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fv", symbol_prefix);
         SET_VertexAttrib4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fvARB", symbol_prefix);
         SET_VertexAttrib4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4ivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4iv", symbol_prefix);
         SET_VertexAttrib4ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4ivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ivARB", symbol_prefix);
         SET_VertexAttrib4ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4s", symbol_prefix);
         SET_VertexAttrib4sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4sARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sARB", symbol_prefix);
         SET_VertexAttrib4sARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sv", symbol_prefix);
         SET_VertexAttrib4svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4svARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4svARB", symbol_prefix);
         SET_VertexAttrib4svARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4ubvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubv", symbol_prefix);
         SET_VertexAttrib4ubvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4ubvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubvARB", symbol_prefix);
         SET_VertexAttrib4ubvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4uivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4uiv", symbol_prefix);
         SET_VertexAttrib4uivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4uivARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4uivARB", symbol_prefix);
         SET_VertexAttrib4uivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4usvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4usv", symbol_prefix);
         SET_VertexAttrib4usvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4usvARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4usvARB", symbol_prefix);
         SET_VertexAttrib4usvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribPointerARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribPointer", symbol_prefix);
         SET_VertexAttribPointerARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribPointerARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribPointerARB", symbol_prefix);
         SET_VertexAttribPointerARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sBindBuffer", symbol_prefix);
         SET_BindBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sBindBufferARB", symbol_prefix);
         SET_BindBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BufferDataARB) {
         snprintf(symboln, sizeof(symboln), "%sBufferData", symbol_prefix);
         SET_BufferDataARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BufferDataARB) {
         snprintf(symboln, sizeof(symboln), "%sBufferDataARB", symbol_prefix);
         SET_BufferDataARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BufferSubDataARB) {
         snprintf(symboln, sizeof(symboln), "%sBufferSubData", symbol_prefix);
         SET_BufferSubDataARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BufferSubDataARB) {
         snprintf(symboln, sizeof(symboln), "%sBufferSubDataARB", symbol_prefix);
         SET_BufferSubDataARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sDeleteBuffers", symbol_prefix);
         SET_DeleteBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sDeleteBuffersARB", symbol_prefix);
         SET_DeleteBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GenBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sGenBuffers", symbol_prefix);
         SET_GenBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GenBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sGenBuffersARB", symbol_prefix);
         SET_GenBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferParameterivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferParameteriv", symbol_prefix);
         SET_GetBufferParameterivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferParameterivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferParameterivARB", symbol_prefix);
         SET_GetBufferParameterivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferPointervARB) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferPointerv", symbol_prefix);
         SET_GetBufferPointervARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferPointervARB) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferPointervARB", symbol_prefix);
         SET_GetBufferPointervARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferSubDataARB) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferSubData", symbol_prefix);
         SET_GetBufferSubDataARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBufferSubDataARB) {
         snprintf(symboln, sizeof(symboln), "%sGetBufferSubDataARB", symbol_prefix);
         SET_GetBufferSubDataARB(disp, dlsym(handle, symboln));
    }


    if(!disp->IsBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sIsBuffer", symbol_prefix);
         SET_IsBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->IsBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sIsBufferARB", symbol_prefix);
         SET_IsBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MapBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sMapBuffer", symbol_prefix);
         SET_MapBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->MapBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sMapBufferARB", symbol_prefix);
         SET_MapBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UnmapBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sUnmapBuffer", symbol_prefix);
         SET_UnmapBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UnmapBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sUnmapBufferARB", symbol_prefix);
         SET_UnmapBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginQueryARB) {
         snprintf(symboln, sizeof(symboln), "%sBeginQuery", symbol_prefix);
         SET_BeginQueryARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginQueryARB) {
         snprintf(symboln, sizeof(symboln), "%sBeginQueryARB", symbol_prefix);
         SET_BeginQueryARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteQueriesARB) {
         snprintf(symboln, sizeof(symboln), "%sDeleteQueries", symbol_prefix);
         SET_DeleteQueriesARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteQueriesARB) {
         snprintf(symboln, sizeof(symboln), "%sDeleteQueriesARB", symbol_prefix);
         SET_DeleteQueriesARB(disp, dlsym(handle, symboln));
    }


    if(!disp->EndQueryARB) {
         snprintf(symboln, sizeof(symboln), "%sEndQuery", symbol_prefix);
         SET_EndQueryARB(disp, dlsym(handle, symboln));
    }


    if(!disp->EndQueryARB) {
         snprintf(symboln, sizeof(symboln), "%sEndQueryARB", symbol_prefix);
         SET_EndQueryARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GenQueriesARB) {
         snprintf(symboln, sizeof(symboln), "%sGenQueries", symbol_prefix);
         SET_GenQueriesARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GenQueriesARB) {
         snprintf(symboln, sizeof(symboln), "%sGenQueriesARB", symbol_prefix);
         SET_GenQueriesARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryObjectivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryObjectiv", symbol_prefix);
         SET_GetQueryObjectivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryObjectivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryObjectivARB", symbol_prefix);
         SET_GetQueryObjectivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryObjectuivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryObjectuiv", symbol_prefix);
         SET_GetQueryObjectuivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryObjectuivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryObjectuivARB", symbol_prefix);
         SET_GetQueryObjectuivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryiv", symbol_prefix);
         SET_GetQueryivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryivARB", symbol_prefix);
         SET_GetQueryivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->IsQueryARB) {
         snprintf(symboln, sizeof(symboln), "%sIsQuery", symbol_prefix);
         SET_IsQueryARB(disp, dlsym(handle, symboln));
    }


    if(!disp->IsQueryARB) {
         snprintf(symboln, sizeof(symboln), "%sIsQueryARB", symbol_prefix);
         SET_IsQueryARB(disp, dlsym(handle, symboln));
    }


    if(!disp->AttachObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sAttachObjectARB", symbol_prefix);
         SET_AttachObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompileShaderARB) {
         snprintf(symboln, sizeof(symboln), "%sCompileShader", symbol_prefix);
         SET_CompileShaderARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CompileShaderARB) {
         snprintf(symboln, sizeof(symboln), "%sCompileShaderARB", symbol_prefix);
         SET_CompileShaderARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CreateProgramObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sCreateProgramObjectARB", symbol_prefix);
         SET_CreateProgramObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->CreateShaderObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sCreateShaderObjectARB", symbol_prefix);
         SET_CreateShaderObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sDeleteObjectARB", symbol_prefix);
         SET_DeleteObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DetachObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sDetachObjectARB", symbol_prefix);
         SET_DetachObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetActiveUniformARB) {
         snprintf(symboln, sizeof(symboln), "%sGetActiveUniform", symbol_prefix);
         SET_GetActiveUniformARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetActiveUniformARB) {
         snprintf(symboln, sizeof(symboln), "%sGetActiveUniformARB", symbol_prefix);
         SET_GetActiveUniformARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetAttachedObjectsARB) {
         snprintf(symboln, sizeof(symboln), "%sGetAttachedObjectsARB", symbol_prefix);
         SET_GetAttachedObjectsARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetHandleARB) {
         snprintf(symboln, sizeof(symboln), "%sGetHandleARB", symbol_prefix);
         SET_GetHandleARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetInfoLogARB) {
         snprintf(symboln, sizeof(symboln), "%sGetInfoLogARB", symbol_prefix);
         SET_GetInfoLogARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetObjectParameterfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetObjectParameterfvARB", symbol_prefix);
         SET_GetObjectParameterfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetObjectParameterivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetObjectParameterivARB", symbol_prefix);
         SET_GetObjectParameterivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetShaderSourceARB) {
         snprintf(symboln, sizeof(symboln), "%sGetShaderSource", symbol_prefix);
         SET_GetShaderSourceARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetShaderSourceARB) {
         snprintf(symboln, sizeof(symboln), "%sGetShaderSourceARB", symbol_prefix);
         SET_GetShaderSourceARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformLocationARB) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformLocation", symbol_prefix);
         SET_GetUniformLocationARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformLocationARB) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformLocationARB", symbol_prefix);
         SET_GetUniformLocationARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformfv", symbol_prefix);
         SET_GetUniformfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformfvARB", symbol_prefix);
         SET_GetUniformfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformiv", symbol_prefix);
         SET_GetUniformivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformivARB", symbol_prefix);
         SET_GetUniformivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->LinkProgramARB) {
         snprintf(symboln, sizeof(symboln), "%sLinkProgram", symbol_prefix);
         SET_LinkProgramARB(disp, dlsym(handle, symboln));
    }


    if(!disp->LinkProgramARB) {
         snprintf(symboln, sizeof(symboln), "%sLinkProgramARB", symbol_prefix);
         SET_LinkProgramARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ShaderSourceARB) {
         snprintf(symboln, sizeof(symboln), "%sShaderSource", symbol_prefix);
         SET_ShaderSourceARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ShaderSourceARB) {
         snprintf(symboln, sizeof(symboln), "%sShaderSourceARB", symbol_prefix);
         SET_ShaderSourceARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1f", symbol_prefix);
         SET_Uniform1fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1fARB", symbol_prefix);
         SET_Uniform1fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1fv", symbol_prefix);
         SET_Uniform1fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1fvARB", symbol_prefix);
         SET_Uniform1fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1i", symbol_prefix);
         SET_Uniform1iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1iARB", symbol_prefix);
         SET_Uniform1iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1iv", symbol_prefix);
         SET_Uniform1ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform1ivARB", symbol_prefix);
         SET_Uniform1ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2f", symbol_prefix);
         SET_Uniform2fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2fARB", symbol_prefix);
         SET_Uniform2fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2fv", symbol_prefix);
         SET_Uniform2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2fvARB", symbol_prefix);
         SET_Uniform2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2i", symbol_prefix);
         SET_Uniform2iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2iARB", symbol_prefix);
         SET_Uniform2iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2iv", symbol_prefix);
         SET_Uniform2ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform2ivARB", symbol_prefix);
         SET_Uniform2ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3f", symbol_prefix);
         SET_Uniform3fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3fARB", symbol_prefix);
         SET_Uniform3fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3fv", symbol_prefix);
         SET_Uniform3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3fvARB", symbol_prefix);
         SET_Uniform3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3i", symbol_prefix);
         SET_Uniform3iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3iARB", symbol_prefix);
         SET_Uniform3iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3iv", symbol_prefix);
         SET_Uniform3ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform3ivARB", symbol_prefix);
         SET_Uniform3ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4f", symbol_prefix);
         SET_Uniform4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4fARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4fARB", symbol_prefix);
         SET_Uniform4fARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4fv", symbol_prefix);
         SET_Uniform4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4fvARB", symbol_prefix);
         SET_Uniform4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4i", symbol_prefix);
         SET_Uniform4iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4iARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4iARB", symbol_prefix);
         SET_Uniform4iARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4iv", symbol_prefix);
         SET_Uniform4ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4ivARB) {
         snprintf(symboln, sizeof(symboln), "%sUniform4ivARB", symbol_prefix);
         SET_Uniform4ivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix2fv", symbol_prefix);
         SET_UniformMatrix2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix2fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix2fvARB", symbol_prefix);
         SET_UniformMatrix2fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix3fv", symbol_prefix);
         SET_UniformMatrix3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix3fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix3fvARB", symbol_prefix);
         SET_UniformMatrix3fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix4fv", symbol_prefix);
         SET_UniformMatrix4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UniformMatrix4fvARB) {
         snprintf(symboln, sizeof(symboln), "%sUniformMatrix4fvARB", symbol_prefix);
         SET_UniformMatrix4fvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UseProgramObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sUseProgram", symbol_prefix);
         SET_UseProgramObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->UseProgramObjectARB) {
         snprintf(symboln, sizeof(symboln), "%sUseProgramObjectARB", symbol_prefix);
         SET_UseProgramObjectARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ValidateProgramARB) {
         snprintf(symboln, sizeof(symboln), "%sValidateProgram", symbol_prefix);
         SET_ValidateProgramARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ValidateProgramARB) {
         snprintf(symboln, sizeof(symboln), "%sValidateProgramARB", symbol_prefix);
         SET_ValidateProgramARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BindAttribLocationARB) {
         snprintf(symboln, sizeof(symboln), "%sBindAttribLocation", symbol_prefix);
         SET_BindAttribLocationARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BindAttribLocationARB) {
         snprintf(symboln, sizeof(symboln), "%sBindAttribLocationARB", symbol_prefix);
         SET_BindAttribLocationARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetActiveAttribARB) {
         snprintf(symboln, sizeof(symboln), "%sGetActiveAttrib", symbol_prefix);
         SET_GetActiveAttribARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetActiveAttribARB) {
         snprintf(symboln, sizeof(symboln), "%sGetActiveAttribARB", symbol_prefix);
         SET_GetActiveAttribARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetAttribLocationARB) {
         snprintf(symboln, sizeof(symboln), "%sGetAttribLocation", symbol_prefix);
         SET_GetAttribLocationARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetAttribLocationARB) {
         snprintf(symboln, sizeof(symboln), "%sGetAttribLocationARB", symbol_prefix);
         SET_GetAttribLocationARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawBuffers", symbol_prefix);
         SET_DrawBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawBuffersARB", symbol_prefix);
         SET_DrawBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawBuffersARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawBuffersATI", symbol_prefix);
         SET_DrawBuffersARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ClampColorARB) {
         snprintf(symboln, sizeof(symboln), "%sClampColorARB", symbol_prefix);
         SET_ClampColorARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawArraysInstancedARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawArraysInstancedARB", symbol_prefix);
         SET_DrawArraysInstancedARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawArraysInstancedARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawArraysInstancedEXT", symbol_prefix);
         SET_DrawArraysInstancedARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawArraysInstancedARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawArraysInstanced", symbol_prefix);
         SET_DrawArraysInstancedARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawElementsInstancedARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedARB", symbol_prefix);
         SET_DrawElementsInstancedARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawElementsInstancedARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedEXT", symbol_prefix);
         SET_DrawElementsInstancedARB(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawElementsInstancedARB) {
         snprintf(symboln, sizeof(symboln), "%sDrawElementsInstanced", symbol_prefix);
         SET_DrawElementsInstancedARB(disp, dlsym(handle, symboln));
    }


    if(!disp->RenderbufferStorageMultisample) {
         snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageMultisample", symbol_prefix);
         SET_RenderbufferStorageMultisample(disp, dlsym(handle, symboln));
    }


    if(!disp->RenderbufferStorageMultisample) {
         snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageMultisampleEXT", symbol_prefix);
         SET_RenderbufferStorageMultisample(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTextureARB) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTextureARB", symbol_prefix);
         SET_FramebufferTextureARB(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTextureFaceARB) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTextureFaceARB", symbol_prefix);
         SET_FramebufferTextureFaceARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramParameteriARB) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameteriARB", symbol_prefix);
         SET_ProgramParameteriARB(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribDivisorARB) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribDivisorARB", symbol_prefix);
         SET_VertexAttribDivisorARB(disp, dlsym(handle, symboln));
    }


    if(!disp->FlushMappedBufferRange) {
         snprintf(symboln, sizeof(symboln), "%sFlushMappedBufferRange", symbol_prefix);
         SET_FlushMappedBufferRange(disp, dlsym(handle, symboln));
    }


    if(!disp->MapBufferRange) {
         snprintf(symboln, sizeof(symboln), "%sMapBufferRange", symbol_prefix);
         SET_MapBufferRange(disp, dlsym(handle, symboln));
    }


    if(!disp->TexBufferARB) {
         snprintf(symboln, sizeof(symboln), "%sTexBufferARB", symbol_prefix);
         SET_TexBufferARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BindVertexArray) {
         snprintf(symboln, sizeof(symboln), "%sBindVertexArray", symbol_prefix);
         SET_BindVertexArray(disp, dlsym(handle, symboln));
    }


    if(!disp->GenVertexArrays) {
         snprintf(symboln, sizeof(symboln), "%sGenVertexArrays", symbol_prefix);
         SET_GenVertexArrays(disp, dlsym(handle, symboln));
    }


    if(!disp->CopyBufferSubData) {
         snprintf(symboln, sizeof(symboln), "%sCopyBufferSubData", symbol_prefix);
         SET_CopyBufferSubData(disp, dlsym(handle, symboln));
    }


    if(!disp->ClientWaitSync) {
         snprintf(symboln, sizeof(symboln), "%sClientWaitSync", symbol_prefix);
         SET_ClientWaitSync(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteSync) {
         snprintf(symboln, sizeof(symboln), "%sDeleteSync", symbol_prefix);
         SET_DeleteSync(disp, dlsym(handle, symboln));
    }


    if(!disp->FenceSync) {
         snprintf(symboln, sizeof(symboln), "%sFenceSync", symbol_prefix);
         SET_FenceSync(disp, dlsym(handle, symboln));
    }


    if(!disp->GetInteger64v) {
         snprintf(symboln, sizeof(symboln), "%sGetInteger64v", symbol_prefix);
         SET_GetInteger64v(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSynciv) {
         snprintf(symboln, sizeof(symboln), "%sGetSynciv", symbol_prefix);
         SET_GetSynciv(disp, dlsym(handle, symboln));
    }


    if(!disp->IsSync) {
         snprintf(symboln, sizeof(symboln), "%sIsSync", symbol_prefix);
         SET_IsSync(disp, dlsym(handle, symboln));
    }


    if(!disp->WaitSync) {
         snprintf(symboln, sizeof(symboln), "%sWaitSync", symbol_prefix);
         SET_WaitSync(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawElementsBaseVertex) {
         snprintf(symboln, sizeof(symboln), "%sDrawElementsBaseVertex", symbol_prefix);
         SET_DrawElementsBaseVertex(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawElementsInstancedBaseVertex) {
         snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedBaseVertex", symbol_prefix);
         SET_DrawElementsInstancedBaseVertex(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawRangeElementsBaseVertex) {
         snprintf(symboln, sizeof(symboln), "%sDrawRangeElementsBaseVertex", symbol_prefix);
         SET_DrawRangeElementsBaseVertex(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiDrawElementsBaseVertex) {
         snprintf(symboln, sizeof(symboln), "%sMultiDrawElementsBaseVertex", symbol_prefix);
         SET_MultiDrawElementsBaseVertex(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationSeparateiARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateiARB", symbol_prefix);
         SET_BlendEquationSeparateiARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationSeparateiARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateIndexedAMD", symbol_prefix);
         SET_BlendEquationSeparateiARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationiARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationiARB", symbol_prefix);
         SET_BlendEquationiARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationiARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationIndexedAMD", symbol_prefix);
         SET_BlendEquationiARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFuncSeparateiARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateiARB", symbol_prefix);
         SET_BlendFuncSeparateiARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFuncSeparateiARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateIndexedAMD", symbol_prefix);
         SET_BlendFuncSeparateiARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFunciARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendFunciARB", symbol_prefix);
         SET_BlendFunciARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFunciARB) {
         snprintf(symboln, sizeof(symboln), "%sBlendFuncIndexedAMD", symbol_prefix);
         SET_BlendFunciARB(disp, dlsym(handle, symboln));
    }


    if(!disp->BindSampler) {
         snprintf(symboln, sizeof(symboln), "%sBindSampler", symbol_prefix);
         SET_BindSampler(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteSamplers) {
         snprintf(symboln, sizeof(symboln), "%sDeleteSamplers", symbol_prefix);
         SET_DeleteSamplers(disp, dlsym(handle, symboln));
    }


    if(!disp->GenSamplers) {
         snprintf(symboln, sizeof(symboln), "%sGenSamplers", symbol_prefix);
         SET_GenSamplers(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSamplerParameterIiv) {
         snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterIiv", symbol_prefix);
         SET_GetSamplerParameterIiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSamplerParameterIuiv) {
         snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterIuiv", symbol_prefix);
         SET_GetSamplerParameterIuiv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSamplerParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterfv", symbol_prefix);
         SET_GetSamplerParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->GetSamplerParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sGetSamplerParameteriv", symbol_prefix);
         SET_GetSamplerParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->IsSampler) {
         snprintf(symboln, sizeof(symboln), "%sIsSampler", symbol_prefix);
         SET_IsSampler(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplerParameterIiv) {
         snprintf(symboln, sizeof(symboln), "%sSamplerParameterIiv", symbol_prefix);
         SET_SamplerParameterIiv(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplerParameterIuiv) {
         snprintf(symboln, sizeof(symboln), "%sSamplerParameterIuiv", symbol_prefix);
         SET_SamplerParameterIuiv(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplerParameterf) {
         snprintf(symboln, sizeof(symboln), "%sSamplerParameterf", symbol_prefix);
         SET_SamplerParameterf(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplerParameterfv) {
         snprintf(symboln, sizeof(symboln), "%sSamplerParameterfv", symbol_prefix);
         SET_SamplerParameterfv(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplerParameteri) {
         snprintf(symboln, sizeof(symboln), "%sSamplerParameteri", symbol_prefix);
         SET_SamplerParameteri(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplerParameteriv) {
         snprintf(symboln, sizeof(symboln), "%sSamplerParameteriv", symbol_prefix);
         SET_SamplerParameteriv(disp, dlsym(handle, symboln));
    }


    if(!disp->BindTransformFeedback) {
         snprintf(symboln, sizeof(symboln), "%sBindTransformFeedback", symbol_prefix);
         SET_BindTransformFeedback(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteTransformFeedbacks) {
         snprintf(symboln, sizeof(symboln), "%sDeleteTransformFeedbacks", symbol_prefix);
         SET_DeleteTransformFeedbacks(disp, dlsym(handle, symboln));
    }


    if(!disp->DrawTransformFeedback) {
         snprintf(symboln, sizeof(symboln), "%sDrawTransformFeedback", symbol_prefix);
         SET_DrawTransformFeedback(disp, dlsym(handle, symboln));
    }


    if(!disp->GenTransformFeedbacks) {
         snprintf(symboln, sizeof(symboln), "%sGenTransformFeedbacks", symbol_prefix);
         SET_GenTransformFeedbacks(disp, dlsym(handle, symboln));
    }


    if(!disp->IsTransformFeedback) {
         snprintf(symboln, sizeof(symboln), "%sIsTransformFeedback", symbol_prefix);
         SET_IsTransformFeedback(disp, dlsym(handle, symboln));
    }


    if(!disp->PauseTransformFeedback) {
         snprintf(symboln, sizeof(symboln), "%sPauseTransformFeedback", symbol_prefix);
         SET_PauseTransformFeedback(disp, dlsym(handle, symboln));
    }


    if(!disp->ResumeTransformFeedback) {
         snprintf(symboln, sizeof(symboln), "%sResumeTransformFeedback", symbol_prefix);
         SET_ResumeTransformFeedback(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearDepthf) {
         snprintf(symboln, sizeof(symboln), "%sClearDepthf", symbol_prefix);
         SET_ClearDepthf(disp, dlsym(handle, symboln));
    }


    if(!disp->DepthRangef) {
         snprintf(symboln, sizeof(symboln), "%sDepthRangef", symbol_prefix);
         SET_DepthRangef(disp, dlsym(handle, symboln));
    }


    if(!disp->GetShaderPrecisionFormat) {
         snprintf(symboln, sizeof(symboln), "%sGetShaderPrecisionFormat", symbol_prefix);
         SET_GetShaderPrecisionFormat(disp, dlsym(handle, symboln));
    }


    if(!disp->ReleaseShaderCompiler) {
         snprintf(symboln, sizeof(symboln), "%sReleaseShaderCompiler", symbol_prefix);
         SET_ReleaseShaderCompiler(disp, dlsym(handle, symboln));
    }


    if(!disp->ShaderBinary) {
         snprintf(symboln, sizeof(symboln), "%sShaderBinary", symbol_prefix);
         SET_ShaderBinary(disp, dlsym(handle, symboln));
    }


    if(!disp->GetGraphicsResetStatusARB) {
         snprintf(symboln, sizeof(symboln), "%sGetGraphicsResetStatusARB", symbol_prefix);
         SET_GetGraphicsResetStatusARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnColorTableARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnColorTableARB", symbol_prefix);
         SET_GetnColorTableARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnCompressedTexImageARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnCompressedTexImageARB", symbol_prefix);
         SET_GetnCompressedTexImageARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnConvolutionFilterARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnConvolutionFilterARB", symbol_prefix);
         SET_GetnConvolutionFilterARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnHistogramARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnHistogramARB", symbol_prefix);
         SET_GetnHistogramARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnMapdvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnMapdvARB", symbol_prefix);
         SET_GetnMapdvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnMapfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnMapfvARB", symbol_prefix);
         SET_GetnMapfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnMapivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnMapivARB", symbol_prefix);
         SET_GetnMapivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnMinmaxARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnMinmaxARB", symbol_prefix);
         SET_GetnMinmaxARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnPixelMapfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnPixelMapfvARB", symbol_prefix);
         SET_GetnPixelMapfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnPixelMapuivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnPixelMapuivARB", symbol_prefix);
         SET_GetnPixelMapuivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnPixelMapusvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnPixelMapusvARB", symbol_prefix);
         SET_GetnPixelMapusvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnPolygonStippleARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnPolygonStippleARB", symbol_prefix);
         SET_GetnPolygonStippleARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnSeparableFilterARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnSeparableFilterARB", symbol_prefix);
         SET_GetnSeparableFilterARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnTexImageARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnTexImageARB", symbol_prefix);
         SET_GetnTexImageARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnUniformdvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnUniformdvARB", symbol_prefix);
         SET_GetnUniformdvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnUniformfvARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnUniformfvARB", symbol_prefix);
         SET_GetnUniformfvARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnUniformivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnUniformivARB", symbol_prefix);
         SET_GetnUniformivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->GetnUniformuivARB) {
         snprintf(symboln, sizeof(symboln), "%sGetnUniformuivARB", symbol_prefix);
         SET_GetnUniformuivARB(disp, dlsym(handle, symboln));
    }


    if(!disp->ReadnPixelsARB) {
         snprintf(symboln, sizeof(symboln), "%sReadnPixelsARB", symbol_prefix);
         SET_ReadnPixelsARB(disp, dlsym(handle, symboln));
    }


    if(!disp->PolygonOffsetEXT) {
         snprintf(symboln, sizeof(symboln), "%sPolygonOffsetEXT", symbol_prefix);
         SET_PolygonOffsetEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPixelTexGenParameterfvSGIS) {
         snprintf(symboln, sizeof(symboln), "%sGetPixelTexGenParameterfvSGIS", symbol_prefix);
         SET_GetPixelTexGenParameterfvSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->GetPixelTexGenParameterivSGIS) {
         snprintf(symboln, sizeof(symboln), "%sGetPixelTexGenParameterivSGIS", symbol_prefix);
         SET_GetPixelTexGenParameterivSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTexGenParameterfSGIS) {
         snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterfSGIS", symbol_prefix);
         SET_PixelTexGenParameterfSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTexGenParameterfvSGIS) {
         snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterfvSGIS", symbol_prefix);
         SET_PixelTexGenParameterfvSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTexGenParameteriSGIS) {
         snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameteriSGIS", symbol_prefix);
         SET_PixelTexGenParameteriSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTexGenParameterivSGIS) {
         snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterivSGIS", symbol_prefix);
         SET_PixelTexGenParameterivSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->SampleMaskSGIS) {
         snprintf(symboln, sizeof(symboln), "%sSampleMaskSGIS", symbol_prefix);
         SET_SampleMaskSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->SampleMaskSGIS) {
         snprintf(symboln, sizeof(symboln), "%sSampleMaskEXT", symbol_prefix);
         SET_SampleMaskSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplePatternSGIS) {
         snprintf(symboln, sizeof(symboln), "%sSamplePatternSGIS", symbol_prefix);
         SET_SamplePatternSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->SamplePatternSGIS) {
         snprintf(symboln, sizeof(symboln), "%sSamplePatternEXT", symbol_prefix);
         SET_SamplePatternSGIS(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sColorPointerEXT", symbol_prefix);
         SET_ColorPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->EdgeFlagPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sEdgeFlagPointerEXT", symbol_prefix);
         SET_EdgeFlagPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IndexPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sIndexPointerEXT", symbol_prefix);
         SET_IndexPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->NormalPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sNormalPointerEXT", symbol_prefix);
         SET_NormalPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TexCoordPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sTexCoordPointerEXT", symbol_prefix);
         SET_TexCoordPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexPointerEXT", symbol_prefix);
         SET_VertexPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterf", symbol_prefix);
         SET_PointParameterfEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfARB", symbol_prefix);
         SET_PointParameterfEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfEXT", symbol_prefix);
         SET_PointParameterfEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfSGIS", symbol_prefix);
         SET_PointParameterfEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfvEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfv", symbol_prefix);
         SET_PointParameterfvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfvEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfvARB", symbol_prefix);
         SET_PointParameterfvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfvEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfvEXT", symbol_prefix);
         SET_PointParameterfvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterfvEXT) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterfvSGIS", symbol_prefix);
         SET_PointParameterfvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->LockArraysEXT) {
         snprintf(symboln, sizeof(symboln), "%sLockArraysEXT", symbol_prefix);
         SET_LockArraysEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->UnlockArraysEXT) {
         snprintf(symboln, sizeof(symboln), "%sUnlockArraysEXT", symbol_prefix);
         SET_UnlockArraysEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3bEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3b", symbol_prefix);
         SET_SecondaryColor3bEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3bEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bEXT", symbol_prefix);
         SET_SecondaryColor3bEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3bvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bv", symbol_prefix);
         SET_SecondaryColor3bvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3bvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bvEXT", symbol_prefix);
         SET_SecondaryColor3bvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3dEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3d", symbol_prefix);
         SET_SecondaryColor3dEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3dEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dEXT", symbol_prefix);
         SET_SecondaryColor3dEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3dvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dv", symbol_prefix);
         SET_SecondaryColor3dvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3dvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dvEXT", symbol_prefix);
         SET_SecondaryColor3dvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3fEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3f", symbol_prefix);
         SET_SecondaryColor3fEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3fEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fEXT", symbol_prefix);
         SET_SecondaryColor3fEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3fvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fv", symbol_prefix);
         SET_SecondaryColor3fvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3fvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fvEXT", symbol_prefix);
         SET_SecondaryColor3fvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3iEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3i", symbol_prefix);
         SET_SecondaryColor3iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3iEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3iEXT", symbol_prefix);
         SET_SecondaryColor3iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3iv", symbol_prefix);
         SET_SecondaryColor3ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ivEXT", symbol_prefix);
         SET_SecondaryColor3ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3sEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3s", symbol_prefix);
         SET_SecondaryColor3sEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3sEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3sEXT", symbol_prefix);
         SET_SecondaryColor3sEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3svEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3sv", symbol_prefix);
         SET_SecondaryColor3svEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3svEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3svEXT", symbol_prefix);
         SET_SecondaryColor3svEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3ubEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ub", symbol_prefix);
         SET_SecondaryColor3ubEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3ubEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubEXT", symbol_prefix);
         SET_SecondaryColor3ubEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3ubvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubv", symbol_prefix);
         SET_SecondaryColor3ubvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3ubvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubvEXT", symbol_prefix);
         SET_SecondaryColor3ubvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ui", symbol_prefix);
         SET_SecondaryColor3uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uiEXT", symbol_prefix);
         SET_SecondaryColor3uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uiv", symbol_prefix);
         SET_SecondaryColor3uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uivEXT", symbol_prefix);
         SET_SecondaryColor3uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3usEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3us", symbol_prefix);
         SET_SecondaryColor3usEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3usEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usEXT", symbol_prefix);
         SET_SecondaryColor3usEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3usvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usv", symbol_prefix);
         SET_SecondaryColor3usvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColor3usvEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usvEXT", symbol_prefix);
         SET_SecondaryColor3usvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColorPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColorPointer", symbol_prefix);
         SET_SecondaryColorPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->SecondaryColorPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sSecondaryColorPointerEXT", symbol_prefix);
         SET_SecondaryColorPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiDrawArraysEXT) {
         snprintf(symboln, sizeof(symboln), "%sMultiDrawArrays", symbol_prefix);
         SET_MultiDrawArraysEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiDrawArraysEXT) {
         snprintf(symboln, sizeof(symboln), "%sMultiDrawArraysEXT", symbol_prefix);
         SET_MultiDrawArraysEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiDrawElementsEXT) {
         snprintf(symboln, sizeof(symboln), "%sMultiDrawElements", symbol_prefix);
         SET_MultiDrawElementsEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiDrawElementsEXT) {
         snprintf(symboln, sizeof(symboln), "%sMultiDrawElementsEXT", symbol_prefix);
         SET_MultiDrawElementsEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoordPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordPointer", symbol_prefix);
         SET_FogCoordPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoordPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordPointerEXT", symbol_prefix);
         SET_FogCoordPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoorddEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordd", symbol_prefix);
         SET_FogCoorddEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoorddEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoorddEXT", symbol_prefix);
         SET_FogCoorddEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoorddvEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoorddv", symbol_prefix);
         SET_FogCoorddvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoorddvEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoorddvEXT", symbol_prefix);
         SET_FogCoorddvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoordfEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordf", symbol_prefix);
         SET_FogCoordfEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoordfEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordfEXT", symbol_prefix);
         SET_FogCoordfEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoordfvEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordfv", symbol_prefix);
         SET_FogCoordfvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FogCoordfvEXT) {
         snprintf(symboln, sizeof(symboln), "%sFogCoordfvEXT", symbol_prefix);
         SET_FogCoordfvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->PixelTexGenSGIX) {
         snprintf(symboln, sizeof(symboln), "%sPixelTexGenSGIX", symbol_prefix);
         SET_PixelTexGenSGIX(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFuncSeparateEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparate", symbol_prefix);
         SET_BlendFuncSeparateEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFuncSeparateEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateEXT", symbol_prefix);
         SET_BlendFuncSeparateEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendFuncSeparateEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateINGR", symbol_prefix);
         SET_BlendFuncSeparateEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FlushVertexArrayRangeNV) {
         snprintf(symboln, sizeof(symboln), "%sFlushVertexArrayRangeNV", symbol_prefix);
         SET_FlushVertexArrayRangeNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexArrayRangeNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexArrayRangeNV", symbol_prefix);
         SET_VertexArrayRangeNV(disp, dlsym(handle, symboln));
    }


    if(!disp->CombinerInputNV) {
         snprintf(symboln, sizeof(symboln), "%sCombinerInputNV", symbol_prefix);
         SET_CombinerInputNV(disp, dlsym(handle, symboln));
    }


    if(!disp->CombinerOutputNV) {
         snprintf(symboln, sizeof(symboln), "%sCombinerOutputNV", symbol_prefix);
         SET_CombinerOutputNV(disp, dlsym(handle, symboln));
    }


    if(!disp->CombinerParameterfNV) {
         snprintf(symboln, sizeof(symboln), "%sCombinerParameterfNV", symbol_prefix);
         SET_CombinerParameterfNV(disp, dlsym(handle, symboln));
    }


    if(!disp->CombinerParameterfvNV) {
         snprintf(symboln, sizeof(symboln), "%sCombinerParameterfvNV", symbol_prefix);
         SET_CombinerParameterfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->CombinerParameteriNV) {
         snprintf(symboln, sizeof(symboln), "%sCombinerParameteriNV", symbol_prefix);
         SET_CombinerParameteriNV(disp, dlsym(handle, symboln));
    }


    if(!disp->CombinerParameterivNV) {
         snprintf(symboln, sizeof(symboln), "%sCombinerParameterivNV", symbol_prefix);
         SET_CombinerParameterivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->FinalCombinerInputNV) {
         snprintf(symboln, sizeof(symboln), "%sFinalCombinerInputNV", symbol_prefix);
         SET_FinalCombinerInputNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetCombinerInputParameterfvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetCombinerInputParameterfvNV", symbol_prefix);
         SET_GetCombinerInputParameterfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetCombinerInputParameterivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetCombinerInputParameterivNV", symbol_prefix);
         SET_GetCombinerInputParameterivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetCombinerOutputParameterfvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetCombinerOutputParameterfvNV", symbol_prefix);
         SET_GetCombinerOutputParameterfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetCombinerOutputParameterivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetCombinerOutputParameterivNV", symbol_prefix);
         SET_GetCombinerOutputParameterivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFinalCombinerInputParameterfvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetFinalCombinerInputParameterfvNV", symbol_prefix);
         SET_GetFinalCombinerInputParameterfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFinalCombinerInputParameterivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetFinalCombinerInputParameterivNV", symbol_prefix);
         SET_GetFinalCombinerInputParameterivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ResizeBuffersMESA) {
         snprintf(symboln, sizeof(symboln), "%sResizeBuffersMESA", symbol_prefix);
         SET_ResizeBuffersMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2d", symbol_prefix);
         SET_WindowPos2dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2dARB", symbol_prefix);
         SET_WindowPos2dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2dMESA", symbol_prefix);
         SET_WindowPos2dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2dv", symbol_prefix);
         SET_WindowPos2dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2dvARB", symbol_prefix);
         SET_WindowPos2dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2dvMESA", symbol_prefix);
         SET_WindowPos2dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2f", symbol_prefix);
         SET_WindowPos2fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2fARB", symbol_prefix);
         SET_WindowPos2fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2fMESA", symbol_prefix);
         SET_WindowPos2fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2fv", symbol_prefix);
         SET_WindowPos2fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2fvARB", symbol_prefix);
         SET_WindowPos2fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2fvMESA", symbol_prefix);
         SET_WindowPos2fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2i", symbol_prefix);
         SET_WindowPos2iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2iARB", symbol_prefix);
         SET_WindowPos2iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2iMESA", symbol_prefix);
         SET_WindowPos2iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2iv", symbol_prefix);
         SET_WindowPos2ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2ivARB", symbol_prefix);
         SET_WindowPos2ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2ivMESA", symbol_prefix);
         SET_WindowPos2ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2s", symbol_prefix);
         SET_WindowPos2sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2sARB", symbol_prefix);
         SET_WindowPos2sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2sMESA", symbol_prefix);
         SET_WindowPos2sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2sv", symbol_prefix);
         SET_WindowPos2svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2svARB", symbol_prefix);
         SET_WindowPos2svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos2svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos2svMESA", symbol_prefix);
         SET_WindowPos2svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3d", symbol_prefix);
         SET_WindowPos3dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3dARB", symbol_prefix);
         SET_WindowPos3dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3dMESA", symbol_prefix);
         SET_WindowPos3dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3dv", symbol_prefix);
         SET_WindowPos3dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3dvARB", symbol_prefix);
         SET_WindowPos3dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3dvMESA", symbol_prefix);
         SET_WindowPos3dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3f", symbol_prefix);
         SET_WindowPos3fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3fARB", symbol_prefix);
         SET_WindowPos3fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3fMESA", symbol_prefix);
         SET_WindowPos3fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3fv", symbol_prefix);
         SET_WindowPos3fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3fvARB", symbol_prefix);
         SET_WindowPos3fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3fvMESA", symbol_prefix);
         SET_WindowPos3fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3i", symbol_prefix);
         SET_WindowPos3iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3iARB", symbol_prefix);
         SET_WindowPos3iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3iMESA", symbol_prefix);
         SET_WindowPos3iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3iv", symbol_prefix);
         SET_WindowPos3ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3ivARB", symbol_prefix);
         SET_WindowPos3ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3ivMESA", symbol_prefix);
         SET_WindowPos3ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3s", symbol_prefix);
         SET_WindowPos3sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3sARB", symbol_prefix);
         SET_WindowPos3sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3sMESA", symbol_prefix);
         SET_WindowPos3sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3sv", symbol_prefix);
         SET_WindowPos3svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3svARB", symbol_prefix);
         SET_WindowPos3svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos3svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos3svMESA", symbol_prefix);
         SET_WindowPos3svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4dMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4dMESA", symbol_prefix);
         SET_WindowPos4dMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4dvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4dvMESA", symbol_prefix);
         SET_WindowPos4dvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4fMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4fMESA", symbol_prefix);
         SET_WindowPos4fMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4fvMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4fvMESA", symbol_prefix);
         SET_WindowPos4fvMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4iMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4iMESA", symbol_prefix);
         SET_WindowPos4iMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4ivMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4ivMESA", symbol_prefix);
         SET_WindowPos4ivMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4sMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4sMESA", symbol_prefix);
         SET_WindowPos4sMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->WindowPos4svMESA) {
         snprintf(symboln, sizeof(symboln), "%sWindowPos4svMESA", symbol_prefix);
         SET_WindowPos4svMESA(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiModeDrawArraysIBM) {
         snprintf(symboln, sizeof(symboln), "%sMultiModeDrawArraysIBM", symbol_prefix);
         SET_MultiModeDrawArraysIBM(disp, dlsym(handle, symboln));
    }


    if(!disp->MultiModeDrawElementsIBM) {
         snprintf(symboln, sizeof(symboln), "%sMultiModeDrawElementsIBM", symbol_prefix);
         SET_MultiModeDrawElementsIBM(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteFencesNV) {
         snprintf(symboln, sizeof(symboln), "%sDeleteFencesNV", symbol_prefix);
         SET_DeleteFencesNV(disp, dlsym(handle, symboln));
    }


    if(!disp->FinishFenceNV) {
         snprintf(symboln, sizeof(symboln), "%sFinishFenceNV", symbol_prefix);
         SET_FinishFenceNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GenFencesNV) {
         snprintf(symboln, sizeof(symboln), "%sGenFencesNV", symbol_prefix);
         SET_GenFencesNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFenceivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetFenceivNV", symbol_prefix);
         SET_GetFenceivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->IsFenceNV) {
         snprintf(symboln, sizeof(symboln), "%sIsFenceNV", symbol_prefix);
         SET_IsFenceNV(disp, dlsym(handle, symboln));
    }


    if(!disp->SetFenceNV) {
         snprintf(symboln, sizeof(symboln), "%sSetFenceNV", symbol_prefix);
         SET_SetFenceNV(disp, dlsym(handle, symboln));
    }


    if(!disp->TestFenceNV) {
         snprintf(symboln, sizeof(symboln), "%sTestFenceNV", symbol_prefix);
         SET_TestFenceNV(disp, dlsym(handle, symboln));
    }


    if(!disp->AreProgramsResidentNV) {
         snprintf(symboln, sizeof(symboln), "%sAreProgramsResidentNV", symbol_prefix);
         SET_AreProgramsResidentNV(disp, dlsym(handle, symboln));
    }


    if(!disp->BindProgramNV) {
         snprintf(symboln, sizeof(symboln), "%sBindProgramARB", symbol_prefix);
         SET_BindProgramNV(disp, dlsym(handle, symboln));
    }


    if(!disp->BindProgramNV) {
         snprintf(symboln, sizeof(symboln), "%sBindProgramNV", symbol_prefix);
         SET_BindProgramNV(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteProgramsNV) {
         snprintf(symboln, sizeof(symboln), "%sDeleteProgramsARB", symbol_prefix);
         SET_DeleteProgramsNV(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteProgramsNV) {
         snprintf(symboln, sizeof(symboln), "%sDeleteProgramsNV", symbol_prefix);
         SET_DeleteProgramsNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ExecuteProgramNV) {
         snprintf(symboln, sizeof(symboln), "%sExecuteProgramNV", symbol_prefix);
         SET_ExecuteProgramNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GenProgramsNV) {
         snprintf(symboln, sizeof(symboln), "%sGenProgramsARB", symbol_prefix);
         SET_GenProgramsNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GenProgramsNV) {
         snprintf(symboln, sizeof(symboln), "%sGenProgramsNV", symbol_prefix);
         SET_GenProgramsNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramParameterdvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramParameterdvNV", symbol_prefix);
         SET_GetProgramParameterdvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramParameterfvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramParameterfvNV", symbol_prefix);
         SET_GetProgramParameterfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramStringNV) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramStringNV", symbol_prefix);
         SET_GetProgramStringNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramivNV", symbol_prefix);
         SET_GetProgramivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTrackMatrixivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetTrackMatrixivNV", symbol_prefix);
         SET_GetTrackMatrixivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribPointervNV) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointerv", symbol_prefix);
         SET_GetVertexAttribPointervNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribPointervNV) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointervARB", symbol_prefix);
         SET_GetVertexAttribPointervNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribPointervNV) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointervNV", symbol_prefix);
         SET_GetVertexAttribPointervNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribdvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdvNV", symbol_prefix);
         SET_GetVertexAttribdvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribfvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfvNV", symbol_prefix);
         SET_GetVertexAttribfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribivNV) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribivNV", symbol_prefix);
         SET_GetVertexAttribivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->IsProgramNV) {
         snprintf(symboln, sizeof(symboln), "%sIsProgramARB", symbol_prefix);
         SET_IsProgramNV(disp, dlsym(handle, symboln));
    }


    if(!disp->IsProgramNV) {
         snprintf(symboln, sizeof(symboln), "%sIsProgramNV", symbol_prefix);
         SET_IsProgramNV(disp, dlsym(handle, symboln));
    }


    if(!disp->LoadProgramNV) {
         snprintf(symboln, sizeof(symboln), "%sLoadProgramNV", symbol_prefix);
         SET_LoadProgramNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramParameters4dvNV) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameters4dvNV", symbol_prefix);
         SET_ProgramParameters4dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramParameters4fvNV) {
         snprintf(symboln, sizeof(symboln), "%sProgramParameters4fvNV", symbol_prefix);
         SET_ProgramParameters4fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->RequestResidentProgramsNV) {
         snprintf(symboln, sizeof(symboln), "%sRequestResidentProgramsNV", symbol_prefix);
         SET_RequestResidentProgramsNV(disp, dlsym(handle, symboln));
    }


    if(!disp->TrackMatrixNV) {
         snprintf(symboln, sizeof(symboln), "%sTrackMatrixNV", symbol_prefix);
         SET_TrackMatrixNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1dNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dNV", symbol_prefix);
         SET_VertexAttrib1dNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dvNV", symbol_prefix);
         SET_VertexAttrib1dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1fNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fNV", symbol_prefix);
         SET_VertexAttrib1fNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fvNV", symbol_prefix);
         SET_VertexAttrib1fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1sNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sNV", symbol_prefix);
         SET_VertexAttrib1sNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib1svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib1svNV", symbol_prefix);
         SET_VertexAttrib1svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2dNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dNV", symbol_prefix);
         SET_VertexAttrib2dNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dvNV", symbol_prefix);
         SET_VertexAttrib2dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2fNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fNV", symbol_prefix);
         SET_VertexAttrib2fNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fvNV", symbol_prefix);
         SET_VertexAttrib2fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2sNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sNV", symbol_prefix);
         SET_VertexAttrib2sNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib2svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib2svNV", symbol_prefix);
         SET_VertexAttrib2svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3dNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dNV", symbol_prefix);
         SET_VertexAttrib3dNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dvNV", symbol_prefix);
         SET_VertexAttrib3dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3fNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fNV", symbol_prefix);
         SET_VertexAttrib3fNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fvNV", symbol_prefix);
         SET_VertexAttrib3fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3sNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sNV", symbol_prefix);
         SET_VertexAttrib3sNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib3svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib3svNV", symbol_prefix);
         SET_VertexAttrib3svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4dNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dNV", symbol_prefix);
         SET_VertexAttrib4dNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dvNV", symbol_prefix);
         SET_VertexAttrib4dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4fNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fNV", symbol_prefix);
         SET_VertexAttrib4fNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fvNV", symbol_prefix);
         SET_VertexAttrib4fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4sNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sNV", symbol_prefix);
         SET_VertexAttrib4sNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4svNV", symbol_prefix);
         SET_VertexAttrib4svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4ubNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubNV", symbol_prefix);
         SET_VertexAttrib4ubNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttrib4ubvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubvNV", symbol_prefix);
         SET_VertexAttrib4ubvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribPointerNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribPointerNV", symbol_prefix);
         SET_VertexAttribPointerNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs1dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs1dvNV", symbol_prefix);
         SET_VertexAttribs1dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs1fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs1fvNV", symbol_prefix);
         SET_VertexAttribs1fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs1svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs1svNV", symbol_prefix);
         SET_VertexAttribs1svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs2dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs2dvNV", symbol_prefix);
         SET_VertexAttribs2dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs2fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs2fvNV", symbol_prefix);
         SET_VertexAttribs2fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs2svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs2svNV", symbol_prefix);
         SET_VertexAttribs2svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs3dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs3dvNV", symbol_prefix);
         SET_VertexAttribs3dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs3fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs3fvNV", symbol_prefix);
         SET_VertexAttribs3fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs3svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs3svNV", symbol_prefix);
         SET_VertexAttribs3svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs4dvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs4dvNV", symbol_prefix);
         SET_VertexAttribs4dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs4fvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs4fvNV", symbol_prefix);
         SET_VertexAttribs4fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs4svNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs4svNV", symbol_prefix);
         SET_VertexAttribs4svNV(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribs4ubvNV) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribs4ubvNV", symbol_prefix);
         SET_VertexAttribs4ubvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexBumpParameterfvATI) {
         snprintf(symboln, sizeof(symboln), "%sGetTexBumpParameterfvATI", symbol_prefix);
         SET_GetTexBumpParameterfvATI(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexBumpParameterivATI) {
         snprintf(symboln, sizeof(symboln), "%sGetTexBumpParameterivATI", symbol_prefix);
         SET_GetTexBumpParameterivATI(disp, dlsym(handle, symboln));
    }


    if(!disp->TexBumpParameterfvATI) {
         snprintf(symboln, sizeof(symboln), "%sTexBumpParameterfvATI", symbol_prefix);
         SET_TexBumpParameterfvATI(disp, dlsym(handle, symboln));
    }


    if(!disp->TexBumpParameterivATI) {
         snprintf(symboln, sizeof(symboln), "%sTexBumpParameterivATI", symbol_prefix);
         SET_TexBumpParameterivATI(disp, dlsym(handle, symboln));
    }


    if(!disp->AlphaFragmentOp1ATI) {
         snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp1ATI", symbol_prefix);
         SET_AlphaFragmentOp1ATI(disp, dlsym(handle, symboln));
    }


    if(!disp->AlphaFragmentOp2ATI) {
         snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp2ATI", symbol_prefix);
         SET_AlphaFragmentOp2ATI(disp, dlsym(handle, symboln));
    }


    if(!disp->AlphaFragmentOp3ATI) {
         snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp3ATI", symbol_prefix);
         SET_AlphaFragmentOp3ATI(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginFragmentShaderATI) {
         snprintf(symboln, sizeof(symboln), "%sBeginFragmentShaderATI", symbol_prefix);
         SET_BeginFragmentShaderATI(disp, dlsym(handle, symboln));
    }


    if(!disp->BindFragmentShaderATI) {
         snprintf(symboln, sizeof(symboln), "%sBindFragmentShaderATI", symbol_prefix);
         SET_BindFragmentShaderATI(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorFragmentOp1ATI) {
         snprintf(symboln, sizeof(symboln), "%sColorFragmentOp1ATI", symbol_prefix);
         SET_ColorFragmentOp1ATI(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorFragmentOp2ATI) {
         snprintf(symboln, sizeof(symboln), "%sColorFragmentOp2ATI", symbol_prefix);
         SET_ColorFragmentOp2ATI(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorFragmentOp3ATI) {
         snprintf(symboln, sizeof(symboln), "%sColorFragmentOp3ATI", symbol_prefix);
         SET_ColorFragmentOp3ATI(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteFragmentShaderATI) {
         snprintf(symboln, sizeof(symboln), "%sDeleteFragmentShaderATI", symbol_prefix);
         SET_DeleteFragmentShaderATI(disp, dlsym(handle, symboln));
    }


    if(!disp->EndFragmentShaderATI) {
         snprintf(symboln, sizeof(symboln), "%sEndFragmentShaderATI", symbol_prefix);
         SET_EndFragmentShaderATI(disp, dlsym(handle, symboln));
    }


    if(!disp->GenFragmentShadersATI) {
         snprintf(symboln, sizeof(symboln), "%sGenFragmentShadersATI", symbol_prefix);
         SET_GenFragmentShadersATI(disp, dlsym(handle, symboln));
    }


    if(!disp->PassTexCoordATI) {
         snprintf(symboln, sizeof(symboln), "%sPassTexCoordATI", symbol_prefix);
         SET_PassTexCoordATI(disp, dlsym(handle, symboln));
    }


    if(!disp->SampleMapATI) {
         snprintf(symboln, sizeof(symboln), "%sSampleMapATI", symbol_prefix);
         SET_SampleMapATI(disp, dlsym(handle, symboln));
    }


    if(!disp->SetFragmentShaderConstantATI) {
         snprintf(symboln, sizeof(symboln), "%sSetFragmentShaderConstantATI", symbol_prefix);
         SET_SetFragmentShaderConstantATI(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameteriNV) {
         snprintf(symboln, sizeof(symboln), "%sPointParameteri", symbol_prefix);
         SET_PointParameteriNV(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameteriNV) {
         snprintf(symboln, sizeof(symboln), "%sPointParameteriNV", symbol_prefix);
         SET_PointParameteriNV(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterivNV) {
         snprintf(symboln, sizeof(symboln), "%sPointParameteriv", symbol_prefix);
         SET_PointParameterivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->PointParameterivNV) {
         snprintf(symboln, sizeof(symboln), "%sPointParameterivNV", symbol_prefix);
         SET_PointParameterivNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ActiveStencilFaceEXT) {
         snprintf(symboln, sizeof(symboln), "%sActiveStencilFaceEXT", symbol_prefix);
         SET_ActiveStencilFaceEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindVertexArrayAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sBindVertexArrayAPPLE", symbol_prefix);
         SET_BindVertexArrayAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteVertexArraysAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sDeleteVertexArrays", symbol_prefix);
         SET_DeleteVertexArraysAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteVertexArraysAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sDeleteVertexArraysAPPLE", symbol_prefix);
         SET_DeleteVertexArraysAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->GenVertexArraysAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sGenVertexArraysAPPLE", symbol_prefix);
         SET_GenVertexArraysAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->IsVertexArrayAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sIsVertexArray", symbol_prefix);
         SET_IsVertexArrayAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->IsVertexArrayAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sIsVertexArrayAPPLE", symbol_prefix);
         SET_IsVertexArrayAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramNamedParameterdvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramNamedParameterdvNV", symbol_prefix);
         SET_GetProgramNamedParameterdvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->GetProgramNamedParameterfvNV) {
         snprintf(symboln, sizeof(symboln), "%sGetProgramNamedParameterfvNV", symbol_prefix);
         SET_GetProgramNamedParameterfvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramNamedParameter4dNV) {
         snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4dNV", symbol_prefix);
         SET_ProgramNamedParameter4dNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramNamedParameter4dvNV) {
         snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4dvNV", symbol_prefix);
         SET_ProgramNamedParameter4dvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramNamedParameter4fNV) {
         snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4fNV", symbol_prefix);
         SET_ProgramNamedParameter4fNV(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramNamedParameter4fvNV) {
         snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4fvNV", symbol_prefix);
         SET_ProgramNamedParameter4fvNV(disp, dlsym(handle, symboln));
    }


    if(!disp->PrimitiveRestartIndexNV) {
         snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartIndexNV", symbol_prefix);
         SET_PrimitiveRestartIndexNV(disp, dlsym(handle, symboln));
    }


    if(!disp->PrimitiveRestartIndexNV) {
         snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartIndex", symbol_prefix);
         SET_PrimitiveRestartIndexNV(disp, dlsym(handle, symboln));
    }


    if(!disp->PrimitiveRestartNV) {
         snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartNV", symbol_prefix);
         SET_PrimitiveRestartNV(disp, dlsym(handle, symboln));
    }


    if(!disp->DepthBoundsEXT) {
         snprintf(symboln, sizeof(symboln), "%sDepthBoundsEXT", symbol_prefix);
         SET_DepthBoundsEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationSeparateEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparate", symbol_prefix);
         SET_BlendEquationSeparateEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationSeparateEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateEXT", symbol_prefix);
         SET_BlendEquationSeparateEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlendEquationSeparateEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateATI", symbol_prefix);
         SET_BlendEquationSeparateEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindFramebufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindFramebuffer", symbol_prefix);
         SET_BindFramebufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindFramebufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindFramebufferEXT", symbol_prefix);
         SET_BindFramebufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindRenderbufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindRenderbuffer", symbol_prefix);
         SET_BindRenderbufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindRenderbufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindRenderbufferEXT", symbol_prefix);
         SET_BindRenderbufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->CheckFramebufferStatusEXT) {
         snprintf(symboln, sizeof(symboln), "%sCheckFramebufferStatus", symbol_prefix);
         SET_CheckFramebufferStatusEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->CheckFramebufferStatusEXT) {
         snprintf(symboln, sizeof(symboln), "%sCheckFramebufferStatusEXT", symbol_prefix);
         SET_CheckFramebufferStatusEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteFramebuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sDeleteFramebuffers", symbol_prefix);
         SET_DeleteFramebuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteFramebuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sDeleteFramebuffersEXT", symbol_prefix);
         SET_DeleteFramebuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteRenderbuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sDeleteRenderbuffers", symbol_prefix);
         SET_DeleteRenderbuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->DeleteRenderbuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sDeleteRenderbuffersEXT", symbol_prefix);
         SET_DeleteRenderbuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferRenderbufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferRenderbuffer", symbol_prefix);
         SET_FramebufferRenderbufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferRenderbufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferRenderbufferEXT", symbol_prefix);
         SET_FramebufferRenderbufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture1DEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture1D", symbol_prefix);
         SET_FramebufferTexture1DEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture1DEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture1DEXT", symbol_prefix);
         SET_FramebufferTexture1DEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture2DEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture2D", symbol_prefix);
         SET_FramebufferTexture2DEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture2DEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture2DEXT", symbol_prefix);
         SET_FramebufferTexture2DEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture3DEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture3D", symbol_prefix);
         SET_FramebufferTexture3DEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTexture3DEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTexture3DEXT", symbol_prefix);
         SET_FramebufferTexture3DEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GenFramebuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sGenFramebuffers", symbol_prefix);
         SET_GenFramebuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GenFramebuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sGenFramebuffersEXT", symbol_prefix);
         SET_GenFramebuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GenRenderbuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sGenRenderbuffers", symbol_prefix);
         SET_GenRenderbuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GenRenderbuffersEXT) {
         snprintf(symboln, sizeof(symboln), "%sGenRenderbuffersEXT", symbol_prefix);
         SET_GenRenderbuffersEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GenerateMipmapEXT) {
         snprintf(symboln, sizeof(symboln), "%sGenerateMipmap", symbol_prefix);
         SET_GenerateMipmapEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GenerateMipmapEXT) {
         snprintf(symboln, sizeof(symboln), "%sGenerateMipmapEXT", symbol_prefix);
         SET_GenerateMipmapEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFramebufferAttachmentParameterivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetFramebufferAttachmentParameteriv", symbol_prefix);
         SET_GetFramebufferAttachmentParameterivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFramebufferAttachmentParameterivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetFramebufferAttachmentParameterivEXT", symbol_prefix);
         SET_GetFramebufferAttachmentParameterivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetRenderbufferParameterivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetRenderbufferParameteriv", symbol_prefix);
         SET_GetRenderbufferParameterivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetRenderbufferParameterivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetRenderbufferParameterivEXT", symbol_prefix);
         SET_GetRenderbufferParameterivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IsFramebufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sIsFramebuffer", symbol_prefix);
         SET_IsFramebufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IsFramebufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sIsFramebufferEXT", symbol_prefix);
         SET_IsFramebufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IsRenderbufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sIsRenderbuffer", symbol_prefix);
         SET_IsRenderbufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IsRenderbufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sIsRenderbufferEXT", symbol_prefix);
         SET_IsRenderbufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->RenderbufferStorageEXT) {
         snprintf(symboln, sizeof(symboln), "%sRenderbufferStorage", symbol_prefix);
         SET_RenderbufferStorageEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->RenderbufferStorageEXT) {
         snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageEXT", symbol_prefix);
         SET_RenderbufferStorageEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlitFramebufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlitFramebuffer", symbol_prefix);
         SET_BlitFramebufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BlitFramebufferEXT) {
         snprintf(symboln, sizeof(symboln), "%sBlitFramebufferEXT", symbol_prefix);
         SET_BlitFramebufferEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BufferParameteriAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sBufferParameteriAPPLE", symbol_prefix);
         SET_BufferParameteriAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->FlushMappedBufferRangeAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sFlushMappedBufferRangeAPPLE", symbol_prefix);
         SET_FlushMappedBufferRangeAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->BindFragDataLocationEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindFragDataLocationEXT", symbol_prefix);
         SET_BindFragDataLocationEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindFragDataLocationEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindFragDataLocation", symbol_prefix);
         SET_BindFragDataLocationEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFragDataLocationEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetFragDataLocationEXT", symbol_prefix);
         SET_GetFragDataLocationEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetFragDataLocationEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetFragDataLocation", symbol_prefix);
         SET_GetFragDataLocationEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformuivEXT", symbol_prefix);
         SET_GetUniformuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetUniformuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetUniformuiv", symbol_prefix);
         SET_GetUniformuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribIivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIivEXT", symbol_prefix);
         SET_GetVertexAttribIivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribIivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIiv", symbol_prefix);
         SET_GetVertexAttribIivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribIuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIuivEXT", symbol_prefix);
         SET_GetVertexAttribIuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetVertexAttribIuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIuiv", symbol_prefix);
         SET_GetVertexAttribIuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform1uiEXT", symbol_prefix);
         SET_Uniform1uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform1ui", symbol_prefix);
         SET_Uniform1uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform1uivEXT", symbol_prefix);
         SET_Uniform1uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform1uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform1uiv", symbol_prefix);
         SET_Uniform1uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform2uiEXT", symbol_prefix);
         SET_Uniform2uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform2ui", symbol_prefix);
         SET_Uniform2uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform2uivEXT", symbol_prefix);
         SET_Uniform2uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform2uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform2uiv", symbol_prefix);
         SET_Uniform2uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform3uiEXT", symbol_prefix);
         SET_Uniform3uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform3ui", symbol_prefix);
         SET_Uniform3uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform3uivEXT", symbol_prefix);
         SET_Uniform3uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform3uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform3uiv", symbol_prefix);
         SET_Uniform3uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform4uiEXT", symbol_prefix);
         SET_Uniform4uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform4ui", symbol_prefix);
         SET_Uniform4uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform4uivEXT", symbol_prefix);
         SET_Uniform4uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->Uniform4uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sUniform4uiv", symbol_prefix);
         SET_Uniform4uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1iEXT", symbol_prefix);
         SET_VertexAttribI1iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1i", symbol_prefix);
         SET_VertexAttribI1iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1ivEXT", symbol_prefix);
         SET_VertexAttribI1ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1iv", symbol_prefix);
         SET_VertexAttribI1ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uiEXT", symbol_prefix);
         SET_VertexAttribI1uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1ui", symbol_prefix);
         SET_VertexAttribI1uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uivEXT", symbol_prefix);
         SET_VertexAttribI1uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI1uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uiv", symbol_prefix);
         SET_VertexAttribI1uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2iEXT", symbol_prefix);
         SET_VertexAttribI2iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2i", symbol_prefix);
         SET_VertexAttribI2iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2ivEXT", symbol_prefix);
         SET_VertexAttribI2ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2iv", symbol_prefix);
         SET_VertexAttribI2ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uiEXT", symbol_prefix);
         SET_VertexAttribI2uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2ui", symbol_prefix);
         SET_VertexAttribI2uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uivEXT", symbol_prefix);
         SET_VertexAttribI2uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI2uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uiv", symbol_prefix);
         SET_VertexAttribI2uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3iEXT", symbol_prefix);
         SET_VertexAttribI3iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3i", symbol_prefix);
         SET_VertexAttribI3iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3ivEXT", symbol_prefix);
         SET_VertexAttribI3ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3iv", symbol_prefix);
         SET_VertexAttribI3ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uiEXT", symbol_prefix);
         SET_VertexAttribI3uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3ui", symbol_prefix);
         SET_VertexAttribI3uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uivEXT", symbol_prefix);
         SET_VertexAttribI3uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI3uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uiv", symbol_prefix);
         SET_VertexAttribI3uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4bvEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4bvEXT", symbol_prefix);
         SET_VertexAttribI4bvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4bvEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4bv", symbol_prefix);
         SET_VertexAttribI4bvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4iEXT", symbol_prefix);
         SET_VertexAttribI4iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4iEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4i", symbol_prefix);
         SET_VertexAttribI4iEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ivEXT", symbol_prefix);
         SET_VertexAttribI4ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4ivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4iv", symbol_prefix);
         SET_VertexAttribI4ivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4svEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4svEXT", symbol_prefix);
         SET_VertexAttribI4svEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4svEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4sv", symbol_prefix);
         SET_VertexAttribI4svEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4ubvEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ubvEXT", symbol_prefix);
         SET_VertexAttribI4ubvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4ubvEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ubv", symbol_prefix);
         SET_VertexAttribI4ubvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uiEXT", symbol_prefix);
         SET_VertexAttribI4uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4uiEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ui", symbol_prefix);
         SET_VertexAttribI4uiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uivEXT", symbol_prefix);
         SET_VertexAttribI4uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4uivEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uiv", symbol_prefix);
         SET_VertexAttribI4uivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4usvEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4usvEXT", symbol_prefix);
         SET_VertexAttribI4usvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribI4usvEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribI4usv", symbol_prefix);
         SET_VertexAttribI4usvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribIPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribIPointerEXT", symbol_prefix);
         SET_VertexAttribIPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->VertexAttribIPointerEXT) {
         snprintf(symboln, sizeof(symboln), "%sVertexAttribIPointer", symbol_prefix);
         SET_VertexAttribIPointerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTextureLayerEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayer", symbol_prefix);
         SET_FramebufferTextureLayerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->FramebufferTextureLayerEXT) {
         snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayerEXT", symbol_prefix);
         SET_FramebufferTextureLayerEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorMaskIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sColorMaskIndexedEXT", symbol_prefix);
         SET_ColorMaskIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ColorMaskIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sColorMaski", symbol_prefix);
         SET_ColorMaskIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->DisableIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sDisableIndexedEXT", symbol_prefix);
         SET_DisableIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->DisableIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sDisablei", symbol_prefix);
         SET_DisableIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->EnableIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sEnableIndexedEXT", symbol_prefix);
         SET_EnableIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->EnableIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sEnablei", symbol_prefix);
         SET_EnableIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBooleanIndexedvEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetBooleanIndexedvEXT", symbol_prefix);
         SET_GetBooleanIndexedvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetBooleanIndexedvEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetBooleani_v", symbol_prefix);
         SET_GetBooleanIndexedvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetIntegerIndexedvEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetIntegerIndexedvEXT", symbol_prefix);
         SET_GetIntegerIndexedvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetIntegerIndexedvEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetIntegeri_v", symbol_prefix);
         SET_GetIntegerIndexedvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IsEnabledIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sIsEnabledIndexedEXT", symbol_prefix);
         SET_IsEnabledIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->IsEnabledIndexedEXT) {
         snprintf(symboln, sizeof(symboln), "%sIsEnabledi", symbol_prefix);
         SET_IsEnabledIndexedEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearColorIiEXT) {
         snprintf(symboln, sizeof(symboln), "%sClearColorIiEXT", symbol_prefix);
         SET_ClearColorIiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ClearColorIuiEXT) {
         snprintf(symboln, sizeof(symboln), "%sClearColorIuiEXT", symbol_prefix);
         SET_ClearColorIuiEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameterIivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameterIivEXT", symbol_prefix);
         SET_GetTexParameterIivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameterIivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameterIiv", symbol_prefix);
         SET_GetTexParameterIivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameterIuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameterIuivEXT", symbol_prefix);
         SET_GetTexParameterIuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameterIuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameterIuiv", symbol_prefix);
         SET_GetTexParameterIuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameterIivEXT) {
         snprintf(symboln, sizeof(symboln), "%sTexParameterIivEXT", symbol_prefix);
         SET_TexParameterIivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameterIivEXT) {
         snprintf(symboln, sizeof(symboln), "%sTexParameterIiv", symbol_prefix);
         SET_TexParameterIivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameterIuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sTexParameterIuivEXT", symbol_prefix);
         SET_TexParameterIuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TexParameterIuivEXT) {
         snprintf(symboln, sizeof(symboln), "%sTexParameterIuiv", symbol_prefix);
         SET_TexParameterIuivEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginConditionalRenderNV) {
         snprintf(symboln, sizeof(symboln), "%sBeginConditionalRenderNV", symbol_prefix);
         SET_BeginConditionalRenderNV(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginConditionalRenderNV) {
         snprintf(symboln, sizeof(symboln), "%sBeginConditionalRender", symbol_prefix);
         SET_BeginConditionalRenderNV(disp, dlsym(handle, symboln));
    }


    if(!disp->EndConditionalRenderNV) {
         snprintf(symboln, sizeof(symboln), "%sEndConditionalRenderNV", symbol_prefix);
         SET_EndConditionalRenderNV(disp, dlsym(handle, symboln));
    }


    if(!disp->EndConditionalRenderNV) {
         snprintf(symboln, sizeof(symboln), "%sEndConditionalRender", symbol_prefix);
         SET_EndConditionalRenderNV(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginTransformFeedbackEXT) {
         snprintf(symboln, sizeof(symboln), "%sBeginTransformFeedbackEXT", symbol_prefix);
         SET_BeginTransformFeedbackEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BeginTransformFeedbackEXT) {
         snprintf(symboln, sizeof(symboln), "%sBeginTransformFeedback", symbol_prefix);
         SET_BeginTransformFeedbackEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferBaseEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindBufferBaseEXT", symbol_prefix);
         SET_BindBufferBaseEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferBaseEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindBufferBase", symbol_prefix);
         SET_BindBufferBaseEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferOffsetEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindBufferOffsetEXT", symbol_prefix);
         SET_BindBufferOffsetEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferRangeEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindBufferRangeEXT", symbol_prefix);
         SET_BindBufferRangeEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->BindBufferRangeEXT) {
         snprintf(symboln, sizeof(symboln), "%sBindBufferRange", symbol_prefix);
         SET_BindBufferRangeEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->EndTransformFeedbackEXT) {
         snprintf(symboln, sizeof(symboln), "%sEndTransformFeedbackEXT", symbol_prefix);
         SET_EndTransformFeedbackEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->EndTransformFeedbackEXT) {
         snprintf(symboln, sizeof(symboln), "%sEndTransformFeedback", symbol_prefix);
         SET_EndTransformFeedbackEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTransformFeedbackVaryingEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetTransformFeedbackVaryingEXT", symbol_prefix);
         SET_GetTransformFeedbackVaryingEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTransformFeedbackVaryingEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetTransformFeedbackVarying", symbol_prefix);
         SET_GetTransformFeedbackVaryingEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TransformFeedbackVaryingsEXT) {
         snprintf(symboln, sizeof(symboln), "%sTransformFeedbackVaryingsEXT", symbol_prefix);
         SET_TransformFeedbackVaryingsEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TransformFeedbackVaryingsEXT) {
         snprintf(symboln, sizeof(symboln), "%sTransformFeedbackVaryings", symbol_prefix);
         SET_TransformFeedbackVaryingsEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ProvokingVertexEXT) {
         snprintf(symboln, sizeof(symboln), "%sProvokingVertexEXT", symbol_prefix);
         SET_ProvokingVertexEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ProvokingVertexEXT) {
         snprintf(symboln, sizeof(symboln), "%sProvokingVertex", symbol_prefix);
         SET_ProvokingVertexEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetTexParameterPointervAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sGetTexParameterPointervAPPLE", symbol_prefix);
         SET_GetTexParameterPointervAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->TextureRangeAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sTextureRangeAPPLE", symbol_prefix);
         SET_TextureRangeAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->GetObjectParameterivAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sGetObjectParameterivAPPLE", symbol_prefix);
         SET_GetObjectParameterivAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->ObjectPurgeableAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sObjectPurgeableAPPLE", symbol_prefix);
         SET_ObjectPurgeableAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->ObjectUnpurgeableAPPLE) {
         snprintf(symboln, sizeof(symboln), "%sObjectUnpurgeableAPPLE", symbol_prefix);
         SET_ObjectUnpurgeableAPPLE(disp, dlsym(handle, symboln));
    }


    if(!disp->ActiveProgramEXT) {
         snprintf(symboln, sizeof(symboln), "%sActiveProgramEXT", symbol_prefix);
         SET_ActiveProgramEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->CreateShaderProgramEXT) {
         snprintf(symboln, sizeof(symboln), "%sCreateShaderProgramEXT", symbol_prefix);
         SET_CreateShaderProgramEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->UseShaderProgramEXT) {
         snprintf(symboln, sizeof(symboln), "%sUseShaderProgramEXT", symbol_prefix);
         SET_UseShaderProgramEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->TextureBarrierNV) {
         snprintf(symboln, sizeof(symboln), "%sTextureBarrierNV", symbol_prefix);
         SET_TextureBarrierNV(disp, dlsym(handle, symboln));
    }


    if(!disp->StencilFuncSeparateATI) {
         snprintf(symboln, sizeof(symboln), "%sStencilFuncSeparateATI", symbol_prefix);
         SET_StencilFuncSeparateATI(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramEnvParameters4fvEXT) {
         snprintf(symboln, sizeof(symboln), "%sProgramEnvParameters4fvEXT", symbol_prefix);
         SET_ProgramEnvParameters4fvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->ProgramLocalParameters4fvEXT) {
         snprintf(symboln, sizeof(symboln), "%sProgramLocalParameters4fvEXT", symbol_prefix);
         SET_ProgramLocalParameters4fvEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryObjecti64vEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryObjecti64vEXT", symbol_prefix);
         SET_GetQueryObjecti64vEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->GetQueryObjectui64vEXT) {
         snprintf(symboln, sizeof(symboln), "%sGetQueryObjectui64vEXT", symbol_prefix);
         SET_GetQueryObjectui64vEXT(disp, dlsym(handle, symboln));
    }


    if(!disp->EGLImageTargetRenderbufferStorageOES) {
         snprintf(symboln, sizeof(symboln), "%sEGLImageTargetRenderbufferStorageOES", symbol_prefix);
         SET_EGLImageTargetRenderbufferStorageOES(disp, dlsym(handle, symboln));
    }


    if(!disp->EGLImageTargetTexture2DOES) {
         snprintf(symboln, sizeof(symboln), "%sEGLImageTargetTexture2DOES", symbol_prefix);
         SET_EGLImageTargetTexture2DOES(disp, dlsym(handle, symboln));
    }


   return disp;
}

