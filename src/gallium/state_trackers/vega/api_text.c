/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "VG/openvg.h"

#include "vg_context.h"

#include "util/u_memory.h"

#ifdef OPENVG_VERSION_1_1

struct vg_font {
   struct vg_object base;

   VGint glyph_indices[200];
   VGint num_glyphs;
};

VGFont vgCreateFont(VGint glyphCapacityHint)
{
   struct vg_font *font = 0;
   struct vg_context *ctx = vg_current_context();

   if (glyphCapacityHint < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   font = CALLOC_STRUCT(vg_font);
   vg_init_object(&font->base, ctx, VG_OBJECT_FONT);
   vg_context_add_object(ctx, VG_OBJECT_FONT, font);
   return (VGFont)font;
}

void vgDestroyFont(VGFont f)
{
   struct vg_font *font = (struct vg_font *)f;
   struct vg_context *ctx = vg_current_context();

   if (f == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   vg_context_remove_object(ctx, VG_OBJECT_FONT, font);
   /*free(font);*/
}

void vgSetGlyphToPath(VGFont font,
                      VGuint glyphIndex,
                      VGPath path,
                      VGboolean isHinted,
                      VGfloat glyphOrigin [2],
                      VGfloat escapement[2])
{
   struct vg_context *ctx = vg_current_context();
   struct vg_object *pathObj;
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_FONT, (void *)font)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!glyphOrigin || !escapement ||
       !is_aligned(glyphOrigin) || !is_aligned(escapement)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (path != VG_INVALID_HANDLE &&
       !vg_context_is_object_valid(ctx, VG_OBJECT_PATH, (void *)path)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   pathObj = (struct vg_object*)path;
   if (pathObj && pathObj->type != VG_OBJECT_PATH) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   f = (struct vg_font*)font;
   f->glyph_indices[f->num_glyphs] = glyphIndex;
   ++f->num_glyphs;
}

void vgSetGlyphToImage(VGFont font,
                       VGuint glyphIndex,
                       VGImage image,
                       VGfloat glyphOrigin [2],
                       VGfloat escapement[2])
{
   struct vg_context *ctx = vg_current_context();
   struct vg_object *img_obj;
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_FONT, (void *)font)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!glyphOrigin || !escapement ||
       !is_aligned(glyphOrigin) || !is_aligned(escapement)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (image != VG_INVALID_HANDLE &&
       !vg_context_is_object_valid(ctx, VG_OBJECT_IMAGE, (void *)image)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   img_obj = (struct vg_object*)image;
   if (img_obj && img_obj->type != VG_OBJECT_IMAGE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   f = (struct vg_font*)font;
   f->glyph_indices[f->num_glyphs] = glyphIndex;
   ++f->num_glyphs;
}

static INLINE VGboolean font_contains_glyph(struct vg_font *font,
                                            VGuint glyph_index)
{
   VGint i;
   for (i = 0; i < font->num_glyphs; ++i) {
      if (font->glyph_indices[i] == glyph_index) {
         return VG_TRUE;
      }
   }
   return VG_FALSE;
}

void vgClearGlyph(VGFont font,
                  VGuint glyphIndex)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_font *f;
   VGint i;

   if (font == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (glyphIndex <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   f = (struct vg_font*)font;
   if (!font_contains_glyph(f, glyphIndex)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   for (i = 0; i < f->num_glyphs; ++i) {
      if (f->glyph_indices[i] == glyphIndex) {
         /*FIXME*/
         f->glyph_indices[f->num_glyphs] = 0;
         --f->num_glyphs;
         return;
      }
   }
}

void vgDrawGlyph(VGFont font,
                 VGuint glyphIndex,
                 VGbitfield paintModes,
                 VGboolean allowAutoHinting)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (glyphIndex <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (paintModes & (~(VG_STROKE_PATH|VG_FILL_PATH))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   f = (struct vg_font*)font;
   if (!font_contains_glyph(f, glyphIndex)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
}

void vgDrawGlyphs(VGFont font,
                  VGint glyphCount,
                  VGuint *glyphIndices,
                  VGfloat *adjustments_x,
                  VGfloat *adjustments_y,
                  VGbitfield paintModes,
                  VGboolean allowAutoHinting)
{
   struct vg_context *ctx = vg_current_context();
   VGint i;
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (glyphCount <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!glyphIndices || !is_aligned(glyphIndices)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!adjustments_x || !is_aligned(adjustments_x) ||
       !adjustments_y || !is_aligned(adjustments_y)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (paintModes & (~(VG_STROKE_PATH|VG_FILL_PATH))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   f = (struct vg_font*)font;
   for (i = 0; i < glyphCount; ++i) {
      VGuint glyph_index = glyphIndices[i];
      if (!font_contains_glyph(f, glyph_index)) {
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
         return;
      }
   }
}

#endif
