/*
 * FxGLUT version 0.12 - GLUT for Voodoo 1 and 2 under Linux
 * Copyright (C) 1999 Christopher John Purnell
 *                    cjp@lost.org.uk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "internal.h"


void
glutStrokeCharacter (void *font, int c)
{
   const GLUTStrokeFont *sfp = _glut_font(font);
   const GLUTStrokeChar *scp;
   const GLUTStrokeStrip *ssp;
   const GLUTStrokeVertex *svp;
   unsigned i, j;

   if (((unsigned)c) >= sfp->num || !(scp = sfp->table[c]))
      return;

   ssp = scp->strip;

   for (i = 0; i < scp->num; i++, ssp++) {
      svp = ssp->vertex;

      glBegin(GL_LINE_STRIP);
      for (j = 0; j < ssp->num; j++, svp++) {
         glVertex2f(svp->x, svp->y);
      }
      glEnd();
   }

   glTranslatef(scp->right, 0.0, 0.0);
}


void
glutStrokeString (void *font, const unsigned char *string)
{
   const GLUTStrokeFont *sfp = _glut_font(font);
   const GLUTStrokeChar *scp;
   const GLUTStrokeStrip *ssp;
   const GLUTStrokeVertex *svp;
   unsigned char c;
   unsigned i, j;

   while ((c = *(string++))) {
      if (c < sfp->num && (scp = sfp->table[c])) {
         ssp = scp->strip;

         for (i = 0; i < scp->num; i++, ssp++) {
            svp = ssp->vertex;

            glBegin(GL_LINE_STRIP);
            for (j = 0; j < ssp->num; j++, svp++) {
               glVertex2f(svp->x, svp->y);
            }
            glEnd();
         }

         glTranslatef(scp->right, 0.0, 0.0);
      }
   }
}


int
glutStrokeWidth (void *font, int c)
{
   const GLUTStrokeFont *sfp = _glut_font(font);
   const GLUTStrokeChar *scp;

   if (((unsigned)c) >= sfp->num || !(scp = sfp->table[c]))
      return 0;

   return scp->right;
}


int
glutStrokeLength (void *font, const unsigned char *string)
{
   const GLUTStrokeFont *sfp = _glut_font(font);
   const GLUTStrokeChar *scp;
   unsigned char c;
   int length = 0;

   while ((c = *(string++))) {
      if (c < sfp->num && (scp = sfp->table[c]))
         length += scp->right;
   }

   return length;
}


GLfloat
glutStrokeHeight (void *font)
{
   const GLUTStrokeFont *sfp = _glut_font(font);

   return sfp->height;
}
