/* Copyright (c) Mark J. Kilgard, 1994, 1998. 

   This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include "internal.h"
#include "font.h"


#if defined(_WIN32) || defined (GLUT_IMPORT_LIB)

static inline void*
__glutFont( void *font )
{
     switch((long)font) {
          case (long)GLUT_STROKE_ROMAN:
               return &glutStrokeRoman;
          case (long)GLUT_STROKE_MONO_ROMAN:
               return &glutStrokeMonoRoman;
          case (long)GLUT_BITMAP_9_BY_15:
               return &glutBitmap9By15;
          case (long)GLUT_BITMAP_8_BY_13:
               return &glutBitmap8By13;
          case (long)GLUT_BITMAP_TIMES_ROMAN_10:
               return &glutBitmapTimesRoman10;
          case (long)GLUT_BITMAP_TIMES_ROMAN_24:
               return &glutBitmapTimesRoman24;
          case (long)GLUT_BITMAP_HELVETICA_10:
               return &glutBitmapHelvetica10;
          case (long)GLUT_BITMAP_HELVETICA_12:
               return &glutBitmapHelvetica12;
          case (long)GLUT_BITMAP_HELVETICA_18:
               return &glutBitmapHelvetica18;
     }

     return NULL; 
}

#else

static inline void*
__glutFont( void *font )
{
     return font;
}

#endif


void GLUTAPIENTRY 
glutBitmapCharacter( GLUTbitmapFont font, int c )
{
     const BitmapCharRec *ch;
     BitmapFontPtr fontinfo;
     GLint swapbytes, lsbfirst, rowlength;
     GLint skiprows, skippixels, alignment;

     fontinfo = (BitmapFontPtr) __glutFont( font );

     if (!fontinfo || c < fontinfo->first ||
         c >= fontinfo->first + fontinfo->num_chars)
          return;
  
     ch = fontinfo->ch[c - fontinfo->first];
     if (ch) {
          /* Save current modes. */
          glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
          glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
          glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
          glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
          glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
          glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
          /* Little endian machines (DEC Alpha for example) could
             benefit from setting GL_UNPACK_LSB_FIRST to GL_TRUE
             instead of GL_FALSE, but this would require changing the
             generated bitmaps too. */
          glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
          glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
          glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
          glBitmap(ch->width, ch->height, ch->xorig, ch->yorig,
                   ch->advance, 0, ch->bitmap);
          /* Restore saved modes. */
          glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
          glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
          glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
          glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
          glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
     }
}


int GLUTAPIENTRY 
glutBitmapWidth( GLUTbitmapFont font, int c )
{
     BitmapFontPtr fontinfo;
     const BitmapCharRec *ch;

     fontinfo = (BitmapFontPtr) __glutFont( font );

     if (!fontinfo || c < fontinfo->first || 
         c >= fontinfo->first + fontinfo->num_chars)
          return 0;
  
     ch = fontinfo->ch[c - fontinfo->first];
     if (ch)
          return ch->advance;

     return 0;
}


int GLUTAPIENTRY 
glutBitmapLength( GLUTbitmapFont font, const unsigned char *string )
{
     int c, length;
     BitmapFontPtr fontinfo;
     const BitmapCharRec *ch;

     fontinfo = (BitmapFontPtr) __glutFont( font );
     if (!fontinfo)
          return 0;

     for (length = 0; *string != '\0'; string++) {
          c = *string;
          if (c >= fontinfo->first &&
              c < fontinfo->first + fontinfo->num_chars) {
               ch = fontinfo->ch[c - fontinfo->first];
               if (ch)
                    length += ch->advance;
          }
     }
  
     return length;
}


void GLUTAPIENTRY 
glutStrokeCharacter( GLUTstrokeFont font, int c )
{
     const StrokeCharRec *ch;
     const StrokeRec *stroke;
     const CoordRec *coord;
     StrokeFontPtr fontinfo;
     int i, j;

     fontinfo = (StrokeFontPtr) __glutFont( font );

     if (!fontinfo || c < 0 || c >= fontinfo->num_chars)
          return;
  
     ch = &(fontinfo->ch[c]);
     if (ch) {
          for (i = ch->num_strokes, stroke = ch->stroke;
               i > 0; i--, stroke++) {
               glBegin(GL_LINE_STRIP);
               for (j = stroke->num_coords, coord = stroke->coord;
                    j > 0; j--, coord++) {
                    glVertex2f(coord->x, coord->y);
               }
               glEnd();
          }
          glTranslatef(ch->right, 0.0, 0.0);
     }
}


int GLUTAPIENTRY 
glutStrokeWidth( GLUTstrokeFont font, int c )
{
     StrokeFontPtr fontinfo;
     const StrokeCharRec *ch;

     fontinfo = (StrokeFontPtr) __glutFont( font );

     if (!fontinfo || c < 0 || c >= fontinfo->num_chars)
          return 0;
  
     ch = &(fontinfo->ch[c]);
     if (ch)
          return ch->right;
          
     return 0;
}


int GLUTAPIENTRY 
glutStrokeLength( GLUTstrokeFont font, const unsigned char *string )
{
     int c, length;
     StrokeFontPtr fontinfo;
     const StrokeCharRec *ch;

     fontinfo = (StrokeFontPtr) __glutFont( font );
     if (!fontinfo)
          return 0;

     for (length = 0; *string != '\0'; string++) {
          c = *string;
          if (c >= 0 && c < fontinfo->num_chars) {
               ch = &(fontinfo->ch[c]);
               if (ch)
                    length += ch->right;
          }
     }
  
     return length;
}

